#pragma once
#include <random>
#include <cmath>
#include "ray.h"
#include "hit_record.h"
#include "spd.h"

class HitRecord;

class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(
        const Ray& rayIn,
        const HitRecord& rec,
        double& attenuation,    // spectral attenuation at ray.wavelength
        Ray& scattered
    ) const = 0;
};

class Lambertian : public Material {
public:
    const SPD* albedo;

    Lambertian(const SPD* spd) : albedo(spd) {}

    bool scatter(
        const Ray& rayIn,
        const HitRecord& rec,
        double& attenuation,
        Ray& scattered
    ) const override {
        Math::Vec3 scatterDir = rec.normal + Math::Vec3::RandomUnitVector();
        if (scatterDir.NearZero()) scatterDir = rec.normal;

        scattered = Ray(rec.p, scatterDir, rayIn.wavelength);
        attenuation = albedo->evaluate(rayIn.wavelength);
        return true;
    }
};

class Metallic : public Material {
public:
    const SPD* albedo;
    double fuzz;

    Metallic(const SPD* spd, double fuzz = 0.0)
        : albedo(spd), fuzz(std::clamp(fuzz, 0.0, 1.0)) {}

    bool scatter(
        const Ray& rayIn,
        const HitRecord& rec,
        double& attenuation,
        Ray& scattered
    ) const override {
        Math::Vec3 reflected = reflect(rayIn.dir.Normalized(), rec.normal);
        scattered = Ray(rec.p, reflected + fuzz * Math::Vec3::RandomUnitVector(), rayIn.wavelength);
        attenuation = albedo->evaluate(rayIn.wavelength);
        return Math::Vec3::Dot(scattered.dir, rec.normal) > 0;
    }

private:
    static Math::Vec3 reflect(const Math::Vec3& v, const Math::Vec3& n) {
        return v - 2.0 * Math::Vec3::Dot(v, n) * n;
    }
};

class Dielectric : public Material {
public:
    double cauchyA;  // Unitless
    double cauchyB;  // μm²

    // e.g. crown glass:    A=1.5220, B=0.00459
    //      flint glass:    A=1.6200, B=0.01050
    //      fused silica:   A=1.4580, B=0.00354
    Dielectric(double A, double B) : cauchyA(A), cauchyB(B) {}

    bool scatter(
        const Ray& rayIn,
        const HitRecord& rec,
        double& attenuation,
        Ray& scattered
    ) const override {
        attenuation = 1.0;  // Glass absorbs nothing

        double ior = cauchyIOR(rayIn.wavelength);
        double refractionRatio = rec.frontFace ? (1.0 / ior) : ior;

        Math::Vec3 unitDir = rayIn.dir.Normalized();
        double cosTheta = std::min(Math::Vec3::Dot(-unitDir, rec.normal), 1.0);
        double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);

        bool totalInternalReflection = refractionRatio * sinTheta > 1.0;

        Math::Vec3 direction;
        if (totalInternalReflection || schlick(cosTheta, refractionRatio) > randomDouble()) {
            direction = reflect(unitDir, rec.normal);
        } else {
            direction = refract(unitDir, rec.normal, refractionRatio);
        }

        scattered = Ray(rec.p, direction, rayIn.wavelength);
        return true;
    }

private:
    // Cauchy's equation — lambda in nm, B in μm²
    double cauchyIOR(double lambdaNm) const {
        double lambdaMicron = lambdaNm * 1e-3;  // nm → μm
        return cauchyA + cauchyB / (lambdaMicron * lambdaMicron);
    }

    static double schlick(double cosine, double ior) {
        double r0 = (1.0 - ior) / (1.0 + ior);
        r0 = r0 * r0;
        return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
    }

    static Math::Vec3 reflect(const Math::Vec3& v, const Math::Vec3& n) {
        return v - 2.0 * Math::Vec3::Dot(v, n) * n;
    }

    static Math::Vec3 refract(const Math::Vec3& uv, const Math::Vec3& n, double ratio) {
        double cosTheta = std::min(Math::Vec3::Dot(-uv, n), 1.0);
        Math::Vec3 rayPerp = ratio * (uv + cosTheta * n);
        Math::Vec3 rayPara = -std::sqrt(std::abs(1.0 - rayPerp.MagnitudeSquared())) * n;
        return rayPerp + rayPara;
    }

    static double randomDouble() {
        static thread_local std::mt19937_64 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng);
    }
};
