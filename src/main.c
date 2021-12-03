#include "main.h"
#include "shapes.h"
#include "draw.h"
#include "utils.h"

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define STATE_QUEUE_LENGTH 1

#ifdef TRACE_FUNCTIONS
#include "tracer.h"
#endif

#define STATE_COUNT     3
#define STATE_ONE       0
#define STATE_TWO       1
#define STATE_THREE     2
#define STARTING_STATE STATE_ONE

#define NEXT_TASK       0
#define PREV_TASK       1

#define STATE_DEBOUNCE_DELAY 300
const unsigned char next_state_signal = NEXT_TASK;
const unsigned char prev_state_signal = PREV_TASK;

static TaskHandle_t StateMachine = NULL;
static TaskHandle_t BufferSwap = NULL;
static TaskHandle_t TaskEx2 = NULL;
static TaskHandle_t TaskEx3 = NULL;
static TaskHandle_t TaskEx4 = NULL;

static TaskHandle_t TaskBlink1 = NULL;

#define STACK_SIZE 200
StaticTask_t xTaskBuffer;
StackType_t xStack[mainGENERIC_STACK_SIZE];
static TaskHandle_t TaskBlink2 = NULL;

static TaskHandle_t TaskSynch1 = NULL;
static TaskHandle_t TaskSynch2 = NULL;
static TaskHandle_t TaskResetTimer = NULL;
static TaskHandle_t TaskStopResume = NULL;

static TaskHandle_t TaskTick1 = NULL;
static TaskHandle_t TaskTick2 = NULL;
static TaskHandle_t TaskTick3 = NULL;
static TaskHandle_t TaskTick4 = NULL;

static QueueHandle_t StateQueue = NULL;
static QueueHandle_t SynchOneQueue = NULL;
static QueueHandle_t SynchTwoQueue = NULL;
static QueueHandle_t ResetTimerQueue = NULL;
static QueueHandle_t StopResumeQueue = NULL;
static QueueHandle_t TicksQueue = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t SynchTwoBinary = NULL;
static SemaphoreHandle_t TaskTicksBinary = NULL;

static TimerHandle_t xResetTimer = NULL;

static buttons_buffer_t buttons = { 0 };

static flag_buffer_t circle_1hz = { 0 };
static flag_buffer_t circle_2hz = { 0 };

static flag_buffer_t SynchOnePress = { 0 };
static flag_buffer_t SynchTwoPress = { 0 };

static message_buffer_t ticks_message = { 0 };

// Static Tasks
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{

static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

// Timers
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{

static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];


    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}


/*
 * Changes the state, either forwards of backwards
 */
void changeState(volatile unsigned char *state, unsigned char forwards)
{
    switch (forwards) {
        case NEXT_TASK:
            if (*state == STATE_COUNT - 1) {
                *state = 0;
            }
            else {
                (*state)++;
            }
            break;
        case PREV_TASK:
            if (*state == 0) {
                *state = STATE_COUNT - 1;
            }
            else {
                (*state)--;
            }
            break;
        default:
            break;
    }
}

static int vCheckStateInput(void)
{
        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                if (buttons.buttons[KEYCODE(E)]) {
                        buttons.buttons[KEYCODE(E)] = 0;
                        if (StateQueue) {
                                xSemaphoreGive(buttons.lock);
                                xQueueSend(StateQueue, &next_state_signal, 0);
                                return 0;
                        }
                        return -1;
                }
                xSemaphoreGive(buttons.lock);
    }

    return 0;
}

/*
 * Example basic state machine with sequential states
 */

