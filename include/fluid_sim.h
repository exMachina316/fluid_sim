#ifndef FLUID_SIM_H
#define FLUID_SIM_H

#include <vector>
#include <cmath>

class FluidSim {
public:
    struct Vector2 {
        float x, y;
        
        Vector2() : x(0.0f), y(0.0f) {}
        Vector2(float x_, float y_) : x(x_), y(y_) {}
        
        Vector2 operator+(const Vector2& other) const {
            return Vector2(x + other.x, y + other.y);
        }
        
        Vector2 operator-(const Vector2& other) const {
            return Vector2(x - other.x, y - other.y);
        }
        
        Vector2 operator*(float scalar) const {
            return Vector2(x * scalar, y * scalar);
        }
        
        Vector2& operator+=(const Vector2& other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        
        Vector2& operator-=(const Vector2& other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        
        Vector2& operator/=(float scalar) {
            if (scalar != 0.0f) {
                x /= scalar;
                y /= scalar;
            }
            return *this;
        }
    };
    
    // Calculate length of vector
    static float length(const Vector2& v) {
        return sqrtf(v.x * v.x + v.y * v.y);
    }
    
    // Calculate dot product
    static float dot(const Vector2& a, const Vector2& b) {
        return a.x * b.x + a.y * b.y;
    }

    struct Particle {
        Vector2 position;
        Vector2 velocity;
    };

    FluidSim(int numParticles);
    void step(float dt);
    void setGravity(const Vector2& gravityVector);
    const std::vector<Particle>& getParticles() const;

private:
    std::vector<Particle> particles;
    Vector2 gravity;
    const float particleRadius = 0.01f;
    const float restitution = 0.8f;
    
    void handleCollisions(float dt);
    void resolveCollision(size_t i, size_t j);
};

#endif // FLUID_SIM_H