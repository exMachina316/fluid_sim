// fluid_sim.h
#ifndef FLUID_SIM_H
#define FLUID_SIM_H

#include <vector>
#include <glm/glm.hpp>

const float particleRadius = 0.01f;  // Particle radius
const float restitution = 0.5f;      // Coefficient of restitution

class FluidSim {
public:
    glm::vec2 gravity;

    struct Particle {
        glm::vec2 position;
        glm::vec2 velocity;
    };

    FluidSim(int numParticles);
    void step(float dt);
    const std::vector<Particle>& getParticles() const;
    void setGravity(const glm::vec2& gravityVector);

private:
    std::vector<Particle> particles;
    
    void handleCollisions(float dt);
    void resolveCollision(size_t i, size_t j);
};

#endif // FLUID_SIM_H