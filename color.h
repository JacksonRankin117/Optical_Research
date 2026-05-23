#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "math_objects.h"

class Color {
public:
    float r, g, b;

    // Default is black
    Color() : r(0.0f), g(0.0f), b(0.0f) {}

    // Arbitrary Color
    Color(float R, float G, float B) : r(R), g(G), b(B) {}

    // Color based on vector
    explicit Color(const Math::Vec3& vec) {
        r = std::clamp(vec.X, 0.0, 1.0);
        g = std::clamp(vec.Y, 0.0, 1.0);
        b = std::clamp(vec.Z, 0.0, 1.0);
    }

    // Ouptut a color file
    static void OutputPFM(const std::string& filename,
                          const std::vector<Color>& image,
                          int width, int height)
    {
        std::ofstream file(filename, std::ios::binary);

        if (!file) {
            std::cerr << "Unable to create file!\n";
            return;
        }

        // Header
        file << "PF\n";
        file << width << " " << height << "\n";
        file << "-1.0\n"; // little-endian

        // PFM stores rows bottom-to-top
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {

                const Color& c = image[y * width + x];

                file.write(reinterpret_cast<const char*>(&c.r), sizeof(float));
                file.write(reinterpret_cast<const char*>(&c.g), sizeof(float));
                file.write(reinterpret_cast<const char*>(&c.b), sizeof(float));
            }
        }

        std::cout << "File saved to " << filename << "\n";
    }

    // Gradient (debug)
    static void GradientPFM(const std::string& filename,
                            int width, int height)
    {
        std::ofstream file(filename, std::ios::binary);

        if (!file) {
            std::cerr << "Unable to create file!\n";
            return;
        }

        // Header (ASCII only)
        file << "PF\n";
        file << width << " " << height << "\n";
        file << "-1.0\n";

        // Bottom-to-top row order
        for (int y = height - 1; y >= 0; --y)
        {
            float ty = (height <= 1) ? 0.0f : float(y) / float(height - 1);

            for (int x = 0; x < width; ++x)
            {
                float tx = (width <= 1) ? 0.0f : float(x) / float(width - 1);

                Color c;
                c.r = tx;        // horizontal gradient
                c.g = ty;        // vertical gradient
                c.b = 0.2f;      // constant debug channel

                file.write(reinterpret_cast<const char*>(&c.r), sizeof(float));
                file.write(reinterpret_cast<const char*>(&c.g), sizeof(float));
                file.write(reinterpret_cast<const char*>(&c.b), sizeof(float));
            }
        }

        std::cout << "File saved to " << filename << "\n";
    }

};
