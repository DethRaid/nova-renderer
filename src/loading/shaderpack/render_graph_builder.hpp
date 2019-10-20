/*!
 * \author ddubois
 * \date 17-Sep-18.
 */

#ifndef NOVA_RENDERER_RENDER_GRAPH_BUILDER_HPP
#define NOVA_RENDERER_RENDER_GRAPH_BUILDER_HPP

#include <nova_renderer/shaderpack_data.hpp>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <nova_renderer/util/result.hpp>

namespace nova::renderer::shaderpack {
    struct Range {
        uint32_t first_write_pass = ~0U;
        uint32_t last_write_pass = 0;
        uint32_t first_read_pass = ~0U;
        uint32_t last_read_pass = 0;

        [[nodiscard]] bool has_writer() const;

        [[nodiscard]] bool has_reader() const;

        [[nodiscard]] bool is_used() const;

        [[nodiscard]] bool can_alias() const;

        [[nodiscard]] unsigned last_used_pass() const;

        [[nodiscard]] unsigned first_used_pass() const;

        [[nodiscard]] bool is_disjoint_with(const Range& other) const;
    };

    /*!
     * \brief Orders the provided render passes to satisfy both their implicit and explicit dependencies
     *
     * \param passes A map from pass name to pass of all the passes to order
     * \return The names of the passes in submission order
     */
    Result<eastl::vector<RenderPassCreateInfo>> order_passes(const eastl::vector<RenderPassCreateInfo>& passes);

    /*!
     * \brief Puts textures in usage order and determines which have overlapping usage ranges
     *
     * Knowing which textures have an overlapping usage range is super important cause if their ranges overlap, they can't be aliased
     *
     * \param passes All the passes in the current frame graph
     * \param resource_used_range A map to hold the usage ranges of each texture
     * \param resources_in_order A vector to hold the textures in usage order
     */
    void determine_usage_order_of_textures(const eastl::vector<RenderPassCreateInfo>& passes,
                                           eastl::unordered_map<eastl::string, Range>& resource_used_range,
                                           eastl::vector<eastl::string>& resources_in_order);

    /*!
     * \brief Determines which textures can be aliased to which other textures
     *
     * \param textures All the dynamic textures that this frame graph needs
     * \param resource_used_range The range of passes where each texture is used
     * \param resources_in_order The dynamic textures in usage order
     *
     * \return A map from texture name to the name of the texture the first texture can be aliased with
     */
    eastl::unordered_map<eastl::string, eastl::string> determine_aliasing_of_textures(
        const eastl::unordered_map<eastl::string, TextureCreateInfo>& textures,
        const eastl::unordered_map<eastl::string, Range>& resource_used_range,
        const eastl::vector<eastl::string>& resources_in_order);
} // namespace nova::renderer::shaderpack

#endif // NOVA_RENDERER_RENDER_GRAPH_BUILDER_HPP
