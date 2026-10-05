#include "Figures.h"
#include <cmath>

// -------------------------------------------------------------
// RectangleFig
// -------------------------------------------------------------
RectangleFig::RectangleFig() {
    anchor_ = Vec(1.0f, 1.0f);
    ang_ = 0.25f * PI;
}

void RectangleFig::buildVertices(RenderBatch& batch) const {
    Vec p0 = position_;
    Vec p1 = position_ + anchor_ * 0.5f + (anchor_ * 0.5f << (ang_ * -2.0f));
    Vec p2 = position_ + anchor_;
    Vec p3 = position_ + anchor_ * 0.5f - (anchor_ * 0.5f << (ang_ * -2.0f));

    batch.addQuad(p0.getX(), p0.getY(),
                  p1.getX(), p1.getY(),
                  p2.getX(), p2.getY(),
                  p3.getX(), p3.getY(),
                  color_);
}

// -------------------------------------------------------------
// TriangleFig
// -------------------------------------------------------------
TriangleFig::TriangleFig() {
    anchor_ = Vec(1.0f, 1.0f);
}

void TriangleFig::buildVertices(RenderBatch& batch) const {
    Vec len = anchor_;
    Vec p0 = position_;
    Vec p1 = position_ + len;
    Vec p2 = position_ + (len << (PI / 3.0f));

    batch.addTriangle(p0.getX(), p0.getY(),
                      p1.getX(), p1.getY(),
                      p2.getX(), p2.getY(),
                      color_);
}

// -------------------------------------------------------------
// TrapezoidFig
// -------------------------------------------------------------
TrapezoidFig::TrapezoidFig() {
    anchor_ = Vec(1.0f, 1.0f);
}

void TrapezoidFig::buildVertices(RenderBatch& batch) const {
    Vec len = anchor_;
    Vec p0 = position_;
    Vec p1 = position_ + len;

    len = len << (PI / 6.0f);
    len = len * 0.8660254038f;
    Vec p2 = position_ + len;

    len = len << (PI / 6.0f);
    len = len * 0.5773502692f;
    Vec p3 = position_ + len;

    batch.addQuad(p0.getX(), p0.getY(),
                  p1.getX(), p1.getY(),
                  p2.getX(), p2.getY(),
                  p3.getX(), p3.getY(),
                  color_);
}

// -------------------------------------------------------------
// CircleFig
// -------------------------------------------------------------
CircleFig::CircleFig() {
    anchor_ = Vec(1.0f, 0.0f);
}

void CircleFig::buildVertices(RenderBatch& batch) const {
    float radius = anchor_.length();
    batch.addCircle(position_.getX(), position_.getY(), radius, color_, 40);
}

// -------------------------------------------------------------
// HalfCircleFig
// -------------------------------------------------------------
HalfCircleFig::HalfCircleFig() {
    anchor_ = Vec(1.0f, 0.0f);
}

void HalfCircleFig::buildVertices(RenderBatch& batch) const {
    float radius = anchor_.length();
    float startAngle = std::atan2(anchor_.getY(), anchor_.getX());
    batch.addHalfCircle(position_.getX(), position_.getY(), radius, startAngle, color_, 25);
}

// -------------------------------------------------------------
// CarFig
// -------------------------------------------------------------
CarFig::CarFig() {
    position_ = Vec(0.0f, 0.0f);
    anchor_ = Vec(0.4f, 0.0f);
    updateComponents();
}

CarFig::CarFig(const Vec& pos, const Vec& anc) {
    position_ = pos;
    anchor_ = anc;
    updateComponents();
}