void basicSequentialStateMachine(void *pvParameters)
{
    unsigned char current_state = STARTING_STATE; // Default state
    unsigned char state_changed = 1; // Only re-evaluate state if it has changed
    unsigned char input = 0;

    const int state_change_period = STATE_DEBOUNCE_DELAY;

    TickType_t last_change = xTaskGetTickCount();

    while (1) {
        if (state_changed) {
            goto initial_state;
        }

        // Handle state machine input
        if (StateQueue)
            if (xQueueReceive(StateQueue, &input, portMAX_DELAY) ==
                pdTRUE)
                if (xTaskGetTickCount() - last_change > state_change_period) {
                    changeState(&current_state, input);
                    state_changed = 1;
                    last_change = xTaskGetTickCount();
                }

initial_state:
        // Handle current state
        if (state_changed) {
                switch (current_state) {
                case STATE_ONE:
                    if (TaskEx3) {
                        vTaskSuspend(TaskEx3);
                    }
                    if (TaskEx4) {
                        vTaskSuspend(TaskEx4);
                    }
                    if (TaskBlink1) {
                        vTaskSuspend(TaskBlink1);
                    }
                    if (TaskBlink2) {
                        vTaskSuspend(TaskBlink2);
                    }
                    if (TaskSynch1) {
                        vTaskSuspend(TaskSynch1);
                    }
                    if (TaskSynch2) {
                        vTaskSuspend(TaskSynch2);
                    }
                    if (TaskResetTimer) {
                        vTaskSuspend(TaskResetTimer);
                    }
                    if (TaskStopResume) {
                        vTaskSuspend(TaskStopResume);
                    }

                    if (TaskEx2) {
                        vTaskResume(TaskEx2);
                    }
                    break;

                case STATE_TWO:
                    if (TaskEx2) {
                        vTaskSuspend(TaskEx2);
                    }
                    if (TaskEx3) {
                        vTaskResume(TaskEx3);
                    }
                    if (TaskBlink1) {
                        vTaskResume(TaskBlink1);
                    }
                    if (TaskBlink2) {
                        vTaskResume(TaskBlink2);
                    }
                    if (TaskSynch1) {
                        vTaskResume(TaskSynch1);
                    }
                    if (TaskSynch2) {
                        vTaskResume(TaskSynch2);
                    }
                    if (TaskResetTimer) {
                        vTaskResume(TaskResetTimer);
                    }
                    if (TaskStopResume) {
                        vTaskResume(TaskStopResume);
                    }
                    break;

                case STATE_THREE:
                    if (TaskEx2) {
                        vTaskSuspend(TaskEx2);
                    }
                    if (TaskEx3) {
                        vTaskSuspend(TaskEx3);
                    }
                    if (TaskBlink1) {
                        vTaskSuspend(TaskBlink1);
                    }
                    if (TaskBlink2) {
                        vTaskSuspend(TaskBlink2);
                    }
                    if (TaskSynch1) {
                        vTaskSuspend(TaskSynch1);
                    }
                    if (TaskSynch2) {
                        vTaskSuspend(TaskSynch2);
                    }
                    if (TaskResetTimer) {
                        vTaskSuspend(TaskResetTimer);
                    }
                    if (TaskStopResume) {
                        vTaskSuspend(TaskStopResume);
                    }
                    if (TaskStopResume) {
                        vTaskSuspend(TaskStopResume);
                    }
                    if (TaskEx4) {
                        vTaskResume(TaskEx4);
                    }
                    break;

                default:
                    break;
                }
                printf("State Change\n");
                tumFUtilPrintTaskStateList();
                state_changed = 0;
        }
    }
}

void vSwapBuffers(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 20;

    tumDrawBindThread(); // Setup Rendering handle with correct GL context

    while (1) {
        if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
            tumDrawUpdateScreen();
            tumEventFetchEvents(FETCH_EVENT_BLOCK);
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}

void xGetFlagValue(flag_buffer_t flag_buffer, unsigned char *flag_copy)
{
    if (xSemaphoreTake(flag_buffer.lock, 0) == pdTRUE) {
        *flag_copy = flag_buffer.flag; 
        xSemaphoreGive(flag_buffer.lock);
    }
}

void xGetButtonInput(unsigned char *buttons_copy)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        memcpy(buttons_copy, buttons.buttons, sizeof(unsigned char) * SDL_NUM_SCANCODES);
        xSemaphoreGive(buttons.lock);
    }
}

void xGetValueSynchOne(unsigned char *synch_one_num)
{
        unsigned char val;
        if (SynchOneQueue) {
                if(xQueueReceive(SynchOneQueue, &val, 0) == pdTRUE)
                        *synch_one_num = val;
        }
}

void xGetValueSynchTwo(unsigned char *synch_two_num)
{
        unsigned char val;
        if (SynchTwoQueue) {
                if(xQueueReceive(SynchTwoQueue, &val, 0) == pdTRUE)
                        *synch_two_num = val;
        }
}

void xGetValueStopCount(unsigned char *stop_count)
{
        unsigned char val;
        if (SynchTwoQueue)
                if(xQueueReceive(StopResumeQueue, &val, 0) == pdTRUE)
                        *stop_count = val;
}

void xGetMouseXY(mouse_xy_t *mouse_xy_copy)
{
        mouse_xy_copy->x = tumEventGetMouseX();
        mouse_xy_copy->y = tumEventGetMouseY();
}

