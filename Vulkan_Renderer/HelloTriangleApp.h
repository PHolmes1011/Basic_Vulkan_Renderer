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
//==================================================================================================
//=================================================
//              IBuffer Interface
//=================================================
// Holds some general functions to be used by buffer specialisations
class IBuffer
{
public:
    template<typename BufferType>
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags uFlags, VkMemoryPropertyFlags pFlags, BufferType& buffer, VkDeviceMemory& memory);
    template<typename BufferType>
    void AllocateBindBuffer(VkMemoryPropertyFlags pFlags, BufferType& buffer, VkDeviceMemory& memory,
        void(*reqFunction)(VkDevice, BufferType, VkMemoryRequirements*), VkResult(*bindFunction)(VkDevice, BufferType, VkDeviceMemory, VkDeviceSize));
    void CopyBuffer(VkBuffer srcBuff, VkBuffer dstBuff, VkDeviceSize size);

private:

};
//=================================================
//                  Buffer
//=================================================
// Basic buffer for holding information and memory
class Buffer : public IBuffer
{
public:
    Buffer() = default;
    void CleanUp();

    VkBuffer& GetBuffer() { return m_buffer; }
    VkDeviceMemory& GetBufferMemory() { return m_memory; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

};
//=================================================
//                  Image Buffer
//=================================================
// A more specialised buffer for holding images and their views
class ImageBuffer : public IBuffer
{
public:
    ImageBuffer() = default;
    void CleanUp();

    VkImage& GetImage() { return m_image; }
    void SetImage(VkImage image) { m_image = image; }
    VkImageView& GetImageView() { return m_imageView; }
    VkDeviceMemory& GetBufferMemory() { return m_memory; }
    [[nodiscard]] uint8_t GetMipLevels() const { return m_mipLevels; }
    void SetMipLevels(uint8_t mips) { m_mipLevels = mips; }

    void CreateImageViews(VkFormat format, VkImageAspectFlags aspectFlags);
    void CopyBuffer2Image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void CreateImageBuffer(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint8_t mipLevels, VkSampleCountFlagBits sampleCount);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
    uint8_t m_mipLevels = 1;
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;

};
//==================================================================================================
//=================================================
//                  Texture
//=================================================
// A texture class with an ImageBuffer and MipMap generation
class Texture
{
public:
    Texture() = default;
    Texture(const char* filePath, VkFormat format, VkImageAspectFlags aspectFlags, bool hasMipLevels)
    {
        CreateTextureImage(filePath, hasMipLevels);
        m_imageBuffer.CreateImageViews(format, aspectFlags);
    }

    ImageBuffer GetImageBuffer() { return m_imageBuffer; }
private:

    void CreateTextureImage(const char* filePath, bool hasMipLevels);
    void GenerateMipmaps();
    
    ImageBuffer m_imageBuffer;   // A specialised buffer for images (faster accessing times)

    int32_t m_texWidth = 0;
    int32_t m_texHeight = 0;
};
//=================================================
//                  Model
//=================================================
// A model class containing a texture and model (with it's verts and indices)
class Model
{
public:
    Model() = default;
    Model(const char* modelFilePath, const char* textureFilePath, bool hasMipLevels)
    {
        m_texture = Texture(textureFilePath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, hasMipLevels);

        LoadModel(modelFilePath);
        CreateVertexIndexBuffer(m_verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vertexBuffer);
        CreateVertexIndexBuffer(m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_indexBuffer);
    }
    void CleanUp();

    Texture& GetTexture() { return m_texture; }
    Buffer& GetVertexBuffer() { return m_vertexBuffer; }
    Buffer& GetIndexBuffer() { return m_indexBuffer; }  
    std::vector<Vertex>& GetVertexArray() { return m_verts; }
    std::vector<uint32_t>& GetIndexArray() { return m_indices; }

    void LoadModel(const char* filePath);
    template<typename Type>
    void CreateVertexIndexBuffer(std::vector<Type>dataVec, VkBufferUsageFlagBits useFlag, Buffer& buffer);

private:
    std::vector<Vertex> m_verts;
    Buffer m_vertexBuffer;

    std::vector<uint32_t> m_indices;
    Buffer m_indexBuffer;

    Texture m_texture;
};
//==================================================================================================
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

    static HelloTriangleApplication& Instance();
    const VkDevice& GetDevice() const { return m_device; }
    const VkPhysicalDevice& GetPhysicalDevice() const { return m_physicalDevice; }

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmdBuffer);

    uint8_t m_maxMip = 1;
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
    VkSampleCountFlagBits GetMaxUsableSampleCount();
    void PickPhysicalDevice();
    int RateSuitableDevices(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    void CreateSwapChain();
    void RecreateSwapChain();
    void CreateFrameBuffers();
    void CreateCommandPool();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);
    void CreateDepthResources();
    
    void CreateImageSampler();
    
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
    void CreateRenderTargets();
    void CreateSyncObjects();

    // --- Private Attributes ---
    static HelloTriangleApplication* m_appPointer;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    std::vector<Buffer> m_uniformBuffers;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkSampler m_textureSampler = VK_NULL_HANDLE;
    bool m_AnisotropyEnabled = VK_TRUE;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    ImageBuffer m_renderTargetImageBuffer;

    ImageBuffer m_depthImageBuffer;

    GLFWwindow* m_window = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;          // The surface handle we use for render targets

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkSwapchainKHR m_oldSwapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<ImageBuffer> m_swapChainImageBuffers;
    std::vector<VkFramebuffer> m_swapChainFrameBuffers;

    VkDescriptorSetLayout m_descriptorSetLayout;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;           // Managing memory for command buffer
    std::vector<VkCommandBuffer> m_commandBuffers;          // The command buffer we execute each update

    std::vector<VkSemaphore> m_imageAvailableSemaphores;    // Semaphore for each frame
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;                  // Sync GPU and CPU
    std::vector<VkFence> m_imagesInFlight;
    size_t m_currentFrame = 0;

    const uint32_t WIDTH = 800;
    const uint32_t HIGHT = 600;

    std::vector<Model> m_models;

    VkInstance m_instance = VK_NULL_HANDLE;                        // The vulkan library instance
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;    // The vulkan debug messenger
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;     // The graphics card that we'll end up selecting
    VkDevice m_device = VK_NULL_HANDLE;                            // Logical device handle
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;                      // Handle for the graphics queues made by the device
    VkQueue m_presentQueue = VK_NULL_HANDLE;                       // Handle for presentation queues

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
inline void Buffer::CleanUp()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    vkDestroyBuffer(application.GetDevice(), m_buffer, nullptr);
    vkFreeMemory(application.GetDevice(), m_memory, nullptr);

}
inline void ImageBuffer::CleanUp()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();
    
    vkDestroyImageView(application.GetDevice(), m_imageView, nullptr);
    vkDestroyImage(application.GetDevice(), m_image, nullptr);
    
}
inline void Model::CleanUp()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    if (m_vertexBuffer.GetBuffer() != nullptr) {
        vkDestroyBuffer(application.GetDevice(), m_vertexBuffer.GetBuffer(), nullptr);
        vkFreeMemory(application.GetDevice(), m_vertexBuffer.GetBufferMemory(), nullptr);
    }
    if (m_indexBuffer.GetBuffer() != nullptr) {
        vkDestroyBuffer(application.GetDevice(), m_indexBuffer.GetBuffer(), nullptr);
        vkFreeMemory(application.GetDevice(), m_indexBuffer.GetBufferMemory(), nullptr);
    }
}



// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>