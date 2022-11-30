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

    static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
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
    void CleanUp();

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
//              RenderManager
//=================================================
// Handles rendering functions and presents images to the screen.
// This includes: SwapChain, RenderPass and Pipeline
class RenderManager
{
public:
    static RenderManager& Instance();

    void InitialiseRenderer()
    {
        m_renderMPointer = this;

        //---- Rendering ----
        CreateSwapChain();
        CreateRenderPass();
        CreateDescriptorSetLayouts();
        CreateGraphicsPipeline();
    }
    void RecreateSwapChain();
    void CleanUp();

    const VkSwapchainKHR GetSwapChain() const { return m_swapChain; }
    std::vector<ImageBuffer>& GetSwapChainImageBuffers() { return m_swapChainImageBuffers; }
    std::vector<VkFramebuffer>& GetSwapChainFrameBuffers() { return m_swapChainFrameBuffers; }
    std::vector<VkCommandBuffer>& GetCommandBuffer() { return m_commandBuffers; }
    const VkRenderPass GetRenderPass() const { return m_renderPass; }
    const VkExtent2D GetSwapChainExtent() const { return m_swapChainExtent; }
    const VkFormat GetSwapChainFormat() const { return m_swapChainImageFormat; }
    const VkPipeline GetGraphicsPipeline() const { return m_graphicsPipeline; }
    const VkPipelineLayout GetGraphicsPipelineLayout() const { return m_pipelineLayout; }
    const VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
private:
    void CreateSwapChain();
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void CreateRenderPass();

    void CreateDescriptorSetLayouts();

    void CreateGraphicsPipeline();
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkSwapchainKHR m_oldSwapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<ImageBuffer> m_swapChainImageBuffers;
    std::vector<VkFramebuffer> m_swapChainFrameBuffers;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> m_commandBuffers;          // The command buffer we execute each update

    VkDescriptorSetLayout m_descriptorSetLayout;

    static RenderManager* m_renderMPointer;
};
//==================================================================================================
//=================================================
//               BufferManager
//=================================================
// Creates and fills buffers, descriptors and push constants
class BufferManager
{
public:
    static BufferManager& Instance();

    void CreateBuffers()
    {
        m_bufferMPointer = this;

        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateCommandBuffers();
    }
    void UpdateUniformBuffers(uint32_t currentImage);
    void RecordCommandBuffer(VkCommandBuffer buffer, uint32_t imageidx);
    void CleanUp();

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmdBuffer);

private:

    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateCommandBuffers();

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    std::vector<Buffer> m_uniformBuffers;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptorSets;

    static BufferManager* m_bufferMPointer;
};
//==================================================================================================
//=================================================
//               InstanceManager
//=================================================
// Handles the window and Vulkan instance, including finding the device
class InstanceManager
{
public:
    static InstanceManager& Instance();

    void CreateAppInstance()
    {
        m_instanceMPointer = this;

        //---- Instance ----
        InitWindow();
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
    }
    void CleanUp();
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

    const VkDevice& GetDevice() const { return m_device; }
    const VkPhysicalDevice& GetPhysicalDevice() const { return m_physicalDevice; }
    const VkSampleCountFlagBits GetMSAASampleCount() const { return m_msaaSamples; }
    const VkSurfaceKHR& GetSurface() const { return m_surface; }
    const VkCommandPool GetCommandPool() const { return m_commandPool; }
    const VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    const VkQueue GetPresentQueue() const { return m_presentQueue; }
    GLFWwindow* GetWindow() const { return m_window; }

    bool m_AnisotropyEnabled = VK_TRUE;
private:
    // ================== Methods ==================
    void InitWindow();
    void CreateInstance();
    std::vector<const char*> GetRequiredExtensions() const;
    void SetupDebugMessenger();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    bool CheckValidationLayerSupport() const;
    static VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator,
        VkDebugUtilsMessengerEXT* p_debug_messenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    void CreateSurface();

    void PickPhysicalDevice();
    int  RateSuitableDevices(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    VkSampleCountFlagBits GetMaxUsableSampleCount();

    void CreateLogicalDevice();

    void CreateCommandPool();

    // ================== Attributes ==================

    VkInstance m_instance = VK_NULL_HANDLE;                        // The vulkan library instance
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;    // The vulkan debug messenger
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;            // The graphics card that we'll end up selecting
    VkDevice m_device = VK_NULL_HANDLE;                            // Logical device handle
    VkCommandPool m_commandPool = VK_NULL_HANDLE;                  // Managing memory for command buffer
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;                      // Handle for the graphics queues made by the device
    VkQueue m_presentQueue = VK_NULL_HANDLE;                       // Handle for presentation queues

    GLFWwindow* m_window = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;          // The surface handle we use for render targets

    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    const uint32_t m_windowWidth = 800;
    const uint32_t m_windowHeight = 600;

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

    static InstanceManager* m_instanceMPointer;
};
//==================================================================================================
//=================================================
//                ImageManager
//=================================================
// Creates and owns images, like Render Target and the Depth Buffer. But not the SwapChain
class ImageManager
{
public:
    static ImageManager& Instance();