void CarFig::updateComponents() {
    // 1. Trapezoid roof
    Vec p1 = anchor_ << PI;
    p1 = p1 * 20.0f;
    roof_.setPosition(position_ + p1);
    roof_.setAnchor(anchor_ * 40.0f);
    roof_.setColor(0.1f, 0.0f, 0.9f);

    // 2. Rectangle body
    p1 = anchor_ << 3.816333596f;
    p1 = p1 * 32.01562119f;
    body_.setPosition(position_ + p1);
    body_.setAngle(0.3805063771f);
    p1 = anchor_ << 0.3805063771f;
    p1 = p1 * 53.85164807f;
    body_.setAnchor(p1);
    body_.setColor(0.5f, 0.5f, 0.5f);

    // 3. Wheel Front
    p1 = anchor_ << (PI * 1.25f);
    p1 = p1 * 28.28427125f;
    wheelFront_.setPosition(position_ + p1);
    wheelFront_.setAnchor(anchor_ * 4.0f);
    wheelFront_.setColor(0.1f, 0.1f, 0.1f);

    // 4. Wheel Rear
    p1 = anchor_ << (PI * 1.75f);
    p1 = p1 * 28.28427125f;
    wheelRear_.setPosition(position_ + p1);
    wheelRear_.setAnchor(anchor_ * 4.0f);
    wheelRear_.setColor(0.1f, 0.1f, 0.1f);
}

void CarFig::rotate(float angle) {
    anchor_ = anchor_ << angle;
    updateComponents();
}

void CarFig::move(const Vec& delta) {
    position_ = position_ + delta;
    updateComponents();
}

void CarFig::buildVertices(RenderBatch& batch) const {
    roof_.buildVertices(batch);
    body_.buildVertices(batch);
    wheelFront_.buildVertices(batch);
    wheelRear_.buildVertices(batch);
}

// -------------------------------------------------------------
// ParkingLotFig
// -------------------------------------------------------------
static void drawPavementStroke(RenderBatch& batch, float x1, float y1, float x2, float y2, float thickness, const Color& color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) return;
    float nx = -dy / len * (thickness * 0.5f);
    float ny =  dx / len * (thickness * 0.5f);
    batch.addQuad(x1 + nx, y1 + ny,
                  x2 + nx, y2 + ny,
                  x2 - nx, y2 - ny,
                  x1 - nx, y1 - ny,
                  color);
}