#define CIRCLE_X SCREEN_WIDTH/3
#define CIRCLE_Y SCREEN_HEIGHT/2
#define RADIUS 25
#define REC_X SCREEN_WIDTH*2/3
#define REC_Y SCREEN_HEIGHT/2
#define REC_H 50
#define REC_W 50
#define TRI_X SCREEN_WIDTH/2
#define TRI_Y SCREEN_HEIGHT/2
#define TRI_H 50
#define TRI_B 50
#define EX_FONT_S 20
#define EX_TEXT_X SCREEN_WIDTH - 25
#define EX_TEXT_Y 25
#define BELOW_TEXT_X SCREEN_WIDTH/2
#define BELOW_TEXT_Y SCREEN_HEIGHT - 25
#define UPPER_TEXT_X SCREEN_WIDTH/2
#define UPPER_TEXT_Y 100
#define PATH_RADIUS SCREEN_WIDTH / 8
#define PATH_ANGLE 1
void vTaskEx2(void *pvParameters)
{
        unsigned char buttons_copy[SDL_NUM_SCANCODES] = { 0 };
        mouse_xy_t mouse_xy_copy = { 0 };

        direction_t moving_text_dir = LEFT;

        text_t *exercise_text = drawCreateText("Ex: 2", EX_TEXT_X, EX_TEXT_Y, Red, EX_FONT_S);
        text_t *moving_text = drawCreateText("Moving Upper Text", UPPER_TEXT_X, UPPER_TEXT_Y, Red, 15);
        text_t *still_text = drawCreateText("Still Below Text", BELOW_TEXT_X, BELOW_TEXT_Y, Red, 15);

        circle_t *circle = shapesCreateCircle(CIRCLE_X, CIRCLE_Y, RADIUS, Blue);
        triangle_t *tri = shapesCreateTriangle(TRI_X, TRI_Y, TRI_H, TRI_B, Green);
        rectangle_t *rect = shapesCreateRectangle(REC_X, REC_Y, REC_H, REC_W, Red);

        path_t *round_path = shapesCreateRoundPath(PATH_ANGLE, PATH_RADIUS);

        path_t circle_path = *round_path;
        path_t rect_path = *round_path;

        circle_path.cur = 180;
        rect_path.cur = 0;

        buttons_ex2_t buttons_ex2;

        while (1) {
                if (DrawSignal)
                        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                                    FETCH_EVENT_NO_GL_CHECK);
                                xGetButtonInput(buttons_copy);  // Update buttons from global buttons
                                xGetMouseXY(&mouse_xy_copy);    // Update mouse

                                vMonitorMouse(&buttons_ex2);
                                vMonitorButtonsEx2(&buttons_ex2, buttons_copy);
                                vRotateFigures(circle, rect, &circle_path, &rect_path);
                                vMoveUpperText(moving_text, &moving_text_dir, 2);

                                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                                // Clear screen
                                drawCheckStatus(tumDrawClear(White), __FUNCTION__);
                                drawButtonText(buttons_ex2, mouse_xy_copy);

                                drawCircle(circle);
                                drawTriangle(tri);
                                drawRectangle(rect);

                                // Draw FPS in lower right corner
                                drawFPS();

                                drawText(exercise_text);
                                drawText(moving_text);
                                drawText(still_text);

                                xSemaphoreGive(ScreenLock);

                                // Get input and check for state change
                                vCheckStateInput();
                }
        }
}

