#include "TestHarness.h"
#include "sim/Simulation.h"

TEST_CASE(Integration_Simulation, FullArrivalAndDepartureLifecycle) {
    Simulation sim;
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 0);
    ASSERT_EQ(sim.getActiveAgents().size(), 0);
    ASSERT_EQ(sim.getArrivalQueueSize(), 0);

    // 1. Trigger Arrival
    sim.triggerArrival(VehicleType::CAR);
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 1);
    ASSERT_EQ(sim.getTicketLog().size(), 1);

    // 2. Advance until parked
    int frames = 0;
    while ((!sim.getActiveAgents().empty() || sim.getArrivalQueueSize() > 0) && frames < 3000) {
        sim.update(0.016f);
        frames++;
    }


    // Vehicle has now parked and is stationary in the lot
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 1);
    ASSERT_EQ(sim.getActiveAgents().size(), 0);
    const auto* veh = sim.getParkingLot().getVehicle(1, 4); // First car spot
    ASSERT_TRUE(veh != nullptr);
    int vehId = veh->id;

    // 3. Trigger Departure
    sim.triggerDeparture(vehId);
    ASSERT_TRUE(sim.isVehicleDeparting(vehId));
    ASSERT_EQ(sim.getActiveAgents().size(), 1);

    // 4. Advance until departure finishes
    frames = 0;
    while (sim.getParkingLot().getOccupancy() > 0 && frames < 3000) {
        sim.update(0.016f);
        frames++;
    }

    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 0);
    ASSERT_FALSE(sim.isVehicleDeparting(vehId));
    // A departure ticket must have been logged
    ASSERT_GE(sim.getTicketLog().size(), 2);
    ASSERT_TRUE(sim.getTicketLog().back().find("DEPARTURE TICKET") != std::string::npos);
}

TEST_CASE(Integration_Simulation, ResetLotClearsAllState) {
    Simulation sim;
    sim.triggerArrival(VehicleType::MOTORBIKE);
    sim.triggerArrival(VehicleType::BICYCLE);
    sim.triggerArrival(VehicleType::CAR);

    // Advance 50 frames
    for (int i = 0; i < 50; ++i) {
        sim.update(0.016f);
    }

    sim.resetLot();
    ASSERT_EQ(sim.getParkingLot().getOccupancy(), 0);
    ASSERT_EQ(sim.getActiveAgents().size(), 0);
    ASSERT_EQ(sim.getArrivalQueueSize(), 0);
}

TEST_CASE(Integration_Simulation, TicketLogCapacityLimit) {
    Simulation sim;
    // Log 60 messages
    for (int i = 0; i < 60; ++i) {
        sim.logMessage("Log entry #" + std::to_string(i) + "\n");
    }

    // Capacity is strictly capped at 50 FIFO
    ASSERT_EQ(sim.getTicketLog().size(), 50);
    // Oldest entries 0..9 should have been evicted; front should be entry #10
    ASSERT_TRUE(sim.getTicketLog().front().find("#10") != std::string::npos);
    ASSERT_TRUE(sim.getTicketLog().back().find("#59") != std::string::npos);
}
