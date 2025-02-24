#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "../loader/ModelLoader.h"  // For MeshData

class Mesh {
public:
    // Construct a Mesh from loaded mesh data
    Mesh(VkDevice device, VkPhysicalDevice physicalDevice, const MeshData& meshData);
    ~Mesh();

    // Create GPU buffers for vertices and indices using provided command pool and graphics queue
    void createBuffers(VkCommandPool commandPool, VkQueue graphicsQueue);

    // Bind vertex and index buffers to the given command buffer
    void bind(VkCommandBuffer commandBuffer) const;

    // Issue draw command for this mesh
    void draw(VkCommandBuffer commandBuffer) const;

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;

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