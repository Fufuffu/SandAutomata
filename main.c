#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 1664
#define SCREEN_HEIGHT 936
#define PIXEL_SIZE 4

#define WORKING_WIDTH SCREEN_WIDTH / PIXEL_SIZE
#define WORKING_HEIGHT SCREEN_HEIGHT / PIXEL_SIZE
#define TOTAL_WORKING_PIXELS WORKING_WIDTH *WORKING_HEIGHT

typedef enum {
    BLOCK_AIR,
    BLOCK_SAND,
    BLOCK_WATER,
    BLOCK_WOOD,
    BLOCK_ACID,
    BLOCK_COUNT
} BlockType;

typedef struct CellData {
    BlockType type;
    bool processed;
} CellData;

CellData CellFromType(BlockType type) {
    switch (type) {
    case BLOCK_AIR:
    case BLOCK_SAND:
    case BLOCK_WOOD:
    case BLOCK_WATER:
    case BLOCK_ACID:
        return (CellData){type, false};
    default:
        assert("unreachable");
    }
}

static inline bool IsLiquid(BlockType block) {
    return block == BLOCK_WATER || block == BLOCK_ACID;
}

static inline int TrySlide(int startRow, int col, int dir, CellData *state) {
    int currentRow = startRow;
    for (int steps = 1; steps <= 8; ++steps) {
        currentRow += dir;
        if (currentRow < 0 || currentRow >= WORKING_WIDTH) break;

        int pos = col * WORKING_WIDTH + currentRow;
        if (!IsLiquid(state[pos].type)) {
            if (state[pos].type == BLOCK_AIR) {
                return pos;
            }
            break;
        }
    }
    return -1;
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Window");
    SetTargetFPS(60);
    CellData currState[TOTAL_WORKING_PIXELS] = {0};

    RenderTexture drawTexture = LoadRenderTexture(WORKING_WIDTH, WORKING_HEIGHT);
    Color *pixels = (Color *)malloc(TOTAL_WORKING_PIXELS * sizeof(Color));
    unsigned char selectedBlock = BLOCK_SAND;
    unsigned int frameCounter = 0;

    while (!WindowShouldClose()) {
        frameCounter++;

        if (GetMouseWheelMove() != 0) {
            selectedBlock++;
            selectedBlock = selectedBlock % BLOCK_COUNT;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            int row = floor(mousePos.x) / PIXEL_SIZE;
            int col = floor(mousePos.y) / PIXEL_SIZE;

            int brushSize = 2;

            if (row >= 0 && row < WORKING_WIDTH && col >= 0 && col < WORKING_HEIGHT) {
                for (int i = -brushSize; i < brushSize + 1; i++) {
                    for (int j = -brushSize; j < brushSize + 1; j++) {
                        currState[((col + i) * WORKING_WIDTH) + row + j] = CellFromType(selectedBlock);
                    }
                }
            }
        }

        // Reset all processed flags
        for (int i = 0; i < TOTAL_WORKING_PIXELS; i++)
            currState[i].processed = false;

        for (int col = WORKING_HEIGHT - 1; col >= 0; --col) {
            int rowStart = (frameCounter & 1) ? WORKING_WIDTH - 1 : 0;
            int rowEnd = (frameCounter & 1) ? -1 : WORKING_WIDTH;
            int step = (frameCounter & 1) ? -1 : 1;

            for (int row = rowStart; row != rowEnd; row += step) {
                int pos = col * WORKING_WIDTH + row;
                if (currState[pos].processed) continue;

                BlockType type = currState[pos].type;
                if (type == BLOCK_AIR || type == BLOCK_WOOD) continue;

                int under = (col + 1) < WORKING_HEIGHT ? pos + WORKING_WIDTH : -1;
                int left = row > 0 ? pos - 1 : -1;
                int right = row < WORKING_WIDTH - 1 ? pos + 1 : -1;
                int underLeft = (under != -1 && row > 0) ? under - 1 : -1;
                int underRight = (under != -1 && row < WORKING_WIDTH - 1) ? under + 1 : -1;

                int dest = -1;

                switch (type) {
                case BLOCK_SAND:
                    if (under != -1 && (currState[under].type == BLOCK_AIR || IsLiquid(currState[under].type)))
                        dest = under;
                    else if (underLeft != -1 && (currState[underLeft].type == BLOCK_AIR || IsLiquid(currState[underLeft].type)))
                        dest = underLeft;
                    else if (underRight != -1 && (currState[underRight].type == BLOCK_AIR || IsLiquid(currState[underRight].type)))
                        dest = underRight;
                    break;
                case BLOCK_WATER:
                case BLOCK_ACID:
                    if (under != -1 && currState[under].type == BLOCK_AIR) {
                        dest = under;
                    } else {
                        bool preferLeft = ((row + col + frameCounter) & 1) == 0;
                        if (preferLeft) {
                            if (underLeft != -1 && currState[underLeft].type == BLOCK_AIR)
                                dest = underLeft;
                            else if (underRight != -1 && currState[underRight].type == BLOCK_AIR)
                                dest = underRight;
                        } else {
                            if (underRight != -1 && currState[underRight].type == BLOCK_AIR)
                                dest = underRight;
                            else if (underLeft != -1 && currState[underLeft].type == BLOCK_AIR)
                                dest = underLeft;
                        }

                        if (dest == -1) {
                            int slideDirFirst = preferLeft ? -1 : 1;
                            int slideDirSecond = -slideDirFirst;

                            dest = TrySlide(row, col, slideDirFirst, currState);
                            if (dest == -1)
                                dest = TrySlide(row, col, slideDirSecond, currState);
                        }
                    }
                    break;
                default:
                    break;
                }

                if (dest != -1) {
                    CellData oldDest = currState[dest];

                    currState[dest] = currState[pos];
                    currState[dest].processed = true;

                    currState[pos] = oldDest;
                    currState[pos].processed = true;
                }
                currState[pos].processed = true;
            }
        }

        for (int col = 0; col < WORKING_HEIGHT; col++) {
            for (int row = 0; row < WORKING_WIDTH; row++) {
                int currentPosition = (col * WORKING_WIDTH) + row;

                switch (currState[currentPosition].type) {
                case BLOCK_AIR:
                    pixels[currentPosition] = WHITE;
                    break;
                case BLOCK_SAND:
                    pixels[currentPosition] = YELLOW;
                    break;
                case BLOCK_WATER:
                    pixels[currentPosition] = BLUE;
                    break;
                case BLOCK_WOOD:
                    pixels[currentPosition] = BROWN;
                    break;
                case BLOCK_ACID:
                    pixels[currentPosition] = GREEN;
                    break;
                default:
                    assert("unreachable");
                }
            }
        }
        UpdateTexture(drawTexture.texture, pixels);

        BeginDrawing();
        {
            ClearBackground(BLACK);
            DrawTexturePro(drawTexture.texture, (Rectangle){0, 0, WORKING_WIDTH, WORKING_HEIGHT}, (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (Vector2){0, 0}, 0, WHITE);
            DrawText(TextFormat("Select block: %u", selectedBlock), 10, 10, 10, BLACK);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
