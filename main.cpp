#include <vector>
#include <memory>
#include "camera.h"
#include "math_objects.h"
#include "scene.h"
#include "object.h"
#include "material.h"
#include "spd.h"

int main() {
    // ===================================================== Scene =====================================================
    // Materials using measured Macbeth SPDs
    Lambertian groundMat(&SPD::Gray());        // Ideal matte grey
    Lambertian redMat(&SPD::Red());            // Ideal matte red
    Lambertian blueMat(&SPD::Blue());          // Ideal matte blue
    Lambertian greenMat(&SPD::Green());        // Ideal matte green
    Metallic   metalMat(&SPD::White(), 0.05);  // Polished silver
    Dielectric glassMat(1.5220, 0.00459);      // Crown glass (1.5220, 0.00459)

    std::vector<std::unique_ptr<Object>> objects;
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 0.00,  0.0,  0.5),    0.5,   &redMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3(-1.50,  0.0,  0.5),    0.5,   &greenMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 1.50,  0.0,  0.5),    0.5,   &blueMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3(-0.75, -1.0,  0.5),    0.5,   &glassMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 0.75,  1.0,  0.5),    0.5,   &metalMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 0.00,  0.0, -1000.0), 1000.0, &groundMat));

    // ==================================================== Camera =====================================================


    // Image dimensions
    int width  = 2160;
    int height = 1440;

    // Camera attributes
    Math::Vec3     position(0, -4,  1);
    Math::Vec3       target(0,  0,  0);
    Math::Vec3 up_direction(0,  0,  1);

    // Field-of-view
    double fov = 40.0;

    // Number of samples per pixel (250 for quick render, 10,000+ for production-quality)
    int samples = 4'000;

    int ray_depth = 20;

    // Camera object construction
    Pinhole cam(width,
                height,
                position,
                target,
                up_direction,
                fov,
                samples);

    Scene scene(cam, std::move(objects));
    scene.Render("render.pfm", ray_depth);
    return 0;
}
