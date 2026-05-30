#include <vector>
#include <memory>
#include "camera.h"
#include "scene.h"
#include "object.h"
#include "material.h"
#include "spd.h"

int main() {
    // Materials using measured Macbeth SPDs
    Lambertian groundMat(&SPD::Gray());        // Ideal matte grey
    Lambertian redMat(&SPD::Red());            // Ideal matte red
    Lambertian blueMat(&SPD::Blue());    // Ideal matte blue
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

    Pinhole cam(
        2160, 1440,
        Math::Vec3(0, -4,  1),
        Math::Vec3(0,  0,  0),
        Math::Vec3(0,  0,  1),
        40.0,
        4000            // 16,000 samples needed for production quality, :(
    );

    Scene scene(cam, std::move(objects));
    scene.Render("render.pfm", 20);
    return 0;
}
