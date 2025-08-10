#include "../include/debug/DebugUIManager.h"

#include "../VulkanRenderer.h"
#include "../include/camera/Camera.h"
#include "../include/scene/Scene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "../include/debug/DebugPanel.h"
#include <algorithm>
#include <cstring> 

//-----------------------------------------------------------------------------
// DebugUIManager Implementation
//-----------------------------------------------------------------------------

DebugUIManager::DebugUIManager(VulkanRenderer* renderer) 
    : renderer(renderer), device(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
      isVisible(true), initialized(false) {
}

DebugUIManager::~DebugUIManager() {
    if (initialized) {
        cleanup();
    }
}

void DebugUIManager::initialize(GLFWwindow* window, VkInstance instance, 
                                VkPhysicalDevice physicalDevice, VkDevice device,
                                uint32_t queueFamily, VkQueue queue, 
                                VkRenderPass renderPass, uint32_t imageCount) {
    this->device = device;
    
    // Create descriptor pool for ImGui
    createDescriptorPool(device);
    
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.Alpha = 0.9f;
    
    // Initialize ImGui for GLFW and Vulkan
    ImGui_ImplGlfw_InitForVulkan(window, true);
    
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = queueFamily;
    init_info.Queue = queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = imageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    init_info.RenderPass = renderPass;  // <-- IMPORTANT: Add this line!
    
    // Initialize ImGui Vulkan
    ImGui_ImplVulkan_Init(&init_info);
    
    initialized = true;
}

void DebugUIManager::createDescriptorPool(VkDevice device) {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
    };
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 100 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    
    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor pool!");
    }
}

void DebugUIManager::cleanup() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
        
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
    }
    
    initialized = false;
}

void DebugUIManager::beginFrame() {
    if (!initialized || !isVisible) return;
    
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Draw all active panels
    for (auto& panel : panels) {
        if (panel->getOpen()) {
            panel->draw();
        }
    }
}

void DebugUIManager::endFrame(VkCommandBuffer commandBuffer) {
    if (!initialized || !isVisible) return;
    
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void DebugUIManager::addPanel(std::shared_ptr<DebugPanel> panel) {
    panels.push_back(panel);
}

void DebugUIManager::removePanel(const std::string& name) {
    panels.erase(
        std::remove_if(panels.begin(), panels.end(),
            [&name](const std::shared_ptr<DebugPanel>& panel) {
                return panel->getName() == name;
            }),
        panels.end()
    );
}

void DebugUIManager::togglePanel(const std::string& name) {
    for (auto& panel : panels) {
        if (panel->getName() == name) {
            panel->toggle();
            break;
        }
    }
}


