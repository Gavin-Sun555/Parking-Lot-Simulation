cmake_minimum_required(VERSION 3.16)
project(ParkingLotSimulation VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add Homebrew search paths for macOS
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find GLFW
find_package(glfw3 CONFIG QUIET)
if(NOT glfw3_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW QUIET glfw3)
    endif()
endif()

# Fallback paths for GLFW on macOS if find_package didn't set targets
if(NOT TARGET glfw AND NOT TARGET glfw3::glfw)
    find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h HINTS /opt/homebrew/include /usr/local/include)
    find_library(GLFW_LIBRARY NAMES glfw glfw3 HINTS /opt/homebrew/lib /usr/local/lib)
endif()

# Optional shader compilation with glslc
find_program(GLSLC_EXECUTABLE glslc HINTS /opt/homebrew/bin /usr/local/bin)
if(GLSLC_EXECUTABLE)
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
    set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    add_custom_target(compile_shaders
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_DIR}/shader.vert -o ${SHADER_DIR}/vert.spv
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_DIR}/shader.frag -o ${SHADER_DIR}/frag.spv
        COMMENT "Compiling GLSL shaders to SPIR-V"
    )
endif()

# ImGui Sources
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/ParkingLot.cpp
    src/sim/Figures.cpp
    src/sim/CarParking.cpp
    src/sim/Simulation.cpp
    src/render/VulkanRenderer.cpp
    src/render/SimulationUI.cpp
    ${IMGUI_SOURCES}
)

set(HEADERS
    src/core/Vehicle.h
    src/core/Ticket.h
    src/core/ParkingLot.h
    src/sim/Vec.h
    src/sim/Figures.h
    src/sim/CarParking.h
    src/sim/Simulation.h
    src/render/RenderTypes.h
    src/render/VulkanRenderer.h
    src/render/SimulationUI.h
)

# Executable target
add_executable(ParkingLotSimulation ${SOURCES} ${HEADERS})

if(TARGET compile_shaders)
    add_dependencies(ParkingLotSimulation compile_shaders)
endif()

target_include_directories(ParkingLotSimulation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

if(TARGET glfw)
    target_link_libraries(ParkingLotSimulation PRIVATE glfw)
elseif(TARGET glfw3::glfw)
    target_link_libraries(ParkingLotSimulation PRIVATE glfw3::glfw)
elseif(GLFW_LIBRARY)
    target_include_directories(ParkingLotSimulation PRIVATE ${GLFW_INCLUDE_DIR})
    target_link_libraries(ParkingLotSimulation PRIVATE ${GLFW_LIBRARY})
else()
    message(FATAL_ERROR "GLFW library not found!")
endif()

target_link_libraries(ParkingLotSimulation PRIVATE
    Vulkan::Vulkan
)

if(APPLE)
    # Link Cocoa and IOKit required by GLFW on macOS
    target_link_libraries(ParkingLotSimulation PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework QuartzCore"
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(ParkingLotSimulation PRIVATE /W4)
else()
    target_compile_options(ParkingLotSimulation PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# -------------------------------------------------------------
# Testing Configuration
# -------------------------------------------------------------
enable_testing()

set(SIM_COMMON_SOURCES
    src/core/ParkingLot.cpp
    src/sim/Figures.cpp
    src/sim/CarParking.cpp
    src/sim/Simulation.cpp
)

set(UNIT_TEST_SOURCES
    tests/unit/test_vec.cpp
    tests/unit/test_vehicle_ticket.cpp
    tests/unit/test_parking_lot.cpp
    tests/unit/test_car_agent.cpp
)

set(INTEGRATION_TEST_SOURCES
    tests/integration/test_simulation_lifecycle.cpp
    tests/integration/test_gate_queue.cpp
    tests/integration/test_priority_and_unparking.cpp
    tests/integration/test_targeted_parking.cpp
)

set(STRESS_TEST_SOURCES
    tests/stress/test_stress_telemetry.cpp
)

# 1. Master test runner (all tests)
add_executable(run_all_tests
    tests/run_all_tests.cpp
    ${UNIT_TEST_SOURCES}
    ${INTEGRATION_TEST_SOURCES}
    ${STRESS_TEST_SOURCES}
    ${SIM_COMMON_SOURCES}
)
target_include_directories(run_all_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# 2. Unit tests runner
add_executable(unit_tests
    tests/unit/main_unit.cpp
    ${UNIT_TEST_SOURCES}
    ${SIM_COMMON_SOURCES}
)
target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# 3. Integration tests runner
add_executable(integration_tests
    tests/integration/main_integration.cpp
    ${INTEGRATION_TEST_SOURCES}
    ${SIM_COMMON_SOURCES}
)
target_include_directories(integration_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# 4. Stress tests runner
add_executable(stress_tests
    tests/stress/main_stress.cpp
    ${STRESS_TEST_SOURCES}
    ${SIM_COMMON_SOURCES}
)
target_include_directories(stress_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Register with CTest
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)
add_test(NAME StressTests COMMAND stress_tests)
add_test(NAME AllTests COMMAND run_all_tests)

