#pragma once
#include <vector>

#include <vulkan/vulkan.h>

namespace VkUtils
{
	const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

namespace VkUtils
{
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1;
		int presentationFamily = -1;

		bool IsValid(){ return graphicsFamily >= 0 && presentationFamily >= 0; }
	};

	struct SwapChainDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};
}

namespace VkUtils
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

	QueueFamilyIndices GetQueueFamiilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);

	SwapChainDetails CheckSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
}

