#pragma once
//---- Include Vulkan ----
// Provides the functions, structures and enumerations
#include <vulkan/vulkan.h>
//---- GLFW Window includes ----
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
//---- GLM maths includes ----
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//---- VS functionality includes ----
// iostream and stdexcept headers are included for reporting and propagating errors
#include <iostream>
#include <stdexcept>
// cstdlib header provides the EXIT_SUCCESS and EXIT_FAILURE
#include <cstdlib>
#include <vector>
#include <optional>
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//=================================================
//       HelloTriangleApplication Structs
//=================================================
// --- Public Structs ---
struct QueueFamilyIndices {
    // Queue families supporting drawing commands and the ones supporting presentation may not overlap
    std::optional<uint32_t> graphics_family;    // Drawing commands
    std::optional<uint32_t> present_family;     // Presentation ability

    // If it can do both
    bool IsComplete() {
        return graphics_family.has_value() && present_family.has_value();
    }
};
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};
struct Vertex {
    glm::vec3 pos;
    glm::vec3 colour;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription;         // Describes the binding for the GPU, including:
        bindingDescription.binding = 0;                             // Binding location
        bindingDescription.stride = sizeof(Vertex);                 // The size (bytes) between data entries
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Whether to move to the next entry after vertex or instance

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescription(3);

        attributeDescription[0].binding = 0;                         // Where the per-vertex data comes from
        attributeDescription[0].location = 0;                        // and the location of the float in the binding
        attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT; // The format of the data (vec3)
        attributeDescription[0].offset = offsetof(Vertex, pos);      // Where to read data from, since the start

    	attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].offset = offsetof(Vertex, colour);

        attributeDescription[2].binding = 0;
        attributeDescription[2].location = 2;
        attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[2].offset = offsetof(Vertex, texCoord);

        return attributeDescription;
    }
};
//=================================================
//          HelloTriangleApplication
//=================================================
// The class responsible for making the window managing our app
class HelloTriangleApplication {
public:
    // --- Public Functions ---
    // Runs the app, called in main 
    void Run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        CleanUp();
    }
    
private:
    // --- Private Functions ---
    void InitWindow();
    void CreateSurface();
    void InitVulkan();
    void MainLoop();
    void DrawFrame();
    auto CleanUpSwapChain() -> void;
    void CleanUp();
    void CreateInstance();
    void PickPhysicalDevice();
    int RateSuitableDevices(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    void CreateSwapChain();
    void RecreateSwapChain();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateTextureImage();
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void CreateImageSampler();
    template<typename BufferType>
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags uFlags, VkMemoryPropertyFlags pFlags, BufferType& buffer, VkDeviceMemory& memory);
    template<typename BufferType>
    void AllocateBindBuffer(VkMemoryPropertyFlags pFlags, BufferType& buffer, VkDeviceMemory& memory, 
        void(*reqFunction)(VkDevice, BufferType, VkMemoryRequirements*), VkResult(*bindFunction)(VkDevice, BufferType, VkDeviceMemory, VkDeviceSize));
    void CopyBuffer(VkBuffer srcBuff, VkBuffer dstBuff, VkDeviceSize size);
    void CopyBuffer2Image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void CreateImageBuffer(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags usage,
                           VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmdBuffer);
    template<typename Type>
    void CreateVertexIndexBuffer(std::vector<Type>dataVec, VkBufferUsageFlagBits useFlag, VkBuffer& buffer, VkDeviceMemory& memory);
    void CreateUniformBuffers();
    void UpdateUniformBuffers(uint32_t currentImage);
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer buffer, uint32_t imageidx);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkImageView CreateImageViews(VkImage image, VkFormat format);
    void CreateLogicalDevice();
    // -- Debug Functions --
    bool CheckValidationLayerSupport() const;
    std::vector<const char*> GetRequiredExtensions() const;
    static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                  VkDebugUtilsMessageTypeFlagsEXT message_type,
                                  const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
                                                 const VkAllocationCallbacks* p_allocator,
                                                 VkDebugUtilsMessengerEXT* p_debug_messenger);
    void SetupDebugMessenger();
    void CreateDescriptorSetLayouts();
    void CreateGraphicsPipeline();
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateRenderPass();
    void CreateSyncObjects();

    // --- Private Attributes ---
    const std::vector<Vertex> verts{
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };
    VkBuffer m_vertexBuffer = nullptr;
    VkDeviceMemory m_vertexBufferMemory;

    const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
    };
    VkBuffer m_indexBuffer = nullptr;
    VkDeviceMemory m_indexBufferMemory;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformMemory;

    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkImage m_textImage;           // A specialised buffer for images (faster accessing times)
    VkDeviceMemory m_textMemory;
    VkImageView m_textImgView;
    VkSampler m_textureSampler;
    bool m_AnisotropyEnabled = VK_TRUE;

    GLFWwindow* m_window = nullptr;
    VkSurfaceKHR m_surface = nullptr;          // The surface handle we use for render targets

    VkSwapchainKHR m_swapChain = nullptr;
    VkSwapchainKHR m_oldSwapChain = nullptr;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout = nullptr;
    VkRenderPass m_renderPass = nullptr;
    VkPipeline m_graphicsPipeline = nullptr;

    VkCommandPool m_commandPool = nullptr;                  // Managing memory for command buffer
    std::vector<VkCommandBuffer> m_commandBuffers;          // The command buffer we execute each update

    std::vector<VkSemaphore> m_imageAvailableSemaphores;    // Semaphore for each frame
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;                  // Sync GPU and CPU
    std::vector<VkFence> m_imagesInFlight;
    size_t m_currentFrame = 0;

    const uint32_t WIDTH = 800;
    const uint32_t HIGHT = 600;

    VkInstance m_instance = nullptr;                        // The vulkan library instance
    VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;    // The vulkan debug messenger
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;     // The graphics card that we'll end up selecting
    VkDevice m_device = nullptr;                            // Logical device handle
    VkQueue m_graphicsQueue = nullptr;                      // Handle for the graphics queues made by the device
    VkQueue m_presentQueue = nullptr;                       // Handle for presentation queues

    const std::vector<const char*> m_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // -- Debug Attributes --
    const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool m_enableValidationLayers = false;
#else
    const bool m_enableValidationLayers = true;
#endif
};
//=================================================
//       END OF HelloTriangleApplication
//=================================================
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>