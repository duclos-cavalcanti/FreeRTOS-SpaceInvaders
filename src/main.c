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

#include "ship.h"

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

#define SHIPSIZE 20
#define SHIPSPEED 2

#define WALLTHICKNESS 3
#define WALLPOSITION SCREEN_HEIGHT*95/100

static TaskHandle_t MainMenu = NULL;
static TaskHandle_t IntroGame = NULL;
static TaskHandle_t ShipBulletTask = NULL;

static TaskHandle_t SwapBuffers = NULL;
static TaskHandle_t StateMachine = NULL;

static QueueHandle_t StateQueue = NULL;
static QueueHandle_t ShipBulletQueue = NULL;

static image_handle_t TitleScreen = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;

const unsigned char nextstate_signal=NEXT_TASK; //0
const unsigned char prevstate_signal=PREV_TASK; //1

typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };

typedef struct ShipBuffer_t{
    ship_t* Ship;
    SemaphoreHandle_t lock;
}ShipBuffer_t;

static ShipBuffer_t ShipBuffer = { 0 };

typedef struct PlayerBuffer_t{
    unsigned short LivesLeft;
    unsigned short Level;
    SemaphoreHandle_t lock;
}PlayerBuffer_t;

PlayerBuffer_t PlayerInfoBuffer = { 0 };

void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

