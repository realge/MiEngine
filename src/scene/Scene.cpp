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
    Material material(texture);
    
    // Create meshes with the material
    createMeshesFromData(meshDataList, transform, material);
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
    Material material = createMaterialWithTextures(texturePaths);
    
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
                               const Material& material) {
    for (const auto& meshData : meshDataList) {
        // Create a new mesh with the provided material
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

void Scene::draw(VkCommandBuffer commandBuffer, const glm::mat4& view, const glm::mat4& proj) {
    for (const auto& instance : meshInstances) {
        // Get the model matrix for this instance
        glm::mat4 model = instance.transform.getModelMatrix();
        
        // Update uniform buffer with MVP matrices
        renderer->updateMVPMatrices(model, view, proj);
        
        // Update descriptor set with mesh's texture (if any)
        if (instance.mesh->getMaterial().useTexture && 
            instance.mesh->getMaterial().diffuseTexture) {
            renderer->updateTextureDescriptor(instance.mesh->getMaterial().getTextureImageInfo());
        }
        
        // Draw the mesh
        instance.mesh->bind(commandBuffer);
        instance.mesh->draw(commandBuffer);
    }
}