static void drawPavementChar(RenderBatch& batch, char ch, float cx, float cy, float w, float h, float t, const Color& c) {
    float xL = cx - w * 0.5f;
    float xM = cx;
    float xR = cx + w * 0.5f;
    float yB = cy - h * 0.5f;
    float yM = cy;
    float yT = cy + h * 0.5f;

    auto line = [&](float x1, float y1, float x2, float y2) {
        drawPavementStroke(batch, x1, y1, x2, y2, t, c);
    };

    switch (ch) {
        case 'O':
            line(xL, yB, xL, yT);
            line(xL, yT, xR, yT);
            line(xR, yT, xR, yB);
            line(xR, yB, xL, yB);
            break;
        case 'N':
            line(xL, yB, xL, yT);
            line(xL, yT, xR, yB);
            line(xR, yB, xR, yT);
            break;
        case 'E':
            line(xL, yB, xL, yT);
            line(xL, yT, xR, yT);
            line(xL, yM, xR * 0.85f + xL * 0.15f, yM);
            line(xL, yB, xR, yB);
            break;
        case 'W':
            line(xL, yT, xL + w * 0.25f, yB);
            line(xL + w * 0.25f, yB, xM, yT * 0.6f + yB * 0.4f);
            line(xM, yT * 0.6f + yB * 0.4f, xR - w * 0.25f, yB);
            line(xR - w * 0.25f, yB, xR, yT);
            break;
        case 'A':
            line(xL, yB, xM, yT);
            line(xM, yT, xR, yB);
            line(xL + w * 0.2f, yM, xR - w * 0.2f, yM);
            break;
        case 'Y':
            line(xL, yT, xM, yM);
            line(xR, yT, xM, yM);
            line(xM, yM, xM, yB);
            break;
        case 'I':
            line(xM, yB, xM, yT);
            line(xL + w * 0.15f, yT, xR - w * 0.15f, yT);
            line(xL + w * 0.15f, yB, xR - w * 0.15f, yB);
            break;
        case 'U':
            line(xL, yT, xL, yB);
            line(xL, yB, xR, yB);
            line(xR, yB, xR, yT);
            break;
        case 'T':
            line(xL, yT, xR, yT);
            line(xM, yT, xM, yB);
            break;
        case '>':
            line(xL, yT, xR, yM);
            line(xR, yM, xL, yB);
            break;
        case '<':
            line(xR, yT, xL, yM);
            line(xL, yM, xR, yB);
            break;
        case '-':
            line(xL, yM, xR, yM);
            break;
        case 'S':
            line(xR, yT, xL, yT);
            line(xL, yT, xL, yM);
            line(xL, yM, xR, yM);
            line(xR, yM, xR, yB);
            line(xR, yB, xL, yB);
            break;
        case '0':
            line(xL, yB, xL, yT);
            line(xL, yT, xR, yT);
            line(xR, yT, xR, yB);
            line(xR, yB, xL, yB);
            break;
        case '1':
            line(xM, yB, xM, yT);
            line(xL, yT - h * 0.25f, xM, yT);
            line(xL, yB, xR, yB);
            break;
        case '2':
            line(xL, yT, xR, yT);
            line(xR, yT, xR, yM);
            line(xR, yM, xL, yM);
            line(xL, yM, xL, yB);
            line(xL, yB, xR, yB);
            break;
        case '3':
            line(xL, yT, xR, yT);
            line(xR, yT, xR, yB);
            line(xL, yM, xR, yM);
            line(xL, yB, xR, yB);
            break;
        case '4':
            line(xL, yT, xL, yM);
            line(xL, yM, xR, yM);
            line(xR, yT, xR, yB);
            break;
        case '5':
            line(xR, yT, xL, yT);
            line(xL, yT, xL, yM);
            line(xL, yM, xR, yM);
            line(xR, yM, xR, yB);
            line(xR, yB, xL, yB);
            break;
        case '6':
            line(xR, yT, xL, yT);
            line(xL, yT, xL, yB);
            line(xL, yB, xR, yB);
            line(xR, yB, xR, yM);
            line(xR, yM, xL, yM);
            break;
        case '7':
            line(xL, yT, xR, yT);
            line(xR, yT, xM - w * 0.1f, yB);
            break;
        case '8':
            line(xL, yT, xR, yT);
            line(xL, yM, xR, yM);
            line(xL, yB, xR, yB);
            line(xL, yB, xL, yT);
            line(xR, yB, xR, yT);
            break;
        case '9':
            line(xL, yM, xL, yT);
            line(xL, yT, xR, yT);
            line(xR, yT, xR, yB);
            line(xL, yM, xR, yM);
            line(xL, yB, xR, yB);
            break;
        default:
            break;
    }
}

static void drawPavementString(RenderBatch& batch, const std::string& text, float cx, float cy,
                               float charW, float charH, float strokeW, float spacing, const Color& color) {
    if (text.empty()) return;
    float totalW = text.size() * charW + (text.size() - 1) * spacing;
    float curX = cx - totalW * 0.5f + charW * 0.5f;
    for (char ch : text) {
        if (ch != ' ') {
            drawPavementChar(batch, ch, curX, cy, charW, charH, strokeW, color);
        }
        curX += charW + spacing;
    }
}

