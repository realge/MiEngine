#include "VulkanRenderer.h"

#include "include/debug/CameraDebugPanel.h"
#include "include/debug/PerformancePanel.h"
#include "include/debug/RenderDebugPanel.h"
#include "include/debug/SceneHierarchyPanel.h"
#include "include/debug/SettingsPanel.h"


//===================camera==================
static VulkanRenderer* rendererInstance = nullptr;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (rendererInstance) {
        rendererInstance->processKeyboard(key, action);
    }
    
    // Handle escape key to toggle mouse capture
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (rendererInstance->mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            rendererInstance->mouseCaptured = false;
            rendererInstance->firstMouse = true;
        }
    }
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (rendererInstance) {
        rendererInstance->processMouseMovement(xpos, ypos);
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (rendererInstance) {
        rendererInstance->processMouseButton(button, action);
    }
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (rendererInstance) {
        rendererInstance->processScroll(xoffset, yoffset);
    }
}

void VulkanRenderer::processKeyboard(int key, int action) {
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }
    if (action == GLFW_PRESS) {
        // Toggle debug UI visibility
        if (key == GLFW_KEY_F1) {
            if (debugUI) {
                debugUI->toggleVisibility();
            }
        }
        
        // Toggle specific panels
        if (key == GLFW_KEY_F2) {
            if (debugUI) {
                debugUI->togglePanel("Camera Debug");
            }
        }
        if (key == GLFW_KEY_F3) {
            if (debugUI) {
                debugUI->togglePanel("Render Debug");
            }
        }
        if (key == GLFW_KEY_F4) {
            if (debugUI) {
                debugUI->togglePanel("Performance");
            }
        }
        if (key == GLFW_KEY_F5) {
            if (debugUI) {
                debugUI->togglePanel("Scene Hierarchy");
            }
        }
        if (key == GLFW_KEY_F6) {
            if (debugUI) {
                debugUI->togglePanel("Settings");
            }
        }
    }
}

void VulkanRenderer::processMouseMovement(double xpos, double ypos) {
    if (!mouseCaptured) return;
    
    float xposf = static_cast<float>(xpos);
    float yposf = static_cast<float>(ypos);
    
    if (firstMouse) {
        lastX = xposf;
        lastY = yposf;
        firstMouse = false;
    }
    
    float xoffset = xposf - lastX;
    float yoffset = lastY - yposf; // Reversed since y-coordinates range from bottom to top
    
    lastX = xposf;
    lastY = yposf;
    
    if (camera) {
        camera->processMouseMovement(xoffset, yoffset);
    }
}

void VulkanRenderer::processMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (!mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
            firstMouse = true;
        }
    }
}

void VulkanRenderer::processScroll(double xoffset, double yoffset) {
    if (camera) {
        camera->processMouseScroll(static_cast<float>(yoffset));
    }
}

void VulkanRenderer::updateCamera(float deltaTime) {
    if (!camera || !mouseCaptured) return;
    
    // Camera movement
    if (keys[GLFW_KEY_W]) {
        camera->processKeyboard(CameraMovement::FORWARD, deltaTime);
    }
    if (keys[GLFW_KEY_S]) {
        camera->processKeyboard(CameraMovement::BACKWARD, deltaTime);
    }
    if (keys[GLFW_KEY_A]) {
        camera->processKeyboard(CameraMovement::LEFT, deltaTime);
    }
    if (keys[GLFW_KEY_D]) {
        camera->processKeyboard(CameraMovement::RIGHT, deltaTime);
    }
    if (keys[GLFW_KEY_SPACE]) {
        camera->processKeyboard(CameraMovement::UP, deltaTime);
    }
    if (keys[GLFW_KEY_LEFT_SHIFT] || keys[GLFW_KEY_LEFT_CONTROL]) {
        camera->processKeyboard(CameraMovement::DOWN, deltaTime);
    }
    
    // Speed boost with shift
    if (keys[GLFW_KEY_LEFT_SHIFT]) {
        camera->setMovementSpeed(10.0f);
    } else {
        camera->setMovementSpeed(5.0f);
    }
}

// Helper methods for single time commands:
VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

//========================================

std::vector<const char*> deviceExtensions = {
   VK_KHR_SWAPCHAIN_EXTENSION_NAME
};



const uint32_t WIDTH = 1800;
const uint32_t HEIGHT = 900;

VulkanRenderer::VulkanRenderer() {
   

}



QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());
    int i = 0;
    for (const auto& family : families) {
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            indices.presentFamily = i;
        if (indices.isComplete()) break;
        i++;
    }
    return indices;
}

bool VulkanRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        VkExtent2D actualExtent = { WIDTH, HEIGHT };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    // Fallback to the first format if your preferred one isn’t found
    return availableFormats[0];
}
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // First try to find mailbox mode (triple buffering)
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

//above is all help functions

void VulkanRenderer::initVulkan() {
    createInstance();
    std::cout << "Instance created" << std::endl;
    setupDebugMessenger();
    std::cout << "Debug messenger created" << std::endl;
    createSurface();
    std::cout << "Surface created" << std::endl;
    pickPhysicalDevice();
    std::cout << "Physical device picked" << std::endl;
    createLogicalDevice();
    std::cout << "Logical device created" << std::endl;
    createSwapChain();
    std::cout << "Swap chain created" << std::endl;
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    createImageViews();
    std::cout << "Image views created" << std::endl;
    createRenderPass();
    std::cout << "Render pass created" << std::endl;

    initializeDebugUI();
    std::cout << "Debug UI initialized" << std::endl;
    createDescriptorSetLayouts();
    std::cout << "Descriptor set layouts created" << std::endl;
    createLightDescriptorSetLayout();
    std::cout << "Light descriptor set layout created" << std::endl;
    createLightUniformBuffers();
    std::cout << "Light uniform buffers created" << std::endl;
    
    createGraphicsPipeline();
    std::cout << "Graphics pipeline created" << std::endl;
    
    createDepthResources();
    std::cout << "Depth resources created" << std::endl;
    createFramebuffers();
    std::cout << "Framebuffers created" << std::endl;
    
    // Create command pool BEFORE anything that needs it
    createCommandPool();
    std::cout << "Command pool created" << std::endl;
    
    // Create skybox mesh BEFORE descriptor pool (it doesn't need descriptors, just command pool)
    std::cout << "Creating skybox mesh..." << std::endl;
    MeshData skyboxData = modelLoader.CreateCube(1.0f);
    if (skyboxData.vertices.empty() || skyboxData.indices.empty()) {
        throw std::runtime_error("Failed to create skybox mesh data!");
    }
    auto skyboxMaterial = std::make_shared<Material>();
    skyboxMesh = std::make_shared<Mesh>(device, physicalDevice, skyboxData, skyboxMaterial);
    skyboxMesh->createBuffers(commandPool, graphicsQueue);
    std::cout << "Skybox mesh created and buffers initialized." << std::endl;
    
    // Create default textures (needs command pool)
    createDefaultTextures();
    std::cout << "Default textures created" << std::endl;
    
    // Create uniform buffers
    createUniformBuffers();
    std::cout << "Uniform buffers created" << std::endl;
    createMaterialUniformBuffers();
    std::cout << "Material uniform buffers created" << std::endl;
    
    // IMPORTANT: Create descriptor pool BEFORE IBL initialization
    createDescriptorPool();
    std::cout << "Descriptor pool created" << std::endl;

    // Set IBL Quality
    TextureUtils::setIBLQuality(TextureUtils::IBLQuality::MEDIUM);
    std::cout << "IBL Quality set to HIGH" << std::endl;
    
    // NOW initialize IBL system (after descriptor pool is created)
    std::cout << "Initializing IBL system..." << std::endl;
    iblSystem = std::make_unique<IBLSystem>(this);
    bool iblInitialized = false;
    if (iblSystem->initialize("hdr/sky.hdr")) {
        std::cout << "IBL system initialized successfully" << std::endl;
        iblInitialized = true;
        
        // Create skybox pipeline after IBL is ready
        createSkyboxPipeline();
        std::cout << "Skybox pipeline created" << std::endl;
    } else {
        std::cerr << "Failed to initialize IBL system - skybox will not be available" << std::endl;
    }
    
    // Create PBR pipeline (with or without IBL)
    createPBRPipeline();
    std::cout << "PBR pipeline created" << (iblInitialized ? " with IBL" : " without IBL") << std::endl;
    
    // Create descriptor sets (after pool and IBL are ready)
    createDescriptorSets();
    std::cout << "Descriptor sets created" << std::endl;
    
    createLightDescriptorSets();
    std::cout << "Light descriptor sets created" << std::endl;
    
    createCommandBuffers();
    std::cout << "Command buffers created" << std::endl;
    
    createSyncObjects();
    std::cout << "Sync objects created" << std::endl;
    
    // Initialize scene
    scene = std::make_unique<Scene>(this);
    
    // Load initial models
    Transform modelTransform;
    modelTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    modelTransform.scale = glm::vec3(19.0f);
    
    // Load your model
    scene->setupDefaultLighting();  // <-- ADD THIS LINE

   
    
    // Set up camera
    cameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    fov = 100.0f;
    nearPlane = 0.01f;
    farPlane = 600.0f;




    // Initialize camera system
    camera = std::make_unique<Camera>(
        glm::vec3(2.0f, 2.0f, 2.0f),  // Initial position
        glm::vec3(0.0f, 1.0f, 0.0f),  // Up vector
        0.0f,                        // Yaw
        0.0f                           // Pitch
    );
    
    // Look at origin initially
    camera->lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
    
    std::cout << "Camera system initialized" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  - Click to capture mouse" << std::endl;
    std::cout << "  - WASD to move" << std::endl;
    std::cout << "  - Space/Shift to move up/down" << std::endl;
    std::cout << "  - Mouse to look around" << std::endl;
    std::cout << "  - Scroll to zoom" << std::endl;
    std::cout << "  - ESC to release mouse" << std::endl;
    
    // Set render mode if you want PBR with IBL
    if (iblInitialized && iblSystem && iblSystem->isReady()) {
        renderMode = RenderMode::PBR_IBL;
        std::cout << "Render mode set to PBR_IBL" << std::endl;
    } else {
        renderMode = RenderMode::PBR;
        std::cout << "Render mode set to PBR (without IBL)" << std::endl;
    }
    
    createPBRIBLTestScene();

    scene->loadTexturedModel("models/blackrat.fbx", "texture/blackrat_color.png", modelTransform);
}

