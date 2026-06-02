// SPDX-License-Identifier: GPL-3.0-only
// Vulkan renderer — uses GLFW for cross-platform windowing.
//
// Rendering approach:
//   1. Upload each GB frame (160×144, BGRA8) to a host-visible staging buffer.
//   2. Copy staging → a dedicated 160×144 source image (TRANSFER_SRC layout).
//   3. Blit that source image into the swapchain image with a letterboxed/
//      pillarboxed destination rect to maintain the GB aspect ratio.
//   4. Clear the bars to black before the blit via vkCmdClearColorImage.

#include "gameboy.hpp"
#include "renderer_vulkan.hpp"
#include "controls.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace cboy {
namespace renderer {

static const uint32_t GB_W  = 160;
static const uint32_t GB_H  = 144;
static const int      SCALE = 4;

// ---- Input ---------------------------------------------------------------

static bool glfw_key_to_button(int key, Button &out) {
    switch (key) {
    case GLFW_KEY_RIGHT: out = Button::RIGHT;  return true;
    case GLFW_KEY_LEFT:  out = Button::LEFT;   return true;
    case GLFW_KEY_UP:    out = Button::UP;     return true;
    case GLFW_KEY_DOWN:  out = Button::DOWN;   return true;
    case GLFW_KEY_A:     out = Button::A;      return true;
    case GLFW_KEY_S:     out = Button::B;      return true;
    case GLFW_KEY_Q:     out = Button::START;  return true;
    case GLFW_KEY_W:     out = Button::SELECT; return true;
    default:             return false;
    }
}

static void key_callback(GLFWwindow *window, int key, int, int action, int) {
    auto *gb = static_cast<Gameboy *>(glfwGetWindowUserPointer(window));
    if (!gb || action == GLFW_REPEAT) return;
    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_F5) { gb->load_state(); return; }
        if (key == GLFW_KEY_F6) { gb->save_state(); return; }
    }
    Button btn;
    if (glfw_key_to_button(key, btn)) {
        if (action == GLFW_PRESS) gb->controls().press(btn);
        else                      gb->controls().release(btn);
    }
}

// ---- Helpers -------------------------------------------------------------

