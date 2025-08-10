#include "../include/scene/Scene.h"
#include "../VulkanRenderer.h"
#include <iostream>
#include <filesystem>


Scene::Scene(VulkanRenderer* renderer) : renderer(renderer) {
}

Scene::~Scene() {
    meshInstances.clear();
    textureCache.clear();
}

bool Scene::loadModel(const std::string& filename, const Transform& transform) {
    if (!modelLoader.LoadModel(filename)) {
        std::cerr << "Failed to load model: " << filename << std::endl;
        return false;
    }
    
    const std::vector<MeshData>& meshDataList = modelLoader.GetMeshData();
    if (meshDataList.empty()) {
        std::cerr << "No meshes found in model: " << filename << std::endl;
        return false;
    }
    
    createMeshesFromData(meshDataList, transform);
    return true;
}

void Scene::createMeshesFromData(const std::vector<MeshData>& meshDataList, 
                                 const Transform& transform) {//overloaded
    // Create a default material
    auto defaultMaterial = std::make_shared<Material>();
    
    // Call the full version of the method
    createMeshesFromData(meshDataList, transform, defaultMaterial);
}

// In Scene.cpp, modify loadTexturedModel to add better error checking:

bool Scene::loadTexturedModel(const std::string& modelFilename, const std::string& textureFilename, 
                             const Transform& transform) {
    if (!modelLoader.LoadModel(modelFilename)) {
        std::cerr << "Failed to load model: " << modelFilename << std::endl;
        return false;
    }
    
    const std::vector<MeshData>& meshDataList = modelLoader.GetMeshData();
    if (meshDataList.empty()) {
        std::cerr << "No meshes found in model: " << modelFilename << std::endl;
        return false;
    }
    
    // Create material first
    auto myMaterial = std::make_shared<Material>();
    
    // Load texture if filename provided
    std::shared_ptr<Texture> texture = nullptr;
if (!textureFilename.empty()) {
    std::cout << "Loading texture: " << textureFilename << std::endl;
    
    // Check if file exists
    if (!std::filesystem::exists(textureFilename)) {
        std::cerr << "ERROR: Texture file does not exist: " << textureFilename << std::endl;
    } else {
        texture = loadTexture(textureFilename);
        if (!texture) {
            std::cerr << "Failed to load texture: " << textureFilename << std::endl;
            std::cerr << "Using default white texture instead." << std::endl;
        } else {
            std::cout << "Texture loaded successfully: " << textureFilename << std::endl;
            
            // IMPORTANT: Set the texture on the material
            myMaterial->setTexture(TextureType::Diffuse, texture);
            std::cout << "Texture set on material as Diffuse map" << std::endl;
        }
    }
}
    
    // Set default material properties
    myMaterial->diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f); // White base color
    myMaterial->metallic = 0.0f;  // Non-metallic
    myMaterial->roughness = 0.8f; // Slightly rough
    myMaterial->alpha = 1.0f;     // Fully opaque
    
    // Create descriptor set for the material
    VkDescriptorSet materialDescriptorSet = renderer->createMaterialDescriptorSet(*myMaterial);
    if (materialDescriptorSet == VK_NULL_HANDLE) {
        std::cerr << "Failed to create material descriptor set!" << std::endl;
        return false;
    }
    myMaterial->setDescriptorSet(materialDescriptorSet);
    std::cout << "Material descriptor set created and assigned" << std::endl;
    
    // Create meshes with the material
    createMeshesFromData(meshDataList, transform, myMaterial);
    
    std::cout << "Model loaded with " << meshDataList.size() << " mesh(es)" << std::endl;
    return true;
}


bool Scene::loadTexturedModelPBR(const std::string& modelFilename, 
                               const MaterialTexturePaths& texturePaths,
                               const Transform& transform) {
    if (!modelLoader.LoadModel(modelFilename)) {
        std::cerr << "Failed to load model: " << modelFilename << std::endl;
        return false;
    }
    
    const std::vector<MeshData>& meshDataList = modelLoader.GetMeshData();
    if (meshDataList.empty()) {
        std::cerr << "No meshes found in model: " << modelFilename << std::endl;
        return false;
    }
    
    // Create material with multiple textures
    auto material = std::make_shared<Material>();
    material->diffuseColor = glm::vec3(0.8f, 0.2f, 0.2f); // Bright red color
    material->metallic = 0.0f; // Non-metallic
    material->roughness = 0.5f; // Medium roughness
    material->alpha = 1.0f; 
    //Material material = createMaterialWithTextures(texturePaths);
    
    // Create meshes with the material
    createMeshesFromData(meshDataList, transform, material);
    return true;
}

