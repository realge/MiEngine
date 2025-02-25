#include "../include/scene/Scene.h"
#include "../VulkanRenderer.h"
#include <iostream>

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

std::shared_ptr<Texture> Scene::loadTexture(const std::string& filename) {
    // Check if texture is already loaded
    auto it = textureCache.find(filename);
    if (it != textureCache.end()) {
        return it->second;
    }
    
    // Create new texture
    auto texture = std::make_shared<Texture>(renderer->getDevice(), renderer->getPhysicalDevice());
    
    // Load texture from file
    if (!texture->loadFromFile(filename, renderer->getCommandPool(), renderer->getGraphicsQueue())) {
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