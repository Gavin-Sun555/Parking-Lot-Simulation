#include "TestHarness.h"
#include "sim/Simulation.h"

TEST_CASE(Integration_TargetedParking, SpecificSpotTargeting) {
    Simulation sim;

    // In an empty lot, specifically target Spot 7 (index 6)
    sim.triggerArrival(VehicleType::CAR, 6);
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 1);
    ASSERT_TRUE(sim.getParkingLot().isSpotOccupied(1, 6));

    // Stalls 4 and 5 must remain completely vacant!
    ASSERT_FALSE(sim.getParkingLot().isSpotOccupied(1, 4));
    ASSERT_FALSE(sim.getParkingLot().isSpotOccupied(1, 5));

    // Specifically target Spot 10 (index 9)
    sim.triggerArrival(VehicleType::VAN, 9);
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 2);
    ASSERT_TRUE(sim.getParkingLot().isSpotOccupied(1, 9));
    ASSERT_FALSE(sim.getParkingLot().isSpotOccupied(1, 8));

    // Specifically target Spot 2 (index 1)
    sim.triggerArrival(VehicleType::MOTORBIKE, 1);
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 3);
    ASSERT_TRUE(sim.getParkingLot().isSpotOccupied(1, 1));
    ASSERT_FALSE(sim.getParkingLot().isSpotOccupied(1, 0));

    // Attempting to target an already occupied spot must fail
    sim.triggerArrival(VehicleType::CAR, 6);
    // Occupancy should remain 3
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 3);
}

TEST_CASE(Integration_TargetedParking, NoGhostStallAppearancesWhileDriving) {
    Simulation sim;
    sim.triggerArrival(VehicleType::MOTORBIKE, 0);
    sim.triggerArrival(VehicleType::BICYCLE, 2);

    // Initial check: Both are queued, after 1 update step 1 enters driving and 1 remains queued
    ASSERT_EQ(sim.getArrivalQueueSize(), 2);
    sim.update(0.016f);
    ASSERT_EQ(sim.getActiveAgents().size(), 1);
    ASSERT_EQ(sim.getArrivalQueueSize(), 1);


    RenderBatch batch;
    // Check throughout driving lifecycle
    for (int frame = 0; frame < 1000; ++frame) {
        sim.update(0.016f);

        for (int spot = 0; spot < 10; ++spot) {
            const auto* veh = sim.getParkingLot().getVehicle(1, spot);
            if (!veh) continue;

            if (sim.isVehicleDriving(veh->id)) {
                // Must not be parked
                for (const auto& a : sim.getActiveAgents()) {
                    if (a.getVehicleId() == veh->id) {
                        ASSERT_FALSE(a.isParked());
                    }
                }
            }
        }
    }
}
