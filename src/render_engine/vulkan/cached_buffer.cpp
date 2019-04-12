#include "cached_buffer.hpp"

#include "../../util/logger.hpp"

namespace nova::renderer {
    cached_buffer::cached_buffer(
        std::string name, VkDevice device, VmaAllocator allocator, const VkBufferCreateInfo& create_info, const VkDeviceSize alignment)
        : name(std::move(name)), alignment(alignment), allocator(allocator), device(device) {
        VmaAllocationCreateInfo alloc_create = {};
        alloc_create.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        alloc_create.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkBufferCreateInfo device_buffer_create_info = create_info;
        device_buffer_create_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        const VkResult buffer_create_result = vmaCreateBuffer(allocator,
                                                              &device_buffer_create_info,
                                                              &alloc_create,
                                                              reinterpret_cast<VkBuffer*>(&buffer),
                                                              &allocation,
                                                              &allocation_info);

        if(buffer_create_result != VK_SUCCESS) {
            NOVA_LOG(ERROR) << "Could not allocate a uniform buffer because " << buffer_create_result;
        }

        VkBufferCreateInfo host_buffer_create_info = create_info;
        host_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        // Allocate the CPU cache
        VmaAllocationCreateInfo cpu_alloc_create = {};
        cpu_alloc_create.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        cpu_alloc_create.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        cpu_alloc_create.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        const VkResult cpu_buffer_create_result = vmaCreateBuffer(allocator,
                                                                  &host_buffer_create_info,
                                                                  &cpu_alloc_create,
                                                                  &cpu_buffer,
                                                                  &cpu_allocation,
                                                                  &cpu_alloc_info);

        if(cpu_buffer_create_result != VK_SUCCESS) {
            NOVA_LOG(ERROR) << "Could not allocate a uniform buffer cache because " << cpu_buffer_create_result;
        }

        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        vkCreateFence(device, &info, nullptr, &dummy_fence);
    }

    cached_buffer::~cached_buffer() {
        if(allocator != nullptr && buffer != nullptr) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }

        if(cpu_allocation != nullptr && cpu_buffer != nullptr) {
            vmaDestroyBuffer(allocator, cpu_buffer, cpu_allocation);
        }
    }

    VmaAllocation& cached_buffer::get_allocation() { return allocation; }

    VmaAllocationInfo& cached_buffer::get_allocation_info() { return allocation_info; }

    const std::string& cached_buffer::get_name() const { return name; }

    const VkBuffer& cached_buffer::get_vk_buffer() const { return buffer; }

    VkDeviceSize cached_buffer::get_size() const { return allocation_info.size; }

    VkFence cached_buffer::get_dummy_fence() const { return dummy_fence; }

    void cached_buffer::record_buffer_upload(VkCommandBuffer cmds) {
        VkBufferCopy copy = {};
        copy.size = allocation_info.size;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        vkCmdCopyBuffer(cmds, cpu_buffer, buffer, 1, &copy);
    }
} // namespace nova::renderer
