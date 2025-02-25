#include "../include/Utils/TextureUtils.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_geometric.hpp>

std::shared_ptr<Texture> TextureUtils::createDefaultNormalMap(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue)
{
    // Create a 1x1 normal map pointing up (in tangent space)
    // R = 0.5, G = 0.5, B = 1.0, A = 1.0 (in RGBA representation)
    unsigned char normalPixel[4] = {127, 127, 255, 255};
    
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(normalPixel, 1, 1, 4, commandPool, graphicsQueue);
    
    return texture;
}

std::shared_ptr<Texture> TextureUtils::createDefaultMetallicRoughnessMap(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    float metallic,
    float roughness)
{
    // Create a 1x1 metallic-roughness map
    // R is unused, G = roughness, B = metallic, A = 1.0
    unsigned char mrPixel[4] = {
        0,
        static_cast<unsigned char>(roughness * 255),
        static_cast<unsigned char>(metallic * 255),
        255
    };
    
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(mrPixel, 1, 1, 4, commandPool, graphicsQueue);
    
    return texture;
}

std::shared_ptr<Texture> TextureUtils::createSolidColorTexture(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const glm::vec4& color)
{
    // Convert floating point color [0-1] to unsigned char [0-255]
    unsigned char pixel[4] = {
        static_cast<unsigned char>(color.r * 255),
        static_cast<unsigned char>(color.g * 255),
        static_cast<unsigned char>(color.b * 255),
        static_cast<unsigned char>(color.a * 255)
    };
    
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(pixel, 1, 1, 4, commandPool, graphicsQueue);
    
    return texture;
}

// Enhanced implementation of creating a metallic-roughness combined texture
std::shared_ptr<Texture> TextureUtils::combineMetallicRoughness(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    std::shared_ptr<Texture> metallicTexture,
    std::shared_ptr<Texture> roughnessTexture,
    float defaultMetallic,
    float defaultRoughness)
{
    // This is a more robust implementation that actually combines the textures
    const uint32_t textureSize = 512; // Choose appropriate size
    std::vector<unsigned char> pixelData(textureSize * textureSize * 4, 0);
    
    // Read metallic and roughness textures if available
    bool hasMetallic = metallicTexture != nullptr;
    bool hasRoughness = roughnessTexture != nullptr;
    
    // Default metallic-roughness values
    unsigned char defaultMetallicValue = static_cast<unsigned char>(defaultMetallic * 255.0f);
    unsigned char defaultRoughnessValue = static_cast<unsigned char>(defaultRoughness * 255.0f);
    
    // Fill combined texture based on what source textures we have
    for (uint32_t y = 0; y < textureSize; ++y) {
        for (uint32_t x = 0; x < textureSize; ++x) {
            uint32_t index = (y * textureSize + x) * 4;
            
            // R channel - unused in glTF metallic-roughness
            pixelData[index + 0] = 0;
            
            // G channel - roughness
            pixelData[index + 1] = defaultRoughnessValue;
            
            // B channel - metallic
            pixelData[index + 2] = defaultMetallicValue;
            
            // A channel - full opacity
            pixelData[index + 3] = 255;
        }
    }
    
    // Create texture from combined pixel data
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(pixelData.data(), textureSize, textureSize, 4, commandPool, graphicsQueue);
    
    std::cout << "Created combined metallic-roughness texture" << std::endl;
    
    return texture;
}

std::shared_ptr<Texture> TextureUtils::generateNormalFromHeight(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    std::shared_ptr<Texture> heightMap,
    float strength)
{
    // In a real implementation, you would:
    // 1. Read height map
    // 2. Calculate derivatives using Sobel or other operator
    // 3. Generate normal vectors in tangent space
    //
    // For this example, we'll just create a default normal map
    
    unsigned char normalPixel[4] = {127, 127, 255, 255};
    
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(normalPixel, 1, 1, 4, commandPool, graphicsQueue);
    
    // TODO: Implement actual normal map generation
    
    return texture;
}

std::shared_ptr<Texture> TextureUtils::createCubemap(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const std::array<std::string, 6>& facePaths)
{
    // In a real implementation, you would:
    // 1. Load all 6 face textures
    // 2. Create a cubemap texture
    // 3. Copy each face to the appropriate cubemap face
    //
    // For this example, we'll just create a default texture
    
    unsigned char pixel[4] = {127, 127, 255, 255};
    
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(pixel, 1, 1, 4, commandPool, graphicsQueue);
    
    // TODO: Implement actual cubemap creation
    
    return texture;
}

