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
		VkUtils::DestroyVkDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
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

void VkApplication::CreateVkLogicalDevice()
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

void VkApplication::CreateVkSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create VkSurface !\n");
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