// In VulkanRenderer.cpp, replace the setupIBL method with this fixed version:

bool VulkanRenderer::setupIBL(const std::string& hdriPath) {
    if (!iblSystem) {
        iblSystem = std::make_unique<IBLSystem>(this);
    }
    
    bool success = iblSystem->initialize(hdriPath);
    
    if (success) {
        // IMPORTANT: Don't destroy and recreate the skybox pipeline layout here!
        // The skybox pipeline was already created during initialization.
        // We just need to update the existing descriptor sets.
        
        // Update skybox descriptor sets if they were already created
        if (!skyboxDescriptorSets.empty() && skyboxDescriptorSetLayout != VK_NULL_HANDLE) {
            // Wait for device to be idle before updating descriptor sets
            vkDeviceWaitIdle(device);
            
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
                VkDescriptorImageInfo skyboxImageInfo{};
                skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                
                // Make sure the environment map is valid
                if (iblSystem->getEnvironmentMap()) {
                    skyboxImageInfo.imageView = iblSystem->getEnvironmentMap()->getImageView();
                    skyboxImageInfo.sampler = iblSystem->getEnvironmentMap()->getSampler();
                } else {
                    std::cerr << "Error: Environment map is null!" << std::endl;
                    return false;
                }

                VkWriteDescriptorSet skyboxWrite{};
                skyboxWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                skyboxWrite.dstSet = skyboxDescriptorSets[i];
                skyboxWrite.dstBinding = 0;
                skyboxWrite.dstArrayElement = 0;
                skyboxWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                skyboxWrite.descriptorCount = 1;
                skyboxWrite.pImageInfo = &skyboxImageInfo;

                vkUpdateDescriptorSets(device, 1, &skyboxWrite, 0, nullptr);
            }
        } else {
            // If descriptor sets don't exist yet, we need to create them
            // But the pipeline and layout should already exist from initialization
            std::cerr << "Warning: Skybox descriptor sets not initialized during setupIBL" << std::endl;
        }
        
        // Update PBR pipeline's IBL descriptor sets if needed
        if (renderMode == RenderMode::PBR_IBL && iblSystem->isReady()) {
            // The IBL descriptor sets should already be created and updated by the IBL system
            // Just verify they're ready
            if (iblSystem->getDescriptorSets().empty()) {
                std::cerr << "Warning: IBL descriptor sets are empty!" << std::endl;
                return false;
            }
        }
    }
    
    return success;
}

void VulkanRenderer::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan 3D Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get required extensions
    std::vector<const char*> extensions;
    uint32_t glfwExtCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    
    for (uint32_t i = 0; i < glfwExtCount; i++) {
        extensions.push_back(glfwExtensions[i]);
    }
    
    // Add debug extension when validation layers are enabled
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Add validation layers if enabled
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        
        // Setup debug messenger for instance creation/destruction
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void VulkanRenderer::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface!");
}

void VulkanRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
}
void VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::set<uint32_t> uniqueQueues = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float priority = 1.0f;
    for (uint32_t queueFamily : uniqueQueues) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueInfo);
    }
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy=VK_TRUE;
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");
    // (Extensions omitted for brevity)



    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanRenderer::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;


    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanRenderer::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainImageFormat;
        viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create image views!");
    }
}

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanRenderer::createPBRPipeline() {
    // Check if shader files exist and are valid
    std::vector<char> vertCode, fragCode;
    
    try {
        vertCode = readFile("shaders/pbr.vert.spv");
        fragCode = readFile("shaders/pbr.frag.spv");
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load PBR shaders: " << e.what() << std::endl;
        std::cerr << "PBR pipeline will not be available. Using standard pipeline instead." << std::endl;
        return; // Exit without creating PBR pipeline
    }
    
    // Check if shader sizes are valid (must be multiple of 4)
    if (vertCode.size() % 4 != 0) {
        std::cerr << "Error: pbr.vert.spv has invalid size (" << vertCode.size() 
                  << " bytes). Must be multiple of 4. Please recompile the shader." << std::endl;
        return;
    }
    
    if (fragCode.size() % 4 != 0) {
        std::cerr << "Error: pbr.frag.spv has invalid size (" << fragCode.size() 
                  << " bytes). Must be multiple of 4. Please recompile the shader." << std::endl;
        return;
    }

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

    // Vertex input state
    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;  // Cull back faces
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // Standard winding
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and stencil testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Create pipeline layout with descriptor set layouts
    // Check how many descriptor sets we actually need
    uint32_t setLayoutCount = 3; // MVP, Material, Light
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
        mvpDescriptorSetLayout,
        materialDescriptorSetLayout,
        lightDescriptorSetLayout
    };
    
    // Only add IBL descriptor set if it's available
    if (iblSystem && iblSystem->isReady() && iblSystem->getDescriptorSetLayout() != VK_NULL_HANDLE) {
        descriptorSetLayouts.push_back(iblSystem->getDescriptorSetLayout());
        setLayoutCount = 4;
    }

    // Add push constant for material properties
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstant);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = setLayoutCount;
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pbrPipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create PBR pipeline layout!" << std::endl;
        vkDestroyShaderModule(device, fragModule, nullptr);
        vkDestroyShaderModule(device, vertModule, nullptr);
        return;
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pbrPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pbrPipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create PBR graphics pipeline!" << std::endl;
        vkDestroyPipelineLayout(device, pbrPipelineLayout, nullptr);
        pbrPipelineLayout = VK_NULL_HANDLE;
    }

    // Clean up shader modules
    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
}