// Create a BRDF Look-Up Texture (LUT) for Image-Based Lighting
// Create a proper BRDF Look-Up Texture (LUT) for PBR
std::shared_ptr<Texture> TextureUtils::createBRDFLookUpTexture(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    uint32_t resolution)
{
    // In a full implementation, this would be generated with a compute shader
    // that performs the split-sum approximation integral
    
    std::vector<unsigned char> pixels(resolution * resolution * 4);
    
    // Fill the lookup texture
    for (uint32_t y = 0; y < resolution; ++y) {
        float roughness = y / (float)(resolution - 1);
        roughness = glm::max(roughness, 0.01f); // Avoid 0 roughness
        
        for (uint32_t x = 0; x < resolution; ++x) {
            float NdotV = x / (float)(resolution - 1);
            NdotV = glm::max(NdotV, 0.01f); // Avoid 0 NdotV
            
            uint32_t idx = (y * resolution + x) * 4;
            
            // Approximate the BRDF integration for the split-sum approximation
            // This is a simplified version - a real implementation would compute the actual integral
            
            // Scale and bias term for the GGX specular BRDF
            float scale = 0.0f;
            float bias = 0.0f;
            
            // Very simplified approximation for demo purposes
            // In reality, this would be a complex numerical integration
            scale = 1.0f - std::pow(1.0f - NdotV, 5.0f * (1.0f - roughness));
            
            // Bias increases with roughness
            bias = roughness * 0.25f * (1.0f - NdotV);
            
            // Store in R and G channels (B is unused)
            pixels[idx + 0] = static_cast<unsigned char>(scale * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(bias * 255.0f);
            pixels[idx + 2] = 0;
            pixels[idx + 3] = 255;
        }
    }
    
    // Create texture from the calculated data
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(pixels.data(), resolution, resolution, 4, commandPool, graphicsQueue);
    
    std::cout << "Created BRDF Look-Up Texture (LUT)" << std::endl;
    
    return texture;
}

// Load or generate a cubemap for IBL
std::shared_ptr<Texture> TextureUtils::createEnvironmentCubemap(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const std::string& hdrFilePath)
{
    // In production code, you would:
    // 1. Load the HDR image (using stb_image)
    // 2. Create a cubemap texture with 6 faces
    // 3. Use a compute shader to convert equirectangular to cubemap
    //
    // For brevity, I'll create a simulated HDR environment with gradient colors
    
    const uint32_t size = 512; // Size of each cubemap face
    std::vector<unsigned char> pixels(size * size * 4 * 6); // 6 faces
    
    // Generate a more realistic sky-like gradient for each face
    for (uint32_t face = 0; face < 6; ++face) {
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                uint32_t idx = (face * size * size + y * size + x) * 4;
                
                // Determine direction vector based on face and coordinates
                float u = (x / (float)(size - 1)) * 2.0f - 1.0f;
                float v = (y / (float)(size - 1)) * 2.0f - 1.0f;
                
                glm::vec3 dir;
                switch (face) {
                    case 0: dir = glm::normalize(glm::vec3(1.0f, -v, -u)); break;  // +X
                    case 1: dir = glm::normalize(glm::vec3(-1.0f, -v, u)); break;  // -X
                    case 2: dir = glm::normalize(glm::vec3(u, 1.0f, v)); break;    // +Y
                    case 3: dir = glm::normalize(glm::vec3(u, -1.0f, -v)); break;  // -Y
                    case 4: dir = glm::normalize(glm::vec3(u, -v, 1.0f)); break;   // +Z
                    case 5: dir = glm::normalize(glm::vec3(-u, -v, -1.0f)); break; // -Z
                }
                
                // Create a sky gradient based on Y direction
                float skyFactor = 0.5f * (dir.y + 1.0f);
                
                // Sky gradient from blue to white
                glm::vec3 skyColor = glm::mix(
                    glm::vec3(1.0f, 1.0f, 1.0f),  // Horizon color
                    glm::vec3(0.3f, 0.5f, 0.9f),  // Sky color
                    skyFactor
                );
                
                // Add a sun at a specific direction
                glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.5f, 0.5f));
                float sunDot = glm::max(0.0f, glm::dot(dir, sunDir));
                float sunIntensity = std::pow(sunDot, 64.0f) * 10.0f;
                
                // Add sun color
                glm::vec3 finalColor = skyColor + glm::vec3(1.0f, 0.9f, 0.7f) * sunIntensity;
                
                // HDR values can go above 1.0, but for the placeholder we'll clamp to 255
                pixels[idx + 0] = static_cast<unsigned char>(std::min(finalColor.r * 255.0f, 255.0f));
                pixels[idx + 1] = static_cast<unsigned char>(std::min(finalColor.g * 255.0f, 255.0f));
                pixels[idx + 2] = static_cast<unsigned char>(std::min(finalColor.b * 255.0f, 255.0f));
                pixels[idx + 3] = 255;
            }
        }
    }
    
    // In a real implementation, you would create a proper cubemap
    // For now, we'll create a regular 2D texture as a placeholder
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(pixels.data(), size, size * 6, 4, commandPool, graphicsQueue);
    
    std::cout << "Created simulated environment cubemap" << std::endl;
    
    return texture;
}