#define SYNCH_FONT_S 15
void vTaskEx3(void *pvParameters)
{
        text_t *exercise_text = drawCreateText("Ex: 3", EX_TEXT_X, EX_TEXT_Y, Red, EX_FONT_S);
        text_t *synch1_binary_text = drawCreateText("[T]Task Notif. Task | Num: %d", 
                                                    BELOW_TEXT_X, BELOW_TEXT_Y - 20, Red, SYNCH_FONT_S);
        text_t *synch2_binary_text = drawCreateText("[S]Binary Sem. Task | Num: %d", 
                                                    BELOW_TEXT_X, BELOW_TEXT_Y, Red, SYNCH_FONT_S);

        text_t *reset_text = drawCreateText("Reset!", 
                                            BELOW_TEXT_X, BELOW_TEXT_Y - 40, Red, SYNCH_FONT_S);

        text_t *stop_text = drawCreateText("Stop!", 
                                            BELOW_TEXT_X, BELOW_TEXT_Y - 60, Red, SYNCH_FONT_S);

        circle_t *c_1 = shapesCreateCircle(CIRCLE_X, CIRCLE_Y, RADIUS, Blue);
        circle_t *c_2 = shapesCreateCircle(REC_X, REC_Y, RADIUS, Red);

        unsigned char buttons_copy[SDL_NUM_SCANCODES] = { 0 };

        unsigned char stop_resume_count = 0;
        unsigned char stop_resume_signal = 1;

        unsigned char synch_one_num = 0;
        unsigned char synch_two_num = 0;
        unsigned char reset_cnt = 0;

        buttons_ex3_t buttons_ex3;

        while (1) {
                if (DrawSignal)
                        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                                xGetButtonInput(buttons_copy); // copy global button data
                                vMonitorButtonsEx3(&buttons_ex3, buttons_copy, &SynchTwoBinary, &TaskSynch1, &TaskStopResume, &stop_resume_signal);

                                xGetValueSynchOne(&synch_one_num);
                                xGetValueSynchTwo(&synch_two_num);
                                xGetValueStopCount(&stop_resume_count);

                                vMonitorResetTimer(&reset_cnt, &ResetTimerQueue);

                                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                                // Clear screen
                                drawCheckStatus(tumDrawClear(White), __FUNCTION__);

                                // Draws FPS
                                drawFPS();

                                vMonitorBlinkCircle(c_1, &circle_1hz);
                                vMonitorBlinkCircle(c_2, &circle_2hz);

                                drawText(exercise_text);

                                sprintf(synch1_binary_text->str, 
                                        "[T] Binary Sem. Task | Num: %d", 
                                        synch_one_num);
                                drawText(synch1_binary_text);

                                sprintf(synch2_binary_text->str, 
                                        "[S] Task Notif. Task | Num: %d", 
                                        synch_two_num);
                                drawText(synch2_binary_text);

                                sprintf(reset_text->str, 
                                        "Reset Count [15]: %d", 
                                        reset_cnt);
                                drawText(reset_text);

                                sprintf(stop_text->str, 
                                        "[O] Stop/Resume Count: %d", 
                                        stop_resume_count);
                                drawText(stop_text);

                                xSemaphoreGive(ScreenLock);

                                // Check for state change
                                vCheckStateInput();
                        }
        }
}

void vTaskBlink1(void *pvParameters)
{

        TickType_t Last_Tick, Prev_Tick = 0;
        TickType_t Interval = 0;

        while(1) {
                Last_Tick=xTaskGetTickCount();
                Interval = Last_Tick - Prev_Tick;
                if (Interval > 500){
                        /* printf("Interval Value from Blink1: %d\n", (int)Interval); */
                        if (xSemaphoreTake(circle_1hz.lock, 0) == pdTRUE) {
                                circle_1hz.flag=!circle_1hz.flag;
                                xSemaphoreGive(circle_1hz.lock);
                        }
                        Prev_Tick = Last_Tick;
                }
                /* vTaskDelay((TickType_t)10); */
                vTaskDelayUntil(&Last_Tick,
                                pdMS_TO_TICKS(500));

        }
}

void vTaskBlink2(void *pvParameters)
{

        TickType_t Last_Tick, Prev_Tick = 0;
        TickType_t Interval = 0;

        Last_Tick=xTaskGetTickCount();

        while(1) {
                Last_Tick=xTaskGetTickCount();
                Interval = Last_Tick - Prev_Tick;
                if (Interval > 250){
                        /* printf("Interval Value from Blink2: %d\n", (int)Interval); */
                        if (xSemaphoreTake(circle_2hz.lock, 0) == pdTRUE) {
                                circle_2hz.flag=!circle_2hz.flag;
                                xSemaphoreGive(circle_2hz.lock);
                        }
                        Prev_Tick = Last_Tick;
                }
                /* vTaskDelay((TickType_t)10); */
                vTaskDelayUntil(&Last_Tick,
                                pdMS_TO_TICKS(250));
        }
}

#define COUNT_STATE 1
#define RESET_STATE 2
void vTaskSynch1(void *pvParameters)
{

        int cnt = 0;
        while(1) {
                uint32_t Receiving;
                if (xTaskNotifyWait(0x00, 0xffffffff, &Receiving, portMAX_DELAY) == pdTRUE) {
                        if (xSemaphoreTake(SynchOnePress.lock, 0) == pdTRUE) {
                                SynchOnePress.flag ++;
                                cnt = SynchOnePress.flag;
                                xQueueSend(SynchOneQueue, &cnt, 0);
                                /* printf("Overwritten from Synch1\n"); */
                                xSemaphoreGive(SynchOnePress.lock);
                        }
                }
        }
}

