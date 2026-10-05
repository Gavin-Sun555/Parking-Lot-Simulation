#include "TestHarness.h"
#include "core/Vehicle.h"
#include "core/Ticket.h"

TEST_CASE(Unit_Vehicle, TypeNamesAndDailyRates) {
    ASSERT_EQ(vehicleTypeName(VehicleType::VAN), "van");
    ASSERT_EQ(vehicleTypeName(VehicleType::CAR), "car");
    ASSERT_EQ(vehicleTypeName(VehicleType::MOTORBIKE), "motorbike");
    ASSERT_EQ(vehicleTypeName(VehicleType::BICYCLE), "bicycle");
    ASSERT_EQ(vehicleTypeName(static_cast<VehicleType>(99)), "unknown");

    ASSERT_NEAR(vehicleDailyRate(VehicleType::VAN), 20.0, 1e-4);
    ASSERT_NEAR(vehicleDailyRate(VehicleType::CAR), 15.0, 1e-4);
    ASSERT_NEAR(vehicleDailyRate(VehicleType::MOTORBIKE), 10.0, 1e-4);
    ASSERT_NEAR(vehicleDailyRate(VehicleType::BICYCLE), 5.0, 1e-4);
}

TEST_CASE(Unit_Vehicle, DateDifferenceCalculations) {
    Date d1{2026, 10, 4};
    Date d2{2026, 10, 4};
    ASSERT_EQ(Date::diffDays(d1, d2), 0);

    Date d3{2026, 10, 9};
    ASSERT_EQ(Date::diffDays(d1, d3), 5);

    // Across months
    Date d4{2026, 11, 4};
    ASSERT_EQ(Date::diffDays(d1, d4), 30);

    // Across years
    Date d5{2027, 10, 4};
    ASSERT_EQ(Date::diffDays(d1, d5), 365);

    // Departure before arrival: should clamp to 0
    ASSERT_EQ(Date::diffDays(d3, d1), 0);
}

TEST_CASE(Unit_Ticket, ArrivalTicketGeneration) {
    ArrivalTicket ticket;
    ticket.vehicleId = 1001;
    ticket.vehicleType = VehicleType::CAR;
    ticket.arrivalDate = {2026, 10, 5};
    ticket.floor = 1;
    ticket.spaceNumber = 6; // Spot 7 (0-indexed 6)

    std::string str = ticket.toString();
    ASSERT_TRUE(str.find("ARRIVAL TICKET") != std::string::npos);
    ASSERT_TRUE(str.find("1001") != std::string::npos);
    ASSERT_TRUE(str.find("car") != std::string::npos);
    ASSERT_TRUE(str.find("Space 7") != std::string::npos);
    ASSERT_TRUE(str.find("$15.00") != std::string::npos);
}

TEST_CASE(Unit_Ticket, DepartureTicketPricingAndFormatting) {
    DepartureTicket ticket;
    ticket.vehicleId = 2002;
    ticket.vehicleType = VehicleType::VAN;
    ticket.arrivalDate = {2026, 10, 1};
    ticket.departureDate = {2026, 10, 4};
    ticket.totalDays = 3;
    ticket.price = 3 * vehicleDailyRate(VehicleType::VAN); // 3 * $20 = $60
    ticket.floor = 1;
    ticket.spaceNumber = 9; // Spot 10

    ASSERT_NEAR(ticket.price, 60.0, 1e-4);

    std::string str = ticket.toString();
    ASSERT_TRUE(str.find("DEPARTURE TICKET") != std::string::npos);
    ASSERT_TRUE(str.find("2002") != std::string::npos);
    ASSERT_TRUE(str.find("van") != std::string::npos);
    ASSERT_TRUE(str.find("Space 10") != std::string::npos);
    ASSERT_TRUE(str.find("$60.00") != std::string::npos);
}