static void drawRoadArrow(RenderBatch& batch, float cx, float cy, float dirX, float dirY,
                          float length, float headW, float headL, float strokeW, const Color& color) {
    float len = std::sqrt(dirX * dirX + dirY * dirY);
    if (len < 0.0001f) return;
    float dx = dirX / len;
    float dy = dirY / len;
    float nx = -dy;
    float ny =  dx;

    float halfLen = length * 0.5f;
    float shaftStart_x = cx - dx * halfLen;
    float shaftStart_y = cy - dy * halfLen;
    float shaftEnd_x   = cx + dx * (halfLen - headL * 0.5f);
    float shaftEnd_y   = cy + dy * (halfLen - headL * 0.5f);

    drawPavementStroke(batch, shaftStart_x, shaftStart_y, shaftEnd_x, shaftEnd_y, strokeW, color);

    float tipX = cx + dx * (halfLen + headL * 0.5f);
    float tipY = cy + dy * (halfLen + headL * 0.5f);
    float leftX = shaftEnd_x + nx * (headW * 0.5f);
    float leftY = shaftEnd_y + ny * (headW * 0.5f);
    float rightX = shaftEnd_x - nx * (headW * 0.5f);
    float rightY = shaftEnd_y - ny * (headW * 0.5f);

    batch.addTriangle(tipX, tipY, leftX, leftY, rightX, rightY, color);
}

static void drawStallPavementMarking(RenderBatch& batch, int spotIndex, float spotX, float spotY) {
    int displayNum = spotIndex + 1; // 1 to 10
    float pw = (displayNum == 10) ? 9.5f : 6.5f;
    float ph = 5.6f;
    Color plateColor(0.10f, 0.11f, 0.14f, 0.95f);
    Color borderColor(0.35f, 0.38f, 0.45f, 0.8f);
    Color textColor(1.0f, 0.88f, 0.18f, 1.0f); // Bright highway yellow road paint

    // Backing plate
    batch.addRect(spotX - pw * 0.5f, spotY - ph * 0.5f, spotX + pw * 0.5f, spotY + ph * 0.5f, plateColor);
    // Boundary line
    batch.addLine(spotX - pw * 0.5f, spotY - ph * 0.5f, spotX + pw * 0.5f, spotY - ph * 0.5f, borderColor);
    batch.addLine(spotX + pw * 0.5f, spotY - ph * 0.5f, spotX + pw * 0.5f, spotY + ph * 0.5f, borderColor);
    batch.addLine(spotX + pw * 0.5f, spotY + ph * 0.5f, spotX - pw * 0.5f, spotY + ph * 0.5f, borderColor);
    batch.addLine(spotX - pw * 0.5f, spotY + ph * 0.5f, spotX - pw * 0.5f, spotY - ph * 0.5f, borderColor);

    if (displayNum == 10) {
        float charW = 2.2f;
        float charH = 3.8f;
        float strokeW = 0.55f;
        drawPavementChar(batch, '1', spotX - 1.6f, spotY, charW, charH, strokeW, textColor);
        drawPavementChar(batch, '0', spotX + 1.6f, spotY, charW, charH, strokeW, textColor);
    } else {
        float charW = 2.8f;
        float charH = 4.0f;
        float strokeW = 0.65f;
        drawPavementChar(batch, static_cast<char>('0' + displayNum), spotX, spotY, charW, charH, strokeW, textColor);
    }
}

