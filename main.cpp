#include <vector>

#include "camera.h"
#include "color.h"
#include "math_objects.h"

int main() {
    // Image storage
    std::vector<Color> image;

    // Camera parameters
    int width = 1920;
    int height = 1080;

    Math::Vec3 position(10, 10, 10);  // Camera position
    Math::Vec3 target(0, 0, 0);        // Target
    Math::Vec3 worldUp(0, 0, 1);       // Up is +z

    double FOV = 60;  // Horizontal FOV, in degrees

    image.resize(width * height);

    Pinhole pinhole = Pinhole(width, height, position, target, worldUp, FOV);

    for (int j = height - 1; j >= 0; j--) {
        for (int i = width - 1; i >= 0; i--) {
            Ray currentRay = pinhole.GenerateRay(i, j);
            image[i + width * j] = Color(
                0.5 * (currentRay.dir.X + 1),
                0.5 * (currentRay.dir.Y + 1),
                0.5 * (currentRay.dir.Z + 1)
            );
        }
    }

    Color::OutputPFM("output.pfm", image, width, height);

    return 0;
}
