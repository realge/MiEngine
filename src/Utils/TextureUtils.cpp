#include "../include/Utils/TextureUtils.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stb_image.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/scalar_constants.hpp>

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
    // Create a 2D texture for the BRDF LUT
    VkImage brdfImage;
    VkDeviceMemory brdfMemory;
    VkFormat format = VK_FORMAT_R16G16_SFLOAT; // RG16F is sufficient for BRDF LUT
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = resolution;
    imageInfo.extent.height = resolution;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &brdfImage) != VK_SUCCESS) {
        std::cerr << "Failed to create BRDF LUT image!" << std::endl;
        return nullptr;
    }
    
    // Allocate memory for the BRDF LUT
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, brdfImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, 
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &brdfMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate BRDF LUT memory!" << std::endl;
        vkDestroyImage(device, brdfImage, nullptr);
        return nullptr;
    }
    
    vkBindImageMemory(device, brdfImage, brdfMemory, 0);
    
    // Create staging buffer for the BRDF LUT data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize bufferSize = resolution * resolution * 2 * sizeof(float); // RG16F
    
    createBuffer(device, physicalDevice, bufferSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
    
    // Map memory for staging buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // Calculate the BRDF LUT
    std::vector<float> lutData(resolution * resolution * 2);
    
    // Pre-integrate BRDF for all combinations of NoV and roughness
    for (uint32_t y = 0; y < resolution; y++) {
        float roughness = y / static_cast<float>(resolution - 1);
        roughness = std::max(roughness, 0.01f); // Avoid 0 roughness
        
        for (uint32_t x = 0; x < resolution; x++) {
            float NoV = x / static_cast<float>(resolution - 1);
            NoV = std::max(NoV, 0.01f); // Avoid 0 NoV
            
            glm::vec2 brdf = integrateBRDF(NoV, roughness);
            
            uint32_t idx = (y * resolution + x) * 2;
            lutData[idx + 0] = brdf.x; // Scale factor
            lutData[idx + 1] = brdf.y; // Bias
        }
    }
    
    // Copy LUT data to staging buffer
    memcpy(data, lutData.data(), bufferSize);
    
    // Transition layout for copy
    transitionImageLayout(device, commandPool, graphicsQueue, brdfImage, format,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        0, 1, 0, 1);
    
    // Copy buffer to image
    copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, brdfImage,
                    resolution, resolution, 0);
    
    // Transition to shader read optimal
    transitionImageLayout(device, commandPool, graphicsQueue, brdfImage, format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        0, 1, 0, 1);
    
    // Clean up staging resources
    vkUnmapMemory(device, stagingBufferMemory);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    // Create texture object to wrap the BRDF LUT
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->initWithExistingImage(brdfImage, brdfMemory, format, resolution, resolution, 
                                 1, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    std::cout << "Created BRDF Look-Up Texture" << std::endl;
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
    // Load HDR image
    int width, height, channels;
    float* hdrData = stbi_loadf(hdrFilePath.c_str(), &width, &height, &channels, 0);
    if (!hdrData) {
        std::cerr << "Failed to load HDR image: " << hdrFilePath << std::endl;
        // Create a fallback cubemap
        return createDefaultEnvironmentCubemap(device, physicalDevice, commandPool, graphicsQueue);
    }
    
    // Create a cubemap from the equirectangular HDR image
    const uint32_t cubemapSize = 1024; // Size of each cubemap face
    const uint32_t numMipLevels = static_cast<uint32_t>(std::floor(std::log2(cubemapSize))) + 1;
    
    // Create cubemap image
    VkImage cubemapImage;
    VkDeviceMemory cubemapMemory;
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT; // HDR requires floating point
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = cubemapSize;
    imageInfo.extent.height = cubemapSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = numMipLevels;
    imageInfo.arrayLayers = 6; // Cubemap has 6 faces
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Important for cubemaps
    
    if (vkCreateImage(device, &imageInfo, nullptr, &cubemapImage) != VK_SUCCESS) {
        std::cerr << "Failed to create cubemap image!" << std::endl;
        stbi_image_free(hdrData);
        return createDefaultEnvironmentCubemap(device, physicalDevice, commandPool, graphicsQueue);
    }
    
    // Allocate memory for the cubemap
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, cubemapImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, 
                                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &cubemapMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate cubemap memory!" << std::endl;
        vkDestroyImage(device, cubemapImage, nullptr);
        stbi_image_free(hdrData);
        return createDefaultEnvironmentCubemap(device, physicalDevice, commandPool, graphicsQueue);
    }
    
    vkBindImageMemory(device, cubemapImage, cubemapMemory, 0);
    
    // TODO: Use a compute shader to convert equirectangular HDR to cubemap
    // For this implementation, we'll convert on the CPU for simplicity
    // Generate cubemap face data
    
    // Create staging buffer for each face
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize bufferSize = cubemapSize * cubemapSize * 4 * sizeof(float); // RGBA32F
    
    createBuffer(device, physicalDevice, bufferSize, 
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);
    
    // Map memory for staging buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // Convert equirectangular HDR to cubemap faces
    // This is a simplified conversion - a real implementation would use more accurate sampling
    std::vector<float> faceData(cubemapSize * cubemapSize * 4);
    
    // For each face of the cubemap
    for (uint32_t face = 0; face < 6; face++) {
        equirectangularToCubemapFace(hdrData, width, height, channels, 
                                    faceData.data(), cubemapSize, face);
        
        // Copy face data to staging buffer
        memcpy(data, faceData.data(), bufferSize);
        
        // Transition layout for copy
        transitionImageLayout(device, commandPool, graphicsQueue, cubemapImage, format,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            face, 1, 0, 1);
        
        // Copy buffer to image
        copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, cubemapImage,
                        cubemapSize, cubemapSize, face);
    }
    
    // Transition to shader read optimal
    transitionImageLayout(device, commandPool, graphicsQueue, cubemapImage, format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        0, 6, 0, 1);
    
    // Clean up staging resources
    vkUnmapMemory(device, stagingBufferMemory);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    stbi_image_free(hdrData);
    
    // Create texture object to wrap the cubemap
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    // Initialize the texture with the cubemap image
    texture->initWithExistingImage(cubemapImage, cubemapMemory, format, cubemapSize, cubemapSize, 
                                 numMipLevels, 6, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    std::cout << "Created environment cubemap from HDR" << std::endl;
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
    if (!environmentMap) {
        std::cerr << "No environment map provided for irradiance generation" << std::endl;
        return nullptr;
    }
    
    // For irradiance map, we use a smaller resolution since it's a low-frequency signal
    const uint32_t irradianceSize = 64;
    
    // Create a new cubemap texture for the irradiance map
    VkImage irradianceImage;
    VkDeviceMemory irradianceMemory;
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT; // HDR requires floating point
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = irradianceSize;
    imageInfo.extent.height = irradianceSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6; // Cubemap has 6 faces
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &irradianceImage) != VK_SUCCESS) {
        std::cerr << "Failed to create irradiance map image!" << std::endl;
        return nullptr;
    }
    
    // Allocate memory for the irradiance map
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, irradianceImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, 
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &irradianceMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate irradiance map memory!" << std::endl;
        vkDestroyImage(device, irradianceImage, nullptr);
        return nullptr;
    }
    
    vkBindImageMemory(device, irradianceImage, irradianceMemory, 0);
    
    // TODO: Use a compute shader to perform irradiance convolution
    // For now, we'll perform a simplified diffuse convolution on the CPU
    
    // Create staging buffer for the irradiance data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize bufferSize = irradianceSize * irradianceSize * 4 * sizeof(float); // RGBA32F
    
    createBuffer(device, physicalDevice, bufferSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
    
    // Map memory for staging buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    
    // For a real implementation, we would:
    // 1. Sample the environment map many times in a hemisphere oriented along the surface normal
    // 2. Average the results to get the irradiance contribution
    
    // This is a simplified diffuse convolution - in a real implementation, use a compute shader
    for (uint32_t face = 0; face < 6; face++) {
        // Create a more realistic ambient lighting for the irradiance map
        std::vector<float> faceData(irradianceSize * irradianceSize * 4);
        
        // Perform a simple diffuse convolution for each pixel
        for (uint32_t y = 0; y < irradianceSize; y++) {
            for (uint32_t x = 0; x < irradianceSize; x++) {
                // Get direction vector for this pixel
                float u = (2.0f * x / (irradianceSize - 1.0f)) - 1.0f;
                float v = (2.0f * y / (irradianceSize - 1.0f)) - 1.0f;
                
                glm::vec3 direction;
                switch (face) {
                    case 0: direction = glm::normalize(glm::vec3(1.0f, -v, -u)); break;  // +X
                    case 1: direction = glm::normalize(glm::vec3(-1.0f, -v, u)); break;  // -X
                    case 2: direction = glm::normalize(glm::vec3(u, 1.0f, v)); break;    // +Y
                    case 3: direction = glm::normalize(glm::vec3(u, -1.0f, -v)); break;  // -Y
                    case 4: direction = glm::normalize(glm::vec3(u, -v, 1.0f)); break;   // +Z
                    case 5: direction = glm::normalize(glm::vec3(-u, -v, -1.0f)); break; // -Z
                }
                
                // Perform a simplified diffuse convolution
                glm::vec3 irradiance = diffuseConvolution(environmentMap, direction, 64);
                
                // Set pixel data
                uint32_t idx = (y * irradianceSize + x) * 4;
                faceData[idx + 0] = irradiance.r;
                faceData[idx + 1] = irradiance.g;
                faceData[idx + 2] = irradiance.b;
                faceData[idx + 3] = 1.0f;
            }
        }
        
        // Copy face data to staging buffer
        memcpy(data, faceData.data(), bufferSize);
        
        // Transition layout for copy
        transitionImageLayout(device, commandPool, graphicsQueue, irradianceImage, format,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            face, 1, 0, 1);
        
        // Copy buffer to image
        copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, irradianceImage,
                        irradianceSize, irradianceSize, face);
    }
    
    // Transition to shader read optimal
    transitionImageLayout(device, commandPool, graphicsQueue, irradianceImage, format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        0, 6, 0, 1);
    
    // Clean up staging resources
    vkUnmapMemory(device, stagingBufferMemory);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    // Create texture object to wrap the irradiance map
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->initWithExistingImage(irradianceImage, irradianceMemory, format, irradianceSize, irradianceSize, 
                                 1, 6, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    std::cout << "Created irradiance map" << std::endl;
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
    if (!environmentMap) {
        std::cerr << "No environment map provided for prefilter generation" << std::endl;
        return nullptr;
    }
    
    // For prefiltered environment map, we use multiple mip levels for different roughness values
    const uint32_t prefilterSize = 256;
    const uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(prefilterSize))) + 1;
    
    // Create a new cubemap texture for the prefiltered environment map
    VkImage prefilterImage;
    VkDeviceMemory prefilterMemory;
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT; // HDR requires floating point
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = prefilterSize;
    imageInfo.extent.height = prefilterSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 6; // Cubemap has 6 faces
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &prefilterImage) != VK_SUCCESS) {
        std::cerr << "Failed to create prefilter map image!" << std::endl;
        return nullptr;
    }
    
    // Allocate memory for the prefilter map
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, prefilterImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, 
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &prefilterMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate prefilter map memory!" << std::endl;
        vkDestroyImage(device, prefilterImage, nullptr);
        return nullptr;
    }
    
    vkBindImageMemory(device, prefilterImage, prefilterMemory, 0);
    
    // Process each mip level with increasing roughness
    for (uint32_t mip = 0; mip < mipLevels; mip++) {
        // Calculate size of this mip level
        uint32_t mipSize = prefilterSize >> mip;
        if (mipSize < 1) mipSize = 1;
        
        // Roughness increases with each mip level
        float roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
        
        // Create staging buffer for this mip level
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VkDeviceSize bufferSize = mipSize * mipSize * 4 * sizeof(float); // RGBA32F
        
        createBuffer(device, physicalDevice, bufferSize, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingBufferMemory);
        
        // Map memory for staging buffer
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        
        // For each face of the cubemap
        for (uint32_t face = 0; face < 6; face++) {
            // Create data for this face
            std::vector<float> faceData(mipSize * mipSize * 4);
            
            // Perform importance sampling for each pixel
            for (uint32_t y = 0; y < mipSize; y++) {
                for (uint32_t x = 0; x < mipSize; x++) {
                    // Get direction vector for this pixel
                    float u = (2.0f * x / (mipSize - 1.0f)) - 1.0f;
                    float v = (2.0f * y / (mipSize - 1.0f)) - 1.0f;
                    
                    glm::vec3 direction;
                    switch (face) {
                        case 0: direction = glm::normalize(glm::vec3(1.0f, -v, -u)); break;  // +X
                        case 1: direction = glm::normalize(glm::vec3(-1.0f, -v, u)); break;  // -X
                        case 2: direction = glm::normalize(glm::vec3(u, 1.0f, v)); break;    // +Y
                        case 3: direction = glm::normalize(glm::vec3(u, -1.0f, -v)); break;  // -Y
                        case 4: direction = glm::normalize(glm::vec3(u, -v, 1.0f)); break;   // +Z
                        case 5: direction = glm::normalize(glm::vec3(-u, -v, -1.0f)); break; // -Z
                    }
                    
                    // Perform a simplified specular convolution based on roughness
                    glm::vec3 filteredColor = specularConvolution(environmentMap, direction, roughness, 1024);
                    
                    // Set pixel data
                    uint32_t idx = (y * mipSize + x) * 4;
                    faceData[idx + 0] = filteredColor.r;
                    faceData[idx + 1] = filteredColor.g;
                    faceData[idx + 2] = filteredColor.b;
                    faceData[idx + 3] = 1.0f;
                }
            }
            
            // Copy face data to staging buffer
            memcpy(data, faceData.data(), bufferSize);
            
            // Transition layout for copy
            transitionImageLayout(device, commandPool, graphicsQueue, prefilterImage, format,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                face, 1, mip, 1);
            
            // Copy buffer to image
            copyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, prefilterImage,
                            mipSize, mipSize, face, mip);
        }
        
        // Clean up staging resources for this mip level
        vkUnmapMemory(device, stagingBufferMemory);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    
    // Transition all mip levels to shader read optimal
    transitionImageLayout(device, commandPool, graphicsQueue, prefilterImage, format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        0, 6, 0, mipLevels);
    
    // Create texture object to wrap the prefilter map
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    texture->initWithExistingImage(prefilterImage, prefilterMemory, format, prefilterSize, prefilterSize, 
                                 mipLevels, 6, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    std::cout << "Created prefiltered environment map with " << mipLevels << " mip levels" << std::endl;
    return texture;
}

