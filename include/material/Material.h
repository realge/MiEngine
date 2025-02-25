#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "../texture/Texture.h"

struct Material {
    // Base properties
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    float alpha = 1.0f;
    
    // PBR properties (for future expansion)
    float metallic = 0.0f;
    float roughness = 0.5f;
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    
    // Textures
    std::shared_ptr<Texture> diffuseTexture = nullptr;
    std::shared_ptr<Texture> normalTexture = nullptr;
    
    // Settings
    bool useTexture = false;
    
    // Constructor with default values
    Material() = default;
    
    // Constructor with diffuse color
    Material(const glm::vec3& color) : diffuseColor(color) {}
    
    // Constructor with texture
    Material(std::shared_ptr<Texture> texture) 
        : diffuseTexture(texture), useTexture(texture != nullptr) {}
        
    // Get texture binding info for updating descriptor sets
    VkDescriptorImageInfo getTextureImageInfo() const {
        VkDescriptorImageInfo imageInfo{};
        if (diffuseTexture) {
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = diffuseTexture->getImageView();
            imageInfo.sampler = diffuseTexture->getSampler();
        }
        return imageInfo;
    }
};