#include "VkUtils.h"

#include <iostream>
#include <fstream>

static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageServerities,
	VkDebugUtilsMessageTypeFlagsEXT messageFlags,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (messageServerities > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "\nVULKAN DEBUG MESSAGE : " << pCallbackData->pMessage << "\n" << std::endl;

	return VK_FALSE;
}

namespace VkUtils
{
	VkVertexInputBindingDescription Vertex::GetBindingDescription()
	{
		VkVertexInputBindingDescription desc{};
		desc.binding = 0;
		desc.stride = sizeof(Vertex);
		desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return desc;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> descs;
		descs.resize(2, {});

		descs[0].binding = 0;
		descs[0].location = 0;
		descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		descs[0].offset = offsetof(Vertex, Pos);

		descs[1].binding = 0;
		descs[1].location = 1;
		descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		descs[1].offset = offsetof(Vertex, Color);

		return descs;
	}
}

namespace VkUtils
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
		std::cout << "VULKAN REQUIRED INSTANCE EXTENSIONS : \n";
		for (const auto& requiredExtension : requiredExtensions)
			std::cout << "\t" << requiredExtension << "\n";

		std::cout << "\n" << extensionsCount << " instance extensions are supported by Vulkan \n";
		std::cout << "VULKAN INSTANCE EXTENSIONS : \n";
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
		std::cout << "VULKAN SUPPORTED VALIDATION LAYERS : \n";
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
		std::cout << "\nVULKAN SUPPORTED DEVICE EXTENSIONS :\n";
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

		bool isQueueFamiliesValid = GetQueueFamiilyIndices(device, surface).IsValid();
		if (!isQueueFamiliesValid) return false;

		bool isExtensionsSupported = CheckVkDeviceExtensionsSupport(device, VkUtils::DEVICE_EXTENSIONS);
		if (!isExtensionsSupported) return false;

		auto swapChainDetails = CheckSwapChainDetails(device, surface);
		bool isSwapchainValid = !swapChainDetails.Formats.empty() && !swapChainDetails.PresentModes.empty();
		if (!isSwapchainValid) return false;

		return true;
	}

	QueueFamilyIndices GetQueueFamiilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

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

	SwapChainDetails CheckSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapChainDetails details;

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

	std::vector<char> ReadBinaryFile(const char* fileName)
	{
		// Read to the end of the file to know file's size
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);
		
		if (!file.is_open())
			return std::vector<char>();

		// Take the last position of byte in this file
		// This position is the size of the file
		auto byteCount = static_cast<size_t>(file.tellg());

		// Move to beginning of the file to start reading
		file.seekg(0);

		std::vector<char> buffer(byteCount);
		file.read(buffer.data(), byteCount);

		file.close();

		return buffer;
	}

	VkShaderModule CreateShaderModule(VkDevice device, const VkAllocationCallbacks* pAllocator, const char* spvFileName)
	{
		auto bytecode = ReadBinaryFile(spvFileName);

		if (bytecode.empty())
			throw std::runtime_error("\nVULKAN ERROR : Failed to load shader's bytecode !\n");

		VkShaderModuleCreateInfo createInfo {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = bytecode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, pAllocator, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create shader module !\n");

		return shaderModule;
	}

	VkBuffer CreateVertexBuffer(VkDevice device, uint64_t vertexSize)
	{
		VkBuffer buffer = VK_NULL_HANDLE;

		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = vertexSize;
		createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer(device, &createInfo, nullptr, &buffer);

		return buffer;
	}

	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t allowedType, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProps{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

		for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
		{
			if ((allowedType & (1 << i)) && 
				((memProps.memoryTypes[i].propertyFlags & properties) == properties)
				)
				return i;
		}

		return UINT32_MAX;
	}
}
