#include "TestHarness.h"
#include "sim/CarParking.h"
#include <cmath>

TEST_CASE(Unit_CarAgent, InitialStateAndProperties) {
    CarAgent agent(101, VehicleType::CAR, 3, 1);
    ASSERT_EQ(agent.getVehicleId(), 101);
    ASSERT_EQ(agent.getType(), VehicleType::CAR);
    ASSERT_EQ(agent.getTargetSpot(), 3);
    ASSERT_EQ(agent.getFloor(), 1);
    ASSERT_EQ(static_cast<int>(agent.getState()), static_cast<int>(CarState::IDLE));
    ASSERT_FALSE(agent.isParked());
    ASSERT_FALSE(agent.isDeparting());
    ASSERT_FALSE(agent.isFinished());
    ASSERT_FALSE(agent.isMoving());
}

TEST_CASE(Unit_CarAgent, BottomRowParkingLifecycleAndSmoothness) {
    // Target Bottom row Spot 3 (index 2)
    CarAgent agent(201, VehicleType::BICYCLE, 2, 1);
    agent.startParking();
    ASSERT_TRUE(agent.isMoving());
    ASSERT_EQ(static_cast<int>(agent.getState()), static_cast<int>(CarState::ENTERING));

    std::vector<CarAgent> emptyOtherAgents;
    Vec lastPos = agent.getPosition();
    Vec lastAnc = agent.getAnchor();
    float maxStep = 0.0f;
    float maxAngleDeg = 0.0f;

    int frames = 0;
    while (!agent.isParked() && frames < 2000) {
        agent.update(emptyOtherAgents);
        frames++;

        Vec curPos = agent.getPosition();
        Vec curAnc = agent.getAnchor();

        float step = (curPos - lastPos).length();
        if (step > maxStep) maxStep = step;

        Vec fwd1 = curAnc.normalized();
        Vec fwd2 = lastAnc.normalized();
        float dot = std::max(-1.0f, std::min(1.0f, fwd1.dot(fwd2)));
        float angleDeg = std::acos(dot) * (180.0f / PI);
        if (step > 0.01f && angleDeg > maxAngleDeg) maxAngleDeg = angleDeg;

        lastPos = curPos;
        lastAnc = curAnc;
    }

    ASSERT_TRUE(agent.isParked());
    ASSERT_FALSE(agent.isMoving());
    // Position should be inside Spot 3 (Bottom row y = -50)
    ASSERT_NEAR(agent.getPosition().getY(), -50.0f, 1.0f);

    // Verify kinematic smoothness: no jumps or sudden snaps
    ASSERT_LT(maxStep, 1.2f);
    ASSERT_LT(maxAngleDeg, 30.0f);
}

TEST_CASE(Unit_CarAgent, TopRowParkingLifecycleAndSmoothness) {
    // Target Top row Spot 8 (index 7)
    CarAgent agent(301, VehicleType::CAR, 7, 1);
    agent.startParking();

    std::vector<CarAgent> emptyOtherAgents;
    Vec lastPos = agent.getPosition();
    Vec lastAnc = agent.getAnchor();
    float maxStep = 0.0f;
    float maxAngleDeg = 0.0f;

    int frames = 0;
    while (!agent.isParked() && frames < 2500) {
        agent.update(emptyOtherAgents);
        frames++;

        Vec curPos = agent.getPosition();
        Vec curAnc = agent.getAnchor();

        float step = (curPos - lastPos).length();
        if (step > maxStep) maxStep = step;

        Vec fwd1 = curAnc.normalized();
        Vec fwd2 = lastAnc.normalized();
        float dot = std::max(-1.0f, std::min(1.0f, fwd1.dot(fwd2)));
        float angleDeg = std::acos(dot) * (180.0f / PI);
        if (step > 0.01f && angleDeg > maxAngleDeg) maxAngleDeg = angleDeg;

        lastPos = curPos;
        lastAnc = curAnc;
    }

    ASSERT_TRUE(agent.isParked());
    // Position should be inside Spot 8 (Top row y = 80)
    ASSERT_NEAR(agent.getPosition().getY(), 80.0f, 1.0f);

    // Verify kinematic smoothness
    ASSERT_LT(maxStep, 1.2f);
    ASSERT_LT(maxAngleDeg, 30.0f);
}

TEST_CASE(Unit_CarAgent, DepartureLifecycleToFinished) {
    // Instantiate an agent directly into parked position at Spot 6
    CarAgent agent(401, VehicleType::CAR, 5, 1);
    agent.startDeparture();
    ASSERT_TRUE(agent.isDeparting());
    ASSERT_TRUE(agent.isMoving());

    std::vector<CarAgent> emptyOtherAgents;
    Vec lastPos = agent.getPosition();
    float maxStep = 0.0f;

    int frames = 0;
    while (!agent.isFinished() && frames < 2500) {
        agent.update(emptyOtherAgents);
        frames++;

        Vec curPos = agent.getPosition();
        float step = (curPos - lastPos).length();
        if (step > maxStep) maxStep = step;
        lastPos = curPos;
    }

    ASSERT_TRUE(agent.isFinished());
    ASSERT_EQ(static_cast<int>(agent.getState()), static_cast<int>(CarState::DEPARTED));
    ASSERT_LT(maxStep, 1.2f);
}
