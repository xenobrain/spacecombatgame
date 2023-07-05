#ifndef ENGINE_RENDERER_RENDERER_SYSTEM_H
#define ENGINE_RENDERER_RENDERER_SYSTEM_H

#include <engine/renderer/renderer_types.h>

namespace xc::renderer {
    auto initialize() -> bool;
    auto uninitialize() -> void;
    auto tick() -> void;

    auto create_shader(char const* vs_source, char const* fs_source) -> shader_t;
    auto bind_shader(shader_t const& shader) -> void;
    auto set_shader_uniform_float3(shader_t const& shader, char const* name, float u1, float u2, float u3) -> void;

    auto swap() -> void;
}

#endif