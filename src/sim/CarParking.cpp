#include "CarParking.h"
#include <cmath>
#include <iostream>

CarAgent::CarAgent() 
    : pos_(-96.0f, -80.0f), anc_(0.0f, 0.4f), carFig_(pos_, anc_) {}

CarAgent::CarAgent(int vehicleId, VehicleType type, int targetSpot, int floor)
    : vehicleId_(vehicleId), type_(type), targetSpot_(targetSpot), floor_(floor),
      pos_(-96.0f, -80.0f), anc_(0.0f, 0.4f), carFig_(pos_, anc_) {}

void CarAgent::startParking() {
    isDeparting_ = false;
    yieldFrames_ = 0;
    progress_ = 0.0f;
    pos_ = Vec(-96.0f, -80.0f);
    anc_ = Vec(0.0f, 0.4f);
    state_ = CarState::ENTERING;
    carFig_ = CarFig(pos_, anc_);
}

void CarAgent::startDeparture() {
    isDeparting_ = true;
    yieldFrames_ = 0;
    progress_ = 0.0f;
    if (targetSpot_ <= 4) {
        state_ = CarState::UNPARKING_BOTTOM_PULLOUT;
        pos_ = Vec(-68.0f + 34.0f * targetSpot_, -50.0f);
        anc_ = Vec(0.0f, 0.4f);
    } else {
        state_ = CarState::UNPARKING_TOP_PULLOUT;
        pos_ = Vec(238.0f - 34.0f * targetSpot_, 80.0f);
        anc_ = Vec(0.0f, -0.4f);
    }
    carFig_ = CarFig(pos_, anc_);
}



bool CarAgent::shouldYield(const std::vector<CarAgent>& allAgents) const {
    // 0. Active reverse parking maneuvers have absolute right of way to complete into their stall
    if (state_ == CarState::PARKING_BOTTOM || state_ == CarState::PARKING_TOP) {
        return false;
    }

    // A car unparking/getting out of a stall yields to ANY nearby moving vehicle!
    if (state_ == CarState::UNPARKING_BOTTOM_PULLOUT ||
        state_ == CarState::UNPARKING_BOTTOM_TURN ||
        state_ == CarState::UNPARKING_TOP_PULLOUT ||
        state_ == CarState::UNPARKING_TOP_TURN) {
        for (const auto& other : allAgents) {
            if (other.vehicleId_ == vehicleId_) continue;
            if (!other.isMoving()) continue;
            float dist = (other.pos_ - pos_).length();
            if (dist < 45.0f) {
                return true; // Unparking vehicle stops!
            }
        }
    }

    Vec fwd = anc_.normalized();
    Vec right(-fwd.getY(), fwd.getX());

    for (const auto& other : allAgents) {
        if (other.vehicleId_ == vehicleId_) continue;
        if (!other.isMoving()) continue;

        // 1. Moving cars NEVER yield to vehicles getting out of stalls!
        // The car getting out stops in its stall; moving cars never stop!
        if (other.state_ == CarState::UNPARKING_BOTTOM_PULLOUT ||
            other.state_ == CarState::UNPARKING_BOTTOM_TURN ||
            other.state_ == CarState::UNPARKING_TOP_PULLOUT ||
            other.state_ == CarState::UNPARKING_TOP_TURN) {
            continue;
        }

        // 2. Parallel independent lanes in western corridor:
        // Entrance path is at x ≈ -96 (Northbound).
        // Exit path is at x ≈ -108 (Southbound).
        bool thisInOutbound = (pos_.getX() <= -103.0f);
        bool otherInOutbound = (other.pos_.getX() <= -103.0f);
        if (thisInOutbound != otherInOutbound) {
            continue; // Independent corridor lanes
        }

        Vec toOther = other.pos_ - pos_;
        float dist = toOther.length();
        if (dist > 45.0f) continue;

        float forwardDist = toOther.dot(fwd);
        float lateralDist = std::abs(toOther.dot(right));

        // 3. Stop before a vehicle actively reverse parking into its stall ahead in the aisle
        if (other.state_ == CarState::PARKING_BOTTOM) {
            if (pos_.getY() < 0.0f) { // Bottom aisle
                float spotX = -68.0f + 34.0f * other.targetSpot_;
                if (anc_.getX() > 0.0f && pos_.getX() < spotX - 10.0f && dist < 45.0f) {
                    return true;
                }
            }
        }
        if (other.state_ == CarState::PARKING_TOP) {
            if (pos_.getY() > 0.0f) { // Top aisle
                float spotX = 238.0f - 34.0f * other.targetSpot_;
                if (anc_.getX() < 0.0f && pos_.getX() > spotX + 10.0f && dist < 45.0f) {
                    return true;
                }
            }
        }

        Vec otherFwd = other.anc_.normalized();
        float fwdDot = fwd.dot(otherFwd);

        // 4. Same lane forward following:
        // Keep safe following distance behind car in front in the one-way circle flow
        if (fwdDot > 0.7f && forwardDist > 0.0f && forwardDist < 24.0f && lateralDist < 6.5f) {
            return true;
        }

        // 5. Northwest corner merge: Top aisle car yields to car already in the exit corridor
        if (pos_.getX() > -102.0f && other.pos_.getX() <= -102.0f && pos_.getY() > 30.0f) {
            if (state_ == CarState::TURNING_CORRIDOR_TOP && other.pos_.getY() > 15.0f && other.pos_.getY() < 45.0f) {
                return true;
            }
        }
        if (pos_.getX() <= -102.0f && other.pos_.getX() > -102.0f && other.pos_.getY() > 30.0f) {
            continue; // Corridor traffic proceeds without stopping
        }

        // 6. Same corridor southbound following (x <= -102): downstream car (lower Y) has priority
        if (pos_.getX() <= -102.0f && other.pos_.getX() <= -102.0f) {
            if (other.pos_.getY() < pos_.getY() && (pos_.getY() - other.pos_.getY()) < 24.0f) {
                return true;
            }
        }
    }
    return false;
}

