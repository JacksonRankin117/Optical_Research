#pragma once
#include "math_objects.h"

// Film frame basis vectors in world space
struct FilmFrame {
    Math::Vec3 origin;
    Math::Vec3 right;
    Math::Vec3 up;
    Math::Vec3 normal;
};

// Lens frame basis vectors in world space
struct LensFrame {
    Math::Vec3 origin;
    Math::Vec3 right;
    Math::Vec3 up;
    Math::Vec3 normal;
};

// Snapshot of camera geometry passed to optics at ray generation time
struct CameraGeometry {
    FilmFrame film;
    LensFrame lens;
};
