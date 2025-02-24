#include "../include//scene/Scene.h"

#include "../VulkanRenderer.h"
#include <iostream>

Scene::Scene(VulkanRenderer* renderer) : renderer(renderer) {
}

Scene::~Scene() {
    uniqueMeshes.clear();
    meshInstances.clear();
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

void Scene::createMeshesFromData(const std::vector<MeshData>& meshDataList, const Transform& transform) {
    for (const auto& meshData : meshDataList) {
        // Create a new mesh
        auto mesh = std::make_shared<Mesh>(renderer->getDevice(), renderer->getPhysicalDevice(), meshData);
        mesh->createBuffers(renderer->getCommandPool(), renderer->getGraphicsQueue());
        
        // Store the unique mesh
        uniqueMeshes.push_back(mesh);
        
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
        
        // Draw the mesh
        instance.mesh->bind(commandBuffer);
        instance.mesh->draw(commandBuffer);
    }
}