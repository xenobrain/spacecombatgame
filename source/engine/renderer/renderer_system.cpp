// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <engine/renderer/renderer_system.h>
#include <engine/platform/platform_system.h>
#include <engine/core/array.h>
#include <engine/core/string.h>


// Metal Renderer //////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(RENDERER_METAL)
#import <Metal/Metal.h>

// TODO: fetch without using extern
extern id metalLayer;

namespace xc::renderer {
    auto initialize() -> bool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        id<MTLCommandQueue> command_queue = [device newCommandQueue];
        //metalLayer.device = MTLCreateSystemDefaultDevice();
        //device = metalLayer.device;
        return true;
    }

    auto uninitialize() -> void {

    }

    auto tick() -> void {

    }
};
#endif


// OpenGL Renderer /////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(RENDERER_OPENGL)
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


#if defined(PLATFORM_WINDOWS)
#include <Windows.h>
#include <gl/GL.h>

#define GLDECL APIENTRY

using GLchar = char;
using GLintptr = ptrdiff_t;
using GLsizeiptr = ptrdiff_t;

using PFN_wglCreateContext = HGLRC(WINAPI*)(HDC);
using PFN_wglDeleteContext = BOOL(WINAPI*)(HGLRC);
using PFN_wglGetProcAddress = PROC(WINAPI*)(LPCSTR);
using PFN_wglGetCurrentDC = HDC(WINAPI*)(void);
using PFN_wglGetCurrentContext = HGLRC(WINAPI*)(void);
using PFN_wglMakeCurrent = BOOL(WINAPI*)(HDC, HGLRC);

static HDC hdc;
static HGLRC context;
static const PIXELFORMATDESCRIPTOR pfd = {
        sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,
        32, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
};
#endif // PLATFORM_WINDOWS

#define GL_EXTENSION_LIST \
GL_EXTENSION(GLuint, CreateShader, GLenum type)                                                             \
GL_EXTENSION(GLuint, CreateShaderProgramv, GLenum type, GLsizei count, GLchar const* const* strings)        \
GL_EXTENSION(void, CompileShader, GLuint shader)                                                            \
GL_EXTENSION(void, ShaderSource, GLuint shader, GLsizei count, GLchar const** string, GLint const* length)  \
GL_EXTENSION(void, UseProgram, GLuint program)                                                              \
GL_EXTENSION(GLint, GetUniformLocation, GLuint program, GLchar const* name)                                 \
GL_EXTENSION(void, Uniform3f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2)                           \

#define GL_EXTENSION(ret, name, ...) using PFN_##name = ret GLDECL (__VA_ARGS__); PFN_##name* gl##name;
GL_EXTENSION_LIST
#undef GL_EXTENSION

#define GL_FRAGMENT_SHADER  0x8B30
#define GL_VERTEX_SHADER    0x8B31

// OpenGL Utility //////////////////////////////////////////////////////////////////////////////////////////////////////
auto static compile_shader(char const* source, GLuint type) -> GLuint {
    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // error handling
    return shader;
}

namespace xc::renderer {
    auto initialize() -> bool {
#if defined(PLATFORM_WINDOWS)
        SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);

        auto library = platform::load_library("OpenGL32.dll");

        context = ((PFN_wglCreateContext)platform::load_function(library, "wglCreateContext"))(hdc);
        ((PFN_wglMakeCurrent)platform::load_function(library, "wglMakeCurrent"))(hdc, context);


        auto wgl_get_proc_address = (PFN_wglGetProcAddress)platform::load_function(library, "wglGetProcAddress");



#define GL_EXTENSION(ret, name, ...)                    \
        gl##name = (PFN_##name*)wgl_get_proc_address("gl" #name); \
        if (!gl##name) return false;
        GL_EXTENSION_LIST
#undef GL_EXTENSION

#endif // PLATFORM_WINDOWS


        auto program = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &fs_shader);
        glUseProgram(program);


        auto location = glGetUniformLocation(program, "position");
        glUniform3f(location, 0.f, 0.5f, 0.f);

