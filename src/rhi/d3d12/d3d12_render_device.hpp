#pragma once

#include "nova_renderer/rhi/render_device.hpp"

#include <dxgi.h>
#include <d3d12.h>

namespace nova::renderer::rhi {
    class D3D12RenderDevice final : public RenderDevice {
    public:
        D3D12RenderDevice(NovaSettingsAccessManager& settings, NovaWindow& window, rx::memory::allocator& allocator);

        ~D3D12RenderDevice() override = default;

#pragma region RenderDevice
        void set_num_renderpasses(uint32_t num_renderpasses) override;

        [[nodiscard]] ntl::Result<RhiRenderpass*> create_renderpass(const renderpack::RenderPassCreateInfo& data,
                                                                    const glm::vec<2, unsigned, glm::qualifier::defaultp>& framebuffer_size,
                                                                    rx::memory::allocator& allocator) override;

        [[nodiscard]] RhiFramebuffer* create_framebuffer(const RhiRenderpass* renderpass,
                                                         const rx::vector<RhiImage*>& color_attachments,
                                                         const rx::optional<RhiImage*> depth_attachment,
                                                         const glm::vec<2, unsigned, glm::qualifier::defaultp>& framebuffer_size,
                                                         rx::memory::allocator& allocator) override;

        [[nodiscard]] rx::ptr<RhiPipeline> create_surface_pipeline(const RhiGraphicsPipelineState& pipeline_state,
                                                                   rx::memory::allocator& allocator) override;

        [[nodiscard]] rx::ptr<RhiPipeline> create_global_pipeline(const RhiGraphicsPipelineState& pipeline_state,
                                                                  rx::memory::allocator& allocator) override;

        [[nodiscard]] rx::ptr<RhiResourceBinder> create_resource_binder_for_pipeline(const RhiPipeline& pipeline,
                                                                                     rx::memory::allocator& allocator) override;

        [[nodiscard]] RhiBuffer* create_buffer(const RhiBufferCreateInfo& info, rx::memory::allocator& allocator) override;

        void write_data_to_buffer(const void* data, mem::Bytes num_bytes, const RhiBuffer* buffer) override;

        [[nodiscard]] RhiSampler* create_sampler(const RhiSamplerCreateInfo& create_info, rx::memory::allocator& allocator) override;

        [[nodiscard]] RhiImage* create_image(const renderpack::TextureCreateInfo& info, rx::memory::allocator& allocator) override;

        [[nodiscard]] RhiSemaphore* create_semaphore(rx::memory::allocator& allocator) override;

        [[nodiscard]] rx::vector<RhiSemaphore*> create_semaphores(uint32_t num_semaphores, rx::memory::allocator& allocator) override;

        [[nodiscard]] RhiFence* create_fence(bool signaled, rx::memory::allocator& allocator) override;

        [[nodiscard]] rx::vector<RhiFence*> create_fences(uint32_t num_fences, bool signaled, rx::memory::allocator& allocator) override;

        void wait_for_fences(rx::vector<RhiFence*> fences) override;

        void reset_fences(const rx::vector<RhiFence*>& fences) override;

        void destroy_renderpass(RhiRenderpass* pass, rx::memory::allocator& allocator) override;

        void destroy_framebuffer(RhiFramebuffer* framebuffer, rx::memory::allocator& allocator) override;

        void destroy_texture(RhiImage* resource, rx::memory::allocator& allocator) override;

        void destroy_semaphores(rx::vector<RhiSemaphore*>& semaphores, rx::memory::allocator& allocator) override;

        void destroy_fences(const rx::vector<RhiFence*>& fences, rx::memory::allocator& allocator) override;

        RhiRenderCommandList* create_command_list(uint32_t thread_idx,
                                                  QueueType needed_queue_type,
                                                  RhiRenderCommandList::Level level,
                                                  rx::memory::allocator& allocator) override;

        void submit_command_list(RhiRenderCommandList* cmds,
                                 QueueType queue,
                                 RhiFence* fence_to_signal,
                                 const rx::vector<RhiSemaphore*>& wait_semaphores,
                                 const rx::vector<RhiSemaphore*>& signal_semaphores) override;

        void end_frame(FrameContext& ctx) override;
#pragma endregion

    private:
        IDXGIFactory* factory;

        ID3D12Device* device;

        void initialize_dxgi();

        void select_adapter();
    };
} // namespace nova::renderer::rhi
