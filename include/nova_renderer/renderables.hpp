//
// Created by jannis on 11.09.18.
//

#ifndef NOVA_RENDERER_RENDER_OBJECT_HPP
#define NOVA_RENDERER_RENDER_OBJECT_HPP

#include <atomic>

#include <nova_renderer/shaderpack_data.hpp>

namespace nova::renderer {
    struct full_vertex_t {
        glm::vec3 position;          // 12 bytes
        glm::vec3 normal;            // 12 bytes
        glm::vec3 tangent;           // 12 bytes
        glm::u16vec2 main_uv;        // 4 bytes
        glm::u8vec2 secondary_uv;    // 2 bytes
        uint32_t virtual_texture_id; // 4 bytes
        glm::vec4 additional_stuff;  // 16 bytes
    };

    static_assert(sizeof(full_vertex_t) % 16 == 0, "full_vertex struct is not aligned to 16 bytes!");

    /*!
     * \brief All the data needed to make a single mesh
     *
     * Meshes all have the same data. Chunks need all the mesh data, and they're most of the world. Entities, GUI,
     * particles, etc will probably have FAR fewer vertices than chunks, meaning that there's not a huge savings by
     * making them use special vertex formats
     */
    struct mesh_data_t {
        std::vector<full_vertex_t> vertex_data;
        std::vector<uint32_t> indices;
    };

    using mesh_id_t = uint32_t;

    struct static_mesh_renderable_update_data_t {
        std::string material_name;

        mesh_id_t mesh;
    };

    struct static_mesh_renderable_data_t : static_mesh_renderable_update_data_t {
        glm::vec3 initial_position;
        glm::vec3 initial_rotation;
        glm::vec3 initial_scale = glm::vec3(1);

        bool is_static = true;
    };

    using renderable_id_t = uint64_t;

    static std::atomic<renderable_id_t> next_renderable_id;

    struct renderable_metadata_t {
        renderable_id_t id = 0;

        std::vector<std::string> passes;
    };

    struct renderable_base_t {
        renderable_id_t id = 0;

        bool is_visible = true;

        glm::mat4 model_matrix = glm::mat4(1);
    };

    struct static_mesh_renderable_t : renderable_base_t {};
} // namespace nova::renderer

#endif // NOVA_RENDERER_RENDER_OBJECT_HPP
