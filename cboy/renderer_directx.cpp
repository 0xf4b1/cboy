// SPDX-License-Identifier: GPL-3.0-only
// Direct3D 11 renderer.

#include "gameboy.hpp"
#include "renderer_directx.hpp"
#include "controls.hpp"
#include "frame_pacer.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace cboy {
namespace renderer {

static const int GB_W  = 160;
static const int GB_H  = 144;
static const int SCALE = 4;

// Vertex shader: full-screen quad from a vertex buffer.
// Matches the input layout: POSITION (float3) + TEXCOORD (float2).
static const char *k_vs = R"(
    struct VS_INPUT {
        float3 position : POSITION;
        float2 texCoord : TEXCOORD0;
    };
    struct PS_INPUT {
        float4 position : SV_POSITION;
        float2 texCoord : TEXCOORD0;
    };
    PS_INPUT main(VS_INPUT input) {
        PS_INPUT output;
        output.position = float4(input.position, 1.0f);
        output.texCoord = input.texCoord;
        return output;
    }
)";

// Pixel shader: sample the GB framebuffer texture.
static const char *k_ps = R"(
    Texture2D objTexture : register(t0);
    SamplerState objSamplerState : register(s0);
    struct PS_INPUT {
        float4 position : SV_POSITION;
        float2 texCoord : TEXCOORD0;
    };
    float4 main(PS_INPUT input) : SV_TARGET {
        return objTexture.Sample(objSamplerState, input.texCoord);
    }
)";

struct Vertex {
    float position[3];
    float texCoord[2];
};

// Map Win32 VK → GB Button.
static bool vkey_to_button(int vkey, Button &out) {
    switch (vkey) {
    case VK_RIGHT: out = Button::RIGHT;  return true;
    case VK_LEFT:  out = Button::LEFT;   return true;
    case VK_UP:    out = Button::UP;     return true;
    case VK_DOWN:  out = Button::DOWN;   return true;
    case 'A':      out = Button::A;      return true;
    case 'S':      out = Button::B;      return true;
    case 'Q':      out = Button::START;  return true;
    case 'W':      out = Button::SELECT; return true;
    default:       return false;
    }
}

struct DXState {
    Gameboy *gameboy    = nullptr;
    bool     running    = true;
    bool     resizing   = false; // guard against re-entrant resize during init

    ComPtr<IDXGISwapChain>           swap_chain;
    ComPtr<ID3D11Device>             device;
    ComPtr<ID3D11DeviceContext>      ctx;
    ComPtr<ID3D11RenderTargetView>   rtv;
    ComPtr<ID3D11Texture2D>          tex;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState>       sampler;
    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11InputLayout>        input_layout;
    ComPtr<ID3D11Buffer>             vertex_buffer;
    HWND hwnd = nullptr;
};

static void recreate_rtv(DXState &s) {
    s.ctx->OMSetRenderTargets(0, nullptr, nullptr);
    s.rtv.Reset();
    ComPtr<ID3D11Texture2D> back;
    if (SUCCEEDED(s.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back))))
        s.device->CreateRenderTargetView(back.Get(), nullptr, &s.rtv);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto *s = reinterpret_cast<DXState *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_SIZE:
        // Only resize when fully initialised and not already resizing
        if (s && !s->resizing && s->swap_chain && s->device &&
            LOWORD(lp) > 0 && HIWORD(lp) > 0) {
            s->swap_chain->ResizeBuffers(1, LOWORD(lp), HIWORD(lp),
                                         DXGI_FORMAT_R8G8B8A8_UNORM, 0);
            recreate_rtv(*s);
        }
        return 0;
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (s && s->gameboy) {
            if (wp == VK_F5 && msg == WM_KEYUP) { s->gameboy->load_state(); break; }
            if (wp == VK_F6 && msg == WM_KEYUP) { s->gameboy->save_state(); break; }
            Button btn;
            if (vkey_to_button(static_cast<int>(wp), btn)) {
                if (msg == WM_KEYDOWN) s->gameboy->controls().press(btn);
                else                   s->gameboy->controls().release(btn);
            }
        }
        return 0;
    case WM_DESTROY:
        if (s) s->running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static ComPtr<ID3DBlob> compile_shader(const char *src, const char *target) {
    ComPtr<ID3DBlob> blob, err;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr,
                             "main", target, 0, 0, &blob, &err);
    if (FAILED(hr)) {
        if (err) fprintf(stderr, "Shader error: %s\n",
                         static_cast<char *>(err->GetBufferPointer()));
        throw std::runtime_error("Shader compilation failed");
    }
    return blob;
}

