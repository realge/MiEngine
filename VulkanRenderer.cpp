#include "VulkanRenderer.h"



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
    createDescriptorSetLayouts();
    std::cout << "Descriptor set layouts created" << std::endl;
    createGraphicsPipeline();
    std::cout << "Graphics pipeline created" << std::endl;
    //createPBRPipeline();
    createDepthResources();
    std::cout << "Depth resources created" << std::endl;
    createFramebuffers();
    std::cout << "Framebuffers created" << std::endl;
    createCommandPool();
    std::cout << "Command pool created" << std::endl;
    
    // Create default textures first
    createDefaultTextures();
    std::cout << "Default textures created" << std::endl;
    //createLightUniformBuffers();
    // Create uniform buffers
    createUniformBuffers();
    std::cout << "Uniform buffers created" << std::endl;
    createMaterialUniformBuffers(); // Add this line
    std::cout << "Material uniform buffers created" << std::endl;
    createDescriptorPool();
    std::cout << "Descriptor pool created" << std::endl;
    createDescriptorSets();
    std::cout << "Descriptor sets created" << std::endl;
    createCommandBuffers();
    std::cout << "Command buffers created" << std::endl;
    createSyncObjects();
    std::cout << "Sync objects created" << std::endl;
    // Initialize scene
    scene = std::make_unique<Scene>(this);

    // Load initial models
    Transform modelTransform;
    modelTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    //modelTransform.rotation = glm::vec3(0.0f, 0.0f, 90.0f);
    modelTransform.scale = glm::vec3(19.0f);


    Transform modelTransform2;
    modelTransform2.position = glm::vec3(0.0f, 0.0f, 0.0f);
    //modelTransform.rotation = glm::vec3(0.0f, 0.0f, 90.0f);
    modelTransform2.scale = glm::vec3(12.0f);

    MaterialTexturePaths texturePaths;
    texturePaths.diffuse = "texture/blackrat_color.png";  // Albedo/color texture
    texturePaths.normal = "texture/blackrat_normal.png";  // Normal map if available
    MaterialTexturePaths texturePaths1;
    //scene->loadModel("models/blackrat.fbx", modelTransform);
    scene->loadTexturedModel("models/blackrat.fbx", "texture/blackrat_color.png", modelTransform);
    //scene->loadTexturedModel("models/test_model.fbx", "texture/blackrat_color.png", modelTransform);
    //scene->loadTexturedModel("models/animal.fbx", "", modelTransform);
    //scene->loadTexturedModel("models/eyeball.fbx", "", modelTransform2);
    // scene->loadModel("models/test_model.fbx", modelTransform);
    scene->loadTexturedModel("models/house.fbx", "texture/house.png", modelTransform2);
    //scene->loadTexturedModel("models/house.fbx", "texture/house.png", modelTransform);

    //scene->loadTexturedModelPBR("models/blackrat.fbx", texturePaths, modelTransform);
    //scene->loadTexturedModelPBR("models/eye.fbx", texturePaths1, modelTransform);

    //testPBRRendering();
    // or
    // createPBRTestGrid();
    
    // Make sure PBR pipeline is created if not done elsewhere
    
    
    // Set the render mode to PBR
    renderMode = RenderMode::Standard;
    
    // Set up camera
    cameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    fov = 90.0f;
    nearPlane = 0.1f;
    farPlane = 10.0f;
  
   
    
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
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;;//VK_POLYGON_MODE_FILL
    rasterizer.lineWidth = 1.f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; //TODO: change to VK_CULL_MODE_BACK_BIT
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//VK_FRONT_FACE_CLOCKWISE
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
    pushConstantRange.size = sizeof(ModelPushConstant);
    

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
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

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
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan 2D Square", nullptr, nullptr);
}

