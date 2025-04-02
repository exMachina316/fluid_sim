#include "fluid_sim.h"
#include <cstdlib>
#include <unordered_map>

const std::vector<FluidSim::Particle>& FluidSim::getParticles() const {
    return particles;
}

void FluidSim::setGravity(const glm::vec2& gravityVector) {
    gravity = gravityVector;
}

FluidSim::FluidSim(int numParticles) {
    particles.resize(numParticles);
    for (auto& p : particles) {
        p.position = glm::vec2(static_cast<float>(rand() % 100) / 100.0f,
                               static_cast<float>(rand() % 100) / 100.0f);
        p.velocity = glm::vec2(static_cast<float>(rand() % 100) / 100.0f,
                               static_cast<float>(rand() % 100) / 100.0f);
    }
}

void FluidSim::step(float dt) {
    // Apply gravity and update positions
    for (auto& p : particles) {
        p.velocity += gravity * dt;
        p.position += p.velocity * dt;
    }

    // Handle particle-particle collisions
    handleCollisions(dt);

    // Handle boundary conditions
    for (auto& p : particles) {
        if (p.position.x < 0.0f) {
            p.position.x = 0.0f;
            p.velocity.x *= -0.9f;
        }
        if (p.position.x > 1.0f) {
            p.position.x = 1.0f;
            p.velocity.x *= -0.9f;
        }
        if (p.position.y < 0.0f) {
            p.position.y = 0.0f;
            p.velocity.y *= -0.9f;
        }
        if (p.position.y > 1.0f) {
            p.position.y = 1.0f;
            p.velocity.y *= -0.9f;
        }
    }
}

void FluidSim::handleCollisions(float dt) {
    const float cellSize = 2.1f * particleRadius; // Slightly larger than particle diameter

    // Create spatial hash grid (mapping from cell index to particles in that cell)
    std::unordered_map<int, std::vector<size_t>> grid;

    // Hash function to convert 2D position to 1D cell index
    auto hashFunction = [cellSize](const glm::vec2& pos) -> int {
        int x = static_cast<int>(pos.x / cellSize);
        int y = static_cast<int>(pos.y / cellSize);
        // Cantor pairing function for combining two integers into one
        return ((x + y) * (x + y + 1) / 2) + y;
    };

    // Insert particles into the grid
    for (size_t i = 0; i < particles.size(); i++) {
        int cellIndex = hashFunction(particles[i].position);
        grid[cellIndex].push_back(i);
    }

    // Check for collisions only between particles in the same or neighboring cells
    for (const auto& cell : grid) {
        // Get the current cell's particles
        const auto& cellParticles = cell.second;

        // Check collisions within this cell
        for (size_t i = 0; i < cellParticles.size(); i++) {
            size_t p1Index = cellParticles[i];

            // Check against other particles in the same cell
            for (size_t j = i + 1; j < cellParticles.size(); j++) {
                size_t p2Index = cellParticles[j];
                resolveCollision(p1Index, p2Index);
            }

            // Generate neighboring cell indices
            glm::vec2 pos = particles[p1Index].position;
            int x = static_cast<int>(pos.x / cellSize);
            int y = static_cast<int>(pos.y / cellSize);

            // Check all 8 neighboring cells
            for (int nx = x - 1; nx <= x + 1; nx++) {
                for (int ny = y - 1; ny <= y + 1; ny++) {
                    // Skip the current cell (already checked)
                    if (nx == x && ny == y) continue;

                    // Calculate neighbor cell index
                    int neighborCellIndex = ((nx + ny) * (nx + ny + 1) / 2) + ny;

                    // Check if the neighboring cell exists in our grid
                    auto neighborIt = grid.find(neighborCellIndex);
                    if (neighborIt != grid.end()) {
                        // Check against all particles in the neighboring cell
                        for (size_t p2Index : neighborIt->second) {
                            // Avoid duplicate checks (only check if p1 < p2)
                            if (p1Index < p2Index) {
                                resolveCollision(p1Index, p2Index);
                            }
                        }
                    }
                }
            }
        }
    }
}

void FluidSim::resolveCollision(size_t i, size_t j) {
    auto& p1 = particles[i];
    auto& p2 = particles[j];

    // Vector from p1 to p2
    glm::vec2 diff = p2.position - p1.position;
    float distance = glm::length(diff);

    // Check if particles are colliding
    float minDistance = 2.0f * particleRadius;
    if (distance < minDistance) {
        // Normalize the difference vector
        glm::vec2 collisionNormal = diff;
        if (distance > 0.0001f) {  // Avoid division by zero
            collisionNormal /= distance;
        } else {
            // If particles are at the same position, use a random direction
            float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
            collisionNormal = glm::vec2(cos(angle), sin(angle));
        }

        // Move particles apart to prevent overlap
        float overlap = minDistance - distance;
        p1.position -= 0.5f * overlap * collisionNormal;
        p2.position += 0.5f * overlap * collisionNormal;

        // Calculate relative velocity
        glm::vec2 relativeVelocity = p2.velocity - p1.velocity;

        // Calculate impulse
        float velocityAlongNormal = glm::dot(relativeVelocity, collisionNormal);

        // Only resolve if objects are moving toward each other
        if (velocityAlongNormal < 0.0f) {
            // Calculate impulse scalar
            float j = -(1.0f + restitution) * velocityAlongNormal;
            j /= 2.0f;  // Assuming equal mass for all particles

            // Apply impulse
            glm::vec2 impulse = j * collisionNormal;
            p1.velocity -= impulse;
            p2.velocity += impulse;
        }
    }
}