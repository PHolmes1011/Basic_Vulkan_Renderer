#include <set>
#include <map>
#include <algorithm>
#include <fstream>      // For loading shaders
#include <chrono>

#include "HelloTriangleApp.h"

//---- stb image loader ----
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
//---- tiny obj loader ----
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

HelloTriangleApplication* HelloTriangleApplication::m_appPointer = nullptr;

HelloTriangleApplication& HelloTriangleApplication::Instance()
{
    if (!m_appPointer)
        m_appPointer = new HelloTriangleApplication();

    return *m_appPointer;
}

//==================================================================================================
//  InitWindow
//==================================================================================================
// Name: InitWindow
// Desc: Creates the GLFW window, called once at the beginning
// Params: NONE
// Return: NONE
void HelloTriangleApplication::InitWindow()
{
    // Initializes the GLFW library
    glfwInit();

    // Tell it to NOT use OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(WIDTH, HIGHT, "Vulkan Window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";
}
//=================================================
// Name: CreateSurface
// Desc: Creates a surface to work with the GLFW window
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateSurface()
{
    // ------ NOTE ------
	// This has to be called before devices get picked as this can interfere
    // ------------------

    // Creates our surface, changing to suit what OS we are using
    assert(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) == VK_SUCCESS);
}
//==================================================================================================
//  InitVulkan
//==================================================================================================
// Name: InitVulkan
// Desc: Called once at the beginning, calling other init functions
// Params: NONE
// Return: NONE
void HelloTriangleApplication::InitVulkan()
{
    m_appPointer = this;

    //---- Instance ----
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    //---- Rendering ----
    CreateSwapChain();
    CreateRenderPass();
    CreateDescriptorSetLayouts();
    CreateGraphicsPipeline();
    CreateCommandPool();
    //---- Images ----
    CreateRenderTargets();
    CreateDepthResources();
    CreateFrameBuffers();
    CreateImageSampler();
    //---- Buffers ----
    Model vikingRoom = Model("Models/viking_room.txt", "Textures/viking_room.png", true);
    m_models.push_back(vikingRoom);
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    //---- Sync Objects ----
    CreateSyncObjects();
}
// =================================================
// Name: CreateInstance
// Desc: Create and load a vulkan instance
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateInstance()
{
    // Check the validation layer code worked correctly
    assert(m_enableValidationLayers && CheckValidationLayerSupport());

    // Fill in a struct with some information about our application
    // This is technically optional but it may provide some useful information to driver
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Tells the Vulkan driver which global extensions and validation layers we want to use
    // Not optional
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // The next two layers specify the desired global extensions
    // Vulkan needs an extension to interface with the window system
    std::vector<const char*> extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // The last two members of the struct determine the global validation layers to enable
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_enableValidationLayers) {
        // Select the validation layer we want for the amount of debugging we want
        // By default Vulkan has little to no error checking or handling
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        // Should be 0 in release mode
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    assert(vkCreateInstance(&createInfo, nullptr, &m_instance) == VK_SUCCESS);

#ifdef _DEBUG
    // To retrieve a list of supported extensions before creating an instance
    // Find how many extensions there are
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    // Allocate an array to hold the extension details
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    // Finally we can query the extension details
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());
    // Each VkExtensionProperties struct contains the name and version of an extension
    std::cout << "available extensions:\n";

    for (const std::vector<VkExtensionProperties>::value_type& extensionProperty : extensionProperties) {
        std::cout << '\t' << extensionProperty.extensionName << '\n';
    }
#endif
}
// =================================================
// Name: GetMaxUsableSampleCount
// Desc: Ask the device whats the most samples it's capable of
// Params: NONE
// Return: NONE
VkSampleCountFlagBits HelloTriangleApplication::GetMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}
// =================================================
// Name: PickPhysicalDevice
// Desc: Look for physical devices to run the code (CPU / GPU)
// Params: NONE
// Return: NONE
void HelloTriangleApplication::PickPhysicalDevice()
{
    // Querying just the number of devices present
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    // Make sure there are devices
    assert(deviceCount);

    // An array to hold all of the VkPhysicalDevice handles
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    // Check if any of the physical devices meet the requirements
    for (const auto& device : devices) {
        int score = RateSuitableDevices(device);
        candidates.insert(std::make_pair(score, device));
    }

    // Check to make sure the best is suitable at all
    if (candidates.rbegin()->first > 0)
    {
        m_physicalDevice = candidates.rbegin()->second;
        m_msaaSamples = GetMaxUsableSampleCount();
    }

    // Check again if we found a device
    assert(m_physicalDevice);
}
// =================================================
// Name: RateSuitableDevices
// Desc: For evaluating graphics cards to see if they are suitable
// Params: device
// Return: bool
int HelloTriangleApplication::RateSuitableDevices(VkPhysicalDevice device)
{
    int score = 0;

    // Evaluate the suitability of a device by querying for some details
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // Check for optional features on the device
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (!deviceFeatures.samplerAnisotropy)
        m_AnisotropyEnabled = VK_FALSE;

    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
    }

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

#define VK_API_VERSION_1_2_198 VK_MAKE_API_VERSION(0, 1, 2, 198) //define a version number for this version

    // Check for critical features we need
    // In this case we check the GPU can run this or previous versions of Vulkan,
    // if it can use the commands we want to use
    // And can run and support our swap chain on our surface

	score *= !(deviceProperties.apiVersion <= VK_API_VERSION_1_2_198 || !deviceFeatures.geometryShader ||
        !extensionsSupported || !swapChainAdequate || !indices.IsComplete());

    return score;
}
// =================================================
// Name: FindQueueFamilies
// Desc: Check our devices for the queue families they can use - Queue families being different command lists for the GPU
// Params: device
// Return: QueueFamilym_indices(struct)
QueueFamilyIndices HelloTriangleApplication::FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    // Logic to find queue family m_indices to populate struct with
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());


    // Look for at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
    // And supports drawing to our surface
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {

		VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport) {
            indices.present_family = i;
        }

        //TODO: Maybe look into the Transfer queue implementation
        //https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {   // Also gives VK_QUEUE_TRANSFER_BIT for staging buffer
            indices.graphics_family = i;
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

	return indices;
}
// =================================================
// Name: CheckDeviceExtensionSupport
// Desc: Check the device has all the extensions we want
// Params: device
// Return: bool
bool HelloTriangleApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}
// =================================================
// Name: CreateSwapChain
// Desc: Makes the swap chain calling the other methods to format it
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateSwapChain() {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // As is tradition with Vulkan objects, creating the swap chain object requires filling in a large structure
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.oldSwapchain = m_oldSwapChain;                       // The last swapchain (if there is one)

    // Next, we need to specify how to handle swap chain images that will be used across multiple queue families
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphics_family.value(), indices.present_family.value() };

    if (indices.graphics_family != indices.present_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //Images can be used across multiple queue families
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //Image is owned by one queue family at a time
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    assert(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) == VK_SUCCESS);

    // Save what we've made to reference later
    m_swapChainImageBuffers.resize(imageCount);
    m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;

    // Iterate over all of the swap chain images
    for (size_t i = 0; i < m_swapChainImageBuffers.size(); i++)
    {
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, &m_swapChainImageBuffers[i].GetImage());
        m_swapChainImageBuffers[i].CreateImageViews(m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}