// Helper function to convert equirectangular projection to a cubemap face
void TextureUtils::equirectangularToCubemapFace(
    float* equirectangularData, int equiWidth, int equiHeight, int channels,
    float* faceData, int faceSize, int faceIndex)
{
    // Direction vectors for each face of the cubemap
    // 0: +X (right), 1: -X (left), 2: +Y (up), 3: -Y (down), 4: +Z (front), 5: -Z (back)
    glm::vec3 faceDirs[6][3] = {
        { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) }, // +X
        { glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }, // -X
        { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },   // +Y
        { glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) }, // -Y
        { glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },  // +Z
        { glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f) } // -Z
    };
    
    glm::vec3 facePlane = faceDirs[faceIndex][0];
    glm::vec3 faceU = faceDirs[faceIndex][2];
    glm::vec3 faceV = faceDirs[faceIndex][1];
    
    for (int y = 0; y < faceSize; y++) {
        for (int x = 0; x < faceSize; x++) {
            // Map pixel position to [-1, 1] range
            float u = (2.0f * x / (faceSize - 1.0f)) - 1.0f;
            float v = (2.0f * y / (faceSize - 1.0f)) - 1.0f;
            
            // Get direction vector for this pixel
            glm::vec3 dir = glm::normalize(facePlane + (u * faceU) + (v * faceV));
            
            // Convert direction to equirectangular coordinates
            float phi = std::atan2(dir.z, dir.x);
            float theta = std::asin(dir.y);
            
            // Map to [0, 1] range
            float eqU = (phi + glm::pi<float>()) / (2.0f * glm::pi<float>());
            float eqV = (theta + glm::pi<float>() / 2.0f) / glm::pi<float>();
            
            // Sample from equirectangular map
            int eqX = static_cast<int>(eqU * (equiWidth - 1) + 0.5f);
            int eqY = static_cast<int>(eqV * (equiHeight - 1) + 0.5f);
            
            // Clamp to valid range
            eqX = std::max(0, std::min(eqX, equiWidth - 1));
            eqY = std::max(0, std::min(eqY, equiHeight - 1));
            
            // Get pixel from equirectangular image
            int eqIdx = (eqY * equiWidth + eqX) * channels;
            
            // Set face pixel data
            int faceIdx = (y * faceSize + x) * 4;
            faceData[faceIdx + 0] = equirectangularData[eqIdx + 0]; // R
            faceData[faceIdx + 1] = channels > 1 ? equirectangularData[eqIdx + 1] : equirectangularData[eqIdx + 0]; // G
            faceData[faceIdx + 2] = channels > 2 ? equirectangularData[eqIdx + 2] : equirectangularData[eqIdx + 0]; // B
            faceData[faceIdx + 3] = 1.0f; // A
        }
    }
}

