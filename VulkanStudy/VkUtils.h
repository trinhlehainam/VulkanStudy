#pragma once
#include <vector>

#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace VkUtils
{
	const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
}

namespace VkUtils
{
	struct QueueFamilyIndices
	{
		uint32_t graphicsFamilyIndex = UINT32_MAX;
		uint32_t presentationFamilyIndex = UINT32_MAX;

		bool IsValid(){ return graphicsFamilyIndex != UINT32_MAX && presentationFamilyIndex != UINT32_MAX; }
	};

	struct SwapChainDetails
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	struct Vertex
	{
		glm::vec3 Pos;
		glm::vec3 Color;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
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

	// If it failed to open file, return the empty object
	std::vector<char> ReadBinaryFile(const char* fileName);

	// spvFileName : compiled shader file to use with vulkan
	VkShaderModule CreateShaderModule(VkDevice device, const VkAllocationCallbacks* pAllocator, const char* spvFileName);

	// If function fails to create vertex buffer, it returns VK_NULL_HANDLE
	VkBuffer CreateBuffer(VkDevice device, uint64_t bufferSize, VkBufferUsageFlags usageFlags);

	// If it failed to create VkDeviceMemory , it returns VK_NULL_HANDLE
	VkDeviceMemory AllocateBufferMemory(VkPhysicalDevice physDevice, VkDevice device, VkBuffer buffer, VkMemoryPropertyFlags memProps);

	// If function doesn't find any suitable memory type, it returns UINT32_MAX
	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t allowedType, VkMemoryPropertyFlags properties);

	void CopyBuffer(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);
}

