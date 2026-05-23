#pragma once

#include <cmath>

#include "math_objects.h"
#include "ray.h"

// Camera Parent Class
class Camera {
public:

    virtual ~Camera() = default;

    virtual void BuildBasis() = 0;
};

// Pinhole Camera
class Pinhole : public Camera {
public:
    int width;            // Pixel width, e.i. 1920
    int height;           // Pixel height, e.i. 1080
    Math::Vec3 position;  // Position of the pinhole
    Math::Vec3 target;    // Camera target
    Math::Vec3 worldUp;   // This is what is considered 'up' in the scene
    double fov;           // Field of view

    // Default constructor
    Pinhole()
        : width(800),
          height(600),
          position(Math::Vec3(0, -10, 2)),
          target(Math::Vec3(0, 0, 0)),
          worldUp(Math::Vec3(0, 0, 1)),
          fov(Math::Vec3::DegToRad(35.0))
    {
        Initialize();
    }

    // Primary constructor
    Pinhole(int w, int h,
            Math::Vec3 p,
            Math::Vec3 t,
            Math::Vec3 u,
            double FOV)
        : width(w),
          height(h),
          position(p),
          target(t),
          worldUp(u),
          fov(Math::Vec3::DegToRad(FOV))
    {
        Initialize();
    }

    // Generate rays one at a time
    Ray GenerateRay(int i, int j)
    {
        // Convert pixel coordinates to normal coordinates
        double u = (i + 0.5) / width;
        double v = (j + 0.5) / height;

        // Map to [-1, 1]
        double x = 2.0 * u - 1.0;
        double y = 1.0 - 2.0 * v;

        // Coordinates of the center of the image plane
        Math::Vec3 imageCenter = position - cameraForward;

        // Pixel position as an offset from the image center
        Math::Vec3 pixelPos = imageCenter +           // Image plane center
                              x * hW * cameraRight +  // x-offset
                              y * hH * cameraUp;      // y-offset

        // Ray direction
        Math::Vec3 rayDir = (position - pixelPos).Normalize();

        return Ray(pixelPos, rayDir);
    }


private:
    // Initialize the camera
    void Initialize() {
        BuildBasis();

        hH = std::tan(fov * 0.5);
        hW = AspectRatio() * hH;
    }

    // Build basis vectors for the camera
    void BuildBasis() override {
        // Creates an orthonormal basis for the camera
        cameraForward = (target - position).Normalize();                       // Forward direction
        cameraRight = Math::Vec3::Cross(cameraForward, worldUp).Normalize();   // Right-hand direction
        cameraUp = Math::Vec3::Cross(cameraRight, cameraForward).Normalize();  // Upward direction
    }

    // Fetch the aspect
    double AspectRatio() const
    {
        return static_cast<double>(width) / height;
    }

    // Store basis vectors
    Math::Vec3 cameraForward;
    Math::Vec3 cameraRight;
    Math::Vec3 cameraUp;

    // Parameters for ray generation
    double hH;  // Half the height of the sensor
    double hW;  // Half the width of the sensor
};