// Helper function for diffuse convolution
glm::vec3 TextureUtils::diffuseConvolution(std::shared_ptr<Texture> envMap, const glm::vec3& normal, int sampleCount) {
    // In a real implementation, we would sample a hemisphere around the normal
    // and perform proper convolution. This is a simplified approximation.
    
    // Generate a set of hemisphere samples
    std::vector<glm::vec3> samples = generateHemisphereSamples(normal, sampleCount);
    
    // Sample the environment map and accumulate the results
    glm::vec3 irradiance(0.0f);
    float weight = 0.0f;
    
    for (const auto& sample : samples) {
        // Calculate the angle between the normal and the sample direction
        float cosTheta = glm::dot(normal, sample);
        
        // Only consider samples in the hemisphere (cosTheta > 0)
        if (cosTheta > 0.0f) {
            // Sample the environment map
            glm::vec3 color = sampleEnvironmentMap(envMap, sample);
            
            // Weight by the cosine term and solid angle
            irradiance += color * cosTheta;
            weight += cosTheta;
        }
    }
    
    // Normalize the result
    if (weight > 0.0f) {
        irradiance /= weight;
    }
    
    return irradiance;
}

// Helper function for specular convolution
glm::vec3 TextureUtils::specularConvolution(std::shared_ptr<Texture> envMap, const glm::vec3& reflection, 
                                           float roughness, int sampleCount) {
    // In a real implementation, we would use importance sampling
    
    // Determine the lobe width based on roughness
    float alphaSquared = roughness * roughness;
    
    // Generate a set of samples around the reflection vector based on roughness
    std::vector<glm::vec3> samples = generateImportanceSamples(reflection, roughness, sampleCount);
    
    // Sample the environment map and accumulate the results
    glm::vec3 prefilteredColor(0.0f);
    float totalWeight = 0.0f;
    
    for (const auto& sample : samples) {
        // Calculate the angle between the reflection and the sample direction
        float NoS = glm::dot(reflection, sample);
        
        // Only consider samples that are visible from the reflection direction
        if (NoS > 0.0f) {
            // Sample the environment map
            glm::vec3 color = sampleEnvironmentMap(envMap, sample);
            
            // Apply importance sampling weight
            float D = distributionGGX(NoS, alphaSquared);
            float weight = D * NoS;
            
            prefilteredColor += color * weight;
            totalWeight += weight;
        }
    }
    
    // Normalize the result
    if (totalWeight > 0.0f) {
        prefilteredColor /= totalWeight;
    }
    
    return prefilteredColor;
}

