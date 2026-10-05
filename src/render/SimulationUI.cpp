#include "SimulationUI.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

SimulationUI::SimulationUI() {}

void SimulationUI::initStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding & Spacing
    style.WindowRounding    = 0.0f; // Docked panels seamlessly touch window edges
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 5.0f;

    style.WindowPadding     = ImVec2(12, 12);
    style.FramePadding      = ImVec2(8, 5);
    style.ItemSpacing       = ImVec2(8, 7);

    // Modern Dark Slate Palette
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.09f, 0.10f, 0.13f, 0.98f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.12f, 0.14f, 0.18f, 0.85f);
    colors[ImGuiCol_Border]             = ImVec4(0.20f, 0.24f, 0.32f, 0.70f);

    colors[ImGuiCol_Header]             = ImVec4(0.18f, 0.24f, 0.34f, 0.85f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.25f, 0.33f, 0.48f, 0.95f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.32f, 0.42f, 0.60f, 1.00f);

    colors[ImGuiCol_Button]             = ImVec4(0.18f, 0.23f, 0.32f, 0.90f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.26f, 0.34f, 0.48f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.16f, 0.42f, 0.80f, 1.00f);

    colors[ImGuiCol_FrameBg]            = ImVec4(0.14f, 0.16f, 0.22f, 0.90f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.20f, 0.24f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.25f, 0.30f, 0.40f, 1.00f);

    colors[ImGuiCol_Tab]                = ImVec4(0.13f, 0.16f, 0.22f, 0.90f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.25f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.30f, 0.45f, 1.00f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.10f, 0.12f, 0.16f, 0.80f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.22f, 0.32f, 1.00f);

    colors[ImGuiCol_PlotHistogram]      = ImVec4(0.20f, 0.75f, 0.40f, 1.00f);
}

void SimulationUI::render(Simulation& simulation) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float vpW = viewport->WorkSize.x;
    float vpH = viewport->WorkSize.y;

    float minSidebarW = 280.0f;
    float maxSidebarW = std::min(600.0f, vpW * 0.60f);
    sidebarWidth_ = std::max(minSidebarW, std::min(maxSidebarW, sidebarWidth_));

    // 0. Handle interactive draggable splitter on left border of sidebar BEFORE computing canvas width
    isMouseOverSplitter_ = false;
    if (sidebarOpen_) {
        float actualSidebarW = sidebarWidth_;
        float sidebarX = viewport->WorkPos.x + vpW - actualSidebarW;
        float sidebarY = viewport->WorkPos.y + topBarHeight_;
        float sidebarH = vpH - topBarHeight_;

        ImVec2 mousePos = ImGui::GetMousePos();
        float splitterHitRadius = 6.0f;
        isMouseOverSplitter_ = (mousePos.x >= sidebarX - splitterHitRadius &&
                               mousePos.x <= sidebarX + splitterHitRadius &&
                               mousePos.y >= sidebarY && mousePos.y <= sidebarY + sidebarH);

        if (isMouseOverSplitter_ || isDraggingSplitter_) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        if (isMouseOverSplitter_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            isDraggingSplitter_ = true;
        }

        if (isDraggingSplitter_) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
                sidebarWidth_ -= mouseDeltaX; // Dragging left increases width
                sidebarWidth_ = std::max(minSidebarW, std::min(maxSidebarW, sidebarWidth_));
            } else {
                isDraggingSplitter_ = false;
            }
        }

        // Double click splitter to reset width to default 370px
        if (isMouseOverSplitter_ && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            sidebarWidth_ = 370.0f;
        }
    }

    float actualSidebarW = sidebarOpen_ ? sidebarWidth_ : 0.0f;
    float canvasX = 0.0f;
    float canvasY = topBarHeight_;
    float canvasW = std::max(100.0f, vpW - actualSidebarW);
    float canvasH = std::max(100.0f, vpH - topBarHeight_);

    // 1. Render parking stall hover highlights, badges, and interactive click triggers in canvas area
    renderStallOverlays(simulation, canvasX, canvasY, canvasW, canvasH);

    // 2. Render integrated Top Bar (pinned to window top)
    renderTopBar(simulation);

    // 3. Render integrated Sidebar (pinned to window right with drag resize)
    if (sidebarOpen_) {
        renderSidebar(simulation);
    }
}


