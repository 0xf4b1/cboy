// SPDX-License-Identifier: GPL-3.0-only
// Metal renderer — macOS / Apple Silicon native GPU API.
//
// Pipeline:
//   MTKView (MetalKit) provides the CAMetalLayer and render loop callbacks.
//   Each GB frame is uploaded to a private MTLTexture via a staging buffer,
//   then drawn as a full-screen textured quad using a minimal vertex/fragment
//   shader pair.  A letterboxed viewport preserves the 160:144 aspect ratio.
//
// The MTKViewDelegate drives the draw loop at the display refresh rate;
// the GB emulator runs one frame per draw call (targeting 60 fps on 60 Hz
// displays, or every other draw call on 120 Hz ProMotion displays).

#include "gameboy.hpp"
#include "renderer_metal.hpp"
#include "controls.hpp"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

static const uint32_t GB_W = 160;
static const uint32_t GB_H = 144;

// ---- Color conversion ----------------------------------------------------

static inline uint32_t rgb555_to_bgra(uint16_t c) {
    // Metal MTLPixelFormatBGRA8Unorm: B in byte 0, G byte 1, R byte 2, A byte 3
    uint8_t r = static_cast<uint8_t>(((c >>  0) & 0x1F) * 255 / 31);
    uint8_t g = static_cast<uint8_t>(((c >>  5) & 0x1F) * 255 / 31);
    uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
    return (0xFFu << 24) | (static_cast<uint32_t>(r) << 16)
                         | (static_cast<uint32_t>(g) <<  8)
                         |  static_cast<uint32_t>(b);
}

// ---- Input ---------------------------------------------------------------

static bool ns_key_to_button(unsigned short key_code, cboy::Button &out) {
    // macOS virtual key codes (kVK_* from Carbon/HIToolbox)
    switch (key_code) {
    case 0x7C: out = cboy::Button::RIGHT;  return true; // kVK_RightArrow
    case 0x7B: out = cboy::Button::LEFT;   return true; // kVK_LeftArrow
    case 0x7E: out = cboy::Button::UP;     return true; // kVK_UpArrow
    case 0x7D: out = cboy::Button::DOWN;   return true; // kVK_DownArrow
    case 0x00: out = cboy::Button::A;      return true; // kVK_ANSI_A
    case 0x01: out = cboy::Button::B;      return true; // kVK_ANSI_S
    case 0x0C: out = cboy::Button::START;  return true; // kVK_ANSI_Q
    case 0x0D: out = cboy::Button::SELECT; return true; // kVK_ANSI_W
    default:   return false;
    }
}

// ---- Metal shaders (inline MSL) -----------------------------------------

static NSString *kShaderSrc = @R"MSL(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

// Full-screen quad from vertex ID — no vertex buffer needed.
vertex VertexOut vert(uint vid [[vertex_id]]) {
    // NDC positions and UVs for a triangle strip covering the viewport
    constexpr float2 positions[4] = {
        {-1.0,  1.0}, { 1.0,  1.0},
        {-1.0, -1.0}, { 1.0, -1.0}
    };
    constexpr float2 uvs[4] = {
        {0.0, 0.0}, {1.0, 0.0},
        {0.0, 1.0}, {1.0, 1.0}
    };
    VertexOut out;
    out.position = float4(positions[vid], 0.0, 1.0);
    out.uv       = uvs[vid];
    return out;
}

fragment float4 frag(VertexOut in [[stage_in]],
                     texture2d<float> tex [[texture(0)]]) {
    constexpr sampler s(filter::nearest);
    return tex.sample(s, in.uv);
}
)MSL";

// ---- Delegate ------------------------------------------------------------

@interface CBoyMTKDelegate : NSObject <MTKViewDelegate>
@property (nonatomic, assign) cboy::Gameboy   *gameboy;
@property (nonatomic, strong) id<MTLDevice>    device;
@property (nonatomic, strong) id<MTLCommandQueue> queue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipeline;
@property (nonatomic, strong) id<MTLTexture>   gbTexture;
@property (nonatomic, strong) id<MTLBuffer>    stagingBuf;
@end

@implementation CBoyMTKDelegate

- (instancetype)initWithDevice:(id<MTLDevice>)dev gameboy:(cboy::Gameboy *)gb {
    self = [super init];
    if (!self) return nil;
    self.device  = dev;
    self.gameboy = gb;
    self.queue   = [dev newCommandQueue];

    // Compile shaders
    NSError *err = nil;
    id<MTLLibrary> lib = [dev newLibraryWithSource:kShaderSrc
                                           options:nil
                                             error:&err];
    if (!lib) {
        NSLog(@"Metal shader compile error: %@", err);
        return nil;
    }

    MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
    pd.vertexFunction               = [lib newFunctionWithName:@"vert"];
    pd.fragmentFunction             = [lib newFunctionWithName:@"frag"];
    pd.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    self.pipeline = [dev newRenderPipelineStateWithDescriptor:pd error:&err];
    if (!self.pipeline) {
        NSLog(@"Metal pipeline error: %@", err);
        return nil;
    }

    // 160×144 BGRA8 texture updated every frame
    MTLTextureDescriptor *td = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:GB_W
                                    height:GB_H
                                 mipmapped:NO];
    td.usage       = MTLTextureUsageShaderRead;
    td.storageMode = MTLStorageModePrivate; // GPU-only, fastest sampling

    self.gbTexture = [dev newTextureWithDescriptor:td];

    // Shared staging buffer — CPU writes here, we blit to the private texture
    NSUInteger buf_size = GB_W * GB_H * 4;
    self.stagingBuf = [dev newBufferWithLength:buf_size
                                       options:MTLResourceStorageModeShared];
    return self;
}

