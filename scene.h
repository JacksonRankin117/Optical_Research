#pragma once
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <limits>
#include <cmath>
#include <random>
#include <thread>
#include <atomic>
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
                closest     = temp.t;
                rec         = temp;
            }
        }
        return hitAnything;
    }

    void Render(const std::string& filename, int maxDepth = 16) const {
        const int W = camera.back.pixelWidth();
        const int H = camera.back.pixelHeight();
        const int S = camera.front.optic->Samples;
        const int total = W * H;

        // Accumulate XYZ per pixel
        struct XYZPixel { double X = 0, Y = 0, Z = 0; };
        std::vector<XYZPixel> pixels(total);

        constexpr double lambdaMin   = CIE::LAMBDA_MIN;
        constexpr double lambdaMax   = CIE::LAMBDA_MAX;
        constexpr double lambdaRange = lambdaMax - lambdaMin;
        const double scale = lambdaRange / (S * CIE::Y_INTEGRAL);

        // Thread pool setup
        const unsigned int nThreads = std::thread::hardware_concurrency();
        constexpr int CHUNK = 16;  // pixels per work unit

        std::atomic<int> nextPixel{0};
        std::atomic<int> pixelsDone{0};

        std::cerr << "Rendering " << W << "x" << H
                  << " at " << S << " spp"
                  << " using " << nThreads << " threads\n";

        auto worker = [&]() {
            while (true) {
                int start = nextPixel.fetch_add(CHUNK);
                if (start >= total) break;

                for (int k = 0; k < CHUNK; ++k) {
                    int idx = start + k;
                    if (idx >= total) break;

                    int i = idx % W;
                    int j = idx / W;

                    XYZPixel px;

                    constexpr int N_HERO = 4;
                    const double scale = lambdaRange / (S * N_HERO * CIE::Y_INTEGRAL);

                    for (int s = 0; s < S; ++s) {
                        double stratum    = (s + randomDouble()) / S;
                        double heroLambda = lambdaMin + stratum * lambdaRange;

                        Ray baseRay = camera.GenerateRay(i, j);  // one ray per sample

                        for (int h = 0; h < N_HERO; ++h) {
                            double lambda = lambdaMin + std::fmod(
                                (heroLambda - lambdaMin) + h * (lambdaRange / N_HERO),
                                lambdaRange
                            );

                            Ray ray        = baseRay;            // copy, don't regenerate
                            ray.wavelength = lambda;

                            double intensity = traceRay(ray, maxDepth);
                            intensity = std::min(intensity, 10.0);  // Clamp brighter samples.
                            double d65       = D65(lambda);
                            auto [x, y, z]   = CIE::sample(lambda);
                            px.X += intensity * x * d65;
                            px.Y += intensity * y * d65;
                            px.Z += intensity * z * d65;
                        }
                    }

                    px.X *= scale;
                    px.Y *= scale;
                    px.Z *= scale;

                    pixels[idx] = px;

                    // Progress reporting
                    int done = pixelsDone.fetch_add(1) + 1;
                    if (done % 5000 == 0 || done == total)
                        std::cerr << "\r" << done << " / " << total
                                  << " pixels  (" << (100 * done / total) << "%)" << std::flush;
                }
            }
        };

        // Launch threads
        std::vector<std::thread> threads;
        threads.reserve(nThreads);
        for (unsigned int t = 0; t < nThreads; ++t)
            threads.emplace_back(worker);
        for (auto& t : threads)
            t.join();

        std::cerr << "\nWriting " << filename << "...\n";

        // Write PFM — binary, little-endian, top-down
        std::ofstream out(filename, std::ios::binary);
        out << "PF\n" << W << " " << H << "\n-1.0\n";

        for (int j = 0; j < H; ++j) {
            for (int i = 0; i < W; ++i) {
                const XYZPixel& px = pixels[j * W + i];

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

    double traceRay(const Ray& ray, int maxDepth) const {
        if (maxDepth <= 0) return 0.0;

        HitRecord rec;
        if (hit(ray, 1e-4, std::numeric_limits<double>::infinity(), rec)) {
            Ray    scattered;
            double attenuation;
            if (rec.material->scatter(ray, rec, attenuation, scattered))
                return attenuation * traceRay(scattered, maxDepth - 1);
            return 0.0;
        }

        // Background sky weighted by D65
        Math::Vec3 unitDir  = ray.dir.Normalized();
        double t            = 0.5 * (unitDir.Z + 1.0);
        double skyBlue      = std::exp(-0.5 * std::pow((ray.wavelength - 460.0) / 40.0, 2.0));
        double skyIntensity = (1.0 - t) * 1.0 + t * (0.5 + 0.5 * skyBlue);
        return skyIntensity;
    }

    // CIE D65 illuminant, 380-780nm at 10nm steps, normalized to 1.0 at 560nm
    static double D65(double lambda) {
        static constexpr double table[] = {
            49.98,  52.31,  54.65,  68.70,  82.75,  87.12,  91.49,  92.46,
            93.43,  90.06,  86.68,  95.77, 104.86, 110.94, 117.01, 117.41,
           117.81, 116.34, 114.86, 115.39, 115.92, 112.37, 108.81, 109.08,
           109.35, 108.58, 107.80, 106.30, 104.79, 106.24, 107.69, 106.05,
           104.41, 104.23, 104.05, 102.02, 100.00,  98.17,  96.33,  96.06,
            95.79
        };
        int idx = static_cast<int>((lambda - 380.0) / 10.0);
        if (idx < 0 || idx >= 41) return 0.0;
        double t  = (lambda - (380.0 + idx * 10.0)) / 10.0;
        double lo = table[idx];
        double hi = (idx < 40) ? table[idx + 1] : table[idx];
        return (lo + t * (hi - lo)) / 100.0;
    }

    static double randomDouble() {
        static thread_local std::mt19937_64 rng(std::random_device{}());
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng);
    }
};