    void InitialiseImages()
    {
        m_imageMPointer = this;

        CreateRenderTargets();
        CreateDepthResources();
        CreateFrameBuffers();
    }
    void CreateImageSampler();
    void CleanUp();

    ImageBuffer& GetRenderTargetImageBuffer() { return m_renderTargetImageBuffer; }
    ImageBuffer& GetDepthImageBuffer() { return m_depthImageBuffer; }
    const VkSampler GetTextureSampler() const { return m_textureSampler; }
private:
    void CreateRenderTargets();
    void CreateDepthResources();
    void CreateFrameBuffers();

    VkSampler m_textureSampler = VK_NULL_HANDLE;
    ImageBuffer m_renderTargetImageBuffer;
    ImageBuffer m_depthImageBuffer;

    static ImageManager* m_imageMPointer;
};
//==================================================================================================
//=================================================
//             SyncObjectManager
//=================================================
// Fills and 
class SyncObjectManager
{
public:
    static SyncObjectManager& Instance();

    void CreateSyncObjects();
    void CleanUp();

    std::vector<VkFence>& GetInFlightFences() { return m_inFlightFences; }
    std::vector<VkFence>& GetInFlightImages() { return m_imagesInFlight; }
    std::vector<VkSemaphore>& GetAvailableSemaphores() { return m_imageAvailableSemaphores; }
    std::vector<VkSemaphore>& GetFinishedSemaphores() { return m_renderFinishedSemaphores; }

private:

    std::vector<VkSemaphore> m_imageAvailableSemaphores;    // Semaphore for each frame
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;                  // Sync GPU and CPU
    std::vector<VkFence> m_imagesInFlight;

    static SyncObjectManager* m_syncMPointer;
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
        InitVulkan();
        MainLoop();
        CleanUp();
    }

    static HelloTriangleApplication& Instance();

    const InstanceManager& GetInstanceManager() const { return m_instanceManager; }
    const RenderManager& GetRenderManager() const { return m_renderManager; }
    const ImageManager& GetImageManager() const { return m_imageManager; }
    const BufferManager& GetBufferManager() const { return m_bufferManager; }
    const SyncObjectManager& GetSyncObjectManager() const { return m_syncManager; }

    const size_t GetCurrentFrame() const { return m_currentFrame; }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
        VkFormatFeatureFlags features);

    std::vector<Model> m_models;
    uint8_t m_maxMip = 1;
private:
    // --- Private Functions ---    
    void InitVulkan();
    void MainLoop();
    void DrawFrame();
    void CleanUp();
    
    // --- Private Attributes ---
    static HelloTriangleApplication* m_appPointer;

    InstanceManager m_instanceManager;
    RenderManager m_renderManager;
    BufferManager m_bufferManager;
    ImageManager m_imageManager;
    SyncObjectManager m_syncManager;

    size_t m_currentFrame = 0;
};
//=================================================
//       END OF HelloTriangleApplication
//=================================================
inline void Buffer::CleanUp()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    vkDestroyBuffer(application.GetInstanceManager().GetDevice(), m_buffer, nullptr);
    vkFreeMemory(application.GetInstanceManager().GetDevice(), m_memory, nullptr);

}
inline void ImageBuffer::CleanUp()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();
    
    vkDestroyImageView(application.GetInstanceManager().GetDevice(), m_imageView, nullptr);
    vkDestroyImage(application.GetInstanceManager().GetDevice(), m_image, nullptr);
    vkFreeMemory(application.GetInstanceManager().GetDevice(), m_memory, nullptr);
    
}
inline void Model::CleanUp()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    if (m_vertexBuffer.GetBuffer() != nullptr) {
        vkDestroyBuffer(application.GetInstanceManager().GetDevice(), m_vertexBuffer.GetBuffer(), nullptr);
        vkFreeMemory(application.GetInstanceManager().GetDevice(), m_vertexBuffer.GetBufferMemory(), nullptr);
    }
    if (m_indexBuffer.GetBuffer() != nullptr) {
        vkDestroyBuffer(application.GetInstanceManager().GetDevice(), m_indexBuffer.GetBuffer(), nullptr);
        vkFreeMemory(application.GetInstanceManager().GetDevice(), m_indexBuffer.GetBufferMemory(), nullptr);
    }
    m_texture.CleanUp();
}
inline void Texture::CleanUp()
{
    m_imageBuffer.CleanUp();
}




// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>