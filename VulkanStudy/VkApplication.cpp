#include "VkApplication.h"

#include <iostream>
#include <cstdint>

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
	std::cerr << " \nVULKAN DEBUG MESSAGE : " << pCallbackData->pMessage << std::endl;

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

	bool CheckVkPhysicalDeviceSuitable(VkPhysicalDevice device);

	VkUtils::QueueFamilyIndices GetQueueFamiilyIndices(VkPhysicalDevice);
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
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT || VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			|| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT || VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ||
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VkDebugCallback;
	}

	bool CheckVkInstanceExtensionsSupport(const std::vector<const char*>& requiredExtensions)
	{
		uint32_t extensionsCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);

		std::vector<VkExtensionProperties> vkSupportExtensions(extensionsCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, vkSupportExtensions.data());

#ifdef _DEBUG || DEBUG
		std::cout << "\n" << requiredExtensions.size() << " required instance extensions \n";
		std::cout << "Required Instance Extensions Supported By Vulkan : \n";
		for (const auto& requiredExtension : requiredExtensions)
			std::cout << "\t" << requiredExtension << "\n";

		std::cout << "\n" << extensionsCount << " instance extensions are supported by Vulkan \n";
		std::cout << "Instance Extensions Supported By Vulkan : \n";
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

	bool CheckVkPhysicalDeviceSuitable(VkPhysicalDevice device)
	{
		// Information about device itself
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties(device, &deviceProps);

		// Information about device can do
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

#ifdef _DEBUG || DEBUG
		std::cout << "\nVULKAN SUPPORTED DEVICE : " << deviceProps.deviceName;
#endif

		return GetQueueFamiilyIndices(device).IsValid();
	}

	VkUtils::QueueFamilyIndices GetQueueFamiilyIndices(VkPhysicalDevice device)
	{
		VkUtils::QueueFamilyIndices indices;

		uint32_t queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

		std::vector<VkQueueFamilyProperties> queues(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queues.data());

		// Check queue family's required features are valid
		int index = 0;
		for (const auto& queueFamily : queues)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = index;
			if (indices.IsValid())
				break;
			++index;
		}

		return indices;
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
	GetVkPhysicalDevice();
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
		DestroyVkDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
	vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
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
		throw std::runtime_error("VULKAN INIT ERROR : some Vulkan Instance Extensions are not supported !");

	vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	vkCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (m_enableValidationLayer && !CheckVkValidationLayersSupport())
		throw std::runtime_error("VULKAN INIT ERROR : vallidation layers required , but not found !");

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

	if (vkCreateInstance(&vkCreateInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
		throw std::runtime_error("VULKAN INIT ERROR : falied to create Vulkan instance!");
}

void VkApplication::CreateVkLogicalDevice()
{
	auto indices = GetQueueFamiilyIndices(m_mainDevice.physDevice);

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
	float priority = 1.0f;
	// Vullkan need to know how to handle multiple queue, so set priority to show Vulkan
	queueCreateInfo.pQueuePriorities = &priority;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;

	VkPhysicalDeviceFeatures features = {};

	createInfo.pEnabledFeatures = &features;

	if (vkCreateDevice(m_mainDevice.physDevice, &createInfo, nullptr, &m_mainDevice.logicalDevice) != VK_SUCCESS)
		throw std::runtime_error("VULKAN INIT ERROR : Failed to create logical devices");

	// Get Queue that created inside logical device to use later
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.graphicsFamily, 0, &m_vkQueue);
}

void VkApplication::SetUpVkDebugMessengerEXT()
{
	if (!m_enableValidationLayer) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	PopulateVkDebugMessengerCreateInfo(createInfo);

	if (CreateVkDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS)
		throw std::runtime_error("VULKAN INIT ERROR : Failed to create debug utils messenger extension !");
}

bool VkApplication::CheckVkValidationLayersSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> supportLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, supportLayers.data());

#ifdef _DEBUG || DEBUG
	std::cout << "\n" << layerCount << " validations layers supported by Vulkan \n";
	std::cout << "Supported validation layers : \n";
	for (const auto& layer : supportLayers)
		std::cout << "\t" << layer.layerName << "\n";
#endif

	for (const auto& requiredLayer : m_validationLayers)
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

void VkApplication::GetVkPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("VUKAN INIT ERROR : Can't find physical devices (GPUs) supported by Vulkan Instance !");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (CheckVkPhysicalDeviceSuitable(device))
		{
			m_mainDevice.physDevice = device;
			break;
		}	
	}

	if (m_mainDevice.physDevice == nullptr)
		throw std::runtime_error("\nVULKAN INIT ERROR : Can't find suitable physical devices !");
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
