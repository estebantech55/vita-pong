#include <stdio.h>
#include <stdlib.h>
#include <vita2d.h>
#include <psp2/ctrl.h>

#define SCREEN_W 960
#define SCREEN_H 544

int main(void) {
    vita2d_init();
    vita2d_set_clear_color(RGBA8(20, 20, 30, 255));

    float paddle_x = 40.0f;
    float paddle_y = (SCREEN_H / 2.0f) - 50.0f;
    float paddle_w = 20.0f;
    float paddle_h = 100.0f;
    float paddle_speed = 7.0f;

    float ball_x = SCREEN_W / 2.0f;
    float ball_y = SCREEN_H / 2.0f;
    float ball_size = 16.0f;
    float ball_vx = 5.0f;
    float ball_vy = 5.0f;

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    SceCtrlData pad;

    while (1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);

        if (pad.buttons & SCE_CTRL_START) {
            break;
        }

        if (pad.buttons & SCE_CTRL_UP)    paddle_y -= paddle_speed;
        if (pad.buttons & SCE_CTRL_DOWN)  paddle_y += paddle_speed;
        if (pad.ly < 80)                  paddle_y -= paddle_speed;
        if (pad.ly > 175)                 paddle_y += paddle_speed;

        if (paddle_y < 0) paddle_y = 0;
        if (paddle_y + paddle_h > SCREEN_H) paddle_y = SCREEN_H - paddle_h;

        ball_x += ball_vx;
        ball_y += ball_vy;

        if (ball_y <= 0 || (ball_y + ball_size) >= SCREEN_H) {
            ball_vy = -ball_vy;
        }
        if ((ball_x + ball_size) >= SCREEN_W) {
            ball_vx = -ball_vx;
        }

        if (ball_x <= (paddle_x + paddle_w) &&
            (ball_x + ball_size) >= paddle_x &&
            (ball_y + ball_size) >= paddle_y &&
            ball_y <= (paddle_y + paddle_h)) 
        {
            ball_vx = -ball_vx;
            ball_x = paddle_x + paddle_w + 1;
        }

        if (ball_x < 0) {
            ball_x = SCREEN_W / 2.0f;
            ball_y = SCREEN_H / 2.0f;
            ball_vx = 5.0f;
            ball_vy = (rand() % 2 == 0) ? 5.0f : -5.0f;
        }

        vita2d_start_drawing();
        vita2d_clear_screen();

        vita2d_draw_rectangle(paddle_x, paddle_y, paddle_w, paddle_h, RGBA8(255, 255, 255, 255));
        vita2d_draw_rectangle(ball_x, ball_y, ball_size, ball_size, RGBA8(255, 215, 0, 255));

        for (int y = 0; y < SCREEN_H; y += 40) {
            vita2d_draw_rectangle(SCREEN_W / 2.0f - 2, y, 4, 20, RGBA8(80, 80, 100, 255));
        }

        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    vita2d_fini();
    return 0;
}