// =================================================

void HelloTriangleApplication::RecreateSwapChain()
{
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device);     // Wait until the resources are free to use

    m_oldSwapChain = m_swapChain;   // Set the current chain to the old one so we can keep rendering using it

    CleanUpSwapChain();             // Cleanup the current one

    // Call associated swap chain functions to remake it
    CreateSwapChain();                      // Swap chain itself and image views based on swap chain images
    CreateRenderPass();                     // Depends of the format of the swap chain (rare to change)
    CreateGraphicsPipeline();               // Viewport and scissors need to be remade (could be avoided with dynamic states)
    CreateRenderTargets();
    CreateDepthResources();
    CreateFrameBuffers();                   // Directly depend on the swap chain

    m_oldSwapChain = nullptr;
}
// =================================================
// Name: CreateFrameBuffers
// Desc: Create the frame buffers we use to send data to the swap chain images
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateFrameBuffers()
{
    // Resizing the container to hold all of the frame buffers
    m_swapChainFrameBuffers.resize(m_swapChainImageBuffers.size());

    // Iterate through the image views and create framebuffers from them
    for (size_t i = 0; i < m_swapChainFrameBuffers.size(); i++) {
        std::vector<VkImageView>  attachments = {
            m_renderTargetImageBuffer.GetImageView(),
            m_depthImageBuffer.GetImageView(),
            m_swapChainImageBuffers[i].GetImageView()
        };

        VkFramebufferCreateInfo frameBufferInfo{};
        frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferInfo.renderPass = m_renderPass;
        frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferInfo.pAttachments = attachments.data();
        frameBufferInfo.width = m_swapChainExtent.width;
        frameBufferInfo.height = m_swapChainExtent.height;
        frameBufferInfo.layers = 1;

        assert(vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_swapChainFrameBuffers[i]) == VK_SUCCESS);
    }
}
// =================================================
// Name: CreateCommandPool
// Desc: Makes the command pool for managing command buffer memory
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateCommandPool()
{
    // Command pool creation only takes two parameters
    // Command buffers run on the device queue
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics_family.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    assert(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) == VK_SUCCESS);
}
// =================================================
// Check if the format found has a stencil component or not
bool HasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
// =================================================
// Name: FindSupportedFormat
// Desc: Finds and returns the best format for images
// Params: candidates, tiling, features
// Return: VkFormat
VkFormat HelloTriangleApplication::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    // The best format depends on the tiling mode and usage
    for (VkFormat format : candidates) {
        VkFormatProperties props; // Has three formats: linearTilingFeatures, optimalTilingFeatures and bufferFeatures
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        // We only care about the first two
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    // A fail-safe selecting a default format if the best is not found. It's the most commonly supported
    printf("%s", "Failed to find suitable format. Default: VK_FORMAT_D32_SFLOAT selected\n");
    return VK_FORMAT_D32_SFLOAT;
}
// =================================================
// Name: CreateDepthResources
// Desc: Creates a depth buffer for layering fragments in order of distance
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateDepthResources()
{
    // Find the best format
    VkFormat depthFormat = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    m_depthImageBuffer.CreateImageBuffer(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, m_msaaSamples);

    m_depthImageBuffer.CreateImageViews(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT); // Doesn't need a specific format, just enough accuracy

    // The image doesn't have to be transitioned as it will be done implicitly in the render pass

}
// =================================================
// Name: GenerateMipmaps
// Desc: Generates the specified number of mipmaps from a given image. Also transfers the sum to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
// Params: image, texWidth, texHeight, mipLevels
// Return: NONE
void Texture::GenerateMipmaps()
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    VkCommandBuffer commandBuffer = application.BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_imageBuffer.GetImage();
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = m_texWidth;
    int32_t mipHeight = m_texHeight;

    for (uint32_t i = 1; i < m_imageBuffer.GetMipLevels(); ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1; // Wait for previous blit or copy command to finish
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0,0,0 };                 // Define the 3D area to blit from
        blit.srcOffsets[1] = { mipWidth,mipHeight,1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;                     // Previous mip
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };               // And the area to blit to (divided by 2 as it's half the size)
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;                         // This mip
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        // TODO: If using dedicated transfer queue, blit requires a queue with graphics capability
        vkCmdBlitImage(commandBuffer,
            m_imageBuffer.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Image is the source
            m_imageBuffer.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // and the dst
            1, &blit,
            VK_FILTER_NEAREST);      // The same as on the sampler - pixelly

        // Another barrier waiting for the blit to finish
        // Sampling will also wait for this command 
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // Divide the mip width and height for the next cycle
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // We need to include another barrier outside the loop for the last mip that was never blitted from
    barrier.subresourceRange.baseMipLevel = m_imageBuffer.GetMipLevels() - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    application.EndSingleTimeCommands(commandBuffer);
}
// =================================================
// Name: CreateTextureImage
// Desc: Load a image file for a texture
// Params: NONE
// Return: NONE
void Texture::CreateTextureImage(const char* filePath, bool hasMipLevels)
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    // Use the STB library to load our image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    // max (largest dimension)  log2 (how many times can be divided by 2)  floor (for times when it's not divisible by 2)
    uint8_t mipLevels = static_cast<uint8_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1; // +1 for original image level
    assert(pixels);
    m_imageBuffer.SetMipLevels(mipLevels);
    if (application.m_maxMip < mipLevels)
        application.m_maxMip = mipLevels;

    // Define the size of the image in memory
    VkDeviceSize imgSize = texWidth * texHeight * 4; // 4 bytes per pixel

    // The usual staging buffer setup
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    m_imageBuffer.CreateBuffer(imgSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(application.GetDevice(), stagingMemory, 0, imgSize, 0, &data);
    memcpy(data, pixels, imgSize);
    vkUnmapMemory(application.GetDevice(), stagingMemory);

    // Free the pixel array made by stb
    stbi_image_free(pixels);

    // Create an image buffer
    m_imageBuffer.CreateImageBuffer(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_imageBuffer.GetMipLevels(), VK_SAMPLE_COUNT_1_BIT);

    // Transfer it to the right layout
    m_imageBuffer.TransitionImageLayout(m_imageBuffer.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy the staging buffer to texture image buffer
    m_imageBuffer.CopyBuffer2Image(stagingBuffer, m_imageBuffer.GetImage(), texWidth, texHeight);

    if (hasMipLevels)
        GenerateMipmaps();  // Generate smaller images for lower LOD
    else
        m_imageBuffer.TransitionImageLayout(m_imageBuffer.GetImage(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		// Convert back to a format that the shader can access

    vkDestroyBuffer(application.GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(application.GetDevice(), stagingMemory, nullptr);
}
// =================================================
// Name: TransitionImageLayout
// Desc: Transforms an image's layout for copying
// Params: image, format, oldLayout, newLayout
// Return: NONE
void ImageBuffer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    VkCommandBuffer commandBuffer = application.BeginSingleTimeCommands();

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    // Barrier provides a way of syncing access to resources
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // For transferring ownership
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // We don't want to
    barrier.image = image;                                  // Image and its associated info
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // There are two transfers we need to handle
    // Undefined -> transfer destination: transfer writes that don't need to wait on anything
    // Transfer destination -> shader reading: The shader needs to wait for the data to be readable
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;                            // Don't need to wait for anything
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for the transfer to finish

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;      // Since there's no need to wait we can do it ASAP
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;    // Must occur at the transfer stage (not "real" stage when transfers happen)
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for the transfer to finish
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;    // Wait for the read to finish before changing again

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;             // Wait until the transfer stage
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // Do this when reading in the fragment shader
    }

    // All barriers are submitted using the same function

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage, // The before and after operations from before
        0,                                       // Either 0 or VK_DEPENDENCY_BY_REGION_BIT meaning regions done writing can be read
        0, nullptr,                // The last three are for arrays and other memory types
        0, nullptr,
        1, &barrier            // We want one image one
    );

	application.EndSingleTimeCommands(commandBuffer);
}
// =================================================
// Name: CreateImageSampler
// Desc: Create a sampler to format the image views
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateImageSampler()
{
    VkSamplerCreateInfo samplerInfo;
    samplerInfo.pNext = nullptr;
    samplerInfo.flags = 0;
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;  // For oversampling - off, to make it pixelly
    samplerInfo.minFilter = VK_FILTER_LINEAR;   // For undersampling - on, to sharpen the edges
    // For when the image is smaller then the size of the texture
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // Repeat
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // or Repeat mirrored
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;  // Clamp or Clamp to border
    // Check the device for it's maximum anisotropy amount
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
    // And set it here
    samplerInfo.anisotropyEnable = m_AnisotropyEnabled;                 // If not supported toggle it off
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // Bigger number = better result : worse performance
    // The colour for fragments off the texture
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;  // invalidated by repeat address mode
    // Setting to false makes coordinates 0 to 1
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // If true texels will be compared before filtering
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // Then the mipmapping fields
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f; // Optional
    samplerInfo.minLod = 0.0f;     // Optional
    samplerInfo.maxLod = static_cast<float>(m_maxMip);

    // Finally we can make our sampler
    assert(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) == VK_SUCCESS);
}
// =================================================
// Name: LoadModel
// Desc: Loads a model to be used in the vertex buffer
// Params: NONE
// Return: NONE
void Model::LoadModel(const char* filePath)
{
    // Declare our variables
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Call tiny OBJ's function
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath)) {
        throw std::runtime_error(warn + err);
        //TODO: load error model
    }

    // Format the loaded info
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // If we have UV coords use them
            if (index.texcoord_index >= 0)
            {
                vertex.texCoord = {
                	attrib.texcoords[2 * index.texcoord_index + 0],
                	1.0f - attrib.texcoords[2 * index.texcoord_index + 1]   // Invert our y
                };
            }
            else     // Else specify a default and warn the user
            {
	            vertex.texCoord = { 0.0f, 1.0f };
                printf("%s", "Loaded model contains no UV coordinates. Fallback used.");
            }

            vertex.colour = { 1.0f, 1.0f, 1.0f };

            // Loop assumes each vert is unique and no duplication occurs
            // It will still work if it does, just less efficiently
            m_verts.emplace_back(vertex);
            m_indices.emplace_back(m_indices.size());
        }
    }
}
// =================================================
uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t filter, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProperties);
    // Gets two arrays, memoryType and memoryHeap

    // Check for a suitable memory type (can hold vertex data, can be read and written to)
    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if (filter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    }

    // No check for memoryHeap, can lead to less then desirable performance

    // If nothing suitable is found assert false
    assert(false);
}
// =================================================
// Name: CreateBuffer
// Desc: Creates a buffer usable buffer for vertex and index data
// Params: NONE
// Return: NONE
template<typename BufferType>
void Buffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags uFlags, VkMemoryPropertyFlags pFlags, 
    BufferType& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = size;      // Size of buffer
    bufferInfo.usage = uFlags;   // The purpose of the data
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;     // Its level of protection, only the graphics queue needs it so exclusive
    bufferInfo.flags = 0;   // Used to configure sparse memory

    // Create a buffer
    assert(vkCreateBuffer(HelloTriangleApplication::Instance().GetDevice(), &bufferInfo, nullptr, &buffer) == VK_SUCCESS);

    AllocateBindBuffer(pFlags, buffer, memory, vkGetBufferMemoryRequirements, vkBindBufferMemory);
}
// =================================================
template<typename BufferType>
void Buffer::AllocateBindBuffer(VkMemoryPropertyFlags pFlags, BufferType& buffer, VkDeviceMemory& memory, 
    void(*reqFunction)(VkDevice, BufferType, VkMemoryRequirements*), VkResult(*bindFunction)(VkDevice, BufferType, VkDeviceMemory, VkDeviceSize))
{
    // Get the memory requirements for our allocator
    VkMemoryRequirements memoryRequirements;
    reqFunction(HelloTriangleApplication::Instance().GetDevice(), buffer, &memoryRequirements);

    // The memory allocator to be paired with the buffer
    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(HelloTriangleApplication::Instance().GetPhysicalDevice(), memoryRequirements.memoryTypeBits, pFlags);
    // vkFlushMappedMemoryRanges(m_device, memoryrange.length, memoryrange.data) can be used instead of VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    // Ensures the memory is made available immidiately with explicit caching

    // Allocate the memory that can accomdate our requirements
    assert(vkAllocateMemory(HelloTriangleApplication::Instance().GetDevice(), &allocInfo, nullptr, &memory) == VK_SUCCESS);
    // TODO: IRL calling allocate for every buffer is bad practice, the 'right' way is for many buffers to share an allocate
    // Could make own allocator or use the VulkanMemoryAllocator library (Red Kite recommended making own allocator remember)

    // Bind our buffer and memory together (Vertex buffer in this case)
    bindFunction(HelloTriangleApplication::Instance().GetDevice(), buffer, memory, 0);
    // Offset should always be divisible by memReqs.allignment
}
// =================================================
void Buffer::CopyBuffer(VkBuffer srcBuff, VkBuffer dstBuff, VkDeviceSize size)
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    // Memory transfer operations are done using command buffers, like drawing commands
    VkCommandBuffer commandBuffer = application.BeginSingleTimeCommands();

    // The copy info for..
    VkBufferCopy copyInfo;
    copyInfo.srcOffset = 0; // Optional 
    copyInfo.dstOffset = 0; // Optional 
    copyInfo.size = size;
    // .. The copy instruction
    vkCmdCopyBuffer(commandBuffer, srcBuff, dstBuff, 1, &copyInfo);

    application.EndSingleTimeCommands(commandBuffer);
}
// =================================================
void ImageBuffer::CopyBuffer2Image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    HelloTriangleApplication application = HelloTriangleApplication::Instance();

    VkCommandBuffer commandBuffer = application.BeginSingleTimeCommands();

    VkBufferImageCopy region;
    region.bufferOffset = 0;        // Where the pixel values start
    region.bufferRowLength = 0;     // How the pixels are laid out in memory
    region.bufferImageHeight = 0;   // 0 means there is no padding out 

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    // To which part of the image we want to copy to
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height,1 };

    // The copy command
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    // The 4th param assumes the image is already in the most optimal format for copying

    application.EndSingleTimeCommands(commandBuffer);
}
// =================================================
void ImageBuffer::CreateImageBuffer(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, uint8_t mipLevels, VkSampleCountFlagBits sampleCount)
{
    // To make an image we need an info struct (classic Vulkan stuff)
    VkImageCreateInfo imageInfo;
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = format; // Pixel and Texel formats have to be the same else copying wont work
    imageInfo.tiling = tiling; // Optimal: Texels laid out for best accessing speeds but CANNOT be edited after creation
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // First transition will discard the texels
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;   // Only to be used by one queue family
    imageInfo.usage = usage;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;                 // The max value for UB coordinates
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;                  // Not an array
    imageInfo.samples = sampleCount;            // For multisampling
    imageInfo.flags = 0;                        // Optional - for sparse memory (texture on voxel terrain)

    // Make our image using the info specified
    // Possible for VK_FORMAT_R8G8B8A8_SRGB to not be supported but uncommon
    assert(vkCreateImage(HelloTriangleApplication::Instance().GetDevice(), &imageInfo, nullptr, &m_image) == VK_SUCCESS);

    AllocateBindBuffer<VkImage>(properties, m_image, GetBufferMemory(), vkGetImageMemoryRequirements, vkBindImageMemory);
}
// =================================================
VkCommandBuffer HelloTriangleApplication::BeginSingleTimeCommands()
{
    // Create temporary command buffer for this operation
    // TODO: Make this have it's own command pool with VK_COMMAND_POOL_CREATE_TRANSIENT_BIT enabled
    VkCommandBufferAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    // Immediately start recording to the command buffer
    VkCommandBufferBeginInfo beginInfo;
    beginInfo.pNext = nullptr;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Tells the driver we just want to do this once

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
// =================================================
void HelloTriangleApplication::EndSingleTimeCommands(VkCommandBuffer cmdBuffer)
{
    // And then end it
    vkEndCommandBuffer(cmdBuffer);

    // and execute it to complete the transfer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);
    // There are no events to wait on like draw commands
    // TODO: Could use WaitForFences to schedule multiple transfers, vkQueueWaitIdle has to be done one at a time

    // Free the command buffer now we no longer need it
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmdBuffer);
}
// =================================================
// Name: CreateVertexIndexBuffer
// Desc: Fills out the vertex buffer for binding
// Params: NONE
// Return: NONE
template<typename Type>
void Model::CreateVertexIndexBuffer(std::vector<Type>dataVec, VkBufferUsageFlagBits useFlag, Buffer& buffer)
{
    const VkDeviceSize size = sizeof(dataVec[0]) * dataVec.size();

    // Create the staging buffer to temporarily hold our data on the CPU for reading and writing
    // Holds all the same vertex buffer data as the real deal
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    buffer.CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);
    // Without the 'SRC_BIT and the 'DST_BIT we cannot copy data between them!

    // A pointer to the memory location
    void* data;
    // Assign an area in VRAM and set the pointer to be that location
    vkMapMemory(HelloTriangleApplication::Instance().GetDevice(), stagingMemory, 0, size, 0, &data);
    // Copy the data to that location
    memcpy(data, dataVec.data(), size);
    // And release it to the GPU for processing
    vkUnmapMemory(HelloTriangleApplication::Instance().GetDevice(), stagingMemory);

    // Create the actual vertex buffer using the aptly named function
    buffer.CreateBuffer(size, useFlag | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer.GetBuffer(), buffer.GetBufferMemory());
    // 'LOCAL_BIT disables the use of vkMapMemory

    // So we use our own copy function instead
    // copying the staging buffer to the vertex buffer on the GPU
    buffer.CopyBuffer(stagingBuffer, buffer.GetBuffer(), size);

    // Our staging buffer is no longer needed, so can be cleaned up
    vkDestroyBuffer(HelloTriangleApplication::Instance().GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(HelloTriangleApplication::Instance().GetDevice(), stagingMemory, nullptr);
}
// =================================================
// Name: CreateUniformBuffers
// Desc: Create what will be the uniform buffers
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateUniformBuffers()
{
    VkDeviceSize size = sizeof(UniformBufferObject);

    // Have a buffer for each frame, since values may change from frame to frame
    // And we don't want to have one value being changed while being read
    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    // Make our buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_uniformBuffers[i].CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_uniformBuffers[i].GetBuffer(), m_uniformBuffers[i].GetBufferMemory());
    }
}
// =================================================
// Name: UpdateUniformBuffers
// Desc: Update the uniform buffers with the new information we want
// Params: NONE
// Return: NONE
void HelloTriangleApplication::UpdateUniformBuffers(uint32_t currentImage)
{
    // The time
    static auto startTime = std::chrono::high_resolution_clock::now();
    // Used for moving objects
    // TODO: move this time code somewhere more appropriate
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Our UBO
    UniformBufferObject ubo{};
    // Some transforms
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height), 0.1f, 10.0f);
    // Invert as glm was made for OpenGL
    ubo.proj[1][1] *= -1;

    // TODO: This is inefficient and would be better as a push constant
    void* data;
    vkMapMemory(m_device, m_uniformBuffers[currentImage].GetBufferMemory(), 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_uniformBuffers[currentImage].GetBufferMemory());
}
// =================================================
// Name: CreateDescriptorPool
// Desc: Makes the command buffers for each swap chain image
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes(2);
    
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                      // What our descriptor sets will contain
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // How many of them
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;              // What our descriptor sets will contain
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // How many of them

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());       // How many to create each frame
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);         // The maximum amount that can exist from the pool

    assert(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) == VK_SUCCESS);
}
// =================================================
// Name: CreateDescriptorSets
// Desc: Makes the command buffers for each swap chain image
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;                                // The pool to allocate from
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // How many
    allocInfo.pSetLayouts = layouts.data();                                     // The layout to use

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);  

    assert(vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) == VK_SUCCESS);

    // Loop to populate the descriptors
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = m_uniformBuffers[i].GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_models.data()->GetTexture().GetImageBuffer().GetImageView();
        imageInfo.sampler = m_textureSampler;

        std::vector<VkWriteDescriptorSet> writeDescriptors(2);
        
        writeDescriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[0].dstSet = m_descriptorSets[i];
        writeDescriptors[0].dstBinding = 0;                                      // The binding of the uniform buffer in the shader
        writeDescriptors[0].dstArrayElement = 0;                                 // It's not an array, so just 0
        writeDescriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // Type 
        writeDescriptors[0].descriptorCount = 1;                                 // How many
        writeDescriptors[0].pBufferInfo = &bufferInfo;
        writeDescriptors[0].pImageInfo = nullptr;           // Optional - for sampling 
        writeDescriptors[0].pTexelBufferView = nullptr;     // Optional - buffer views
      
        writeDescriptors[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[1].dstSet = m_descriptorSets[i];
        writeDescriptors[1].dstBinding = 1;                                              // The binding of the uniform buffer in the shader
        writeDescriptors[1].dstArrayElement = 0;                                         // It's not an array, so just 0
        writeDescriptors[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;  // Type 
        writeDescriptors[1].descriptorCount = 1;                                         // How many
        writeDescriptors[1].pBufferInfo = nullptr;
        writeDescriptors[1].pImageInfo = &imageInfo;           // for sampling 
        writeDescriptors[1].pTexelBufferView = nullptr;

        // Apply the descriptor set
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(),
            0, nullptr);
    }
}
// =================================================
// Name: CreateCommandBuffers
// Desc: Makes the command buffers for each swap chain image
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateCommandBuffers()
{
    m_commandBuffers.resize(m_swapChainFrameBuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Specifies if the allocated command buffers are primary or secondary
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
	// VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called from primary command buffers.

    assert(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) == VK_SUCCESS);

}
// =================================================
// Name: RecordCommandBuffer
// Desc: The draw commands to execute each frame
// Params: buffer, imageidx
// Return: NONE
void HelloTriangleApplication::RecordCommandBuffer(VkCommandBuffer buffer, uint32_t imageidx)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional   - how we will use it
    beginInfo.pInheritanceInfo = nullptr; // Optional   - which primary buffer to inherit from

    assert(vkBeginCommandBuffer(buffer, &beginInfo) == VK_SUCCESS);
    // Will end any other command buffer being recorded when called

    // Drawing starts by beginning the render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFrameBuffers[imageidx];

    // Define the size of the render area
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    // Define the clear colour of our frame
    std::vector<VkClearValue> clearValues(2);                            // Order of clear colours have to match attachments
    clearValues[0].color = {{0.0f, 0.0f, 1.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};                    // 1 is the furthest away and 0 the closest
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // The render pass has now begun
    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // Inline doesn't call secondary command buffers

    // Bind the graphics pipeline
    // The second parameter specifies the pipeline type
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    //VkBuffer vertexBuffers[] = { m_vertexBuffer }; // We only have one buffer
    std::vector<VkDeviceSize> offsets;              // And offset
	offsets.emplace_back(0);
    uint32_t maxIndices = m_models[0].GetVertexArray().size();

    for (uint32_t i = 1; i < m_models.size(); ++i)
    {
        offsets.emplace_back(offsets[i - 1] + sizeof(m_models[i].GetVertexBuffer()));
        maxIndices += m_models[i].GetVertexArray().size();

    }

    // Bind our vertex buffers to the bindings specified
    vkCmdBindVertexBuffers(buffer, 0, 1, &m_models.data()->GetVertexBuffer().GetBuffer(), offsets.data());

    vkCmdBindIndexBuffer(buffer, m_models.data()->GetIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);

    // All that remains is to tell it to draw the triangle

    vkCmdDrawIndexed(buffer, maxIndices, 1, 0, 0, 0);
    // vkCmdDraw()
    // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
    // instanceCount: Used for instanced rendering, use 1 if you're not doing that.
    // firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
    // firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.

    // Now end the render pass
    vkCmdEndRenderPass(buffer);

    // And finish recording the buffer
    assert(vkEndCommandBuffer(buffer) == VK_SUCCESS);
}
// =================================================
// Name: CheckDeviceExtensionSupport
// Desc: Check the device's swap chain we found works with our surface
// Params: device
// Return: SwapChainSupportDetails
SwapChainSupportDetails HelloTriangleApplication::QuerySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    // Check basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    // Querying the supported surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    // querying the supported presentation modes works exactly the same way as our surface
    if (presentModeCount != 0) {
        details.present_modes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.present_modes.data());
    }

    return details;
}
// =================================================
// Name: ChooseSwapSurfaceFormat
// Desc: Choose the properties for our swap chain
// Params: available_formats
// Return: VkSurfaceFormatKHR
VkSurfaceFormatKHR HelloTriangleApplication::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    // Go through the list and see if the preferred combination is available
    for (const auto& availableFormat : available_formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return available_formats[0];
}
// =================================================
// Name: ChooseSwapPresentMode
// Desc: Choose the swap mode between the different frames
// Params: available_present_modes
// Return: VkSurfaceFormatKHR
VkPresentModeKHR HelloTriangleApplication::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for (const auto& availablePresentMode : available_present_modes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
// =================================================
// Name: ChooseSwapExtent
// Desc: Get the extents of the swap chain frames
// Params: capabilities
// Return: VkExtent2D
VkExtent2D HelloTriangleApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
   
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    // Bound the values of width and height between the allowed minimum and maximum extents
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
// =================================================
// Name: CreateImageViews
// Desc: Creates a basic image view for the passed in image with the passed format
// Params: image, format
// Return: VkImageView
void ImageBuffer::CreateImageViews(VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = m_image;
    
    // How the image data should be interpreted
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    
    // For swizzling colours, which we don't want to do
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    // Describes the image's purpose, and what we need to access
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = m_mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    const auto& device = HelloTriangleApplication::Instance();

    assert(vkCreateImageView(HelloTriangleApplication::Instance().GetDevice(), &createInfo, nullptr, &m_imageView) == VK_SUCCESS);
    
}
// =================================================
// Name: CreateLogicalDevice
// Desc: Set up a logical device to interface with the physical device we chose
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateLogicalDevice()
{
    // Creation of a logical device involves specifying a bunch of details in structs again
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphics_family.value(), indices.present_family.value() };

    // Assign priorities to queues to influence the scheduling of command buffer execution
    float queuePriority = 1.0f;                                 // Numbers should be between 0 and 1
    // This structure describes the number of queues we want for a single queue family
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;      // This is required even with one queue
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // ------ NOTE ------
	// You can only create a small number of queues for each queue family
	// ------------------

    // Specify is the set of device features that we'll be using (The ones we queried)
    VkPhysicalDeviceFeatures deviceFeatures{};  // Leave everything false for now
    deviceFeatures.samplerAnisotropy = m_AnisotropyEnabled;
    deviceFeatures.sampleRateShading = VK_TRUE;

    // Start filling in the main VkDeviceCreateInfo structure
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // Pointers to the queue creation info and device features struct
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // VkInstanceCreateInfo struct and requires you to specify extensions and validation layers
    // (The difference is that these are device specific this time)
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

    // This step is no longer required but can be added for backwards compatability
    // ---------------
    if (m_enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }
    // ---------------

    // Instantiate the logical device, and check it was successful
    assert(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) == VK_SUCCESS);

    // Retrieve queue handles for each queue family
    // We only have one queue so we'll just use 0
    vkGetDeviceQueue(m_device, indices.graphics_family.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.present_family.value(), 0, &m_presentQueue);
}
//==================================================================================================
//  Debug Functions
//==================================================================================================
// =================================================
// Name: CheckValidationLayerSupport
// Desc: Check how many debug validation layers
// Params: NONE
// Return: bool
bool HelloTriangleApplication::CheckValidationLayerSupport() const
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : VALIDATION_LAYERS) {
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
// =================================================
// Name: GetRequiredExtensions
// Desc: Get the required extensions for debugging layers
// Params: NONE
// Return: const char vector
std::vector<const char*> HelloTriangleApplication::GetRequiredExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (m_enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
// =================================================
// Name: DebugCallback
// Desc: Ouputs messages from the debug layer
// Params: message_severity, message_type, p_callback_data, p_user_data
// Return: VKAPI_ATTR VkBool32 VKAPI_CALL
VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,            // The messages severity level from minor to critical
    VkDebugUtilsMessageTypeFlagsEXT message_type,                       // If the error is related to Vulkan or not
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,        // Struct containing the details of the message itself
    void* p_user_data) {                                                // Allows passing of info to the callback

    // Message is important enough to show
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;
    }    

    return VK_FALSE;
}
// =================================================
// Name: PopulateDebugMessengerCreateInfo
// Desc: Fill out a VkDebugUtilsMessengerCreateInfoEXT struct
// Params: createInfo
// Return: NONE
void HelloTriangleApplication::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;
}
// =================================================
// Name: PopulateDebugMessengerCreateInfo
// Desc: Fill out a VkDebugUtilsMessengerCreateInfoEXT struct
// Params: createInfo
// Return: VkResult
VkResult HelloTriangleApplication::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
// =================================================
// Name: SetupDebugMessenger
// Desc: Sets up the debug messenger
// Params: NONE
// Return: NONE
void HelloTriangleApplication::SetupDebugMessenger() {
    if (!m_enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    assert(CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) == VK_SUCCESS);
}
//==================================================================================================
//  Graphics Pipeline
//==================================================================================================
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    // ate: Start reading at the end of the file
	// binary: Read the file as binary file(avoid text transformations)

    assert(file.is_open()); // Make sure we opened the file

    // We start from the back so we can allocate a buffer from the starting read position
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    // Go back to front reading all the data
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();   // Close the file

    // Return all the data we extracted
    return buffer;
}
void HelloTriangleApplication::CreateDescriptorSetLayouts()
{
    VkDescriptorSetLayoutBinding uboLayout;
    uboLayout.binding = 0;                                          // Binding used in the shader
    uboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;              // And which shader it is
    uboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;   // What we want to bind
    uboLayout.descriptorCount = 1;                                  // And how many of that thing we want to bind
    uboLayout.pImmutableSamplers = nullptr;                         // For image sampling (we don't want in this case)

    VkDescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    const std::vector<VkDescriptorSetLayoutBinding> bindings = { uboLayout, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    assert(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) == VK_SUCCESS);

}
// =================================================
// Name: CreateGraphicsPipeline
// Desc: Sets up the grapics pipeline
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateGraphicsPipeline()
{
// index/          [FIXED]     |----------[PROGRAMMABLE]----------|      [FIXED]    [PROGRAMMABLE]  [FIXED]
// vertex  |_____   input   ___ vertex ___ tesselation ___ geometry ___ rasteriser ___ fragment ___ colour  _____| frame
// buffer  |      assembler     shader        stage         shader                      shader      binding      | buffer
	
//  input assembler - collects    tessellation - subdivides        geometry shader - Adds   rasteriser - discretizes
//  the raw vertex data.          faces to improve mesh quality    or removes primitives    the primitives into fragments

//  fragment shader - decides     colour blending - blends the
//  which frags are written,      colours of fragments mapped
//  their colour and depth        to the same pixel

// Shader code in Vulkan has to be specified in a bytecode format as opposed to hlsl / glsl
// This bytecode format is called SPIR-V (we can however use glslc.exe to code and then compile into SPIR-V)

    // Load our shaders
    std::vector<char> m_vertshaderCode = readFile("Vertex_Shader.spv");
    std::vector<char> fragShaderCode = readFile("Frag_Shader.spv");

    // Create them from the loaded data
    VkShaderModule m_vertshaderModule = CreateShaderModule(m_vertshaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    // To use the shaders assign them to a specific pipeline stage
    VkPipelineShaderStageCreateInfo m_vertshaderStageInfo{};
    m_vertshaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_vertshaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    // Which pipeline stage the shader is going to be used
    m_vertshaderStageInfo.module = m_vertshaderModule;
    m_vertshaderStageInfo.pName = "main";
    // pSpecializationInfo is an optional member used for shader constants

    // Do the same for the fragment shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // Make an array containing our shader stages
    VkPipelineShaderStageCreateInfo shaderStages[] = { m_vertshaderStageInfo, fragShaderStageInfo };

    // -=-=-=-=-=-=-=-=-=- VERTEX INPUT AND ASSEMBLY -=-=-=-=-=-=-=-=-=-

    VkVertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::GetAttributeDescriptions();

    // Describes the format of the vertex data that will be passed to the vertex shader
    // We'll leave it for now 
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;    // Bindings: spacing between data and whether the data is per-vertex or per-instance
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size(); // Attribute descriptions: type of the attributes passed to the vertex shader, 
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();    // which binding to load them from and at which offset
    // Descriptions point to an array of structs that describe the aforementioned details for loading vertex data

    // Describes the kind of geometry and the topology
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;    // Setting to true allows the reuse of m_verts

    // -=-=-=-=-=-=-=-=-=- VIEWPORTS AND SCISSORS -=-=-=-=-=-=-=-=-=-

    // Viewport describes the region of the framebuffer that the output will be rendered to
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;   // Values should be within 0 and 1
    viewport.maxDepth = 1.0f;

    // The scissor clips part of viewport
    // In our case we don't want to clip any of it
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapChainExtent;

    // Combine the viewport and scissor to one struct
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;    // It's possible to use multiple on some GPUs
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // -=-=-=-=-=-=-=-=-=- RASTERISER -=-=-=-=-=-=-=-=-=-

    // Configure the fixed rasteriser stage
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // When set to true values beyond a near and far clip plane are removed

    // When this is set to true all values are discarded
    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    // The polygonMode determines how fragments are generated for geometry
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

    // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
	// VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
	// VK_POLYGON_MODE_POINT : polygon vertices are drawn as points
    // (Using the last two requires a GPU feature)

    // Describes the thickness of lines in terms of number of fragments
    rasterizer.lineWidth = 1.0f;    // Lines larger than 1 require a GPU feature

    // Determines the type of face culling to use
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;             // Which faces to cull: front, back or none
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // Describes which faces are considered front

    // Can be used for altering depth
    rasterizer.depthBiasEnable = VK_FALSE;  // We don't want to
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // -=-=-=-=-=-=-=-=-=- MULTISAMPLING -=-=-=-=-=-=-=-=-=-

    // This struct configures multisampling. Used for anti-aliasing
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;        // Enables shading in the pipeline
    multisampling.rasterizationSamples = m_msaaSamples;
    multisampling.minSampleShading = .2f; // Optional - The minimum fraction for sample shading (closer to 1 is smoother)
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // -=-=-=-=-=-=-=-=-=- DEPTH AND STENCIL -=-=-=-=-=-=-=-=-=-

    // Describe depth and stencil testing
    // This one does nothing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;             // Should fragments be depth tested
    depthStencil.depthWriteEnable = VK_TRUE;            // Write to the fragments that survive the depth test their depth
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;   // Lower equals closer so keep the ones that are less
    depthStencil.depthBoundsTestEnable = VK_FALSE;      // For custom depth bounds tests
    depthStencil.minDepthBounds = 0.5f;                 // Example values
	depthStencil.maxDepthBounds = 0.8f;                 // Example values
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    // -=-=-=-=-=-=-=-=-=- COLOUR BLENDING -=-=-=-=-=-=-=-=-=-

    // contains the configuration per attached framebuffer
    // VkPipelineColorBlendStateCreateInfo contains the global colour blending settings
    // This one has alpha blending enabled
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_FALSE;  // If set to false the output colour is unmodified
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    // References the array of structures for all of the framebuffers and allows you to set blend constants
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // -=-=-=-=-=-=-=-=-=- DYNAMIC STATE -=-=-=-=-=-=-=-=-=-

    // Used for specifing parts of the pipeline we can change at runtime
    // This step is optional
    VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // -=-=-=-=-=-=-=-=-=- PIPELINE SETUP -=-=-=-=-=-=-=-=-=-

    // The structure also specifies push constants, which are another way of passing dynamic values to shaders
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; 
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout; 
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    // -=-=-=-=-=-=-=-=-=- PIPELINE SETUP -=-=-=-=-=-=-=-=-=-

    assert(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) == VK_SUCCESS);

    // Now create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    // Fill out all the stages we defined
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional

    // Describes the layout
    pipelineInfo.layout = m_pipelineLayout;

    // Reference the render pass and the index of the sub pass where this graphics pipeline will be used
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    // Vulkan allows to make subpipelines from this one
    // We don't want to
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    // Make it
    assert(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) == VK_SUCCESS);

    // -=-=-=-=-=-=-=-=-=- CLEANUP -=-=-=-=-=-=-=-=-=-

    // Cleanup should then happen at the end of the function
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, m_vertshaderModule, nullptr);
}
// =================================================
// Name: CreateShaderModule
// Desc: Create a Vulkan shader object from the file data we read
// Params: code
// Return: VkShaderModule
VkShaderModule HelloTriangleApplication::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());  // Cast the char to a uint32_t

    VkShaderModule shaderModule;

    // The nullptr is an optional pointer to custom allocators
    assert(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS);

    return shaderModule;
}