void VulkanRenderer::createGraphicsPipeline() {

    std::array<VkDescriptorSetLayout, 2> setLayouts = {
        mvpDescriptorSetLayout,
        materialDescriptorSetLayout
    };

    
    // Load SPIR-V shader binaries (ensure they are compiled and available)
    auto vertCode = readFile("shaders/VertexShader.vert.spv");
    auto fragCode = readFile("shaders/ComputerShader.frag.spv");

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

    // Vertex input: specify binding and attribute descriptions for our Vertex structure
    auto bindingDesc = Vertex::getBindingDescription();
    auto attrDescs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;  // Cull back faces
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // Standard winding
    // If model appears inside-out, change to:
    // rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
 
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

   VkPipelineColorBlendAttachmentState colorBlendAttachment{};
colorBlendAttachment.colorWriteMask = 
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Add push constant range
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstant);
    

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());                     
    layoutInfo.pSetLayouts = setLayouts.data();        
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("failed to create pipeline layout!");

  
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;  // Near objects occlude far objects
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline!");

    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
}

void VulkanRenderer::createLightDescriptorSetLayout() {
    // Create binding for light uniform buffer
    VkDescriptorSetLayoutBinding lightBinding{};
    lightBinding.binding = 0;
    lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightBinding.descriptorCount = 1;
    lightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Used in fragment shader
    lightBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &lightBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &lightDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create light descriptor set layout!");
    }
}

void VulkanRenderer::createLightUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(LightUniformBuffer);

    lightUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    lightUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    lightUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            lightUniformBuffers[i],
            lightUniformBuffersMemory[i]
        );

        vkMapMemory(device, lightUniformBuffersMemory[i], 0, bufferSize, 0, &lightUniformBuffersMapped[i]);
        
        // Initialize with default values
        LightUniformBuffer lightData{};
        lightData.lightCount = 0;
        lightData.ambientColor = glm::vec4(0.03f, 0.03f, 0.03f, 1.0f);
        
        memcpy(lightUniformBuffersMapped[i], &lightData, sizeof(lightData));
    }
}

void VulkanRenderer::createLightDescriptorSets() {
    // Allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, lightDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    lightDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, lightDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate light descriptor sets!");
    }

    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = lightUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(LightUniformBuffer);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = lightDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}


void VulkanRenderer::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
void VulkanRenderer::createCommandPool() {
    auto indices = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo poolInfo{};
    
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool!");
}



void VulkanRenderer::createCommandBuffers() {
    // Resize the command buffer vector based on swap chain images
    commandBuffers.resize(swapChainFramebuffers.size());
    
    // Create the command buffer allocation info
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    
    // Allocate the command buffers
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    
    // Don't record any commands here!
    // All command recording will happen in drawFrame() for each frame
}

void VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create synchronization objects!");
    }
}
void VulkanRenderer::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanRenderer::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan 3D Engine with Camera", nullptr, nullptr);
    
    // Store the renderer instance for callbacks
    rendererInstance = this;
    
    // Set GLFW callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // Initialize mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
}

