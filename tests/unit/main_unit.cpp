#include "TestHarness.h"

int main() {
    return TestFramework::TestRegistry::instance().runAll("Unit_");
}
