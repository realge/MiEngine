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
    
    // Load texture
    std::shared_ptr<Texture> texture = loadTexture(textureFilename);
    if (!texture) {
        std::cerr << "Failed to load texture: " << textureFilename << std::endl;
        // Continue without texture
        createMeshesFromData(meshDataList, transform);
        return true;
    }
    
    // Create material with texture
    auto myMaterial = std::make_shared<Material>();
    myMaterial->setTexture(TextureType::Diffuse, texture);
    // Create meshes with the material
    VkDescriptorSet materialDescriptorSet = renderer->createMaterialDescriptorSet(*myMaterial);
    myMaterial->setDescriptorSet(materialDescriptorSet);
    createMeshesFromData(meshDataList, transform, myMaterial);
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


    vkCmdBindDescriptorSets(
    commandBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    renderer->getPipelineLayout(),
    0,  // Set index 0
    1,  // One descriptor set
    &renderer->mvpDescriptorSets[frameIndex],
    0, nullptr
);
    renderer->updateViewProjection( view, proj);
    for (const auto& instance : meshInstances) {
        // Get the model matrix for this instance
        glm::mat4 model = instance.transform.getModelMatrix();

        //model = glm::scale(glm::mat4(1.0f), glm::vec3(19.0f));
        // Update uniform buffer with MVP matrices
        
        // Push the model matrix as a push constant
        ModelPushConstant pushConstant = { model };
        vkCmdPushConstants(
            commandBuffer,
            renderer->getPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(ModelPushConstant),
            &pushConstant
        );
       
        VkDescriptorSet materialDescriptorSet = instance.mesh->getMaterial()->getDescriptorSet();
        if (materialDescriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                renderer->getPipelineLayout(),  // You'll need to add a getter for this
                1,  // First set index
                1,  // Number of descriptor sets
                &materialDescriptorSet,
                0,
                nullptr
            );
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

void Scene::removeLight(size_t index) {
    if (index < lights.size()) {
        lights.erase(lights.begin() + index);
    }
}



void Scene::setupDefaultLighting() {
    // Clear any existing lights
    clearLights();
    
    // Add a main directional light (sun)
    addLight(
        glm::vec3(1.0f, 1.0f, 1.0f),    // Direction (will be normalized)
        glm::vec3(1.0f, 0.95f, 0.9f),   // Slightly warm white color
        2.0f,                           // Intensity
        0.0f,                           // Radius (0 for directional lights)
        1.0f,                           // Falloff (unused for directional)
        true                            // isDirectional = true
    );
    
    // Add a fill light from the opposite direction
    addLight(
        glm::vec3(-0.5f, 0.2f, -0.5f),  // Direction
        glm::vec3(0.6f, 0.7f, 1.0f),    // Slightly blue color
        0.5f,                           // Lower intensity
        0.0f,                           // Radius
        1.0f,                           // Falloff
        true                            // isDirectional
    );
    
    // Add a point light
    addLight(
        glm::vec3(2.0f, 1.0f, 2.0f),    // Position
        glm::vec3(1.0f, 0.8f, 0.6f),    // Warm color
        5.0f,                           // Intensity
        10.0f,                          // Radius
        2.0f,                           // Falloff
        false                           // isPoint
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

bool Scene::setupEnvironment(const std::string& hdriPath) {
    if (!renderer) {
        std::cerr << "Renderer not initialized" << std::endl;
        return false;
    }
    
    // Set up IBL with the given HDRI environment map
    try {
        //renderer->setupIBL(hdriPath);
        return true;
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
