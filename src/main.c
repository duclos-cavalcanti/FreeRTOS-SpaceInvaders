#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"

#include "AsyncIO.h"

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

#define STATE_DEBOUNCE_DELAY 1000

#define STATE_ONE 0
#define STATE_TWO 1
#define STATE_THREE 2

#define STATE_COUNT 2
#define STATE_Q_LEN 1
#define BEGIN STATE_ONE

#define NEXT_TASK 0
#define PREV_TASK 1

static TaskHandle_t MainMenu = NULL;
static TaskHandle_t SwapBuffers = NULL;
static TaskHandle_t StateMachine = NULL;

static QueueHandle_t StateQueue = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;

const unsigned char nextstate_signal=NEXT_TASK; //0
const unsigned char prevstate_signal=PREV_TASK; //1

typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };

void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

void checkDraw(unsigned char status, const char *msg)
{
    if (status) {
        if (msg)
            fprintf(stderr, "[ERROR] %s, %s\n", msg,
                    tumGetErrorMessage());
        else {
            fprintf(stderr, "[ERROR] %s\n", tumGetErrorMessage());
        }
    }
}

void vDrawFPS(void)
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

    xLastWakeTime = xTaskGetTickCount();

    if (prevWakeTime != xLastWakeTime) {
        periods[index] =
            configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
        prevWakeTime = xLastWakeTime;
    }
    else {
        periods[index] = 0;
    }

    periods_total += periods[index];

    if (index == (FPS_AVERAGE_COUNT - 1)) {
        index = 0;
    }
    else {
        index++;
    }

    if (average_count < FPS_AVERAGE_COUNT) {
        average_count++;
    }
    else {
        periods_total -= periods[index];
    }

    fps = periods_total / average_count;

    tumFontSelectFontFromName(FPS_FONT);

    sprintf(str, "FPS: %2d", fps);

    if (!tumGetTextSize((char *)str, &text_width, NULL))
        checkDraw(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Green),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
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
            tumEventFetchEvents(FETCH_EVENT_NONBLOCK); // Query events backend for new events, ie. button presses
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}

void ChangeMyState(volatile unsigned char* state, unsigned char forwards)
{
    switch (forwards){

        case NEXT_TASK: 
            if(*state == STATE_COUNT){ //If its at the limit
                *state = 0; //Reset
            }
            else (*state)++; //Increment
            break;

        case PREV_TASK:
            if ((*state)==0){
                *state = STATE_COUNT; //Reset
            }
            else (*state)--; //Decrement
            break;

        default:
            break;

    }

}


void vCheckSM_Input()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
            if (buttons.buttons[KEYCODE(E)]){
                    buttons.buttons[KEYCODE(E)]=0; //reset its value (Debouncing)
                    if(StateQueue){
                        xSemaphoreGive(buttons.lock);
                        xQueueSend(StateQueue,&nextstate_signal, 0);
                    }
                xSemaphoreGive(buttons.lock);
            }
            xSemaphoreGive(buttons.lock);
    }
}

void vStateMachine(void *pvParameters){
    unsigned char current_state = BEGIN;
    unsigned char changed_state = 1;

    unsigned char input = 0;
    TickType_t Last_Change = 0;

    const int state_change_period = STATE_DEBOUNCE_DELAY;

    while(1){
        Last_Change=xTaskGetTickCount();
        
        if (changed_state) goto initial_state;

        if (StateQueue){
            if (xQueueReceive(StateQueue,&input,portMAX_DELAY)==pdTRUE){
                    if(xTaskGetTickCount() - Last_Change > state_change_period){
                        ChangeMyState(&current_state, input);
                        changed_state=1;
                        Last_Change=xTaskGetTickCount();
                    }
            }
        }


        initial_state:
            if (changed_state){
                switch (current_state){
                    case STATE_ONE: //The case when the Machine is just started
                        if(MainMenu){
                            vTaskResume(MainMenu); 
                        }
                        break;

                    case STATE_TWO:
                        break;
                    case STATE_THREE:
                        break;

                    default:
                        break;
                }
                changed_state=0; //Resets changed state
            }
        }
}

void vMainMenu(void *pvParameters)
{
    char our_time_string[100];
    static int  our_time_strings_width=0;

    while (1) {
        xGetButtonInput();
        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
            if (buttons.buttons[KEYCODE(Q)]) { // Equiv to SDL_SCANCODE_Q
                exit(EXIT_SUCCESS);
            }
            xSemaphoreGive(buttons.lock);
        }
        xSemaphoreTake(ScreenLock,portMAX_DELAY);
        
            tumDrawClear(Black); // Clear screen


            // Format our string into our char array
            sprintf(our_time_string,
                    "Testing");

            if (!tumGetTextSize((char *)our_time_string,
                                &our_time_strings_width, NULL))
                tumDrawText(our_time_string,
                            SCREEN_WIDTH / 2 -
                            our_time_strings_width / 2,
                            SCREEN_HEIGHT / 2 - DEFAULT_FONT_SIZE / 2,
                            White);
            vCheckSM_Input();
            vDrawFPS();


        xSemaphoreGive(ScreenLock);
        vTaskDelay((TickType_t)20);
    }
}

int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

    printf("Initializing: ");

    if (tumDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize drawing");
        goto err_init_drawing;
    }

    if (tumEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }

    if (tumSoundInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize audio");
        goto err_init_audio;
    }

    buttons.lock = xSemaphoreCreateMutex(); // Locking mechanism
    if (!buttons.lock) {
        PRINT_ERROR("Failed to create buttons lock");
        goto err_buttons_lock;
    }

    if (xTaskCreate(vSwapBuffers, "SwapBuffers", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES, &SwapBuffers) != pdPASS) {
        goto err_swapbuffers;
    }
    
    ScreenLock = xSemaphoreCreateMutex(); //Screen Locking for Blinking Objects
    if (!ScreenLock) {
        PRINT_ERROR("Failed to create Screen Lock");
        goto err_screenlock;
    }

    DrawSignal = xSemaphoreCreateBinary(); //Screen Locking for Blinking Objects
    if (!DrawSignal) {
        PRINT_ERROR("Failed to create draw signal");
        goto err_drawsignal;
    }
    
    StateQueue = xQueueCreate(STATE_Q_LEN, sizeof(unsigned char)); 
    if(!StateQueue){
        PRINT_ERROR("Could not open State Queue.");
        goto err_StateQueue;
    }
    if (xTaskCreate(vStateMachine, "StateMachine", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-1, &StateMachine) != pdPASS) {
        goto err_StateMachine;
    }
    
    if (xTaskCreate(vMainMenu, "MainMenu", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-5, &MainMenu) != pdPASS) {
        goto err_mainmenu;
    }

    vTaskSuspend(MainMenu);

    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_mainmenu:
    vTaskDelete(StateMachine);
err_StateMachine:
    vQueueDelete(StateQueue);
err_StateQueue:
    vSemaphoreDelete(DrawSignal);
err_drawsignal:
    vSemaphoreDelete(ScreenLock);
err_screenlock:
    vSemaphoreDelete(buttons.lock);
err_swapbuffers:
    vTaskDelete(SwapBuffers);
err_buttons_lock:
    tumSoundExit();
err_init_audio:
    tumEventExit();
err_init_events:
    tumDrawExit();
err_init_drawing:
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