void vTaskSynch2(void *pvParameters)
{

        int cnt = 0;
        while(1){
                if ( xSemaphoreTake(SynchTwoBinary,portMAX_DELAY) == pdTRUE) {
                        if (xSemaphoreTake(SynchTwoPress.lock, 0) == pdTRUE) {
                                SynchTwoPress.flag ++;
                                cnt = SynchTwoPress.flag;
                                xQueueSend(SynchTwoQueue, &cnt, 0);
                                /* printf("Overwritten from Synch2\n"); */
                                xSemaphoreGive(SynchTwoPress.lock);
                        }
	        }
        }
}

void vResetTimerCallback(TimerHandle_t xResetTimer)
{
        xTaskNotify(TaskResetTimer, 0x01, eSetValueWithOverwrite);
}

void vTaskResetTimer(void *pvParameters)
{

        xResetTimer = xTimerCreate("Reset Timer", pdMS_TO_TICKS(1000), pdTRUE, (void *) 0, vResetTimerCallback);
        xTimerStart(xResetTimer, 0);

        int cnt = 0;
        while(1){
                uint32_t Receiving;
                if (xTaskNotifyWait(0x00, 0xffffffff, &Receiving, portMAX_DELAY) == pdTRUE) {
                        if (cnt < 15 - 1) {
                                cnt++;
                                xQueueSend(ResetTimerQueue, &cnt, 0);
                        } else {
                                cnt = 0;
                                xQueueSend(ResetTimerQueue, &cnt, 0);
                                if (xSemaphoreTake(SynchOnePress.lock, 0) == pdTRUE) {
                                        SynchOnePress.flag = 0;
                                        xQueueOverwrite(SynchOneQueue, &(SynchOnePress.flag));
                                        xSemaphoreGive(SynchOnePress.lock);
                                }

                                if (xSemaphoreTake(SynchTwoPress.lock, 0) == pdTRUE) {
                                        SynchTwoPress.flag = 0;
                                        xQueueOverwrite(SynchTwoQueue, &(SynchTwoPress.flag));
                                        xSemaphoreGive(SynchTwoPress.lock);
                                }
                        }
                }
        }
}

void vTaskStopResume(void *pvParameters)
{
        int cnt = 0;
        TickType_t Last_Tick, Prev_Tick = 0;
        TickType_t Interval = 0;

        while(1) {
                Last_Tick=xTaskGetTickCount();
                Interval = Last_Tick - Prev_Tick;
		if (Interval >= 1000) {
                        /* printf("Interval Value from vTaskStopResume: %d\n", (int)Interval); */
                        cnt++;
                        xQueueSend(StopResumeQueue, &cnt, 0);
                        Prev_Tick = Last_Tick;
		}

                /* vTaskDelay((TickType_t)20); */
                vTaskDelayUntil(&Last_Tick,
                                pdMS_TO_TICKS(1000));
        }
}


void xGetMessage(message_t *mess)
{
        if(xSemaphoreTake(ticks_message.lock, portMAX_DELAY) == pdTRUE) {
                *(mess) = ticks_message.message;
        }
}

void vTaskTick1(void *pvParameters);
void vTaskTick2(void *pvParameters);
void vTaskTick3(void *pvParameters);
void vTaskTick4(void *pvParameters);

void vProcessTicks();

void vTaskEx4(void *pvParameters)
{

        text_t *exercise_text = drawCreateText("Ex: 4", EX_TEXT_X, EX_TEXT_Y, Red, EX_FONT_S);
        unsigned char buttons_copy[SDL_NUM_SCANCODES] = { 0 };

        message_t mess = { 0 };

        xTaskCreate(vTaskTick1, "TaskTick1", mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 1, &TaskTick1);
        xTaskCreate(vTaskTick2, "TaskTick2", mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 2, &TaskTick2);
        xTaskCreate(vTaskTick3, "TaskTick3", mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 3, &TaskTick3);
        xTaskCreate(vTaskTick4, "TaskTick4", mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY + 4, &TaskTick4);

        unsigned char finished = 0;
        while(1) {
                if (DrawSignal)
                        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                                if (!finished) {
                                        vTaskDelay(15 + 1);

                                        vProcessTicks();
                                        xGetMessage(&mess);
                                        vTaskDelete(TaskTick3); // clean up task 3
                                        tumFUtilPrintTaskStateList();
                                        finished = 1;
                                }

                                xGetButtonInput(buttons_copy); // copy global button data

                                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                                // Clear screen
                                drawCheckStatus(tumDrawClear(White), __FUNCTION__);

                                drawText(exercise_text);
                                drawMessages(mess);

                                // Draws FPS
                                drawFPS();

                                xSemaphoreGive(ScreenLock);

                                // Check for state change
                                vCheckStateInput();
                        }
        }
}

