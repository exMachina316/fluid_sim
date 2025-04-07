import pygame
import random
import math

# Initialize pygame
pygame.init()

# Scale factor for the window size
SCALE = 40

# Window dimensions
GRID_WIDTH, GRID_HEIGHT = 16, 16
CELL_SIZE = SCALE
WINDOW_WIDTH, WINDOW_HEIGHT = GRID_WIDTH * SCALE, GRID_HEIGHT * SCALE

# Set up the display
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Fluid Simulation - Improved")

# Define colors
BLACK = (0, 0, 0)
BLUE = (0, 0, 255)

# Particle settings
NUM_PARTICLES = 300
GRAVITY = 5.0
SPREAD = 0.3  # Strength of the particle repulsion
RADIUS = 10   # Distance for interaction
DAMPING = 0.5  # Reduces particle velocity over time for stability

# Create particles as a list of dictionaries
particles = [
    {"x": random.randint(0, GRID_WIDTH), "y": random.randint(0, GRID_HEIGHT // 2), "vx": 0, "vy": 0}
    for _ in range(NUM_PARTICLES)
]

# Simulation update function
def update_particles(particles):
    for p in particles:
        # Apply gravity
        p["vy"] += GRAVITY

        # Update position
        p["x"] += p["vx"]
        p["y"] += p["vy"]

        # Apply damping
        p["vx"] *= DAMPING
        p["vy"] *= DAMPING

        # Collision with boundaries
        if p["x"] < 0:
            p["x"] = 0
            p["vx"] *= -0.5
        if p["x"] > GRID_WIDTH - 1:
            p["x"] = GRID_WIDTH - 1
            p["vx"] *= -0.5
        if p["y"] < 0:
            p["y"] = 0
            p["vy"] *= -0.5
        if p["y"] > GRID_HEIGHT - 1:
            p["y"] = GRID_HEIGHT - 1
            p["vy"] *= -0.5

    # Particle interaction
    for i, p1 in enumerate(particles):
        for j, p2 in enumerate(particles):
            if i != j:
                dx = p2["x"] - p1["x"]
                dy = p2["y"] - p1["y"]
                dist = math.sqrt(dx**2 + dy**2)
                if dist < RADIUS and dist > 0:
                    # Repel particles to maintain distance
                    force = SPREAD/(RADIUS - dist)**2
                    angle = math.atan2(dy, dx)
                    fx = math.cos(angle) * force
                    fy = math.sin(angle) * force
                    p1["vx"] -= fx
                    p1["vy"] -= fy
                    p2["vx"] += fx
                    p2["vy"] += fy

# Main loop
running = True
clock = pygame.time.Clock()

while running:
    # Handle events
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # Update the simulation
    update_particles(particles)

    # Draw the grid
    screen.fill(BLACK)
    for p in particles:
        pygame.draw.circle(screen, BLUE, (int(p["x"] * CELL_SIZE), int(p["y"] * CELL_SIZE)), CELL_SIZE)

    # Update the display
    pygame.display.flip()

    # Cap the frame rate
    clock.tick(30)

# Quit pygame
pygame.quit()
