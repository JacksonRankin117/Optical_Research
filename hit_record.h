#pragma once

#include "material.h"
#include "math_objects.h"
#include "ray.h"

// Forward declaration so the compiler doesn't yell at me
class Material;

// Helper struct
struct HitRecord {
    Math::Vec3 p;          // hit point
    Math::Vec3 normal;       // surface normal
    double t;          // ray parameter

    bool frontFace;

    Material* material;

    void setFaceNormal(const Ray& r, const Math::Vec3& outwardNormal) {
        frontFace = Math::Vec3::Dot(r.direction(), outwardNormal) < 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};