static uint32_t find_memory_type(VkPhysicalDevice gpu, uint32_t type_bits,
                                  VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(gpu, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
        if ((type_bits & (1u << i)) &&
            (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    throw std::runtime_error("No suitable Vulkan memory type");
}

static void transition_image(VkCommandBuffer cmd, VkImage img,
                              VkImageLayout old_l, VkImageLayout new_l,
                              VkPipelineStageFlags src, VkPipelineStageFlags dst,
                              VkAccessFlags src_a, VkAccessFlags dst_a) {
    VkImageMemoryBarrier b = {};
    b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.oldLayout           = old_l;
    b.newLayout           = new_l;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image               = img;
    b.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    b.srcAccessMask       = src_a;
    b.dstAccessMask       = dst_a;
    vkCmdPipelineBarrier(cmd, src, dst, 0, 0, nullptr, 0, nullptr, 1, &b);
}

// Compute a centered letterbox/pillarbox rect for the GB aspect ratio.
static void letterbox_rect(uint32_t win_w, uint32_t win_h,
                            int32_t &x, int32_t &y,
                            uint32_t &w, uint32_t &h) {
    if (win_w * GB_H <= win_h * GB_W) {
        // Window taller than GB ratio → bars top/bottom
        w = win_w;
        h = win_w * GB_H / GB_W;
    } else {
        // Window wider than GB ratio → bars left/right
        h = win_h;
        w = win_h * GB_W / GB_H;
    }
    x = static_cast<int32_t>((win_w - w) / 2);
    y = static_cast<int32_t>((win_h - h) / 2);
}

// ---- Swapchain state (rebuilt on resize) ---------------------------------

struct SwapchainState {
    VkSwapchainKHR           swapchain = VK_NULL_HANDLE;
    std::vector<VkImage>     images;
    std::vector<VkCommandBuffer> cmd_bufs;
    VkExtent2D               extent{};
};

static SwapchainState create_swapchain(VkDevice device, VkPhysicalDevice gpu,
                                        VkSurfaceKHR surface,
                                        uint32_t graphics_family,
                                        VkCommandPool cmd_pool,
                                        VkSwapchainKHR old_swapchain,
                                        GLFWwindow *window) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps);

    // Use the surface extent, but clamp to the GLFW framebuffer size if
    // the driver reports 0xFFFFFFFF (meaning "you choose").
    VkExtent2D ext = caps.currentExtent;
    if (ext.width == 0xFFFFFFFF) {
        int fw, fh;
        glfwGetFramebufferSize(window, &fw, &fh);
        ext.width  = std::max(caps.minImageExtent.width,
                              std::min(caps.maxImageExtent.width,  (uint32_t)fw));
        ext.height = std::max(caps.minImageExtent.height,
                              std::min(caps.maxImageExtent.height, (uint32_t)fh));
    }

    VkSwapchainCreateInfoKHR sci = {};
    sci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface          = surface;
    sci.minImageCount    = caps.minImageCount;
    sci.imageFormat      = VK_FORMAT_B8G8R8A8_UNORM;
    sci.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent      = ext;
    sci.imageArrayLayers = 1;
    // TRANSFER_DST: we blit into swapchain images
    sci.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform     = caps.currentTransform;
    sci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped          = VK_TRUE;
    sci.oldSwapchain     = old_swapchain;

    SwapchainState s;
    s.extent = ext;
    if (vkCreateSwapchainKHR(device, &sci, nullptr, &s.swapchain) != VK_SUCCESS)
        throw std::runtime_error("vkCreateSwapchainKHR failed");

    uint32_t img_count = 0;
    vkGetSwapchainImagesKHR(device, s.swapchain, &img_count, nullptr);
    s.images.resize(img_count);
    vkGetSwapchainImagesKHR(device, s.swapchain, &img_count, s.images.data());

    s.cmd_bufs.resize(img_count);
    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool        = cmd_pool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = img_count;
    vkAllocateCommandBuffers(device, &cbai, s.cmd_bufs.data());

    return s;
}

// ---- Renderer ------------------------------------------------------------

void VulkanRenderer::run(Gameboy &gameboy) {
    // --- GLFW window ---
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(static_cast<int>(GB_W * SCALE),
                                          static_cast<int>(GB_H * SCALE),
                                          "cboy — Vulkan", nullptr, nullptr);
    if (!window) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }
    glfwSetWindowUserPointer(window, &gameboy);
    glfwSetKeyCallback(window, key_callback);

    // --- Instance ---
    uint32_t glfw_ext_count = 0;
    const char **glfw_exts  = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    VkApplicationInfo app_info = {};
    app_info.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName  = "cboy";
    app_info.apiVersion        = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info   = {};
    inst_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo        = &app_info;
    inst_info.enabledExtensionCount   = glfw_ext_count;
    inst_info.ppEnabledExtensionNames = glfw_exts;

    VkInstance instance;
    if (vkCreateInstance(&inst_info, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");

    // --- Surface ---
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("glfwCreateWindowSurface failed");

    // --- Physical device ---
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
    if (gpu_count == 0) throw std::runtime_error("No Vulkan GPU found");
    VkPhysicalDevice gpu = gpus[0];

    // --- Queue family (graphics + present on same family) ---
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &qf_count, nullptr);
    std::vector<VkQueueFamilyProperties> qf_props(qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &qf_count, qf_props.data());

    uint32_t graphics_family = UINT32_MAX;
    for (uint32_t i = 0; i < qf_count; ++i) {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present);
        if ((qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
            graphics_family = i; break;
        }
    }
    if (graphics_family == UINT32_MAX)
        throw std::runtime_error("No suitable Vulkan queue family");

    // --- Logical device ---
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {};
    qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = graphics_family;
    qci.queueCount       = 1;
    qci.pQueuePriorities = &priority;

    const char *dev_exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo dci  = {};
    dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount    = 1;
    dci.pQueueCreateInfos       = &qci;
    dci.enabledExtensionCount   = 1;
    dci.ppEnabledExtensionNames = dev_exts;

    VkDevice device;
    if (vkCreateDevice(gpu, &dci, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed");

    VkQueue queue;
    vkGetDeviceQueue(device, graphics_family, 0, &queue);

    // --- Command pool ---
    VkCommandPoolCreateInfo cpci = {};
    cpci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = graphics_family;
    cpci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool cmd_pool;
    vkCreateCommandPool(device, &cpci, nullptr, &cmd_pool);

    // --- Initial swapchain ---
    SwapchainState sc = create_swapchain(device, gpu, surface, graphics_family,
                                          cmd_pool, VK_NULL_HANDLE, window);

    // --- Staging buffer (160×144×4 bytes, persistently mapped) ---
    const uint32_t pixel_bytes = GB_W * GB_H * 4;

    VkBufferCreateInfo bci = {};
    bci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size        = pixel_bytes;
    bci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer staging;
    vkCreateBuffer(device, &bci, nullptr, &staging);

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(device, staging, &mem_req);

    VkMemoryAllocateInfo mai = {};
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize  = mem_req.size;
    mai.memoryTypeIndex = find_memory_type(gpu, mem_req.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory staging_mem;
    vkAllocateMemory(device, &mai, nullptr, &staging_mem);
    vkBindBufferMemory(device, staging, staging_mem, 0);

    void *mapped_staging = nullptr;
    vkMapMemory(device, staging_mem, 0, pixel_bytes, 0, &mapped_staging);

    // --- Intermediate 160×144 image (the GB source image for blitting) ---
    VkImageCreateInfo ici = {};
    ici.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType     = VK_IMAGE_TYPE_2D;
    ici.format        = VK_FORMAT_B8G8R8A8_UNORM;
    ici.extent        = {GB_W, GB_H, 1};
    ici.mipLevels     = 1;
    ici.arrayLayers   = 1;
    ici.samples       = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ici.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage src_img;
    vkCreateImage(device, &ici, nullptr, &src_img);

    VkMemoryRequirements img_req;
    vkGetImageMemoryRequirements(device, src_img, &img_req);

    VkMemoryAllocateInfo img_mai = {};
    img_mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    img_mai.allocationSize  = img_req.size;
    img_mai.memoryTypeIndex = find_memory_type(gpu, img_req.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory src_img_mem;
    vkAllocateMemory(device, &img_mai, nullptr, &src_img_mem);
    vkBindImageMemory(device, src_img, src_img_mem, 0);

    // --- Per-frame sync objects (one set per swapchain image) ---
    // Each frame gets its own pair of semaphores and a fence so we never
    // reuse a semaphore that is still in flight.
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> image_ready_sems(MAX_FRAMES_IN_FLIGHT);
    std::vector<VkSemaphore> render_done_sems(MAX_FRAMES_IN_FLIGHT);
    std::vector<VkFence>     frame_fences(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo sem_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fence_info   = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags               = VK_FENCE_CREATE_SIGNALED_BIT; // start signalled

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkCreateSemaphore(device, &sem_info, nullptr, &image_ready_sems[i]);
        vkCreateSemaphore(device, &sem_info, nullptr, &render_done_sems[i]);
        vkCreateFence(device, &fence_info, nullptr, &frame_fences[i]);
    }

    uint32_t current_frame = 0;
    bool need_rebuild = false;

    // --- Main loop ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Rebuild swapchain if window was resized
        if (need_rebuild) {
            vkDeviceWaitIdle(device);
            int fw, fh;
            glfwGetFramebufferSize(window, &fw, &fh);
            if (fw == 0 || fh == 0) continue; // minimised — wait

            VkSwapchainKHR old = sc.swapchain;
            vkFreeCommandBuffers(device, cmd_pool,
                                 static_cast<uint32_t>(sc.cmd_bufs.size()),
                                 sc.cmd_bufs.data());
            sc = create_swapchain(device, gpu, surface, graphics_family,
                                  cmd_pool, old, window);
            vkDestroySwapchainKHR(device, old, nullptr);
            need_rebuild = false;
        }

        // --- Upload GB frame to staging buffer (GB RGB555 → BGRA8) ---
        const display::Frame &frame = gameboy.run_frame();
        auto *px = static_cast<uint8_t *>(mapped_staging);
        for (uint32_t y = 0; y < GB_H; ++y) {
            for (uint32_t x = 0; x < GB_W; ++x) {
                uint16_t c = frame[y][x];
                uint8_t r = static_cast<uint8_t>(((c >>  0) & 0x1F) * 255 / 31);
                uint8_t g = static_cast<uint8_t>(((c >>  5) & 0x1F) * 255 / 31);
                uint8_t b = static_cast<uint8_t>(((c >> 10) & 0x1F) * 255 / 31);
                uint32_t i = (y * GB_W + x) * 4;
                px[i]   = b;
                px[i+1] = g;
                px[i+2] = r;
                px[i+3] = 0xFF;
            }
        }

        // --- Acquire swapchain image ---
        uint32_t img_idx;
        VkResult acq = vkAcquireNextImageKHR(device, sc.swapchain, UINT64_MAX,
                                              image_ready_sems[current_frame],
                                              VK_NULL_HANDLE, &img_idx);
        if (acq == VK_ERROR_OUT_OF_DATE_KHR || acq == VK_SUBOPTIMAL_KHR) {
            need_rebuild = true; continue;
        }

        // --- Record command buffer ---
        VkCommandBuffer cmd = sc.cmd_bufs[img_idx];
        VkCommandBufferBeginInfo cbbi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(cmd, &cbbi);

        // 1. Copy staging buffer → intermediate 160×144 image
        transition_image(cmd, src_img,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, VK_ACCESS_TRANSFER_WRITE_BIT);

        VkBufferImageCopy buf_copy = {};
        buf_copy.bufferRowLength   = GB_W;
        buf_copy.bufferImageHeight = GB_H;
        buf_copy.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        buf_copy.imageExtent       = {GB_W, GB_H, 1};
        vkCmdCopyBufferToImage(cmd, staging, src_img,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buf_copy);

        // 2. Transition src image → TRANSFER_SRC
        transition_image(cmd, src_img,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);

        // 3. Transition swapchain image → TRANSFER_DST
        transition_image(cmd, sc.images[img_idx],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, VK_ACCESS_TRANSFER_WRITE_BIT);

        // 4. Clear swapchain image to black (fills letterbox/pillarbox bars)
        VkClearColorValue black = {};
        VkImageSubresourceRange full_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdClearColorImage(cmd, sc.images[img_idx],
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &full_range);

        // 5. Blit src_img (160×144) → letterboxed rect in swapchain image
        int32_t  dst_x, dst_y;
        uint32_t dst_w, dst_h;
        letterbox_rect(sc.extent.width, sc.extent.height, dst_x, dst_y, dst_w, dst_h);

        VkImageBlit blit = {};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.srcOffsets[0]  = {0, 0, 0};
        blit.srcOffsets[1]  = {static_cast<int32_t>(GB_W), static_cast<int32_t>(GB_H), 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        blit.dstOffsets[0]  = {dst_x,               dst_y,               0};
        blit.dstOffsets[1]  = {dst_x + (int32_t)dst_w, dst_y + (int32_t)dst_h, 1};
        vkCmdBlitImage(cmd,
                       src_img,          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       sc.images[img_idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit, VK_FILTER_NEAREST);

        // 6. Transition swapchain image → PRESENT
        transition_image(cmd, sc.images[img_idx],
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, 0);

        vkEndCommandBuffer(cmd);

        // Wait for this frame slot's previous submission to finish before
        // reusing its semaphores and command buffer.
        vkWaitForFences(device, 1, &frame_fences[current_frame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &frame_fences[current_frame]);

        // --- Submit ---
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo si = {};
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount   = 1;
        si.pWaitSemaphores      = &image_ready_sems[current_frame];
        si.pWaitDstStageMask    = &wait_stage;
        si.commandBufferCount   = 1;
        si.pCommandBuffers      = &cmd;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores    = &render_done_sems[current_frame];
        vkQueueSubmit(queue, 1, &si, frame_fences[current_frame]);

        // --- Present ---
        VkPresentInfoKHR pi = {};
        pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores    = &render_done_sems[current_frame];
        pi.swapchainCount     = 1;
        pi.pSwapchains        = &sc.swapchain;
        pi.pImageIndices      = &img_idx;
        VkResult present_result = vkQueuePresentKHR(queue, &pi);
        if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
            present_result == VK_SUBOPTIMAL_KHR)
            need_rebuild = true;

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // --- Cleanup ---
    vkDeviceWaitIdle(device);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, image_ready_sems[i], nullptr);
        vkDestroySemaphore(device, render_done_sems[i], nullptr);
        vkDestroyFence(device, frame_fences[i], nullptr);
    }
    vkDestroyImage(device, src_img, nullptr);
    vkFreeMemory(device, src_img_mem, nullptr);
    vkUnmapMemory(device, staging_mem);
    vkFreeMemory(device, staging_mem, nullptr);
    vkDestroyBuffer(device, staging, nullptr);
    vkFreeCommandBuffers(device, cmd_pool,
                         static_cast<uint32_t>(sc.cmd_bufs.size()),
                         sc.cmd_bufs.data());
    vkDestroySwapchainKHR(device, sc.swapchain, nullptr);
    vkDestroyCommandPool(device, cmd_pool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}

std::unique_ptr<IRenderer> create() {
    return std::make_unique<VulkanRenderer>();
}

} // namespace renderer
} // namespace cboy
