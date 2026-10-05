#include "TestHarness.h"

int main(int argc, char** argv) {
    std::string filter = (argc > 1) ? argv[1] : "";
    return TestFramework::TestRegistry::instance().runAll(filter);
}
