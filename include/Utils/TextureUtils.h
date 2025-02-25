#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/vec4.hpp>
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
   static  std::shared_ptr<Texture> createSolidColorTexture(
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
   static  std::shared_ptr<Texture> generateNormalFromHeight(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        std::shared_ptr<Texture> heightMap,
        float strength = 1.0f);
    
    /**
     * Create a cubemap from 6 individual textures
     */
   static  std::shared_ptr<Texture> createCubemap(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::array<std::string, 6>& facePaths);
    
    /**
     * Create a BRDF look-up texture for PBR lighting
     */
   static  std::shared_ptr<Texture> createBRDFLookUpTexture(
        VkDevice device, 
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        uint32_t resolution = 512);
   static std::shared_ptr<Texture> createEnvironmentCubemap(VkDevice device, VkPhysicalDevice physicalDevice,
                                                      VkCommandPool commandPool, VkQueue graphicsQueue,
                                                      const std::string& hdrFilePath);
   static std::shared_ptr<Texture> createIrradianceMap(VkDevice device, VkPhysicalDevice physicalDevice,
                                                 VkCommandPool commandPool, VkQueue graphicsQueue,
                                                 std::shared_ptr<Texture> environmentMap);
   static std::shared_ptr<Texture> createPrefilterMap(VkDevice device, VkPhysicalDevice physicalDevice,
                                                VkCommandPool commandPool,
                                                VkQueue graphicsQueue, std::shared_ptr<Texture> environmentMap);
};
