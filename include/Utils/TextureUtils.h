#pragma once
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.h>
#include "../include/texture/Texture.h"

/**
 * Texture utilities specifically for PBR workflow
 */
class TextureUtils {
public:
    /**
     * Create a default normal map (pointing up in tangent space)
     */
    static glm::vec2 integrateBRDF(float NoV, float roughness);

    bool initWithExistingImage(
    VkImage image, 
    VkDeviceMemory memory,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    uint32_t layerCount,
    VkImageViewType viewType,
    VkImageLayout initialLayout);
    
    static std::shared_ptr<Texture> createDefaultNormalMap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue);
    
    /**
     * Create a default metallic-roughness map with given values
     * @param metallic Value for metallic channel (0-1)
     * @param roughness Value for roughness channel (0-1)
     */
    static std::shared_ptr<Texture> createDefaultMetallicRoughnessMap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        float metallic = 0.0f,
        float roughness = 0.5f);
    
    /**
     * Create a solid color texture
     */
    static std::shared_ptr<Texture> createSolidColorTexture(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const glm::vec4& color);
    
    /**
     * Combine separate metallic and roughness textures into a single texture
     * (metallic in blue channel, roughness in green channel)
     */
    static std::shared_ptr<Texture> combineMetallicRoughness(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        std::shared_ptr<Texture> metallicTexture,
        std::shared_ptr<Texture> roughnessTexture,
        float defaultMetallic = 0.0f,
        float defaultRoughness = 0.5f);
    
    /**
     * Generate a normal map from a height map
     */
    static std::shared_ptr<Texture> generateNormalFromHeight(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        std::shared_ptr<Texture> heightMap,
        float strength = 1.0f);
    
    /**
     * Create a cubemap from 6 individual textures
     */
    static std::shared_ptr<Texture> createCubemap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::array<std::string, 6>& facePaths);
    
    /**
     * Create a BRDF look-up texture for PBR lighting
     */
    static std::shared_ptr<Texture> createBRDFLookUpTexture(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        uint32_t resolution = 512);

    /**
     * Create an environment cubemap from an HDR file
     */
    static std::shared_ptr<Texture> createEnvironmentCubemap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, 
        VkQueue graphicsQueue,
        const std::string& hdrFilePath);
        
    /**
     * Creates a fallback environment cubemap when no HDR file is available
     */
    static std::shared_ptr<Texture> createDefaultEnvironmentCubemap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, 
        VkQueue graphicsQueue);

    /**
     * Create an irradiance map from an environment map for diffuse IBL
     */
    static std::shared_ptr<Texture> createIrradianceMap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, 
        VkQueue graphicsQueue,
        std::shared_ptr<Texture> environmentMap);

    /**
     * Create a prefiltered environment map for specular IBL
     */
    static std::shared_ptr<Texture> createPrefilterMap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue, 
        std::shared_ptr<Texture> environmentMap);

private:
    // Helper functions for IBL
    static void equirectangularToCubemapFace(
        float* equirectangularData, int equiWidth, int equiHeight, int channels,
        float* faceData, int faceSize, int faceIndex);
        
    static glm::vec3 specularConvolution(
        std::shared_ptr<Texture> envMap, 
        const glm::vec3& reflection, 
        float roughness, 
        int sampleCount);
        
    static glm::vec3 diffuseConvolution(
        std::shared_ptr<Texture> envMap, 
        const glm::vec3& normal, 
        int sampleCount);
        
    
    
    static float distributionGGX(float NoH, float alphaSquared);
    
    static std::vector<glm::vec3> generateHemisphereSamples(
        const glm::vec3& normal, 
        int sampleCount);
        
    static std::vector<glm::vec3> generateImportanceSamples(
        const glm::vec3& reflection, 
        float roughness, 
        int sampleCount);
        
    static glm::vec3 sampleEnvironmentMap(
        std::shared_ptr<Texture> envMap, 
        const glm::vec3& direction);
        
    // Utility functions for Vulkan operations
    static void createBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
        
    static void copyBufferToImage(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height,
        uint32_t baseArrayLayer = 0,
        uint32_t mipLevel = 0);
        
    static void transitionImageLayout(
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = 1,
        uint32_t baseMipLevel = 0,
        uint32_t levelCount = 1);
        
    static uint32_t findMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties);
};