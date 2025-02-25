#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <array> 
#include <vector>
#include <optional>
#include "../include/mesh/Mesh.h"
#include <fstream>
#include <chrono>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../include/Utils/CommonVertex.h"
#include "include/texture/Texture.h"
#include <set>
#include "include/Utils/TextureUtils.h"
#include "include/scene/Scene.h"


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

enum class RenderMode {
    Standard,
    PBR,
    PBR_IBL
};

struct MaterialUniformBuffer {
    alignas(16) glm::vec4 baseColorFactor;    // RGB + alpha
    alignas(4) float metallicFactor;
    alignas(4) float roughnessFactor;
    alignas(4) float aoStrength;
    alignas(4) float emissiveStrength;
    
    // Flags for which textures are available (0 = not used, 1 = used)
    alignas(4) int hasBaseColorMap;
    alignas(4) int hasNormalMap;
    alignas(4) int hasMetallicRoughnessMap;
    alignas(4) int hasOcclusionMap;
    alignas(4) int hasEmissiveMap;
    
    // Additional parameters
    alignas(4) float alphaCutoff;
    alignas(4) int alphaMode;  // 0 = opaque, 1 = mask, 2 = blend
    alignas(8) glm::vec2 padding;
};

class VulkanRenderer
{
   
    //====================================================================
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 cameraPos;
        float time;
    };
public: //texture related
    // Update texture descriptor
    void updateTextureDescriptor(const VkDescriptorImageInfo& imageInfo);
    
    // Get the current descriptor set
    VkDescriptorSet getCurrentDescriptorSet() const { 
        return descriptorSets[currentFrame]; 
    }
public: //light related
    struct LightData {
        alignas(16) glm::vec4 position;   // w=1 for point, w=0 for directional
        alignas(16) glm::vec4 color;      // rgb + intensity
        alignas(4) float radius;
        alignas(4) float falloff;
        alignas(8) glm::vec2 padding;
    };

    #define MAX_LIGHTS 16

    struct LightUniformBuffer {
        alignas(4) int lightCount;
        alignas(4) int padding[3];
        alignas(16) LightData lights[MAX_LIGHTS];
        alignas(16) glm::vec4 ambientColor;
    };

    // Add light buffer members
    std::vector<VkBuffer> lightUniformBuffers;
    std::vector<VkDeviceMemory> lightUniformBuffersMemory;
    std::vector<void*> lightUniformBuffersMapped;

    void createLightUniformBuffers();
    void updateLights();
    void setupIBL(const std::string& hdriPath);
    void createIBLDescriptorSetLayout();
    void createPBRPipeline();
    void createIBLDescriptorSets();
    void drawWithIBL(VkCommandBuffer commandBuffer);
    void cleanupIBL();

public:
    void createDefaultTextures();
    void createMaterialUniformBuffers();
    void updateMaterialProperties(const Material& material);
    void updateAllTextureDescriptors(const Material& material);

private:

    // IBL-related resources
    VkDescriptorSetLayout iblDescriptorSetLayout;
    VkDescriptorSet iblDescriptorSet;
    VkPipelineLayout pbrPipelineLayout;  // Pipeline layout for PBR rendering
    VkPipeline pbrPipeline;              // PBR pipeline
    
    // Current rendering mode
    RenderMode renderMode = RenderMode::Standard;
    std::shared_ptr<Texture> defaultAlbedoTexture;
    std::shared_ptr<Texture> defaultNormalTexture;
    std::shared_ptr<Texture> defaultMetallicRoughnessTexture;
    std::shared_ptr<Texture> defaultOcclusionTexture;
    std::shared_ptr<Texture> defaultEmissiveTexture;
    
    // Material UBO buffers
    std::vector<VkBuffer> materialUniformBuffers;
    std::vector<VkDeviceMemory> materialUniformBuffersMemory;
    std::vector<void*> materialUniformBuffersMapped;
    
    std::shared_ptr<Texture> environmentMap; // Cubemap for reflections
    std::shared_ptr<Texture> irradianceMap;  // Diffuse environment lighting
    std::shared_ptr<Texture> prefilterMap;   // Prefiltered specular environment
    std::shared_ptr<Texture> brdfLUT;

    void drawWithPBR(VkCommandBuffer commandBuffer, const glm::mat4& view, const glm::mat4& proj);
private:
    std::shared_ptr<Texture> defaultTexture;
    // Create a default white texture
    void createDefaultTexture();
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // Helper methods for depth resources
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                VkImageTiling tiling,
                                VkFormatFeatureFlags features);
    bool hasStencilComponent(VkFormat format);
    void createImage(uint32_t width, uint32_t height, VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkImage& image,
                    VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createDepthResources();
public:
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkCommandPool getCommandPool() const { return commandPool; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    void recreateSwapChain();
    void cleanupSwapChain();

private:
   //TODO::need to refactor the code later
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    // Uniform buffer members
    float rotationAngle = 0.0f;

    // Descriptor members
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<MeshData> meshes;

    std::unique_ptr<Scene> scene;
    
    // Camera properties
    glm::vec3 cameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 10.0f;

    //====================================================================
    
   
    
        GLFWwindow* window;
        VkInstance instance;
        VkSurfaceKHR surface;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device;
        VkQueue graphicsQueue, presentQueue;
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;

        // Synchronization objects
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        size_t currentFrame = 0;
        const int MAX_FRAMES_IN_FLIGHT = 2;

  
    
   
    public:
        VulkanRenderer();
       
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        
        void initVulkan();
        void createInstance();
        void createSurface();
        void pickPhysicalDevice();

        void createLogicalDevice();

        void createSwapChain();

        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        
        void createCommandBuffers();
        void createSyncObjects();
        void run();
        void initWindow();
        void mainLoop();
        void drawFrame();
        void createDescriptorSetLayout();
       
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
    void updateMVPMatrices(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);
        void cleanup();

        bool isDeviceSuitable(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
public:

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
                }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("failed to open file: " + filename);
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS)
            throw std::runtime_error("failed to create shader module!");
        return module;
    }
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

};

