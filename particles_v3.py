import pygame
import math

# Initialize pygame
pygame.init()

# Scale factor for the window size
SCALE = 40

# Window dimensions
GRID_WIDTH, GRID_HEIGHT = 16, 8
CELL_SIZE = SCALE
WINDOW_WIDTH, WINDOW_HEIGHT = GRID_WIDTH * SCALE, GRID_HEIGHT * SCALE
VER_LIMIT = WINDOW_HEIGHT - CELL_SIZE/2
HOR_LIMIT = WINDOW_WIDTH - CELL_SIZE/2

# Set up the display
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Fluid Simulation")

# Define colors
BLACK = (0, 0, 0)
BLUE = (2, 204, 254)
WHITE = (255, 255, 255)

# Simulation settings
FRAME_RATE = 60
NUM_PARTICLES = 216//2
PARTICLE_SPACING = CELL_SIZE
SPHERE_OF_INFLUENCE = CELL_SIZE * 4
FORCE_CONSTANT = 10
gravity = 10.0
collisionDamping = 0.7
deltaTime = (1*SCALE)/FRAME_RATE

# Particle settings
MASS = 1
positions = []
velocities = []

# Render Update Function
def update(screen):
    for i in range(len(positions)):

        velocities[i].y += gravity * deltaTime

        for j in range(len(positions)):
            if j < 0 or j >= len(positions):
                continue

            if i == j:
                continue

            distance = positions[i].distance_to(positions[j])

            if CELL_SIZE < distance < SPHERE_OF_INFLUENCE:
                direction = positions[j] - positions[i]
                direction = direction.normalize()
                force = FORCE_CONSTANT*direction / distance*distance

                velocities[i] += force * deltaTime
                velocities[j] -= force * deltaTime

            elif 0 < distance <= CELL_SIZE:
                positions[i] = positions[i] + (positions[i] - positions[j]).normalize() * (CELL_SIZE - distance)
                # velocities[i] = velocities[i] - velocities[j] * collisionDamping

        positions[i].x += velocities[i].x * deltaTime
        positions[i].y += velocities[i].y * deltaTime

        resolve_collisions(positions[i], velocities[i])

        velocities[i].x *= 0.99
        velocities[i].y *= 0.99

        # Draw the particle
        pygame.draw.circle(screen, BLUE, positions[i], CELL_SIZE/2)

def resolve_collisions(position, velocity):
    if position.y > VER_LIMIT:
        position.y = VER_LIMIT
        velocity.y *= -1 * collisionDamping
    elif position.y < CELL_SIZE/2:
        position.y = CELL_SIZE/2
        velocity.y *= -1 * collisionDamping

    if position.x > HOR_LIMIT:
        position.x = HOR_LIMIT
        velocity.x *= -1
    elif position.x < CELL_SIZE/2:
        position.x = CELL_SIZE/2
        velocity.x *= -1

def start():
    for i in range(1, NUM_PARTICLES):
        if NUM_PARTICLES % i == 0 and i <= NUM_PARTICLES/i:
            num_rows = i
            num_cols = NUM_PARTICLES // i

    start_x = (WINDOW_WIDTH - num_cols * CELL_SIZE) // 2
    start_y = (WINDOW_HEIGHT - num_rows * CELL_SIZE) // 2

    for row in range(num_rows):
        for col in range(num_cols):
            x = start_x + col * CELL_SIZE
            y = start_y + row * CELL_SIZE
            positions.append(pygame.math.Vector2(x, y))
            velocities.append(pygame.math.Vector2(0, 0))

    pygame.time.wait(400)

if __name__ == "__main__":
    running = True
    clock = pygame.time.Clock()

    start()

    while running:
        # Handle events
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        # Clear the screen
        screen.fill(BLACK)

        # Update the render
        update(screen)

        # Update the display
        pygame.display.flip()

        # Cap the frame rate
        clock.tick(FRAME_RATE)

    # Quit pygame
    pygame.quit()