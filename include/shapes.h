#ifndef __SHAPES_H__
#define __SHAPES_H__

#include "main.h"

typedef struct circle {
        signed short x,y;
        signed short r;
        unsigned int color;
} circle_t;

typedef struct rectangle {
        signed short x,y;
        signed short h;
        signed short w;
        unsigned int color;
} rectangle_t;

typedef struct triangle {
        signed short x,y;
        signed short b;
        signed short h;
        unsigned int color;
} triangle_t;

typedef struct path_coord {
        signed short x;        	
        signed short y;        	
} path_coord_t;

typedef struct path {
        path_coord_t *path_coord;
        int path_size;
        int cur;
} path_t;


circle_t *shapesCreateCircle(int x, int y, int r, unsigned int color);
triangle_t *shapesCreateTriangle(int x, int y, int h, int b, unsigned int color);
rectangle_t *shapesCreateRectangle(int x, int y, int h, int w, unsigned int color);

path_t *shapesCreateRoundPath(signed short angle_path, signed short r_path);
#endif