void ParkingLotFig::buildVertices(RenderBatch& batch) {
    // --- 1. Colors Palette ---
    Color baseGroundColor(0.11f, 0.12f, 0.14f);      // Dark parking lot base
    Color roadSurfaceColor(0.18f, 0.19f, 0.23f);     // Smooth driving asphalt
    Color stallPadColor(0.14f, 0.15f, 0.18f);        // Parking stall surface
    Color wallColor(0.35f, 0.38f, 0.44f);            // Outer enclosure walls
    Color dividerColor(0.85f, 0.88f, 0.92f, 0.85f);  // Clean white stall striping
    Color yellowMarkingColor(1.0f, 0.88f, 0.18f);    // Highway yellow paint
    Color islandGrassColor(0.15f, 0.30f, 0.18f);     // Landscaped median green
    Color islandAccentColor(0.20f, 0.38f, 0.24f);    // Inner flowerbed
    Color islandCurbColor(0.44f, 0.46f, 0.50f);      // Concrete median curb
    Color inSignColor(0.20f, 0.85f, 0.35f);          // Gate IN green
    Color outSignColor(0.95f, 0.30f, 0.25f);         // Gate OUT red

    // --- 2. Base Ground ---
    batch.addRect(-120.0f, -80.0f, 120.0f, 90.0f, baseGroundColor);

    // --- 3. One-Way Circle Roadbed Pavement ---
    // (A) Entrance Roadway: South to North
    batch.addRect(-103.0f, -80.0f, -89.0f, -30.0f, roadSurfaceColor);

    // (B) Entrance Fillet Turn into Bottom Aisle
    batch.addQuad(-103.0f, -30.0f, -86.0f, -36.0f, -70.0f, 0.0f, -89.0f, -30.0f, roadSurfaceColor);

    // (C) Bottom Driving Aisle (Eastbound)
    batch.addRect(-86.0f, -36.0f, 84.0f, 0.0f, roadSurfaceColor);

    // (D) East Turnaround Loop (Curving from Bottom Aisle East around to Top Aisle West)
    float loopCx = 84.0f;
    float loopCy = 15.0f;
    float rxIn = 9.5f;
    float rxOut = 35.5f;
    float rxMid = 25.5f;
    float ryIn = 17.0f;
    float ryOut = 51.0f;
    float ryMid = 33.0f;
    const int loopSegments = 24;
    for (int i = 0; i < loopSegments; ++i) {
        float a1 = -0.5f * PI + (PI / loopSegments) * i;
        float a2 = -0.5f * PI + (PI / loopSegments) * (i + 1);

        float inX1 = loopCx + rxIn * std::cos(a1);
        float inY1 = loopCy + ryIn * std::sin(a1);
        float outX1 = loopCx + rxOut * std::cos(a1);
        float outY1 = loopCy + ryOut * std::sin(a1);

        float inX2 = loopCx + rxIn * std::cos(a2);
        float inY2 = loopCy + ryIn * std::sin(a2);
        float outX2 = loopCx + rxOut * std::cos(a2);
        float outY2 = loopCy + ryOut * std::sin(a2);

        batch.addQuad(inX1, inY1, outX1, outY1, outX2, outY2, inX2, inY2, roadSurfaceColor);

        // Dashed yellow centerline along the turnaround loop
        if (i % 2 == 0) {
            float midX1 = loopCx + rxMid * std::cos(a1);
            float midY1 = loopCy + ryMid * std::sin(a1);
            float midX2 = loopCx + rxMid * std::cos(a2);
            float midY2 = loopCy + ryMid * std::sin(a2);
            drawPavementStroke(batch, midX1, midY1, midX2, midY2, 0.7f, yellowMarkingColor);
        }
    }

    // (E) Top Driving Aisle (Westbound)
    batch.addRect(-98.0f, 30.0f, 84.0f, 66.0f, roadSurfaceColor);

    // (F) Northwest Fillet Turn into West Corridor
    batch.addQuad(-98.0f, 30.0f, -98.0f, 66.0f, -116.0f, 36.0f, -100.0f, 30.0f, roadSurfaceColor);

    // (G) West Exit Corridor (Southbound)
    batch.addRect(-116.0f, -80.0f, -100.0f, 36.0f, roadSurfaceColor);

    // --- 4. Parking Stalls Pavements & Wheel Stops ---
    batch.addRect(-86.0f, -60.0f, 84.0f, -36.0f, stallPadColor);
    batch.addRect(-86.0f, 66.0f, 84.0f, 90.0f, stallPadColor);

    // White Stall Bay Dividers
    for (float x : {-86.0f, -52.0f, -18.0f, 16.0f, 50.0f, 84.0f}) {
        // Bottom stall divider
        drawPavementStroke(batch, x, -60.0f, x, -36.0f, 0.6f, dividerColor);
        // Top stall divider
        drawPavementStroke(batch, x, 66.0f, x, 90.0f, 0.6f, dividerColor);
    }

    // Wheel stops
    for (int s = 0; s <= 4; ++s) {
        float sx = -68.0f + 34.0f * s;
        drawPavementStroke(batch, sx - 8.0f, -56.0f, sx + 8.0f, -56.0f, 0.8f, Color(0.7f, 0.7f, 0.75f, 0.9f));
    }
    for (int s = 5; s <= 9; ++s) {
        float sx = 238.0f - 34.0f * s;
        drawPavementStroke(batch, sx - 8.0f, 86.0f, sx + 8.0f, 86.0f, 0.8f, Color(0.7f, 0.7f, 0.75f, 0.9f));
    }

    // --- 5. Center Landscaped Median Island ---
    // Central rectangle
    batch.addRect(-75.0f, 4.0f, 75.0f, 26.0f, islandGrassColor);
    batch.addRect(-65.0f, 8.0f, 65.0f, 22.0f, islandAccentColor);

    // End caps (East and West)
    batch.addHalfCircle(75.0f, 15.0f, 11.0f, -0.5f * PI, islandGrassColor, 20);
    batch.addHalfCircle(-75.0f, 15.0f, 11.0f, 0.5f * PI, islandGrassColor, 20);

    // Island curb outlines
    drawPavementStroke(batch, -75.0f, 4.0f, 75.0f, 4.0f, 0.9f, islandCurbColor);
    drawPavementStroke(batch, -75.0f, 26.0f, 75.0f, 26.0f, 0.9f, islandCurbColor);
    for (int i = 0; i < 20; ++i) {
        float a1 = -0.5f * PI + (PI / 20) * i;
        float a2 = -0.5f * PI + (PI / 20) * (i + 1);
        batch.addLine(75.0f + 11.0f * std::cos(a1), 15.0f + 11.0f * std::sin(a1),
                      75.0f + 11.0f * std::cos(a2), 15.0f + 11.0f * std::sin(a2), islandCurbColor);
        float wa1 = 0.5f * PI + (PI / 20) * i;
        float wa2 = 0.5f * PI + (PI / 20) * (i + 1);
        batch.addLine(-75.0f + 11.0f * std::cos(wa1), 15.0f + 11.0f * std::sin(wa1),
                      -75.0f + 11.0f * std::cos(wa2), 15.0f + 11.0f * std::sin(wa2), islandCurbColor);
    }

    // --- 6. Entrance / Exit Channel Divider Island ---
    batch.addRect(-103.0f, -80.0f, -99.0f, -32.0f, islandCurbColor);
    batch.addRect(-102.0f, -79.0f, -100.0f, -33.0f, islandGrassColor);

    // --- 7. Yellow Dashed Centerlines along the One-Way Circle ---
    // Bottom aisle centerline dashes (Eastbound, y = -18)
    for (float x = -80.0f; x < 84.0f; x += 10.0f) {
        drawPavementStroke(batch, x, -18.0f, std::min(x + 5.5f, 84.0f), -18.0f, 0.7f, yellowMarkingColor);
    }
    // Top aisle centerline dashes (Westbound, y = 48)
    for (float x = 84.0f; x > -95.0f; x -= 10.0f) {
        drawPavementStroke(batch, x, 48.0f, std::max(x - 5.5f, -95.0f), 48.0f, 0.7f, yellowMarkingColor);
    }
    // West corridor centerline dashes (Southbound, x = -108)
    for (float y = 30.0f; y > -75.0f; y -= 10.0f) {
        drawPavementStroke(batch, -108.0f, y, -108.0f, std::max(y - 5.5f, -75.0f), 0.7f, yellowMarkingColor);
    }

    // --- 8. One-Way Highway Directional Arrows ---
    // (A) Entrance: pointing North & curve East
    drawRoadArrow(batch, -96.0f, -55.0f, 0.0f, 1.0f, 10.0f, 4.8f, 4.0f, 1.4f, yellowMarkingColor);
    drawRoadArrow(batch, -90.0f, -24.0f, 0.707f, 0.707f, 9.0f, 4.8f, 4.0f, 1.4f, yellowMarkingColor);

    // (B) Bottom Aisle: pointing East
    drawRoadArrow(batch, -45.0f, -18.0f, 1.0f, 0.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, 15.0f, -18.0f, 1.0f, 0.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, 60.0f, -18.0f, 1.0f, 0.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);

    // (C) East Turnaround Loop: pointing around counter-clockwise arc
    drawRoadArrow(batch, 101.0f, -6.0f, 0.64f, 0.77f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, 109.5f, 15.0f, 0.0f, 1.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, 101.0f, 36.0f, -0.64f, 0.77f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);

    // (D) Top Aisle: pointing West
    drawRoadArrow(batch, 55.0f, 48.0f, -1.0f, 0.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, 0.0f, 48.0f, -1.0f, 0.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, -55.0f, 48.0f, -1.0f, 0.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);

    // (E) Northwest Curve: pointing South-West
    drawRoadArrow(batch, -96.0f, 42.0f, -0.707f, -0.707f, 9.0f, 4.8f, 4.0f, 1.4f, yellowMarkingColor);

    // (F) West Exit Corridor: pointing South
    drawRoadArrow(batch, -108.0f, 18.0f, 0.0f, -1.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, -108.0f, -18.0f, 0.0f, -1.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);
    drawRoadArrow(batch, -108.0f, -55.0f, 0.0f, -1.0f, 11.0f, 5.2f, 4.5f, 1.5f, yellowMarkingColor);

    // --- 9. Highway Pavement Lettering ---
    // Bottom Aisle: "ONE WAY >>"
    drawPavementString(batch, "ONE WAY >>", -15.0f, -27.0f, 2.8f, 4.6f, 0.7f, 1.0f, yellowMarkingColor);

    // Top Aisle: "<< ONE WAY"
    drawPavementString(batch, "<< ONE WAY", -15.0f, 57.0f, 2.8f, 4.6f, 0.7f, 1.0f, yellowMarkingColor);

    // Entrance Gate "IN"
    drawPavementString(batch, "IN", -96.0f, -73.0f, 2.6f, 4.0f, 0.7f, 1.0f, inSignColor);
    drawPavementStroke(batch, -101.0f, -77.0f, -91.0f, -77.0f, 0.8f, inSignColor);

    // Exit Gate "OUT"
    drawPavementString(batch, "OUT", -108.0f, -73.0f, 2.4f, 4.0f, 0.7f, 0.8f, outSignColor);
    drawPavementStroke(batch, -114.0f, -77.0f, -102.0f, -77.0f, 0.8f, outSignColor);

    // --- 10. Outer Boundary Walls ---
    // Left outer wall
    drawPavementStroke(batch, -120.0f, -80.0f, -120.0f, 90.0f, 1.2f, wallColor);
    // Top outer wall
    drawPavementStroke(batch, -120.0f, 90.0f, 120.0f, 90.0f, 1.2f, wallColor);
    // Right outer wall
    drawPavementStroke(batch, 120.0f, 90.0f, 120.0f, -60.0f, 1.2f, wallColor);
    // Bottom outer wall
    drawPavementStroke(batch, 120.0f, -60.0f, -86.0f, -60.0f, 1.2f, wallColor);

    // --- 11. Painted Stall Numbers (1 to 10) ---
    // Bottom row (spots 0..4 -> 1..5)
    for (int s = 0; s <= 4; ++s) {
        float spotX = -68.0f + 34.0f * s;
        drawStallPavementMarking(batch, s, spotX, -38.0f);
    }
    // Top row (spots 5..9 -> 6..10)
    for (int s = 5; s <= 9; ++s) {
        float spotX = 238.0f - 34.0f * s;
        drawStallPavementMarking(batch, s, spotX, 68.0f);
    }
}
