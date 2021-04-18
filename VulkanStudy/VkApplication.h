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
	void SetUpVkDebugMessengerEXT();

	void GetVkPhysicalDevice();

	bool CheckVkValidationLayersSupport();

	std::vector<const char*> GetRequiredInstanceExtensions();

	GLFWwindow* m_window;
	VkInstance m_vkInstance;
	struct
	{
		VkPhysicalDevice physDevice;
		VkDevice logicalDevice;
	}m_mainDevice;
	VkQueue m_vkQueue;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
	bool m_enableValidationLayer;
};