bool CarAgent::isAisleClear(const std::vector<CarAgent>& allAgents) const {
    for (const auto& other : allAgents) {
        if (other.vehicleId_ == vehicleId_) continue;
        if (!other.isMoving()) continue;

        if (targetSpot_ <= 4) { // Bottom aisle (Spots 1-5, indices 0-4, Eastbound)
            // 1. Any vehicle entering from entrance or turning into bottom aisle
            if (other.state_ == CarState::ENTERING) {
                return false;
            }
            if (other.state_ == CarState::TURNING_AISLE) {
                return false;
            }

            // 2. Any vehicle driving East on bottom aisle upstream of our stall (or within 35 units ahead)
            if (other.state_ == CarState::DRIVING_AISLE_BOTTOM ||
                other.state_ == CarState::DRIVING_AISLE_BOTTOM_EXIT) {
                if (other.pos_.getX() < pos_.getX() + 35.0f) {
                    return false;
                }
            }

            // 3. Any vehicle actively reverse parking into a bottom stall
            if (other.state_ == CarState::PARKING_BOTTOM) {
                if (other.pos_.getX() < pos_.getX() + 35.0f) {
                    return false;
                }
            }

            // 4. Another bottom stall is currently unparking
            if (other.state_ == CarState::UNPARKING_BOTTOM_PULLOUT ||
                other.state_ == CarState::UNPARKING_BOTTOM_TURN) {
                if (other.state_ == CarState::UNPARKING_BOTTOM_TURN ||
                    other.pos_.getY() > -48.0f ||
                    other.vehicleId_ < vehicleId_) {
                    return false;
                }
            }
        } else { // Top aisle (Spots 6-10, indices 5-9, Westbound)
            // 1. Any vehicle in turnaround loop approaching top aisle
            if (other.state_ == CarState::TURNING_LOOP) {
                return false;
            }

            // 2. Any vehicle driving West on top aisle upstream of our stall (or within 35 units ahead)
            if (other.state_ == CarState::DRIVING_AISLE_TOP ||
                other.state_ == CarState::DRIVING_AISLE_TOP_EXIT) {
                if (other.pos_.getX() > pos_.getX() - 35.0f) {
                    return false;
                }
            }

            // 3. Any vehicle actively reverse parking into a top stall
            if (other.state_ == CarState::PARKING_TOP) {
                if (other.pos_.getX() > pos_.getX() - 35.0f) {
                    return false;
                }
            }

            // 4. Another top stall is currently unparking
            if (other.state_ == CarState::UNPARKING_TOP_PULLOUT ||
                other.state_ == CarState::UNPARKING_TOP_TURN) {
                if (other.state_ == CarState::UNPARKING_TOP_TURN ||
                    other.pos_.getY() < 78.0f ||
                    other.vehicleId_ < vehicleId_) {
                    return false;
                }
            }
        }
    }
    return true;
}