Material Scene::createMaterialWithTextures(const MaterialTexturePaths& texturePaths) {
    Material material;
    
    // Load diffuse/albedo texture if provided
    if (!texturePaths.diffuse.empty()) {
        auto texture = loadTexture(texturePaths.diffuse);
        if (texture) {
            material.setTexture(TextureType::Diffuse, texture);
        }
    }
    
    // Load normal map if provided
    if (!texturePaths.normal.empty()) {
        auto texture = loadTexture(texturePaths.normal);
        if (texture) {
            material.setTexture(TextureType::Normal, texture);
            
        }
    }
    
    // Load metallic map if provided
    if (!texturePaths.metallic.empty()) {
        auto texture = loadTexture(texturePaths.metallic);
        if (texture) {
            material.setTexture(TextureType::Metallic, texture);
        }
    }
    
    // Load roughness map if provided
    if (!texturePaths.roughness.empty()) {
        auto texture = loadTexture(texturePaths.roughness);
        if (texture) {
            material.setTexture(TextureType::Roughness, texture);
        }
    }
    
    // Load ambient occlusion map if provided
    if (!texturePaths.ambientOcclusion.empty()) {
        auto texture = loadTexture(texturePaths.ambientOcclusion);
        if (texture) {
            material.setTexture(TextureType::AmbientOcclusion, texture);
        }
    }
    
    // Load emissive map if provided
    if (!texturePaths.emissive.empty()) {
        auto texture = loadTexture(texturePaths.emissive);
        if (texture) {
            material.setTexture(TextureType::Emissive, texture);
        }
    }
    
    // Load height/displacement map if provided
    if (!texturePaths.height.empty()) {
        auto texture = loadTexture(texturePaths.height);
        if (texture) {
            material.setTexture(TextureType::Height, texture);
        }
    }
    
    // Load specular map if provided
    if (!texturePaths.specular.empty()) {
        auto texture = loadTexture(texturePaths.specular);
        if (texture) {
            material.setTexture(TextureType::Specular, texture);
        }
    }
    
    return material;
}

std::shared_ptr<Texture> Scene::loadTexture(const std::string& filename) {
    // Check if file exists
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Texture file does not exist: " << filename << std::endl;
        return nullptr;
    }
    
    // Check if texture is already loaded
    auto it = textureCache.find(filename);
    if (it != textureCache.end()) {
        return it->second;
    }
    
    // Create new texture
    auto texture = std::make_shared<Texture>(renderer->getDevice(), renderer->getPhysicalDevice());
    
    // Load texture from file
    if (!texture->loadFromFile(filename, renderer->getCommandPool(), renderer->getGraphicsQueue())) {
        std::cerr << "Failed to load texture from file: " << filename << std::endl;
        return nullptr;
    }
    
    // Cache the texture
    textureCache[filename] = texture;
    
    return texture;
}

void Scene::createMeshesFromData(const std::vector<MeshData>& meshDataList, const Transform& transform,
                               const std::shared_ptr<Material>& material) {
    for (const auto& meshData : meshDataList) {
        // Create a new mesh with the provided material (shared pointer)
        auto mesh = std::make_shared<Mesh>(renderer->getDevice(), renderer->getPhysicalDevice(), 
                                        meshData, material);
        mesh->createBuffers(renderer->getCommandPool(), renderer->getGraphicsQueue());
        
        // Create an instance of this mesh
        meshInstances.emplace_back(mesh, transform);
    }
}

void Scene::addMeshInstance(std::shared_ptr<Mesh> mesh, const Transform& transform) {
    meshInstances.emplace_back(mesh, transform);
}

void Scene::update(float deltaTime) {
    // Update transforms or animations if needed
    for (auto& instance : meshInstances) {
        // Example: rotate each mesh
        instance.transform.rotation.y += deltaTime * 0.5f; // Rotate around Y axis
    }
    
}