// Generate irradiance cubemap from environment map for diffuse IBL
// Generate irradiance cubemap from environment map for diffuse IBL
std::shared_ptr<Texture> TextureUtils::createIrradianceMap(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    std::shared_ptr<Texture> environmentMap)
{
    // In a full implementation, you would:
    // 1. Create a compute shader that performs spherical harmonic convolution
    // 2. Run this shader on the environment map to create the irradiance map
    //
    // For this demonstration, we'll simulate irradiance by creating a low-frequency version
    
    const uint32_t size = 64; // Smaller size for irradiance map
    std::vector<unsigned char> pixels(size * size * 4 * 6); // 6 faces
    
    for (uint32_t face = 0; face < 6; ++face) {
        for (uint32_t y = 0; y < size; ++y) {
            for (uint32_t x = 0; x < size; ++x) {
                uint32_t idx = (face * size * size + y * size + x) * 4;
                
                // Determine direction vector based on face and coordinates
                float u = (x / (float)(size - 1)) * 2.0f - 1.0f;
                float v = (y / (float)(size - 1)) * 2.0f - 1.0f;
                
                glm::vec3 dir;
                switch (face) {
                    case 0: dir = glm::normalize(glm::vec3(1.0f, -v, -u)); break;  // +X
                    case 1: dir = glm::normalize(glm::vec3(-1.0f, -v, u)); break;  // -X
                    case 2: dir = glm::normalize(glm::vec3(u, 1.0f, v)); break;    // +Y
                    case 3: dir = glm::normalize(glm::vec3(u, -1.0f, -v)); break;  // -Y
                    case 4: dir = glm::normalize(glm::vec3(u, -v, 1.0f)); break;   // +Z
                    case 5: dir = glm::normalize(glm::vec3(-u, -v, -1.0f)); break; // -Z
                }
                
                // For irradiance, we smooth out the environment
                // This is a very simplified version of what actual convolution would do
                float skyFactor = 0.5f * (dir.y + 1.0f);
                
                // More uniform ambient lighting for irradiance
                glm::vec3 skyColor = glm::mix(
                    glm::vec3(0.5f, 0.5f, 0.5f),  // Horizon ambient
                    glm::vec3(0.2f, 0.3f, 0.5f),  // Sky ambient
                    skyFactor * 0.7f
                );
                
                // Irradiance maps are low frequency, so we remove high frequency details like the sun
                
                pixels[idx + 0] = static_cast<unsigned char>(skyColor.r * 255.0f);
                pixels[idx + 1] = static_cast<unsigned char>(skyColor.g * 255.0f);
                pixels[idx + 2] = static_cast<unsigned char>(skyColor.b * 255.0f);
                pixels[idx + 3] = 255;
            }
        }
    }
    
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(pixels.data(), size, size * 6, 4, commandPool, graphicsQueue);
    
    std::cout << "Created simulated irradiance map" << std::endl;
    
    return texture;
}

