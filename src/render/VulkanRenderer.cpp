#include "VulkanRenderer.h"
#include "../../shaders/vert_spv.h"
#include "../../shaders/frag_spv.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>

VulkanRenderer::VulkanRenderer(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title) {}

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::init() {
    if (!glfwInit()) {
        std::cerr << "[VulkanRenderer] Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "[VulkanRenderer] Failed to create GLFW window\n";
        return false;
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetKeyCallback(window_, glfwKeyCallback);

    if (!createInstance()) return false;
    if (!createSurface()) return false;
    if (!pickPhysicalDevice()) return false;
    if (!createLogicalDevice()) return false;
    if (!createSwapchain()) return false;
    if (!createImageViews()) return false;
    if (!createRenderPass()) return false;
    if (!createGraphicsPipelines()) return false;
    if (!createFramebuffers()) return false;
    if (!createCommandPool()) return false;
    if (!createVertexBuffers()) return false;
    if (!createCommandBuffers()) return false;
    if (!createSyncObjects()) return false;
    if (!initImGui()) return false;

    std::cout << "[VulkanRenderer] Vulkan initialization successful on " << title_ << "!\n";
    return true;
}

void VulkanRenderer::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->framebufferResized_ = true;
    }
}

void VulkanRenderer::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    if (app && app->keyCallback_) {
        app->keyCallback_(key, scancode, action, mods);
    }
}

bool VulkanRenderer::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Parking Lot Simulator";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Get GLFW extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // Required for macOS portability / MoltenVK
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    VkResult res = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (res != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create Vulkan instance (code " << res << ")\n";
        return false;
    }
    return true;
}

bool VulkanRenderer::createSurface() {
    VkResult res = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
    if (res != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create window surface (code " << res << ")\n";
        return false;
    }
    return true;
}

bool VulkanRenderer::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::cerr << "[VulkanRenderer] Failed to find GPUs with Vulkan support\n";
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    for (const auto& dev : devices) {
        // Find queue families
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

        int graphicsIdx = -1;
        int presentIdx = -1;

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsIdx = i;
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface_, &presentSupport);
            if (presentSupport) {
                presentIdx = i;
            }
            if (graphicsIdx >= 0 && presentIdx >= 0) break;
        }

        if (graphicsIdx >= 0 && presentIdx >= 0) {
            physicalDevice_ = dev;
            graphicsQueueFamily_ = static_cast<uint32_t>(graphicsIdx);
            presentQueueFamily_ = static_cast<uint32_t>(presentIdx);

            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            std::cout << "[VulkanRenderer] Selected GPU: " << props.deviceName << "\n";
            return true;
        }
    }

    std::cerr << "[VulkanRenderer] Failed to find a suitable GPU\n";
    return false;
}

bool VulkanRenderer::createLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueFamily_, presentQueueFamily_};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Check device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionCount, availableExtensions.data());

    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    for (const auto& ext : availableExtensions) {
        if (strcmp(ext.extensionName, "VK_KHR_portability_subset") == 0) {
            deviceExtensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    VkResult res = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (res != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create logical device (code " << res << ")\n";
        return false;
    }

    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);
    return true;
}

bool VulkanRenderer::createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }
    swapchainImageFormat_ = surfaceFormat.format;

    // Extent
    if (capabilities.currentExtent.width != UINT32_MAX) {
        swapchainExtent_ = capabilities.currentExtent;
    } else {
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        swapchainExtent_.width = std::clamp(static_cast<uint32_t>(w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        swapchainExtent_.height = std::clamp(static_cast<uint32_t>(h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {graphicsQueueFamily_, presentQueueFamily_};
    if (graphicsQueueFamily_ != presentQueueFamily_) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // VSync guaranteed
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult res = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);
    if (res != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create swapchain (code " << res << ")\n";
        return false;
    }

    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

    return true;
}

bool VulkanRenderer::createImageViews() {
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages_[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat_;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &createInfo, nullptr, &swapchainImageViews_[i]) != VK_SUCCESS) {
            std::cerr << "[VulkanRenderer] Failed to create image view " << i << "\n";
            return false;
        }
    }
    return true;
}

bool VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create render pass\n";
        return false;
    }
    return true;
}

VkShaderModule VulkanRenderer::createShaderModule(const uint8_t* code, size_t size) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create shader module\n";
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