void Scene::draw(VkCommandBuffer commandBuffer, const glm::mat4& view, const glm::mat4& proj, uint32_t frameIndex) {
    // Common setup - update view/projection matrices
    renderer->updateViewProjection(view, proj);
    
    // Check which pipeline to use
    bool usePBR = renderer->getRenderMode() == RenderMode::PBR || 
                 renderer->getRenderMode() == RenderMode::PBR_IBL;
    
    // Bind the appropriate pipeline
    if (usePBR) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->getPBRPipeline());
        
        // Bind MVP descriptor set (set 0)
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->getPBRPipelineLayout(),
            0,  // Set index 0
            1,  // One descriptor set
            &renderer->getMVPDescriptorSets()[frameIndex],
            0, nullptr
        );
        
        // Bind light descriptor set (set 2)
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->getPBRPipelineLayout(),
            2,  // Set index 2
            1,  // One descriptor set
            &renderer->getLightDescriptorSets()[frameIndex],
            0, nullptr
        );
    } else {
        // Use standard pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->getGraphicsPipeline());
        
        // Bind MVP descriptor set
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->getPipelineLayout(),
            0,  // Set index 0
            1,  // One descriptor set
            &renderer->getMVPDescriptorSets()[frameIndex],
            0, nullptr
        );
    }
    
    // Draw each mesh instance
    for (const auto& instance : meshInstances) {
        // Get the model matrix for this instance
        glm::mat4 model = instance.transform.getModelMatrix();
        
        // Push the model matrix as a push constant
     
        PushConstant pushConstant = renderer->createPushConstant(
                model, 
                *instance.mesh->getMaterial()
            );
        if (usePBR) {
            // Push model matrix to the PBR pipeline
            vkCmdPushConstants(
                commandBuffer,
                renderer->getPBRPipelineLayout(),
                 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstant),
                &pushConstant
            );
            
            // Get the material descriptor set (set 1)
            VkDescriptorSet materialDescriptorSet = instance.mesh->getMaterial()->getDescriptorSet();
            if (materialDescriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderer->getPBRPipelineLayout(),
                    1,  // Set index 1
                    1,  // One descriptor set
                    &materialDescriptorSet,
                    0, nullptr
                );
            }
            
        } else {
            // Use standard pipeline
            vkCmdPushConstants(
                commandBuffer,
                renderer->getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstant),
                &pushConstant
            );
            
            // Bind material descriptor set for standard pipeline
            VkDescriptorSet materialDescriptorSet = instance.mesh->getMaterial()->getDescriptorSet();
            if (materialDescriptorSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderer->getPipelineLayout(),
                    1,  // Set index 1
                    1,  // One descriptor set
                    &materialDescriptorSet,
                    0, nullptr
                );
            }
        }
        
        // Draw the mesh
        instance.mesh->bind(commandBuffer);
        instance.mesh->draw(commandBuffer);
    }
}



void Scene::addLight(const glm::vec3& position, const glm::vec3& color, 
                    float intensity, float radius, float falloff, bool isDirectional) {
    Light light;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    light.falloff = falloff;
    light.isDirectional = isDirectional;
    
    lights.push_back(light);
}


void Scene::clearMeshInstances() {
    meshInstances.clear();
}


void Scene::removeLight(size_t index) {
    if (index < lights.size()) {
        lights.erase(lights.begin() + index);
    }
}



// In Scene::setupDefaultLighting(), reduce light intensities:

void Scene::setupDefaultLighting() {
    // Clear any existing lights
    clearLights();
    
    // Add a main directional light (sun) with REDUCED intensity
    addLight(
        glm::vec3(1.0f, 1.0f, 1.0f),    // Direction (will be normalized)
        glm::vec3(1.0f, 0.95f, 0.9f),   // Slightly warm white color
        1.0f,                            // Reduced intensity (was 2.0f)
        0.0f,                            // Radius (0 for directional lights)
        1.0f,                            // Falloff (unused for directional)
        true                             // isDirectional = true
    );
    
    // Add a fill light from the opposite direction with REDUCED intensity
    addLight(
        glm::vec3(-0.5f, 0.2f, -0.5f),  // Direction
        glm::vec3(0.6f, 0.7f, 1.0f),    // Slightly blue color
        0.3f,                            // Lower intensity (was 0.5f)
        0.0f,                            // Radius
        1.0f,                            // Falloff
        true                             // isDirectional
    );
    
    // Add a point light with REDUCED intensity
    addLight(
        glm::vec3(2.0f, 1.0f, 2.0f),    // Position
        glm::vec3(1.0f, 0.8f, 0.6f),    // Warm color
        2.0f,                            // Reduced intensity (was 5.0f)
        10.0f,                           // Radius
        2.0f,                            // Falloff
        false                            // isPoint
    );
}

