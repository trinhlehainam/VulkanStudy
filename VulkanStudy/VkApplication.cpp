#include "VkApplication.h"

#include <iostream>
#include <algorithm>
#include <cstdint>
#include <set>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ONE_TO_ZERO
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "VkUtils.h"

namespace
{
	const std::vector<VkUtils::Vertex> g_vertices = {
	{{-0.5f, -0.5f,0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f,0.0f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f,0.0f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f,0.0f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint32_t> g_indices = { 0,1,2,2,3,0 };
}

const uint16_t MAX_FRAMES_IN_FLIGHT = 2;


VkApplication::VkApplication(int width, int height, const char* window_title):
	m_screenWidth(width),m_screenHeight(height),m_title(window_title),m_currenFrame(0)
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

	m_window = glfwCreateWindow(m_screenWidth, m_screenHeight, m_title, nullptr, nullptr);
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

	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();

	CreateFramebuffers();
	CreateCommandPool();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateDescriptorPool();
	AllocateDescriptorSets();

	AllocateCommandBuffers();
	RecordCommands();
	CreateSyncObjects();
}

void VkApplication::MainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		RenderFrame();
	}
}

void VkApplication::CleanUp()
{
	vkDeviceWaitIdle(m_mainDevice.logicalDevice);

	if (m_enableValidationLayer)
		VkUtils::DestroyVkDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	
	// Synchronisation objects
	for (uint16_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroySemaphore(m_mainDevice.logicalDevice, m_imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(m_mainDevice.logicalDevice, m_renderFinishedSemapheres[i], nullptr);
		vkDestroyFence(m_mainDevice.logicalDevice, m_inFlightFences[i], nullptr);
	}

	// Buffers and memories
	vkDestroyBuffer(m_mainDevice.logicalDevice, m_vertexBuffer, nullptr);
	vkDestroyBuffer(m_mainDevice.logicalDevice, m_indexBuffer, nullptr);
	for (auto& buffer : m_uniformBuffers)
		vkDestroyBuffer(m_mainDevice.logicalDevice, buffer, nullptr);

	vkFreeMemory(m_mainDevice.logicalDevice, m_vertexBufferMemory, nullptr);
	vkFreeMemory(m_mainDevice.logicalDevice, m_indexBufferMemory, nullptr);
	for (auto& memory : m_uniformBufferMemorys)
		vkFreeMemory(m_mainDevice.logicalDevice, memory, nullptr);
	
	// Pipeline objects
	for (auto& framebuffer : m_swapchainFramebuffers)
		vkDestroyFramebuffer(m_mainDevice.logicalDevice, framebuffer, nullptr);
	vkDestroyCommandPool(m_mainDevice.logicalDevice, m_cmdPool, nullptr);
	vkDestroyPipeline(m_mainDevice.logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyDescriptorSetLayout(m_mainDevice.logicalDevice, m_descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_mainDevice.logicalDevice, m_descriptorPool, nullptr);
	vkDestroyPipelineLayout(m_mainDevice.logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_mainDevice.logicalDevice, m_renderPass, nullptr);

	// Presentation objects
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
	VkApplicationInfo appInfo {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = m_title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// Struct carries info to create VkInstance
	VkInstanceCreateInfo vkCreateInfo {};
	vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInfo.pApplicationInfo = &appInfo;

	auto requiredExtensions = GetRequiredInstanceExtensions();

	if (!VkUtils::CheckVkInstanceExtensionsSupport(requiredExtensions))
		throw std::runtime_error("\nVULKAN INIT ERROR : some Vulkan Instance Extensions are not supported !\n");

	vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	vkCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	if (m_enableValidationLayer && !VkUtils::CheckVkValidationLayersSupport(VkUtils::VALIDATION_LAYERS))
		throw std::runtime_error("\nVULKAN INIT ERROR : vallidation layers required , but not found !\n");

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
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
	auto indices = VkUtils::GetQueueFamiilyIndices(m_mainDevice.physicalDevice, m_surface);

	// std::set only allow one object to hold one specific value
	// Use std::set to check if presentation queue is inside graphics queue or in the seperate queue
	// If presentation queue is inside graphics queue -> only create one queue
	// Else create the seperate queue for presentation queue
	std::set<uint32_t> queueFamilyIndices = { indices.graphicsFamilyIndex, indices.presentationFamilyIndex };

	std::vector <VkDeviceQueueCreateInfo> queueCreateInfos;
	queueCreateInfos.reserve(queueFamilyIndices.size());

	// Vullkan need to know how to handle multiple queue, so set priority to show Vulkan
	float priority = 1.0f;
	for (auto queueIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkDeviceCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(VkUtils::DEVICE_EXTENSIONS.size());
	createInfo.ppEnabledExtensionNames = VkUtils::DEVICE_EXTENSIONS.data();

	VkPhysicalDeviceFeatures features {};
	createInfo.pEnabledFeatures = &features;

	if (vkCreateDevice(m_mainDevice.physicalDevice, &createInfo, nullptr, &m_mainDevice.logicalDevice) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create logical devices !\n");

	// Get Queue that created inside logical device to use later
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.graphicsFamilyIndex, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.presentationFamilyIndex, 0, &m_presentationQueue);
}

void VkApplication::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN INIT ERROR : Failed to create VkSurface !\n");
}

void VkApplication::CreateSwapchain()
{
	auto details = VkUtils::CheckSwapChainDetails(m_mainDevice.physicalDevice, m_surface);

	auto extent = PickVkSwapchainImageExtent(details.Capabilities);
	auto presentMode = PickVkPresentModes(details.PresentModes);
	auto format = PickVkSurfaceFormats(details.Formats);

	// Provide one more image to keep GPU busy
	auto imageCount = details.Capabilities.minImageCount + 1;

	// If maxImageCount = 0, it means there is no limit
	if (details.Capabilities.maxImageCount > 0 && imageCount > details.Capabilities.maxImageCount)
		imageCount = details.Capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.minImageCount = imageCount;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	auto indices = VkUtils::GetQueueFamiilyIndices(m_mainDevice.physicalDevice, m_surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamilyIndex, indices.presentationFamilyIndex };

	if (indices.graphicsFamilyIndex != indices.presentationFamilyIndex)
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
		VkImageViewCreateInfo createInfo {};
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

void VkApplication::CreateRenderPass()
{
	VkAttachmentDescription attachment{};
	attachment.format = m_swapchainFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentRef{};
	attachmentRef.attachment = 0;				// attachment's index
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE_KHR;
	dependency.dstSubpass = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &attachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_mainDevice.logicalDevice, &createInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create render pass !\n");

}

void VkApplication::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding.descriptorCount = 1;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = 1;
	createInfo.pBindings = &binding;

	if (vkCreateDescriptorSetLayout(m_mainDevice.logicalDevice, &createInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create Descriptor Set Layout !\n");
}

void VkApplication::CreateGraphicsPipeline()
{
	VkShaderModule vertShaderModule = VkUtils::CreateShaderModule(m_mainDevice.logicalDevice, nullptr, "assets/shaders/vert.spv");
	VkShaderModule fragShaderModule = VkUtils::CreateShaderModule(m_mainDevice.logicalDevice, nullptr, "assets/shaders/frag.spv");

	VkPipelineShaderStageCreateInfo vertStageCreateInfo {};
	vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageCreateInfo.module = vertShaderModule;
	vertStageCreateInfo.pName = "main";											// main of shader's start up function

	VkPipelineShaderStageCreateInfo fragStageCreateInfo {};
	fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageCreateInfo.module = fragShaderModule;
	fragStageCreateInfo.pName = "main";											// main of shader's start up function

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = { vertStageCreateInfo , fragStageCreateInfo };
	
	auto bindingDescs = VkUtils::Vertex::GetBindingDescription();
	auto attributeDescs = VkUtils::Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescs;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescs.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(m_screenWidth);
	viewport.height = static_cast<float>(m_screenHeight);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 0.0f;

	VkRect2D scissor{};
	scissor.offset = { 0 , 0 };
	scissor.extent = m_swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportCreateInfo{};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 1.0f;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCreateInfo.depthBiasClamp = 0.0f;
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.pSampleMask = nullptr;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState attachment{};
	attachment.blendEnable = VK_FALSE;
	attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment.colorBlendOp = VK_BLEND_OP_ADD;
	attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
	colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &attachment;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicCreateInfo{};
	dynamicCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicCreateInfo.dynamicStateCount = _countof(dynamicStates);
	dynamicCreateInfo.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create pipeline layout !\n");

	VkGraphicsPipelineCreateInfo graphicsCreateInfo{};
	graphicsCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsCreateInfo.stageCount = _countof(shaderStageCreateInfos);
	graphicsCreateInfo.pStages = shaderStageCreateInfos;
	graphicsCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsCreateInfo.pViewportState = &viewportCreateInfo;
	graphicsCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsCreateInfo.pMultisampleState = &multisampleCreateInfo;
	graphicsCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	//graphicsCreateInfo.pDynamicState = &dynamicCreateInfo;
	graphicsCreateInfo.renderPass = m_renderPass;
	graphicsCreateInfo.subpass = 0;
	graphicsCreateInfo.layout = m_pipelineLayout;
	graphicsCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &graphicsCreateInfo, nullptr, &m_graphicsPipeline)
		!= VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Falied to create graphics pipeline !\n");

	vkDestroyShaderModule(m_mainDevice.logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_mainDevice.logicalDevice, fragShaderModule, nullptr);
}

void VkApplication::CreateFramebuffers()
{
	m_swapchainFramebuffers.resize(m_swapchainImages.size());

	for (int i = 0; i < m_imageViews.size(); ++i)
	{
		VkImageView attachments[] = { m_imageViews[i] };

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_renderPass;
		createInfo.attachmentCount = _countof(attachments);
		createInfo.pAttachments = attachments;
		createInfo.width = m_screenWidth;
		createInfo.height = m_screenHeight;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(m_mainDevice.logicalDevice, &createInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create framebuffer !\n");
	}
}

void VkApplication::CreateCommandPool()
{
	auto indices = VkUtils::GetQueueFamiilyIndices(m_mainDevice.physicalDevice, m_surface);

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = indices.graphicsFamilyIndex;

	if (vkCreateCommandPool(m_mainDevice.logicalDevice, &createInfo, nullptr, &m_cmdPool) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create command pool !\n");
}

void VkApplication::CreateVertexBuffer()
{
	// Create vertex buffer and allocate memory
	auto bufferSize = sizeof(g_vertices[0]) * g_vertices.size();
	m_vertexBuffer = VkUtils::CreateBuffer(m_mainDevice.logicalDevice, bufferSize, 
						VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	if (m_vertexBuffer == VK_NULL_HANDLE)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create vertex buffer !\n");
	m_vertexBufferMemory = VkUtils::AllocateBufferMemory(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, m_vertexBuffer,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Create transfer buffer and allocate memory (to upload data to GPU's buffer)
	VkBuffer transferBuffer = VkUtils::CreateBuffer(m_mainDevice.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	if (transferBuffer == VK_NULL_HANDLE)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create transfer buffer !\n");
	VkDeviceMemory transferMemory = VkUtils::AllocateBufferMemory(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, transferBuffer,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// Map data to transfer buffer
	void* data = nullptr;
	vkMapMemory(m_mainDevice.logicalDevice, transferMemory, 0, bufferSize, 0, &data);
	memcpy(data, g_vertices.data(), bufferSize);
	vkUnmapMemory(m_mainDevice.logicalDevice, transferMemory);

	VkCommandBuffer tempCmdBuffer = VkUtils::BeginSingleTimeCommands(m_mainDevice.logicalDevice, m_cmdPool);
	VkUtils::CopyBuffer(tempCmdBuffer, transferBuffer, m_vertexBuffer, bufferSize);
	VkUtils::EndSingleTimeCommands(m_graphicsQueue, tempCmdBuffer);

	// Release transfer resources
	vkDestroyBuffer(m_mainDevice.logicalDevice, transferBuffer, nullptr);
	vkFreeMemory(m_mainDevice.logicalDevice, transferMemory, nullptr);
}

void VkApplication::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(g_indices[0]) * g_indices.size();
	m_indexBuffer = VkUtils::CreateBuffer(m_mainDevice.logicalDevice, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	if (m_indexBuffer == VK_NULL_HANDLE)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create index buffer !\n");
	m_indexBufferMemory = VkUtils::AllocateBufferMemory(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, m_indexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	auto transferBuffer = VkUtils::CreateBuffer(m_mainDevice.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	if (transferBuffer == VK_NULL_HANDLE)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create staging buffer to transfer resources to INDEX buffer !\n");
	auto transferMemory = VkUtils::AllocateBufferMemory(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, transferBuffer, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data = nullptr;
	vkMapMemory(m_mainDevice.logicalDevice, transferMemory, 0, bufferSize, 0, &data);
	memcpy(data, g_indices.data(), bufferSize);
	vkUnmapMemory(m_mainDevice.logicalDevice, transferMemory);
	
	VkCommandBuffer tempCmdBuffer = VkUtils::BeginSingleTimeCommands(m_mainDevice.logicalDevice, m_cmdPool);
	VkUtils::CopyBuffer(tempCmdBuffer, transferBuffer, m_vertexBuffer, bufferSize);
	VkUtils::EndSingleTimeCommands(m_graphicsQueue, tempCmdBuffer);

	vkDestroyBuffer(m_mainDevice.logicalDevice, transferBuffer, nullptr);
	vkFreeMemory(m_mainDevice.logicalDevice, transferMemory, nullptr);
}

void VkApplication::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(m_uniformBuffers.size());

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_uniformBuffers.size());
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = &poolSize;

	if (vkCreateDescriptorPool(m_mainDevice.logicalDevice, &poolCreateInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to create Descriptor Pool !\n");;
}

void VkApplication::AllocateDescriptorSets()
{
	m_descriptorSets.resize(m_uniformBuffers.size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_descriptorSets.size(), m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_descriptorSets.size());
	allocInfo.pSetLayouts = setLayouts.data();

	if (vkAllocateDescriptorSets(m_mainDevice.logicalDevice, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to allocate Descriptor Sets !\n");

	for (size_t i = 0; i < m_uniformBuffers.size(); ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(VkUtils::UniformBufferObject);

		VkWriteDescriptorSet writeSet{};
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet = m_descriptorSets[i];
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = 0;
		writeSet.pBufferInfo = &bufferInfo;
		writeSet.descriptorCount = 1;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		vkUpdateDescriptorSets(m_mainDevice.logicalDevice, 1, &writeSet, 0, nullptr);
	}
}

void VkApplication::CreateUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(VkUtils::UniformBufferObject);
	auto imageCount = m_swapchainImages.size();

	m_uniformBuffers.resize(imageCount);
	m_uniformBufferMemorys.resize(imageCount);

	for (uint16_t i = 0; i < imageCount; ++i)
	{
		auto& buffer = m_uniformBuffers[i];
		auto& memory = m_uniformBufferMemorys[i];

		buffer = VkUtils::CreateBuffer(m_mainDevice.logicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		if (buffer == VK_NULL_HANDLE)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create Uniform Buffer !\n");

		memory = VkUtils::AllocateBufferMemory(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, buffer,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}
}

void VkApplication::AllocateCommandBuffers()
{
	m_cmdBuffers.resize(m_swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_cmdPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_cmdBuffers.size());

	if (vkAllocateCommandBuffers(m_mainDevice.logicalDevice, &allocInfo, m_cmdBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to allocate command buffers!\n");
}

void VkApplication::CreateSyncObjects()
{
	m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemapheres.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(m_swapchainFramebuffers.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaCreateInfo{};
	semaCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint16_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		if (vkCreateSemaphore(m_mainDevice.logicalDevice, &semaCreateInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERORR : Failed to create image available semaphore !\n");
		if (vkCreateSemaphore(m_mainDevice.logicalDevice, &semaCreateInfo, nullptr, &m_renderFinishedSemapheres[i]) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERORR : Failed to create render finished semaphore !\n");
		if (vkCreateFence(m_mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to create fences !\n");
	}
}

void VkApplication::RecordCommands()
{
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkRenderPassBeginInfo renderBeginInfo{};
	renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderBeginInfo.renderPass = m_renderPass;
	renderBeginInfo.renderArea.offset = { 0,0 };
	renderBeginInfo.renderArea.extent = m_swapchainExtent;

	VkClearValue clearValues[] = 
	{ {0.0f,0.0f,0.0f,1.0f} };
	renderBeginInfo.clearValueCount = _countof(clearValues);
	renderBeginInfo.pClearValues = clearValues;

	for (int i = 0; i < m_swapchainFramebuffers.size(); ++i)
	{
		renderBeginInfo.framebuffer = m_swapchainFramebuffers[i];
		auto& cmdBuffer = m_cmdBuffers[i];

		if (vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to start record commands !\n");

		// Record
		vkCmdBeginRenderPass(cmdBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
		VkBuffer buffers[] = { m_vertexBuffer };
		VkDeviceSize deviceSizes[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, buffers, deviceSizes);
		vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);
		vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(cmdBuffer);

		if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS)
			throw std::runtime_error("\nVULKAN ERROR : Failed to stop record commands !\n");
	}
}

void VkApplication::RenderFrame()
{
	uint32_t imageIndex = 0;
	if (vkAcquireNextImageKHR(m_mainDevice.logicalDevice, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currenFrame], VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to acquire swap chain's image !\n");

	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currenFrame];

	vkWaitForFences(m_mainDevice.logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(m_mainDevice.logicalDevice, 1, &m_imagesInFlight[imageIndex]);
		
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_cmdBuffers[imageIndex];

	VkPipelineStageFlags pipelineStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currenFrame] };
	submitInfo.pWaitDstStageMask = pipelineStages;
	submitInfo.waitSemaphoreCount = _countof(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemapheres[m_currenFrame] };
	submitInfo.signalSemaphoreCount = _countof(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;
	
	UpdateUniformBuffer(imageIndex);

	if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_imagesInFlight[imageIndex]) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Failed to submit rendering to swap chain's image !\n");

	VkSwapchainKHR swapchains[] = { m_swapchain };

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = _countof(swapchains);
	presentInfo.pSwapchains = swapchains;
	presentInfo.waitSemaphoreCount = _countof(signalSemaphores);
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.pImageIndices = &imageIndex;

	if (vkQueuePresentKHR(m_presentationQueue, &presentInfo) != VK_SUCCESS)
		throw std::runtime_error("\nVULKAN ERROR : Falied to submit present info to queue !\n");

	m_currenFrame = (m_currenFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VkApplication::UpdateUniformBuffer(uint16_t imageIndex)
{
	VkDeviceSize bufferSize = sizeof(VkUtils::UniformBufferObject);
	auto& memory = m_uniformBufferMemorys[imageIndex];

	static auto s_startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - s_startTime).count();

	VkUtils::UniformBufferObject ubo{};
	ubo.Model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.View = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.Proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_screenWidth) / m_screenHeight, 0.1f, 10.0f);
	ubo.Proj[1][1] *= -1;

	void* data = nullptr;
	vkMapMemory(m_mainDevice.logicalDevice, memory, 0, bufferSize, 0, &data);
	memcpy(data, &ubo, bufferSize);
	vkUnmapMemory(m_mainDevice.logicalDevice, memory);
}

void VkApplication::SetUpVkDebugMessengerEXT()
{
	if (!m_enableValidationLayer) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo {};
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
			m_mainDevice.physicalDevice = device;
			break;
		}	
	}

	if (m_mainDevice.physicalDevice == nullptr)
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
