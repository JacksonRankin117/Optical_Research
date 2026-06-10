#include <vector>
#include <memory>
#include "camera.h"
#include "optics.h"
#include "scene.h"
#include "object.h"
#include "material.h"
#include "spd.h"

int main() {
    // =================================================== Materials ===================================================
    Lambertian groundMat(&SPD::Gray());
    Lambertian redMat(&SPD::Red());
    Lambertian blueMat(&SPD::Blue());
    Lambertian greenMat(&SPD::Green());
    Metallic   metalMat(&SPD::Silver(), 0.05);
    Dielectric glassMat(1.5220, 0.00459);  // Crown glass

    // ==================================================== Objects ====================================================
    std::vector<std::unique_ptr<Object>> objects;
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 0.00,   0.0,     0.5),    0.5,   &redMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3(-1.50,   0.0,     0.5),    0.5,   &blueMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 1.50,   0.0,     0.5),    0.5,   &greenMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3(-0.75,   1.0,     0.5),    0.5,   &glassMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 0.75,  -1.0,     0.5),    0.5,   &metalMat));
    objects.push_back(std::make_unique<Sphere>(Math::Vec3( 0.00,   0.0, -1000.0), 1000.0,   &groundMat));

    // ==================================================== Camera =====================================================
    // Optic type
    ThinLens optic;  // Ideal thin lens
    Pinhole pinhole(1e-6, 250);

    // Optical properties
    double focal_length = 0.240;             // 240mm focal length, typical for 8x10
    double f_num = 5.6;                      // Stopped down for more DOF
    int samples = 1'000;                       // Caustics need more samples

    // Set the optical properties of the optic
    optic.SetFocalLength(focal_length);
    optic.SetFNumber(f_num);
    optic.Samples = samples;

    // Camera attributes
    Math::Vec3 c_pos(0.0, 5.0, 1.5);       // Camera position
    Math::Vec3 c_target(0.0, 0.0, 0.5);     // Aimed at the red sphere
    Math::Vec3 c_up(0.0, 0.0, 1.0);         // Camera up

    // Back standard object
    BackStandard back(0.2540,  // Film/sensor width
                      0.2032,  // Film/sensor height
                      1600,    // Number of pixels across the width of the image.
                      0.0,     // Rise/fall (meters)
                      0.0,     // Shift (meters)
                      0.0,     // Swing (deg)
                      0.0);    // Tilt (deg)

    // Front standard object
    FrontStandard front(&optic,  // Front standard optic
                        0.0,     // Rise/fall (meters)
                        0.0,     // Shift (meters)
                       35.0,     // Swing (deg)
                        0.0);    // Tilt (deg)

    // Calculate Rail Extension (thin lens)
    double rail = ThinLens::CalcRailExt(focal_length, c_pos, c_target);

    // Camera Object
    Camera cam(back,      // Back standard
               front,     // Front standard
               rail,      // Distance between front and back standard
               c_pos,     // Position of the front standard
               c_target,  // Position of the camera target
               c_up);     // Camera Up-direction

    Scene scene(cam, std::move(objects));
    scene.Render("render.pfm");
    return 0;
}
