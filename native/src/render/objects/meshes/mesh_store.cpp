/*!
* \brief
*
* \author ddubois
* \date 27-Sep-16.
*/

#include <algorithm>
#include <easylogging++.h>
#include <regex>
#include <iomanip>
#include <utility>
#include "mesh_store.h"
#include "../../nova_renderer.h"
#include "vk_mesh.h"
#include <glm/gtc/matrix_transform.hpp>

namespace nova {
    mesh_store::mesh_store(std::shared_ptr<render_context> context, std::shared_ptr<shader_resource_manager> shader_resources)
            : context(context), shader_resources(shader_resources) {
    }

    std::vector<render_object>& mesh_store::get_meshes_for_material(std::string material_name) {
        if(renderables_grouped_by_material.find(material_name) != renderables_grouped_by_material.end()) {
            return renderables_grouped_by_material.at(material_name);
        } else {
            return default_vector;
        }
    }

    void mesh_store::add_gui_buffers(const char *geo_type, mc_gui_geometry *command) {
        LOG(DEBUG) << "Adding GUI geometry " << command->texture_name << " for geometry type " << geo_type;
        std::string texture_name(command->texture_name);
        texture_name = std::regex_replace(texture_name, std::regex("^textures/"), "");
        texture_name = std::regex_replace(texture_name, std::regex(".png$"), "");
        texture_name = "minecraft:" + texture_name;
        const texture_manager::texture_location tex_location = shader_resources->get_texture_manager().get_texture_location(texture_name);
        glm::vec2 tex_size = tex_location.max - tex_location.min;

        mesh_definition gui_mesh = {};
        //gui_mesh.vertex_data.resize(28 * static_cast<unsigned long>(command->vertex_buffer_size / 9), 0);
        for (int i = 0; i + 8 < command->vertex_buffer_size; i += 9) {
            nova_vertex new_vertex = {};

            // position
            new_vertex.position.x = command->vertex_buffer[i];
            new_vertex.position.y = command->vertex_buffer[i+1];
            new_vertex.position.z = command->vertex_buffer[i+2];

            // UV0
            float u = command->vertex_buffer[i+3] * tex_size.x + tex_location.min.x;
            new_vertex.uv0.x = u;
            float v = command->vertex_buffer[i+4] * tex_size.y + tex_location.min.y;
            new_vertex.uv0.y = v;

            // Color
            new_vertex.color.r = command->vertex_buffer[i+5];
            new_vertex.color.g = command->vertex_buffer[i+6];
            new_vertex.color.b = command->vertex_buffer[i+7];
            new_vertex.color.a = command->vertex_buffer[i+8];

            gui_mesh.vertex_data.push_back(new_vertex);
        }

        gui_mesh.indices.resize(static_cast<unsigned long>(command->index_buffer_size), 0);
        for(int i = 0; i < command->index_buffer_size; i++) {
            gui_mesh.indices[i] = (unsigned int)command->index_buffer[i];
        }

        gui_mesh.vertex_format = format::POS_UV_COLOR;
        gui_mesh.type = geometry_type::gui;
        if(!strcmp(geo_type, "gui_background")) {
            gui_mesh.type = geometry_type::gui_background;

        } else if(!strcmp(geo_type, "gui_text")) {
            gui_mesh.type = geometry_type::text;
        }

        geometry_to_upload_lock.lock();
        geometry_to_upload.emplace(geo_type, gui_mesh);
        geometry_to_upload_lock.unlock();
    }

    void mesh_store::add_chunk_render_object(std::string filter_name, mc_chunk_render_object &chunk) {
        mesh_definition def = {};
        auto& vertex_data = def.vertex_data;

        for(int i = 0; i < chunk.vertex_buffer_size / sizeof(mc_block_vertex); i++) {
            const mc_block_vertex& cur_vertex = chunk.vertex_data[i];
            nova_vertex new_vertex = {};

            // Position
            new_vertex.position.x = cur_vertex.x;
            new_vertex.position.y = cur_vertex.y;
            new_vertex.position.z = cur_vertex.z;

            new_vertex.uv0.x = cur_vertex.uv0_u;
            new_vertex.uv0.y = cur_vertex.uv0_v;

            new_vertex.color.r = (float)cur_vertex.r / 255.0f;
            new_vertex.color.g = (float)cur_vertex.g / 255.0f;
            new_vertex.color.b = (float)cur_vertex.b / 255.0f;
            new_vertex.color.a = (float)cur_vertex.a / 255.0f;

            new_vertex.uv1.x = cur_vertex.uv1_u;
            new_vertex.uv1.y = cur_vertex.uv1_v;

            vertex_data.push_back(new_vertex);
        }

        for(int i = 0; i < chunk.index_buffer_size; i++) {
            def.indices.push_back(chunk.indices[i]);
        }

        def.vertex_format = format::all_values()[chunk.format];
        def.position = {chunk.x, chunk.y, chunk.z};
        def.id = chunk.id;

        def.type = geometry_type::block;

        remove_chunk_render_object(filter_name,chunk);

        geometry_to_upload_lock.lock();
        geometry_to_upload.emplace(filter_name, def);
        geometry_to_upload_lock.unlock();
    }

