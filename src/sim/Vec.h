#ifndef SIM_VEC_H
#define SIM_VEC_H

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr float PI = static_cast<float>(M_PI);

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

class Vec {
private:
    float x_ = 0.0f;
    float y_ = 0.0f;

public:
    Vec() : x_(-108.0f), y_(-80.0f) {}
    Vec(float x, float y) : x_(x), y_(y) {}

    float getX() const { return x_; }
    float getY() const { return y_; }
    void setX(float x) { x_ = x; }
    void setY(float y) { y_ = y; }

    Vec operator+(const Vec& v) const {
        return Vec(x_ + v.x_, y_ + v.y_);
    }

    Vec operator-(const Vec& v) const {
        return Vec(x_ - v.x_, y_ - v.y_);
    }

    Vec operator-() const {
        return Vec(-x_, -y_);
    }

    Vec operator*(float k) const {
        return Vec(x_ * k, y_ * k);
    }

    Vec operator/(float k) const {
        return Vec(x_ / k, y_ / k);
    }

    float dot(const Vec& v) const {
        return x_ * v.x_ + y_ * v.y_;
    }

    float length() const {
        return std::sqrt(x_ * x_ + y_ * y_);
    }

    Vec normalized() const {
        float len = length();
        if (len > 1e-6f) return *this / len;
        return Vec(0.0f, 0.0f);
    }

    // 2D rotation counterclockwise by angle (radians)
    Vec operator<<(float ang) const {
        return Vec(x_ * std::cos(ang) - y_ * std::sin(ang),
                   y_ * std::cos(ang) + x_ * std::sin(ang));
    }

    Vec rotate(float ang) const {
        return *this << ang;
    }
};

inline Vec operator*(float k, const Vec& v) {
    return v * k;
}

#endif // SIM_VEC_H
