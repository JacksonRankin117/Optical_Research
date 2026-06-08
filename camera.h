#pragma once
#include "math_objects.h"
#include "camera_geometry.h"
#include "optics.h"
#include "ray.h"

// =====================================================================================================================
// =================================================== Back Standard ===================================================
// =====================================================================================================================
// The back standard holds the film. It can be shifted, risen/fallen, swung, and tilted
// independently of the front standard to control perspective and plane of focus.
class BackStandard {
public:
    double width;      // Physical width of film (meters)
    double height;     // Physical height of film (meters)
    double res;        // Pixels across the width
    double rise_fall;  // Vertical shift of film center (meters)
    double shift;      // Horizontal shift of film center (meters)
    double swing;      // Rotation around vertical axis (radians)
    double tilt;       // Rotation around horizontal axis (radians)

    BackStandard(double width,
                 double height,
                 double res,
                 double rise_fall = 0.0,
                 double shift     = 0.0,
                 double swing_deg = 0.0,
                 double tilt_deg  = 0.0)
        : width(width),
          height(height),
          res(res),
          rise_fall(rise_fall),
          shift(shift),
          swing(Math::Vec3::DegToRad(swing_deg)),
          tilt(Math::Vec3::DegToRad(tilt_deg))
    {}

    // Build film frame in world space (relative to camera origin)
    FilmFrame BuildFrame(const Math::Vec3& cam_right,
                         const Math::Vec3& cam_up,
                         const Math::Vec3& cam_forward,
                         double rail_extension) const
    {
        FilmFrame frame;

        // Film sits behind the lens at -rail_extension along the forward axis
        frame.origin = cam_forward * (-rail_extension)
                     + cam_right   * shift
                     + cam_up      * rise_fall;

        Math::Vec3 right  = cam_right;
        Math::Vec3 up     = cam_up;
        Math::Vec3 normal = cam_forward;

        // Swing: rotate around the film's up axis
        right  = Math::Vec3::RotateAroundAxis(right,  up, swing);
        normal = Math::Vec3::RotateAroundAxis(normal, up, swing);

        // Tilt: rotate around the film's right axis
        up     = Math::Vec3::RotateAroundAxis(up,     right, tilt);
        normal = Math::Vec3::RotateAroundAxis(normal, right, tilt);

        frame.right  = right;
        frame.up     = up;
        frame.normal = normal;

        return frame;
    }

    // Pixel dimensions derived from film aspect ratio and resolution
    int pixelWidth()  const { return static_cast<int>(res); }
    int pixelHeight() const { return static_cast<int>(res * height / width); }
};

// =====================================================================================================================
// ================================================== Front Standard ===================================================
// =====================================================================================================================
// The front standard holds the lens (optic). It can also be shifted, risen/fallen,
// swung, and tilted to control focus plane and perspective independently.
class FrontStandard {
public:
    double rise_fall;
    double shift;
    double swing;  // radians
    double tilt;   // radians
    Optic* optic;  // The front standard knows about the optic, but doesn't own it.

    FrontStandard(Optic* optic,
                  double rise_fall = 0.0,
                  double shift     = 0.0,
                  double swing_deg = 0.0,
                  double tilt_deg  = 0.0)
        : optic(optic),
          rise_fall(rise_fall),
          shift(shift),
          swing(Math::Vec3::DegToRad(swing_deg)),
          tilt(Math::Vec3::DegToRad(tilt_deg))
    {}

    // Build lens frame in world space (relative to camera origin)
    LensFrame BuildFrame(const Math::Vec3& cam_right,
                         const Math::Vec3& cam_up,
                         const Math::Vec3& cam_forward) const
    {
        LensFrame frame;

        // Lens sits at the camera origin, shifted by front standard movements
        frame.origin = cam_right * shift
                     + cam_up    * rise_fall;

        Math::Vec3 right  = cam_right;
        Math::Vec3 up     = cam_up;
        Math::Vec3 normal = cam_forward;

        right  = Math::Vec3::RotateAroundAxis(right,  up,    swing);
        normal = Math::Vec3::RotateAroundAxis(normal, up,    swing);
        up     = Math::Vec3::RotateAroundAxis(up,     right, tilt);
        normal = Math::Vec3::RotateAroundAxis(normal, right, tilt);

        frame.right  = right;
        frame.up     = up;
        frame.normal = normal;

        return frame;
    }
};

// =====================================================================================================================
// ================================================ Camera =============================================================
// =====================================================================================================================
class Camera {
public:
    BackStandard  back;
    FrontStandard front;
    double        rail_extension;  // Distance between front and back standard (meters)
    Math::Vec3    position;
    Math::Vec3    target;
    Math::Vec3    worldUp;

    // Derived basis vectors (built in constructor)
    Math::Vec3 forward;
    Math::Vec3 right;
    Math::Vec3 up;

    Camera(BackStandard  back,
           FrontStandard front,
           double        rail_extension,
           Math::Vec3    position,
           Math::Vec3    target,
           Math::Vec3    worldUp)
        : back(back),
          front(front),
          rail_extension(rail_extension),
          position(position),
          target(target),
          worldUp(worldUp)
    {
        BuildBasis();
    }

    void BuildBasis()
    {
        forward = (target - position).Normalized();
        right   = Math::Vec3::Cross(forward, worldUp).Normalized();
        up      = Math::Vec3::Cross(right, forward).Normalized();
    }

    // Build a full geometry snapshot. Called once per ray generation.
    CameraGeometry BuildGeometry() const
    {
        CameraGeometry geo;

        geo.lens = front.BuildFrame(right, up, forward);
        geo.film = back.BuildFrame(right, up, forward, rail_extension);

        // Translate both frames into world space
        geo.lens.origin += position;
        geo.film.origin += position;

        return geo;
    }

    // Generate a ray for pixel (px, py)
    Ray GenerateRay(int px, int py) const
    {
        CameraGeometry geo = BuildGeometry();

        // Map pixel coordinates to physical film position
        double u = (px + 0.5) / back.pixelWidth();
        double v = (py + 0.5) / back.pixelHeight();

        // Center the film: u,v in [0,1] -> x,y in [-w/2, w/2]
        double x =  (u - 0.5) * back.width;
        double y = -(v - 0.5) * back.height;  // flip Y: pixel row 0 = top of frame

        Math::Vec3 film_point = geo.film.origin
                              + geo.film.right * x
                              + geo.film.up    * y;

        return front.optic->SampleRay(film_point, geo);
    }
};
