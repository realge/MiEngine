#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "../material/Material.h" 
#include "../loader/ModelLoader.h"  // For MeshData

class Mesh {
public:
    // Construct a Mesh from loaded mesh data
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice, 
         const MeshData& meshData, 
         const std::shared_ptr<Material>& material = std::make_shared<Material>());
    ~Mesh();

    // Create GPU buffers for vertices and indices using provided command pool and graphics queue
    void createBuffers(VkCommandPool commandPool, VkQueue graphicsQueue);

    // Bind vertex and index buffers to the given command buffer
    void bind(VkCommandBuffer commandBuffer) const;

    // Issue draw command for this mesh
    void draw(VkCommandBuffer commandBuffer) const;

    // Then update the getter and setter:
    // Get mesh material (returns a reference to the shared_ptr)
    const std::shared_ptr<Material>& getMaterial() const { return material; }

    // Set mesh material (takes a shared_ptr to a material)
    void setMaterial(const std::shared_ptr<Material>& newMaterial) { material = newMaterial; }
    
private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    std::shared_ptr<Material> material;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;

    // Local copies of the mesh data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Internal helpers
    void createVertexBuffer(VkCommandPool commandPool, VkQueue graphicsQueue);
    void createIndexBuffer(VkCommandPool commandPool, VkQueue graphicsQueue);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkCommandPool commandPool, VkQueue graphicsQueue,
                    VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};