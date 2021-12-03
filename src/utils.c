#include "draw.h"
#include "utils.h"

#define TEXT_LEFT_BOUND (tex->x - tex->width/2)
#define TEXT_RIGHT_BOUND (tex->x + tex->width/2)
void vMoveUpperText(text_t *tex, direction_t *dir, unsigned short speed)
{
        switch (*dir) {
        case LEFT:
                if (TEXT_LEFT_BOUND > 0) {
                        tex->x -= speed;
                } else {
                        tex->x += speed;
                        (*dir) = RIGHT;
                }
                break;

        case RIGHT:
                if (TEXT_RIGHT_BOUND < SCREEN_WIDTH) {
                        tex->x += speed;
                } else {
                        tex->x -= speed;
                        (*dir) = LEFT;
                }
                break;
        
        default:
                break;
                        
        }
}

void vRotateFigures(circle_t *circ, rectangle_t *rec, path_t *circle_path, path_t *rect_path)
{
        if (circle_path->cur == circle_path->path_size - 1) {
                circle_path->cur = 0;
        } else {
                circ->x = circle_path->path_coord[circle_path->cur].x;
                circ->y = circle_path->path_coord[circle_path->cur].y;
                circle_path->cur++;
        }

        if (rect_path->cur == rect_path->path_size - 1) {
                rect_path->cur = 0;
        } else {
                rec->x = rect_path->path_coord[rect_path->cur].x;
                rec->y = rect_path->path_coord[rect_path->cur].y;
                rect_path->cur++;
        }
}

void vMonitorButtonsEx2(buttons_ex2_t *buttons_ex2, unsigned char buttons[SDL_NUM_SCANCODES])
{
        if(buttons[KEYCODE(A)]){
                if(buttons[KEYCODE(A)]!=buttons_ex2->a_state)
                        buttons_ex2->a_num+=1;
        } 
        if(buttons[KEYCODE(B)]) {
                if(buttons[KEYCODE(B)]!=buttons_ex2->b_state)
                        buttons_ex2->b_num+=1;
        }
        if(buttons[KEYCODE(C)]) {
                if(buttons[KEYCODE(C)]!=buttons_ex2->c_state)
                        buttons_ex2->c_num+=1;       

        }
        if(buttons[KEYCODE(D)]){ 
                if(buttons[KEYCODE(D)]!=buttons_ex2->d_state)
                        buttons_ex2->d_num+=1;
        }

        // Remembering previous states for debouncing
        buttons_ex2->a_state=buttons[KEYCODE(A)];
        buttons_ex2->b_state=buttons[KEYCODE(B)]; 
        buttons_ex2->c_state=buttons[KEYCODE(C)]; 
        buttons_ex2->d_state=buttons[KEYCODE(D)];
}

void vMonitorButtonsEx3(buttons_ex3_t *buttons_ex3, unsigned char buttons[SDL_NUM_SCANCODES], 
                        SemaphoreHandle_t *SynchTwoBinary, TaskHandle_t *TaskSynch1, 
                        TaskHandle_t *TaskStopResume, unsigned char *stop_resume_signal)
{
        if(buttons[KEYCODE(S)]){
                if(buttons[KEYCODE(S)]!=buttons_ex3->s_state)
                        xSemaphoreGive(*SynchTwoBinary);
        } 

        if(buttons[KEYCODE(T)]){
                if(buttons[KEYCODE(T)]!=buttons_ex3->t_state)
                        xTaskNotify(*TaskSynch1, 0x01, eSetValueWithOverwrite);
        } 

        if(buttons[KEYCODE(O)]){
                if(buttons[KEYCODE(O)]!=buttons_ex3->o_state) {
                        *stop_resume_signal = !(*stop_resume_signal);
                        if ((int) *stop_resume_signal == 1) {
                                vTaskResume(*TaskStopResume);
                        } else {
                                vTaskSuspend(*TaskStopResume);
                        }
                }
        } 
        
        // Remembering previous states for debouncing
        buttons_ex3->s_state=buttons[KEYCODE(S)];
        buttons_ex3->t_state=buttons[KEYCODE(T)];
        buttons_ex3->o_state=buttons[KEYCODE(O)];
}

void vMonitorMouse(buttons_ex2_t *buttons_ex2)
{
        if (tumEventGetMouseLeft()) {
                buttons_ex2->a_num = 0;
                buttons_ex2->b_num = 0;
                buttons_ex2->c_num = 0;
                buttons_ex2->d_num = 0;
        }
}

void vMonitorBlinkCircle(circle_t *circ, flag_buffer_t *circle_flag)
{

        if (xSemaphoreTake(circle_flag->lock, 0) == pdTRUE) {
                if (circle_flag->flag) {
                        drawCircle(circ);
                }
                xSemaphoreGive(circle_flag->lock);
        }
}

#define COUNT_STATE 1
#define RESET_STATE 2
void vMonitorResetTimer(unsigned char *reset_cnt, QueueHandle_t *ResetTimerQueue)
{
        int signal;
        if (xQueueReceive(*ResetTimerQueue, &signal, 0) == pdTRUE) {
                *reset_cnt = signal;
        }
}
