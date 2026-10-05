#include "TestHarness.h"
#include "sim/Simulation.h"
#include <map>
#include <cmath>

namespace {

struct VehicleTracker {
    Vec lastPos;
    Vec lastAnc;
    int stallFrames = 0;
    bool hasLast = false;
};

class TelemetryMonitor {
public:
    int totalFramesChecked = 0;
    int collisionCount = 0;
    int suddenMoveCount = 0;
    int suddenTurnCount = 0;
    int deadlockCount = 0;
    float maxRecordedStep = 0.0f;
    float minRecordedDistance = 9999.0f;

    void update(const Simulation& sim, int frame) {
        totalFramesChecked++;
        const auto& agents = sim.getActiveAgents();

        // 1. Check pairwise inter-vehicle distance
        for (size_t i = 0; i < agents.size(); ++i) {
            for (size_t j = i + 1; j < agents.size(); ++j) {
                const auto& a1 = agents[i];
                const auto& a2 = agents[j];
                if (!a1.isMoving() || !a2.isMoving()) continue;

                float dist = (a1.getPosition() - a2.getPosition()).length();
                if (dist < minRecordedDistance) {
                    minRecordedDistance = dist;
                }

                // Minimum safety physical distance: 8.0 units
                if (dist < 8.0f) {
                    std::cerr << "[TELEMETRY COLLISION] frame " << frame 
                              << " between #" << a1.getVehicleId() << " and #" << a2.getVehicleId()
                              << " dist=" << dist << "\n";
                    collisionCount++;
                }
            }
        }

        // 2. Check each vehicle for sudden movement / teleportation or sharp snaps
        std::map<int, bool> activeIds;
        for (const auto& a : agents) {
            int id = a.getVehicleId();
            activeIds[id] = true;
            Vec pos = a.getPosition();
            Vec anc = a.getAnchor();

            if (trackers_.count(id) && trackers_[id].hasLast) {
                auto& tr = trackers_[id];

                // Displacement delta
                float stepDist = (pos - tr.lastPos).length();
                if (stepDist > maxRecordedStep) {
                    maxRecordedStep = stepDist;
                }

                // Normal step size is ~0.5 units/frame. Any jump > 1.2 units is teleportation
                if (stepDist > 1.2f) {
                    std::cerr << "[TELEMETRY SUDDEN MOVE] frame " << frame
                              << " #" << id << " moved " << stepDist << " units in 1 frame!\n";
                    suddenMoveCount++;
                }

                // Angular orientation delta
                Vec fwdCurrent = anc.normalized();
                Vec fwdLast = tr.lastAnc.normalized();
                float dot = std::max(-1.0f, std::min(1.0f, fwdCurrent.dot(fwdLast)));
                float angleDeg = std::acos(dot) * (180.0f / PI);

                if (stepDist > 0.01f && angleDeg > 30.0f) {
                    std::cerr << "[TELEMETRY SUDDEN ROTATION] frame " << frame
                              << " #" << id << " rotated " << angleDeg << " deg in 1 frame!\n";
                    suddenTurnCount++;
                }

                // Deadlock check: vehicle on the road not advancing
                if (a.isMoving() && a.getState() != CarState::UNPARKING_BOTTOM_PULLOUT && a.getState() != CarState::UNPARKING_TOP_PULLOUT) {
                    if (stepDist < 0.001f) {
                        tr.stallFrames++;
                        if (tr.stallFrames > 200) {
                            std::cerr << "[TELEMETRY DEADLOCK] frame " << frame
                                      << " #" << id << " frozen for " << tr.stallFrames << " frames!\n";
                            deadlockCount++;
                        }
                    } else {
                        tr.stallFrames = 0;
                    }
                } else {
                    tr.stallFrames = 0;
                }

                tr.lastPos = pos;
                tr.lastAnc = anc;
            } else {
                trackers_[id].lastPos = pos;
                trackers_[id].lastAnc = anc;
                trackers_[id].hasLast = true;
                trackers_[id].stallFrames = 0;
            }
        }

        // Clean up departed vehicles from tracking
        for (auto it = trackers_.begin(); it != trackers_.end();) {
            if (!activeIds.count(it->first)) {
                it = trackers_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    std::map<int, VehicleTracker> trackers_;
};

} // anonymous namespace

TEST_CASE(Stress_Telemetry, AdjacentStallConflicts) {
    TelemetryMonitor monitor;

    // Test adjacent stalls along bottom (0..3) and top (5..8)
    for (int s = 0; s < 9; ++s) {
        if (s == 4) continue; // Boundary between bottom and top rows
        int spotA = s;
        int spotB = s + 1;

        Simulation sim;
        for (int i = 0; i <= spotB; ++i) {
            VehicleType t = (i < 2) ? VehicleType::MOTORBIKE : ((i < 4) ? VehicleType::BICYCLE : ((i < 8) ? VehicleType::CAR : VehicleType::VAN));
            sim.triggerArrival(t);
        }

        for (int f = 0; f < 3000; ++f) {
            sim.update(0.016f);
            monitor.update(sim, f);
            if (sim.getParkingLot().getOccupancy() == spotB + 1 && sim.getActiveAgents().empty()) break;
        }

        int idA = sim.getParkingLot().getVehicle(1, spotA)->id;
        sim.triggerDeparture(idA);
        for (int f = 0; f < 2000; ++f) {
            sim.update(0.016f);
            monitor.update(sim, f);
            if (!sim.getParkingLot().isSpotOccupied(1, spotA)) break;
        }

        // Arrival for A, simultaneous departure for B
        VehicleType typeA = (spotA < 2) ? VehicleType::MOTORBIKE : ((spotA < 4) ? VehicleType::BICYCLE : ((spotA < 8) ? VehicleType::CAR : VehicleType::VAN));
        sim.triggerArrival(typeA);
        int idB = sim.getParkingLot().getVehicle(1, spotB)->id;
        sim.triggerDeparture(idB);

        for (int f = 0; f < 2500; ++f) {
            sim.update(0.016f);
            monitor.update(sim, f);
        }
    }

    ASSERT_EQ(monitor.collisionCount, 0);
    ASSERT_EQ(monitor.suddenMoveCount, 0);
    ASSERT_EQ(monitor.suddenTurnCount, 0);
    ASSERT_EQ(monitor.deadlockCount, 0);
}

TEST_CASE(Stress_Telemetry, FullLotWaveDeparture) {
    TelemetryMonitor monitor;
    Simulation sim;

    for (int i = 0; i < 10; ++i) {
        VehicleType t = (i < 2) ? VehicleType::MOTORBIKE : ((i < 4) ? VehicleType::BICYCLE : ((i < 8) ? VehicleType::CAR : VehicleType::VAN));
        sim.triggerArrival(t);
    }

    while (sim.getParkingLot().getOccupancy() < 10 || !sim.getActiveAgents().empty() || sim.getArrivalQueueSize() > 0) {
        sim.update(0.016f);
        monitor.update(sim, monitor.totalFramesChecked);
    }
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 10);

    for (int s = 0; s < 10; ++s) {
        const auto* v = sim.getParkingLot().getVehicle(1, s);
        if (v) sim.triggerDeparture(v->id);
    }

    while (sim.getParkingLot().getOccupancy() > 0 || !sim.getActiveAgents().empty()) {
        sim.update(0.016f);
        monitor.update(sim, monitor.totalFramesChecked);
    }
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 0);

    ASSERT_EQ(monitor.collisionCount, 0);
    ASSERT_EQ(monitor.suddenMoveCount, 0);
    ASSERT_EQ(monitor.suddenTurnCount, 0);
    ASSERT_EQ(monitor.deadlockCount, 0);
}

TEST_CASE(Stress_Telemetry, ContinuousRandomTraffic5000Frames) {
    TelemetryMonitor monitor;
    Simulation sim;
    sim.setAutoMode(true);
    sim.setAutoInterval(0.3f); // Fast event generation every 0.3s

    for (int f = 0; f < 5000; ++f) {
        sim.update(0.016f);
        monitor.update(sim, f);

        if (monitor.collisionCount > 0 || monitor.suddenMoveCount > 0 || monitor.deadlockCount > 0) {
            break;
        }
    }

    ASSERT_EQ(monitor.collisionCount, 0);
    ASSERT_EQ(monitor.suddenMoveCount, 0);
    ASSERT_EQ(monitor.suddenTurnCount, 0);
    ASSERT_EQ(monitor.deadlockCount, 0);
}
