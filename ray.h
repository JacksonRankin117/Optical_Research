#pragma once

#include "math_objects.h"

class Ray {
public:
    Math::Vec3 pos;
    Math::Vec3 dir;

    // Initialize a zero-ray
    Ray() : pos(), dir() {}

    // Arbitrary ray
    Ray(Math::Vec3 P, Math::Vec3 D) {
        pos = P;
        dir = D.Normalize();  // Unit vector for direction
    }

};