void DirectXRenderer::run(Gameboy &gameboy) {
    DXState s;
    s.gameboy  = &gameboy;
    s.resizing = true; // block WM_SIZE handling during init

    // --- Window ---
    const wchar_t CLASS[] = L"cboy_dx";
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = wnd_proc;
    wc.lpszClassName = CLASS;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    RECT r = {0, 0, GB_W * SCALE, GB_H * SCALE};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    s.hwnd = CreateWindowExW(0, CLASS, L"cboy",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             r.right - r.left, r.bottom - r.top,
                             nullptr, nullptr, nullptr, nullptr);
    if (!s.hwnd) throw std::runtime_error("CreateWindowExW failed");

    // Attach state pointer before ShowWindow triggers any messages
    SetWindowLongPtrW(s.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&s));

    // --- Device + Swap chain ---
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount              = 1;
    sd.BufferDesc.Width         = static_cast<UINT>(GB_W * SCALE);
    sd.BufferDesc.Height        = static_cast<UINT>(GB_H * SCALE);
    sd.BufferDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate   = {60, 1};
    sd.BufferUsage              = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow             = s.hwnd;
    sd.SampleDesc.Count         = 1;
    sd.Windowed                 = TRUE;

    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        &fl, 1, D3D11_SDK_VERSION,
        &sd, &s.swap_chain, &s.device, nullptr, &s.ctx);
    if (FAILED(hr)) throw std::runtime_error("D3D11CreateDeviceAndSwapChain failed");

    // --- Shaders ---
    auto vs_blob = compile_shader(k_vs, "vs_5_0");
    auto ps_blob = compile_shader(k_ps, "ps_5_0");
    s.device->CreateVertexShader(vs_blob->GetBufferPointer(),
                                  vs_blob->GetBufferSize(), nullptr, &s.vs);
    s.device->CreatePixelShader(ps_blob->GetBufferPointer(),
                                 ps_blob->GetBufferSize(), nullptr, &s.ps);

    // --- Input layout ---
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    s.device->CreateInputLayout(layout, 2,
                                 vs_blob->GetBufferPointer(),
                                 vs_blob->GetBufferSize(),
                                 &s.input_layout);

    // --- Vertex buffer: two triangles covering NDC clip space ---
    // UV (0,0)=top-left, (1,1)=bottom-right; Y is flipped so top of texture = top of screen.
    Vertex verts[] = {
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    };
    D3D11_BUFFER_DESC bd = {};
    bd.Usage     = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(verts);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vd = {verts};
    s.device->CreateBuffer(&bd, &vd, &s.vertex_buffer);

    // --- GB framebuffer texture (RGBA8, dynamic CPU upload each frame) ---
    D3D11_TEXTURE2D_DESC td = {};
    td.Width            = GB_W;
    td.Height           = GB_H;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DYNAMIC;
    td.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    s.device->CreateTexture2D(&td, nullptr, &s.tex);
    s.device->CreateShaderResourceView(s.tex.Get(), nullptr, &s.srv);

    // Point/nearest sampler — correct for pixel-art
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samp.MaxLOD = D3D11_FLOAT32_MAX;
    s.device->CreateSamplerState(&samp, &s.sampler);

    // Get actual client size after the window is shown (may differ from requested)
    RECT init_cr; GetClientRect(s.hwnd, &init_cr);
    UINT init_w = static_cast<UINT>(init_cr.right);
    UINT init_h = static_cast<UINT>(init_cr.bottom);
    if (init_w > 0 && init_h > 0) {
        s.swap_chain->ResizeBuffers(1, init_w, init_h,
                                    DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        recreate_rtv(s);
    }
    s.resizing = false; // allow WM_SIZE to handle future resizes
    ShowWindow(s.hwnd, SW_SHOWNORMAL);
    UpdateWindow(s.hwnd);

    // --- Main loop ---
    FramePacer pacer;
    MSG msg = {};
    while (s.running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { s.running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!s.running) break;

        // Run one GB frame
        const display::Frame &frame = gameboy.run_frame();
        pacer.wait();

        // Upload pixels to the texture: GB RGB555 → RGBA8
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(s.ctx->Map(s.tex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            auto     *dst   = static_cast<uint32_t *>(mapped.pData);
            uint32_t  pitch = mapped.RowPitch / 4;
            for (int y = 0; y < GB_H; ++y) {
                for (int x = 0; x < GB_W; ++x) {
                    uint16_t c  = frame[y][x];
                    uint8_t  r  = static_cast<uint8_t>(((c >>  0) & 0x1F) * 255 / 31);
                    uint8_t  g  = static_cast<uint8_t>(((c >>  5) & 0x1F) * 255 / 31);
                    uint8_t  b  = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
                    dst[y * pitch + x] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
                }
            }
            s.ctx->Unmap(s.tex.Get(), 0);
        }

        // Draw — letterbox the 160×144 content, maintaining aspect ratio.
        // Query the actual swap chain buffer size — this is what the viewport
        // coordinates are relative to, regardless of the OS window size.
        ComPtr<ID3D11Texture2D> back_buf;
        s.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buf));
        D3D11_TEXTURE2D_DESC back_desc;
        back_buf->GetDesc(&back_desc);
        int buf_w = static_cast<int>(back_desc.Width);
        int buf_h = static_cast<int>(back_desc.Height);

        // Largest centered rect at 160:144 that fits the buffer
        int vp_w, vp_h;
        if (buf_w * GB_H <= buf_h * GB_W) {
            // Buffer taller than GB ratio → pillarbox (bars top/bottom)
            vp_w = buf_w;
            vp_h = buf_w * GB_H / GB_W;
        } else {
            // Buffer wider than GB ratio → letterbox (bars left/right)
            vp_h = buf_h;
            vp_w = buf_h * GB_W / GB_H;
        }
        int vp_x = (buf_w - vp_w) / 2;
        int vp_y = (buf_h - vp_h) / 2;

        D3D11_VIEWPORT vp = {
            static_cast<float>(vp_x), static_cast<float>(vp_y),
            static_cast<float>(vp_w), static_cast<float>(vp_h),
            0.0f, 1.0f
        };

        float clear[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        s.ctx->ClearRenderTargetView(s.rtv.Get(), clear);
        s.ctx->OMSetRenderTargets(1, s.rtv.GetAddressOf(), nullptr);
        s.ctx->RSSetViewports(1, &vp);

        UINT stride = sizeof(Vertex), offset = 0;
        s.ctx->IASetInputLayout(s.input_layout.Get());
        s.ctx->IASetVertexBuffers(0, 1, s.vertex_buffer.GetAddressOf(), &stride, &offset);
        s.ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        s.ctx->VSSetShader(s.vs.Get(), nullptr, 0);
        s.ctx->PSSetShader(s.ps.Get(), nullptr, 0);
        s.ctx->PSSetShaderResources(0, 1, s.srv.GetAddressOf());
        s.ctx->PSSetSamplers(0, 1, s.sampler.GetAddressOf());
        s.ctx->Draw(4, 0);

        s.swap_chain->Present(0, 0); // no vsync — FramePacer controls rate
    }

    DestroyWindow(s.hwnd);
    UnregisterClassW(CLASS, nullptr);
}

std::unique_ptr<IRenderer> create() {
    return std::make_unique<DirectXRenderer>();
}

} // namespace renderer
} // namespace cboy
