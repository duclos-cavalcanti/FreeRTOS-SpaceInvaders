#include "shapes.h"

circle_t *shapesCreateCircle(int x, int y, int r, unsigned int color)
{
        circle_t* circle = (circle_t*) pvPortMalloc(1 * sizeof(circle_t));

        circle->x = (signed short) x;
        circle->y = (signed short) y;
        circle->r = (signed short) r;
        circle->color = color;

        return circle;
}

triangle_t *shapesCreateTriangle(int x, int y, int h, int b, unsigned int color)
{
        triangle_t* triangle = (triangle_t*) pvPortMalloc(1 * sizeof(triangle_t));

        triangle->x = (signed short) x;
        triangle->y = (signed short) y;
        triangle->b = (signed short) b;
        triangle->h = (signed short) h;
        triangle->color = color;

        return triangle;
}

rectangle_t *shapesCreateRectangle(int x, int y, int h, int w, unsigned int color)
{
        rectangle_t* rectangle = (rectangle_t*) pvPortMalloc(1 * sizeof(rectangle_t));

        rectangle->x = (signed short) x;
        rectangle->y = (signed short) y;
        rectangle->w = (signed short) w;
        rectangle->h = (signed short) h;
        rectangle->color = color;

        return rectangle;
}

#define PI (3.14159265358979323846264338327950288)
path_t *shapesCreateRoundPath(signed short angle_path, signed short r_path)
{
        signed short path_size = 360 / angle_path;

        path_t* path = (path_t*) pvPortMalloc(1 * sizeof(path_t));
        path_coord_t* path_coord = (path_coord_t*) pvPortMalloc(path_size * sizeof(path_coord_t));

        float rad=(PI/180);
        float angle_path_in_rad = angle_path * rad;


        for (int i=0; i < path_size; i++){
                static signed short x_coord;
                static signed short y_coord;

                signed short angle_i = angle_path*i;
                float angle_in_rad_i = angle_path_in_rad*i;

                x_coord=abs((int)(r_path * cos(angle_in_rad_i))); 
                y_coord=abs((int)(r_path * sin(angle_in_rad_i)));

                if (0 <= angle_i && angle_i <= 90) {
                        path_coord[i].x = SCREEN_WIDTH/2 + x_coord;
                        path_coord[i].y = SCREEN_HEIGHT/2 - y_coord;

                } else if(90 < angle_i && angle_i <= 180) {
                        path_coord[i].x = SCREEN_WIDTH/2 - x_coord;
                        path_coord[i].y = SCREEN_HEIGHT/2 - y_coord;

                } else if(180 < angle_i && angle_i <= 270) {
                        path_coord[i].x = SCREEN_WIDTH/2 - x_coord;
                        path_coord[i].y = SCREEN_HEIGHT/2 + y_coord;

                } else if(270 < angle_i && angle_i <= 360) {
                        path_coord[i].x = SCREEN_WIDTH/2 + x_coord;
                        path_coord[i].y = SCREEN_HEIGHT/2 + y_coord;
                } 
        }

        path->path_coord = path_coord;
        path->cur = 0;
        path->path_size = path_size;

        return path;
}
