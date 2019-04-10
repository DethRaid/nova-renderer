/*!
 * \author ddubois
 * \date 12-Feb-18.
 */
#include "auto_allocating_buffer.hpp"

#include <fmt/format.h>

#include "../../util/logger.hpp"

namespace nova::renderer {
    auto_buffer::auto_buffer(
        const std::string& name, VkDevice device, VmaAllocator allocator, VkBufferCreateInfo& create_info, const uint64_t alignment)
        : cached_buffer(name, device, allocator, create_info, alignment) {

        chunks.emplace_back(auto_buffer_chunk{VkDeviceSize(0), create_info.size});
    }

    auto_buffer::auto_buffer(auto_buffer&& old) noexcept : cached_buffer(std::forward<cached_buffer>(old)) {
        chunks = std::move(old.chunks);
        old.chunks.clear();
    }

    auto_buffer& auto_buffer::operator=(auto_buffer&& old) noexcept {
        cached_buffer::operator=(std::forward<cached_buffer>(old));

        chunks = std::move(old.chunks);
        old.chunks.clear();

        return *this;
    }

    result<VkDescriptorBufferInfo> auto_buffer::allocate_space(uint64_t size) {
        size = size > alignment ? size : alignment;
        int32_t index_to_allocate_from = -1;
        if(!chunks.empty()) {
            // Iterate backwards so that the blocks are at the start of the vector's memory, so inserting at the end
            // or deleting from the end (expected use case) has a minimal cost
            for(int32_t i = static_cast<int32_t>(chunks.size() - 1); i >= 0; --i) {
                if(chunks[static_cast<uint32_t>(i)].range >= size) {
                    index_to_allocate_from = i;
                }
            }
        }

        VkDescriptorBufferInfo ret_val;

        if(index_to_allocate_from == -1) {
            // Whoops, couldn't find anything
            return result<VkDescriptorBufferInfo>(
                MAKE_ERROR("No big enough slots in the buffer. There's {:d} slots. If there's a lot then you got some fragmentation",
                           chunks.size()));
        }

        auto& chunk_to_allocate_from = chunks[static_cast<uint32_t>(index_to_allocate_from)];
        if(chunk_to_allocate_from.range == size) {
            // Easy: allocate and return the chunk

            ret_val = VkDescriptorBufferInfo{buffer, chunk_to_allocate_from.offset, chunk_to_allocate_from.range};
            chunks.erase(chunks.begin() + index_to_allocate_from);
            return result<VkDescriptorBufferInfo>(ret_val);
        }

        // The chunk is bigger than we need. Allocate at the beginning of it so our iteration algorithm finds the next
        // free chunk nice and early, then shrink it

        ret_val = VkDescriptorBufferInfo{buffer, chunk_to_allocate_from.offset, size};

        chunk_to_allocate_from.offset += size;
        chunk_to_allocate_from.range -= size;

        return result<VkDescriptorBufferInfo>(ret_val);
    }

    void auto_buffer::free_allocation(const VkDescriptorBufferInfo& to_free) {
        // This one will be hard...
        // Properly we should try to find an allocated space of our size and merge the two allocations on either side of
        // it... but that's marginally harder (maybe I'll do it) so let's just try to find the first slot it will fit

        const auto to_free_end = to_free.offset + to_free.range;

        auto& first_chunk = chunks[0];
        auto& last_chunk = chunks[chunks.size() - 1];

        if(chunks.empty()) {
            chunks.emplace_back(auto_buffer_chunk{to_free.offset, to_free.range});
            return;
        }

        if(last_chunk.offset + last_chunk.range == to_free.offset) {
            last_chunk.range += to_free.range;
            return;
        }

        if(last_chunk.offset + last_chunk.range < to_free.offset) {
            chunks.emplace_back(auto_buffer_chunk{to_free.offset, to_free.range});
            return;
        }

        if(to_free_end == first_chunk.offset) {
            first_chunk.offset -= to_free.range;
            first_chunk.range += to_free.range;
            return;
        }

        if(to_free_end < first_chunk.offset) {
            chunks.emplace(chunks.begin(), auto_buffer_chunk{to_free.offset, to_free.range});
            return;
        }

        for(auto i = chunks.size() - 1; i >= 1; i++) {
            auto& behind_space = chunks[i - 1];
            auto& ahead_space = chunks[i];

            const auto behind_space_end = behind_space.offset + behind_space.range;
            const auto space_between_allocs = space_between(behind_space, ahead_space);

            // Do we fit nicely between the two things?
            if(space_between_allocs == to_free.range) {
                // combine these nerds
                chunks[i - 1].range += to_free.range + ahead_space.range;
                chunks.erase(chunks.begin() + static_cast<long>(i));
                return;
            }

            // Do we fit up against one of the two things?
            if(space_between_allocs > to_free.range) {
                if(behind_space_end == to_free.offset) {
                    chunks[i - 1].range += to_free.range;
                    return;
                }

                if(to_free_end == ahead_space.offset) {
                    chunks[i].offset -= to_free.range;
                    chunks[i].range += to_free.range;
                    return;
                }

                chunks.emplace(chunks.begin() + static_cast<long>(i), auto_buffer_chunk{to_free.offset, to_free.range});
                return;
            }
        }

        // We got here... without returning our allocation to the pool. Uhm... Did we double-allocate something? This is
        // a bug in my allocator and not something that should happen during Nova so let's just crash
        NOVA_LOG(FATAL) << "Could not return allocation {offset=" << to_free.offset << " range=" << to_free.range
                        << "} which should not happen. There's probably a bug in the allocator and you need to debug it";
    }

    VkDeviceSize space_between(const auto_buffer_chunk& first, const auto_buffer_chunk& last) {
        return last.offset - (first.offset + first.range);
    }
} // namespace nova::renderer