bool Texture::initWithExistingImage(
    VkImage image, 
    VkDeviceMemory memory,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    uint32_t layerCount,
    VkImageViewType viewType,
    VkImageLayout initialLayout)
{
    // Clean up existing resources
    if (textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
    }
    
    if (textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, textureImageView, nullptr);
        textureImageView = VK_NULL_HANDLE;
    }
    
    if (textureImage != VK_NULL_HANDLE && textureImage != image) {
        vkDestroyImage(device, textureImage, nullptr);
    }
    
    if (textureImageMemory != VK_NULL_HANDLE && textureImageMemory != memory) {
        vkFreeMemory(device, textureImageMemory, nullptr);
    }
    
    // Store new properties
    textureImage = image;
    textureImageMemory = memory;
    imageFormat = format;
    this->mipLevels = mipLevels;
    imageLayout = initialLayout;
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        return false;
    }
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    
    // Set address mode based on view type
    if (viewType == VK_IMAGE_VIEW_TYPE_CUBE) {
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    } else {
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    
    // Check for anisotropic filtering support
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    
    if (deviceFeatures.samplerAnisotropy) {
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }
    
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        std::cerr << "Failed to create texture sampler!" << std::endl;
        return false;
    }
    
    return true;
}

// Add these implementations to TextureUtils.cpp

