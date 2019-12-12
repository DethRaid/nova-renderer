#include "nova_renderer/frontend/rendergraph.hpp"

#include "nova_renderer/nova_renderer.hpp"
#include "nova_renderer/rhi/command_list.hpp"
#include "nova_renderer/rhi/render_engine.hpp"

#include "../util/logger.hpp"

namespace nova::renderer {
    void default_record_into_command_list(const Renderpass& renderpass, rhi::CommandList* cmds, FrameContext& ctx);

    void record_into_command_list(const Pipeline& pipeline, rhi::CommandList* cmds, FrameContext& ctx);

    void record_into_command_list(const MaterialPass& pass, rhi::CommandList* cmds, FrameContext& ctx);

    void record_rendering_static_mesh_batch(const MeshBatch<StaticMeshRenderCommand>& batch, rhi::CommandList* cmds, FrameContext& ctx);

    void record_rendering_static_mesh_batch(const ProceduralMeshBatch<StaticMeshRenderCommand>& batch,
                                            rhi::CommandList* cmds,
                                            FrameContext& ctx);

    void record_into_command_list(const Renderpass& renderpass, rhi::CommandList* cmds, FrameContext& ctx) {
        if(renderpass.record_func) {
            // Gotta unwrap the optional ugh
            (*renderpass.record_func)(renderpass, cmds, ctx);

        } else {
            default_record_into_command_list(renderpass, cmds, ctx);
        }
    }

    void default_record_into_command_list(const Renderpass& renderpass, rhi::CommandList* cmds, FrameContext& ctx) {
        // TODO: Figure if any of these barriers are implicit
        // TODO: Use shader reflection to figure our the stage that the pipelines in this renderpass need access to this resource instead of
        // using a robust default

        if(!renderpass.read_texture_barriers.empty()) {
            // TODO: Use shader reflection to figure our the stage that the pipelines in this renderpass need access to this resource
            // instead of using a robust default
            cmds->resource_barriers(rhi::PipelineStageFlags::ColorAttachmentOutput,
                                    rhi::PipelineStageFlags::FragmentShader,
                                    renderpass.read_texture_barriers);
        }

        if(!renderpass.write_texture_barriers.empty()) {
            // TODO: Use shader reflection to figure our the stage that the pipelines in this renderpass need access to this resource
            // instead of using a robust default
            cmds->resource_barriers(rhi::PipelineStageFlags::ColorAttachmentOutput,
                                    rhi::PipelineStageFlags::FragmentShader,
                                    renderpass.write_texture_barriers);
        }

        if(renderpass.writes_to_backbuffer) {
            rhi::ResourceBarrier backbuffer_barrier{};
            backbuffer_barrier.resource_to_barrier = ctx.swapchain_image;
            backbuffer_barrier.access_before_barrier = rhi::AccessFlags::MemoryRead;
            backbuffer_barrier.access_after_barrier = rhi::AccessFlags::ColorAttachmentWrite;
            backbuffer_barrier.old_state = rhi::ResourceState::PresentSource;
            backbuffer_barrier.new_state = rhi::ResourceState::RenderTarget;
            backbuffer_barrier.source_queue = rhi::QueueType::Graphics;
            backbuffer_barrier.destination_queue = rhi::QueueType::Graphics;
            backbuffer_barrier.image_memory_barrier.aspect = rhi::ImageAspectFlags::Color;

            // TODO: Use shader reflection to figure our the stage that the pipelines in this renderpass need access to this resource
            // instead of using a robust default
            cmds->resource_barriers(rhi::PipelineStageFlags::TopOfPipe,
                                    rhi::PipelineStageFlags::ColorAttachmentOutput,
                                    {backbuffer_barrier});
        }

        const auto framebuffer = [&] {
            if(!renderpass.writes_to_backbuffer) {
                return renderpass.framebuffer;
            } else {
                return ctx.swapchain_framebuffer;
            }
        }();

        cmds->begin_renderpass(renderpass.renderpass, framebuffer);

        for(const Pipeline& pipeline : renderpass.pipelines) {
            record_into_command_list(pipeline, cmds, ctx);
        }

        cmds->end_renderpass();

        if(renderpass.writes_to_backbuffer) {
            rhi::ResourceBarrier backbuffer_barrier{};
            backbuffer_barrier.resource_to_barrier = ctx.swapchain_image;
            backbuffer_barrier.access_before_barrier = rhi::AccessFlags::ColorAttachmentWrite;
            backbuffer_barrier.access_after_barrier = rhi::AccessFlags::MemoryRead;
            backbuffer_barrier.old_state = rhi::ResourceState::RenderTarget;
            backbuffer_barrier.new_state = rhi::ResourceState::PresentSource;
            backbuffer_barrier.source_queue = rhi::QueueType::Graphics;
            backbuffer_barrier.destination_queue = rhi::QueueType::Graphics;
            backbuffer_barrier.image_memory_barrier.aspect = rhi::ImageAspectFlags::Color;

            // When this line executes, the D3D12 debug layer gets mad about "A single command list cannot write to multiple buffers within
            // a particular swapchain" and I don't know why it's mad about that, or even really what that message means
            cmds->resource_barriers(rhi::PipelineStageFlags::ColorAttachmentOutput,
                                    rhi::PipelineStageFlags::BottomOfPipe,
                                    {backbuffer_barrier});
        }
    }