bool Scene::loadPBRModel(
    const std::string& modelFilename,
    const MaterialTexturePaths& texturePaths,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
{
    // Create transform
    Transform transform;
    transform.position = position;
    transform.rotation = rotation;
    transform.scale = scale;
    
    // Load model with PBR materials
    return loadTexturedModelPBR(modelFilename, texturePaths, transform);
}


Material Scene::createPBRMaterial(
    const std::string& albedoPath,
    const std::string& normalPath,
    const std::string& metallicPath,
    const std::string& roughnessPath,
    const std::string& aoPath,
    const std::string& emissivePath,
    float metallic,
    float roughness,
    const glm::vec3& baseColor,
    float emissiveStrength)
{
    Material material;
    
    // Set base color and PBR scalar properties
    material.diffuseColor = baseColor;
    material.setPBRProperties(metallic, roughness);
    material.emissiveStrength = emissiveStrength;
    
    // Load each texture if provided
    std::shared_ptr<Texture> albedoTex = albedoPath.empty() ? nullptr : loadTexture(albedoPath);
    std::shared_ptr<Texture> normalTex = normalPath.empty() ? nullptr : loadTexture(normalPath);
    std::shared_ptr<Texture> metallicTex = metallicPath.empty() ? nullptr : loadTexture(metallicPath);
    std::shared_ptr<Texture> roughnessTex = roughnessPath.empty() ? nullptr : loadTexture(roughnessPath);
    std::shared_ptr<Texture> aoTex = aoPath.empty() ? nullptr : loadTexture(aoPath);
    std::shared_ptr<Texture> emissiveTex = emissivePath.empty() ? nullptr : loadTexture(emissivePath);
    
    // If both metallic and roughness are provided, we could combine them
    std::shared_ptr<Texture> metallicRoughnessTex = nullptr;
    if (metallicTex && roughnessTex) {
        // Try to create a combined texture
        metallicRoughnessTex = TextureUtils::combineMetallicRoughness(
            renderer->getDevice(),
            renderer->getPhysicalDevice(),
            renderer->getCommandPool(),
            renderer->getGraphicsQueue(),
            metallicTex,
            roughnessTex,
            metallic,
            roughness
        );
    } else if (metallicTex) {
        // Just use metallic texture if only that's available
        metallicRoughnessTex = metallicTex;
    } else if (roughnessTex) {
        // Just use roughness texture if only that's available
        metallicRoughnessTex = roughnessTex;
    } else if (metallic >= 0.0f && roughness >= 0.0f) {
        // Create a default texture with the provided scalar values
        metallicRoughnessTex = TextureUtils::createDefaultMetallicRoughnessMap(
            renderer->getDevice(),
            renderer->getPhysicalDevice(),
            renderer->getCommandPool(),
            renderer->getGraphicsQueue(),
            metallic,
            roughness
        );
    }
    
    // Set textures
    material.setPBRTextures(
        albedoTex,
        normalTex,
        metallicRoughnessTex,     // Will contain combined or individual metallic/roughness
        nullptr,                  // Not needed if we have combined texture
        aoTex,
        emissiveTex
    );
    
    return material;
}

// Replace the existing setupEnvironment implementation in Scene.cpp
bool Scene::setupEnvironment(const std::string& hdriPath) {
    if (!renderer) {
        std::cerr << "Renderer not initialized" << std::endl;
        return false;
    }
    
    // Set up IBL with the given HDRI environment map
    try {
        bool success = renderer->setupIBL(hdriPath);
        
        if (success) {
            std::cout << "Environment setup successful with HDRI: " << hdriPath << std::endl;
            
            // Switch to PBR_IBL mode if IBL is successfully set up
            renderer->setRenderMode(RenderMode::PBR_IBL);
        } else {
            std::cerr << "Failed to set up environment with HDRI: " << hdriPath << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set up environment: " << e.what() << std::endl;
        return false;
    }
}




const std::vector<MeshInstance>& Scene::getMeshInstances() const {
    return meshInstances;
}


void Scene::clearLights() {
    lights.clear();
}
