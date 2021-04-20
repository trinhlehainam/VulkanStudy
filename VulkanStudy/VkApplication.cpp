#include "VkApplication.h"

#include <iostream>
#include <cstdint>
#include <set>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ONE_TO_ZERO
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "VkUtils.h"

VkApplication::VkApplication(int width, int height, const char* window_title):
	m_width(width),m_height(height),m_title(window_title)
{
#ifdef _DEBUG || DEBUG
	m_enableValidationLayer = true;
#else
	m_enableValidationLayer = false;
#endif
}

void VkApplication::Run()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	CleanUp();
}

void VkApplication::InitWindow()
{
	glfwInit();

	// Disable OpenGL API
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
}

void VkApplication::InitVulkan()
{
	CreateInstance();
	SetUpVkDebugMessengerEXT();
	CreateSurface();
	PickVkPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapchain();
	CreateImageViews();
	CreateGraphicsPipeline();
}

void VkApplication::MainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
	}
}

void VkApplication::CleanUp()
{
	if (m_enableValidationLayer)
		VkUtils::DestroyVkDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	for (auto& imageView : m_imageViews)
		vkDestroyImageView(m_mainDevice.logicalDevice, imageView, nullptr);
	vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapchain, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void VkApplication::CreateInstance()
{
	// Information about application
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// Struct carries info to create VkInstance
	VkInstanceCreateInfo vkCreateInfo = {};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.pApplicationInfo = &appInfo;

	auto requiredExtensions = GetRequiredInstanceExtensions();

	if (!VkUtils::CheckVkInstanceExtensionsSupport(requiredExtensions))
		throw std::runtime_error("\nVULKAN INIT ERROR : some Vulkan Instance Extensions are not supported !\n");

	vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	vkCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (m_enableValidationLayer && !VkUtils::CheckVkValidationLayersSupport(VkUtils::VALIDATION_LAYERS))
		throw std::runtime_error("\nVULKAN INIT ERROR : vallidation layers required , but not found !\n");

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	if (m_enableValidationLayer)
	{
		vkCreateInfo.enabledLayerCount = static_cast<uint32_t>(VkUtils::VALIDATION_LAYERS.size());
		vkCreateInfo.ppEnabledLayerNames = VkUtils::VALIDATION_LAYERS.data();

		VkUtils::PopulateVkDebugMessengerCreateInfo(debugCreateInfo);
		vkCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
		vkCreateInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&vkCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : falied to create Vulkan instance!\n");
}

void VkApplication::CreateLogicalDevice()
{
	auto indices = VkUtils::GetQueueFamiilyIndices(m_mainDevice.physDevice, m_surface);

	// std::set only allow one object to hold one specific value
	// Use std::set to check if presentation queue is inside graphics queue or in the seperate queue
	// If presentation queue is inside graphics queue -> only create one queue
	// Else create the seperate queue for presentation queue
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	std::vector <VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(queueFamilyIndices.size());

	// Vullkan need to know how to handle multiple queue, so set priority to show Vulkan
	float priority = 1.0f;
	for (auto queueIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(VkUtils::DEVICE_EXTENSIONS.size());
	createInfo.ppEnabledExtensionNames = VkUtils::DEVICE_EXTENSIONS.data();

	VkPhysicalDeviceFeatures features = {};
	createInfo.pEnabledFeatures = &features;

	if (vkCreateDevice(m_mainDevice.physDevice, &createInfo, nullptr, &m_mainDevice.logicalDevice) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create logical devices !\n");

	// Get Queue that created inside logical device to use later
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.presentationFamily, 0, &m_presentationQueue);
}

void VkApplication::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create VkSurface !\n");
}

void VkApplication::CreateSwapchain()
{
	auto details = VkUtils::CheckSwapChainDetails(m_mainDevice.physDevice, m_surface);

	auto extent = PickVkSwapchainImageExtent(details.Capabilities);
	auto presentMode = PickVkPresentModes(details.PresentModes);
	auto format = PickVkSurfaceFormats(details.Formats);

	// Provide one more image to keep GPU busy
	auto imageCount = details.Capabilities.minImageCount + 1;

	// If maxImageCount = 0, it means there is no limit
	if (details.Capabilities.maxImageCount > 0 && imageCount > details.Capabilities.maxImageCount)
		imageCount = details.Capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.minImageCount = imageCount;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = VkUtils::GetQueueFamiilyIndices(m_mainDevice.physDevice, m_surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentationFamily };

	if (indices.graphicsFamily != indices.presentationFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = details.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;

	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_mainDevice.logicalDevice, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create swapchain !\n");

	// When swapchain is created, it also created images in it
	// Retrieve these images to do rendering operation
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &imageCount, m_swapchainImages.data());

	m_swapchainFormat = format.format;
	m_swapchainExtent = extent;
}

void VkApplication::CreateImageViews()
{
	m_imageViews.resize(m_swapchainImages.size());

	for (int i = 0; i < m_swapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.format = m_swapchainFormat;
		createInfo.image = m_swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.levelCount = 1;

		if (vkCreateImageView(m_mainDevice.logicalDevice, &createInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("\nVUKAN INIT ERROR : Failed to create Image Views !\n");
	}
}

void VkApplication::CreateGraphicsPipeline()
{
	VkShaderModule vertShaderModule = VkUtils::CreateShaderModule(m_mainDevice.logicalDevice, nullptr, "assets/shaders/vert.sp");
	VkShaderModule fragShaderModule = VkUtils::CreateShaderModule(m_mainDevice.logicalDevice, nullptr, "assets/shaders/frag.spv");

	VkPipelineShaderStageCreateInfo vertStageCreateInfo = {};
	vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageCreateInfo.module = vertShaderModule;
	vertStageCreateInfo.pName = "main";											// main of shader's start up function

	VkPipelineShaderStageCreateInfo fragStageCreateInfo = {};
	vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vertStageCreateInfo.module = fragShaderModule;
	vertStageCreateInfo.pName = "main";											// main of shader's start up function

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = { vertStageCreateInfo , vertStageCreateInfo };

	vkDestroyShaderModule(m_mainDevice.logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_mainDevice.logicalDevice, fragShaderModule, nullptr);
}

void VkApplication::SetUpVkDebugMessengerEXT()
{
	if (!m_enableValidationLayer) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	VkUtils::PopulateVkDebugMessengerCreateInfo(createInfo);

	if (VkUtils::CreateVkDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create debug utils messenger extension !\n");
}

void VkApplication::PickVkPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("\nVUKAN INIT ERROR : Can't find physical devices (GPUs) supported by Vulkan Instance !\n");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (VkUtils::CheckVkPhysicalDeviceSuitable(device, m_surface))
		{
			m_mainDevice.physDevice = device;
			break;
		}	
	}

	if (m_mainDevice.physDevice == nullptr)
		throw std::runtime_error("\nVULKAN INIT ERROR : Can't find suitable physical devices !\n");
}

VkSurfaceFormatKHR VkApplication::PickVkSurfaceFormats(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_R8G8B8A8_UNORM ,VK_COLORSPACE_SRGB_NONLINEAR_KHR };

	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_SRGB) &&
			format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VkApplication::PickVkPresentModes(const std::vector<VkPresentModeKHR>& presentModes)
{
	for (const auto& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return presentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VkApplication::PickVkSwapchainImageExtent(const VkSurfaceCapabilitiesKHR& capability)
{
	// If system return max value mean extent can be varied or changed
	if (capability.currentExtent.width != UINT32_MAX)
		return capability.currentExtent;
	else
	{
		int width, height;
		// Request window's size from system
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width),static_cast<uint32_t>(height) };
		actualExtent.width = std::max(capability.minImageExtent.width, std::min(capability.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capability.minImageExtent.height, std::min(capability.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

std::vector<const char*> VkApplication::GetRequiredInstanceExtensions()
{
	// Get GLFW required instance extensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// Add debug extention
	if (m_enableValidationLayer)
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return requiredExtensions;
}
