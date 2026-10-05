#include "TestHarness.h"
#include "sim/Vec.h"

TEST_CASE(Unit_Vec, DefaultConstructor) {
    Vec v;
    ASSERT_NEAR(v.getX(), -108.0f, 1e-4f);
    ASSERT_NEAR(v.getY(), -80.0f, 1e-4f);
}

TEST_CASE(Unit_Vec, ParameterizedConstructorAndAccessors) {
    Vec v(12.5f, -34.75f);
    ASSERT_NEAR(v.getX(), 12.5f, 1e-4f);
    ASSERT_NEAR(v.getY(), -34.75f, 1e-4f);

    v.setX(42.0f);
    v.setY(-100.0f);
    ASSERT_NEAR(v.getX(), 42.0f, 1e-4f);
    ASSERT_NEAR(v.getY(), -100.0f, 1e-4f);
}

TEST_CASE(Unit_Vec, BasicArithmeticOperators) {
    Vec a(10.0f, 20.0f);
    Vec b(3.0f, 5.0f);

    // Addition
    Vec sum = a + b;
    ASSERT_NEAR(sum.getX(), 13.0f, 1e-4f);
    ASSERT_NEAR(sum.getY(), 25.0f, 1e-4f);

    // Subtraction
    Vec diff = a - b;
    ASSERT_NEAR(diff.getX(), 7.0f, 1e-4f);
    ASSERT_NEAR(diff.getY(), 15.0f, 1e-4f);

    // Unary Negation
    Vec neg = -a;
    ASSERT_NEAR(neg.getX(), -10.0f, 1e-4f);
    ASSERT_NEAR(neg.getY(), -20.0f, 1e-4f);

    // Scalar Multiplication
    Vec scaled1 = a * 2.5f;
    ASSERT_NEAR(scaled1.getX(), 25.0f, 1e-4f);
    ASSERT_NEAR(scaled1.getY(), 50.0f, 1e-4f);

    Vec scaled2 = 2.5f * a;
    ASSERT_NEAR(scaled2.getX(), 25.0f, 1e-4f);
    ASSERT_NEAR(scaled2.getY(), 50.0f, 1e-4f);

    // Scalar Division
    Vec div = a / 2.0f;
    ASSERT_NEAR(div.getX(), 5.0f, 1e-4f);
    ASSERT_NEAR(div.getY(), 10.0f, 1e-4f);
}

TEST_CASE(Unit_Vec, DotProductAndLength) {
    Vec a(3.0f, 4.0f);
    ASSERT_NEAR(a.length(), 5.0f, 1e-4f);

    Vec b(4.0f, -3.0f);
    // Orthogonal vectors: dot product must be 0
    ASSERT_NEAR(a.dot(b), 0.0f, 1e-4f);

    Vec c(1.0f, 0.0f);
    Vec d(5.0f, 0.0f);
    // Collinear vectors: dot product is product of lengths
    ASSERT_NEAR(c.dot(d), 5.0f, 1e-4f);
}

TEST_CASE(Unit_Vec, Normalization) {
    Vec v(30.0f, 40.0f);
    Vec unit = v.normalized();
    ASSERT_NEAR(unit.length(), 1.0f, 1e-4f);
    ASSERT_NEAR(unit.getX(), 0.6f, 1e-4f);
    ASSERT_NEAR(unit.getY(), 0.8f, 1e-4f);

    // Zero vector normalization should not crash and return zero vector
    Vec zero(0.0f, 0.0f);
    Vec normZero = zero.normalized();
    ASSERT_NEAR(normZero.length(), 0.0f, 1e-4f);
    ASSERT_NEAR(normZero.getX(), 0.0f, 1e-4f);
    ASSERT_NEAR(normZero.getY(), 0.0f, 1e-4f);
}

TEST_CASE(Unit_Vec, Rotation) {
    Vec east(1.0f, 0.0f);

    // 90 degrees CCW (PI/2) should point North (0, 1)
    Vec north = east.rotate(0.5f * PI);
    ASSERT_NEAR(north.getX(), 0.0f, 1e-4f);
    ASSERT_NEAR(north.getY(), 1.0f, 1e-4f);

    // 180 degrees CCW (PI) should point West (-1, 0)
    Vec west = north << (0.5f * PI);
    ASSERT_NEAR(west.getX(), -1.0f, 1e-4f);
    ASSERT_NEAR(west.getY(), 0.0f, 1e-4f);

    // 360 degrees rotation returns to original vector
    Vec fullCircle = east.rotate(2.0f * PI);
    ASSERT_NEAR(fullCircle.getX(), 1.0f, 1e-4f);
    ASSERT_NEAR(fullCircle.getY(), 0.0f, 1e-4f);
}
