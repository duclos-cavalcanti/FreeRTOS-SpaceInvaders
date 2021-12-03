#ifndef __UTILS_H__
#define __UTILS_H__

#include "main.h"
#include "draw.h"

typedef enum direction {
  LEFT,
  RIGHT,
  UP,
  DOWN
} direction_t;

void vMoveUpperText(text_t *tex, direction_t *dir, unsigned short speed);
void vRotateFigures(circle_t *circ, rectangle_t *rec, path_t *circle_path, path_t *rect_path);
void vMonitorButtonsEx2(buttons_ex2_t *buttons_ex2, unsigned char buttons[SDL_NUM_SCANCODES]);

void vMonitorButtonsEx3(buttons_ex3_t *buttons_ex3, unsigned char buttons[SDL_NUM_SCANCODES], 
                        SemaphoreHandle_t *SynchTwoBinary, TaskHandle_t *TaskSynch1, 
                        TaskHandle_t *TaskStopResume, unsigned char *stop_resume_signal);

void vMonitorMouse(buttons_ex2_t *buttons_ex2);
void vMonitorBlinkCircle(circle_t *circ, flag_buffer_t *circle_flag);
void vMonitorResetTimer(unsigned char *reset_cnt, QueueHandle_t *ResetTimerQueue);

#endif