void SimulationUI::renderStallOverlays(Simulation& simulation, float canvasX, float canvasY, float canvasW, float canvasH) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Must match projection in VulkanRenderer.cpp exactly
    float aspect = canvasW / canvasH;
    float baseRangeX = 150.0f;
    float baseRangeY = 115.0f;
    float baseAspect = baseRangeX / baseRangeY;

    float rangeX, rangeY;
    if (aspect > baseAspect) {
        rangeY = baseRangeY;
        rangeX = rangeY * aspect;
    } else {
        rangeX = baseRangeX;
        rangeY = rangeX / aspect;
    }

    auto simToScreen = [&](float sx, float sy) -> ImVec2 {
        float px = canvasX + (sx / rangeX * 0.5f + 0.5f) * canvasW;
        float py = canvasY + (-sy / rangeY * 0.5f + 0.5f) * canvasH;
        return ImVec2(px, py);
    };

    const auto& lot = simulation.getParkingLot();
    ImVec2 mousePos = ImGui::GetMousePos();
    bool anyWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);

    for (int s = 0; s < ParkingLot::SPOTS_PER_FLOOR; ++s) {
        float spotX = (s <= 4) ? (-68.0f + 34.0f * s) : (238.0f - 34.0f * s);
        float spotY = (s <= 4) ? -50.0f : 80.0f;

        // Stall bounding box in screen coordinates
        float worldYMin = (s <= 4) ? -60.0f : 70.0f;
        float worldYMax = (s <= 4) ? -40.0f : 90.0f;
        ImVec2 p0 = simToScreen(spotX - 16.0f, worldYMax); // Top-left
        ImVec2 p1 = simToScreen(spotX + 16.0f, worldYMin); // Bottom-right
        float x0 = std::min(p0.x, p1.x);
        float x1 = std::max(p0.x, p1.x);
        float y0 = std::min(p0.y, p1.y);
        float y1 = std::max(p0.y, p1.y);

        // Only register hover if mouse is strictly inside the canvas area and not over ImGui panels or splitter
        bool isHovered = (!anyWindowHovered && !isDraggingSplitter_ &&
                          mousePos.x >= x0 && mousePos.x <= x1 && 
                          mousePos.y >= y0 && mousePos.y <= y1 &&
                          mousePos.x >= canvasX && mousePos.x <= canvasX + canvasW - 4.0f &&
                          mousePos.y >= canvasY && mousePos.y <= canvasY + canvasH);

        const auto* veh = lot.getVehicle(1, s);
        bool isDeparting = veh ? simulation.isVehicleDeparting(veh->id) : false;
        bool isDriving = veh ? simulation.isVehicleDriving(veh->id) : false;
        int spotNum = s + 1; // 1 to 10

        // Subtle stall watermark number when vacant
        if (!veh) {
            char numStr[4];
            std::snprintf(numStr, sizeof(numStr), "%d", spotNum);
            ImVec2 centerPos = simToScreen(spotX, spotY);
            ImVec2 nSize = ImGui::CalcTextSize(numStr);
            drawList->AddText(ImVec2(centerPos.x - nSize.x * 0.5f, centerPos.y - nSize.y * 0.5f),
                              IM_COL32(200, 220, 240, 75), numStr);
        }

        // Visual hover highlight
        if (isHovered) {
            drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(80, 180, 255, 40), 4.0f);
            drawList->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(100, 200, 255, 230), 4.0f, 0, 2.0f);

            // Click handling in canvas
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (veh) {
                    if (!isDeparting && !isDriving) {
                        simulation.triggerDeparture(veh->id);
                    }
                } else {
                    const auto* spotObj = lot.getSpot(1, s);
                    VehicleType type = spotObj ? spotObj->preferredType : VehicleType::CAR;
                    simulation.triggerArrival(type, s);
                }
            }

            // Interactive tooltip
            ImGui::BeginTooltip();
            if (veh) {
                if (isDeparting) {
                    ImGui::Text("Spot %d: Vehicle #%d (%s) departing...", spotNum, veh->id, vehicleTypeName(veh->type).c_str());
                } else if (isDriving) {
                    ImGui::Text("Spot %d: Vehicle #%d (%s) parking...", spotNum, veh->id, vehicleTypeName(veh->type).c_str());
                } else {
                    ImGui::Text("Spot %d: Occupied by #%d (%s)\nClick to Depart!", spotNum, veh->id, vehicleTypeName(veh->type).c_str());
                }
            } else {
                const char* desc = (s < 2) ? "Motorbike" : ((s < 4) ? "Bicycle" : ((s < 8) ? "Car" : "Van"));
                ImGui::Text("Spot %d: Vacant (%s spot)\nClick to Park in Spot %d!", spotNum, desc, spotNum);
            }
            ImGui::EndTooltip();
        }

        // Header pill badge at back curb
        float badgeWorldY = (s <= 4) ? -64.0f : 88.0f;
        ImVec2 badgeCenter = simToScreen(spotX, badgeWorldY);

        char badgeText[48];
        ImU32 badgeBg, badgeBorder, badgeDot;

        if (veh) {
            if (isDeparting) {
                std::snprintf(badgeText, sizeof(badgeText), "%d : #%d Exit", spotNum, veh->id);
                badgeBg = IM_COL32(45, 30, 15, 230);
                badgeBorder = IM_COL32(230, 140, 40, 230);
                badgeDot = IM_COL32(255, 170, 50, 255);
            } else if (isDriving) {
                std::snprintf(badgeText, sizeof(badgeText), "%d : #%d Park", spotNum, veh->id);
                badgeBg = IM_COL32(15, 35, 55, 230);
                badgeBorder = IM_COL32(40, 150, 240, 230);
                badgeDot = IM_COL32(70, 180, 255, 255);
            } else {
                std::snprintf(badgeText, sizeof(badgeText), "%d : #%d %s", spotNum, veh->id, vehicleTypeName(veh->type).substr(0, 4).c_str());
                badgeBg = IM_COL32(45, 18, 18, 230);
                badgeBorder = IM_COL32(220, 60, 60, 230);
                badgeDot = IM_COL32(255, 80, 80, 255);
            }
        } else {
            const char* typeTag = (s < 2) ? "Moto" : ((s < 4) ? "Bike" : ((s < 8) ? "Car" : "Van"));
            std::snprintf(badgeText, sizeof(badgeText), "%d : %s", spotNum, typeTag);
            badgeBg = IM_COL32(18, 32, 22, 220);
            badgeBorder = IM_COL32(50, 180, 85, 200);
            badgeDot = IM_COL32(70, 220, 110, 255);
        }

        ImVec2 txtSize = ImGui::CalcTextSize(badgeText);
        float padX = 9.0f;
        float padY = 4.0f;
        float pillW = txtSize.x + padX * 2.0f + 10.0f;
        float pillH = txtSize.y + padY * 2.0f;

        ImVec2 pillMin(badgeCenter.x - pillW * 0.5f, badgeCenter.y - pillH * 0.5f);
        ImVec2 pillMax(badgeCenter.x + pillW * 0.5f, badgeCenter.y + pillH * 0.5f);

        drawList->AddRectFilled(pillMin, pillMax, badgeBg, 5.0f);
        drawList->AddRect(pillMin, pillMax, badgeBorder, 5.0f, 0, 1.5f);

        ImVec2 dotPos(pillMin.x + padX + 3.0f, badgeCenter.y);
        drawList->AddCircleFilled(dotPos, 3.5f, badgeDot);

        ImVec2 textPos(pillMin.x + padX + 11.0f, badgeCenter.y - txtSize.y * 0.5f);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 250), badgeText);
    }
}