static Vec evalBezier(const Vec& p0, const Vec& p1, const Vec& p2, const Vec& p3, float t) {
    float u = 1.0f - t;
    return (u * u * u) * p0 + (3.0f * u * u * t) * p1 + (3.0f * u * t * t) * p2 + (t * t * t) * p3;
}

static Vec evalBezierTangent(const Vec& p0, const Vec& p1, const Vec& p2, const Vec& p3, float t) {
    float u = 1.0f - t;
    return (-3.0f * u * u) * p0 + (3.0f * u * u - 6.0f * u * t) * p1 + (6.0f * u * t - 3.0f * t * t) * p2 + (3.0f * t * t) * p3;
}

void CarAgent::stepParking(const std::vector<CarAgent>& allAgents) {
    int lot = targetSpot_;
    float spotX = (lot <= 4) ? (-68.0f + 34.0f * lot) : (238.0f - 34.0f * lot);

    switch (state_) {
        case CarState::ENTERING: {
            pos_.setY(pos_.getY() + 0.5f);
            anc_ = Vec(0.0f, 0.4f);
            if (pos_.getY() >= -32.0f) {
                pos_.setY(-32.0f);
                state_ = CarState::TURNING_AISLE;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::TURNING_AISLE: {
            progress_ += 0.025f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            Vec p0(-96.0f, -32.0f);
            Vec p1(-96.0f, -24.0f);
            Vec p2(-89.0f, -18.0f);
            Vec p3(-82.0f, -18.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            anc_ = tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(0.4f, 0.0f);
                state_ = CarState::DRIVING_AISLE_BOTTOM;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::DRIVING_AISLE_BOTTOM: {
            pos_.setX(pos_.getX() + 0.5f);
            anc_ = Vec(0.4f, 0.0f);

            if (lot <= 4) {
                // Bottom row stall: initiate reverse parking once positioned past stall center
                if (pos_.getX() >= spotX + 15.0f) {
                    pos_.setX(spotX + 15.0f);
                    state_ = CarState::PARKING_BOTTOM;
                    progress_ = 0.0f;
                }
            } else {
                // Top row stall: drive to turnaround loop
                if (pos_.getX() >= 84.0f) {
                    pos_.setX(84.0f);
                    state_ = CarState::TURNING_LOOP;
                    progress_ = 0.0f;
                }
            }
            break;
        }

        case CarState::TURNING_LOOP: {
            progress_ += 0.0045f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            Vec p0(84.0f, -18.0f);
            Vec p1(118.0f, -18.0f);
            Vec p2(118.0f, 48.0f);
            Vec p3(84.0f, 48.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            anc_ = tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(-0.4f, 0.0f);
                state_ = CarState::DRIVING_AISLE_TOP;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::DRIVING_AISLE_TOP: {
            pos_.setX(pos_.getX() - 0.5f);
            anc_ = Vec(-0.4f, 0.0f);

            // Top row stall: initiate reverse parking once positioned past stall center
            if (pos_.getX() <= spotX - 15.0f) {
                pos_.setX(spotX - 15.0f);
                state_ = CarState::PARKING_TOP;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::PARKING_BOTTOM: {
            progress_ += 0.014f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            Vec p0(spotX + 15.0f, -18.0f);
            Vec p1(spotX + 5.0f, -18.0f);
            Vec p2(spotX, -36.0f);
            Vec p3(spotX, -50.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            // Moving in reverse: vehicle heading points opposite of motion direction
            anc_ = -tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(0.0f, 0.4f);
                state_ = CarState::PARKED;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::PARKING_TOP: {
            progress_ += 0.014f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            Vec p0(spotX - 15.0f, 48.0f);
            Vec p1(spotX - 5.0f, 48.0f);
            Vec p2(spotX, 66.0f);
            Vec p3(spotX, 80.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            // Moving in reverse: vehicle heading points opposite of motion direction
            anc_ = -tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(0.0f, -0.4f);
                state_ = CarState::PARKED;
                progress_ = 0.0f;
            }
            break;
        }

        default:
            break;
    }
}

void CarAgent::stepDeparture(const std::vector<CarAgent>& allAgents) {
    int lot = targetSpot_;
    float spotX = (lot <= 4) ? (-68.0f + 34.0f * lot) : (238.0f - 34.0f * lot);

    switch (state_) {
        // --- Bottom Row Stalls (0..4) ---
        case CarState::UNPARKING_BOTTOM_PULLOUT: {
            if (!isAisleClear(allAgents)) {
                if (pos_.getY() < -46.0f) {
                    pos_.setY(-50.0f);
                }
                break; // Car getting out stops and waits inside stall!
            }
            pos_.setY(pos_.getY() + 0.5f);
            anc_ = Vec(0.0f, 0.4f);
            if (pos_.getY() >= -36.0f) {
                pos_.setY(-36.0f);
                state_ = CarState::UNPARKING_BOTTOM_TURN;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::UNPARKING_BOTTOM_TURN: {
            progress_ += 0.025f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            // Turn Eastward into the one-way circle flow
            Vec p0(spotX, -36.0f);
            Vec p1(spotX, -24.0f);
            Vec p2(spotX + 8.0f, -18.0f);
            Vec p3(spotX + 16.0f, -18.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            anc_ = tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(0.4f, 0.0f);
                if (pos_.getX() >= 84.0f) {
                    pos_.setX(84.0f);
                    state_ = CarState::TURNING_LOOP;
                } else {
                    state_ = CarState::DRIVING_AISLE_BOTTOM_EXIT;
                }
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::DRIVING_AISLE_BOTTOM_EXIT: {
            // Drive Eastward along bottom aisle to the turnaround loop
            pos_.setX(pos_.getX() + 0.5f);
            anc_ = Vec(0.4f, 0.0f);
            if (pos_.getX() >= 84.0f) {
                pos_.setX(84.0f);
                state_ = CarState::TURNING_LOOP;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::TURNING_LOOP: {
            // Follow the one-way circle turnaround loop around East end to top aisle
            progress_ += 0.0045f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            Vec p0(84.0f, -18.0f);
            Vec p1(118.0f, -18.0f);
            Vec p2(118.0f, 48.0f);
            Vec p3(84.0f, 48.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            anc_ = tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(-0.4f, 0.0f);
                state_ = CarState::DRIVING_AISLE_TOP_EXIT;
                progress_ = 0.0f;
            }
            break;
        }

        // --- Top Row Stalls (5..9) ---
        case CarState::UNPARKING_TOP_PULLOUT: {
            if (!isAisleClear(allAgents)) {
                if (pos_.getY() > 76.0f) {
                    pos_.setY(80.0f);
                }
                break; // Car getting out stops and waits inside stall!
            }
            pos_.setY(pos_.getY() - 0.5f);
            anc_ = Vec(0.0f, -0.4f);
            if (pos_.getY() <= 66.0f) {
                pos_.setY(66.0f);
                state_ = CarState::UNPARKING_TOP_TURN;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::UNPARKING_TOP_TURN: {
            progress_ += 0.025f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            // Turn Westward into the one-way circle flow
            Vec p0(spotX, 66.0f);
            Vec p1(spotX, 54.0f);
            Vec p2(spotX - 8.0f, 48.0f);
            Vec p3(spotX - 16.0f, 48.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            anc_ = tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(-0.4f, 0.0f);
                state_ = CarState::DRIVING_AISLE_TOP_EXIT;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::DRIVING_AISLE_TOP_EXIT: {
            // Drive Westward along top aisle to the corridor turn
            pos_.setX(pos_.getX() - 0.5f);
            anc_ = Vec(-0.4f, 0.0f);
            if (pos_.getX() <= -98.0f) {
                pos_.setX(-98.0f);
                state_ = CarState::TURNING_CORRIDOR_TOP;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::TURNING_CORRIDOR_TOP: {
            progress_ += 0.025f;
            if (progress_ > 1.0f) progress_ = 1.0f;

            Vec p0(-98.0f, 48.0f);
            Vec p1(-106.0f, 48.0f);
            Vec p2(-110.0f, 44.0f);
            Vec p3(-110.0f, 36.0f);

            pos_ = evalBezier(p0, p1, p2, p3, progress_);
            Vec tan = evalBezierTangent(p0, p1, p2, p3, progress_);
            anc_ = tan.normalized() * 0.4f;

            if (progress_ >= 1.0f) {
                pos_ = p3;
                anc_ = Vec(0.0f, -0.4f);
                state_ = CarState::DRIVING_CORRIDOR_SOUTH;
                progress_ = 0.0f;
            }
            break;
        }

        case CarState::DRIVING_CORRIDOR_SOUTH: {
            pos_.setY(pos_.getY() - 0.6f);
            anc_ = Vec(0.0f, -0.4f);
            if (pos_.getY() <= -44.0f) {
                state_ = CarState::EXITING_GATE;
            }
            break;
        }

        // --- Common Exit Gate Corridor ---
        case CarState::EXITING_GATE: {
            pos_.setY(pos_.getY() - 0.6f);
            anc_ = Vec(0.0f, -0.4f);
            if (pos_.getY() < -110.0f) {
                state_ = CarState::DEPARTED;
            }
            break;
        }

        default:
            break;
    }
}

void CarAgent::update(const std::vector<CarAgent>& allAgents) {
    if (state_ == CarState::IDLE || state_ == CarState::PARKED || state_ == CarState::DEPARTED) {
        return;
    }

    bool yielding = shouldYield(allAgents);
    if (yielding) {
        yieldFrames_++;
        // Failsafe timeout: if yielding for > 180 frames (~3 sec) and nothing directly touching (< 12 units), allow creep
        bool blockedAhead = false;
        for (const auto& other : allAgents) {
            if (other.vehicleId_ == vehicleId_ || !other.isMoving()) continue;
            Vec toOther = other.pos_ - pos_;
            float dist = toOther.length();
            if (dist < 12.0f) {
                blockedAhead = true;
                break;
            }
        }
        if (yieldFrames_ < 180 || blockedAhead) {
            return;
        }
    } else {
        yieldFrames_ = 0;
    }

    if (!isDeparting_) {
        stepParking(allAgents);
    } else {
        stepDeparture(allAgents);
    }
    carFig_ = CarFig(pos_, anc_);
}

void CarAgent::buildVertices(RenderBatch& batch) const {
    if (state_ == CarState::IDLE || state_ == CarState::DEPARTED) {
        return;
    }

    if (type_ == VehicleType::MOTORBIKE || type_ == VehicleType::BICYCLE) {
        Vec fwd = anc_.normalized();
        Vec perp(-fwd.getY(), fwd.getX());
        Color bikeColor = (type_ == VehicleType::MOTORBIKE) ? Color(0.9f, 0.5f, 0.1f) : Color(0.1f, 0.7f, 0.8f);

        // Body chassis
        Vec p0 = pos_ - fwd * 8.0f - perp * 3.0f;
        Vec p1 = pos_ + fwd * 8.0f - perp * 3.0f;
        Vec p2 = pos_ + fwd * 8.0f + perp * 3.0f;
        Vec p3 = pos_ - fwd * 8.0f + perp * 3.0f;
        batch.addQuad(p0.getX(), p0.getY(), p1.getX(), p1.getY(),
                      p2.getX(), p2.getY(), p3.getX(), p3.getY(), bikeColor);

        // Wheels: front and rear
        Vec frontWheel = pos_ + fwd * 7.0f;
        Vec rearWheel = pos_ - fwd * 7.0f;
        batch.addCircle(frontWheel.getX(), frontWheel.getY(), 2.5f, Color::Black());
        batch.addCircle(rearWheel.getX(), rearWheel.getY(), 2.5f, Color::Black());
    } else {
        carFig_.buildVertices(batch);
    }
}