void vProcessTicks()
{
        tick_ex4_t t;
        int j[15 + 1] = { 0 };

        xSemaphoreTake(ticks_message.lock, 0);
        if (TicksQueue) {
                while (xQueueReceive(TicksQueue, &t, 0) == pdTRUE) {
                        int tick = t.tickNo;
                        if (tick <= 15 && j[tick] < 4) {
                                ticks_message.message.str[tick][j[tick]] = t.val;
                                j[tick]++;
                        } else {
                                printf("Invalid Tick\n");
                                printf("TickNo: %d\n", t.tickNo);
                                printf("Tick Val: %c\n\n", t.val);
                        }
                }
                xSemaphoreGive(ticks_message.lock);
        } else {
                xSemaphoreGive(ticks_message.lock);
        }
}

void vTaskTick1(void *pvParameters)
{
        TickType_t Last_Tick = xTaskGetTickCount();
        TickType_t First_Tick = Last_Tick;
        char c = '1';
        tick_ex4_t t;
        while (1) {
                vTaskDelayUntil(&Last_Tick, 1);
                Last_Tick = xTaskGetTickCount();
                if (Last_Tick - First_Tick <= 15) {
                        t.tickNo = (int)(Last_Tick - First_Tick);
                        t.val = c;

                        xQueueSend(TicksQueue, &t, 0);
                } else {
                        vTaskDelete(NULL);
                }

        }
}

void vTaskTick2(void *pvParameters)
{
        TickType_t Last_Tick = xTaskGetTickCount();
        TickType_t First_Tick = Last_Tick;
        char c = '2';
        tick_ex4_t t;
        while (1) {
                vTaskDelayUntil(&Last_Tick, 2);
                Last_Tick = xTaskGetTickCount();
                if (Last_Tick - First_Tick <= 15) {
                        t.tickNo = (int)(Last_Tick - First_Tick);
                        t.val = c;

                        xQueueSend(TicksQueue, &t, 0);
                        xSemaphoreGive(TaskTicksBinary);
                } else {
                        vTaskDelete(NULL);
                }

        }
}

void vTaskTick3(void *pvParameters)
{
        TickType_t Last_Tick = xTaskGetTickCount();
        TickType_t First_Tick = Last_Tick;
        char c = '3';
        tick_ex4_t t;
        while (1) {
                if (xSemaphoreTake(TaskTicksBinary, portMAX_DELAY) == pdTRUE) {
                        Last_Tick = xTaskGetTickCount();
                        if (Last_Tick - First_Tick <= 15) {
                                t.tickNo = (int)(Last_Tick - First_Tick);
                                t.val = c;

                                xQueueSend(TicksQueue, &t, 0);
                                
                        } else {
                                vTaskDelete(NULL);
                        }
                }
        }
}

void vTaskTick4(void *pvParameters)
{
        TickType_t Last_Tick = xTaskGetTickCount();
        TickType_t First_Tick = Last_Tick;
        char c = '4';
        tick_ex4_t t;
        while (1) {
                vTaskDelayUntil(&Last_Tick, 4);
                Last_Tick = xTaskGetTickCount();
                if (Last_Tick - First_Tick <= 15) {
                        t.tickNo = (int)(Last_Tick - First_Tick);
                        t.val = c;

                        xQueueSend(TicksQueue, &t, 0);
                } else {
                        vTaskDelete(NULL);
                }
        }
}

#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");

