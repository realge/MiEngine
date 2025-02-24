#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "../include/mesh/Mesh.h"
#include "../include/loader/ModelLoader.h"

// Forward declarations
class VulkanRenderer;

// Struct to hold transform data for each mesh instance
struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

// Struct to represent a mesh instance in the scene
struct MeshInstance {
    std::shared_ptr<Mesh> mesh;
    Transform transform;
    
    MeshInstance(std::shared_ptr<Mesh> m, const Transform& t = Transform())
        : mesh(m), transform(t) {}
};

class Scene {
public:
    Scene(VulkanRenderer* renderer);
    ~Scene();

    // Load a model and add all its meshes to the scene
    bool loadModel(const std::string& filename, const Transform& transform = Transform());
    
    // Add a single mesh instance to the scene
    void addMeshInstance(std::shared_ptr<Mesh> mesh, const Transform& transform = Transform());
    
    // Update all mesh transforms
    void update(float deltaTime);
    
    // Record draw commands for all meshes
    void draw(VkCommandBuffer commandBuffer, const glm::mat4& view, const glm::mat4& proj);

private:
    VulkanRenderer* renderer;
    std::vector<MeshInstance> meshInstances;
    std::vector<std::shared_ptr<Mesh>> uniqueMeshes; // Store unique mesh data
    ModelLoader modelLoader;

    // Helper to create mesh objects from loaded mesh data
    void createMeshesFromData(const std::vector<MeshData>& meshDataList, const Transform& transform);
};