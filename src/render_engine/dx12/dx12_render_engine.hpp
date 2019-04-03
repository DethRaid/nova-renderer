/*!
 * \author ddubois
 * \date 30-Aug-18.
 */

#ifndef NOVA_RENDERER_DX_12_RENDER_ENGINE_HPP
#define NOVA_RENDERER_DX_12_RENDER_ENGINE_HPP

#include <nova_renderer/render_engine.hpp>

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_2.h>
#include <dxgi1_4.h>
#include "win32_window.hpp"

#include <memory>
#include <mutex>
#include <spirv_hlsl.hpp>
#include <unordered_map>
#include <wrl.h>
#include "dx12_texture.hpp"

using Microsoft::WRL::ComPtr;

namespace nova::ttl {
    class task_scheduler;
}

namespace nova::renderer {
    struct pipeline {
        ComPtr<ID3D12PipelineState> pso;
        ComPtr<ID3D12RootSignature> root_signature;
    };

#define FRAME_BUFFER_COUNT 3
    /*!
     * \brief Implements a render engine for DirectX 12
     */
    class dx12_render_engine : public render_engine {
    public:
        /*!
         * \brief Initializes DX12
         * \param settings The settings that may or may not influence initialization
         */
        explicit dx12_render_engine(nova_settings& settings);

        static std::string get_engine_name();

        /**
         * render_engine overrides
         */

        std::shared_ptr<window> get_window() const override;

        void set_shaderpack(const shaderpack_data_t& data) override;

        result<renderable_id_t> add_renderable(const static_mesh_renderable_data& data) override;

        void set_renderable_visibility(renderable_id_t id, bool is_visible) override;

        void delete_renderable(renderable_id_t id) override;

        result<mesh_id_t> add_mesh(const mesh_data&) override;

        void delete_mesh(uint32_t) override;
        
        command_list_t* allocate_command_list(uint32_t thread_idx, queue_type needed_queue_type, command_list_t::level command_list_type) override;

        void render_frame() override;

    private:
        // direct3d stuff
        ComPtr<IDXGIFactory2> dxgi_factory;

        ComPtr<ID3D12Device> device; // direct3d device

        ComPtr<IDXGISwapChain3> swapchain; // swapchain used to switch between render targets

        ComPtr<ID3D12CommandQueue> direct_command_queue; // container for command lists

        ComPtr<ID3D12DescriptorHeap> rtv_descriptor_heap; // a descriptor heap to hold resources like the render targets

        std::vector<ComPtr<ID3D12Resource>> rendertargets; // number of render targets equal to buffer count

        ComPtr<ID3D12QueryHeap> renderpass_timestamp_query_heap;

        uint32_t rtv_descriptor_size; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

        /*!
         * \brief The command allocators, one per command list type per thread
         */
        std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::vector<ComPtr<ID3D12CommandAllocator>>> command_allocators;

        void initialize_command_allocators();

        ComPtr<ID3D12CommandAllocator> get_allocator_for_thread(uint32_t thread_idx, D3D12_COMMAND_LIST_TYPE type);

        std::mutex buffer_pool_mutex;

        std::mutex lists_to_free_mutex;

        uint32_t frame_index = 0;

        uint32_t num_in_flight_frames = 3;

        std::vector<ComPtr<ID3D12Fence>> frame_fences;
        std::vector<uint32_t> frame_fence_values;
        HANDLE full_frame_fence_event;

        /*!
         * \brief All the textures that the loaded shaderpack has created
         */
        std::unordered_map<std::string, dx12_texture> dynamic_textures;
        std::unordered_map<std::string, uint32_t> dynamic_tex_name_to_idx;

        /*!
         * \brief The passes in the current frame graph, in submission order
         */
        std::unordered_map<std::string, render_pass_create_info_t> render_passes;
        std::vector<std::string> ordered_passes;

        std::shared_ptr<win32_window> window;

        void create_device();

        void create_rtv_command_queue();

    protected:
        void open_window(uint32_t width, uint32_t height) override;

    private:
        /*!
         * \brief Creates the swapchain from the size of the window
         *
         * \pre the window must be initialized
         */
        void create_swapchain();

        /*!
         * \brief Creates the descriptor heap for the swapchain
         */
        void create_render_target_descriptor_heap();

        command_list_t* allocate_command_list(D3D12_COMMAND_LIST_TYPE command_list_type) const;

        void create_full_frame_fences();

        void wait_for_previous_frame();

        void try_to_free_command_lists();

        void create_dynamic_textures(const std::vector<texture_create_into_t>& texture_datas, std::vector<render_pass_create_info_t> passes);

        void make_pipeline_state_objects(const std::vector<pipeline_create_info_t>& pipelines);

        pipeline make_single_pso(const pipeline_create_info_t& input);

        ComPtr<ID3D12RootSignature> create_root_signature(
            const std::unordered_map<uint32_t, std::vector<D3D12_DESCRIPTOR_RANGE1>>& tables) const;

        /*!
         * \brief Creates a timestamp query heap with enough space to time every render pass
         *
         * \param num_queries The number of queries the heap needs to support
         */
        void create_gpu_query_heap(size_t num_queries);
    };

    /*!
     * \brief Compiles the provided shader_source into a DirectX 12 shader
     *
     * \param shader The shader_source to compile. `shader.source` should be SPIR-V code
     * \param target The shader target to compile to. Shader model 5.1 is recommended
     * \param options Any options to use when compiling this shader
     * \param tables All the descriptor tables that this shader declares
     *
     * \return The compiled shader
     */
    ComPtr<ID3DBlob> compile_shader(const shader_source_t& shader,
                                    const std::string& target,
                                    const spirv_cross::CompilerHLSL::Options& options,
                                    std::unordered_map<uint32_t, std::vector<D3D12_DESCRIPTOR_RANGE1>>& tables);

    bool operator==(const D3D12_ROOT_PARAMETER1& param1, const D3D12_ROOT_PARAMETER1& param2);
    bool operator!=(const D3D12_ROOT_PARAMETER1& param1, const D3D12_ROOT_PARAMETER1& param2);

    bool operator==(const D3D12_ROOT_DESCRIPTOR_TABLE1& table1, const D3D12_ROOT_DESCRIPTOR_TABLE1& table2);
    bool operator!=(const D3D12_ROOT_DESCRIPTOR_TABLE1& table1, const D3D12_ROOT_DESCRIPTOR_TABLE1& table2);

    bool operator==(const D3D12_DESCRIPTOR_RANGE1& range1, const D3D12_DESCRIPTOR_RANGE1& range2);
    bool operator!=(const D3D12_DESCRIPTOR_RANGE1& range1, const D3D12_DESCRIPTOR_RANGE1& range2);

    bool operator==(const D3D12_ROOT_CONSTANTS& lhs, const D3D12_ROOT_CONSTANTS& rhs);

    bool operator==(const D3D12_ROOT_DESCRIPTOR1& lhs, const D3D12_ROOT_DESCRIPTOR1& rhs);
} // namespace nova::renderer

#endif // NOVA_RENDERER_DX_12_RENDER_ENGINE_HPP