    void record_into_command_list(const Pipeline& pipeline, rhi::CommandList* cmds, FrameContext& ctx) {
        cmds->bind_pipeline(pipeline.pipeline);

        for(const MaterialPass& pass : pipeline.passes) {
            record_into_command_list(pass, cmds, ctx);
        }
    }

    void record_into_command_list(const MaterialPass& pass, rhi::CommandList* cmds, FrameContext& ctx) {
        cmds->bind_descriptor_sets(pass.descriptor_sets, pass.pipeline_interface);

        for(const MeshBatch<StaticMeshRenderCommand>& batch : pass.static_mesh_draws) {
            record_rendering_static_mesh_batch(batch, cmds, ctx);
        }

        for(const ProceduralMeshBatch<StaticMeshRenderCommand>& batch : pass.static_procedural_mesh_draws) {
            record_rendering_static_mesh_batch(batch, cmds, ctx);
        }
    }

    void record_rendering_static_mesh_batch(const MeshBatch<StaticMeshRenderCommand>& batch, rhi::CommandList* cmds, FrameContext& ctx) {
        const uint64_t start_index = ctx.cur_model_matrix_index;

        for(const StaticMeshRenderCommand& command : batch.commands) {
            if(command.is_visible) {
                auto* model_matrix_buffer = ctx.nova->get_builtin_buffer(MODEL_MATRIX_BUFFER_NAME);
                ctx.nova->get_engine()->write_data_to_buffer(&command.model_matrix,
                                                            sizeof(glm::mat4),
                                                            ctx.cur_model_matrix_index * sizeof(glm::mat4),
                                                            model_matrix_buffer);
                ctx.cur_model_matrix_index++;
            }
        }

        if(start_index != ctx.cur_model_matrix_index) {
            // TODO: There's probably a better way to do this
            const std::vector<rhi::Buffer*> vertex_buffers = std::vector<rhi::Buffer*>(7, batch.vertex_buffer);
            cmds->bind_vertex_buffers(vertex_buffers);
            cmds->bind_index_buffer(batch.index_buffer);

            cmds->draw_indexed_mesh(static_cast<uint32_t>(batch.index_buffer->size / sizeof(uint32_t)),
                                    static_cast<uint32_t>(ctx.cur_model_matrix_index - start_index));
        }
    }

    void record_rendering_static_mesh_batch(const ProceduralMeshBatch<StaticMeshRenderCommand>& batch,
                                            rhi::CommandList* cmds,
                                            FrameContext& ctx) {
        const uint64_t start_index = ctx.cur_model_matrix_index;

        for(const StaticMeshRenderCommand& command : batch.commands) {
            if(command.is_visible) {
                auto* model_matrix_buffer = ctx.nova->get_builtin_buffer(MODEL_MATRIX_BUFFER_NAME);
                ctx.nova->get_engine()->write_data_to_buffer(&command.model_matrix,
                    sizeof(glm::mat4),
                    ctx.cur_model_matrix_index * sizeof(glm::mat4),
                    model_matrix_buffer);
                ctx.cur_model_matrix_index++;
            }
        }

        if(start_index != ctx.cur_model_matrix_index) {
            const auto& [vertex_buffer, index_buffer] = batch.mesh->get_buffers_for_frame(ctx.frame_count % NUM_IN_FLIGHT_FRAMES);
            // TODO: There's probably a better way to do this
            const std::vector<rhi::Buffer*> vertex_buffers = {7, vertex_buffer};
            cmds->bind_vertex_buffers(vertex_buffers);
            cmds->bind_index_buffer(index_buffer);
        }
    }
} // namespace nova::renderer