        return true;
    }

    auto uninitialize() -> void {
#if defined(PLATFORM_WINDOWS)
       // (PFN_wglDeleteContext)(platform::load_function(libary, "wglDeleteContext"))
       //wglDeleteContext(context);
#endif
    }

    auto tick() -> void {
        //glRects(-1, -1, 1, 1);
        swap();
    }

    ///// Shaders //////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto create_shader(char const *vertex_shader_path, char const *fragment_shader_path) -> uint32_t {
#if defined(PLATFORM_WINDOWS)
        auto vertex_shader_file = CreateFile(vertex_shader_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, NULL);
        auto fragment_shader_file = CreateFile(fragment_shader_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                               FILE_ATTRIBUTE_NORMAL, NULL);

        // error handling

        auto constexpr buffer_size = 4096;
        char vertex_shader_source[buffer_size];
        char fragment_shader_source[buffer_size];
        DWORD vs_bytes_read = 0, fs_bytes_read = 0;

        ReadFile(vertex_shader_file, vertex_shader_source, buffer_size, &vs_bytes_read, NULL);
        ReadFile(fragment_shader_file, fragment_shader_source, buffer_size, &fs_bytes_read, NULL);

        CloseHandle(vertex_shader_file);
        CloseHandle(fragment_shader_file);
#endif

        return 0u;
    }

    /// Swapchain //////////////////////////////////////////////////////////////////////////////////////////////////////////
    auto swap() -> void { SwapBuffers(hdc); }
} // namespace xc::renderer
#endif


// Vulkan Renderer /////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(RENDERER_VULKAN)

// Loader //////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <vulkan/vulkan.h>

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

#define VK_FUNCTIONS(VK_FUNCTION)                           \
  VK_FUNCTION(vkEnumerateInstanceLayerProperties)           \
  VK_FUNCTION(vkEnumerateInstanceExtensionProperties)       \
  VK_FUNCTION(vkCreateInstance)                             \

#define VK_INSTANCE_FUNCTIONS(VK_FUNCTION)                  \
  VK_FUNCTION(vkDestroyInstance)                            \
  VK_FUNCTION(vkCreateDebugUtilsMessengerEXT)               \
  VK_FUNCTION(vkDestroyDebugUtilsMessengerEXT)              \
  VK_FUNCTION(vkDestroySurfaceKHR)                          \
  VK_FUNCTION(vkEnumeratePhysicalDevices)                   \
  VK_FUNCTION(vkGetPhysicalDeviceProperties2)               \
  VK_FUNCTION(vkGetPhysicalDeviceFeatures2)                 \
  VK_FUNCTION(vkGetPhysicalDeviceMemoryProperties)          \
  VK_FUNCTION(vkGetPhysicalDeviceFormatProperties)          \
  VK_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)     \
  VK_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)         \
  VK_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)    \
  VK_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)         \
  VK_FUNCTION(vkEnumerateDeviceExtensionProperties)         \
  VK_FUNCTION(vkCreateDevice)                               \
  VK_FUNCTION(vkDestroyDevice)                              \
  VK_FUNCTION(vkGetDeviceQueue)                             \
  VK_FUNCTION(vkGetDeviceProcAddr)                          \

