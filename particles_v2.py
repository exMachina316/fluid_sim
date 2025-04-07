'''
Display Configuration
8
10
12
14
16
16
16
16
16
16
16
16
14
12
10
8

Total 216 LEDs/Pixels
'''

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
gravity = 10.0
collisionDamping = 0.7
deltaTime = 1/FRAME_RATE

# Particle settings
MASS = 1
positions = []
velocities = []

# Render Update Function
def update(screen):
    # # Grid lines
    # for row in range(SCALE, WINDOW_HEIGHT, SCALE):
    #     pygame.draw.line(screen, WHITE, (0, row), (WINDOW_WIDTH, row), 1)

    # for col in range(SCALE, WINDOW_WIDTH, SCALE):
    #     pygame.draw.line(screen, WHITE, (col, 0), (col, WINDOW_HEIGHT), 1)

    pygame.draw.circle(screen, WHITE, (WINDOW_WIDTH//2, WINDOW_HEIGHT//2), SPHERE_OF_INFLUENCE)

    for i in range(len(positions)):

        velocities[i].y += gravity * deltaTime

        positions[i].x += velocities[i].x * deltaTime
        positions[i].y += velocities[i].y * deltaTime
        resolve_collisions(positions[i], velocities[i])

        # Draw the particle
        # pygame.draw.rect(screen, WHITE, (position[0], position[1], CELL_SIZE, CELL_SIZE))
        pygame.draw.circle(screen, BLUE, positions[i], CELL_SIZE/2)
    
    density = calc_density(pygame.math.Vector2(WINDOW_WIDTH//2, WINDOW_HEIGHT//2))
    print(density)

    # render text
    label = myfont.render(f"{density}", 1, (255,255,0))
    screen.blit(label, (10, 10))

def smoothening_kernel(r, dst):
    val = max(0, r*r - dst*dst)
    volume = (math.pi * r**8)/4
    return (val*val*val) / volume

def calc_density(sample_pos: pygame.math.Vector2):
    density = 0

    for i in range(len(positions)):
        dist = (sample_pos - positions[i]).magnitude()
        influence = smoothening_kernel(SPHERE_OF_INFLUENCE, dist)
        density += MASS * influence
    
    return (density*1000000)

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

    myfont = pygame.font.SysFont("monospace", 15)

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


# Particle settings
MASS = 1
positions = []
velocities = []

# Render Update Function
def update(screen):
    # # Grid lines
    # for row in range(SCALE, WINDOW_HEIGHT, SCALE):
    #     pygame.draw.line(screen, WHITE, (0, row), (WINDOW_WIDTH, row), 1)

    # for col in range(SCALE, WINDOW_WIDTH, SCALE):
    #     pygame.draw.line(screen, WHITE, (col, 0), (col, WINDOW_HEIGHT), 1)

    pygame.draw.circle(screen, WHITE, (WINDOW_WIDTH//2, WINDOW_HEIGHT//2), SPHERE_OF_INFLUENCE)

    for i in range(len(positions)):

        velocities[i].y += gravity * deltaTime

        positions[i].x += velocities[i].x * deltaTime
        positions[i].y += velocities[i].y * deltaTime
        resolve_collisions(positions[i], velocities[i])

        # Draw the particle
        # pygame.draw.rect(screen, WHITE, (position[0], position[1], CELL_SIZE, CELL_SIZE))
        pygame.draw.circle(screen, BLUE, positions[i], CELL_SIZE/2)
    
    density = calc_density(pygame.math.Vector2(WINDOW_WIDTH//2, WINDOW_HEIGHT//2))
    print(density)

    # render text
    label = myfont.render(f"{density}", 1, (255,255,0))
    screen.blit(label, (10, 10))

def smoothening_kernel(r, dst):
    val = max(0, r*r - dst*dst)
    volume = (math.pi * r**8)/4
    return (val*val*val) / volume

def calc_density(sample_pos: pygame.math.Vector2):
    density = 0

    for i in range(len(positions)):
        dist = (sample_pos - positions[i]).magnitude()
        influence = smoothening_kernel(SPHERE_OF_INFLUENCE, dist)
        density += MASS * influence
    
    return (density*1000000)

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

    myfont = pygame.font.SysFont("monospace", 15)

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

