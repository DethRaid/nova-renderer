/*!
 * \author ddubois
 * \date 30-Mar-19.
 */

#include "d3d12_command_list.hpp"
#include "d3dx12.h"
#include "dx12_utils.hpp"

namespace nova::renderer {
    d3d12_command_list::d3d12_command_list(ComPtr<ID3D12GraphicsCommandList> cmds) : cmds(std::move(cmds)) {}

    void d3d12_command_list::resource_barrier(const std::vector<resource_barrier_t>& barriers) {
        std::vector<D3D12_RESOURCE_BARRIER> dx12_barriers;
        dx12_barriers.reserve(barriers.size());

        for(const resource_barrier_t& barrier : barriers) {
            if(barrier.access_after_barrier == 0) {
                const D3D12_RESOURCE_STATES initial_state = to_dx12_state(barrier.initial_state);
                const D3D12_RESOURCE_STATES final_state = to_dx12_state(barrier.final_state);
                dx12_barriers.push_back(
                    CD3DX12_RESOURCE_BARRIER::Transition(barrier.resource_to_barrier->resource.Get(), initial_state, final_state));

            } else {
                const CD3DX12_RESOURCE_BARRIER sync = CD3DX12_RESOURCE_BARRIER::UAV(barrier.resource_to_barrier->resource.Get());
                dx12_barriers.push_back(sync);
            }
        }

        cmds->ResourceBarrier(static_cast<UINT>(dx12_barriers.size()), dx12_barriers.data());
    }

} // namespace nova::renderer