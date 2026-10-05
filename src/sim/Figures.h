#ifndef SIM_FIGURES_H
#define SIM_FIGURES_H

#include "Vec.h"
#include "../render/RenderTypes.h"
#include <memory>
#include <vector>

class Figure {
public:
    Figure() : anchor_(0.0f, 0.0f), position_(0.0f, 0.0f), color_(Color::White()) {}
    virtual ~Figure() = default;

    Vec getAnchor() const { return anchor_; }
    void setAnchor(const Vec& a) { anchor_ = a; }

    Vec getPosition() const { return position_; }
    void setPosition(const Vec& p) { position_ = p; }

    Color getColor() const { return color_; }
    void setColor(const Color& c) { color_ = c; }
    void setColor(float r, float g, float b, float a = 1.0f) { color_ = Color(r, g, b, a); }

    virtual void buildVertices(RenderBatch& batch) const = 0;

protected:
    Vec anchor_;
    Vec position_;
    Color color_;
};

class RectangleFig : public Figure {
public:
    RectangleFig();
    void setAngle(float ang) { ang_ = ang; }
    float getAngle() const { return ang_; }
    void buildVertices(RenderBatch& batch) const override;

private:
    float ang_ = 0.25f * PI;
};

class TriangleFig : public Figure {
public:
    TriangleFig();
    void buildVertices(RenderBatch& batch) const override;
};

class TrapezoidFig : public Figure {
public:
    TrapezoidFig();
    void buildVertices(RenderBatch& batch) const override;
};

class CircleFig : public Figure {
public:
    CircleFig();
    void buildVertices(RenderBatch& batch) const override;
};

class HalfCircleFig : public Figure {
public:
    HalfCircleFig();
    void buildVertices(RenderBatch& batch) const override;
};

class CarFig : public Figure {
public:
    CarFig();
    CarFig(const Vec& pos, const Vec& anc);

    void rotate(float angle);
    void move(const Vec& delta);

    void buildVertices(RenderBatch& batch) const override;

private:
    TrapezoidFig roof_;
    RectangleFig body_;
    CircleFig wheelFront_;
    CircleFig wheelRear_;
    void updateComponents();
};

// Parking lot stalls and markings
class ParkingLotFig {
public:
    static void buildVertices(RenderBatch& batch);
};

#endif // SIM_FIGURES_H
