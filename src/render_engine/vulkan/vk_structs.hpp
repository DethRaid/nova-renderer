/*!
 * \brief Vulkan definition of the structs forward-declared in render_engine.hpp
 *
 * \author ddubois 
 * \date 01-Apr-19.
 */

#pragma once

#include <nova_renderer/render_engine_t.hpp>

namespace nova::renderer {
    struct vk_resource_t : resource_t {
        enum class type {
            IMAGE,
            BUFFER,
        };

        type resource_type;

        union {
            VkBuffer buffer;
            VkImage image;
        };
    };

    struct vk_renderpass_t : renderpass_t {
        VkRenderPass pass;
        VkRect2D render_area;
    };

    struct vk_framebuffer_t : framebuffer_t {
        VkFramebuffer framebuffer;
    };

	struct vk_gpu_info {
        VkPhysicalDevice phys_device{};
        std::vector<VkQueueFamilyProperties> queue_family_props;
        std::vector<VkExtensionProperties> available_extensions;
        VkSurfaceCapabilitiesKHR surface_capabilities{};
        std::vector<VkSurfaceFormatKHR> surface_formats;
        VkPhysicalDeviceProperties props{};
        VkPhysicalDeviceFeatures supported_features{};
    };
}
