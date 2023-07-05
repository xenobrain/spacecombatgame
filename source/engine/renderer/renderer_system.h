#ifndef ENGINE_RENDERER_RENDERER_SYSTEM_H
#define ENGINE_RENDERER_RENDERER_SYSTEM_H

#include <engine/renderer/renderer_types.h>

namespace xc::renderer {
    auto initialize() -> bool;
    auto uninitialize() -> void;
    auto tick() -> void;

    // Shaders
    auto create_shader(char const* vs_source, char const* fs_source) -> shader_t;
    auto bind_shader(shader_t const& shader) -> void;

    auto set_shader_uniform(shader_t const& shader, char const* name, vector<float,1> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, vector<float,2> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, vector<float,3> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, vector<float,4> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, matrix<float,1,1> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, matrix<float,2,2> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, matrix<float,3,3> const& value) -> void;
    auto set_shader_uniform(shader_t const& shader, char const* name, matrix<float,4,4> const& value) -> void;

    auto swap() -> void;
}

#endif