void VulkanRenderer::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}
void VulkanRenderer::createPBRIBLTestScene() {
    // Make sure scene is initialized
    if (!scene) {
        scene = std::make_unique<Scene>(this);
    }
    
    // Clear any existing scene
    scene->clearMeshInstances();
    scene->clearLights();
    
    // Set up an HDRI environment for IBL
    scene->setupEnvironment("hdr/test.hdr");
    
    // Add minimal lighting - let IBL do most of the work
    scene->addLight(
        glm::vec3(1.0f, 1.0f, 1.0f),    // Direction (will be normalized)
        glm::vec3(1.0f, 0.95f, 0.9f),   // Slightly warm white color
        0.3f,                            // Low intensity to emphasize reflections
        0.0f,                            // Radius (0 for directional lights)
        1.0f,                            // Falloff (unused for directional)
        true                             // isDirectional = true
    );
    
    // Create sphere mesh data
    MeshData sphereData = modelLoader.CreateSphere(1.0f, 64, 64); // Higher resolution sphere
    
    // Create a single sphere in the center
    Transform transform;
    transform.position = glm::vec3(0.0f, 0.0f, 0.0f);  // Center of screen
    transform.scale = glm::vec3(2.0f);                   // Make it bigger for better visibility
    
    // Create perfect mirror material
    auto material = std::make_shared<Material>();
    
    // Chrome/mirror-like appearance
    material->diffuseColor = glm::vec3(0.95f, 0.95f, 0.95f); // Near white for metal
    
    // PERFECT REFLECTION SETTINGS
    material->setPBRProperties(
        1.0f,  // metallic = 1.0 (fully metallic)
        0.0f   // roughness = 0.0 (perfect mirror)
    );
    
    material->alpha = 1.0f;
    material->emissiveStrength = 0.0f;
    
    // Explicitly set textures to nullptr to use scalar values
    material->setTexture(TextureType::Diffuse, nullptr);
    material->setTexture(TextureType::Normal, nullptr);
    material->setTexture(TextureType::MetallicRoughness, nullptr);
    material->setTexture(TextureType::Emissive, nullptr);
    material->setTexture(TextureType::AmbientOcclusion, nullptr);
    
    // Create descriptor set for this material
    VkDescriptorSet materialDescriptorSet = createMaterialDescriptorSet(*material);
    if (materialDescriptorSet == VK_NULL_HANDLE) {
        std::cerr << "Failed to create material descriptor set for sphere" << std::endl;
        return;
    }
    material->setDescriptorSet(materialDescriptorSet);
    
    // Create mesh with this material
    std::vector<MeshData> meshData = { sphereData };
    scene->createMeshesFromData(meshData, transform, material);
    
    std::cout << "Created single sphere with:" << std::endl;
    std::cout << "  Position: (0, 0, 0)" << std::endl;
    std::cout << "  Scale: 2.0" << std::endl;
    std::cout << "  Metallic: 1.0 (fully metallic)" << std::endl;
    std::cout << "  Roughness: 0.0 (perfect mirror)" << std::endl;
    std::cout << "  This should show perfect reflections of the environment!" << std::endl;
    
    // Optional: Add a floor for reference
    Transform floorTransform;
    floorTransform.position = glm::vec3(0.0f, -3.0f, 0.0f);
    floorTransform.scale = glm::vec3(10.0f, 1.0f, 10.0f);
    
    MeshData floorData = modelLoader.CreatePlane(1.0f, 1.0f);
    
    auto floorMaterial = std::make_shared<Material>();
    floorMaterial->diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
    floorMaterial->setPBRProperties(0.0f, 0.8f); // Non-metallic, slightly rough
    floorMaterial->alpha = 1.0f;
    
    floorMaterial->setTexture(TextureType::Diffuse, nullptr);
    floorMaterial->setTexture(TextureType::Normal, nullptr);
    floorMaterial->setTexture(TextureType::MetallicRoughness, nullptr);
    floorMaterial->setTexture(TextureType::Emissive, nullptr);
    floorMaterial->setTexture(TextureType::AmbientOcclusion, nullptr);
    
    VkDescriptorSet floorDescriptorSet = createMaterialDescriptorSet(*floorMaterial);
    floorMaterial->setDescriptorSet(floorDescriptorSet);
    
    std::vector<MeshData> floorMeshData = { floorData };
    scene->createMeshesFromData(floorMeshData, floorTransform, floorMaterial);
    
    // Set up camera - positioned to look at the sphere
    cameraPos = glm::vec3(4.0f, 2.0f, 4.0f);   // Positioned at an angle
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // Looking at the sphere
    cameraUp = glm::vec3(0.0f, -1.0f, 0.0f);
    
    // Set rendering mode
    renderMode = RenderMode::PBR_IBL;
    std::cout << "Set render mode to PBR_IBL" << std::endl;
    std::cout << "\n=== REFLECTION TEST ===" << std::endl;
    std::cout << "You should see the environment perfectly reflected in the sphere." << std::endl;
    std::cout << "If reflections are blurry, the issue is with IBL texture resolution." << std::endl;
    std::cout << "========================\n" << std::endl;
}
void VulkanRenderer::createPBRTestScene() {
    // Make sure scene is initialized
    if (!scene) {
        scene = std::make_unique<Scene>(this);
    }
    
    // Clear any existing scene
    scene->clearMeshInstances();
    scene->clearLights();
    
    // Set up default lighting
    scene->setupDefaultLighting();
    
    // Add additional light to better show off the materials
    scene->addLight(
        glm::vec3(-4.0f, 3.0f, -2.0f),  // Position
        glm::vec3(0.9f, 0.8f, 0.7f),    // Warm light color
        3.0f,                           // Intensity
        15.0f,                          // Radius
        1.5f,                           // Falloff
        false                           // Point light
    );
    
    // Create sphere mesh data
    MeshData sphereData = modelLoader.CreateSphere(1.0f, 32, 32);
    
    // Place spheres in a line for easier comparison
    const float SPACING = 3.0f;
    const int NUM_SPHERES = 5;
    
    // Different material configurations to showcase PBR
    std::vector<glm::vec3> colors = {
        glm::vec3(0.95f, 0.95f, 0.95f),  // Almost white
        glm::vec3(0.95f, 0.2f, 0.2f),    // Red
        glm::vec3(0.2f, 0.95f, 0.2f),    // Green
        glm::vec3(0.3f, 0.3f, 0.95f),    // Blue
        glm::vec3(0.95f, 0.84f, 0.1f)    // Gold-like
    };
    
    std::vector<float> metallicValues = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    std::vector<float> roughnessValues = {0.1f, 0.3f, 0.6f, 0.9f, 0.2f};
    
    for (int i = 0; i < NUM_SPHERES; i++) {
        float posX = -((NUM_SPHERES-1) * SPACING) / 2.0f + i * SPACING;
        
        // Create transform
        Transform transform;
        transform.position = glm::vec3(posX, 1.0f, 0.0f);
        transform.scale = glm::vec3(1.0f);
        
        // Create material with different properties
        auto material = std::make_shared<Material>();
        material->diffuseColor = colors[i];
        material->setPBRProperties(metallicValues[i], roughnessValues[i]);
        
        std::cout << "Creating sphere " << i << " at " << posX
                  << " with color " << material->diffuseColor.r 
                  << "," << material->diffuseColor.g 
                  << "," << material->diffuseColor.b 
                  << " metallic: " << metallicValues[i]
                  << " roughness: " << roughnessValues[i] << std::endl;
        
        // Skip using any textures for now
        material->setTexture(TextureType::Diffuse, nullptr);
        material->setTexture(TextureType::MetallicRoughness, nullptr);
        
        // Important: Create descriptor set for this material
        VkDescriptorSet materialDescriptorSet = createMaterialDescriptorSet(*material);
        material->setDescriptorSet(materialDescriptorSet);
        
        std::vector<MeshData> singleSphereMesh = { sphereData };
        
        // Call createMeshesFromData with this single sphere and its material
        scene->createMeshesFromData(singleSphereMesh, transform, material);
    }
    
    // Set up camera
    cameraPos = glm::vec3(0.0f, 2.0f, 8.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
  
    // Enable PBR rendering
    renderMode = RenderMode::PBR;
    std::cout << "Set render mode to PBR" << std::endl;
}

PushConstant VulkanRenderer::createPushConstant(const glm::mat4& model, const Material& material) {
    PushConstant pushConstant{};
    
    // Set model matrix
    pushConstant.model = model;
    
    // Set base color (RGB) and alpha
    pushConstant.baseColorFactor = glm::vec4(material.diffuseColor, material.alpha);
    
    // Set PBR properties
    pushConstant.metallicFactor = material.metallic;
    pushConstant.roughnessFactor = material.roughness;
    pushConstant.ambientOcclusion = 1.0f; // Default to full AO if no texture
    pushConstant.emissiveFactor = material.emissiveStrength;
    
    // Set texture flags
    pushConstant.hasAlbedoMap = material.hasTexture(TextureType::Diffuse) ? 1 : 0;
    pushConstant.hasNormalMap = material.hasTexture(TextureType::Normal) ? 1 : 0;
    
    // Handle metallic/roughness textures
    pushConstant.hasMetallicRoughnessMap = material.hasTexture(TextureType::MetallicRoughness) ? 1 : 
                                         (material.hasTexture(TextureType::Metallic) && 
                                          material.hasTexture(TextureType::Roughness)) ? 1 : 0;
    
    pushConstant.hasEmissiveMap = material.hasTexture(TextureType::Emissive) ? 1 : 0;
    pushConstant.hasOcclusionMap = material.hasTexture(TextureType::AmbientOcclusion) ? 1 : 0;
    
    return pushConstant;
}

#include <chrono> // Make sure this is included for timing
#include <glm/gtc/matrix_transform.hpp> // For glm::lookAt, glm::perspective
#include <iostream> // For error/warning messages
#include <array>    // For clear values

// Ensure PushConstant and SkyboxPushConstant structs are defined in VulkanRenderer.h
// Ensure skyboxMesh, skyboxPipeline, skyboxPipelineLayout, skyboxDescriptorSetLayout
// are members of VulkanRenderer class and initialized.
// Ensure iblSystem is a member and initialized.

#include <chrono> // Make sure this is included for timing
#include <glm/gtc/matrix_transform.hpp> // For glm::lookAt, glm::perspective
#include <iostream> // For error/warning messages
#include <array>    // For clear values

// Ensure PushConstant and SkyboxPushConstant structs are defined in VulkanRenderer.h
// Ensure relevant handles (pipelines, layouts, sets, mesh) are members and initialized.

void VulkanRenderer::drawFrame() {
    // 1. Wait for the previous frame's fence
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // 2. Acquire the next available image index from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                           imageAvailableSemaphores[currentFrame],
                                           VK_NULL_HANDLE,
                                           &imageIndex);

    // Handle swap chain becoming out of date
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "Swap chain out of date during acquire, recreating..." << std::endl;
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // 3. Calculate delta time for smooth camera movement
    float currentFrameTime = static_cast<float>(glfwGetTime());
    deltaTime = currentFrameTime - lastFrame;
    lastFrame = currentFrameTime;
    
    // 4. Update camera based on keyboard/mouse input
    updateCamera(deltaTime);

    // 5. Update scene logic (animations, physics, etc.)
    if (scene) {
        scene->update(deltaTime);
    }

    // 6. Check if the acquired image is still being used by a previous frame
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // 7. Reset the fence for the current frame
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // 8. Reset and begin recording the command buffer
    vkResetCommandBuffer(commandBuffers[imageIndex], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // 9. Begin the Render Pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // --- Command Recording within Render Pass ---

    // 10. Calculate View and Projection matrices from the camera
    glm::mat4 view, proj;
    
    if (camera) {
        // Use camera for view matrix
        view = camera->getViewMatrix();
        
        // Use camera for projection matrix with current FOV
        float aspectRatio = swapChainExtent.width / static_cast<float>(swapChainExtent.height);
        proj = camera->getProjectionMatrix(aspectRatio, nearPlane, farPlane);
        
        // Update camera position for shaders (lighting calculations)
        cameraPos = camera->getPosition();
        
        // Update FOV from camera
        fov = camera->getFOV();
    } else {
        // Fallback if camera is not initialized
        view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        proj = glm::perspective(glm::radians(fov),
                              swapChainExtent.width / (float)swapChainExtent.height,
                              nearPlane, farPlane);
    }
    
    // Flip Y axis for Vulkan's clip space convention
    proj[1][1] *= -1;

    // 11. Update the main Uniform Buffer Object
    updateViewProjection(view, proj);

    // Define viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    // --- Skybox Rendering ---
    bool canRenderSkybox = iblSystem && iblSystem->isReady() &&
                           skyboxMesh &&
                           skyboxPipeline != VK_NULL_HANDLE &&
                           skyboxPipelineLayout != VK_NULL_HANDLE &&
                           !skyboxDescriptorSets.empty();

    if (canRenderSkybox) {
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

        // Update skybox push constant (remove translation from view matrix)
        SkyboxPushConstant skyboxPushConstant;
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // Remove translation
        skyboxPushConstant.model = glm::mat4(1.0f);
        skyboxPushConstant.view = skyboxView;
        skyboxPushConstant.proj = proj;

        vkCmdPushConstants(
            commandBuffers[imageIndex],
            skyboxPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(SkyboxPushConstant),
            &skyboxPushConstant
        );

        vkCmdBindDescriptorSets(
            commandBuffers[imageIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            skyboxPipelineLayout,
            0,
            1,
            &skyboxDescriptorSets[currentFrame],
            0, nullptr
        );

        skyboxMesh->bind(commandBuffers[imageIndex]);
        vkCmdDraw(commandBuffers[imageIndex], 36, 1, 0, 0);
    }

    // --- Main Scene Rendering ---
    bool usePBR = (renderMode == RenderMode::PBR || renderMode == RenderMode::PBR_IBL);
    bool useIBL = (renderMode == RenderMode::PBR_IBL);

    VkPipeline pipelineToBind = usePBR ? pbrPipeline : graphicsPipeline;
    VkPipelineLayout layoutToUse = usePBR ? pbrPipelineLayout : pipelineLayout;

    if (pipelineToBind != VK_NULL_HANDLE && layoutToUse != VK_NULL_HANDLE) {
        // Bind the appropriate pipeline
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineToBind);

        // Set dynamic viewport and scissor
        vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
        vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

        // Bind MVP descriptor set (set 0)
        vkCmdBindDescriptorSets(
            commandBuffers[imageIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            layoutToUse,
            0,
            1,
            &mvpDescriptorSets[currentFrame],
            0, nullptr
        );

        // Bind light descriptor set (set 2) for PBR
        if (usePBR) {
            updateLights();
            vkCmdBindDescriptorSets(
                commandBuffers[imageIndex],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pbrPipelineLayout,
                2,
                1,
                &lightDescriptorSets[currentFrame],
                0, nullptr
            );
        }

        // Bind IBL descriptor set (set 3) for PBR_IBL
        if (useIBL && iblSystem && iblSystem->isReady() && !iblSystem->getDescriptorSets().empty()) {
            vkCmdBindDescriptorSets(
                commandBuffers[imageIndex],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pbrPipelineLayout,
                3,
                1,
                &iblSystem->getDescriptorSets()[currentFrame],
                0, nullptr
            );
        }

        // Draw the scene
        if (scene) {
            scene->draw(commandBuffers[imageIndex], view, proj, currentFrame);
        }
    }

    if (debugUI) {
        debugUI->beginFrame();
        
        // Update performance metrics
        if (auto perfPanel = debugUI->getPanel<PerformancePanel>("Performance")) {
            perfPanel->updateFrameTime(deltaTime);
        }
        
        debugUI->endFrame(commandBuffers[imageIndex]);
    }

    // 12. End the Render Pass
    vkCmdEndRenderPass(commandBuffers[imageIndex]);

    // 13. Finish recording the command buffer
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }

    // 14. Submit the command buffer to the graphics queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // 15. Present the rendered image to the screen
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        std::cout << "Swap chain out of date/suboptimal after present, recreating..." << std::endl;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    // 16. Advance to the next frame
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
// Replace the material descriptor set layout creation in VulkanRenderer.cpp
// with this enhanced version that supports all PBR textures

void VulkanRenderer::createDescriptorSetLayouts() {
    // MVP descriptor set layout (set = 0)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo mvpLayoutInfo{};
    mvpLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    mvpLayoutInfo.bindingCount = 1;
    mvpLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &mvpLayoutInfo, nullptr, &mvpDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create MVP descriptor set layout!");
    }

    // Material descriptor set layout (set = 1)
    // Create bindings for all PBR material textures
    std::array<VkDescriptorSetLayoutBinding, 5> materialBindings{};
    
    // Binding 0: Albedo/Base Color texture
    materialBindings[0].binding = 0;
    materialBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialBindings[0].descriptorCount = 1;
    materialBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings[0].pImmutableSamplers = nullptr;
    
    // Binding 1: Normal map
    materialBindings[1].binding = 1;
    materialBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialBindings[1].descriptorCount = 1;
    materialBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings[1].pImmutableSamplers = nullptr;
    
    // Binding 2: Metallic-roughness map
    materialBindings[2].binding = 2;
    materialBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialBindings[2].descriptorCount = 1;
    materialBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings[2].pImmutableSamplers = nullptr;
    
    // Binding 3: Emissive map
    materialBindings[3].binding = 3;
    materialBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialBindings[3].descriptorCount = 1;
    materialBindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings[3].pImmutableSamplers = nullptr;
    
    // Binding 4: Occlusion map
    materialBindings[4].binding = 4;
    materialBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialBindings[4].descriptorCount = 1;
    materialBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings[4].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount = static_cast<uint32_t>(materialBindings.size());
    materialLayoutInfo.pBindings = materialBindings.data();

    if (vkCreateDescriptorSetLayout(device, &materialLayoutInfo, nullptr, &materialDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create material descriptor set layout!");
    }
}



void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i],
            uniformBuffersMemory[i]
        );

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void VulkanRenderer::createDescriptorPool() {
    // ***** CHANGE ARRAY SIZE FROM 5 TO 4 *****
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    // *****************************************

    // MVP Uniform buffer pool size & Light Uniform buffer pool size
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2); // MVP UBO + Light UBO per frame

    // Material Texture sampler pool sizes
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 5 * 20); // Example: Max 20 materials * 5 textures per material * frames (Adjust as needed)

    // IBL Texture sampler pool sizes
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 4); // 4 IBL textures * frames

    // Skybox Texture sampler pool size
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // Allow allocating one per frame temporarily

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    // ***** CHANGE COUNT FROM poolSizes.size() TO 4 (or use the array size directly) *****
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size()); // Or explicitly 4
    // *************************************************************************************
    poolInfo.pPoolSizes = poolSizes.data();

    // Adjust maxSets calculation if needed, ensure it's sufficient
    uint32_t maxMaterialSets = 100; // Example: Max 100 unique materials
    uint32_t maxTempSetsPerFrame = 1; // For the skybox set allocated/freed each frame
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 3) // MVP, Lights, IBL per frame
                      + maxMaterialSets
                      + (MAX_FRAMES_IN_FLIGHT * maxTempSetsPerFrame); // Enough temp sets buffered across frames if needed


    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void VulkanRenderer::createDescriptorSets() {
    // Allocate MVP descriptor sets (one per frame in flight)
    std::vector<VkDescriptorSetLayout> mvpLayouts(MAX_FRAMES_IN_FLIGHT, mvpDescriptorSetLayout);
    VkDescriptorSetAllocateInfo mvpAllocInfo{};
    mvpAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    mvpAllocInfo.descriptorPool = descriptorPool;
    mvpAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    mvpAllocInfo.pSetLayouts = mvpLayouts.data();

    mvpDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &mvpAllocInfo, mvpDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate MVP descriptor sets!");
    }

    // IMPORTANT: Update the MVP descriptor sets right after allocation
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = mvpDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
    
    std::vector<VkDescriptorSetLayout> materialLayouts(MAX_FRAMES_IN_FLIGHT, materialDescriptorSetLayout);//TODO: harded coded to 1 but need to be changed to be dynamic
    VkDescriptorSetAllocateInfo materialAllocInfo{};
    materialAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    materialAllocInfo.descriptorPool = descriptorPool;
    materialAllocInfo.descriptorSetCount = static_cast<uint32_t>(1);
    materialAllocInfo.pSetLayouts = materialLayouts.data();

    materialDescriptorSets.resize(1);
    if (vkAllocateDescriptorSets(device, &materialAllocInfo, materialDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate material descriptor sets!");
    }

    if (iblSystem && iblSystem->isReady() && skyboxDescriptorSetLayout != VK_NULL_HANDLE) { // Check if IBL is ready
    std::vector<VkDescriptorSetLayout> skyboxLayouts(MAX_FRAMES_IN_FLIGHT, skyboxDescriptorSetLayout);
    VkDescriptorSetAllocateInfo skyboxAllocInfo{};
    skyboxAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    skyboxAllocInfo.descriptorPool = descriptorPool;
    skyboxAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    skyboxAllocInfo.pSetLayouts = skyboxLayouts.data();

    skyboxDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &skyboxAllocInfo, skyboxDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate persistent skybox descriptor sets!");
    }

    // Update the sets once during initialization
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo skyboxImageInfo{};
        skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // Ensure getEnvironmentMap() returns a valid texture
        if (iblSystem->getEnvironmentMap()) {
            skyboxImageInfo.imageView = iblSystem->getEnvironmentMap()->getImageView();
            skyboxImageInfo.sampler = iblSystem->getEnvironmentMap()->getSampler();
        } else {
            // Handle error: IBL system ready but no environment map? Fallback?
             std::cerr << "Error: IBL System ready but environment map is null during skybox descriptor update." << std::endl;
             // Use a default texture maybe? For now, just log.
             continue; // Skip update for this frame's set if map is missing
        }


        VkWriteDescriptorSet skyboxWrite{};
        skyboxWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyboxWrite.dstSet = skyboxDescriptorSets[i];
        skyboxWrite.dstBinding = 0; // Matches binding in skybox.frag
        skyboxWrite.dstArrayElement = 0;
        skyboxWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxWrite.descriptorCount = 1;
        skyboxWrite.pImageInfo = &skyboxImageInfo;

        vkUpdateDescriptorSets(device, 1, &skyboxWrite, 0, nullptr);
    }
     std::cout << "Persistent skybox descriptor sets created and updated." << std::endl;
} else {
     std::cerr << "Warning: Skipping skybox descriptor set creation because IBL system is not ready or layout is null." << std::endl;
}

    
}