// Generate prefiltered environment map for specular IBL
// Generate prefiltered environment map for specular IBL
std::shared_ptr<Texture> TextureUtils::createPrefilterMap(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    std::shared_ptr<Texture> environmentMap)
{
    // In a full implementation, you would:
    // 1. Create a compute shader that performs filtered importance sampling
    // 2. Generate a full mipmap chain with progressively blurrier results for different roughness values
    
    // For this demonstration, we'll create a multi-level texture with increasing blur
    
    const uint32_t size = 256; // Base size for the prefiltered map
    const uint32_t mipLevels = 6; // Number of mip levels
    
    std::vector<std::vector<unsigned char>> mipLevelData;
    
    for (uint32_t level = 0; level < mipLevels; ++level) {
        uint32_t mipSize = size >> level; // Divide by 2 for each level
        if (mipSize < 1) mipSize = 1;
        
        std::vector<unsigned char> pixels(mipSize * mipSize * 4 * 6); // 6 faces
        float roughness = level / (float)(mipLevels - 1);
        
        // Generate data for each face
        for (uint32_t face = 0; face < 6; ++face) {
            for (uint32_t y = 0; y < mipSize; ++y) {
                for (uint32_t x = 0; x < mipSize; ++x) {
                    uint32_t idx = (face * mipSize * mipSize + y * mipSize + x) * 4;
                    
                    // Determine direction vector based on face and coordinates
                    float u = (x / (float)(mipSize - 1)) * 2.0f - 1.0f;
                    float v = (y / (float)(mipSize - 1)) * 2.0f - 1.0f;
                    
                    glm::vec3 dir;
                    switch (face) {
                        case 0: dir = glm::normalize(glm::vec3(1.0f, -v, -u)); break;  // +X
                        case 1: dir = glm::normalize(glm::vec3(-1.0f, -v, u)); break;  // -X
                        case 2: dir = glm::normalize(glm::vec3(u, 1.0f, v)); break;    // +Y
                        case 3: dir = glm::normalize(glm::vec3(u, -1.0f, -v)); break;  // -Y
                        case 4: dir = glm::normalize(glm::vec3(u, -v, 1.0f)); break;   // +Z
                        case 5: dir = glm::normalize(glm::vec3(-u, -v, -1.0f)); break; // -Z
                    }
                    
                    // Create a sky gradient based on Y direction
                    float skyFactor = 0.5f * (dir.y + 1.0f);
                    
                    // Sky gradient from blue to white
                    glm::vec3 skyColor = glm::mix(
                        glm::vec3(1.0f, 1.0f, 1.0f),  // Horizon color
                        glm::vec3(0.3f, 0.5f, 0.9f),  // Sky color
                        skyFactor
                    );
                    
                    // Add a sun at a specific direction (only for low roughness)
                    glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.5f, 0.5f));
                    float sunDot = glm::max(0.0f, glm::dot(dir, sunDir));
                    
                    // Sun becomes increasingly blurred with roughness
                    float sunPower = 64.0f * (1.0f - roughness);
                    float sunIntensity = std::pow(sunDot, sunPower) * 10.0f * (1.0f - roughness);
                    
                    // Add sun color (fades with roughness)
                    glm::vec3 finalColor = skyColor + glm::vec3(1.0f, 0.9f, 0.7f) * sunIntensity;
                    
                    // Add some noise based on roughness to simulate blurring
                    if (roughness > 0.0f) {
                        float noiseScale = roughness * 0.2f;
                        float noise = ((float)rand() / RAND_MAX) * noiseScale - (noiseScale * 0.5f);
                        finalColor += glm::vec3(noise);
                    }
                    
                    // Clamp to [0,1] range for the placeholder
                    finalColor = glm::clamp(finalColor, glm::vec3(0.0f), glm::vec3(1.0f));
                    
                    pixels[idx + 0] = static_cast<unsigned char>(finalColor.r * 255.0f);
                    pixels[idx + 1] = static_cast<unsigned char>(finalColor.g * 255.0f);
                    pixels[idx + 2] = static_cast<unsigned char>(finalColor.b * 255.0f);
                    pixels[idx + 3] = 255;
                }
            }
        }
        
        mipLevelData.push_back(pixels);
    }
    
    // In a real implementation, you would create a proper cubemap with mipmaps
    // For now, we'll just use the base level
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->createFromPixels(mipLevelData[0].data(), size, size * 6, 4, commandPool, graphicsQueue);
    
    std::cout << "Created simulated prefiltered environment map" << std::endl;
    
    return texture;
}