void VulkanRenderer::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}
void VulkanRenderer::drawFrame() {
    // Wait for the previous frame to complete
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    // Acquire the next image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "out of date" << std::endl;
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Calculate delta time for scene update
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>
        (currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;
   
    

    // Update scene
    if (scene) {
        scene->update(deltaTime);
    }

    // Check if a previous frame is using this image (i.e., there is its fence to wait on)
    if (imagesInFlight.size() > 0 && imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    
    // Reset the fence only if we are submitting work
    vkResetFences(device, 1, &inFlightFences[currentFrame]);


    
    
    // Reset and begin command buffer
    vkResetCommandBuffer(commandBuffers[imageIndex], 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    vkDeviceWaitIdle(device);
    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Begin render pass
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

    // Record commands
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Calculate view and projection matrices
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 proj = glm::perspective(glm::radians(fov),
        swapChainExtent.width / (float)swapChainExtent.height,
        nearPlane, farPlane);
    
    // Important: Update uniform buffers for the CURRENT frame index
    // This ensures we're updating the uniform buffer that's properly synchronized
    //updateMVPMatrices(glm::mat4(1.0f), view, proj);
   


    
    // Choose rendering path based on render mode
    if (renderMode == RenderMode::PBR || renderMode == RenderMode::PBR_IBL) {
        // Update lights for PBR
        updateLights();
        
        // Use PBR rendering pipeline
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline);
        
        // Important: Bind the descriptor set for the CURRENT frame
        vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
            pbrPipelineLayout, 0, 1, &mvpDescriptorSets[currentFrame], 0, nullptr);
        
        // Draw scene with PBR pipeline
        if (scene) {
            scene->draw(commandBuffers[imageIndex], view, proj, currentFrame);
        }
        
        // If IBL is enabled and IBL descriptor sets are available, bind them
        if (renderMode == RenderMode::PBR_IBL && iblDescriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS,
                pbrPipelineLayout, 1, 1, &iblDescriptorSet, 0, nullptr);
        }
    } else {
        // Use standard pipeline
        vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        
        // Draw scene with standard pipeline
        if (scene) {
            scene->draw(commandBuffers[imageIndex], view, proj, currentFrame);
        }
    }

    vkCmdEndRenderPass(commandBuffers[imageIndex]);

    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    // Submit command buffer
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
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present the frame
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
void VulkanRenderer::createDescriptorSetLayouts() {
    // MVP descriptor set layout (set = 0)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo mvpLayoutInfo{};
    mvpLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    mvpLayoutInfo.bindingCount = 1;
    mvpLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &mvpLayoutInfo, nullptr, &mvpDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create MVP descriptor set layout!");
    }

    // Material descriptor set layout (set = 1)
    // Create multiple bindings for the material descriptor set
    std::array<VkDescriptorSetLayoutBinding, 1> materialBindings{};
    
    // First binding for material uniform buffer (if needed)
    materialBindings[0].binding = 0;
    materialBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    materialBindings[0].descriptorCount = 1;
    materialBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBindings[0].pImmutableSamplers = nullptr;
    
  
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
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    
    // Uniform buffer pool size
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    // Texture sampler pool sizes for each texture type
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Diffuse texture
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
   

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT+4);//TODO: harded coded the amount of descriptor we can have

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
    ubo.proj[1][1] *= -1; // Flip Y coordinate for Vulkan
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
    }
    if (pbrPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pbrPipelineLayout, nullptr);
    }

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

    // Cleanup old swap chain and dependent resources
    cleanupSwapChain();

    // Recreate swap chain and dependent resources
    createSwapChain();
    // Resize imagesInFlight for the new swap chain
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources(); // If you have depth buffer

    createFramebuffers();
    // Recreate PBR pipeline if needed
   
    createCommandBuffers();
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        imagesInFlight[i] = VK_NULL_HANDLE;
    }
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

