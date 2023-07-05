#include <engine/renderer/renderer_system.h>
#include <engine/platform/platform_system.h>

// Linux Platform //////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(PLATFORM_LINUX)
#endif

// MacOS Platform //////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(PLATFORM_MACOS)
#endif

// Windows Platform ////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#include <gl/GL.h>

extern HDC hdc;
static HGLRC context;
static HMODULE library;

#define GL_API APIENTRY
#define GL_LOAD_EXTENSION

using PFN_wglCreateContext  = HGLRC(WINAPI*)(HDC);
using PFN_wglDeleteContext  = BOOL(WINAPI*)(HGLRC);
using PFN_wglGetProcAddress = PROC(WINAPI*)(LPCSTR);
using PFN_wglMakeCurrent    = BOOL(WINAPI*)(HDC, HGLRC);

static PFN_wglGetProcAddress gl_load_function;

// Temp
using PFN_glRects = void (APIENTRY*)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static PFN_glRects gl_rects;
// Temp

auto static constexpr pixel_format = PIXELFORMATDESCRIPTOR{
        sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
        32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
};


auto static create_context() -> void {
    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pixel_format), &pixel_format);

    library = reinterpret_cast<HMODULE>(xc::platform::load_library("OpenGL32.dll"));
    context = (reinterpret_cast<PFN_wglCreateContext>(xc::platform::load_function(library, "wglCreateContext")))(hdc);

    reinterpret_cast<PFN_wglMakeCurrent>(xc::platform::load_function(library, "wglMakeCurrent"))(hdc, context);
    gl_load_function = reinterpret_cast<PFN_wglGetProcAddress>(xc::platform::load_function(library, "wglGetProcAddress"));
}

auto static destroy_context() -> void {
    reinterpret_cast<PFN_wglDeleteContext>(xc::platform::load_function(library, "wglDeleteContext"))(context);
}

#endif // PLATFORM_WINDOWS


// OpenGL Types ////////////////////////////////////////////////////////////////////////////////////////////////////////
using GLchar = char;
using GLintptr = ptrdiff_t;
using GLsizeiptr = ptrdiff_t;

#define GL_FRAGMENT_SHADER  0x8B30
#define GL_VERTEX_SHADER    0x8B31


// OpenGL Extensions ///////////////////////////////////////////////////////////////////////////////////////////////////
#define GL_EXTENSION_LIST \
GL_EXTENSION(GLuint, CreateShader, GLenum type)                                                             \
GL_EXTENSION(GLuint, CreateShaderProgramv, GLenum type, GLsizei count, GLchar const* const* strings)        \
GL_EXTENSION(void, CompileShader, GLuint shader)                                                            \
GL_EXTENSION(void, ShaderSource, GLuint shader, GLsizei count, GLchar const** string, GLint const* length)  \
GL_EXTENSION(void, UseProgram, GLuint program)                                                              \
GL_EXTENSION(GLint, GetUniformLocation, GLuint program, GLchar const* name)                                 \
GL_EXTENSION(void, Uniform3f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)                           \

#define GL_EXTENSION(ret, name, ...) using PFN_##name = ret GL_API (__VA_ARGS__); PFN_##name* gl##name;
GL_EXTENSION_LIST
#undef GL_EXTENSION


namespace xc::renderer {
    // Renderer System /////////////////////////////////////////////////////////////////////////////////////////////////////
    auto initialize() -> bool {
        create_context();

        #define GL_EXTENSION(ret, name, ...)                    \
        gl##name = (PFN_##name*)gl_load_function("gl" #name);   \
        if (!gl##name) return false;
        GL_EXTENSION_LIST

        gl_rects = reinterpret_cast<PFN_glRects>(xc::platform::load_function(library, "glRects")); // Temp

        return true;
    }

    auto tick() -> void {
        gl_rects(-1, -1, 1, 1);
        swap();
    }

    auto swap() -> void {
        SwapBuffers(hdc);
    }


    // Resources //////////////////////////////////////////////////////////////////////////////////////////////////////
    auto create_shader(char const* vs_source, char const* fs_source) -> shader_t {
        auto program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fs_source);
        // TODO: error handling
        return {program};
    }

    auto set_shader_uniform_float3(shader_t const& shader, char const* name, float u1, float u2, float u3) -> void {
        glUseProgram(shader.id);
        glUniform3f(glGetUniformLocation(shader.id, name), u1, u2, u3);
    }
} // namespace xc::renderer