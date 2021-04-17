#include <iostream>
#include <exception>

#include "VkApplication.h"

int main() {
    
    VkApplication vkApp(800, 600, "Vulkan Application");
    
    try
    {
        vkApp.Run();
    } 
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}