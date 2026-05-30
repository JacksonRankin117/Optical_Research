#pragma once
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <limits>
#include <cmath>
#include <random>
#include "camera.h"
#include "object.h"
#include "hit_record.h"
#include "cie.h"
#include "colorspace.h"

class Scene {
public:
    Scene(Camera& camera, std::vector<std::unique_ptr<Object>> objects)
        : camera(camera), objects(std::move(objects)) {}

    bool hit(const Ray& ray, double tMin, double tMax, HitRecord& rec) const {
        HitRecord temp;
        bool hitAnything = false;
        double closest = tMax;

        for (const auto& obj : objects) {
            if (obj->intersect(ray, tMin, closest, temp)) {
                hitAnything = true;
                closest = temp.t;
                rec = temp;
            }
        }
        return hitAnything;
    }

    void Render(const std::string& filename, int maxDepth = 20) const {
        Pinhole& cam = static_cast<Pinhole&>(camera);

        // Accumulate XYZ per pixel
        struct XYZPixel { double X = 0, Y = 0, Z = 0; };
        std::vector<XYZPixel> pixels(cam.width * cam.height);

        // Wavelength range
        constexpr double lambdaMin = CIE::LAMBDA_MIN;
        constexpr double lambdaMax = CIE::LAMBDA_MAX;
        constexpr double lambdaRange = lambdaMax - lambdaMin;

        for (int j = 0; j < cam.height; ++j) {
            std::cerr << "\rScanlines remaining: " << (cam.height - j) << "   " << std::flush;
            for (int i = 0; i < cam.width; ++i) {
                XYZPixel& px = pixels[j * cam.width + i];

                for (int s = 0; s < cam.samples; ++s) {
                    // Sample a random wavelength uniformly over the visible spectrum
                    double lambda = lambdaMin + randomDouble() * lambdaRange;

                    Ray ray = cam.GenerateRay(i, j);
                    ray.wavelength = lambda;

                    // Trace returns spectral intensity at this wavelength
                    double intensity = traceRay(ray, maxDepth);

                    // Weight by CIE color matching functions
                    auto [x, y, z] = CIE::sample(lambda);
                    px.X += intensity * x;
                    px.Y += intensity * y;
                    px.Z += intensity * z;
                }

                // Normalize — scale by wavelength range and divide by sample count
                // The lambdaRange/N factor comes from Monte Carlo integration
                double scale = lambdaRange / (cam.samples * CIE::Y_INTEGRAL);
                px.X *= scale;
                px.Y *= scale;
                px.Z *= scale;
            }

             std::cerr << "\rScanlines remaining: " << 0 << "   " << std::flush;
        }

        std::cerr << "\nWriting " << filename << "...\n";

        // Write PFM — binary, little-endian, top-down
        std::ofstream out(filename, std::ios::binary);
        out << "PF\n" << cam.width << " " << cam.height << "\n-1.0\n";

        for (int j = 0; j < cam.height; ++j) {
            for (int i = 0; i < cam.width; ++i) {
                const XYZPixel& px = pixels[j * cam.width + i];

                // XYZ → linear sRGB → tone map → gamma encode
                Math::Vec3 rgb = Colorspace::XYZToSRGB(px.X, px.Y, px.Z);
                rgb = Colorspace::ACES(rgb);
                rgb = Colorspace::GammaEncode(rgb);

                float r = static_cast<float>(std::clamp(rgb.X, 0.0, 1.0));
                float g = static_cast<float>(std::clamp(rgb.Y, 0.0, 1.0));
                float b = static_cast<float>(std::clamp(rgb.Z, 0.0, 1.0));

                out.write(reinterpret_cast<const char*>(&r), sizeof(float));
                out.write(reinterpret_cast<const char*>(&g), sizeof(float));
                out.write(reinterpret_cast<const char*>(&b), sizeof(float));
            }
        }
        std::cerr << "Done. Output written to " << filename << "\n";
    }

private:
    Camera& camera;
    std::vector<std::unique_ptr<Object>> objects;

    // Returns spectral intensity (scalar) at ray.wavelength
    double traceRay(const Ray& ray, int maxDepth) const {
        if (maxDepth <= 0) return 0.0;

        HitRecord rec;
        if (hit(ray, 1e-4, std::numeric_limits<double>::infinity(), rec)) {
            Ray scattered;
            double attenuation;
            if (rec.material->scatter(ray, rec, attenuation, scattered)) {
                return attenuation * traceRay(scattered, maxDepth - 1);
            }
            return 0.0;
        }

        // Background: sky gradient — returns intensity modulated by wavelength
        Math::Vec3 unitDir = ray.dir.Normalized();
        double t = 0.5 * (unitDir.Z + 1.0);

        // Sky SPD: interpolate between warm white and blue sky
        // Approximate a Rayleigh-ish sky by boosting short wavelengths
        double skyBlue  = std::exp(-0.5 * std::pow((ray.wavelength - 460.0) / 40.0, 2.0));
        double skyWhite = 1.0;
        return (1.0 - t) * skyWhite + t * (0.5 + 0.5 * skyBlue);
    }

    static double randomDouble() {
        static thread_local std::mt19937_64 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng);
    }
};