void SimulationUI::renderTopBar(Simulation& simulation) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, topBarHeight_), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.12f, 1.0f));

    if (ImGui::Begin("##IntegratedTopBar", nullptr, flags)) {
        // App Branding
        ImGui::AlignTextToFramePadding();
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "PARKING SIMULATOR");
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // 1. Vehicle Arrival Buttons
        if (ImGui::Button("Car ($15)")) {
            simulation.triggerArrival(VehicleType::CAR);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Park a standard passenger Car ($15/day)");

        ImGui::SameLine();
        if (ImGui::Button("Van ($20)")) {
            simulation.triggerArrival(VehicleType::VAN);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Park a commercial Van ($20/day)");

        ImGui::SameLine();
        if (ImGui::Button("Moto ($10)")) {
            simulation.triggerArrival(VehicleType::MOTORBIKE);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Park a Motorbike ($10/day)");

        ImGui::SameLine();
        if (ImGui::Button("Bike ($5)")) {
            simulation.triggerArrival(VehicleType::BICYCLE);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Park a Bicycle ($5/day)");

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.35f, 0.55f, 0.9f));
        if (ImGui::Button("Random")) {
            simulation.triggerArrival();
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Random vehicle arrival");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // 2. Departure Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.22f, 0.22f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.28f, 0.28f, 1.0f));
        if (ImGui::Button("Depart Vehicle")) {
            simulation.triggerDeparture();
        }
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Process a parked vehicle departure & issue receipt");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // 3. Auto Traffic Toggle Button
        bool isAuto = simulation.isAutoMode();
        std::string autoBtnText = isAuto ? "Auto: ON" : "Auto: OFF";
        ImGui::PushStyleColor(ImGuiCol_Button, isAuto ? ImVec4(0.18f, 0.55f, 0.25f, 0.9f) : ImVec4(0.30f, 0.33f, 0.38f, 0.8f));
        if (ImGui::Button(autoBtnText.c_str())) {
            simulation.toggleAutoMode();
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle automatic vehicle traffic generation");

        // 4. Right-side Panel Toggle
        float toggleBtnW = 95.0f;
        ImGui::SameLine(viewport->WorkSize.x - toggleBtnW - 12.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, sidebarOpen_ ? ImVec4(0.20f, 0.35f, 0.55f, 0.85f) : ImVec4(0.25f, 0.28f, 0.35f, 0.8f));
        if (ImGui::Button(sidebarOpen_ ? "Panel [ON]" : "Panel [OFF]", ImVec2(toggleBtnW, 0))) {
            sidebarOpen_ = !sidebarOpen_;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show or hide the integrated manager & receipt panel");
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // Subtle bottom hairline separator beneath top bar
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddLine(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + topBarHeight_),
                      ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + topBarHeight_),
                      IM_COL32(50, 60, 80, 180), 1.0f);
}

void SimulationUI::renderSidebar(Simulation& simulation) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float vpW = viewport->WorkSize.x;
    float vpH = viewport->WorkSize.y;
    float actualSidebarW = std::min(sidebarWidth_, vpW * 0.60f);
    float sidebarX = viewport->WorkPos.x + vpW - actualSidebarW;
    float sidebarY = viewport->WorkPos.y + topBarHeight_;
    float sidebarH = vpH - topBarHeight_;


    ImGui::SetNextWindowPos(ImVec2(sidebarX, sidebarY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(actualSidebarW, sidebarH), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.10f, 0.13f, 0.98f));

    if (ImGui::Begin("##IntegratedSidebar", nullptr, flags)) {
        // Draw dividing vertical line with hover/drag highlight
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 splitterColor = isDraggingSplitter_ ? IM_COL32(100, 180, 255, 255) :
                              (isMouseOverSplitter_ ? IM_COL32(80, 160, 240, 200) : IM_COL32(50, 60, 80, 200));
        float splitterThick = (isDraggingSplitter_ || isMouseOverSplitter_) ? 2.5f : 1.0f;
        drawList->AddLine(ImVec2(sidebarX, sidebarY), ImVec2(sidebarX, sidebarY + sidebarH), splitterColor, splitterThick);

        // Sidebar Header
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "PARKING LOT MANAGER");
        ImGui::SameLine();
        const auto& lot = simulation.getParkingLot();
        int occupancy = lot.getOccupancy();
        int capacity = lot.getCapacity();
        ImGui::TextDisabled("(%d/%d Spots)", occupancy, capacity);

        ImGui::Spacing();

        // Integrated Tabs: [Manager & Receipt] vs [All Receipts]
        if (ImGui::BeginTabBar("SidebarTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Manager & Receipt")) {
                currentTab_ = 0;

                // 1. Occupancy Progress Bar
                float fraction = static_cast<float>(occupancy) / static_cast<float>(capacity);
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%d / %d Spots (%.0f%%)", occupancy, capacity, fraction * 100.0f);
                
                // Color gradient based on occupancy
                ImVec4 barColor = (fraction < 0.5f) ? ImVec4(0.20f, 0.75f, 0.40f, 1.0f) :
                                  ((fraction < 0.8f) ? ImVec4(0.95f, 0.75f, 0.20f, 1.0f) : ImVec4(0.90f, 0.25f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                ImGui::ProgressBar(fraction, ImVec2(-1, 18), buf);
                ImGui::PopStyleColor();

                // Breakdown Tags
                int cars = 0, vans = 0, motos = 0, bikes = 0;
                for (int s = 0; s < capacity; ++s) {
                    const auto* v = lot.getVehicle(1, s);
                    if (v) {
                        if (v->type == VehicleType::CAR) cars++;
                        else if (v->type == VehicleType::VAN) vans++;
                        else if (v->type == VehicleType::MOTORBIKE) motos++;
                        else if (v->type == VehicleType::BICYCLE) bikes++;
                    }
                }
                ImGui::TextDisabled("Vehicles: Car:%d | Van:%d | Moto:%d | Bike:%d", cars, vans, motos, bikes);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 2. Interactive Stalls Grid (Direct spot selection)
                ImGui::Text("Click Spot to Park / Depart:");
                renderSpotGrid(simulation);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 3. Simulation & Panel Controls
                ImGui::Text("Simulation Controls:");
                float interval = simulation.getAutoInterval();
                if (ImGui::SliderFloat("Speed", &interval, 0.5f, 6.0f, "%.1fs")) {
                    simulation.setAutoInterval(interval);
                }

                if (ImGui::Button("Reset Lot", ImVec2(95, 24))) {
                    simulation.resetLot();
                }
                ImGui::SameLine();
                if (ImGui::Button("Print Status", ImVec2(95, 24))) {
                    simulation.getParkingLot().printStatus();
                }

                ImGui::Spacing();
                renderPanelResizeControls();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 4. Live Receipt Preview
                ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "LATEST RECEIPT:");
                if (simulation.getArrivalQueueSize() > 0) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "[Gate Queue: %zu]", simulation.getArrivalQueueSize());
                }

                const auto& logs = simulation.getTicketLog();
                if (logs.empty()) {
                    ImGui::TextDisabled("No tickets issued yet.\nClick any spot or arrival button above!");
                } else {
                    renderReceiptCard(logs.back(), true);
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("All Receipts")) {
                currentTab_ = 1;

                const auto& logs = simulation.getTicketLog();

                if (ImGui::Button("Clear History")) {
                    simulation.clearLog();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(%zu tickets logged)", logs.size());

                if (simulation.getArrivalQueueSize() > 0) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Queue: %zu", simulation.getArrivalQueueSize());
                }

                ImGui::Separator();

                if (logs.empty()) {
                    ImGui::TextDisabled("No tickets logged yet.");
                } else {
                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "LATEST RECEIPT:");
                    renderReceiptCard(logs.back(), true);

                    ImGui::Spacing();
                    ImGui::Text("History Log (Most Recent First):");
                    ImGui::BeginChild("TicketHistoryScrollBox", ImVec2(0, 0), true);
                    for (auto it = logs.rbegin(); it != logs.rend(); ++it) {
                        renderReceiptCard(*it, false);
                        ImGui::Spacing();
                    }
                    ImGui::EndChild();
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void SimulationUI::renderPanelResizeControls() {
    if (ImGui::TreeNode("Panel Width & Display Settings")) {
        float currentW = sidebarWidth_;
        if (ImGui::SliderFloat("Width", &currentW, 280.0f, 600.0f, "%.0f px")) {
            setSidebarWidth(currentW);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adjust sidebar width (or drag the left border of this panel)");
        }

        // Quick Preset Width Buttons
        ImGui::TextDisabled("Presets:");
        ImGui::SameLine();
        if (ImGui::SmallButton("Compact (300px)")) {
            setSidebarWidth(300.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Standard (370px)")) {
            setSidebarWidth(370.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Wide (460px)")) {
            setSidebarWidth(460.0f);
        }

        ImGui::TextDisabled("(Tip: Drag the left edge of this panel to resize)");
        ImGui::TreePop();
    }
}

void SimulationUI::renderReceiptCard(const std::string& receiptText, bool highlight) {
    ImVec4 borderCol = highlight ? ImVec4(0.85f, 0.65f, 0.20f, 0.8f) : ImVec4(0.25f, 0.30f, 0.40f, 0.6f);
    ImVec4 bgCol = highlight ? ImVec4(0.12f, 0.15f, 0.20f, 0.95f) : ImVec4(0.11f, 0.13f, 0.17f, 0.90f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgCol);
    ImGui::PushStyleColor(ImGuiCol_Border, borderCol);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

    float boxHeight = highlight ? 145.0f : 135.0f;
    std::string childId = "ReceiptCard_" + std::to_string(reinterpret_cast<uintptr_t>(&receiptText));
    if (ImGui::BeginChild(childId.c_str(), ImVec2(0, boxHeight), true, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushStyleColor(ImGuiCol_Text, highlight ? ImVec4(0.92f, 0.95f, 1.0f, 1.0f) : ImVec4(0.75f, 0.80f, 0.88f, 1.0f));
        ImGui::TextUnformatted(receiptText.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void SimulationUI::renderSpotGrid(Simulation& simulation) {
    auto& lot = simulation.getParkingLot();

    float availW = ImGui::GetContentRegionAvail().x;
    float btnSpacing = 6.0f;
    float btnW = std::max(46.0f, std::min(70.0f, (availW - 4.0f * btnSpacing) / 5.0f));
    float btnH = 36.0f;

    ImGui::TextDisabled("Top Row (Spots 10-6):");
    for (int s = 9; s >= 5; --s) {
        if (s < 9) ImGui::SameLine(0, btnSpacing);
        const auto* veh = lot.getVehicle(1, s);
        int spotNum = s + 1; // 10, 9, 8, 7, 6
        char label[32];
        if (veh) {
            bool isDeparting = simulation.isVehicleDeparting(veh->id);
            bool isDriving = simulation.isVehicleDriving(veh->id);

            if (isDeparting) {
                std::snprintf(label, sizeof(label), "%d\nExit", spotNum);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.15f, 0.9f));
                ImGui::Button(label, ImVec2(btnW, btnH));
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Spot %d: Vehicle #%d is departing...", spotNum, veh->id);
                }
            } else if (isDriving) {
                std::snprintf(label, sizeof(label), "%d\nPark", spotNum);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.9f));
                ImGui::Button(label, ImVec2(btnW, btnH));
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Spot %d: Vehicle #%d is en route to park...", spotNum, veh->id);
                }
            } else {
                std::snprintf(label, sizeof(label), "%d:#%d\n%s", spotNum, veh->id, vehicleTypeName(veh->type).substr(0, 4).c_str());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.9f));
                if (ImGui::Button(label, ImVec2(btnW, btnH))) {
                    simulation.triggerDeparture(veh->id);
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Spot %d: Occupied by #%d (%s)\nClick to Depart!", spotNum, veh->id, vehicleTypeName(veh->type).c_str());
                }
            }
        } else {
            std::snprintf(label, sizeof(label), "%d\nFree", spotNum);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.2f, 0.8f));
            if (ImGui::Button(label, ImVec2(btnW, btnH))) {
                const auto* spotObj = lot.getSpot(1, s);
                VehicleType type = spotObj ? spotObj->preferredType : ((s < 8) ? VehicleType::CAR : VehicleType::VAN);
                simulation.triggerArrival(type, s);
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Spot %d: Vacant (%s spot)\nClick to Park in Spot %d!", spotNum, (s < 8) ? "Car" : "Van", spotNum);
            }
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Bottom Row (Spots 1-5):");
    for (int s = 0; s < 5; ++s) {
        if (s > 0) ImGui::SameLine(0, btnSpacing);
        const auto* veh = lot.getVehicle(1, s);
        int spotNum = s + 1; // 1, 2, 3, 4, 5
        char label[32];
        if (veh) {
            bool isDeparting = simulation.isVehicleDeparting(veh->id);
            bool isDriving = simulation.isVehicleDriving(veh->id);

            if (isDeparting) {
                std::snprintf(label, sizeof(label), "%d\nExit", spotNum);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.15f, 0.9f));
                ImGui::Button(label, ImVec2(btnW, btnH));
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Spot %d: Vehicle #%d is departing...", spotNum, veh->id);
                }
            } else if (isDriving) {
                std::snprintf(label, sizeof(label), "%d\nPark", spotNum);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.9f));
                ImGui::Button(label, ImVec2(btnW, btnH));
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Spot %d: Vehicle #%d is en route to park...", spotNum, veh->id);
                }
            } else {
                std::snprintf(label, sizeof(label), "%d:#%d\n%s", spotNum, veh->id, vehicleTypeName(veh->type).substr(0, 4).c_str());
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.9f));
                if (ImGui::Button(label, ImVec2(btnW, btnH))) {
                    simulation.triggerDeparture(veh->id);
                }
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Spot %d: Occupied by #%d (%s)\nClick to Depart!", spotNum, veh->id, vehicleTypeName(veh->type).c_str());
                }
            }
        } else {
            std::snprintf(label, sizeof(label), "%d\nFree", spotNum);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.2f, 0.8f));
            if (ImGui::Button(label, ImVec2(btnW, btnH))) {
                const auto* spotObj = lot.getSpot(1, s);
                VehicleType type = spotObj ? spotObj->preferredType : VehicleType::CAR;
                simulation.triggerArrival(type, s);
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                const char* desc = (s < 2) ? "Moto" : ((s < 4) ? "Bike" : "Car");
                ImGui::SetTooltip("Spot %d: Vacant (%s spot)\nClick to Park in Spot %d!", spotNum, desc, spotNum);
            }
        }
    }
}
