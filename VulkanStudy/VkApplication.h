#pragma once
#include <string>
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
	std::string m_title;
private:
	void InitWindow();
	void InitVulkan();
	void MainLoop();
	void CleanUp();

	void CreateVkInstance();
	void CreateVkLogicalDevice();
	void CreateVkSurface();

	void SetUpVkDebugMessengerEXT();

	void PickVkPhysicalDevice();

	bool CheckVkValidationLayersSupport();
	bool CheckVkDeviceExtensionsSupport();

	std::vector<const char*> GetRequiredInstanceExtensions();

	GLFWwindow* m_window;
	VkInstance m_instance;
	struct{
		VkPhysicalDevice physDevice;
		VkDevice logicalDevice;
	}m_mainDevice;
	VkQueue m_graphicsQueue;
	VkQueue m_presentationQueue;
	VkSurfaceKHR m_surface;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	bool m_enableValidationLayer;
};