#define VK_DEVICE_FUNCTIONS(VK_FUNCTION)                    \
  VK_FUNCTION(vkSetDebugUtilsObjectNameEXT)                 \
  VK_FUNCTION(vkDeviceWaitIdle)                             \
  VK_FUNCTION(vkQueueSubmit)                                \
  VK_FUNCTION(vkQueuePresentKHR)                            \
  VK_FUNCTION(vkCreateSwapchainKHR)                         \
  VK_FUNCTION(vkDestroySwapchainKHR)                        \
  VK_FUNCTION(vkGetSwapchainImagesKHR)                      \
  VK_FUNCTION(vkAcquireNextImageKHR)                        \
  VK_FUNCTION(vkCreateCommandPool)                          \
  VK_FUNCTION(vkDestroyCommandPool)                         \
  VK_FUNCTION(vkResetCommandPool)                           \
  VK_FUNCTION(vkAllocateCommandBuffers)                     \
  VK_FUNCTION(vkBeginCommandBuffer)                         \
  VK_FUNCTION(vkEndCommandBuffer)                           \
  VK_FUNCTION(vkCreateFence)                                \
  VK_FUNCTION(vkDestroyFence)                               \
  VK_FUNCTION(vkResetFences)                                \
  VK_FUNCTION(vkGetFenceStatus)                             \
  VK_FUNCTION(vkWaitForFences)                              \
  VK_FUNCTION(vkCreateSemaphore)                            \
  VK_FUNCTION(vkDestroySemaphore)                           \
  VK_FUNCTION(vkCmdPipelineBarrier)                         \
  VK_FUNCTION(vkCreateQueryPool)                            \
  VK_FUNCTION(vkDestroyQueryPool)                           \
  VK_FUNCTION(vkCmdResetQueryPool)                          \
  VK_FUNCTION(vkCmdBeginQuery)                              \
  VK_FUNCTION(vkCmdEndQuery)                                \
  VK_FUNCTION(vkCmdWriteTimestamp)                          \
  VK_FUNCTION(vkCmdCopyQueryPoolResults)                    \
  VK_FUNCTION(vkCreateBuffer)                               \
  VK_FUNCTION(vkDestroyBuffer)                              \
  VK_FUNCTION(vkGetBufferMemoryRequirements)                \
  VK_FUNCTION(vkBindBufferMemory)                           \
  VK_FUNCTION(vkCreateImage)                                \
  VK_FUNCTION(vkDestroyImage)                               \
  VK_FUNCTION(vkGetImageMemoryRequirements)                 \
  VK_FUNCTION(vkBindImageMemory)                            \
  VK_FUNCTION(vkCmdCopyBuffer)                              \
  VK_FUNCTION(vkCmdCopyImage)                               \
  VK_FUNCTION(vkCmdBlitImage)                               \
  VK_FUNCTION(vkCmdCopyBufferToImage)                       \
  VK_FUNCTION(vkCmdCopyImageToBuffer)                       \
  VK_FUNCTION(vkCmdFillBuffer)                              \
  VK_FUNCTION(vkCmdClearColorImage)                         \
  VK_FUNCTION(vkCmdClearDepthStencilImage)                  \
  VK_FUNCTION(vkAllocateMemory)                             \
  VK_FUNCTION(vkFreeMemory)                                 \
  VK_FUNCTION(vkMapMemory)                                  \
  VK_FUNCTION(vkCreateSampler)                              \
  VK_FUNCTION(vkDestroySampler)                             \
  VK_FUNCTION(vkCreateRenderPass)                           \
  VK_FUNCTION(vkDestroyRenderPass)                          \
  VK_FUNCTION(vkCmdBeginRenderPass)                         \
  VK_FUNCTION(vkCmdEndRenderPass)                           \
  VK_FUNCTION(vkCreateImageView)                            \
  VK_FUNCTION(vkDestroyImageView)                           \
  VK_FUNCTION(vkCreateFramebuffer)                          \
  VK_FUNCTION(vkDestroyFramebuffer)                         \
  VK_FUNCTION(vkCreateShaderModule)                         \
  VK_FUNCTION(vkDestroyShaderModule)                        \
  VK_FUNCTION(vkCreateDescriptorSetLayout)                  \
  VK_FUNCTION(vkDestroyDescriptorSetLayout)                 \
  VK_FUNCTION(vkCreatePipelineLayout)                       \
  VK_FUNCTION(vkDestroyPipelineLayout)                      \
  VK_FUNCTION(vkCreateDescriptorPool)                       \
  VK_FUNCTION(vkDestroyDescriptorPool)                      \
  VK_FUNCTION(vkAllocateDescriptorSets)                     \
  VK_FUNCTION(vkResetDescriptorPool)                        \
  VK_FUNCTION(vkUpdateDescriptorSets)                       \
  VK_FUNCTION(vkCreatePipelineCache)                        \
  VK_FUNCTION(vkDestroyPipelineCache)                       \
  VK_FUNCTION(vkGetPipelineCacheData)                       \
  VK_FUNCTION(vkCreateGraphicsPipelines)                    \
  VK_FUNCTION(vkCreateComputePipelines)                     \
  VK_FUNCTION(vkDestroyPipeline)                            \
  VK_FUNCTION(vkCmdSetViewport)                             \
  VK_FUNCTION(vkCmdSetScissor)                              \
  VK_FUNCTION(vkCmdPushConstants)                           \
  VK_FUNCTION(vkCmdBindPipeline)                            \
  VK_FUNCTION(vkCmdBindDescriptorSets)                      \
  VK_FUNCTION(vkCmdBindVertexBuffers)                       \
  VK_FUNCTION(vkCmdBindIndexBuffer)                         \
  VK_FUNCTION(vkCmdDraw)                                    \
  VK_FUNCTION(vkCmdDrawIndexed)                             \
  VK_FUNCTION(vkCmdDrawIndirect)                            \
  VK_FUNCTION(vkCmdDrawIndexedIndirect)                     \
  VK_FUNCTION(vkCmdDispatch)                                \
  VK_FUNCTION(vkCmdDispatchIndirect)                        \

