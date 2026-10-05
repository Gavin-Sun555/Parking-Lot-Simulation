#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "RenderTypes.h"
#include <vector>
#include <string>
#include <optional>
#include <functional>

struct PushConstants {
    float scale[2];
    float offset[2];
};

class VulkanRenderer {
public:
    VulkanRenderer(int width = 1024, int height = 768, const std::string& title = "Parking Lot Simulator - Vulkan");
    ~VulkanRenderer();

    bool init();
    void cleanup();

    bool shouldClose() const;
    void pollEvents();
    void newFrame();
    void render(const RenderBatch& batch);

    GLFWwindow* getWindow() const { return window_; }

    void setCanvasMargins(float left, float top, float right, float bottom) {
        marginLeft_ = left;
        marginTop_ = top;
        marginRight_ = right;
        marginBottom_ = bottom;
    }

    void setKeyCallback(std::function<void(int key, int scancode, int action, int mods)> cb) {
        keyCallback_ = cb;
    }

private:
    int width_;
    int height_;
    std::string title_;
    GLFWwindow* window_ = nullptr;
    bool framebufferResized_ = false;

    float marginLeft_ = 0.0f;
    float marginTop_ = 48.0f;
    float marginRight_ = 370.0f;
    float marginBottom_ = 0.0f;

    std::function<void(int, int, int, int)> keyCallback_;


    // Vulkan Handles
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    uint32_t graphicsQueueFamily_ = 0;
    uint32_t presentQueueFamily_ = 0;

    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    VkFormat swapchainImageFormat_;
    VkExtent2D swapchainExtent_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;

    // Render Pass & Pipelines
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline trianglePipeline_ = VK_NULL_HANDLE;
    VkPipeline linePipeline_ = VK_NULL_HANDLE;

    // Command Pool & Buffers
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores_;
    std::vector<VkSemaphore> renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    uint32_t currentFrame_ = 0;

    // Per-frame Vertex Buffers (Host Visible & Coherent)
    static constexpr size_t VERTEX_BUFFER_CAPACITY = 65536; // 64K vertices
    std::vector<VkBuffer> vertexBuffers_;
    std::vector<VkDeviceMemory> vertexBufferMemories_;
    std::vector<void*> mappedVertexBuffers_;

    // Initialization helpers
    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createImageViews();
    bool createRenderPass();
    bool createGraphicsPipelines();
    bool createFramebuffers();
    bool createCommandPool();
    bool createVertexBuffers();
    bool createCommandBuffers();
    bool createSyncObjects();

    void cleanupSwapchain();
    void recreateSwapchain();

    VkShaderModule createShaderModule(const uint8_t* code, size_t size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // ImGui
    VkDescriptorPool imguiDescriptorPool_ = VK_NULL_HANDLE;
    bool imguiInitialized_ = false;
    bool createImGuiDescriptorPool();
    bool initImGui();
    void cleanupImGui();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

#endif // VULKAN_RENDERER_H