- (void)drawInMTKView:(MTKView *)view {
    cboy::Gameboy *gb = self.gameboy;

    // Run one GB frame and convert to BGRA8 in the staging buffer
    const cboy::display::Frame &frame = gb->run_frame();
    auto *dst = static_cast<uint32_t *>(self.stagingBuf.contents);
    for (uint32_t y = 0; y < GB_H; ++y)
        for (uint32_t x = 0; x < GB_W; ++x)
            dst[y * GB_W + x] = rgb555_to_bgra(frame[y][x]);

    id<MTLCommandBuffer> cmd = [self.queue commandBuffer];

    // Blit staging → private texture
    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    [blit copyFromBuffer:self.stagingBuf
            sourceOffset:0
       sourceBytesPerRow:GB_W * 4
     sourceBytesPerImage:GB_W * GB_H * 4
              sourceSize:MTLSizeMake(GB_W, GB_H, 1)
               toTexture:self.gbTexture
        destinationSlice:0
        destinationLevel:0
       destinationOrigin:MTLOriginMake(0, 0, 0)];
    [blit endEncoding];

    // Render pass — letterbox viewport
    MTLRenderPassDescriptor *rpd = view.currentRenderPassDescriptor;
    if (!rpd) { [cmd commit]; return; }

    // Clear to black (fills letterbox/pillarbox bars)
    rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0, 0, 0, 1);
    rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
    rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
    [enc setRenderPipelineState:self.pipeline];

    // Compute letterboxed viewport within the drawable
    CGSize drawable_size = view.drawableSize;
    double dw = drawable_size.width;
    double dh = drawable_size.height;
    double vp_w, vp_h, vp_x, vp_y;
    if (dw * GB_H <= dh * GB_W) {
        vp_w = dw;
        vp_h = dw * GB_H / GB_W;
    } else {
        vp_h = dh;
        vp_w = dh * GB_W / GB_H;
    }
    vp_x = (dw - vp_w) * 0.5;
    vp_y = (dh - vp_h) * 0.5;

    MTLViewport vp = {vp_x, vp_y, vp_w, vp_h, 0.0, 1.0};
    [enc setViewport:vp];

    [enc setFragmentTexture:self.gbTexture atIndex:0];
    [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip
           vertexStart:0
           vertexCount:4];
    [enc endEncoding];

    [cmd presentDrawable:view.currentDrawable];
    [cmd commit];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Nothing to do — viewport is recomputed from drawableSize each frame.
    (void)view; (void)size;
}

@end

// ---- Window delegate (quit on close) ------------------------------------

@interface CBoyWindowDelegate : NSObject <NSWindowDelegate>
@end
@implementation CBoyWindowDelegate
- (void)windowWillClose:(NSNotification *)n {
    (void)n;
    [NSApp stop:nil];
}
- (void)windowDidBecomeKey:(NSNotification *)n {
    (void)n;
}
// Forward key events from the window to the Metal view
@end

// ---- Custom NSView for key input ----------------------------------------

@interface CBoyKeyView : MTKView
@property (nonatomic, assign) cboy::Gameboy *gameboy;
@end

@implementation CBoyKeyView
- (BOOL)acceptsFirstResponder { return YES; }
- (void)keyDown:(NSEvent *)event {
    cboy::Button btn;
    if (!event.isARepeat && ns_key_to_button(event.keyCode, btn))
        self.gameboy->controls().press(btn);
    // Don't call super — prevents the beep on unhandled keys
}
- (void)keyUp:(NSEvent *)event {
    if (event.keyCode == 96)  { self.gameboy->load_state(); return; } // F5
    if (event.keyCode == 97)  { self.gameboy->save_state(); return; } // F6
    cboy::Button btn;
    if (ns_key_to_button(event.keyCode, btn))
        self.gameboy->controls().release(btn);
}
@end

// ---- Renderer entry point -----------------------------------------------

namespace cboy {
namespace renderer {

void MetalRenderer::run(Gameboy &gameboy) {
    // Create the NSApplication if we're not already running in one
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device)
        throw std::runtime_error("Metal not supported on this device");

    // Window
    NSRect frame = NSMakeRect(0, 0, GB_W * 4, GB_H * 4);
    NSWindowStyleMask style = NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskResizable
                            | NSWindowStyleMaskMiniaturizable;
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.title = @"cboy — Metal";

    // MTKView
    CBoyKeyView *view     = [[CBoyKeyView alloc] initWithFrame:frame device:device];
    view.gameboy          = &gameboy;
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    view.clearColor       = MTLClearColorMake(0, 0, 0, 1);
    // preferredFramesPerSecond defaults to 60; MTKView calls drawInMTKView: at that rate
    view.preferredFramesPerSecond = 60;
    view.enableSetNeedsDisplay    = NO; // timer-driven, not event-driven
    view.paused                   = NO;

    CBoyMTKDelegate *delegate = [[CBoyMTKDelegate alloc] initWithDevice:device
                                                                 gameboy:&gameboy];
    view.delegate = delegate;

    CBoyWindowDelegate *win_delegate = [CBoyWindowDelegate new];
    window.delegate       = win_delegate;
    window.contentView    = view;

    [window center];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:view];
    [NSApp activateIgnoringOtherApps:YES];

    [NSApp run]; // blocks until windowWillClose calls [NSApp stop:]
}

std::unique_ptr<IRenderer> create() {
    return std::make_unique<MetalRenderer>();
}

} // namespace renderer
} // namespace cboy