#define VK_DECLARE(fn) static PFN_##fn fn;
#define VK_LOAD_FUNCTIONS(fn) fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr({}, #fn));
#define VK_LOAD_DEVICE_FUNCTIONS(fn) fn = reinterpret_cast<PFN_##fn>(vkGetDeviceProcAddr(device, #fn));
#define VK_LOAD_INSTANCE_FUNCTIONS(fn) fn = reinterpret_cast<PFN_##fn>(vkGetInstanceProcAddr(instance, #fn));

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

VK_FUNCTIONS(VK_DECLARE)
VK_DEVICE_FUNCTIONS(VK_DECLARE)
VK_INSTANCE_FUNCTIONS(VK_DECLARE)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// Error Handling //////////////////////////////////////////////////////////////////////////////////////////////////////
#define VK_RESULT_CASE(result) case result: return #result "\n"

auto static result_to_string(VkResult result) -> const char * {
    switch (result) {
        VK_RESULT_CASE(VK_SUCCESS);
        VK_RESULT_CASE(VK_NOT_READY);
        VK_RESULT_CASE(VK_TIMEOUT);
        VK_RESULT_CASE(VK_EVENT_SET);
        VK_RESULT_CASE(VK_EVENT_RESET);
        VK_RESULT_CASE(VK_INCOMPLETE);
        VK_RESULT_CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
        VK_RESULT_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        VK_RESULT_CASE(VK_ERROR_INITIALIZATION_FAILED);
        VK_RESULT_CASE(VK_ERROR_DEVICE_LOST);
        VK_RESULT_CASE(VK_ERROR_MEMORY_MAP_FAILED);
        VK_RESULT_CASE(VK_ERROR_LAYER_NOT_PRESENT);
        VK_RESULT_CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
        VK_RESULT_CASE(VK_ERROR_FEATURE_NOT_PRESENT);
        VK_RESULT_CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
        VK_RESULT_CASE(VK_ERROR_TOO_MANY_OBJECTS);
        VK_RESULT_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
        VK_RESULT_CASE(VK_ERROR_FRAGMENTED_POOL);
        default:
            return "VK_ERROR_UNKNOWN";
    }
}

#undef VL_RESULT_CASE

auto static vk_check(VkResult result, const char *file, int line) -> bool {
    if (result >= 0) return true;
    print("Error in %s:%d - %s", file, line, result_to_string(result));
    return false;
}

#define VK_CHECK(x) do { if (!vk_check(x, __FILE__, __LINE__)) DEBUG_BREAK; } while (0);


// Platforms ///////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(PLATFORM_MACOS)
#include <objc/runtime.h>
#define LIBRARY_NAME "libvulkan.dylib"
#define VK_CREATE_SURFACE vkCreateMetalSurfaceEXT
extern id metalLayer;
auto surface_create_info = VkMetalSurfaceCreateInfoEXT{VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT, {}, {}, metalLayer};

