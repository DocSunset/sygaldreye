// Copyright 2024 Travis West
#include "grab_detector.hpp"
#include <limits>
#include "grab_target.hpp"

void update_grabs(std::span<const HandState> hands,
                  std::span<GrabTarget>      targets)
{
    // Release: clear grabs for hands no longer pressing
    for (GrabTarget& tgt : targets) {
        if (!tgt.grabbed) { continue; }
        int hand = tgt.grabbing_hand;
        if (hand < 0 || hand >= static_cast<int>(hands.size())) { continue; }
        if (!hands[hand].valid || !hands[hand].grip_pressed) {
            tgt.grabbed       = false;
            tgt.grabbing_hand = -1;
        }
    }
    // Grab: for each valid pressing hand, find closest ungrabbed target in range
    for (int idx = 0; idx < static_cast<int>(hands.size()); ++idx) {
        const HandState& hand = hands[idx];
        if (!hand.valid || !hand.grip_pressed) { continue; }
        // Check if this hand already holds something
        bool already_grabbing = false;
        for (const GrabTarget& tgt : targets) {
            if (tgt.grabbed && tgt.grabbing_hand == idx) {
                already_grabbing = true;
                break;
            }
        }
        if (already_grabbing) { continue; }
        // Find closest ungrabbed target in range
        float        closest_dist = std::numeric_limits<float>::max();
        GrabTarget*  closest      = nullptr;
        for (GrabTarget& tgt : targets) {
            if (tgt.grabbed) { continue; }
            float dist = (tgt.position - hand.position).norm();
            if (dist <= tgt.radius && dist < closest_dist) {
                closest_dist = dist;
                closest      = &tgt;
            }
        }
        if (closest != nullptr) {
            closest->grabbed       = true;
            closest->grabbing_hand = idx;
            closest->grab_offset   = closest->position - hand.position;
        }
    }
}
