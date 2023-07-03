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
#endif


// Vulkan Renderer /////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(RENDERER_VULKAN)

// Loader //////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <vulkan/vulkan.h>

#define VK_FUNCTIONS(VK_FUNCTION)                           \
  VK_FUNCTION(vkEnumerateInstanceLayerProperties)           \
  VK_FUNCTION(vkEnumerateInstanceExtensionProperties)       \
  VK_FUNCTION(vkCreateInstance)                             \

#define VK_INSTANCE_FUNCTIONS(VK_FUNCTION)                  \
  VK_FUNCTION(vkCreateMetalSurfaceEXT)                      \
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

VK_FUNCTIONS(VK_DECLARE)
VK_DEVICE_FUNCTIONS(VK_DECLARE)
VK_INSTANCE_FUNCTIONS(VK_DECLARE)

#pragma clang diagnostic pop
#endif


#if defined(PLATFORM_MACOS)
#include <objc/runtime.h>
#define LIBRARY_NAME "libvulkan.dylib"
extern id metalLayer;
auto surface_create_info = VkMetalSurfaceCreateInfoEXT{
        VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        {},
        {},
        metalLayer
};
#endif


// Error Handling //////////////////////////////////////////////////////////////////////////////////////////////////////
#define VK_RESULT_CASE(result) case result: return #result "\n"
auto static result_to_string(VkResult result) -> const char* {
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
        default: return "VK_ERROR_UNKNOWN";
    }
}
#undef VL_RESULT_CASE

auto static vk_check(VkResult result, const char* file, int line) -> bool {
    if (result >= 0) return true;
    print("Error in %s:%d - %s", file, line, result_to_string(result));
    return false;
}

#define VK_CHECK(x) do { if (!vk_check(x, __FILE__, __LINE__)) DEBUG_BREAK; } while (0);


// Objects /////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void* library;

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
        auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(platform::load_function(library,"vkGetInstanceProcAddr"));
        VK_FUNCTIONS(VK_LOAD_FUNCTIONS);


        // Create instance
        char const* extensions[] = {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_EXT_METAL_SURFACE_EXTENSION_NAME,
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
        VK_CHECK(vkCreateMetalSurfaceEXT(instance, &surface_create_info, {}, &surface));


        // Pick physical device ////////////////////////////////////////////////////////////////////////////////////////
        auto device_count = 0u;
        auto physical_devices = array<VkPhysicalDevice>{};
        auto t = vkEnumeratePhysicalDevices;

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

        auto queue_priorities = 1.f;
        auto queue_create_info = VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,{},{},graphics_queue_index,1,&queue_priorities};

        char const* device_extensions[] = {VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME};
        auto device_create_info = VkDeviceCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            {},
            {},
            1,
            &queue_create_info,
            {},{},
            count_of(device_extensions),device_extensions,
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