#pragma once

#include "math_objects.h"

// Camera Parent Class
class Camera {
public:


    virtual void BuildBasis() {};
};

// Pinhole Camera
class Pinhole : public Camera {
public:
    Math::Vec3 position;  // Position of the pinhole
    Math::Vec3 target;    // Camera target
    Math::Vec3 worldUp;   // This is what is considered 'up' in the scene
    double FOV;           // 

    Pinhole(Math::Vec3 p, Math::Vec3 t, double FOV, ) {
        
    }

    void BuildBasis() override {
        // Creates an orthonormal basis for the camera
        Math::Vec3 cameraForward = (position - target);                      // Forward direction
        Math::Vec3 cameraRight = Math::Vec3::Cross(cameraForward, worldUp);  // Right-hand direction
        Math::Vec3 cameraUp = Math::Vec3::Cross(cameraRight, cameraUp);      // Upward direction

        // Normalize for unit vectors
        cameraForward = cameraForward.Normalize();
        cameraRight   = cameraRight.Normalize();
        cameraUp      = cameraUp.Normalize();
    }

private:

};
