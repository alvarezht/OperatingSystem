#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdlib.h>
#include <stdio.h>

#include "parking_lot.h"

static Font load_readable_font(void) {
    const char *candidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf"
    };

    for (int i = 0; i < (int)(sizeof(candidates) / sizeof(candidates[0])); i++) {
        if (FileExists(candidates[i])) {
            Font f = LoadFontEx(candidates[i], 26, NULL, 0);
            if (f.texture.id > 0) {
                SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
                return f;
            }
        }
    }

    return GetFontDefault();
}

static void draw_text_ui(Font font, const char *text, int x, int y, int size, Color color) {
    DrawTextEx(font, text, (Vector2){(float)x, (float)y}, (float)size, 1.0f, color);
}

static Color color_for_car(int id) {
    static const Color palette[] = {
        {230, 57, 70, 255},
        {29, 53, 87, 255},
        {69, 123, 157, 255},
        {42, 157, 143, 255},
        {244, 162, 97, 255},
        {233, 196, 106, 255},
        {118, 200, 147, 255},
        {88, 129, 87, 255},
        {131, 56, 236, 255},
        {58, 134, 255, 255}
    };
    int count = (int)(sizeof(palette) / sizeof(palette[0]));
    return palette[id % count];
}

static const char *state_label(int state) {
    if (state == CAR_WAITING) {
        return "WAIT";
    }
    if (state == CAR_PARKED) {
        return "PARK";
    }
    if (state == CAR_DONE) {
        return "DONE";
    }
    return "NEW";
}