void VulkanRenderer::updateMVPMatrices(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj) {
    UniformBufferObject ubo{};
    ubo.model = model;
    ubo.view = view;
    ubo.proj = proj;
    ubo.proj[1][1] *= -1; // Flip Y coordinate for Vulkan
    
    // Update uniform buffer for current frame
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

// Modify the updateMVPMatrices function to only update view and projection
void VulkanRenderer::updateViewProjection(const glm::mat4& view, const glm::mat4& proj) {
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f); // Identity matrix, not used
    ubo.view = view;
    ubo.proj = proj;
    
    ubo.cameraPos = cameraPos;
    ubo.time = static_cast<float>(glfwGetTime());
    
    // Update uniform buffer for current frame
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void VulkanRenderer::cleanupSwapChain() {
    // Destroy depth resources
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    // Destroy framebuffers
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    // Free command buffers
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    // Destroy pipelines and layouts
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    
    // Also destroy PBR pipeline if it exists
    if (pbrPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pbrPipeline, nullptr);
        pbrPipeline = VK_NULL_HANDLE; // Reset to null after destruction
    }
    if (pbrPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pbrPipelineLayout, nullptr);
        pbrPipelineLayout = VK_NULL_HANDLE; // Reset to null after destruction
    }
    
    // Also destroy skybox pipeline if it exists (during swap chain recreation)
    if (skyboxPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, skyboxPipeline, nullptr);
        skyboxPipeline = VK_NULL_HANDLE; // Reset to null after destruction
    }
    if (skyboxPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
        skyboxPipelineLayout = VK_NULL_HANDLE; // Reset to null after destruction
    }
    // Note: Don't destroy skyboxDescriptorSetLayout here as it's reused

    // Destroy render pass
    vkDestroyRenderPass(device, renderPass, nullptr);

    // Destroy swap chain image views
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // Destroy swap chain
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanRenderer::recreateSwapChain() {
    // Handle minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Wait for device to finish current operations
    vkDeviceWaitIdle(device);
    
    // IMPORTANT: Clean up ImGui before destroying render pass
    if (debugUI) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        // Don't destroy context, we'll reinit
    }

    // Cleanup old swap chain and dependent resources
    cleanupSwapChain();

    // Recreate swap chain and dependent resources
    createSwapChain();
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    createImageViews();
    createRenderPass();
    
    // Reinitialize ImGui with new render pass
    if (debugUI) {
        ImGui_ImplGlfw_InitForVulkan(window, true);
        
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = findQueueFamilies(physicalDevice).graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = debugUI->getDescriptorPool();
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = static_cast<uint32_t>(swapChainImages.size());
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        init_info.RenderPass = renderPass;  // Critical!
        
        ImGui_ImplVulkan_Init(&init_info);
        
        // Rebuild fonts
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Build();
    }
    
    // Recreate ALL pipelines
    createGraphicsPipeline();
    createPBRPipeline();
    if (iblSystem && iblSystem->isReady()) {
        createSkyboxPipeline();
    }
    
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
    
    // Reset the imagesInFlight array
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        imagesInFlight[i] = VK_NULL_HANDLE;
    }
    
    std::cout << "Swap chain recreation completed successfully" << std::endl;
}


VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void VulkanRenderer::initializeDebugUI() {
    // Create the debug UI manager
    debugUI = std::make_unique<DebugUIManager>(this);
    
    // Initialize ImGui with render pass
    debugUI->initialize(
        window,
        instance,
        physicalDevice,
        device,
        findQueueFamilies(physicalDevice).graphicsFamily.value(),
        graphicsQueue,
        renderPass,
        static_cast<uint32_t>(swapChainImages.size())
    );
    
    // In newer ImGui versions, fonts are created automatically on first use
    // Or you can manually build them like this:
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
    
    // Create and add debug panels
    auto cameraPanel = std::make_shared<CameraDebugPanel>(this);
    auto renderPanel = std::make_shared<RenderDebugPanel>(this);
    auto perfPanel = std::make_shared<PerformancePanel>(this);
    auto scenePanel = std::make_shared<SceneHierarchyPanel>(this);
    auto settingsPanel = std::make_shared<SettingsPanel>(this);
    
    debugUI->addPanel(cameraPanel);
    debugUI->addPanel(renderPanel);
    debugUI->addPanel(perfPanel);
    debugUI->addPanel(scenePanel);
    debugUI->addPanel(settingsPanel);
    
    // Start with only camera and performance panels open
    scenePanel->setOpen(false);
    settingsPanel->setOpen(false);
    
    std::cout << "Debug UI system initialized with panels" << std::endl;
}

VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                           VkImageTiling tiling,
                                           VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}


void VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format,
                               VkImageTiling tiling, VkImageUsageFlags usage,
                               VkMemoryPropertyFlags properties, VkImage& image,
                               VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkFormat VulkanRenderer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool VulkanRenderer::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height,
               depthFormat,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               depthImage,
               depthImageMemory);

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}
void VulkanRenderer::createDefaultTexture() {
    // Create a 1x1 white texture as default
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    
    defaultTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultTexture->createFromPixels(whitePixel, 1, 1, 4, commandPool, graphicsQueue);
}


void VulkanRenderer::createSkyboxPipeline() {
    // 1. Shader modules
    auto vertCode = readFile("shaders/skybox.vert.spv"); // Make sure this file exists and is compiled
    auto fragCode = readFile("shaders/skybox.frag.spv"); // Make sure this file exists and is compiled

    if (vertCode.empty() || fragCode.empty()) {
        throw std::runtime_error("Failed to load skybox shader(s)!");
    }

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    // 2. Vertex input state - Skybox doesn't use traditional vertex buffers
    // Vertex data is generated directly in the vertex shader using gl_VertexIndex
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // No vertex bindings
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // No vertex attributes

    // 3. Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 4. Viewport and scissor state - Will be set dynamically
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1; // We will set 1 viewport dynamically
    viewportState.scissorCount = 1;  // We will set 1 scissor dynamically
    // pViewports and pScissors are omitted because they are dynamic

    // 5. Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // Cull front faces for skybox (important!)
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Or CLOCKWISE depending on your cube winding order
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // 6. Multisample
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Adjust if using MSAA
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // 7. Depth-stencil testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE; // <<<--- Important: Don't write depth for skybox
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // <<<--- Draw skybox pixel if depth is <= 1.0
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {};  // Optional

    // 8. Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // No blending for skybox
    // Other blend factors are irrelevant if blendEnable is VK_FALSE

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // 9. Dynamic States
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    // 10. Descriptor Set Layout for Skybox (Sampler only)
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0; // Matches layout (set = 0, binding = 0) in skybox.frag
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Sampler used in fragment shader
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    // Destroy previous layout if it exists (important during recreation)
    if (skyboxDescriptorSetLayout != VK_NULL_HANDLE) {
         vkDestroyDescriptorSetLayout(device, skyboxDescriptorSetLayout, nullptr);
    }

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &skyboxDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox descriptor set layout!");
    }

    // 11. Pipeline layout (Uses SkyboxPushConstant and skyboxDescriptorSetLayout)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Push constant used in vertex shader
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyboxPushConstant); // Use the specific struct size

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &skyboxDescriptorSetLayout; // Use the skybox layout
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    // Destroy previous layout if it exists (important during recreation)
     if (skyboxPipelineLayout != VK_NULL_HANDLE) {
         vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
     }

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &skyboxPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox pipeline layout!");
    }

    // 12. Graphics pipeline creation
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState; // References the struct, but pViewports/pScissors are ignored
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo; // Point to the dynamic state info
    pipelineInfo.layout = skyboxPipelineLayout;     // Use the skybox layout
    pipelineInfo.renderPass = renderPass;           // Use the main render pass
    pipelineInfo.subpass = 0;                       // Render in the first subpass
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

     // Destroy previous pipeline if it exists (important during recreation)
     if (skyboxPipeline != VK_NULL_HANDLE) {
         vkDestroyPipeline(device, skyboxPipeline, nullptr);
     }

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyboxPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create skybox graphics pipeline!");
    }

    // 13. Cleanup shader modules
    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);

    std::cout << "Skybox pipeline created successfully." << std::endl;
}

void VulkanRenderer::createDefaultTextures() {
    // Create a 2x2 white texture as default albedo (some GPUs prefer power-of-2 textures)
    unsigned char whitePixels[16] = {
        255, 255, 255, 255,  255, 255, 255, 255,
        255, 255, 255, 255,  255, 255, 255, 255
    };
    defaultAlbedoTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultAlbedoTexture->createFromPixels(whitePixels, 2, 2, 4, commandPool, graphicsQueue);
    
    // Create a 2x2 normal map (pointing straight up in tangent space)
    // Normal maps use RGB where R=x, G=y, B=z in tangent space
    // (0.5, 0.5, 1.0) in [0,1] range = (0, 0, 1) in [-1,1] range = pointing up
    unsigned char normalPixels[16] = {
        128, 128, 255, 255,  128, 128, 255, 255,
        128, 128, 255, 255,  128, 128, 255, 255
    };
    defaultNormalTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultNormalTexture->createFromPixels(normalPixels, 2, 2, 4, commandPool, graphicsQueue);
    
    // Create a 2x2 metallic-roughness map
    // In glTF 2.0 format: R=unused, G=roughness, B=metallic, A=unused
    // Set to non-metallic (0) and medium roughness (128)
    unsigned char mrPixels[16] = {
        0, 128, 0, 255,  0, 128, 0, 255,
        0, 128, 0, 255,  0, 128, 0, 255
    };
    defaultMetallicRoughnessTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultMetallicRoughnessTexture->createFromPixels(mrPixels, 2, 2, 4, commandPool, graphicsQueue);
    
    // Create a 2x2 white texture for occlusion (no occlusion = white)
    defaultOcclusionTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultOcclusionTexture->createFromPixels(whitePixels, 2, 2, 4, commandPool, graphicsQueue);
    
    // Create a 2x2 black texture for emissive (no emission = black)
    unsigned char blackPixels[16] = {
        0, 0, 0, 255,  0, 0, 0, 255,
        0, 0, 0, 255,  0, 0, 0, 255
    };
    defaultEmissiveTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultEmissiveTexture->createFromPixels(blackPixels, 2, 2, 4, commandPool, graphicsQueue);
    
    std::cout << "Default textures created successfully" << std::endl;
}


void VulkanRenderer::createMaterialUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(MaterialUniformBuffer);

    materialUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    materialUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    materialUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            materialUniformBuffers[i],
            materialUniformBuffersMemory[i]
        );

        vkMapMemory(device, materialUniformBuffersMemory[i], 0, bufferSize, 0, &materialUniformBuffersMapped[i]);
        
        // Initialize with default values
        MaterialUniformBuffer defaultMaterial{};
        defaultMaterial.baseColorFactor = glm::vec4(1.0f);
        defaultMaterial.metallicFactor = 0.0f;
        defaultMaterial.roughnessFactor = 0.5f;
        defaultMaterial.aoStrength = 1.0f;
        defaultMaterial.emissiveStrength = 0.0f;
        defaultMaterial.hasBaseColorMap = 0;
        defaultMaterial.hasNormalMap = 0;
        defaultMaterial.hasMetallicRoughnessMap = 0;
        defaultMaterial.hasOcclusionMap = 0;
        defaultMaterial.hasEmissiveMap = 0;
        defaultMaterial.alphaCutoff = 0.5f;
        defaultMaterial.alphaMode = 0; // Opaque
        
        memcpy(materialUniformBuffersMapped[i], &defaultMaterial, sizeof(defaultMaterial));
    }
}







