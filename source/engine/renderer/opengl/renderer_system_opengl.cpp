#include <engine/renderer/renderer_system.h>
#include <engine/platform/platform_system.h>

auto static constexpr fs_shader = R"(
#version 330

out vec4 outColor;
uniform vec3 position;

const vec2 uResolution = vec2(1280.0, 720.0);

const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float EPSILON = 0.0001;


// SDF Functions
float sphere_sdf(vec3 sample_point, float radius)
{
  return length(sample_point) - radius;
}

float intersect_sdf(float dist0, float dist1)
{
  return max(dist0, dist1);
}

float union_sdf(float dist0, float dist1)
{
  return min(dist0, dist1);
}

float difference_sdf(float dist0, float dist1)
{
  return max(dist0, -dist1);
}


// Scene
float scene(vec3 sample_point)
{
  float sphere_a = sphere_sdf(sample_point, 0.4);
  float sphere_b = sphere_sdf(sample_point - vec3(-0.3, 0.0, 0.0), 0.4);
  return union_sdf(sphere_a, sphere_b);
}


// Raymarching
float raymarch(vec3 eye, vec3 raymarch_direction, float start, float end)
{
  float depth = start;
  for (int i = 0; i < MAX_MARCHING_STEPS; ++i)
  {
    float dist = scene(eye + depth * raymarch_direction);

    // We're inside the surface
    if (dist < EPSILON) return depth;

    // Step along the ray
    depth += dist;

    // Reached the end
    if (depth >= end) break;
  }

  return end;
}

vec3 ray_direction(float fov, vec2 size, vec2 frag_coord)
{
  vec2 xy = frag_coord - size / 2.0;
  float z = size.y * 0.5 / tan(radians(fov) / 2.0);
  return normalize(vec3(xy, -z));
}



// Lighting
vec3 estimate_normal(vec3 p)
{
  return normalize(vec3(
    scene(vec3(p.x + EPSILON, p.y, p.z)) - scene(vec3(p.x - EPSILON, p.y, p.z)),
    scene(vec3(p.x, p.y + EPSILON, p.z)) - scene(vec3(p.x, p.y - EPSILON, p.z)),
    scene(vec3(p.x, p.y, p.z + EPSILON)) - scene(vec3(p.x, p.y, p.z - EPSILON))
  ));
}


// k_a: Ambient
// k_d: Diffuse
// k_s: Specular
// alpha: specular
// p: point being lit
// eye: camera position
vec3 phong_contribution(vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye, vec3 light_position, vec3 light_intensity)
{
  vec3 N = estimate_normal(p);
  vec3 L = normalize(light_position - p);
  vec3 V = normalize(eye - p);
  vec3 R = normalize(reflect(-L, N));

  float dot_ln = clamp(dot(L, N), 0.0, 1.0);
  float dot_rv = dot(R, V);

  // Light not visible from this point
  if (dot_ln < 0.0) return vec3(0.0, 0.0, 0.0);

  if (dot_rv < 0.0) return light_intensity * (k_d * dot_ln);

  return light_intensity * (k_d * dot_ln + k_s * pow(dot_rv, alpha));
}

vec3 phong(vec3 k_a, vec3 k_d, vec3 k_s, float alpha, vec3 p, vec3 eye)
{
  const vec3 light_position = vec3(4.0, 2.0, 4.0);
  const vec3 light_intensity = vec3(0.4, 0.4, 0.4);

  const vec3 ambient_light = vec3(0.25, 0.25, 0.25);

  vec3 color = ambient_light * k_a;

  color += phong_contribution(k_d, k_s, alpha, p, eye, light_position, light_intensity);

 return color;
}


// Camera
mat4 look_at(vec3 position, vec3 target, vec3 up)
{
  vec3 z = normalize(target - position);
  vec3 x = normalize(cross(z, up));
  vec3 y = cross(x, z);

  return mat4(
    vec4(x, 0.0),
    vec4(y, 0.0),
    vec4(-z, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0)
  );
}


// Main
void main() {
  vec3 camera_direction = ray_direction(45.0, uResolution, gl_FragCoord.xy);
  vec3 camera_position = vec3(0.0, 0.0, 5.0) - position;

  mat4 view_to_world = look_at(camera_position, vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
  vec3 world_direction = (view_to_world * vec4(camera_direction, 0.0)).xyz;

  float dist = raymarch(camera_position, camera_direction, MIN_DIST, MAX_DIST);

  if (dist > MAX_DIST - EPSILON)
  {
    // didn't hit anything
    vec3 color = vec3(0.0, 0.0, 0.0);
    return;
  }

  vec3 p = camera_position + dist * camera_direction;


  // Light colors
  const vec3 ambient = vec3(0.2, 0.2, 0.2);
  const vec3 diffuse = vec3(0.7, 0.2, 0.2);
  const vec3 specular = vec3(1.0, 1.0, 1.0);
  const float power = 10.0;

  vec3 color = phong(ambient, diffuse, specular, power, p, camera_direction);

  outColor = vec4(color, 1.0);
}
)";

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


// Renderer System /////////////////////////////////////////////////////////////////////////////////////////////////////
namespace xc::renderer {
    auto initialize() -> bool {
        create_context();

        #define GL_EXTENSION(ret, name, ...)                    \
        gl##name = (PFN_##name*)gl_load_function("gl" #name);   \
        if (!gl##name) return false;
        GL_EXTENSION_LIST


        // Temp
        gl_rects = reinterpret_cast<PFN_glRects>(xc::platform::load_function(library, "glRects"));
        auto program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fs_shader);
        glUseProgram(program);
        auto location = glGetUniformLocation(program, "position");
        glUniform3f(location, 0.f, 0.5f, 0.f);
        // Temp

        return true;
    }

    auto tick() -> void {
        gl_rects(-1, -1, 1, 1);
        swap();
    }

    auto swap() -> void {
        SwapBuffers(hdc);
    }
} // namespace xc::renderer