    void mesh_store::add_fullscreen_quad_for_material(const std::string &material_name) {
        if(has_fullscreen_quad.find(material_name) != has_fullscreen_quad.end()) {
            if(has_fullscreen_quad.at(material_name)) {
                return;
            }
        }

        mesh_definition quad;
        quad.vertex_data = std::vector<nova_vertex>{
                nova_vertex{
                        {-1.0f, -3.0f, 0.0f},                 // Position
                        {0.0f, -1.0f},                               // UV0
                        {0.0f, 0.0f},                               // MidTexCoord
                        0,                                                      // VirtualTextureId
                        {1.0f, 1.0f, 1.0f, 1.0f},     // Color
                        {0.0f, 1.0f},                               // UV1
                        {0.0f, 0.0f, 0.0f},                  // Normal
                        {0.0f, 0.0f, 0.0f},                  // Tangent
                        {0.0f, 0.0f, 0.0f, 0.0f}     // McEntityId
                },

                nova_vertex{
                        {3.0f,  1.0f,  0.0f},                  // Position
                        {2.0f, 1.0f},                               // UV0
                        {0.0f, 0.0f},                               // MidTexCoord
                        0,                                                      // VirtualTextureId
                        {1.0f, 1.0f, 1.0f, 1.0f},     // Color
                        {0.0f, 0.0f},                               // UV1
                        {0.0f, 0.0f, 0.0f},                  // Normal
                        {0.0f, 0.0f, 0.0f},                  // Tangent
                        {0.0f, 0.0f, 0.0f, 0.0f}     // McEntityId
                },

                nova_vertex{
                        {-1.0f, 1.0f,  0.0f},                  // Position
                        {0.0f, 1.0f},                               // UV0
                        {0.0f, 0.0f},                               // MidTexCoord
                        0,                                                      // VirtualTextureId
                        {1.0f, 1.0f, 1.0f, 1.0f},     // Color
                        {0.0f, 0.0f},                 // UV1
                        {0.0f, 0.0f, 0.0f},           // Normal
                        {0.0f, 0.0f, 0.0f},           // Tangent
                        {0.0f, 0.0f, 0.0f, 0.0f}     // McEntityId
                }
        };

        quad.indices = {2, 1, 0};
        quad.vertex_format = format::POS_COLOR_UV_LIGHTMAPUV_NORMAL_TANGENT;
        quad.position = glm::vec3(0);
        quad.id = 0;

        geometry_to_upload_lock.lock();
        geometry_to_upload.emplace(material_name, quad);
        geometry_to_upload_lock.unlock();

        has_fullscreen_quad[material_name] = true;
    }

    void mesh_store::remove_gui_render_objects() {
        remove_render_objects([](auto& render_obj) {return render_obj.type == geometry_type::gui || render_obj.type == geometry_type::text || render_obj.type == geometry_type::gui_background;});
    }

    void mesh_store::remove_render_objects(std::function<bool(render_object&)> filter) {
        geometry_to_remove.push(filter);
    }

    void mesh_store::remove_old_geometry() {
        LOG(TRACE) << "Removing old geometry";
        while(!geometry_to_remove.empty()) {
            auto &filter = geometry_to_remove.front();
            auto per_model_buffer = shader_resources->get_uniform_buffers().get_per_model_buffer();
            for (auto &group : renderables_grouped_by_material) {
                const std::vector<render_object>::iterator& removed_elements_ref = std::remove_if(group.second.begin(), group.second.end(), filter);
                group.second.erase(removed_elements_ref, group.second.end());
            }

            geometry_to_remove.pop();
        }
    }

    void mesh_store::upload_new_geometry() {
        LOG(TRACE) << "Uploading " << geometry_to_upload.size() << " new objects";
        geometry_to_upload_lock.lock();
        while(!geometry_to_upload.empty()) {
            const auto& entry = geometry_to_upload.front();
            const std::string& material_name = std::get<0>(entry);
            const auto& def = std::get<1>(entry);

            render_object obj(context, *shader_resources, sizeof(glm::mat4), def);
            LOG(TRACE) << "Created new render object " << obj.id << " for material " << material_name;
            glm::mat4 model_matrix(1.0);
            model_matrix = glm::translate(model_matrix, def.position);
            obj.write_new_model_ubo(model_matrix);
            LOG(TRACE) << "Updated the per-model UBO with this mesh's model matrix";
            obj.upload_model_matrix(context->device);
            LOG(TRACE) << "Uploaded model matrix UBO to the GPU";

            obj.type = def.type;
            obj.parent_id = def.id;
            obj.position = def.position;

            obj.bounding_box.center = {def.position.x + 8, def.position.y + 8, def.position.z + 8};
            obj.bounding_box.extents = {16, 16, 16};   // TODO: Make these values come from Minecraft

            if(renderables_grouped_by_material.find(material_name) == renderables_grouped_by_material.end()) {
                renderables_grouped_by_material[material_name] = std::vector<render_object>{};
                renderables_grouped_by_material[material_name].reserve(15);
                LOG(TRACE) << "Created new list of render objects for material " << material_name;
            }
            renderables_grouped_by_material.at(material_name).push_back(std::move(obj));
            LOG(TRACE) << "Moved the render object into the boi";

            geometry_to_upload.pop();
        }
        geometry_to_upload_lock.unlock();
    }

    void mesh_store::remove_chunk_render_object(std::string filter_name, mc_chunk_render_object &chunk) {
        remove_render_objects([&](render_object& obj) ->bool{
            bool r = (static_cast<int>(obj.position.x) == static_cast<int>(chunk.x)) &&
                   (static_cast<int>(obj.position.y) == static_cast<int>(chunk.y)) &&
                   (static_cast<int>(obj.position.z) == static_cast<int>(chunk.z));
            return r;
        });
    }

    void mesh_store::remove_render_objects_with_parent(long parent_id) {
        remove_render_objects([&](render_object& obj) { return obj.parent_id == parent_id; });
    }
}
