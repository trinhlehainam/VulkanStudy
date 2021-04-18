#include "VkApplication.h"

#include <iostream>
#include <cstdint>
#include <set>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ONE_TO_ZERO
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "VkUtils.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageServerities,
	VkDebugUtilsMessageTypeFlagsEXT messageFlags,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if(messageServerities > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "\nVULKAN DEBUG MESSAGE : " << pCallbackData->pMessage << "\n" << std::endl;
	else
		std::cerr << "VULKAN DEBUG MESSAGE : " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

namespace
{
	VkResult CreateVkDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugUtils);

	void DestroyVkDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
		const VkAllocationCallbacks* pAllocator);

	void PopulateVkDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	bool CheckVkInstanceExtensionsSupport(const std::vector<const char*>& requiredExtensions);

	bool CheckVkValidationLayersSupport(const std::vector<const char*>& requiredLayers);

	bool CheckVkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredDeviceExtensions);

	bool CheckVkPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkUtils::QueueFamilyIndices GetQueueFamiilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkUtils::SwapChainDetails CheckSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
}

namespace
{
	VkResult CreateVkDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyVkDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, messenger, pAllocator);
	}

	void PopulateVkDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VkDebugCallback;
	}

	bool CheckVkInstanceExtensionsSupport(const std::vector<const char*>& requiredExtensions)
	{
		uint32_t extensionsCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

		if (extensionsCount == 0)
			return false;

		std::vector<VkExtensionProperties> vkSupportExtensions(extensionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, vkSupportExtensions.data());

#ifdef _DEBUG || DEBUG
		std::cout << "\n" << requiredExtensions.size() << " required instance extensions \n";
		std::cout << "REQUIRED INSTANCE EXTENSIONS : \n";
		for (const auto& requiredExtension : requiredExtensions)
			std::cout << "\t" << requiredExtension << "\n";

		std::cout << "\n" << extensionsCount << " instance extensions are supported by Vulkan \n";
		std::cout << "INSTANCE EXTENSIONS : \n";
		for (const auto& supportExtension : vkSupportExtensions)
			std::cout << "\t" << supportExtension.extensionName << "\n";
#endif

		for (const auto& requiredExtension : requiredExtensions)
		{
			bool isSupported = false;
			for (const auto& supportExtensions : vkSupportExtensions)
			{
				if (strcmp(requiredExtension, supportExtensions.extensionName))
				{
					isSupported = true;
					break;
				}
			}

			if (!isSupported)
				return false;
		}
		return true;
	}

	bool CheckVkValidationLayersSupport(const std::vector<const char*>& requiredLayers)
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> supportLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, supportLayers.data());

#ifdef _DEBUG || DEBUG
		std::cout << "\n" << layerCount << " validations layers supported by Vulkan \n";
		std::cout << "SUPPORTED VALIDATION LAYERS : \n";
		for (const auto& layer : supportLayers)
			std::cout << "\t" << layer.layerName << "\n";
		std::cout << "\n";
#endif

		for (const auto& requiredLayer : requiredLayers)
		{
			bool isSupported = false;
			for (const auto& supportLayer : supportLayers)
			{
				if (strcmp(requiredLayer, supportLayer.layerName))
				{
					isSupported = true;
					break;
				}
			}

			if (!isSupported)
				return false;
		}

		return true;
	}

	bool CheckVkDeviceExtensionsSupport(VkPhysicalDevice device, const std::vector<const char*>& requiredDeviceExtensions)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		if (extensionCount == 0)
			return false;

		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

#ifdef _DEBUG || DEBUG
		std::cout << "\nSUPPORTED DEVICE EXTENSIONS :\n";
		for (const auto& extension : extensions)
			std::cout << "\t" << extension.extensionName << "\n";
		std::cout << "\n";
#endif

		for (const auto& requiredExtension : requiredDeviceExtensions)
		{
			bool isSupported = false;
			for (const auto& extension : extensions)
			{
				if (strcmp(requiredExtension, extension.extensionName))
				{
					isSupported = true;
					break;
				}
			}

			if (!isSupported)
				return false;
		}

		return true;
	}


	bool CheckVkPhysicalDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		// Information about device itself
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(device, &deviceProps);

		// Information about device can do
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

#ifdef _DEBUG || DEBUG
		std::cout << "\nVULKAN SUPPORTED DEVICE : " << deviceProps.deviceName << "\n";
		std::cout << "\n";
#endif

		auto swapChainDetails = CheckSwapChainDetails(device, surface);
		bool isSwapchainvalid = !swapChainDetails.Formats.empty() && !swapChainDetails.PresentModes.empty();

		return GetQueueFamiilyIndices(device, surface).IsValid() && 
			CheckVkDeviceExtensionsSupport(device, VkUtils::DEVICE_EXTENSIONS) &&
			isSwapchainvalid;
	}

	VkUtils::QueueFamilyIndices GetQueueFamiilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		VkUtils::QueueFamilyIndices indices;

		uint32_t queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies.data());

		// Check queue family's required features are valid
		int index = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = index;

			VkBool32 presentationSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentationSupported);

			if (queueFamily.queueCount > 0 && presentationSupported)
				indices.presentationFamily = index;

			if (indices.IsValid())
				break;

			++index;
		}

		return indices;
	}

	VkUtils::SwapChainDetails CheckSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		VkUtils::SwapChainDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
		}
			
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
		}
			
		return details;
	}
}

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

	m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
}

void VkApplication::InitVulkan()
{
	CreateVkInstance();
	SetUpVkDebugMessengerEXT();
	CreateVkSurface();
	PickVkPhysicalDevice();
	CreateVkLogicalDevice();
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
		DestroyVkDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void VkApplication::CreateVkInstance()
{
	// Information about application
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_title.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// Struct carries info to create VkInstance
	VkInstanceCreateInfo vkCreateInfo = {};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.pApplicationInfo = &appInfo;

	auto requiredExtensions = GetRequiredInstanceExtensions();

	if (!CheckVkInstanceExtensionsSupport(requiredExtensions))
		throw std::runtime_error("\nVULKAN INIT ERROR : some Vulkan Instance Extensions are not supported !\n");

	vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	vkCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (m_enableValidationLayer && !CheckVkValidationLayersSupport(m_validationLayers))
		throw std::runtime_error("\nVULKAN INIT ERROR : vallidation layers required , but not found !\n");

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	if (m_enableValidationLayer)
	{
		vkCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		vkCreateInfo.ppEnabledLayerNames = m_validationLayers.data();

		PopulateVkDebugMessengerCreateInfo(debugCreateInfo);
		vkCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
		vkCreateInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&vkCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : falied to create Vulkan instance!\n");
}

void VkApplication::CreateVkLogicalDevice()
{
	auto indices = GetQueueFamiilyIndices(m_mainDevice.physDevice, m_surface);

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

void VkApplication::CreateVkSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create VkSurface !\n");
}

void VkApplication::SetUpVkDebugMessengerEXT()
{
	if (!m_enableValidationLayer) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	PopulateVkDebugMessengerCreateInfo(createInfo);

	if (CreateVkDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
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
		if (CheckVkPhysicalDeviceSuitable(device, m_surface))
		{
			m_mainDevice.physDevice = device;
			break;
		}	
	}

	if (m_mainDevice.physDevice == nullptr)
		throw std::runtime_error("\nVULKAN INIT ERROR : Can't find suitable physical devices !\n");
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
