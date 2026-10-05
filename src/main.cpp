#include "sim/Simulation.h"
#include "render/VulkanRenderer.h"
#include "render/SimulationUI.h"
#include <iostream>
#include <chrono>
#include <cstdlib>

static void setupVulkanEnvironment() {
    // On macOS with Homebrew, point Vulkan loader to MoltenVK ICD if not already configured
    const char* envIcd = std::getenv("VK_ICD_FILENAMES");
    const char* envDriver = std::getenv("VK_DRIVER_FILES");
    if (!envIcd && !envDriver) {
        const char* defaultIcd = "/opt/homebrew/Cellar/molten-vk/1.4.2/etc/vulkan/icd.d/MoltenVK_icd.json";
        setenv("VK_ICD_FILENAMES", defaultIcd, 0);
    }
}

int main(int argc, char** argv) {
    setupVulkanEnvironment();

    bool testMode = false;
    int testFrames = 10;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--test" || arg == "-t") {
            testMode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                testFrames = std::atoi(argv[++i]);
                if (testFrames <= 0) testFrames = 10;
            }
        }
    }

    std::cout << "========================================================\n"
              << "       PARKING LOT SIMULATION (Vulkan Modernized)       \n"
              << "========================================================\n"
              << " Controls:\n"
              << "   [A]      : Random Vehicle Arrival (Issues Ticket & Parks)\n"
              << "   [1]..[4] : Specific Arrival (1:Van, 2:Car, 3:Moto, 4:Bike)\n"
              << "   [D]      : Vehicle Departure (Processes Ticket & Exits)\n"
              << "   [P]      : Print Parking Lot Status Table\n"
              << "   [Space]  : Toggle Auto Simulation (Off by default)\n"
              << "   [Esc]    : Exit Simulation\n"
              << "========================================================\n" << std::endl;

    Simulation simulation;
    VulkanRenderer renderer(1280, 800, "Parking Lot Simulator - Vulkan");

    SimulationUI ui;

    renderer.setKeyCallback([&simulation, &renderer](int key, int scancode, int action, int mods) {
        if (action != GLFW_PRESS) return;

        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(renderer.getWindow(), GLFW_TRUE);
        } else if (key == GLFW_KEY_A) {
            simulation.triggerArrival();
        } else if (key == GLFW_KEY_1) {
            simulation.triggerArrival(VehicleType::VAN);
        } else if (key == GLFW_KEY_2) {
            simulation.triggerArrival(VehicleType::CAR);
        } else if (key == GLFW_KEY_3) {
            simulation.triggerArrival(VehicleType::MOTORBIKE);
        } else if (key == GLFW_KEY_4) {
            simulation.triggerArrival(VehicleType::BICYCLE);
        } else if (key == GLFW_KEY_D) {
            simulation.triggerDeparture();
        } else if (key == GLFW_KEY_P) {
            simulation.getParkingLot().printStatus();
        } else if (key == GLFW_KEY_SPACE) {
            simulation.toggleAutoMode();
            std::cout << "[SIM] Auto Simulation " << (simulation.isAutoMode() ? "RESUMED" : "PAUSED") << "\n";
        }
    });

    if (!renderer.init()) {
        std::cerr << "Failed to initialize Vulkan renderer!\n";
        return 1;
    }

    ui.initStyle();

    RenderBatch batch;
    auto lastTime = std::chrono::high_resolution_clock::now();

    if (testMode) {
        std::cout << "[Test Mode] Running " << testFrames << " frames of Vulkan simulation...\n" << std::endl;
        simulation.triggerArrival(VehicleType::CAR);
        simulation.triggerArrival(VehicleType::MOTORBIKE);
        simulation.getParkingLot().printStatus();
        simulation.triggerDeparture();
    }

    int frameCount = 0;
    while (!renderer.shouldClose()) {
        renderer.newFrame();
        ui.render(simulation);
        renderer.setCanvasMargins(0.0f, ui.getTopBarHeight(), ui.isSidebarOpen() ? ui.getSidebarWidth() : 0.0f, 0.0f);



        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Cap deltaTime to avoid huge leaps during window move/resize
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        simulation.update(deltaTime);
        simulation.buildRenderBatch(batch);
        renderer.render(batch);

        if (testMode) {
            frameCount++;
            if (frameCount >= testFrames) {
                std::cout << "[Test Mode] Successfully executed " << frameCount << " frames with Vulkan renderer!\n" << std::endl;
                break;
            }
        }
    }

    std::cout << "\nExiting Parking Lot Simulation. Goodbye!\n";
    return 0;
}