void xCheckQuit(void)
{
    xGetButtonInput();
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE){
            if (buttons.buttons[KEYCODE(Q)]){
            exit(EXIT_SUCCESS);
        }              
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
                              White),
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
            tumEventFetchEvents(FETCH_EVENT_NONBLOCK);
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
                *state = BEGIN; //Reset
            }
            else (*state)++; //Increment
            break;

        case PREV_TASK:
            if ((*state)==0){
                *state = STATE_COUNT-1; //Reset
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
                    case STATE_ONE: // Begin 
                        if(MainMenu) vTaskResume(MainMenu);
                        if(IntroGame) vTaskSuspend(IntroGame);
                        break;
                    case STATE_TWO:
                        if(MainMenu) vTaskSuspend(MainMenu);
                        if(IntroGame) vTaskResume(IntroGame);
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

void vDrawStaticTexts(void)
{
    char Score_1[100];   
    char Score_2[100];   
    char Hi_Score[100];   
    
    static int Score1_Width,Score2_Width,HiScoreWidth=0;

    sprintf(Score_1,
            "SCORE<1>");
    sprintf(Score_2,
            "SCORE<2>");
    sprintf(Hi_Score,
            "HI-SCORE");
    
    if(!tumGetTextSize((char *)Score_1,&Score1_Width, NULL)){
                    checkDraw(tumDrawText(Score_1,
                                            SCREEN_WIDTH*1/8-Score1_Width/2,SCREEN_HEIGHT*1/25-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }
    
    if(!tumGetTextSize((char *)Score_2,&Score2_Width, NULL)){
                    checkDraw(tumDrawText(Score_2,
                                            SCREEN_WIDTH*7/8-Score2_Width/2,SCREEN_HEIGHT*1/25-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }
    if(!tumGetTextSize((char *)Hi_Score,&HiScoreWidth, NULL)){
                    checkDraw(tumDrawText(Hi_Score,
                                            SCREEN_WIDTH*2/4-HiScoreWidth/2,SCREEN_HEIGHT*1/25-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }

}
void vDrawMainMenu(void)
{
    char PlayerOptions[100];
    static int PlayerOptionsWidth=0;


    sprintf(PlayerOptions,"< 1 or 2 PLAYERS >");
    if(!tumGetTextSize((char *)PlayerOptions,&PlayerOptionsWidth, NULL)){
                    checkDraw(tumDrawText(PlayerOptions,
                                            SCREEN_WIDTH*2/4-PlayerOptionsWidth/2,SCREEN_HEIGHT*8/10-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
    }

    checkDraw(tumDrawLoadedImage(TitleScreen,
                                    SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(TitleScreen)/2,SCREEN_HEIGHT*4/10-tumDrawGetLoadedImageHeight(TitleScreen)/2),
                                    __FUNCTION__);
}

void vTaskMainMenu(void *pvParameters)
{
    TitleScreen=tumDrawLoadImage("../resources/titlescreen.bmp");

    while (1) {
        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xGetButtonInput();
           
                xSemaphoreTake(ScreenLock,portMAX_DELAY);
                tumDrawClear(Black); 
                vDrawStaticTexts();
                vDrawMainMenu();
                
                
                vDrawFPS();
                xSemaphoreGive(ScreenLock);
                xCheckQuit();
                vCheckSM_Input();
                }
        }
}
void vDrawLevel()
{
    static char str[100] = { 0 };
    static int strWidth = { 0 };
    if(xSemaphoreTake(PlayerInfoBuffer.lock,portMAX_DELAY)==pdTRUE){
        sprintf(str,"Level [ %d  ]", PlayerInfoBuffer.Level);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
    if(!tumGetTextSize((char*)str, &strWidth, NULL))
        checkDraw(tumDrawText(str,120- strWidth/2,
                              SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                              White),
                              __FUNCTION__);
}
void vDrawLives()
{   
    static char str[100] = { 0 };
    static int strWidth;
    
    if(xSemaphoreTake(PlayerInfoBuffer.lock,portMAX_DELAY)==pdTRUE){
        sprintf(str,"Lives: [ %d ]", PlayerInfoBuffer.LivesLeft);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }

    if(!tumGetTextSize((char*)str, &strWidth, NULL))
        checkDraw(tumDrawText(str, 35 - strWidth/2, 
                              SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                              White),
                              __FUNCTION__);
}
void vDrawLowerWall()
{
    checkDraw(tumDrawLine(0, WALLPOSITION,
                          SCREEN_WIDTH, WALLPOSITION,
                          WALLTHICKNESS,Green),
                          __FUNCTION__);
}
void vDrawShip()
{
    if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){
        checkDraw(tumDrawCircle(ShipBuffer.Ship->x_pos,ShipBuffer.Ship->y_pos,
                                ShipBuffer.Ship->radius,ShipBuffer.Ship->color),
                                __FUNCTION__);
        xSemaphoreGive(ShipBuffer.lock);
    }
}
unsigned char xCheckShipShoot()
{
    xGetButtonInput();
    if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {
        if (buttons.buttons[KEYCODE(SPACE)]) {
            if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
                xSemaphoreGive(ShipBuffer.lock);
                xSemaphoreGive(buttons.lock);
                return 1;
            }
            xSemaphoreGive(buttons.lock);
        }
        xSemaphoreGive(buttons.lock);
    }
    return 0;    
}
unsigned char xCheckShipMoved(void)
{
    xGetButtonInput();
    if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {
        if (buttons.buttons[KEYCODE(LEFT)]) {
            if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
                vIncrementShipLeft(ShipBuffer.Ship);
                xSemaphoreGive(ShipBuffer.lock);
                xSemaphoreGive(buttons.lock);
                return 1;
            }
            xSemaphoreGive(buttons.lock);
        }                            
        if (buttons.buttons[KEYCODE(RIGHT)]) { 
            if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
                vIncrementShipRight(ShipBuffer.Ship); 
                xSemaphoreGive(ShipBuffer.lock);
                xSemaphoreGive(buttons.lock);                                                                                                                                                                
                return 1;
            }
            xSemaphoreGive(buttons.lock);                                                                                                                                                                
        }   
        xSemaphoreGive(buttons.lock);
    }   
    return 0;
}

void vTaskShipBulletControl(void *pvParameters)
{

    while(1){
        uint32_t BulletLaunchSignal;

        if(xTaskNotifyWait(0x00, 0xffffffff, &BulletLaunchSignal, portMAX_DELAY) == pdTRUE){
            vUpdateShipBulletPos(ShipBuffer.Ship);
            if (xCheckShipBulletStatus(ShipBuffer.Ship))
                ShipBuffer.Ship->bullet->BulletAliveFlag=0;
        }
    }

}
void vTaskIntroGame(void *pvParameters)
{
    ShipBuffer.Ship = CreateShip(SCREEN_WIDTH/2,SCREEN_HEIGHT*88/100,SHIPSPEED,
                                Green, SHIPSIZE);
    PlayerInfoBuffer.LivesLeft = 3;
    PlayerInfoBuffer.Level = 1;    
    static unsigned char BulletOnScreenFlag = 0;

    while(1){
        xCheckShipMoved();
        if(xCheckShipShoot() && BulletOnScreenFlag==0)
            if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
                CreateBullet(ShipBuffer.Ship);
                xSemaphoreGive(ShipBuffer.lock);
                BulletOnScreenFlag = 1;
            }
        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xSemaphoreTake(ScreenLock,portMAX_DELAY);

                    tumDrawClear(Black);
                    vDrawStaticTexts();
                    vDrawShip();                    
                    if(BulletOnScreenFlag) {
                        if(xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY)==pdTRUE){
                            if(ShipBuffer.Ship->bullet->BulletAliveFlag){
                                vDrawShipBullet(ShipBuffer.Ship);
                                vTaskResume(ShipBulletTask);
                                xTaskNotify(ShipBulletTask, 0x01, eSetValueWithOverwrite);
                                xSemaphoreGive(ShipBuffer.lock);
                            }
                            else BulletOnScreenFlag=0;
                            xSemaphoreGive(ShipBuffer.lock);
                        }
                        xSemaphoreGive(ShipBuffer.lock);
                    }
                    vDrawLowerWall();
                    vDrawLevel();
                    vDrawLives();
                    vDrawFPS();    

                xSemaphoreGive(ScreenLock);
                xCheckQuit();
            }
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
                    configMAX_PRIORITIES-2, &StateMachine) != pdPASS) {
        goto err_StateMachine;
    }
    
    if (xTaskCreate(vTaskMainMenu, "MainMenu", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-4, &MainMenu) != pdPASS) {
        goto err_mainmenu;
    }

    if (xTaskCreate(vTaskIntroGame, "IntroGame", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-4, &IntroGame) != pdPASS) {
        goto err_IntroGame;
    }
    
    ShipBuffer.lock = xSemaphoreCreateMutex();
    if(!ShipBuffer.lock){
        PRINT_ERROR("Failed to create Ship buffer lock.");
        goto err_shipbuffer;
    }
    
    PlayerInfoBuffer.lock = xSemaphoreCreateMutex();
    if(!ShipBuffer.lock){
        PRINT_ERROR("Failed to create Player info buffer lock.");
        goto err_playerinfo;
    }

    if (xTaskCreate(vTaskShipBulletControl, "ShipBulletTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES - 5, &ShipBulletTask)!= pdPASS){
        PRINT_ERROR("Failed to create ShipBulletTask.");
        goto err_shipbulletcontrol;
    }

    ShipBulletQueue = xQueueCreate(STATE_Q_LEN, sizeof(unsigned char));
    if (!ShipBulletQueue){
        PRINT_ERROR("Could not open ShipBulletQ.");
        goto err_shipbulletqueue;
    }

    vTaskSuspend(MainMenu);
    vTaskSuspend(IntroGame);
    vTaskSuspend(ShipBulletTask);

    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_shipbulletqueue:
    vTaskDelete(ShipBulletTask);
err_shipbulletcontrol:
    vSemaphoreDelete(PlayerInfoBuffer.lock);
err_playerinfo:
    vSemaphoreDelete(ShipBuffer.lock);
err_shipbuffer:
    vTaskDelete(IntroGame);
err_IntroGame:
    vTaskDelete(MainMenu);
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
