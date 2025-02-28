#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

#include "../texture/Texture.h"


enum class TextureType {
    Diffuse,      // Base color/albedo
    Normal,       // Normal map
    Metallic,     // Metallic map
    Roughness,    // Roughness map
    AmbientOcclusion, // Ambient occlusion map
    Emissive,     // Emission map
    Height,       // Height/displacement map
    Specular      // Specular map (for non-PBR workflow)
};

struct Material {
private:
    // Descriptor set for this material
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
public:
    // Base properties
    void setDescriptorSet(VkDescriptorSet set) { descriptorSet = set; }
    VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    float alpha = 1.0f;
    
    // PBR properties
    float metallic = 0.0f;
    float roughness = 0.5f;
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float emissiveStrength = 1.0f;
    
    // Legacy texture references for backward compatibility
    std::shared_ptr<Texture> diffuseTexture = nullptr;
    std::shared_ptr<Texture> normalTexture = nullptr;
    
    // New texture storage using enum as key
    std::unordered_map<TextureType, std::shared_ptr<Texture>> textures;
    
    // Settings
    bool useTexture = false;
    
    // Constructor with default values
    Material() = default;
    
    // Constructor with diffuse color
    Material(const glm::vec3& color) : diffuseColor(color) {}
    
    // Constructor with texture
    Material(std::shared_ptr<Texture> texture) 
        : diffuseTexture(texture), useTexture(texture != nullptr) {
        if (texture) {
            textures[TextureType::Diffuse] = texture;
        }
    }
    
    
    // Add a texture of specific type
    void setTexture(TextureType type, std::shared_ptr<Texture> texture) {
        textures[type] = texture;
        
        // Update legacy references for compatibility
        if (type == TextureType::Diffuse) {
            diffuseTexture = texture;
            useTexture = texture != nullptr;
        }
        else if (type == TextureType::Normal) {
            normalTexture = texture;
        }
    }
    
    // Check if material has a specific texture type
    bool hasTexture(TextureType type) const {
        auto it = textures.find(type);
        return it != textures.end() && it->second != nullptr;
    }
    
    // Get a texture of specific type
    std::shared_ptr<Texture> getTexture(TextureType type) const {
        auto it = textures.find(type);
        if (it != textures.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Get texture binding info for updating descriptor sets (for diffuse texture)
    VkDescriptorImageInfo getTextureImageInfo() const {
        VkDescriptorImageInfo imageInfo{};
        if (diffuseTexture) {
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = diffuseTexture->getImageView();
            imageInfo.sampler = diffuseTexture->getSampler();
        }
        return imageInfo;
    }
    
    // Get texture binding info for a specific texture type
    VkDescriptorImageInfo getTextureImageInfo(TextureType type) const {
        VkDescriptorImageInfo imageInfo{};
        auto texture = getTexture(type);
        if (texture) {
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture->getImageView();
            imageInfo.sampler = texture->getSampler();
        }
        return imageInfo;
    }

    std::shared_ptr<Texture> createCombinedMetallicRoughnessTexture(VkDevice device, VkPhysicalDevice physicalDevice,
                                                                    VkCommandPool commandPool, VkQueue graphicsQueue,
                                                                    std::shared_ptr<Texture> metallicTex,
                                                                    std::shared_ptr<Texture> roughnessTex);


    // PBR Utility setters
    void setPBRProperties(float metallic, float roughness) {
        this->metallic = metallic;
        this->roughness = roughness;
    }

    void setPBRTextures(std::shared_ptr<Texture> albedo, 
                        std::shared_ptr<Texture> normal = nullptr,
                        std::shared_ptr<Texture> metallic = nullptr,
                        std::shared_ptr<Texture> roughness = nullptr,
                        std::shared_ptr<Texture> ao = nullptr,
                        std::shared_ptr<Texture> emissive = nullptr) {
    
        if (albedo) {
            setTexture(TextureType::Diffuse, albedo);
        }
        if (normal) {
            setTexture(TextureType::Normal, normal);
        }
        if (metallic) {
            setTexture(TextureType::Metallic, metallic);
        }
        if (roughness) {
            setTexture(TextureType::Roughness, roughness);
        }
        if (ao) {
            setTexture(TextureType::AmbientOcclusion, ao);
        }
        if (emissive) {
            setTexture(TextureType::Emissive, emissive);
        }
    }

    void setEmissive(const glm::vec3& color, float strength = 1.0f) {
        emissiveColor = color;
        emissiveStrength = strength;
    }

};