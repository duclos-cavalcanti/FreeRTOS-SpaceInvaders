#ifndef __MAIN_H__
#define __MAIN_H__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "task.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Font.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_FreeRTOS_Utils.h"
#include "TUM_Print.h"

#include "AsyncIO.h"

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

typedef struct flag_buffer {
        unsigned char flag;
        SemaphoreHandle_t lock;
} flag_buffer_t;

typedef struct buttons_buffer {
        unsigned char buttons[SDL_NUM_SCANCODES];
        SemaphoreHandle_t lock;
} buttons_buffer_t;

typedef struct mouse_xy {
        signed short x,y;
} mouse_xy_t;

typedef struct buttons_ex2 {
        signed short a_num;
        signed short b_num;
        signed short c_num;
        signed short d_num;

        signed short a_state;
        signed short b_state;
        signed short c_state;
        signed short d_state;
} buttons_ex2_t;

typedef struct buttons_ex3 {
        signed short s_num;
        signed short t_num;
        signed short o_num;

        signed short s_state;
        signed short t_state;
        signed short o_state;
} buttons_ex3_t;

typedef struct message {
        char str[15 + 1][4];
} message_t;

typedef struct message_buffer {
        message_t message;
        SemaphoreHandle_t lock;
} message_buffer_t;

typedef struct tick_ex4 {
        int tickNo;
        int val;
} tick_ex4_t;


#endif 