// Update updateLights method in VulkanRenderer.cpp
void VulkanRenderer::updateLights() {
    // Get the lights from the scene
    const auto& lights = scene->getLights();
    
    // Create and update the light uniform buffer
    LightUniformBuffer lightData{};
    
    // Initialize with safe defaults
    lightData.lightCount = 0;
    lightData.ambientColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);  // Increase from 0.1f to 0.3f
    
    // Debug output
   
    
    // Set light count (clamped to MAX_LIGHTS)
    lightData.lightCount = static_cast<int>(std::min(lights.size(), static_cast<size_t>(MAX_LIGHTS)));

   

   
    // Set light data for each light
    for (int i = 0; i < lightData.lightCount; i++) {
        const auto& light = lights[i];
        
        // For directional lights, normalize the direction
        if (light.isDirectional) {
            glm::vec3 normalizedDir = glm::normalize(light.position);
            lightData.lights[i].position = glm::vec4(normalizedDir, 0.0f);
        } else {
            lightData.lights[i].position = glm::vec4(light.position, 1.0f);
        }
        
        lightData.lights[i].color = glm::vec4(light.color, light.intensity);
        lightData.lights[i].radius = light.radius;
        lightData.lights[i].falloff = light.falloff;
        
        
    }
    
    // Update the light uniform buffer
    memcpy(lightUniformBuffersMapped[currentFrame], &lightData, sizeof(lightData));

  
}






// Replace the existing createMaterialDescriptorSet method in VulkanRenderer.cpp
VkDescriptorSet VulkanRenderer::createMaterialDescriptorSet(const Material& material) {
    // Allocate a new descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &materialDescriptorSetLayout;
    
    VkDescriptorSet descriptorSet;
    
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate material descriptor set!");
    }
    
    // Prepare descriptor image info array for all textures
    std::array<VkDescriptorImageInfo, 5> imageInfos{};
    
    // Binding 0: Albedo/Base Color texture
    if (material.hasTexture(TextureType::Diffuse)) {
        imageInfos[0] = material.getTextureImageInfo(TextureType::Diffuse);
    } else {
        imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[0].imageView = defaultAlbedoTexture->getImageView();
        imageInfos[0].sampler = defaultAlbedoTexture->getSampler();
    }
    
    // Binding 1: Normal map
    if (material.hasTexture(TextureType::Normal)) {
        imageInfos[1] = material.getTextureImageInfo(TextureType::Normal);
    } else {
        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[1].imageView = defaultNormalTexture->getImageView();
        imageInfos[1].sampler = defaultNormalTexture->getSampler();
    }
    
    // Binding 2: Metallic-Roughness map
    if (material.hasTexture(TextureType::MetallicRoughness)) {
        imageInfos[2] = material.getTextureImageInfo(TextureType::MetallicRoughness);
    } else if (material.hasTexture(TextureType::Metallic) && material.hasTexture(TextureType::Roughness)) {
        // If we have both maps separately, we would ideally combine them
        // For now, just use metallic and scalar for roughness
        imageInfos[2] = material.getTextureImageInfo(TextureType::Metallic);
    } else {
        imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[2].imageView = defaultMetallicRoughnessTexture->getImageView();
        imageInfos[2].sampler = defaultMetallicRoughnessTexture->getSampler();
    }
    
    // Binding 3: Emissive map
    if (material.hasTexture(TextureType::Emissive)) {
        imageInfos[3] = material.getTextureImageInfo(TextureType::Emissive);
    } else {
        imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[3].imageView = defaultEmissiveTexture->getImageView();
        imageInfos[3].sampler = defaultEmissiveTexture->getSampler();
    }
    
    // Binding 4: Occlusion map
    if (material.hasTexture(TextureType::AmbientOcclusion)) {
        imageInfos[4] = material.getTextureImageInfo(TextureType::AmbientOcclusion);
    } else {
        imageInfos[4].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[4].imageView = defaultOcclusionTexture->getImageView();
        imageInfos[4].sampler = defaultOcclusionTexture->getSampler();
    }
    
    // Create write descriptor set array for all textures
    std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
    
    // Write descriptor for Albedo/Base Color texture
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfos[0];
    
    // Write descriptor for Normal map
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfos[1];
    
    // Write descriptor for Metallic-Roughness map
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &imageInfos[2];
    
    // Write descriptor for Emissive map
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = descriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pImageInfo = &imageInfos[3];
    
    // Write descriptor for Occlusion map
    descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[4].dstSet = descriptorSet;
    descriptorWrites[4].dstBinding = 4;
    descriptorWrites[4].dstArrayElement = 0;
    descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[4].descriptorCount = 1;
    descriptorWrites[4].pImageInfo = &imageInfos[4];
    
    // Update all descriptor sets at once
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    
    return descriptorSet;
}





VkPipelineLayout VulkanRenderer::getPipelineLayout()
{
    return pipelineLayout;
}



// Enhanced fragment shader to support IBL








// Update draw function to bind IBL descriptors


// Clean up IBL resources


void VulkanRenderer::cleanup() {
    // Wait for the device to finish operations before cleaning up
    vkDeviceWaitIdle(device);
    
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    
    // First cleanup the swap chain (this handles framebuffers, pipelines, etc.)
    cleanupSwapChain();

    // Cleanup uniform buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Cleanup material uniform buffers
        vkDestroyBuffer(device, materialUniformBuffers[i], nullptr);
        vkFreeMemory(device, materialUniformBuffersMemory[i], nullptr);
        
        // Cleanup light uniform buffers
        vkDestroyBuffer(device, lightUniformBuffers[i], nullptr);
        vkFreeMemory(device, lightUniformBuffersMemory[i], nullptr);
        
        // Cleanup MVP uniform buffers
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    // IMPORTANT: Cleanup skybox mesh BEFORE destroying the device
    // This will properly destroy the vertex and index buffers and their memory
    if (skyboxMesh) {
        skyboxMesh.reset(); // This will call the Mesh destructor which cleans up buffers
    }
    
    // Cleanup skybox pipeline and layout if they exist
    if (skyboxPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, skyboxPipeline, nullptr);
        skyboxPipeline = VK_NULL_HANDLE;
    }
    
    if (skyboxPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
        skyboxPipelineLayout = VK_NULL_HANDLE;
    }
    
    if (skyboxDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, skyboxDescriptorSetLayout, nullptr);
        skyboxDescriptorSetLayout = VK_NULL_HANDLE;
    }

    // Cleanup default textures
    defaultTexture.reset();
    defaultAlbedoTexture.reset();
    defaultNormalTexture.reset();
    defaultMetallicRoughnessTexture.reset();
    defaultOcclusionTexture.reset();
    defaultEmissiveTexture.reset();

    // Cleanup scene (this will clean up all meshes and textures)
    // IMPORTANT: Do this BEFORE destroying the descriptor pool
    scene.reset();
    
    // Cleanup IBL system
    iblSystem.reset();

    // Cleanup descriptor pool and layouts
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, materialDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, mvpDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, lightDescriptorSetLayout, nullptr);

    // Cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    
    // Cleanup command pool
    vkDestroyCommandPool(device, commandPool, nullptr);

    // Cleanup device
    vkDestroyDevice(device, nullptr);

    // Cleanup surface
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // Cleanup instance
    vkDestroyInstance(instance, nullptr);

    if (debugUI) {
        debugUI->cleanup();
        debugUI.reset();
    }

    // Cleanup window
    glfwDestroyWindow(window);
    glfwTerminate();
}


bool VulkanRenderer::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

// Debug callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    // Return false to indicate the error should not be aborted
    return VK_FALSE;
}

VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}


