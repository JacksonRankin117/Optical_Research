#pragma once
#include "math_objects.h"
#include <cmath>
#include <algorithm>

namespace Colorspace {

    // XYZ to linear sRGB (D65 illuminant)
    // Source: IEC 61966-2-1
    inline Math::Vec3 XYZToSRGB(double X, double Y, double Z) {
        double r =  3.2404542*X - 1.5371385*Y - 0.4985314*Z;
        double g = -0.9692660*X + 1.8760108*Y + 0.0415560*Z;
        double b =  0.0556434*X - 0.2040259*Y + 1.0572252*Z;
        return Math::Vec3(r, g, b);
    }

    // Reinhard tone mapping — maps HDR to [0, 1]
    inline Math::Vec3 Reinhard(const Math::Vec3& c) {
        return Math::Vec3(
            c.X / (1.0 + c.X),
            c.Y / (1.0 + c.Y),
            c.Z / (1.0 + c.Z)
        );
    }

    // ACES filmic tone mapping approximation (Narkowicz 2015)
    inline double acesChannel(double x) {
        constexpr double a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
        return std::clamp((x*(a*x+b)) / (x*(c*x+d)+e), 0.0, 1.0);
    }

    inline Math::Vec3 ACES(const Math::Vec3& c) {
        return Math::Vec3(acesChannel(c.X), acesChannel(c.Y), acesChannel(c.Z));
    }

    // Gamma encode for sRGB display (piecewise, proper IEC standard)
    inline double gammaEncode(double linear) {
        if (linear <= 0.0031308)
            return 12.92 * linear;
        return 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
    }

    inline Math::Vec3 GammaEncode(const Math::Vec3& c) {
        return Math::Vec3(gammaEncode(c.X), gammaEncode(c.Y), gammaEncode(c.Z));
    }
}
