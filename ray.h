#pragma once

#include "math_objects.h"

class Ray {
public:
    Math::Vec3 pos;
    Math::Vec3 dir;
    double wavelength;  // nm, in [380, 780]

    Ray() : pos(), dir(), wavelength(550.0) {}

    Ray(Math::Vec3 P, Math::Vec3 D, double lambda = 550.0) {
        pos = P;
        dir = D.Normalize();
        wavelength = lambda;
    }

    const Math::Vec3& direction() const { return dir; }

    Math::Vec3 at(double t) const {
        return pos + dir * t;
    }
};