int main(int argc, char *argv[])
{
        char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

        if (tumDrawInit(bin_folder_path)) {
            PRINT_ERROR("Failed to intialize drawing");
            goto err_init_drawing;
        }
        else {
            prints("drawing");
        }

        if (tumEventInit()) {
            PRINT_ERROR("Failed to initialize events");
            goto err_init_events;
        }
        else {
            prints(", events");
        }

        if (tumSoundInit(bin_folder_path)) {
            PRINT_ERROR("Failed to initialize audio");
            goto err_init_audio;
        }
        else {
            prints(", and audio\n");
        }

        if (safePrintInit()) {
            PRINT_ERROR("Failed to init safe print");
            goto err_init_safe_print;
        }

        atexit(aIODeinit);

        //Load a second font for fun
        tumFontLoadFont(FPS_FONT, DEFAULT_FONT_SIZE);

        buttons.lock = xSemaphoreCreateMutex(); // Locking mechanism
        if (!buttons.lock) {
            PRINT_ERROR("Failed to create buttons lock");
            goto err_buttons_lock;
        }

        DrawSignal = xSemaphoreCreateBinary(); // Screen buffer locking
        if (!DrawSignal) {
            PRINT_ERROR("Failed to create draw signal");
            goto err_draw_signal;
        }

        ScreenLock = xSemaphoreCreateMutex();
        if (!ScreenLock) {
            PRINT_ERROR("Failed to create screen lock");
            goto err_screen_lock;
        }

        // Message sending
        StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char));
        if (!StateQueue) {
            PRINT_ERROR("Could not open state queue");
            goto err_state_queue;
        }

        if (xTaskCreate(basicSequentialStateMachine, "StateMachine",
                        mainGENERIC_STACK_SIZE * 2, NULL,
                        configMAX_PRIORITIES - 1, StateMachine) != pdPASS) {
            PRINT_TASK_ERROR("StateMachine");
            goto err_statemachine;
        }

        if (xTaskCreate(vSwapBuffers, "BufferSwapTask",
                        mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES,
                        BufferSwap) != pdPASS) {
            PRINT_TASK_ERROR("BufferSwapTask");
            goto err_bufferswap;
        }

        /** Tasks */
        if (xTaskCreate(vTaskEx2, "Task2", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY, &TaskEx2) != pdPASS) {
            PRINT_TASK_ERROR("Task2");
            goto err_taskex2;
        }

        if (xTaskCreate(vTaskEx3, "Task3", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY, &TaskEx3) != pdPASS) {
            PRINT_TASK_ERROR("Task3");
            goto err_taskex3;
        }

        if (xTaskCreate(vTaskBlink1, "Blink1", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY + 3, &TaskBlink1) != pdPASS) {
            PRINT_TASK_ERROR("Blink1");
            goto err_blink1;
        }

        circle_1hz.lock = xSemaphoreCreateMutex();
        if (!circle_1hz.lock) {
                PRINT_ERROR("Failed to create circle 1 lock");
                goto err_circle_1;
        }

        TaskBlink2 = xTaskCreateStatic(vTaskBlink2,"Blink2", STACK_SIZE, NULL,
                                        mainGENERIC_PRIORITY + 4, xStack, &xTaskBuffer);
        if (!TaskBlink2) {
                PRINT_ERROR("Failed to create circle 2 task");
                goto err_blink_2;
        }


        circle_2hz.lock = xSemaphoreCreateMutex();
        if (!circle_2hz.lock) {
                PRINT_ERROR("Failed to create circle 2 lock");
                goto err_circle_2;
        }

        if (xTaskCreate(vTaskSynch1, "Synch1", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY + 2, &TaskSynch1) != pdPASS) {
            PRINT_TASK_ERROR("Synch1");
            goto err_synch1;
        }

        if (xTaskCreate(vTaskSynch2, "Synch2", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY + 2, &TaskSynch2) != pdPASS) {
            PRINT_TASK_ERROR("Synch2");
            goto err_synch2;
        }

        SynchTwoBinary = xSemaphoreCreateBinary(); // Screen buffer locking
        if (!SynchTwoBinary) {
            PRINT_ERROR("Failed to create synch 2 signal");
            goto err_synch2_binary;
        }

        SynchOneQueue = xQueueCreate(1, sizeof(unsigned char));
        if (!SynchOneQueue) {
            PRINT_ERROR("Could not open synch 1 queue");
            goto err_synch1_queue;
        }

        SynchTwoQueue = xQueueCreate(1, sizeof(unsigned char));
        if (!SynchTwoQueue) {
            PRINT_ERROR("Could not open synch 2 queue");
            goto err_synch2_queue;
        }

        SynchOnePress.lock = xSemaphoreCreateMutex();
        if (!SynchOnePress.lock) {
            PRINT_ERROR("Failed to create synch one lock");
            goto err_synch1_press;
        }

        SynchTwoPress.lock = xSemaphoreCreateMutex();
        if (!SynchTwoPress.lock) {
            PRINT_ERROR("Failed to create synch two lock");
            goto err_synch2_press;
        }

        if (xTaskCreate(vTaskResetTimer, "Reset Timer", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY + 5, &TaskResetTimer) != pdPASS) {
            PRINT_TASK_ERROR("Reset Timer");
            goto err_resettimer;
        }

        ResetTimerQueue = xQueueCreate(1, sizeof(int));
        if (!ResetTimerQueue) {
            PRINT_ERROR("Could not open reset timer queue");
            goto err_resettimer_queue;
        }

        if (xTaskCreate(vTaskStopResume, "Stop/Resume Count", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY + 5, &TaskStopResume) != pdPASS) {
            PRINT_TASK_ERROR("Stop/Resume Count");
            goto err_stopresume;
        }

        StopResumeQueue = xQueueCreate(1, sizeof(unsigned char));
        if (!StopResumeQueue) {
            PRINT_ERROR("Could not open stop/resume queue");
            goto err_stopresume_queue;
        }

        if (xTaskCreate(vTaskEx4, "TaskEx4", mainGENERIC_STACK_SIZE * 2,
                        NULL, mainGENERIC_PRIORITY + 6, &TaskEx4) != pdPASS) {
            PRINT_TASK_ERROR("Stop/Resume Count");
            goto err_taskex4;
        }

        TicksQueue = xQueueCreate(100, sizeof(tick_ex4_t));
        if (!TicksQueue) {
            PRINT_ERROR("Could not open ticks queue");
            goto err_ticksqueue;
        }

        TaskTicksBinary = xSemaphoreCreateBinary(); // Screen buffer locking
        if (!TaskTicksBinary) {
            PRINT_ERROR("Failed to create task ticks binary");
            goto err_ticks_binary;
        }

        ticks_message.lock = xSemaphoreCreateMutex(); // Screen buffer locking
        if (!ticks_message.lock) {
            PRINT_ERROR("Failed to create ticks message lock");
            goto err_ticks_lock;
        }

        vTaskSuspend(TaskEx2);
        vTaskSuspend(TaskEx3);
        vTaskSuspend(TaskEx4);
        vTaskSuspend(TaskBlink1);
        vTaskSuspend(TaskBlink2);
        vTaskSuspend(TaskSynch1);
        vTaskSuspend(TaskSynch2);
        vTaskSuspend(TaskResetTimer);
        vTaskSuspend(TaskStopResume);

        tumFUtilPrintTaskStateList();

        vTaskStartScheduler();

        return EXIT_SUCCESS;

