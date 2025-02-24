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