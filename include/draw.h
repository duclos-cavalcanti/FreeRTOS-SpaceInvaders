#ifndef __DRAW_H__
#define __DRAW_H__

#include "main.h"
#include "shapes.h"

#define FPS_FONT "IBMPlexSans-Bold.ttf"

typedef struct text {
        char str[100];
        signed short x,y;	
        signed short width;	
        unsigned int color;
        unsigned short f_size;
} text_t;

text_t *drawCreateText(char* str, signed short x, signed short y, unsigned int color, unsigned short f_size);

void drawCheckStatus(unsigned char status, const char *msg);
void drawFPS(void);
void drawButtonText(buttons_ex2_t buttons_ex2, mouse_xy_t mouse_xy_copy);
void drawHelpText(void);
void drawCircle(circle_t *circ);
void drawTriangle(triangle_t *tri);
void drawRectangle(rectangle_t *rec);
void drawText(text_t *text);
void drawMessages(message_t mess);

#endif