bool VulkanRenderer::createGraphicsPipelines() {
    VkShaderModule vertShaderModule = createShaderModule(shaders_vert_spv, shaders_vert_spv_len);
    VkShaderModule fragShaderModule = createShaderModule(shaders_frag_spv, shaders_frag_spv_len);

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input binding & attributes
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2]{};
    // Attribute 0: vec2 inPos
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    // Attribute 1: vec4 inColor
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    // Dynamic Viewport & Scissor
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 2.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color Blending (Alpha Blend)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
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

    // Push Constants
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create pipeline layout\n";
        return false;
    }

    // 1. Triangle Pipeline (TRIANGLE_LIST)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyTriangles{};
    inputAssemblyTriangles.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyTriangles.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyTriangles.primitiveRestartEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyTriangles;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &trianglePipeline_) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create triangle pipeline\n";
        return false;
    }

    // 2. Line Pipeline (LINE_LIST)
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyLines{};
    inputAssemblyLines.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyLines.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssemblyLines.primitiveRestartEnable = VK_FALSE;

    pipelineInfo.pInputAssemblyState = &inputAssemblyLines;
    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &linePipeline_) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create line pipeline\n";
        return false;
    }

    vkDestroyShaderModule(device_, fragShaderModule, nullptr);
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);
    return true;
}

bool VulkanRenderer::createFramebuffers() {
    swapchainFramebuffers_.resize(swapchainImageViews_.size());
    for (size_t i = 0; i < swapchainImageViews_.size(); i++) {
        VkImageView attachments[] = { swapchainImageViews_[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapchainExtent_.width;
        framebufferInfo.height = swapchainExtent_.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchainFramebuffers_[i]) != VK_SUCCESS) {
            std::cerr << "[VulkanRenderer] Failed to create framebuffer " << i << "\n";
            return false;
        }
    }
    return true;
}

bool VulkanRenderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create command pool\n";
        return false;
    }
    return true;
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

bool VulkanRenderer::createVertexBuffers() {
    vertexBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
    vertexBufferMemories_.resize(MAX_FRAMES_IN_FLIGHT);
    mappedVertexBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkDeviceSize bufferSize = sizeof(Vertex) * VERTEX_BUFFER_CAPACITY;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device_, &bufferInfo, nullptr, &vertexBuffers_[i]) != VK_SUCCESS) {
            std::cerr << "[VulkanRenderer] Failed to create vertex buffer\n";
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device_, vertexBuffers_[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device_, &allocInfo, nullptr, &vertexBufferMemories_[i]) != VK_SUCCESS) {
            std::cerr << "[VulkanRenderer] Failed to allocate vertex buffer memory\n";
            return false;
        }

        vkBindBufferMemory(device_, vertexBuffers_[i], vertexBufferMemories_[i], 0);
        vkMapMemory(device_, vertexBufferMemories_[i], 0, bufferSize, 0, &mappedVertexBuffers_[i]);
    }
    return true;
}

bool VulkanRenderer::createCommandBuffers() {
    commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to allocate command buffers\n";
        return false;
    }
    return true;
}

bool VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            std::cerr << "[VulkanRenderer] Failed to create sync objects\n";
            return false;
        }
    }
    return true;
}

void VulkanRenderer::cleanupSwapchain() {
    for (auto fb : swapchainFramebuffers_) {
        vkDestroyFramebuffer(device_, fb, nullptr);
    }
    for (auto iv : swapchainImageViews_) {
        vkDestroyImageView(device_, iv, nullptr);
    }
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
}

void VulkanRenderer::recreateSwapchain() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window_, &w, &h);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_);

    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createFramebuffers();

    if (imguiInitialized_) {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &capabilities);
        ImGui_ImplVulkan_SetMinImageCount(capabilities.minImageCount);
    }
}

bool VulkanRenderer::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void VulkanRenderer::pollEvents() {
    glfwPollEvents();
}

void VulkanRenderer::newFrame() {
    pollEvents();
    if (imguiInitialized_) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
}

