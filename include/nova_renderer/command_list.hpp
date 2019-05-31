/*!
 * \author ddubois
 * \date 30-Mar-19.
 */

#pragma once
#include <vector>

#include "nova_renderer/rhi_types.hpp"

namespace nova::renderer::rhi {
    /*!
     * \brief An API-agnostic command list
     *
     * Command lists are allocated from the render engine. Once allocated, ownership is passed to the callee. You can
     * then record whatever commands you want and submit the command list back to the render engine for execution on
     * the GPU. Once submitted, you may not record any more commands into the command list
     *
     * There is one command list pool per swapchain image per thread. All the pools for one swapchain image are
     * reset at the beginning of a frame that renders to that swapchain image. This means that any command list
     * allocated in one frame will not be valid in the next frame. DO NOT hold on to command lists
     *
     * A command list may only be recorded to from one thread at a time
     *
     * Command lists are fully bound to ChaiScript
     */
    class CommandList {
    public:
        enum class Level {
            Primary,
            Secondary,
        };

        CommandList() = default;

        CommandList(CommandList&& old) noexcept = default;
        CommandList& operator=(CommandList&& old) noexcept = default;

        CommandList(const CommandList& other) = delete;
        CommandList& operator=(const CommandList& other) = delete;

        /*!
         * \brief Inserts a barrier so that all access to a resource before the barrier is resolved before any access
         * to the resource after the barrier
         *
         * \param stages_before_barrier The pipeline stages that should be completed before the barriers take effect
         * \param stages_after_barrier The pipeline stages that must wait for the barrier
         * \param barriers All the resource barriers to use
         */
        virtual void resource_barriers(PipelineStageFlags stages_before_barrier,
                                       PipelineStageFlags stages_after_barrier,
                                       const std::vector<ResourceBarrier>& barriers) = 0;

        /*!
         * \brief Records a command to copy one region of a buffer to another buffer
         *
         * \param destination_buffer The buffer to write data to
         * \param destination_offset The offset in the destination buffer to start writing to. Measured in bytes
         * \param source_buffer The buffer to read data from
         * \param source_offset The offset in the source buffer to start reading from. Measures in bytes
         * \param num_bytes The number of bytes to copy
         *
         * \pre destination_buffer != nullptr
         * \pre destination_buffer is a buffer
         * \pre destination_offset is less than the size of destination_buffer
         * \pre source_buffer != nullptr
         * \pre source_buffer is a buffer
         * \pre source_offset is less than the size of source_buffer
         * \pre destination_offset plus num_bytes is less than the size of destination_buffer
         * \pre destination_offset plus num_bytes is less than the size of source_buffer
         */
        virtual void copy_buffer(
            Buffer* destination_buffer, uint64_t destination_offset, Buffer* source_buffer, uint64_t source_offset, uint64_t num_bytes) = 0;

        /*!
         * \brief Executed a number of command lists
         *
         * These command lists should be secondary command lists. Nova doesn't validate this because yolo but you need
         * to be nice - the API-specific validation layers _will_ yell at you
         */
        virtual void execute_command_lists(const std::vector<CommandList*>& lists) = 0;

        /*!
         * \brief Begins a renderpass
         * 
         * \param renderpass The renderpass to begin
         * \param framebuffer The framebuffer to render to
         */
        virtual void begin_renderpass(Renderpass* renderpass, Framebuffer* framebuffer) = 0;

        virtual void end_renderpass() = 0;

        virtual void bind_pipeline(const rhi::Pipeline* pipeline) = 0;

        virtual void bind_material() = 0;

        virtual void bind_vertex_buffers() = 0;
        virtual void bind_index_buffer() = 0;
        virtual void draw_indexed_mesh() = 0;

        virtual ~CommandList() = default;
    };
} // namespace nova::renderer::rhi
