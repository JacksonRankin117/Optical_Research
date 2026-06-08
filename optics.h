#pragma once
#include <random>
#include <cmath>
#include <numbers>
#include "ray.h"
#include "math_objects.h"
#include "camera_geometry.h"

// =====================================================================================================================
// =================================================== Optic Base ======================================================
// =====================================================================================================================
class Optic {
public:
    int Samples;

    Optic() : Samples(1) {}
    virtual ~Optic() = default;

    virtual Ray SampleRay(const Math::Vec3& film_point,
                          const CameraGeometry& geo) const = 0;
};

// =====================================================================================================================
// ==================================================== Pinhole ========================================================
// =====================================================================================================================
class Pinhole : public Optic {
public:
    double radius;

    Pinhole(double r = 0.0, int s = 1) : radius(r) { Samples = s; }

    Ray SampleRay(const Math::Vec3& film_point,
                  const CameraGeometry& geo) const override
    {
        Math::Vec3 origin;

        if (radius < 1e-12) {
            // True pinhole — single point, no depth of field
            origin = geo.lens.origin;
        } else {
            // Sample a random point within the aperture disk for depth of field
            static thread_local std::mt19937_64 rng(std::random_device{}());
            std::uniform_real_distribution<double> dist(0.0, 1.0);

            double angle = dist(rng) * 2.0 * std::numbers::pi;
            double r     = radius * std::sqrt(dist(rng));

            origin = geo.lens.origin
                   + geo.lens.right * (r * std::cos(angle))
                   + geo.lens.up    * (r * std::sin(angle));
        }

        Math::Vec3 dir = origin - film_point;
        return Ray(origin, dir);
    }
};

// =====================================================================================================================
// ==================================================== Thin Lens ======================================================
// =====================================================================================================================
class ThinLens : public Optic {
public:
    double focalLength;
    double radius;

    ThinLens() : focalLength(0.0), radius(0.0) { Samples = 1; }

    Ray SampleRay(const Math::Vec3& film_point,
                  const CameraGeometry& geo) const override
    {
        // Distance from film to lens
        double d_film = (geo.lens.origin - geo.film.origin).Magnitude();

        // Thin lens equation: 1/f = 1/d_film + 1/d_focus
        double d_focus = (focalLength * d_film) / (d_film - focalLength);

        // Chief ray: from film point through lens center
        // The direction INTO the scene is lens→film flipped
        Math::Vec3 through_lens = (geo.lens.origin - film_point).Normalized();

        // Focus point is in FRONT of the lens (opposite side from film)
        Math::Vec3 focus_point = geo.lens.origin + through_lens * d_focus;

        // Sample aperture disk
        static thread_local std::mt19937_64 rng(std::random_device{}());
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        double angle = dist(rng) * 2.0 * std::numbers::pi;
        double r     = radius * std::sqrt(dist(rng));

        Math::Vec3 origin = geo.lens.origin
                          + geo.lens.right * (r * std::cos(angle))
                          + geo.lens.up    * (r * std::sin(angle));

        // Ray goes from aperture sample point toward the focus point
        Math::Vec3 dir = (focus_point - origin).Normalized();
        return Ray(origin, dir);
    }

    // Set focal length directly
    void SetFocalLength(double f) {
        focalLength = f;
    }

    // Calculate focal length from camera position/target and rail extension
    // using the thin lens equation: 1/f = 1/v + 1/u
    void CalcFocalLength(Math::Vec3 cameraPosition,
                         Math::Vec3 cameraTarget,
                         double rail_extension)
    {
        double subject_distance = Math::Vec3::DistanceBetween(cameraPosition, cameraTarget);
        focalLength = 1.0 / (1.0 / subject_distance + 1.0 / rail_extension);
    }

    // Set aperture radius from f-number
    void SetFNumber(double f_number) {
        radius = focalLength / (2.0 * f_number);
    }

    static double CalcRailExt(const double focal_length, const Math::Vec3 c_pos, const Math::Vec3 c_target) {
        // Calculate Rail Extension (thin lens)
        double sub_dist = Math::Vec3::DistanceBetween(c_pos, c_target);  // Distance from the front standard to the subject
        double rail = 1 / (1 / focal_length - 1 / sub_dist);
        return rail;
    }
};
