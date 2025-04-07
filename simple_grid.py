import pygame
import math

# Initialize pygame
pygame.init()

# Scale factor for the window size
SCALE = 40
CELL_SIZE = int(SCALE*0.8)

# Window dimensions
GRID_WIDTH, GRID_HEIGHT = 16, 8
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
FRAME_RATE = 1
NUM_PARTICLES = 100
deltaTime = (1*SCALE)/FRAME_RATE

grid = [[0 for _ in range(GRID_WIDTH)] for _ in range(GRID_HEIGHT)]

# Render Update Function
def update(screen):
    apply_forces()
    
    for row in range(GRID_HEIGHT):
        for col in range(GRID_WIDTH):
            if grid[row][col] == 1:
                x = col * SCALE + CELL_SIZE/2
                y = row * SCALE + CELL_SIZE/2
                pygame.draw.circle(screen, BLUE, (x, y), CELL_SIZE/2)

def apply_forces():
    # Gravity
    for row in range(GRID_HEIGHT):
        for col in range(GRID_WIDTH):
            if grid[row][col] == 1:
                neighbour_occupancy = get_neighbour_occupancy(row, col)
                for i in range(8):
                    if neighbour_occupancy[i] == 0:
                        grid[row][col] = 0
                        if row+1 < GRID_HEIGHT:
                            grid[row+1][col] = 1
                        break

def get_neighbour_occupancy(row, col):
    neighbours = [0, 0, 0, 0, 0, 0, 0, 0]
    for i in range(-1, 2):
        for j in range(-1, 2):
            if row+i >= 0 and row+i < GRID_HEIGHT and col+j >= 0 and col+j < GRID_WIDTH:
                neighbours[(i)*3 + j] = grid[row+i][col+j]
    return neighbours

def start():
    for i in range(1, NUM_PARTICLES):
        if NUM_PARTICLES % i == 0 and i <= NUM_PARTICLES/i:
            num_rows = min(i, GRID_HEIGHT)
            num_cols = min(NUM_PARTICLES // i, GRID_WIDTH)

    for row in range(num_rows):
        for col in range(num_cols):
            grid[row][col] = 1

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