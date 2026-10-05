#include "TestHarness.h"
#include "core/ParkingLot.h"

TEST_CASE(Unit_ParkingLot, InitialLotState) {
    ParkingLot lot;
    ASSERT_EQ(lot.getOccupancy(), 0);

    for (int s = 0; s < 10; ++s) {
        ASSERT_FALSE(lot.isSpotOccupied(1, s));
        const auto* spot = lot.getSpot(1, s);
        ASSERT_TRUE(spot != nullptr);
        ASSERT_TRUE(spot->vacant);
        ASSERT_EQ(spot->spaceNumber, s);
        ASSERT_EQ(spot->floor, 1);

        if (s < 2) {
            ASSERT_EQ(spot->preferredType, VehicleType::MOTORBIKE);
        } else if (s < 4) {
            ASSERT_EQ(spot->preferredType, VehicleType::BICYCLE);
        } else if (s < 8) {
            ASSERT_EQ(spot->preferredType, VehicleType::CAR);
        } else {
            ASSERT_EQ(spot->preferredType, VehicleType::VAN);
        }
    }
}

TEST_CASE(Unit_ParkingLot, PreferredSpotAllocation) {
    ParkingLot lot;
    Date date{2026, 10, 4};

    // 1. Motorbikes should take spots 0 and 1
    auto t1 = lot.arrival(VehicleType::MOTORBIKE, 101, date);
    ASSERT_TRUE(t1.has_value());
    ASSERT_EQ(t1.value().spaceNumber, 0);

    auto t2 = lot.arrival(VehicleType::MOTORBIKE, 102, date);
    ASSERT_TRUE(t2.has_value());
    ASSERT_EQ(t2.value().spaceNumber, 1);

    // 2. Bicycles should take spots 2 and 3
    auto t3 = lot.arrival(VehicleType::BICYCLE, 201, date);
    ASSERT_TRUE(t3.has_value());
    ASSERT_EQ(t3.value().spaceNumber, 2);

    auto t4 = lot.arrival(VehicleType::BICYCLE, 202, date);
    ASSERT_TRUE(t4.has_value());
    ASSERT_EQ(t4.value().spaceNumber, 3);

    // 3. Cars should take spots 4, 5, 6, 7
    auto t5 = lot.arrival(VehicleType::CAR, 301, date);
    ASSERT_TRUE(t5.has_value());
    ASSERT_EQ(t5.value().spaceNumber, 4);

    auto t6 = lot.arrival(VehicleType::CAR, 302, date);
    ASSERT_TRUE(t6.has_value());
    ASSERT_EQ(t6.value().spaceNumber, 5);

    // 4. Vans should take spots 8 and 9
    auto t7 = lot.arrival(VehicleType::VAN, 401, date);
    ASSERT_TRUE(t7.has_value());
    ASSERT_EQ(t7.value().spaceNumber, 8);

    auto t8 = lot.arrival(VehicleType::VAN, 402, date);
    ASSERT_TRUE(t8.has_value());
    ASSERT_EQ(t8.value().spaceNumber, 9);
}

TEST_CASE(Unit_ParkingLot, TargetedSpotAllocation) {
    ParkingLot lot;
    Date date{2026, 10, 4};

    // Target Spot 7 (index 6) directly in an empty lot
    auto ticket = lot.arrival(VehicleType::CAR, 501, date, 6);
    ASSERT_TRUE(ticket.has_value());
    ASSERT_EQ(ticket.value().spaceNumber, 6);

    // Verify Spot 7 is occupied, while spots 4 and 5 (car preferred spots) are still vacant!
    ASSERT_TRUE(lot.isSpotOccupied(1, 6));
    ASSERT_FALSE(lot.isSpotOccupied(1, 4));
    ASSERT_FALSE(lot.isSpotOccupied(1, 5));

    // Attempting to target Spot 7 again must fail (already occupied)
    auto conflict = lot.arrival(VehicleType::CAR, 502, date, 6);
    ASSERT_FALSE(conflict.has_value());
}

TEST_CASE(Unit_ParkingLot, DuplicateVehicleRejection) {
    ParkingLot lot;
    Date date{2026, 10, 4};

    auto t1 = lot.arrival(VehicleType::CAR, 601, date);
    ASSERT_TRUE(t1.has_value());

    // Same vehicle ID arriving again must be rejected
    auto t2 = lot.arrival(VehicleType::CAR, 601, date);
    ASSERT_FALSE(t2.has_value());
}

TEST_CASE(Unit_ParkingLot, FullLotCapacityAndOverflow) {
    ParkingLot lot;
    Date date{2026, 10, 4};

    // Fill all 10 spots
    for (int i = 0; i < 10; ++i) {
        auto t = lot.arrival(VehicleType::CAR, 700 + i, date);
        ASSERT_TRUE(t.has_value());
    }

    ASSERT_EQ(lot.getOccupancy(), 10);

    // 11th arrival must be rejected
    auto tOverflow = lot.arrival(VehicleType::CAR, 711, date);
    ASSERT_FALSE(tOverflow.has_value());
}

TEST_CASE(Unit_ParkingLot, DepartureAndFeeCalculation) {
    ParkingLot lot;
    Date arrDate{2026, 10, 4};
    Date depDate{2026, 10, 8}; // 4 days later

    lot.arrival(VehicleType::CAR, 801, arrDate, 5); // Spot 6
    ASSERT_EQ(lot.getOccupancy(), 1);
    ASSERT_TRUE(lot.isSpotOccupied(1, 5));

    auto depTicket = lot.departure(801, depDate);
    ASSERT_TRUE(depTicket.has_value());
    ASSERT_EQ(depTicket.value().vehicleId, 801);
    ASSERT_EQ(depTicket.value().totalDays, 4);
    // Car rate is $15/day -> 4 * 15 = $60
    ASSERT_NEAR(depTicket.value().price, 60.0, 1e-4);
    ASSERT_EQ(depTicket.value().spaceNumber, 5);

    // Spot 5 must now be released and vacant
    ASSERT_FALSE(lot.isSpotOccupied(1, 5));
    ASSERT_EQ(lot.getOccupancy(), 0);

    // Depart non-existent vehicle returns nullopt
    auto fakeDep = lot.departure(9999, depDate);
    ASSERT_FALSE(fakeDep.has_value());
}