void VulkanRenderer::createDefaultTextures() {
    // Create a 1x1 white texture as default albedo
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    defaultAlbedoTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultAlbedoTexture->createFromPixels(whitePixel, 1, 1, 4, commandPool, graphicsQueue);
    
    // Create a 1x1 normal map (pointing straight up)
    unsigned char normalPixel[4] = {128, 128, 255, 255};  // RGB format
    defaultNormalTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultNormalTexture->createFromPixels(normalPixel, 1, 1, 4, commandPool, graphicsQueue);
    
    // Create a 1x1 metallic-roughness map (non-metallic, medium roughness)
    // R: Unused, G: Roughness (128), B: Metallic (0), A: 255
    unsigned char mrPixel[4] = {0, 128, 0, 255};
    defaultMetallicRoughnessTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultMetallicRoughnessTexture->createFromPixels(mrPixel, 1, 1, 4, commandPool, graphicsQueue);
    
    // Create a 1x1 white texture for occlusion (no occlusion)
    defaultOcclusionTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultOcclusionTexture->createFromPixels(whitePixel, 1, 1, 4, commandPool, graphicsQueue);
    
    // Create a 1x1 black texture for emissive (no emission)
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    defaultEmissiveTexture = std::make_shared<Texture>(device, physicalDevice);
    defaultEmissiveTexture->createFromPixels(blackPixel, 1, 1, 4, commandPool, graphicsQueue);
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

void VulkanRenderer::updateMaterialProperties(const Material& material) {
    MaterialUniformBuffer materialData{};
    
    // Set base properties
    materialData.baseColorFactor = glm::vec4(material.diffuseColor, material.alpha);
    materialData.metallicFactor = material.metallic;
    materialData.roughnessFactor = material.roughness;
    materialData.aoStrength = 1.0f;  // Default full strength
    materialData.emissiveStrength = material.emissiveStrength;
    
    // Set texture flags
    materialData.hasBaseColorMap = material.hasTexture(TextureType::Diffuse) ? 1 : 0;
    materialData.hasNormalMap = material.hasTexture(TextureType::Normal) ? 1 : 0;
    materialData.hasMetallicRoughnessMap = 
        (material.hasTexture(TextureType::Metallic) || material.hasTexture(TextureType::Roughness)) ? 1 : 0;
    materialData.hasOcclusionMap = material.hasTexture(TextureType::AmbientOcclusion) ? 1 : 0;
    materialData.hasEmissiveMap = material.hasTexture(TextureType::Emissive) ? 1 : 0;
    
    // Alpha settings (defaults for now)
    materialData.alphaCutoff = 0.5f;
    materialData.alphaMode = 0;  // Opaque
    
    // Copy data to the current frame's uniform buffer
    memcpy(materialUniformBuffersMapped[currentFrame], &materialData, sizeof(materialData));
}


void VulkanRenderer::updateTextureDescriptor(const VkDescriptorImageInfo& imageInfo) {
    // Update MVP descriptor sets
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
    std::cout << "Updated texture descriptor" << std::endl;
    // Update texture descriptor
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = materialDescriptorSets[currentFrame];
    descriptorWrite.dstBinding = 0;  // Now this binding exists!
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
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
// Update updateLights method in VulkanRenderer.cpp
void VulkanRenderer::updateLights() {
    // Get the lights from the scene
    const auto& lights = scene->getLights();
    
    // Create and update the light uniform buffer
    LightUniformBuffer lightData{};
    
    // Set light count (clamped to MAX_LIGHTS)
    lightData.lightCount = static_cast<int>(std::min(lights.size(), static_cast<size_t>(MAX_LIGHTS)));
    
    // Set ambient light
    lightData.ambientColor = glm::vec4(0.03f, 0.03f, 0.03f, 1.0f);
    
    // Set light data for each light
    for (int i = 0; i < lightData.lightCount; i++) {
        const auto& light = lights[i];
        
        // Position (w=0 for directional, w=1 for point light)
        lightData.lights[i].position = glm::vec4(light.position, light.isDirectional ? 0.0f : 1.0f);
        
        // Color and intensity
        lightData.lights[i].color = glm::vec4(light.color, light.intensity);
        
        // Range and falloff
        lightData.lights[i].radius = light.radius;
        lightData.lights[i].falloff = light.falloff;
    }
    
    // Update the light uniform buffer
    memcpy(lightUniformBuffersMapped[currentFrame], &lightData, sizeof(lightData));
}






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
    
    // Only update with texture (COMBINED_IMAGE_SAMPLER) for binding 0
    VkDescriptorImageInfo imageInfo{};
    if (material.hasTexture(TextureType::Diffuse)) {
        imageInfo = material.getTextureImageInfo(TextureType::Diffuse);
    } else {
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = defaultAlbedoTexture->getImageView();
        imageInfo.sampler = defaultAlbedoTexture->getSampler();
    }
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;  // This matches your layout
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;  // This matches your layout
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    
    return descriptorSet;
}

VkPipelineLayout VulkanRenderer::getPipelineLayout()
{
    return pipelineLayout;
}



// Enhanced fragment shader to support IBL





void VulkanRenderer::drawWithPBR(VkCommandBuffer commandBuffer, const glm::mat4& view, const glm::mat4& proj) {
    // Bind the appropriate pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline);
    
    // Set dynamic viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    // Render the scene
    for (const auto& instance : scene->getMeshInstances()) {
        // Calculate model matrix for this instance
        glm::mat4 model = instance.transform.getModelMatrix();
        
        // Update MVP matrices and camera position
        UniformBufferObject ubo{};
        ubo.model = model;
        ubo.view = view;
        ubo.proj = proj;
        ubo.proj[1][1] *= -1; // Flip Y for Vulkan
        ubo.cameraPos = cameraPos;
        ubo.time = static_cast<float>(glfwGetTime()); // Current time for animations
        
        // Copy UBO data to the mapped uniform buffer
        memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
        
        // Update material properties - IMPORTANT: This updates descriptor sets
        // These updates should ideally be done before command buffer recording begins
       //const Material& material = instance.mesh->getMaterial();
        //updateMaterialProperties(material);
        //updateAllTextureDescriptors(material);
        
        // Bind primary descriptor set (materials, textures, etc.)
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pbrPipelineLayout,
            0, // First set index
            1, // Number of descriptor sets
            &mvpDescriptorSets[currentFrame],
            0, nullptr
        );
        
        // If using IBL, bind the IBL descriptor set
        if (renderMode == RenderMode::PBR_IBL && iblDescriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pbrPipelineLayout,
                1, // Second set index
                1, // Number of descriptor sets
                &iblDescriptorSet,
                0, nullptr
            );
        }
        
        // Draw the mesh
        instance.mesh->bind(commandBuffer);
        instance.mesh->draw(commandBuffer);
    }
}


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
        //vkDestroyBuffer(device, lightUniformBuffers[i], nullptr);
        //vkFreeMemory(device, lightUniformBuffersMemory[i], nullptr);
        
        // Cleanup MVP uniform buffers
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    // Cleanup IBL resources TODO: implement IBL
    if (renderMode == RenderMode::PBR_IBL) {
       
    }

    // Cleanup default textures
    defaultTexture.reset();
    defaultAlbedoTexture.reset();
    defaultNormalTexture.reset();
    defaultMetallicRoughnessTexture.reset();
    defaultOcclusionTexture.reset();
    defaultEmissiveTexture.reset();

    // Cleanup descriptor pool and layouts
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, materialDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, mvpDescriptorSetLayout, nullptr);
    // If IBL is used, also destroy its descriptor set layout
    if (iblDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, iblDescriptorSetLayout, nullptr);
    }

    // Cleanup scene (this will clean up all meshes and textures)
    scene.reset();

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


