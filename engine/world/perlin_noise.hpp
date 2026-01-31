#pragma once
#include <cmath>
#include <vector>
#include <random>

class PerlinNoise {
private:
    std::vector<int> permutation;
    
    double fade(double t) const {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    double lerp(double t, double a, double b) const {
        return a + t * (b - a);
    }
    
    double grad(int hash, double x, double y) const {
        int h = hash & 7;
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }
    
public:
    PerlinNoise(unsigned int seed = 0) {
        permutation.resize(512);
        
        // Initialize permutation table
        std::vector<int> p(256);
        for (int i = 0; i < 256; ++i) {
            p[i] = i;
        }
        
        // Shuffle using seed
        std::mt19937 rng(seed);
        for (int i = 255; i > 0; --i) {
            int j = rng() % (i + 1);
            std::swap(p[i], p[j]);
        }
        
        // Duplicate for overflow
        for (int i = 0; i < 512; ++i) {
            permutation[i] = p[i % 256];
        }
    }
    
    // 2D Perlin noise
    double noise(double x, double y) const {
        // Find unit grid cell
        int xi = (int)floor(x) & 255;
        int yi = (int)floor(y) & 255;
        
        // Relative position within cell
        double xf = x - floor(x);
        double yf = y - floor(y);
        
        // Fade curves
        double u = fade(xf);
        double v = fade(yf);
        
        // Hash coordinates of 4 corners
        int aa = permutation[permutation[xi] + yi];
        int ab = permutation[permutation[xi] + yi + 1];
        int ba = permutation[permutation[xi + 1] + yi];
        int bb = permutation[permutation[xi + 1] + yi + 1];
        
        // Blend results from 4 corners
        double x1 = lerp(u, grad(aa, xf, yf), grad(ba, xf - 1, yf));
        double x2 = lerp(u, grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1));
        
        return lerp(v, x1, x2);
    }
    
    // Octave noise (fractal Brownian motion)
    double octave_noise(double x, double y, int octaves = 4, double persistence = 0.5) const {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double max_value = 0.0;
        
        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency) * amplitude;
            max_value += amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }
        
        return total / max_value;
    }
    
    // Generate 2D noise map
    std::vector<std::vector<double>> generate_map(int width, int height, 
                                                   double scale = 0.1, 
                                                   int octaves = 4, 
                                                   double persistence = 0.5) const {
        std::vector<std::vector<double>> map(height, std::vector<double>(width));
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                map[y][x] = octave_noise(x * scale, y * scale, octaves, persistence);
            }
        }
        
        return map;
    }
    
    // Normalized map (0 to 1 range)
    std::vector<std::vector<double>> generate_normalized_map(int width, int height,
                                                              double scale = 0.1,
                                                              int octaves = 4,
                                                              double persistence = 0.5) const {
        auto map = generate_map(width, height, scale, octaves, persistence);
        
        // Find min and max
        double min_val = map[0][0];
        double max_val = map[0][0];
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (map[y][x] < min_val) min_val = map[y][x];
                if (map[y][x] > max_val) max_val = map[y][x];
            }
        }
        
        // Normalize
        double range = max_val - min_val;
        if (range > 0) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    map[y][x] = (map[y][x] - min_val) / range;
                }
            }
        }
        
        return map;
    }
};