void VulkanRenderer::render(const RenderBatch& batch) {
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
        imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "[VulkanRenderer] Failed to acquire swap chain image\n";
        return;
    }

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    // Copy batch vertex data into mapped vertex buffer
    size_t triBytes = batch.triangles.size() * sizeof(Vertex);
    size_t lineBytes = batch.lines.size() * sizeof(Vertex);
    size_t totalBytes = triBytes + lineBytes;

    if (totalBytes <= sizeof(Vertex) * VERTEX_BUFFER_CAPACITY) {
        uint8_t* dest = static_cast<uint8_t*>(mappedVertexBuffers_[currentFrame_]);
        if (triBytes > 0) {
            std::memcpy(dest, batch.triangles.data(), triBytes);
        }
        if (lineBytes > 0) {
            std::memcpy(dest + triBytes, batch.lines.data(), lineBytes);
        }
    }

    VkCommandBuffer cmd = commandBuffers_[currentFrame_];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent_;

    // Modern sleek dark canvas clear color
    VkClearValue clearColor = {{{0.12f, 0.13f, 0.16f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    int winW = 1, winH = 1, fbW = 1, fbH = 1;
    glfwGetWindowSize(window_, &winW, &winH);
    glfwGetFramebufferSize(window_, &fbW, &fbH);
    float scaleX = (winW > 0) ? (static_cast<float>(fbW) / static_cast<float>(winW)) : 1.0f;
    float scaleY = (winH > 0) ? (static_cast<float>(fbH) / static_cast<float>(winH)) : 1.0f;

    float canvasX = marginLeft_ * scaleX;
    float canvasY = marginTop_ * scaleY;
    float canvasW = std::max(100.0f, static_cast<float>(swapchainExtent_.width) - (marginLeft_ + marginRight_) * scaleX);
    float canvasH = std::max(100.0f, static_cast<float>(swapchainExtent_.height) - (marginTop_ + marginBottom_) * scaleY);

    VkViewport viewport{};
    viewport.x = canvasX;
    viewport.y = canvasY;
    viewport.width = canvasW;
    viewport.height = canvasH;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {static_cast<int32_t>(canvasX), static_cast<int32_t>(canvasY)};
    scissor.extent = {static_cast<uint32_t>(canvasW), static_cast<uint32_t>(canvasH)};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Push constants for orthographic projection with comfortable margins ensuring the parking lot never clips
    float aspect = canvasW / canvasH;
    float baseRangeX = 150.0f;
    float baseRangeY = 115.0f;
    float baseAspect = baseRangeX / baseRangeY;

    float rangeX, rangeY;
    if (aspect > baseAspect) {
        rangeY = baseRangeY;
        rangeX = rangeY * aspect;
    } else {
        rangeX = baseRangeX;
        rangeY = rangeX / aspect;
    }




    PushConstants pc;
    pc.scale[0] = 1.0f / rangeX;
    pc.scale[1] = 1.0f / rangeY;
    pc.offset[0] = 0.0f;
    pc.offset[1] = 0.0f;

    vkCmdPushConstants(cmd, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pc);

    VkBuffer vertexBuffers[] = { vertexBuffers_[currentFrame_] };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    // 1. Draw Triangles (Quads, circles, shapes)
    if (!batch.triangles.empty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline_);
        vkCmdDraw(cmd, static_cast<uint32_t>(batch.triangles.size()), 1, 0, 0);
    }

    // 2. Draw Lines (Outer walls, stall dividers, antennae)
    if (!batch.lines.empty()) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline_);
        uint32_t lineFirstVertex = static_cast<uint32_t>(batch.triangles.size());
        vkCmdDraw(cmd, static_cast<uint32_t>(batch.lines.size()), 1, lineFirstVertex, 0);
    }

    if (imguiInitialized_) {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData) {
            ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
        }
    }

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to record command buffer\n";
        return;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores_[currentFrame_] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores_[currentFrame_] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to submit draw command buffer\n";
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = { swapchain_ };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapchain();
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::cleanup() {
    cleanupImGui();

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (mappedVertexBuffers_[i]) {
                vkUnmapMemory(device_, vertexBufferMemories_[i]);
                mappedVertexBuffers_[i] = nullptr;
            }
            if (vertexBuffers_[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device_, vertexBuffers_[i], nullptr);
            }
            if (vertexBufferMemories_[i] != VK_NULL_HANDLE) {
                vkFreeMemory(device_, vertexBufferMemories_[i], nullptr);
            }
            if (imageAvailableSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
            }
            if (renderFinishedSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
            }
            if (inFlightFences_[i] != VK_NULL_HANDLE) {
                vkDestroyFence(device_, inFlightFences_[i], nullptr);
            }
        }

        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
        }

        cleanupSwapchain();

        if (trianglePipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, trianglePipeline_, nullptr);
        }
        if (linePipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, linePipeline_, nullptr);
        }
        if (pipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        }
        if (renderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_, renderPass_, nullptr);
        }

        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        if (surface_ != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance_, surface_, nullptr);
            surface_ = VK_NULL_HANDLE;
        }
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

bool VulkanRenderer::createImGuiDescriptorPool() {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16 },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 16;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = pool_sizes;

    VkResult res = vkCreateDescriptorPool(device_, &pool_info, nullptr, &imguiDescriptorPool_);
    if (res != VK_SUCCESS) {
        std::cerr << "[VulkanRenderer] Failed to create ImGui descriptor pool\n";
        return false;
    }
    return true;
}

bool VulkanRenderer::initImGui() {
    if (!createImGuiDescriptorPool()) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(window_, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance_;
    init_info.PhysicalDevice = physicalDevice_;
    init_info.Device = device_;
    init_info.QueueFamily = graphicsQueueFamily_;
    init_info.Queue = graphicsQueue_;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imguiDescriptorPool_;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(swapchainImages_.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = renderPass_;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        std::cerr << "[VulkanRenderer] Failed to initialize ImGui Vulkan backend\n";
        return false;
    }

    imguiInitialized_ = true;
    return true;
}

void VulkanRenderer::cleanupImGui() {
    if (imguiInitialized_) {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        if (imguiDescriptorPool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, imguiDescriptorPool_, nullptr);
            imguiDescriptorPool_ = VK_NULL_HANDLE;
        }
        imguiInitialized_ = false;
    }
}

