# Parking Lot Simulation (Vulkan Edition)

Modern 2D Parking Lot Simulation & Ticketing Management Engine rewritten in C++17 and Vulkan.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.1%2B-red.svg)](https://www.vulkan.org/)
[![GLFW](https://img.shields.io/badge/GLFW-3.5%2B-orange.svg)](https://www.glfw.org/)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-brightgreen.svg)](https://cmake.org/)

---

## Highlights & Features

- **Unified Single Project**: Consolidated the legacy standalone ticketing console (`P3_part_1`) and graphical simulator (`P3_part_2`) into a cohesive, modular architecture.
- **Modern Vulkan Pipeline**:
  - Rewritten from legacy fixed-function OpenGL/GLUT to modern **Vulkan** (SPIR-V vertex & fragment shaders, custom graphics pipelines, double buffering with fences and semaphores).
  - Native Apple Silicon / macOS support via **MoltenVK** and cross-platform **GLFW**.
  - Dynamic host-coherent vertex buffers with alpha blending and orthographic aspect-ratio preservation.
- **Integrated Parking & Ticketing Database**:
  - Real-time tracking of 10 parking stalls on a single floor.
  - Category-based parking (Vans, Cars, Motorbikes, Bicycles).
  - Arrival and departure tickets with duration and fee calculations.
- **Autonomous & Interactive Simulation**:
  - Full bidirectional animated vehicle maneuvers:
    - **Arrival**: Gate entry, aisle navigation, bay alignment, and reverse docking.
    - **Departure**: Pullout unparking, aisle cruising, cornering into western corridor, and exit gate departure.
  - **Zero-Collision Traffic Intelligence**:
    - **Lane Separation**: Inbound corridor ($x = -96$) and outbound corridor ($x = -110$) run in parallel with 14 units of lateral clearance. Bottom aisle separated into Eastbound ($y = -20$) and Westbound ($y = -32$) lanes.
    - **Adaptive Cruise Control (ACC)**: Moving vehicles sense cars ahead in their trajectory cone and smoothly yield/brake to maintain safe following distances.
    - **Unparking Cross-Traffic Yielding**: Vehicles in stalls check aisle clearance and hold until passing traffic has cleared before pulling out.
    - **Entrance Gate Queuing**: Consecutive arrivals queue cleanly outside the entrance gate and enter sequentially when the gate area is clear.
  - Interactive clickable buttons and hotkeys to spawn specific vehicles, depart cars, and pause/resume automated traffic.

---

## Architecture Overview

```
Parking-Lot-Simulation/
├── CMakeLists.txt              # Unified root CMake build configuration
├── shaders/
│   ├── shader.vert             # GLSL vertex shader (orthographic projection)
│   ├── shader.frag             # GLSL fragment shader
│   ├── vert.spv / frag.spv     # Compiled SPIR-V binaries
│   └── vert_spv.h / frag_spv.h # Embedded SPIR-V headers for portability
├── src/
│   ├── main.cpp                # Application entry point, event loop & input
│   ├── core/                   # Ticketing and database engine
│   │   ├── Vehicle.h           # Vehicle definitions, types, and rates
│   │   ├── Ticket.h            # Arrival and Departure ticket models
│   │   ├── ParkingLot.h        # 10-stall parking lot slot management
│   │   └── ParkingLot.cpp
│   ├── sim/                    # Simulation and animation logic
│   │   ├── Vec.h               # 2D Vector math with rotation operators
│   │   ├── Figures.h           # Geometric shapes (Vehicles, Stalls, Markings)
│   │   ├── Figures.cpp
│   │   ├── CarParking.h        # Vehicle pathing and docking/undocking state machine
│   │   ├── CarParking.cpp
│   │   ├── Simulation.h        # Coordinator between core database and visual agents
│   │   └── Simulation.cpp
│   └── render/                 # Graphics abstraction
│       ├── RenderTypes.h       # Vertex, Color, and RenderBatch data structures
│       ├── VulkanRenderer.h    # Vulkan instance, swapchain, pipelines, and commands
│       └── VulkanRenderer.cpp
```

---

## Building and Running

### Prerequisites

#### macOS (Homebrew)
```bash
brew install cmake molten-vk vulkan-headers vulkan-loader glfw shaderc
```

#### Linux (Debian / Ubuntu)
```bash
sudo apt update
sudo apt install build-essential cmake libvulkan-dev vulkan-tools libglfw3-dev glslc
```

### Build with CMake

```bash
# Configure build
cmake -B build -S .

# Compile
cmake --build build -j
```

### Running the Application

```bash
# Interactive GUI mode
./build/ParkingLotSimulation

# Automated test / verification mode (runs N frames then exits)
./build/ParkingLotSimulation --test 10
```

### Running Automated Unit, Integration & Stress Tests

The project includes a comprehensive, dependency-free test suite with **31 rigorous tests** covering all modules:

```bash
# Run all tests via CTest
ctest --test-dir build --output-on-failure

# Or run individual test runner binaries directly:
./build/run_all_tests          # Complete test suite (Unit + Integration + Stress)
./build/unit_tests             # 20 Unit tests (Vec, Vehicle, Ticket, ParkingLot, CarAgent)
./build/integration_tests      # 8 Integration tests (Queues, Priority, Spot targeting, Lifecycle)
./build/stress_tests           # 3 Stress tests (Adjacent stalls, Wave departure, 5000 frames)
```


---

## Controls & Interaction

The simulation provides **both intuitive on-screen clickable buttons** (via Dear ImGui rendered natively in Vulkan) and **keyboard shortcuts**:

### 1. Integrated Modern UI Layout

The UI is built directly into the application window with zero detached floating dialogs:

- **Integrated Top Bar**:
  - **`🚗 Car ($15)`**, **`🚐 Van ($20)`**, **`🏍️ Moto ($10)`**, **`🚲 Bike ($5)`**: Issue ticket and park that specific vehicle type.
  - **`🎲 Random`**: Spawn random vehicle arrival.
  - **`🚪 Depart Vehicle`**: Depart a vehicle and calculate duration/fee.
  - **`▶️ / ⏸️ Auto Traffic`**: Toggle background automated traffic.
  - **`◫ Panel [ON/OFF]`**: Toggle the integrated side dashboard.
- **Integrated Resizable Sidebar (Right)**:
  - **Draggable Edge Resizer**: Click and drag the left border of the sidebar to freely adjust its width (280px to 600px); double-click to reset to standard 370px.
  - **Width Slider & Presets**: Built-in width slider and quick presets (`Compact 300px`, `Standard 370px`, `Wide 460px`).
  - **Dynamic Scaling**: The 2D parking lot automatically shifts and scales with generous breathing margins so it **never overlaps with the panel**.
  - **`[📋 Manager & Receipt]` Tab**:
    - **Live Occupancy Gauge**: Color-coded capacity bar (Green/Yellow/Red) with vehicle breakdown counts (Car/Van/Moto/Bike).
    - **Interactive 10-Stall Grid**: Click free spots (1-10) to park directly; click occupied spots to depart!
    - **Simulation Controls**: Event rate slider, Reset Lot button, Print Status button.
    - **Live Receipt Card**: Instant high-contrast receipt for the latest arrival or departure.
  - **`[📜 All Receipts]` Tab**:
    - Full-height scrollable receipt history log with "Clear History" button and gate queue count.


### 2. Keyboard Shortcuts (Optional)

| Key | Action |
|:---:|:---|
| **`A`** | Trigger random vehicle arrival (allocates spot, prints ticket, animates parking) |
| **`1`** | Spawn Van arrival ($20 / day) |
| **`2`** | Spawn Car arrival ($15 / day) |
| **`3`** | Spawn Motorbike arrival ($10 / day) |
| **`4`** | Spawn Bicycle arrival ($5 / day) |
| **`D`** | Trigger vehicle departure (calculates fee, prints departure ticket, exits lot) |
| **`P`** | Print current Parking Lot status table to terminal |
| **`Space`** | Toggle Automated Simulation (starts/pauses automated traffic, OFF by default) |
| **`Esc`** | Exit application |

---

## Sample Tickets

### Arrival Ticket
```
========================================
            ARRIVAL TICKET              
========================================
Vehicle ID:          1
Vehicle Type:        car
Time of Arrival:     10/4/2026
Allocated Spot:      Space 5
Daily Rate:          $15.00
========================================
```

### Departure Ticket
```
========================================
           DEPARTURE TICKET             
========================================
Vehicle ID:          1
Vehicle Type:        car
Time of Arrival:     10/4/2026
Time of Departure:   10/7/2026
Duration:            0 years, 0 months, 3 days (3 total days)
Spot Released:       Space 5
Total Price:         $45.00
========================================
```

---

## License & Attribution
Originally created for UM-SJTU Joint Institute VE101. Fully modernized, refactored into a unified architecture, and rewritten with Vulkan.
