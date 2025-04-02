#include "fluid_sim.h"
#include <stdlib.h>

struct CellData {
    int cellIndex;
    std::vector<size_t> particleIndices;
    CellData* next;
};

// Define a small fixed-size hash table (power of 2 for optimization)
const int HASH_TABLE_SIZE = 64;
CellData* hashTable[HASH_TABLE_SIZE] = {nullptr};
CellData cellDataPool[256]; // Pre-allocated pool for cell data
int cellDataPoolIndex = 0;

const std::vector<FluidSim::Particle>& FluidSim::getParticles() const {
    return particles;
}

void FluidSim::setGravity(const Vector2& gravityVector) {
    gravity = gravityVector;
}

FluidSim::FluidSim(int numParticles) {
    particles.resize(numParticles);
    for (auto& p : particles) {
        p.position = Vector2(static_cast<float>(rand() % 100) / 100.0f,
                            static_cast<float>(rand() % 100) / 100.0f);
        p.velocity = Vector2(static_cast<float>(rand() % 100) / 100.0f,
                            static_cast<float>(rand() % 100) / 100.0f);
    }
}

void FluidSim::step(float dt) {
    // Apply gravity and update positions
    for (auto& p : particles) {
        p.velocity.x += gravity.x * dt;
        p.velocity.y += gravity.y * dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
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

// Hash function for cell indices
int hashCell(int cellIndex) {
    return cellIndex & (HASH_TABLE_SIZE - 1); // Fast modulo for power of 2
}

// Reset the hash table between simulation steps
void clearHashTable() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        hashTable[i] = nullptr;
    }
    cellDataPoolIndex = 0;
}

// Add a particle to the spatial hash
void addToSpatialHash(int cellIndex, size_t particleIndex) {
    int bucket = hashCell(cellIndex);
    
    // Check if cell already exists
    CellData* current = hashTable[bucket];
    while (current != nullptr) {
        if (current->cellIndex == cellIndex) {
            current->particleIndices.push_back(particleIndex);
            return;
        }
        current = current->next;
    }
    
    // Create new cell data if we have space
    if (cellDataPoolIndex < 256) {
        CellData* newCell = &cellDataPool[cellDataPoolIndex++];
        newCell->cellIndex = cellIndex;
        newCell->particleIndices.clear();
        newCell->particleIndices.push_back(particleIndex);
        newCell->next = hashTable[bucket];
        hashTable[bucket] = newCell;
    }
}

// Find a cell in the hash table
CellData* findCell(int cellIndex) {
    int bucket = hashCell(cellIndex);
    CellData* current = hashTable[bucket];
    
    while (current != nullptr) {
        if (current->cellIndex == cellIndex) {
            return current;
        }
        current = current->next;
    }
    
    return nullptr;
}

void FluidSim::handleCollisions(float dt) {
    const float cellSize = 2.1f * particleRadius; // Slightly larger than particle diameter

    // Clear the hash table
    clearHashTable();

    // Insert particles into the grid
    for (size_t i = 0; i < particles.size(); i++) {
        int x = static_cast<int>(particles[i].position.x / cellSize);
        int y = static_cast<int>(particles[i].position.y / cellSize);
        // Cantor pairing function for combining two integers into one
        int cellIndex = ((x + y) * (x + y + 1) / 2) + y;
        addToSpatialHash(cellIndex, i);
    }

    // Check for collisions
    for (int h = 0; h < HASH_TABLE_SIZE; h++) {
        CellData* cell = hashTable[h];
        
        while (cell != nullptr) {
            const auto& cellParticles = cell->particleIndices;
            
            // Check collisions within this cell
            for (size_t i = 0; i < cellParticles.size(); i++) {
                size_t p1Index = cellParticles[i];

                // Check against other particles in the same cell
                for (size_t j = i + 1; j < cellParticles.size(); j++) {
                    size_t p2Index = cellParticles[j];
                    resolveCollision(p1Index, p2Index);
                }

                // Generate neighboring cell indices
                Vector2 pos = particles[p1Index].position;
                int x = static_cast<int>(pos.x / cellSize);
                int y = static_cast<int>(pos.y / cellSize);

                // Check neighboring cells (more efficient loop unrolling)
                for (int nx = x - 1; nx <= x + 1; nx++) {
                    for (int ny = y - 1; ny <= y + 1; ny++) {
                        // Skip the current cell (already checked)
                        if (nx == x && ny == y) continue;

                        // Calculate neighbor cell index
                        int neighborCellIndex = ((nx + ny) * (nx + ny + 1) / 2) + ny;
                        
                        // Find the neighbor cell
                        CellData* neighborCell = findCell(neighborCellIndex);
                        if (neighborCell != nullptr) {
                            // Check against particles in the neighboring cell
                            for (size_t p2Index : neighborCell->particleIndices) {
                                if (p1Index < p2Index) {
                                    resolveCollision(p1Index, p2Index);
                                }
                            }
                        }
                    }
                }
            }
            
            cell = cell->next;
        }
    }
}

void FluidSim::resolveCollision(size_t i, size_t j) {
    auto& p1 = particles[i];
    auto& p2 = particles[j];

    // Vector from p1 to p2
    Vector2 diff = Vector2(p2.position.x - p1.position.x, p2.position.y - p1.position.y);
    float distance = length(diff);

    // Check if particles are colliding
    float minDistance = 2.0f * particleRadius;
    if (distance < minDistance) {
        // Normalize the difference vector
        Vector2 collisionNormal = diff;
        if (distance > 0.0001f) {  // Avoid division by zero
            collisionNormal.x /= distance;
            collisionNormal.y /= distance;
        } else {
            // If particles are at the same position, use a random direction
            float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
            collisionNormal.x = cosf(angle);
            collisionNormal.y = sinf(angle);
        }

        // Move particles apart to prevent overlap
        float overlap = minDistance - distance;
        p1.position.x -= 0.5f * overlap * collisionNormal.x;
        p1.position.y -= 0.5f * overlap * collisionNormal.y;
        p2.position.x += 0.5f * overlap * collisionNormal.x;
        p2.position.y += 0.5f * overlap * collisionNormal.y;

        // Calculate relative velocity
        Vector2 relativeVelocity = Vector2(p2.velocity.x - p1.velocity.x, p2.velocity.y - p1.velocity.y);

        // Calculate impulse
        float velocityAlongNormal = dot(relativeVelocity, collisionNormal);

        // Only resolve if objects are moving toward each other
        if (velocityAlongNormal < 0.0f) {
            // Calculate impulse scalar
            float j = -(1.0f + restitution) * velocityAlongNormal;
            j /= 2.0f;  // Assuming equal mass for all particles

            // Apply impulse
            Vector2 impulse = Vector2(j * collisionNormal.x, j * collisionNormal.y);
            p1.velocity.x -= impulse.x;
            p1.velocity.y -= impulse.y;
            p2.velocity.x += impulse.x;
            p2.velocity.y += impulse.y;
        }
    }
}