#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vita2d.h>
#include <psp2/ctrl.h>

#define SCREEN_W 960
#define SCREEN_H 544

// Estados del juego
typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
} GameState;

// Modos de juego
typedef enum {
    MODE_VS_NORMAL,
    MODE_VS_HARD,
    MODE_SURVIVAL
} GameMode;

int main(void) {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(15, 15, 25, 255));

    // Cargar la fuente nativa del sistema de PS Vita
    vita2d_pgf *font = vita2d_load_default_pgf();

    GameState state = STATE_MENU;
    GameMode mode = MODE_VS_NORMAL;
    int menu_cursor = 0; // 0, 1 o 2

    // Dimensiones de paletas
    float paddle_w = 18.0f;
    float paddle_h = 95.0f;
    float player_speed = 7.5f;

    // Paleta 1 (Jugador - Izquierda)
    float p1_x = 35.0f;
    float p1_y = (SCREEN_H / 2.0f) - (paddle_h / 2.0f);

    // Paleta 2 (IA - Derecha)
    float p2_x = SCREEN_W - 35.0f - paddle_w;
    float p2_y = (SCREEN_H / 2.0f) - (paddle_h / 2.0f);
    float ai_speed = 4.8f;

    // Pelota
    float ball_size = 16.0f;
    float ball_x = SCREEN_W / 2.0f;
    float ball_y = SCREEN_H / 2.0f;
    float ball_vx = 5.0f;
    float ball_vy = 3.0f;
    float ball_speed_mult = 1.0f;

    // Marcadores
    int score_p1 = 0;
    int score_p2 = 0;
    int combo_hits = 0;
    const int MAX_SCORE = 5;

    // Control de mandos y detección de botón presionado (evitar repetición rápida)
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    SceCtrlData pad;
    SceCtrlData old_pad;
    old_pad.buttons = 0;

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        uint32_t pressed = pad.buttons & ~old_pad.buttons; // Detecta clic único

        if (pad.buttons & SCE_CTRL_START) {
            break; // Salir del juego
        }

        // ==========================================
        // 1. ESTADO: MENÚ PRINCIPAL
        // ==========================================
        if (state == STATE_MENU) {
            if (pressed & SCE_CTRL_DOWN) {
                menu_cursor = (menu_cursor + 1) % 3;
            }
            if (pressed & SCE_CTRL_UP) {
                menu_cursor = (menu_cursor - 1 + 3) % 3;
            }

            // Seleccionar con botón Cruz (X)
            if (pressed & SCE_CTRL_CROSS) {
                mode = (GameMode)menu_cursor;
                score_p1 = 0;
                score_p2 = 0;
                combo_hits = 0;
                ball_speed_mult = 1.0f;
                p1_y = (SCREEN_H / 2.0f) - (paddle_h / 2.0f);
                p2_y = (SCREEN_H / 2.0f) - (paddle_h / 2.0f);

                if (mode == MODE_VS_NORMAL) {
                    ai_speed = 4.8f;
                    ball_vx = 6.0f;
                } else if (mode == MODE_VS_HARD) {
                    ai_speed = 7.0f;
                    ball_vx = 8.0f;
                } else { // SUPERVIVENCIA
                    ball_vx = 6.0f;
                }
                ball_vy = 3.0f;
                ball_x = SCREEN_W / 2.0f;
                ball_y = SCREEN_H / 2.0f;

                state = STATE_PLAYING;
            }
        }
        // ==========================================
        // 2. ESTADO: JUGANDO
        // ==========================================
        else if (state == STATE_PLAYING) {
            // Movimiento Jugador (Cruceta o Stick Izquierdo)
            if (pad.buttons & SCE_CTRL_UP || pad.ly < 80)    p1_y -= player_speed;
            if (pad.buttons & SCE_CTRL_DOWN || pad.ly > 175)  p1_y += player_speed;

            if (p1_y < 0) p1_y = 0;
            if (p1_y + paddle_h > SCREEN_H) p1_y = SCREEN_H - paddle_h;

            // Movimiento IA (en modos de duelo)
            if (mode != MODE_SURVIVAL) {
                float target_y = ball_y - (paddle_h / 2.0f);
                // Si la pelota va hacia la IA, se mueve a interceptarla
                if (ball_vx > 0) {
                    if (p2_y < target_y - 10) p2_y += ai_speed;
                    else if (p2_y > target_y + 10) p2_y -= ai_speed;
                }
                if (p2_y < 0) p2_y = 0;
                if (p2_y + paddle_h > SCREEN_H) p2_y = SCREEN_H - paddle_h;
            }

            // Movimiento de la Pelota
            ball_x += ball_vx * ball_speed_mult;
            ball_y += ball_vy * ball_speed_mult;

            // Rebote techo y piso
            if (ball_y <= 0) {
                ball_y = 0;
                ball_vy = -ball_vy;
            }
            if (ball_y + ball_size >= SCREEN_H) {
                ball_y = SCREEN_H - ball_size;
                ball_vy = -ball_vy;
            }

            // Colisión con Paleta Jugador (P1)
            if (ball_x <= (p1_x + paddle_w) && (ball_x + ball_size) >= p1_x &&
                (ball_y + ball_size) >= p1_y && ball_y <= (p1_y + paddle_h) && ball_vx < 0) 
            {
                ball_vx = -ball_vx;
                ball_x = p1_x + paddle_w + 1;
                combo_hits++;

                // Rebote angular según el punto de golpe
                float hit_offset = (ball_y + (ball_size / 2.0f)) - (p1_y + (paddle_h / 2.0f));
                ball_vy = hit_offset * 0.12f;

                // Aumentar velocidad progresiva (máximo x2.4)
                if (ball_speed_mult < 2.4f) ball_speed_mult += 0.06f;

                if (mode == MODE_SURVIVAL) score_p1++;
            }

            // En modo Supervivencia, la pared derecha rebota la pelota
            if (mode == MODE_SURVIVAL) {
                if (ball_x + ball_size >= SCREEN_W) {
                    ball_x = SCREEN_W - ball_size - 1;
                    ball_vx = -ball_vx;
                }
            } 
            // En modos vs IA, colisión con Paleta IA (P2)
            else {
                if (ball_x + ball_size >= p2_x && ball_x <= (p2_x + paddle_w) &&
                    (ball_y + ball_size) >= p2_y && ball_y <= (p2_y + paddle_h) && ball_vx > 0) 
                {
                    ball_vx = -ball_vx;
                    ball_x = p2_x - ball_size - 1;
                    combo_hits++;

                    float hit_offset = (ball_y + (ball_size / 2.0f)) - (p2_y + (paddle_h / 2.0f));
                    ball_vy = hit_offset * 0.12f;

                    if (ball_speed_mult < 2.4f) ball_speed_mult += 0.05f;
                }
            }

            // Puntuación / Punto perdido
            // Pelota sale por la izquierda
            if (ball_x < 0) {
                if (mode == MODE_SURVIVAL) {
                    state = STATE_GAMEOVER;
                } else {
                    score_p2++;
                    ball_speed_mult = 1.0f;
                    combo_hits = 0;
                    ball_x = SCREEN_W / 2.0f;
                    ball_y = SCREEN_H / 2.0f;
                    ball_vx = (mode == MODE_VS_HARD) ? 7.0f : 5.0f;
                    ball_vy = (rand() % 2 == 0) ? 3.0f : -3.0f;

                    if (score_p2 >= MAX_SCORE) state = STATE_GAMEOVER;
                }
            }

            // Pelota sale por la derecha (Punto para P1)
            if (mode != MODE_SURVIVAL && ball_x > SCREEN_W) {
                score_p1++;
                ball_speed_mult = 1.0f;
                combo_hits = 0;
                ball_x = SCREEN_W / 2.0f;
                ball_y = SCREEN_H / 2.0f;
                ball_vx = (mode == MODE_VS_HARD) ? -7.0f : -5.0f;
                ball_vy = (rand() % 2 == 0) ? 3.0f : -3.0f;

                if (score_p1 >= MAX_SCORE) state = STATE_GAMEOVER;
            }
        }
        // ==========================================
        // 3. ESTADO: FIN DEL JUEGO (GAME OVER)
        // ==========================================
        else if (state == STATE_GAMEOVER) {
            // Volver al menú presionando Cruz (X) o Círculo (O)
            if (pressed & SCE_CTRL_CROSS || pressed & SCE_CTRL_CIRCLE) {
                state = STATE_MENU;
            }
        }

        old_pad = pad;

        // ==========================================
        // RENDERIZADO VISUAL
        // ==========================================
        vita2d_start_drawing();
        vita2d_clear_screen();

        if (state == STATE_MENU) {
            if (font) {
                vita2d_pgf_draw_text(font, 360, 120, RGBA8(255, 215, 0, 255), 1.8f, "VITA PONG");
                vita2d_pgf_draw_text(font, 320, 160, RGBA8(180, 180, 200, 255), 1.0f, "Selecciona Modo de Juego:");

                // Opciones del menú
                unsigned int c0 = (menu_cursor == 0) ? RGBA8(50, 255, 100, 255) : RGBA8(140, 140, 140, 255);
                unsigned int c1 = (menu_cursor == 1) ? RGBA8(255, 80, 80, 255) : RGBA8(140, 140, 140, 255);
                unsigned int c2 = (menu_cursor == 2) ? RGBA8(80, 200, 255, 255) : RGBA8(140, 140, 140, 255);

                vita2d_pgf_draw_text(font, 360, 250, c0, 1.3f, (menu_cursor == 0) ? "> 1. VS IA (Normal)" : "  1. VS IA (Normal)");
                vita2d_pgf_draw_text(font, 360, 310, c1, 1.3f, (menu_cursor == 1) ? "> 2. VS IA (Extremo)" : "  2. VS IA (Extremo)");
                vita2d_pgf_draw_text(font, 360, 370, c2, 1.3f, (menu_cursor == 2) ? "> 3. Supervivencia" : "  3. Supervivencia");

                vita2d_pgf_draw_text(font, 310, 480, RGBA8(200, 200, 200, 255), 1.0f, "Pulsa (X) para Iniciar | START para Salir");
            }
        }
        else if (state == STATE_PLAYING) {
            // Línea central decorativa
            for (int y = 0; y < SCREEN_H; y += 40) {
                vita2d_draw_rectangle(SCREEN_W / 2.0f - 2, y, 4, 20, RGBA8(50, 50, 70, 255));
            }

            // Marcador en texto
            char score_str[64];
            if (font) {
                if (mode == MODE_SURVIVAL) {
                    snprintf(score_str, sizeof(score_str), "REBOTES: %d", score_p1);
                    vita2d_pgf_draw_text(font, 400, 60, RGBA8(255, 255, 255, 255), 1.3f, score_str);
                } else {
                    snprintf(score_str, sizeof(score_str), "%d         %d", score_p1, score_p2);
                    vita2d_pgf_draw_text(font, 420, 60, RGBA8(255, 255, 255, 255), 1.6f, score_str);
                }
            }

            // Paleta 1 (Cyan / Azul Claro)
            vita2d_draw_rectangle(p1_x, p1_y, paddle_w, paddle_h, RGBA8(0, 220, 255, 255));

            // Paleta 2 (Rojo / Rosa Claro) si no es supervivencia
            if (mode != MODE_SURVIVAL) {
                vita2d_draw_rectangle(p2_x, p2_y, paddle_w, paddle_h, RGBA8(255, 70, 90, 255));
            } else {
                // Pared reflectora en supervivencia
                vita2d_draw_rectangle(SCREEN_W - 8, 0, 8, SCREEN_H, RGBA8(100, 255, 100, 255));
            }

            // Color dinámico de la pelota según la velocidad
            unsigned int ball_color = RGBA8(0, 255, 180, 255); // Verde inicial
            if (ball_speed_mult > 1.3f && ball_speed_mult <= 1.7f) {
                ball_color = RGBA8(255, 220, 0, 255); // Amarillo
            } else if (ball_speed_mult > 1.7f && ball_speed_mult <= 2.0f) {
                ball_color = RGBA8(255, 120, 0, 255); // Naranja
            } else if (ball_speed_mult > 2.0f) {
                ball_color = RGBA8(255, 30, 30, 255);  // Rojo fuego
            }

            vita2d_draw_rectangle(ball_x, ball_y, ball_size, ball_size, ball_color);
        }
        else if (state == STATE_GAMEOVER) {
            if (font) {
                if (mode == MODE_SURVIVAL) {
                    char res[64];
                    snprintf(res, sizeof(res), "RECORD FINAL: %d PUNTOS", score_p1);
                    vita2d_pgf_draw_text(font, 300, 220, RGBA8(255, 215, 0, 255), 1.4f, res);
                } else {
                    if (score_p1 >= MAX_SCORE) {
                        vita2d_pgf_draw_text(font, 360, 220, RGBA8(50, 255, 100, 255), 1.6f, "¡VICTORIA!");
                    } else {
                        vita2d_pgf_draw_text(font, 360, 220, RGBA8(255, 50, 50, 255), 1.6f, "¡DERROTA!");
                    }
                }
                vita2d_pgf_draw_text(font, 280, 340, RGBA8(220, 220, 220, 255), 1.1f, "Presiona (X) para volver al Menu");
            }
        }

        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    if (font) vita2d_free_pgf(font);
    vita2d_fini();
    return 0;
}
