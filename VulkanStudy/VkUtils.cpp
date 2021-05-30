#include "VkUtils.h"

#include <iostream>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace
{
	constexpr int kBytesPerPixel = 4;
}

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
		descs.resize(3, {});

		descs[0].binding = 0;
		descs[0].location = 0;
		descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		descs[0].offset = offsetof(Vertex, Pos);

		descs[1].binding = 0;
		descs[1].location = 1;
		descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		descs[1].offset = offsetof(Vertex, Color);

		descs[2].binding = 0;
		descs[2].location = 2;
		descs[2].format = VK_FORMAT_R32G32_SFLOAT;
		descs[2].offset = offsetof(Vertex, TexCoord);

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
				indices.graphicsFamilyIndex = index;

			VkBool32 presentationSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentationSupported);

			if (queueFamily.queueCount > 0 && presentationSupported)
				indices.presentationFamilyIndex = index;

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

	VkBuffer CreateBuffer(VkDevice device, uint64_t bufferSize, VkBufferUsageFlags usageFlags)
	{
		VkBuffer buffer = VK_NULL_HANDLE;

		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = bufferSize;
		createInfo.usage = usageFlags;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer(device, &createInfo, nullptr, &buffer);

		return buffer;
	}

	VkDeviceMemory AllocateBufferMemory(VkPhysicalDevice physDevice, VkDevice device, VkBuffer buffer, VkMemoryPropertyFlags memProps)
	{
		VkDeviceMemory mem = VK_NULL_HANDLE;

		VkMemoryRequirements memRequire{};
		vkGetBufferMemoryRequirements(device, buffer, &memRequire);

		auto suitableMemType = VkUtils::FindMemoryType(physDevice, memRequire.memoryTypeBits, memProps);
		if (suitableMemType == UINT32_MAX)
			throw std::runtime_error("\nVULKAN ERROR : Failed to find suitable memory type for VERTEX BUFFER !\n");

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequire.size;
		allocInfo.memoryTypeIndex = suitableMemType;

		if (vkAllocateMemory(device, &allocInfo, nullptr, &mem) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to allocate memory !\n");

		if (vkBindBufferMemory(device, buffer, mem, 0) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to bind VERTEX BUFFER and DEVICE MEMORY !\n");

		return mem;
	}

	void AllocateImage2D(VkPhysicalDevice physicalDevice, VkDevice device, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, uint32_t mipLevels,
		VkImage* pImage, VkDeviceMemory* pMemory)
	{
		VkImageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.format = format;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.usage = usage;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.extent = extent;
		createInfo.mipLevels = mipLevels;
		createInfo.arrayLayers = 1;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &createInfo, nullptr, pImage) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create image !\n");

		VkMemoryRequirements memRequire{};
		vkGetImageMemoryRequirements(device, *pImage, &memRequire);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequire.size;
		allocInfo.memoryTypeIndex = VkUtils::FindMemoryType(physicalDevice, memRequire.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, pMemory) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to allocate memory for image !\n");

		vkBindImageMemory(device, *pImage, *pMemory, 0);
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

	VkFormat FindDepthFormat(VkPhysicalDevice physicalDevice, VkImageTiling imageTiling)
	{
		return FindSupportedFormat(
			physicalDevice, 
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			imageTiling,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling imageTiling, VkFormatFeatureFlags feature)
	{
		VkFormatProperties props{};
		for (const auto& format : formats)
		{
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			bool is_supported_linear_feature = (props.linearTilingFeatures & feature) == feature;
			bool is_supported_optimal_feature = (props.optimalTilingFeatures & feature) == feature;
			if (imageTiling == VK_IMAGE_TILING_LINEAR && is_supported_linear_feature)
				return format;
			else if (imageTiling == VK_IMAGE_TILING_OPTIMAL && is_supported_optimal_feature)
				return format;
		}
		
		throw std::runtime_error("\nVULKAN ERROR : Don't find any supported formats !\n");
	}

	bool HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void BeginSingleTimeCommands(VkDevice device, VkCommandPool cmdPool, VkCommandBuffer* pCmdBuffer)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool;
		allocInfo.commandBufferCount = 1;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(device, &allocInfo, pCmdBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(*pCmdBuffer, &beginInfo);

	}

	void EndSingleTimeCommands(VkQueue queue, VkCommandBuffer cmdBuffer)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

		vkQueueWaitIdle(queue);
	}

	void CopyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
	{
		VkBufferCopy bufferCopy{};
		bufferCopy.size = bufferSize;
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset = 0;
		vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &bufferCopy);
	}

	void CreateImageFromFile(const char* fileName, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool,
		VkBuffer* pBuffer, VkDeviceMemory* pMemory, VkExtent3D* extent)
	{
		int width, height, channel;

		stbi_uc* pixels = stbi_load(fileName, &width, &height, &channel, STBI_rgb_alpha);
		if (!pixels)
			throw std::runtime_error("\nERROR : Failed to load texture image from file !\n");

		extent->width = width;
		extent->height = height;
		extent->depth = 1;
		VkDeviceSize imageSize = width * height * kBytesPerPixel;
		*pBuffer = CreateBuffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		*pMemory = AllocateBufferMemory(physicalDevice, device, *pBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto transferBuffer = CreateBuffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		auto transferMemory = AllocateBufferMemory(physicalDevice, device, transferBuffer, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data = nullptr;
		vkMapMemory(device, transferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, imageSize);
		vkUnmapMemory(device, transferMemory);

		VkCommandBuffer tmpCmdBuffer;
		BeginSingleTimeCommands(device, cmdPool, &tmpCmdBuffer);
		CopyBuffer(tmpCmdBuffer, transferBuffer, *pBuffer, imageSize);
		EndSingleTimeCommands(queue, tmpCmdBuffer);

		vkDestroyBuffer(device, transferBuffer, nullptr);
		vkFreeMemory(device, transferMemory, nullptr);
	}

	uint32_t CalculateMipLevels(const VkExtent3D& extent)
	{
		return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;;
	}

	void TransitionImageLayout(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = 0;			// TODO
		barrier.dstAccessMask = 0;			// TODO

		VkPipelineStageFlags srcStageMask = 0;
		VkPipelineStageFlags dstStageMask = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
			throw std::runtime_error("\nVULKAN ERROR : Unsupported image layout transition !\n");

		vkCmdPipelineBarrier(cmdBuffer, 
			srcStageMask, dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	void CopyBufferToImage(VkCommandBuffer cmdBuffer, VkExtent3D imageExtent, VkBuffer srcBuffer, VkImage dstImage)
	{
		VkBufferImageCopy region{};
		region.bufferImageHeight = 0;
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.imageExtent = imageExtent;
		region.imageOffset = { 0,0,0 };
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.mipLevel = 0;

		vkCmdCopyBufferToImage(cmdBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	VkImageView CreateImageView2D(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.format = format;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.subresourceRange.aspectMask = aspect;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.levelCount = mipLevels;
		createInfo.subresourceRange.baseMipLevel = 0;

		VkImageView view;
		if (vkCreateImageView(device, &createInfo, nullptr, &view) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create Image View !\n");

		return view;
	}

	VkSampler CreateSampler(VkPhysicalDevice physicalDevice, VkDevice device)
	{
		VkSamplerCreateInfo createInfo{};;
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.compareEnable = VK_FALSE;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.mipLodBias = 0.0f;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 0.0f;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevice, &props);
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = props.limits.maxSamplerAnisotropy;

		VkSampler sampler;
		if (vkCreateSampler(device, &createInfo, nullptr, &sampler) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create Sampler !\n");

		return sampler;
	}

	void LoadModel(const char* modelPath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath)) {
			throw std::runtime_error(warn + err);
		}

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.Pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.TexCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.Color = { 1.0f, 1.0f, 1.0f };

				vertices.push_back(vertex);
				indices.push_back(static_cast<uint32_t>(indices.size()));
			}
		}
	}
}