void HelloTriangleApplication::CreateRenderPass()
{
    // Single color buffer attachment represented by one of the images from the swap chain
    VkAttachmentDescription colourAttachment{};
    colourAttachment.format = m_swapChainImageFormat;    // Format should match the format of the swap images
    colourAttachment.samples = m_msaaSamples;
    // What to do with data before and after rendering
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // We don't use the stencil buffer so we don't care
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Specifies the data format before and after rendering passes
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;    // Sampled images cannot be presented directly
    // Subpass requires a reference to colour attachment
    VkAttachmentReference colourAttachmentRef{};
    colourAttachmentRef.attachment = 0;
    colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Resolve attachment to convert the sampled image to a presentable one
    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = m_swapChainImageFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference resolveReference{};
    resolveReference.attachment = 2;
    resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment doesn't need one as it will never be presented
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);   // Needs to match the format of the depth buffer
    depthAttachment.samples = m_msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't need it after draw calls so discard it
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // Not loading previous depth buffers so undefined
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass description
    // Define one subpass (subpasses can be used for post processing effects)
    // We just want one
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1; // The index of the attachment in this array is directly referenced from the fragment shader
    subpass.pColorAttachments = &colourAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // Can only have one depth buffer test
    subpass.pResolveAttachments = &resolveReference;

    // -=-=-=-=-=-=-=-=-=- RENDER PASS -=-=-=-=-=-=-=-=-=-

    // Dependencies between subpasses
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachDescs = { colourAttachment, depthAttachment, resolveAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachDescs.size());
    renderPassInfo.pAttachments = attachDescs.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    assert(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS);
}
// =================================================
// Name: CreateRenderTargets
// Desc: Generate an image used for multisampling
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CreateRenderTargets()
{
    VkFormat renderTargetFormat = m_swapChainImageFormat;

    // Mip level has to be 1. Enforced by Vulkan
    m_renderTargetImageBuffer.CreateImageBuffer(m_swapChainExtent.width, m_swapChainExtent.height, renderTargetFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, m_msaaSamples);

    m_renderTargetImageBuffer.CreateImageViews(renderTargetFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}
//==================================================================================================
//  Rendering
//==================================================================================================

void HelloTriangleApplication::CreateSyncObjects()
{
    // Assign the size we need
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChainImageBuffers.size(), VK_NULL_HANDLE);

    // Fill out semaphore struct
    // Only sType needs a value
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fill out fence struct
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Make a semaphore for each frame
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        assert(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) == VK_SUCCESS &&
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) == VK_SUCCESS &&
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) == VK_SUCCESS);
    }
}
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>








// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//==================================================================================================
//  MainLoop
//==================================================================================================
// Name: MainLoop
// Desc: The main loop, calling all the render and update needed each cycle
// Params: NONE
// Return: NONE
void HelloTriangleApplication::MainLoop()
{
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        DrawFrame();
    }

    // Wait for everything to finish before destroying
    vkDeviceWaitIdle(m_device);
}

void HelloTriangleApplication::DrawFrame()
{
    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
	vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    // Get the image from the swap chain we want to draw
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    // If the window no longer matches the swap chain recreate it
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
	    return;                 // and exit as we cannot proceed
    }
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    UpdateUniformBuffers(m_currentFrame);

    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
    RecordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

    // Queue submission and synchronization
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // Which buffers to execute
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    // Which semaphore to signal when execution is finished
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);

    assert(result == VK_SUCCESS);

    // Submit the results back to the swap chain
    // Done through the VkPresentInfoKHR struct
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Which semaphores to wait on
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    // The swap chains to present the image to
    VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    // Ask to present an image to the swap chain
    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    // Check if suboptimal as well for best effect
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        RecreateSwapChain();

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    // (%) operator index loops around after max

    // We could use vkQueueWaitIdle to wait here, but we use "frames in flight"
}
// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>








// <><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//==================================================================================================
//  CleanUp
//==================================================================================================
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
// =================================================

void HelloTriangleApplication::CleanUpSwapChain()
{
    
    for (auto framebuffer : m_swapChainFrameBuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}
// =================================================
// Name: CleanUp
// Desc: The terminator, cleaning up all memory and such we allocated
// Params: NONE
// Return: NONE
void HelloTriangleApplication::CleanUp()
{
    CleanUpSwapChain();

    vkDestroySampler(m_device, m_textureSampler, nullptr);

    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    // Sets cleaned up implicitly 

    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(m_device, m_uniformBuffers[i].GetBuffer(), nullptr);
        vkFreeMemory(m_device, m_uniformBuffers[i].GetBufferMemory(), nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    vkDestroyDevice(m_device, nullptr);

    if (m_enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);

    glfwTerminate();

    delete m_appPointer;
    m_appPointer = nullptr;
}