int main(void) {
    InitWindow(980, 620, "Parking Lot - GUI");
    SetTargetFPS(60);

    Font uiFont = load_readable_font();
    GuiSetFont(uiFont);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt((Color){20, 24, 30, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt((Color){20, 24, 30, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt((Color){20, 24, 30, 255}));

    Color textPrimary = (Color){22, 26, 32, 255};
    Color textSecondary = (Color){70, 78, 90, 255};

    int capacity = 3;
    int totalCars = 10;
    char capacityInput[8] = "3";
    char totalCarsInput[8] = "10";
    bool editCapacity = false;
    bool editTotalCars = false;
    char errorText[128] = "";
    ParkingSnapshot snapshot = {0};

    snapshot.capacity = capacity;
    snapshot.total_cars = totalCars;

    while (!WindowShouldClose()) {
        int running = parking_is_running();

        if (running || snapshot.simulation_done) {
            parking_get_snapshot(&snapshot);
        }

        if (running && snapshot.simulation_done) {
            parking_wait_until_done();
            running = 0;
            parking_get_snapshot(&snapshot);
        }

        BeginDrawing();
        ClearBackground((Color){245, 246, 250, 255});

        DrawRectangle(0, 0, 980, 72, (Color){39, 58, 96, 255});
        draw_text_ui(uiFont, "Parking Lot", 24, 18, 30, RAYWHITE);

        GuiGroupBox((Rectangle){20, 90, 300, 190}, "Control");
        draw_text_ui(uiFont, "Capacidad (N)", 34, 100, 20, textPrimary);
        if (GuiTextBox((Rectangle){34, 126, 120, 28}, capacityInput, 7, !running && editCapacity)) {
            editCapacity = !editCapacity;
        }

        draw_text_ui(uiFont, "Total de autos", 34, 160, 20, textPrimary);
        if (GuiTextBox((Rectangle){34, 186, 120, 28}, totalCarsInput, 7, !running && editTotalCars)) {
            editTotalCars = !editTotalCars;
        }
        draw_text_ui(uiFont, "Rangos: N=1..10, autos=1..30", 34, 218, 16, textSecondary);

        if (GuiButton((Rectangle){34, 244, 120, 32}, "Iniciar")) {
            if (!running) {
                int parsedCapacity = atoi(capacityInput);
                int parsedTotalCars = atoi(totalCarsInput);

                if (parsedCapacity < 1 || parsedCapacity > 10 || parsedTotalCars < 1 || parsedTotalCars > 30) {
                    snprintf(errorText, sizeof(errorText), "Valores invalidos. Use N=1..10 y autos=1..30.");
                } else {
                    capacity = parsedCapacity;
                    totalCars = parsedTotalCars;

                    if (parking_start(capacity, totalCars) != 0) {
                        snprintf(errorText, sizeof(errorText), "No se pudo iniciar la simulacion.");
                    } else {
                        errorText[0] = '\0';
                        snapshot.simulation_done = 0;
                    }
                }
            } else {
                snprintf(errorText, sizeof(errorText), "La simulacion ya esta en ejecucion.");
            }
        }

        if (errorText[0] != '\0') {
            draw_text_ui(uiFont, errorText, 34, 294, 18, MAROON);
        }

        GuiGroupBox((Rectangle){340, 90, 620, 190}, "Estado de la simulacion");
        draw_text_ui(uiFont, TextFormat("Ejecucion: %s", running ? "SI" : "NO"), 360, 124, 22, running ? DARKGREEN : textSecondary);
        draw_text_ui(uiFont, TextFormat("Capacidad: %d", snapshot.capacity), 360, 154, 22, textPrimary);
        draw_text_ui(uiFont, TextFormat("Autos: %d", snapshot.total_cars), 520, 154, 22, textPrimary);
        draw_text_ui(uiFont, TextFormat("Llegaron: %d", snapshot.arrived), 360, 184, 22, textPrimary);
        draw_text_ui(uiFont, TextFormat("Esperando: %d", snapshot.waiting), 520, 184, 22, textPrimary);
        draw_text_ui(uiFont, TextFormat("Estacionados: %d", snapshot.parked), 700, 184, 22, textPrimary);
        draw_text_ui(uiFont, TextFormat("Completados: %d", snapshot.completed), 360, 214, 22, textPrimary);
        draw_text_ui(uiFont, TextFormat("Promedio espera: %.2f s", snapshot.average_wait), 560, 214, 22, textPrimary);

        GuiGroupBox((Rectangle){20, 300, 940, 300}, "Visualizacion de carritos y slots");

        {
            float progress = 0.0f;
            if (snapshot.total_cars > 0) {
                progress = (float)snapshot.completed / (float)snapshot.total_cars;
            }
            DrawRectangle(40, 334, 900, 24, (Color){220, 224, 230, 255});
            DrawRectangle(40, 334, (int)(900.0f * progress), 24, (Color){67, 170, 139, 255});
            DrawRectangleLines(40, 334, 900, 24, (Color){120, 130, 145, 255});
            draw_text_ui(uiFont, TextFormat("%d/%d completados", snapshot.completed, snapshot.total_cars), 44, 338, 18, textPrimary);
        }

        draw_text_ui(uiFont, "Carritos en espera", 40, 372, 22, textPrimary);
        {
            int waitDrawn = 0;
            for (int i = 0; i < snapshot.car_count; i++) {
                int state = snapshot.car_states[i];
                if (state != CAR_WAITING) {
                    continue;
                }

                {
                    int col = waitDrawn % 14;
                    int row = waitDrawn / 14;
                    int x = 40 + (col * 64);
                    int y = 398 + (row * 26);
                    Color carColor = color_for_car(i);

                    DrawRectangle(x, y, 56, 18, carColor);
                    DrawRectangleLines(x, y, 56, 18, (Color){50, 55, 65, 255});
                    draw_text_ui(uiFont, TextFormat("C%d", i), x + 5, y + 3, 12, RAYWHITE);
                }
                waitDrawn++;
            }

            if (waitDrawn == 0) {
                draw_text_ui(uiFont, "Sin carritos en espera", 40, 398, 18, textSecondary);
            }
        }

        draw_text_ui(uiFont, "Parking slots", 40, 460, 22, textPrimary);
        for (int i = 0; i < snapshot.slot_count; i++) {
            int x = 40 + (i * 90);
            int y = 486;
            int owner = snapshot.slot_owners[i];
            Color slotColor = (owner >= 0) ? color_for_car(owner) : (Color){150, 150, 150, 255};

            DrawRectangle(x, y, 72, 36, slotColor);
            DrawRectangleLines(x, y, 72, 36, (Color){60, 60, 60, 255});
            draw_text_ui(uiFont, TextFormat("S%d", i), x + 6, y + 9, 16, RAYWHITE);
            if (owner >= 0) {
                draw_text_ui(uiFont, TextFormat("C%d", owner), x + 38, y + 10, 14, RAYWHITE);
            }
        }

        draw_text_ui(uiFont, "Carritos completados", 40, 536, 22, textPrimary);
        {
            int doneDrawn = 0;
            for (int i = 0; i < snapshot.car_count; i++) {
                int state = snapshot.car_states[i];
                if (state != CAR_DONE) {
                    continue;
                }

                {
                    int col = doneDrawn % 14;
                    int row = doneDrawn / 14;
                    int x = 40 + (col * 64);
                    int y = 562 + (row * 26);
                    Color carColor = color_for_car(i);

                    DrawRectangle(x, y, 56, 18, carColor);
                    DrawRectangleLines(x, y, 56, 18, (Color){50, 55, 65, 255});
                    draw_text_ui(uiFont, TextFormat("C%d", i), x + 5, y + 3, 12, RAYWHITE);
                }
                doneDrawn++;
            }

            if (doneDrawn == 0) {
                draw_text_ui(uiFont, "Aun no hay carritos completados", 40, 562, 18, textSecondary);
            }
        }

        EndDrawing();
    }

    parking_shutdown();
    if (uiFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(uiFont);
    }
    CloseWindow();
    return 0;
}