#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <glm/glm.hpp>
#include "VulkanRenderer.h"
#include "include/loader/ModelLoader.h"
#include <glm/gtc/matrix_transform.hpp>

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
int main() {

    // Add this near the beginning of main()
    #ifdef NDEBUG
        std::cout << "Running in RELEASE mode - validation disabled\n";
    #else
        std::cout << "Running in DEBUG mode - validation enabled\n";
    #endif
  
    VulkanRenderer app;
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cout<<"Exception: " << e.what() << std::endl;
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Hello, World!" << std::endl;

    
        
    return EXIT_SUCCESS;
}