err_ticks_lock:
        vSemaphoreDelete(TaskTicksBinary);
err_ticks_binary:
        vQueueDelete(TicksQueue);
err_ticksqueue:
        vTaskDelete(TaskEx4);
err_taskex4:
        vQueueDelete(StopResumeQueue);
err_stopresume_queue:
        vTaskDelete(TaskStopResume);
err_stopresume:
        vQueueDelete(ResetTimerQueue);
err_resettimer_queue:
        vSemaphoreDelete(SynchOnePress.lock);
err_resettimer:
        vSemaphoreDelete(SynchTwoPress.lock);
err_synch2_press:
        vQueueDelete(SynchTwoQueue);
err_synch1_press:
        vQueueDelete(SynchTwoQueue);
err_synch2_queue:
        vQueueDelete(SynchOneQueue);
err_synch1_queue:
        vSemaphoreDelete(SynchTwoBinary);
err_synch2_binary:
        vTaskDelete(TaskSynch2);
err_synch2:
        vTaskDelete(TaskSynch1);
err_synch1:
        vSemaphoreDelete(circle_2hz.lock);
err_circle_2:
        vTaskDelete(TaskBlink2);
err_blink_2:
        vSemaphoreDelete(circle_1hz.lock);
err_circle_1:
        vTaskDelete(TaskBlink1);
err_blink1:
        vTaskDelete(TaskEx3);
err_taskex3:
        vTaskDelete(TaskEx2);
err_taskex2:
        vTaskDelete(BufferSwap);
err_bufferswap:
        vTaskDelete(StateMachine);
err_statemachine:
        vQueueDelete(StateQueue);
err_state_queue:
        vSemaphoreDelete(ScreenLock);
err_screen_lock:
        vSemaphoreDelete(DrawSignal);
err_draw_signal:
        vSemaphoreDelete(buttons.lock);
err_buttons_lock:
err_init_safe_print:
        tumSoundExit();
err_init_audio:
        tumEventExit();
err_init_events:
        tumDrawExit();
err_init_drawing:
        safePrintExit();
        return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
