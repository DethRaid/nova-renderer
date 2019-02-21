#include "uniform_buffer.hpp"
#include "../../util/logger.hpp"

#include <cstring>

namespace nova::renderer {
    uniform_buffer::uniform_buffer(const std::string& name,
                                   VmaAllocator allocator,
                                   const VkBufferCreateInfo& create_info,
                                   const uint64_t alignment,
                                   const bool mapped)
        : name(std::move(name)), alignment(alignment), allocator(allocator) {
        VmaAllocationCreateInfo alloc_create = {};
        alloc_create.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        if(mapped) {
            alloc_create.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        }

        const VkResult buffer_create_result = vmaCreateBuffer(allocator,
                                                              reinterpret_cast<const VkBufferCreateInfo*>(&create_info),
                                                              &alloc_create,
                                                              reinterpret_cast<VkBuffer*>(&buffer),
                                                              &allocation,
                                                              &allocation_info);

        if(buffer_create_result != VK_SUCCESS) {
            NOVA_LOG(ERROR) << "Could not allocate a an autobuffer because " << buffer_create_result;
        }
        else {
            NOVA_LOG(TRACE) << "Auto buffer allocation success! Buffer ID: " << buffer;
        }
    }

    uniform_buffer::uniform_buffer(uniform_buffer&& old) noexcept
        : name(std::move(old.name)),
          alignment(old.alignment),
          allocator(old.allocator),
          device(old.device),
          buffer(old.buffer),
          allocation(old.allocation),
          allocation_info(old.allocation_info) {

        old.device = nullptr;
        old.buffer = nullptr;
        old.allocation = {};
        old.allocation_info = {};
    }

    uniform_buffer& uniform_buffer::operator=(uniform_buffer&& old) noexcept {
        name = std::move(old.name);
        alignment = old.alignment;
        allocator = old.allocator;
        device = old.device;
        buffer = old.buffer;
        allocation = old.allocation;
        allocation_info = old.allocation_info;

        old.device = nullptr;
        old.buffer = nullptr;
        old.allocation = {};
        old.allocation_info = {};

        return *this;
    }

    uniform_buffer::~uniform_buffer() {
        if(allocator != nullptr && buffer != nullptr) {
            NOVA_LOG(TRACE) << "uniform_buffer: About to destroy buffer " << buffer;
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }

    VmaAllocation& uniform_buffer::get_allocation() { return allocation; }

    VmaAllocationInfo& uniform_buffer::get_allocation_info() { return allocation_info; }

    const std::string& uniform_buffer::get_name() const { return name; }

    const VkBuffer& uniform_buffer::get_vk_buffer() const { return buffer; }

    uint64_t uniform_buffer::get_size() const { return alignment; }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void uniform_buffer::set_data(const void* data, uint32_t size) {
        void* mapped_data;
        vmaMapMemory(allocator, allocation, &mapped_data);
        std::memcpy(mapped_data, data, size);
        vmaUnmapMemory(allocator, allocation);
    }
} // namespace nova::renderer