import pygame
import math

# Initialize pygame
pygame.init()

# Scale factor for the window size
SCALE = 40

# Render Settings
GRID_WIDTH, GRID_HEIGHT = 11, 11
WINDOW_WIDTH, WINDOW_HEIGHT = GRID_WIDTH * SCALE, GRID_HEIGHT * SCALE
FRAME_RATE = 1
deltaTime = SCALE / FRAME_RATE

# Simulation Settings
NUM_PARTICLES = 1
GRAVITY = 1.0
# Max pressure is 1.0
OVERELAX_FACT = 1.9
NUM_PRESSURE_ITERATIONS = 35
MAX_VELOCITY = 1.0
min_d = 0
max_d = 0

# Color macros
BLACK = (0, 0, 0)
BLUE = (0, 0, 255)
WHITE = (255, 255, 255)

# Set up the display
screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Elurian Fluid Simulation")

grid = [[{"type": 1, "ux": 0, "uy": 0, "density": 0.0, "r0": r, "c0": c} for c in range(GRID_WIDTH)] for r in range(GRID_HEIGHT)]

# Physics functions
def apply_gravity():
    """Apply gravity to all fluid particles"""

    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:
                if grid[r][c]["uy"] < MAX_VELOCITY:
                    grid[r][c]["uy"] += GRAVITY * deltaTime

def apply_projection():
    """Apply pressure projection for incompressibility"""
    for _ in range(NUM_PRESSURE_ITERATIONS):
        for r in range(GRID_HEIGHT):
            for c in range(GRID_WIDTH):
                if grid[r][c]["type"]:
                    d = OVERELAX_FACT * (grid[r][c+1]["ux"] - grid[r][c]["ux"] + grid[r-1][c]["uy"] - grid[r][c]["uy"])
                    s = grid[r][c+1]["type"] + grid[r][c-1]["type"] + grid[r-1][c]["type"] + grid[r+1][c]["type"]
                    if s>0:
                        grid[r][c]["ux"] += d * grid[r][c-1]["type"] / s
                        grid[r][c+1]["ux"] -= d * grid[r][c+1]["type"] / s
                        grid[r][c]["uy"] += d * grid[r+1][c]["type"] / s
                        grid[r-1][c]["uy"] -= d * grid[r-1][c]["type"] / s 

def apply_advection():
    """Apply advection to all fluid particles"""
    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:
                # Find previous location
                y = r - grid[r][c]["uy"]*deltaTime/SCALE
                x = c - grid[r][c]["ux"]*deltaTime/SCALE

                r0 = math.floor(y)
                c0 = math.ceil(x)

                if 0 < r0 < GRID_HEIGHT - 1 and 0 < c0 < GRID_WIDTH - 1:
                    if grid[r0][c0]["type"]:
                        grid[r][c]["r0"] = r0
                        grid[r][c]["c0"] = c0

                        w00 = (c0 - x) * (y - r0)
                        w01 = (c0 - x) * (r0 + 1 - y)
                        w10 = (c0 - 1 + x) * (y - r0)
                        w11 = (c0 - 1 + x) * (r0 + 1 - y)
                        if w11 < 0:
                            print(w11)
                        grid[r0][c0]["density"] = w00 * grid[r0][c0]["density"] + w01 * grid[r0+1][c0]["density"] + w10 * grid[r0][c0+1]["density"] + w11 * grid[r0+1][c0+1]["density"]
                        grid[r0][c0]["ux"] = w00 * grid[r0][c0]["ux"] + w01 * grid[r0+1][c0]["ux"] + w10 * grid[r0][c0+1]["ux"] + w11 * grid[r0+1][c0+1]["ux"]
                        grid[r0][c0]["uy"] = w00 * grid[r0][c0]["uy"] + w01 * grid[r0+1][c0]["uy"] + w10 * grid[r0][c0+1]["uy"] + w11 * grid[r0+1][c0+1]["uy"]

# Rendering funcitons
def update(screen):
    """Update and draw simulation"""

    # Draw background
    screen.fill(BLACK)

    # Apply Gravity
    # apply_gravity()

    # Apply pressure projection
    apply_projection()

    # Apply advection
    apply_advection()

    # Draw grid particles only if they are fluid
    draw_particles(screen)

def get_color(mag, min_mag, max_mag):
    """Generate a cyclical gradient color based on the magnitude."""
    # Normalize magnitude
    try:
        norm_mag = (mag-min_mag) / (max_mag - min_mag)
    except:
        norm_mag = 1

    # # Convert the hue to RGB
    # r, g, b = colorsys.hsv_to_rgb(norm_mag, 0.5, 1.0)

    # Convert RGB values to the range [0, 255]
    r = int(norm_mag * 255)
    g = int(norm_mag * 255)
    b = int(norm_mag * 255)

    return (r, g, b)

def draw_velocity(screen, r, c):
    ux = grid[r][c]["ux"]
    uy = grid[r][c]["uy"]
    pygame.draw.line(screen, WHITE, (c * SCALE + SCALE//2, r * SCALE + SCALE//2),
                        (c * SCALE + SCALE//2 + ux * SCALE, r * SCALE + SCALE//2 + uy * SCALE), 2)

def draw_particles(screen):
    global min_d, max_d
    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:

                # Draw density
                d = grid[r][c]["density"]
                min_d = min(min_d, d)
                max_d = max(max_d, d)
                color = get_color(d, min_d, max_d)
                # pygame.draw.rect(screen, color, (c * SCALE, r * SCALE, SCALE, SCALE))
                pygame.draw.circle(screen, color, (c * SCALE + SCALE//2, r * SCALE + SCALE//2), SCALE//3)

    for r in range(GRID_HEIGHT):
        for c in range(GRID_WIDTH):
            if grid[r][c]["type"]:
                draw_velocity(screen, r, c)
                r0 = grid[r][c]["r0"]
                c0 = grid[r][c]["c0"]
                pygame.draw.line(screen, BLUE, (c * SCALE + SCALE//2, r * SCALE + SCALE//2),
                                        (c0 * SCALE + SCALE//2, r0 * SCALE + SCALE//2), 2)

def initialize():
    for r in range(GRID_HEIGHT):
        grid[r][0]["type"] = 0
        grid[r][GRID_WIDTH-1]["type"] = 0

    for c in range(GRID_WIDTH):
        grid[0][c]["type"] = 0
        grid[GRID_HEIGHT-1][c]["type"] = 0
        grid[GRID_HEIGHT-2][c]["density"] = 1.0
    
    grid[GRID_HEIGHT//2][GRID_WIDTH//2]["type"] = 0

def main():
    running = True
    clock = pygame.time.Clock()

    initialize()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.MOUSEMOTION:
                x, y = pygame.mouse.get_pos()
                c = x // SCALE
                r = y // SCALE
                if 0 < r < GRID_HEIGHT - 1 and 0 < c < GRID_WIDTH - 1:
                    d = grid[r][c]["density"]
                    print(r, c, d)
            if event.type == pygame.MOUSEBUTTONDOWN:
                x, y = pygame.mouse.get_pos()
                c = x // SCALE
                r = y // SCALE
                if 0 < r < GRID_HEIGHT - 1 and 0 < c < GRID_WIDTH - 1:
                    grid[r][c]["density"] = 1.0

        update(screen)
        pygame.display.flip()
        clock.tick(FRAME_RATE)

    pygame.quit()

if __name__ == "__main__":
    main()