auto static create_surface() -> VkSurfaceKHR {

    auto surface = VkSurfaceKHR{};
    VK_CHECK(vkCreateMetalSurfaceEXT(instance, &surface_create_info, {}, &surface));
    return surface;
}
#endif

#if defined(PLATFORM_LINUX)
#define LIBRARY_NAME "libvulkan.so"

auto static create_surface(VkInstance const& instance) -> VkSurfaceKHR {
    auto surface = VkSurfaceKHR{};
    return surface;
}

#endif

#if defined(PLATFORM_WINDOWS)
#define LIBRARY_NAME "vulkan-1.dll"

extern HINSTANCE hinstance;
auto surface_create_info = VkWin32SurfaceCreateInfoKHR{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, {}, {},
                                                       hinstance}; // hinstance

auto static create_surface(VkInstance const &instance) -> VkSurfaceKHR {
    auto surface = VkSurfaceKHR{};
    VK_CHECK(reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
                     vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"))(instance, &surface_create_info, {},
                                                                                 &surface));
    return surface;
}

#endif


// Objects /////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void *library;

static VkInstance instance;
static VkSurfaceKHR surface;
static VkPhysicalDevice physical_device;

static VkDevice device;
static VkQueue graphics_queue;


// System //////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace xc::renderer {
    auto initialize() -> bool {

        // Load library
        library = platform::load_library(LIBRARY_NAME);
        vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(platform::load_function(library,
                                                                                                    "vkGetInstanceProcAddr"));
        VK_FUNCTIONS(VK_LOAD_FUNCTIONS);


        // Create instance
        char const *extensions[] = {
                VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(PLATFORM_MACOS)
                VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#elif defined(PLATFORM_WINDOWS)
                VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        };

        auto instance_create_info = VkInstanceCreateInfo{
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                {},
                VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
                {},
                {},
                {},
                count_of(extensions), extensions
        };

        VK_CHECK(vkCreateInstance(&instance_create_info, {}, &instance));
        VK_INSTANCE_FUNCTIONS(VK_LOAD_INSTANCE_FUNCTIONS);


        // Create Surface //////////////////////////////////////////////////////////////////////////////////////////////
        surface = create_surface(instance);


        // Pick physical device ////////////////////////////////////////////////////////////////////////////////////////
        auto device_count = 0u;
        auto physical_devices = array<VkPhysicalDevice>{}; // TODO: use standard C types

        VK_CHECK(vkEnumeratePhysicalDevices(instance, &device_count, nullptr));
        physical_devices.resize(device_count);

        VK_CHECK(vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data()));
        // TODO: evaluate devices.  For now, just pick the first
        physical_device = physical_devices[0];


        // Find queue family indices
        auto queue_family_count = 0u;
        auto queue_family_properties = array<VkQueueFamilyProperties>{};
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, {});
        queue_family_properties.resize(queue_family_count);

        auto graphics_queue_index = 0u;
        for (auto i = 0u; i < queue_family_count; ++i) {
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_queue_index = i;
                break;
            }
        }
        queue_family_properties.clear(); // we don't have a destructor for cleanup to avoid SEH

        auto queue_priorities = 1.f;
        auto queue_create_info = VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, {}, {},
                                                         graphics_queue_index, 1, &queue_priorities};

        char const *device_extensions[] = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                #if defined(PLATFORM_MACOS)
                VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
                #endif // PLATFORM_MACOS
        };

        auto device_create_info = VkDeviceCreateInfo{
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                {},
                {},
                1,
                &queue_create_info,
                {}, {},
                count_of(device_extensions), device_extensions,
                {},
        };


        VK_CHECK(vkCreateDevice(physical_device, &device_create_info, {}, &device));
        VK_DEVICE_FUNCTIONS(VK_LOAD_DEVICE_FUNCTIONS);

        vkGetDeviceQueue(device, graphics_queue_index, 0u, &graphics_queue);

        print("Renderer initialization successful\n");

        return true;
    }

    auto uninitialize() -> void {
        vkDestroyInstance(instance, {});
        platform::unload_library(library);
    }

    auto tick() -> void {

    }
}
#endif