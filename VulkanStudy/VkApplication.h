#pragma once
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VkApplication
{
public:
	explicit VkApplication(int width = 800, int height = 600, const char* window_title = "VkApplication");
	void Run();
private:
	int m_width;
	int m_height;
	const char* m_title;
private:
	void InitWindow();
	void InitVulkan();
	void MainLoop();
	void CleanUp();

	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateSwapchain();
	void CreateImageViews();
	void CreateGraphicsPipeline();

	void SetUpVkDebugMessengerEXT();

	void PickVkPhysicalDevice();
	VkSurfaceFormatKHR PickVkSurfaceFormats(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR PickVkPresentModes(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D PickVkSwapchainImageExtent(const VkSurfaceCapabilitiesKHR& capability);

	std::vector<const char*> GetRequiredInstanceExtensions();

	GLFWwindow* m_window;
	VkInstance m_instance;

	bool m_enableValidationLayer;

	VkDebugUtilsMessengerEXT m_debugMessenger;
	struct{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	}m_mainDevice;
	VkQueue m_graphicsQueue;
	VkQueue m_presentationQueue;

	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapchainImages;
	VkFormat m_swapchainFormat;
	VkExtent2D m_swapchainExtent;
	std::vector<VkImageView> m_imageViews;

	VkPipelineLayout m_pipelineLayout;
};

