#ifndef RENDER_TYPES_H
#define RENDER_TYPES_H

#include <vector>
#include <cmath>

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static Color White()  { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color Black()  { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color Gray()   { return {0.5f, 0.5f, 0.5f, 1.0f}; }
    static Color Red()    { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static Color Green()  { return {0.0f, 0.8f, 0.2f, 1.0f}; }
    static Color Blue()   { return {0.1f, 0.2f, 0.9f, 1.0f}; }
    static Color Yellow() { return {1.0f, 0.9f, 0.0f, 1.0f}; }
    static Color Purple() { return {0.4f, 0.2f, 0.7f, 1.0f}; }
};

struct Vertex {
    float pos[2];
    float color[4];
};

class RenderBatch {
public:
    std::vector<Vertex> triangles;
    std::vector<Vertex> lines;

    void clear() {
        triangles.clear();
        lines.clear();
    }

    void addTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& c) {
        triangles.push_back({{x1, y1}, {c.r, c.g, c.b, c.a}});
        triangles.push_back({{x2, y2}, {c.r, c.g, c.b, c.a}});
        triangles.push_back({{x3, y3}, {c.r, c.g, c.b, c.a}});
    }

    void addQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, const Color& c) {
        addTriangle(x1, y1, x2, y2, x3, y3, c);
        addTriangle(x1, y1, x3, y3, x4, y4, c);
    }

    void addRect(float xMin, float yMin, float xMax, float yMax, const Color& c) {
        addQuad(xMin, yMin, xMax, yMin, xMax, yMax, xMin, yMax, c);
    }

    void addLine(float x1, float y1, float x2, float y2, const Color& c) {
        lines.push_back({{x1, y1}, {c.r, c.g, c.b, c.a}});
        lines.push_back({{x2, y2}, {c.r, c.g, c.b, c.a}});
    }

    void addCircle(float cx, float cy, float radius, const Color& c, int segments = 40) {
        float angleStep = 2.0f * 3.14159265f / segments;
        for (int i = 0; i < segments; ++i) {
            float a1 = i * angleStep;
            float a2 = (i + 1) * angleStep;
            float x1 = cx + radius * std::cos(a1);
            float y1 = cy + radius * std::sin(a1);
            float x2 = cx + radius * std::cos(a2);
            float y2 = cy + radius * std::sin(a2);
            addTriangle(cx, cy, x1, y1, x2, y2, c);
        }
    }

    void addHalfCircle(float cx, float cy, float radius, float startAngle, const Color& c, int segments = 25) {
        float angleStep = 3.14159265f / segments;
        for (int i = 0; i < segments; ++i) {
            float a1 = startAngle + i * angleStep;
            float a2 = startAngle + (i + 1) * angleStep;
            float x1 = cx + radius * std::cos(a1);
            float y1 = cy + radius * std::sin(a1);
            float x2 = cx + radius * std::cos(a2);
            float y2 = cy + radius * std::sin(a2);
            addTriangle(cx, cy, x1, y1, x2, y2, c);
        }
    }
};

#endif // RENDER_TYPES_H
