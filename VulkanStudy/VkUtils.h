#pragma once
#include <vector>
#include <chrono>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
		glm::vec2 TexCoord;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
	};

	struct UniformBufferObject
	{
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Proj;
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

	void AllocateImage2D(VkPhysicalDevice physicalDevice, VkDevice device, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage,
		VkImage* pImage, VkDeviceMemory* pMemory);

	// If function doesn't find any suitable memory type, it returns UINT32_MAX
	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t allowedType, VkMemoryPropertyFlags properties);

	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice, VkImageTiling imageTiling);
	VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling imageTiling, VkFormatFeatureFlags feature);

	void BeginSingleTimeCommands(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer* pCmdBuffer);

	void EndSingleTimeCommands(VkQueue queue, VkCommandBuffer cmdBuffer);

	void CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

	void CreateImageBufferFromFile(const char* fileName, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool, 
		VkBuffer* pBuffer, VkDeviceMemory* pMemory, VkExtent3D* extent);

	void TransitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	void CopyBufferToImage(VkCommandBuffer cmdBuffer, VkExtent3D imageExtent, VkBuffer srcBuffer, VkImage dstImage);

	VkImageView CreateImageView2D(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect);

	VkSampler CreateSampler(VkPhysicalDevice physicalDevice, VkDevice device);
}

