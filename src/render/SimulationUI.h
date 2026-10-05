#ifndef SIMULATION_UI_H
#define SIMULATION_UI_H

#include "../sim/Simulation.h"
#include <string>
#include <algorithm>

class SimulationUI {
public:
    SimulationUI();

    void initStyle();
    void render(Simulation& simulation);

    float getTopBarHeight() const { return topBarHeight_; }
    float getSidebarWidth() const { return sidebarWidth_; }
    void setSidebarWidth(float w) {
        sidebarWidth_ = std::max(280.0f, std::min(600.0f, w));
    }
    void adjustSidebarWidth(float delta) {
        setSidebarWidth(sidebarWidth_ + delta);
    }

    bool isSidebarOpen() const { return sidebarOpen_; }
    void setSidebarOpen(bool open) { sidebarOpen_ = open; }

private:
    float topBarHeight_ = 48.0f;
    float sidebarWidth_ = 370.0f;
    bool sidebarOpen_ = true;
    bool isDraggingSplitter_ = false;
    bool isMouseOverSplitter_ = false;
    int currentTab_ = 0; // 0: Manager & Receipt Dashboard, 1: Full History Log

    void renderStallOverlays(Simulation& simulation, float canvasX, float canvasY, float canvasW, float canvasH);
    void renderTopBar(Simulation& simulation);
    void renderSidebar(Simulation& simulation);
    void renderPanelResizeControls();
    void renderSpotGrid(Simulation& simulation);
    void renderReceiptCard(const std::string& receiptText, bool highlight = false);
};

#endif // SIMULATION_UI_H
