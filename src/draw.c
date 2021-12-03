#include "draw.h"

text_t *drawCreateText(char* str, signed short x, signed short y, unsigned int color, unsigned short f_size)
{
        text_t* text = (text_t*) pvPortMalloc(1 * sizeof(text_t));

        memcpy(text->str, str, sizeof(unsigned char) * 100);
        text->x = x;
        text->y = y;
        text->width = 0;
        text->color = color;
        text->f_size = f_size;

        return text;
}

void drawCheckStatus(unsigned char status, const char *msg)
{
        if (status) {
                if (msg)
                        fprints(stderr, "[ERROR] %s, %s\n", msg,
                                tumGetErrorMessage());
                else {
                        fprints(stderr, "[ERROR] %s\n", tumGetErrorMessage());
                }
        }
}

void drawText(text_t *text)
{
        static int text_width;

        ssize_t prev_font_size = tumFontGetCurFontSize();
        tumFontSetSize((ssize_t)text->f_size);

        if (!tumGetTextSize((char *)text->str, &text_width, NULL))
                drawCheckStatus(tumDrawText(text->str,
                                text->x - text_width / 2,
                                text->y - text->f_size / 2,
                                text->color),
                __FUNCTION__);

        text->width = text_width;
        tumFontSetSize(prev_font_size);
}

void drawTextXY(text_t *text, signed short x, signed short y)
{
        static int text_width;

        ssize_t prev_font_size = tumFontGetCurFontSize();
        tumFontSetSize((ssize_t)text->f_size);

        if (!tumGetTextSize((char *)text->str, &text_width, NULL))
                drawCheckStatus(tumDrawText(text->str,
                                x,
                                y,
                                text->color),
                __FUNCTION__);

        text->width = text_width;
        tumFontSetSize(prev_font_size);
}

void drawCircle(circle_t *circ)
{
        drawCheckStatus(tumDrawCircle(circ->x, circ->y,
                                      circ->r, circ->color),
                        __FUNCTION__);
}

void drawTriangle(triangle_t *tri)
{
        coord_t coords[3] = { {tri->x, (tri->y - tri->h/2)}, 
                              {(tri->x - tri->b/2), (tri->y + tri->h/2)}, 
                              {(tri->x + tri->b/2), (tri->y + tri->h/2)}
                            };
        drawCheckStatus(tumDrawTriangle(coords, tri->color),
                        __FUNCTION__);
}

void drawRectangle(rectangle_t *rec)
{
        drawCheckStatus(tumDrawFilledBox( (rec->x - rec->w/2), (rec->y - rec->h/2), 
                                          rec->w, rec->h, rec->color),
                        __FUNCTION__);
}

#define FPS_AVERAGE_COUNT 50
void drawFPS(void)
{
        static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
        static unsigned int periods_total = 0;
        static unsigned int index = 0;
        static unsigned int average_count = 0;
        static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
        static char str[10] = { 0 };
        static int text_width;
        int fps = 0;
        font_handle_t cur_font = tumFontGetCurFontHandle();

        if (average_count < FPS_AVERAGE_COUNT) {
                average_count++;
        } else {
                periods_total -= periods[index];
        }

        xLastWakeTime = xTaskGetTickCount();

        if (prevWakeTime != xLastWakeTime) {
                periods[index] =
                        configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
                prevWakeTime = xLastWakeTime;
        } else {
                periods[index] = 0;
        }

        periods_total += periods[index];

        if (index == (FPS_AVERAGE_COUNT - 1)) {
                index = 0;
        } else {
                index++;
        }

        fps = periods_total / average_count;

        tumFontSelectFontFromName(FPS_FONT);

        sprintf(str, "FPS: %2d", fps);

        if (!tumGetTextSize((char *)str, &text_width, NULL))
                drawCheckStatus(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                                  SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                                  Skyblue),
                                __FUNCTION__);

        tumFontSelectFontFromHandle(cur_font);
        tumFontPutFontHandle(cur_font);
}

void drawButtonText(buttons_ex2_t buttons_ex2, mouse_xy_t mouse_xy_copy)
{
        static char str[100] = { 0 };
        sprintf(str, "Axis 1: %5d | Axis 2: %5d", mouse_xy_copy.x, mouse_xy_copy.y);

        drawCheckStatus(tumDrawText(str, 10, DEFAULT_FONT_SIZE * 0.5, Black),
                        __FUNCTION__);

        sprintf(str, "A: %d | B: %d | C: %d | D: %d",
                buttons_ex2.a_num,
                buttons_ex2.b_num,
                buttons_ex2.c_num,
                buttons_ex2.d_num);
        drawCheckStatus(tumDrawText(str, 10, DEFAULT_FONT_SIZE * 2, Black),
                  __FUNCTION__);

}


void drawHelpText(void)
{
    static char str[100] = { 0 };
    static int text_width;
    ssize_t prev_font_size = tumFontGetCurFontSize();

    tumFontSetSize((ssize_t)30);

    sprintf(str, "[Q]uit, [C]hange State");

    if (!tumGetTextSize((char *)str, &text_width, NULL))
        drawCheckStatus(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              DEFAULT_FONT_SIZE * 0.5, Black),
                  __FUNCTION__);

    tumFontSetSize(prev_font_size);
}

#define MESS_X SCREEN_WIDTH/2
#define MESS_Y 100
#define MESS_FONT_S 20
void drawMessages(message_t mess)
{
        char val[4] = { 0 };
        for (int i = 0; i <= 15; i++) {
                text_t *tex = drawCreateText("", MESS_X, MESS_Y + i*20, Red, MESS_FONT_S);
                for (int j = 0; j < 4; j++) {
                        val[j] = mess.str[i][j];
                }
                sprintf(tex->str, "Tick Number: %d: %c, %c, %c, %c", 
                                  i, val[0], val[1], val[2], val[3]);
                drawTextXY(tex, MESS_X - 100, tex->y);
                vPortFree(tex) ;
        }
}
