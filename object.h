#pragma once

#include "material.h"
#include "math_objects.h"
#include "ray.h"
#include "hit_record.h"

class Object {
public:
    virtual ~Object() = default;
    virtual bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const = 0;
};

class Sphere : public Object {
public:
    Math::Vec3 pos;
    double r;
    Material* mat;

    Sphere(Math::Vec3 position, double radius, Material* material)
        : pos(position), r(radius), mat(material) {}

    bool intersect(const Ray& ray, double tMin, double tMax, HitRecord& rec) const override {
        Math::Vec3 oc = ray.pos - pos;
        double a = ray.dir.MagnitudeSquared();
        double halfB = Math::Vec3::Dot(oc, ray.dir);
        double c = oc.MagnitudeSquared() - r * r;
        double discriminant = halfB*halfB - a*c;

        if (discriminant < 0) return false;

        double sqrtD = std::sqrt(discriminant);
        double root = (-halfB - sqrtD) / a;

        // Find the nearest root in [tMin, tMax]
        if (root <= tMin || root >= tMax) {
            root = (-halfB + sqrtD) / a;
            if (root <= tMin || root >= tMax) return false;
        }

        rec.t = root;
        rec.p = ray.pos + ray.dir * rec.t;
        Math::Vec3 outwardNormal = (rec.p - pos) / r;
        rec.setFaceNormal(ray, outwardNormal);
        rec.material = mat;
        return true;
    }
};