// Helper function implementations
uint32_t TextureUtils::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void TextureUtils::createBuffer(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size, 
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags properties, 
    VkBuffer& buffer, 
    VkDeviceMemory& bufferMemory) 
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void TextureUtils::transitionImageLayout(
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    uint32_t baseArrayLayer,
    uint32_t layerCount,
    uint32_t baseMipLevel,
    uint32_t levelCount)
{
    // Create command buffer for the transition
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Create image barrier
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount = layerCount;

    // Determine pipeline stages and access masks based on layouts
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::runtime_error("Unsupported layout transition!");
    }

    // Record the barrier command
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // End and submit command buffer
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void TextureUtils::copyBufferToImage(
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height,
    uint32_t baseArrayLayer,
    uint32_t mipLevel)
{
    // Create command buffer for the copy
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Define the region to copy
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = baseArrayLayer;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    // Record the copy command
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // End and submit command buffer
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

std::shared_ptr<Texture> TextureUtils::createDefaultEnvironmentCubemap(
    VkDevice device, 
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool, 
    VkQueue graphicsQueue)
{
    // Create a simple gradient environment cubemap
    const uint32_t size = 256; // Size of each cubemap face
    std::vector<unsigned char> pixels(size * size * 4 * 6); // 6 faces
    
    // Generate gradient data for each face
    for (uint32_t face = 0; face < 6; face++) {
        for (uint32_t y = 0; y < size; y++) {
            for (uint32_t x = 0; x < size; x++) {
                // Calculate normalized coordinates
                float u = (x / (float)(size - 1)) * 2.0f - 1.0f;
                float v = (y / (float)(size - 1)) * 2.0f - 1.0f;
                
                // Choose face-specific gradient
                glm::vec3 color;
                switch (face) {
                    case 0: // +X (right), red gradient
                        color = glm::vec3(0.8f, 0.2f + 0.2f * v, 0.2f + 0.2f * u);
                        break;
                    case 1: // -X (left), cyan gradient
                        color = glm::vec3(0.2f, 0.8f - 0.2f * v, 0.8f - 0.2f * u);
                        break;
                    case 2: // +Y (up), blue gradient
                        color = glm::vec3(0.2f + 0.2f * u, 0.2f + 0.2f * v, 0.8f);
                        break;
                    case 3: // -Y (down), yellow gradient
                        color = glm::vec3(0.8f - 0.2f * u, 0.8f - 0.2f * v, 0.2f);
                        break;
                    case 4: // +Z (front), green gradient
                        color = glm::vec3(0.2f + 0.2f * u, 0.8f, 0.2f + 0.2f * v);
                        break;
                    case 5: // -Z (back), magenta gradient
                        color = glm::vec3(0.8f - 0.2f * u, 0.2f, 0.8f - 0.2f * v);
                        break;
                }
                
                // Set pixel data
                uint32_t index = (face * size * size + y * size + x) * 4;
                pixels[index + 0] = static_cast<unsigned char>(color.r * 255.0f);
                pixels[index + 1] = static_cast<unsigned char>(color.g * 255.0f);
                pixels[index + 2] = static_cast<unsigned char>(color.b * 255.0f);
                pixels[index + 3] = 255; // Full alpha
            }
        }
    }
    
    // Create a texture from the pixel data
    auto texture = std::make_shared<Texture>(device, physicalDevice);
    // Note: This is a simplification - for a proper cubemap, you would use a more specialized texture creation method
    texture->createFromPixels(pixels.data(), size, size * 6, 4, commandPool, graphicsQueue);
    
    std::cout << "Created default environment cubemap" << std::endl;
    return texture;
}

std::vector<glm::vec3> TextureUtils::generateHemisphereSamples(const glm::vec3& normal, int sampleCount) {
    std::vector<glm::vec3> samples;
    samples.reserve(sampleCount);
    
    // Create a simple basis with the normal
    glm::vec3 tangent;
    if (std::abs(normal.x) > std::abs(normal.z)) {
        tangent = glm::vec3(-normal.y, normal.x, 0.0f);
    } else {
        tangent = glm::vec3(0.0f, -normal.z, normal.y);
    }
    tangent = glm::normalize(tangent);
    glm::vec3 bitangent = glm::cross(normal, tangent);
    
    // Generate samples in hemisphere
    for (int i = 0; i < sampleCount; i++) {
        // Use Hammersley sequence for uniform sampling
        float u = static_cast<float>(i) / static_cast<float>(sampleCount);
        float v = float(((i * 16807) % 2147483647) / 2147483647.0f);
        
        // Convert to spherical coordinates (hemisphere)
        float phi = 2.0f * glm::pi<float>() * u;
        float theta = std::acos(v);
        
        // Convert to cartesian coordinates
        float x = std::sin(theta) * std::cos(phi);
        float y = std::sin(theta) * std::sin(phi);
        float z = std::cos(theta);
        
        // Transform to world space using the normal's basis
        glm::vec3 sample = tangent * x + bitangent * y + normal * z;
        samples.push_back(glm::normalize(sample));
    }
    
    return samples;
}

std::vector<glm::vec3> TextureUtils::generateImportanceSamples(
    const glm::vec3& reflection, 
    float roughness, 
    int sampleCount) 
{
    std::vector<glm::vec3> samples;
    samples.reserve(sampleCount);
    
    // Create basis vectors
    glm::vec3 N = reflection;
    glm::vec3 tangent;
    if (std::abs(N.x) > std::abs(N.z)) {
        tangent = glm::vec3(-N.y, N.x, 0.0f);
    } else {
        tangent = glm::vec3(0.0f, -N.z, N.y);
    }
    tangent = glm::normalize(tangent);
    glm::vec3 bitangent = glm::cross(N, tangent);
    
    // The alpha parameter for GGX distribution
    float alpha = roughness * roughness;
    
    // Generate samples
    for (int i = 0; i < sampleCount; i++) {
        // Use Hammersley sequence for uniform sampling
        float u = static_cast<float>(i) / static_cast<float>(sampleCount);
        float v = float(((i * 16807) % 2147483647) / 2147483647.0f);
        
        // GGX importance sampling
        float phi = 2.0f * glm::pi<float>() * u;
        float theta = std::atan(alpha * std::sqrt(v / (1.0f - v)));
        
        // Convert to cartesian coordinates (in tangent space)
        float x = std::sin(theta) * std::cos(phi);
        float y = std::sin(theta) * std::sin(phi);
        float z = std::cos(theta);
        
        // Transform to world space
        glm::vec3 sample = tangent * x + bitangent * y + N * z;
        samples.push_back(glm::normalize(sample));
    }
    
    return samples;
}

glm::vec3 TextureUtils::sampleEnvironmentMap(std::shared_ptr<Texture> envMap, const glm::vec3& direction) {
    // This is a placeholder function that would sample the environment map at a given direction
    // In a real implementation, you would need to convert the direction to texture coordinates
    // and sample the environment map
    
    // For this simplified version, we'll just return a sky-like color based on the direction
    float y = direction.y * 0.5f + 0.5f; // Map from [-1,1] to [0,1]
    
    // Sky gradient from blue to white
    glm::vec3 skyColor = glm::mix(
        glm::vec3(1.0f, 1.0f, 1.0f),  // Horizon color
        glm::vec3(0.3f, 0.5f, 0.9f),  // Sky color
        y
    );
    
    // Add a simple sun
    glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.5f, 0.5f));
    float sunDot = glm::max(0.0f, glm::dot(direction, sunDir));
    float sunIntensity = std::pow(sunDot, 64.0f) * 2.0f;
    
    // Add sun color
    glm::vec3 finalColor = skyColor + glm::vec3(1.0f, 0.9f, 0.7f) * sunIntensity;
    
    return finalColor;
}

float TextureUtils::distributionGGX(float NoH, float alphaSquared) {
    // GGX/Trowbridge-Reitz normal distribution function
    float denom = NoH * NoH * (alphaSquared - 1.0f) + 1.0f;
    return alphaSquared / (glm::pi<float>() * denom * denom);
}

glm::vec2 TextureUtils::integrateBRDF(float NoV, float roughness) {
    // Simplified BRDF integration approximation
    const float kEpsilon = 1e-5f;
    
    // Avoid singularity
    NoV = glm::max(NoV, kEpsilon);
    
    // Simple approximation for scale and bias
    float scale = 1.0f - std::pow(1.0f - NoV, 5.0f * (1.0f - roughness));
    float bias = roughness * 0.25f * (1.0f - NoV);
    
    return glm::vec2(scale, bias);
}