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

#include "utilities.h"
#include "main.h"
#include "ship.h"
#include "bunkers.h"
#include "saucer.h"
#include "creatures.h"

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

static TaskHandle_t MainMenuTask = NULL;
static TaskHandle_t MainPlayingGameTask = NULL;
static TaskHandle_t PausedGameTask = NULL;
static TaskHandle_t GameOverTask = NULL;
static TaskHandle_t NextLevelTask = NULL;
static TaskHandle_t ShipBulletControlTask = NULL;
static TaskHandle_t BunkerShotControlTask = NULL;
static TaskHandle_t BunkerCreaturesCrashedTask = NULL;
static TaskHandle_t CreaturesShotControlTask = NULL;
static TaskHandle_t CreaturesActionControlTask = NULL;
static TaskHandle_t CreaturesBulletControlTask = NULL;
static TaskHandle_t ShipShotControlTask = NULL;
static TaskHandle_t SaucerActionControlTask = NULL;
static TaskHandle_t SaucerShotControlTask = NULL;

static TaskHandle_t SwapBuffers = NULL;
static TaskHandle_t StateMachine = NULL;

#define STATE_DEBOUNCE_DELAY 250
#define STATE_QUEUE_LENGTH 1
static QueueHandle_t StateQueue = NULL;

static image_handle_t TitleScreen = NULL;
static image_handle_t GameOver = NULL;
static image_handle_t NextLevel = NULL;
static image_handle_t PlayerShip = NULL;
static image_handle_t CreatureMEDIUM_0 = NULL;
static image_handle_t CreatureMEDIUM_1 = NULL;
static image_handle_t CreatureEASY_0 = NULL;
static image_handle_t CreatureEASY_1 = NULL;
static image_handle_t CreatureHARD_0 = NULL;
static image_handle_t CreatureHARD_1 = NULL;

static image_handle_t SaucerBoss = NULL;
static image_handle_t CreaturesEasy_MenuScaled_0 = NULL;
static image_handle_t CreaturesEasy_MenuScaled_1 = NULL;
static image_handle_t CreaturesMedium_MenuScaled_0 = NULL;
static image_handle_t CreaturesMedium_MenuScaled_1 = NULL;
static image_handle_t CreaturesHard_MenuScaled_0 = NULL;
static image_handle_t CreaturesHard_MenuScaled_1 = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;

const unsigned char MainMenuStateSignal=MainMenuState;
const unsigned char PlayingStateSignal=PlayingState;;
const unsigned char PausedGameStateSignal=PausedState;
const unsigned char GameOverStateSignal=GameOverState;
const unsigned char NextLevelStateSignal=NextLevelState;
const unsigned char ResetGameStateSignal=ResetGameState;

typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;
static buttons_buffer_t buttons = { 0 };

typedef struct GameStateBuffer_t{
    GameState_t GameState;
    SemaphoreHandle_t lock; 
}GameStateBuffer_t;
static GameStateBuffer_t GameStateBuffer;

typedef struct MainMenuInfoBuffer_t{
    SelectedMenuOption_t SelectedMenuOption; 
    image_handle_t CreaturesEasyImages[2];
    image_handle_t CreaturesMediumImages[2];
    image_handle_t CreaturesHardImages[2];
    SemaphoreHandle_t lock;
}MainMenuInfoBuffer_t;
static MainMenuInfoBuffer_t MainMenuInfoBuffer = { 0 };

typedef struct OutsideGameActionsBuffer_t{
    PlayerOutsideGameActions_t PlayerOutsideGameActions; 
    SemaphoreHandle_t lock;
}OutsideGameActionsBuffer_t;
static OutsideGameActionsBuffer_t OutsideGameActionsBuffer = { 0 };

typedef struct ShipBuffer_t{
    ship_t* Ship;
    unsigned char BulletCrashed;
    SemaphoreHandle_t lock;
}ShipBuffer_t;
static ShipBuffer_t ShipBuffer = { 0 };

typedef struct SaucerBuffer_t{
    saucer_t* saucer;
    H_Movement_t Direction;
    unsigned char SaucerHitFlag;
    unsigned char SaucerAppearsFlag;
    SemaphoreHandle_t lock;
}SaucerBuffer_t;
static SaucerBuffer_t SaucerBuffer = { 0 };

typedef struct PlayerBuffer_t{
    unsigned short LivesLeft;
    unsigned short Level;
    unsigned short HiScore;
    unsigned short Score;
    unsigned short FreshGame;
    SemaphoreHandle_t lock;
}PlayerBuffer_t;
static PlayerBuffer_t PlayerInfoBuffer = { 0 };

typedef struct BunkersBuffer_t{
    bunkers_t* Bunkers;
    SemaphoreHandle_t lock;
}BunkersBuffer_t;
static BunkersBuffer_t BunkersBuffer = { 0 };

typedef struct CreaturesBuffer_t{
    creature_t* Creatures;
    signed short* FrontierCreaturesID;
    bullet_t CreaturesBullet;
    unsigned short BulletAliveFlag;
    unsigned short NumbOfAliveCreatures;   
    H_Movement_t HorizontalDirection;

    image_handle_t ImagesCatalog[6];
    SemaphoreHandle_t lock;
}CreaturesBuffer_t;
static CreaturesBuffer_t CreaturesBuffer = { 0 };

typedef struct LevelModifiersBuffer_t{
    unsigned short NumberOfSpeedChanges;
    unsigned short SpeedChangeCount;
    TickType_t ShootingPeriod;
    TickType_t MovingPeriod;
    TickType_t AnimationPeriod;
    SemaphoreHandle_t lock;
}LevelModifiersBuffer_t;
static LevelModifiersBuffer_t LevelModifiersBuffer = { 0 };

typedef struct PausedGameInfoBuffer_t{
    PausedGameActions_t PausedGameActions;
    SemaphoreHandle_t lock;
}PausedGameInfoBuffer_t;
static PausedGameInfoBuffer_t PausedGameInfoBuffer = { 0 };

typedef struct GameOverInfoBuffer_t{
    SelectedGameOverOption_t SelectedGameOverOption;
    SemaphoreHandle_t lock;
}GameOverInfoBuffer_t;
static GameOverInfoBuffer_t GameOverInfoBuffer = { 0 };

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
                              White),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}
void vPlayShipShotSound()
{
    tumSoundPlaySample(c3);
}
void vPlayBulletWallSound()
{
    tumSoundPlaySample(d5);
}
void vPlayDeadCreatureSound()
{
    tumSoundPlaySample(g5);
}
void vPlayBunkerShotSound()
{
    tumSoundPlaySample(f5);
}
void vPlayBulletSound()
{
    tumSoundPlaySample(a3);
}

void vSetMainMenuBufferValues()
{
    xSemaphoreTake(MainMenuInfoBuffer.lock, portMAX_DELAY);
        MainMenuInfoBuffer.CreaturesEasyImages[0] = CreaturesEasy_MenuScaled_0;
        MainMenuInfoBuffer.CreaturesEasyImages[1] = CreaturesEasy_MenuScaled_1;
        MainMenuInfoBuffer.CreaturesMediumImages[0] = CreaturesMedium_MenuScaled_0;
        MainMenuInfoBuffer.CreaturesMediumImages[1] = CreaturesMedium_MenuScaled_1;
        MainMenuInfoBuffer.CreaturesHardImages[0] = CreaturesHard_MenuScaled_0;
        MainMenuInfoBuffer.CreaturesHardImages[1] = CreaturesHard_MenuScaled_1;

        MainMenuInfoBuffer.SelectedMenuOption=SinglePlayer;

    xSemaphoreGive(MainMenuInfoBuffer.lock);
}
void vSetMainMenuLoadedImages()
{
    TitleScreen=tumDrawLoadImage("../resources/TitleScreen.bmp");
    CreaturesEasy_MenuScaled_0 = tumDrawLoadImage("../resources/creature_E_0.bmp");
    CreaturesEasy_MenuScaled_1 = tumDrawLoadImage("../resources/creature_E_1.bmp");
    CreaturesMedium_MenuScaled_0 = tumDrawLoadImage("../resources/creature_M_0.bmp");
    CreaturesMedium_MenuScaled_1 = tumDrawLoadImage("../resources/creature_M_1.bmp");
    CreaturesHard_MenuScaled_0 = tumDrawLoadImage("../resources/creature_H_0.bmp");
    CreaturesHard_MenuScaled_1 = tumDrawLoadImage("../resources/creature_H_1.bmp");
    SaucerBoss = tumDrawLoadImage("../resources/saucer.bmp");
}
void vSetPlayersInfoBufferValues()
{
    xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY);

        PlayerInfoBuffer.HiScore=0;
        PlayerInfoBuffer.Score=0;
        PlayerInfoBuffer.LivesLeft = 3;
        PlayerInfoBuffer.Level = 1;    
        PlayerInfoBuffer.FreshGame=1;

    xSemaphoreGive(PlayerInfoBuffer.lock);
}
void vSetOutsideGameActionsBufferValues()
{
    xSemaphoreTake(OutsideGameActionsBuffer.lock,portMAX_DELAY);

        OutsideGameActionsBuffer.PlayerOutsideGameActions=NoAction;

    xSemaphoreGive(OutsideGameActionsBuffer.lock);
}
void vSetShipsBufferValues()
{
    xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY);

        ShipBuffer.Ship = CreateShip(SCREEN_WIDTH/2,PLAYERSHIP_Y_BEGIN,SHIPSPEED,
                                    Green, SHIPSIZE);
        ShipBuffer.BulletCrashed=0;

    xSemaphoreGive(ShipBuffer.lock);

}
void vSetSaucerBufferValues()
{
    xSemaphoreTake(SaucerBuffer.lock, portMAX_DELAY);

        SaucerBuffer.saucer = CreateSinglePlayerSaucer();
        SaucerBuffer.saucer->Image = SaucerBoss;
        SaucerBuffer.SaucerAppearsFlag=0;
        SaucerBuffer.SaucerHitFlag=0;
        SaucerBuffer.Direction = RIGHT;

    xSemaphoreGive(SaucerBuffer.lock);
}
void vSetCreaturesBufferValues()
{
    CreatureHARD_0 = tumDrawLoadImage("../resources/creature_H_0.bmp");
    CreatureHARD_1 = tumDrawLoadImage("../resources/creature_H_1.bmp");

    CreatureMEDIUM_0=tumDrawLoadImage("../resources/creature_M_0.bmp");
    CreatureMEDIUM_1=tumDrawLoadImage("../resources/creature_M_1.bmp");

    CreatureEASY_0=tumDrawLoadImage("../resources/creature_E_0.bmp");
    CreatureEASY_1=tumDrawLoadImage("../resources/creature_E_1.bmp");

    xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY);

        CreaturesBuffer.ImagesCatalog[0]=CreatureEASY_0;
        CreaturesBuffer.ImagesCatalog[1]=CreatureEASY_1;
        CreaturesBuffer.ImagesCatalog[2]=CreatureMEDIUM_0;
        CreaturesBuffer.ImagesCatalog[3]=CreatureMEDIUM_1;
        CreaturesBuffer.ImagesCatalog[4]=CreatureHARD_0;
        CreaturesBuffer.ImagesCatalog[5]=CreatureHARD_1;

        CreaturesBuffer.HorizontalDirection = RIGHT;
        CreaturesBuffer.BulletAliveFlag=0;
        CreaturesBuffer.NumbOfAliveCreatures=NUMB_OF_CREATURES;

        CreaturesBuffer.Creatures = CreateCreatures();
        CreaturesBuffer.FrontierCreaturesID = vAssignFrontierCreatures(CreaturesBuffer.Creatures);
        vAssignCreaturesImages(CreaturesBuffer.Creatures,
                               CreaturesBuffer.ImagesCatalog);

    xSemaphoreGive(CreaturesBuffer.lock);
}

void vSetBunkersBufferValues()
{
    xSemaphoreTake(BunkersBuffer.lock, portMAX_DELAY);

        BunkersBuffer.Bunkers = CreateBunkers();

    xSemaphoreGive(BunkersBuffer.lock);
}

void vSetPausedGameInfoBufferValues()
{
    xSemaphoreTake(PausedGameInfoBuffer.lock, portMAX_DELAY);
    
        PausedGameInfoBuffer.PausedGameActions=RemainPaused;

    xSemaphoreGive(PausedGameInfoBuffer.lock);
}

void vSetGameOverInfoBufferValues()
{
    xSemaphoreTake(GameOverInfoBuffer.lock, portMAX_DELAY);

        GameOverInfoBuffer.SelectedGameOverOption = PlayAgain;

    xSemaphoreGive(GameOverInfoBuffer.lock);
}

void vSetLevelModifiersValues()
{
    xSemaphoreTake(LevelModifiersBuffer.lock, portMAX_DELAY);
        xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY);
            LevelModifiersBuffer.NumberOfSpeedChanges = 0;
            LevelModifiersBuffer.SpeedChangeCount = 10 - 2 * (PlayerInfoBuffer.Level - 1);
            LevelModifiersBuffer.MovingPeriod = 700 - 200*(PlayerInfoBuffer.Level - 1);
            LevelModifiersBuffer.AnimationPeriod = LevelModifiersBuffer.MovingPeriod;
            LevelModifiersBuffer.ShootingPeriod = 2000 - 250* (PlayerInfoBuffer.Level - 1);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    xSemaphoreGive(LevelModifiersBuffer.lock);
}

void vPrepareGameValues(unsigned short Level, unsigned short Score)
{ 
    PlayerInfoBuffer.Score=Score;
    PlayerInfoBuffer.Level=Level;
    PlayerInfoBuffer.FreshGame=0;
    PlayerInfoBuffer.LivesLeft=3;
}

void vUpdatePlayerScoreCreatureKilled(unsigned char CreatureID)
{
    static unsigned char AddOn=0;
    AddOn = xFetchCreatureValue(CreatureID);
    PlayerInfoBuffer.Score+=AddOn;
    if(PlayerInfoBuffer.FreshGame==1 || PlayerInfoBuffer.Score > PlayerInfoBuffer.HiScore)
        PlayerInfoBuffer.HiScore=PlayerInfoBuffer.Score;
}

void vDrawStaticTexts(void)
{
    char Score_1[20];   
    char Score_2[20];   
    char Hi_Score[20];   
    
    static int Score1_Width,Score2_Width,HiScoreWidth=0;
    
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){

        sprintf(Score_1,
                "P1-SCORE [ %d ]", PlayerInfoBuffer.Score);
        sprintf(Score_2,
                "P2-SCORE [ 0 ]");
        sprintf(Hi_Score,
                "HI-SCORE [ %d ]", PlayerInfoBuffer.HiScore);

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
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}

void vDrawPointsExplanation(unsigned char ImageAnimationIndex)
{
    char EasyPointsChar[20];   
    static int EasyPointsCharWidth=0;

    char MediumPointsChar[20];   
    static int MediumPointsCharWidth=0;

    char HardPointsChar[20];   
    static int HardPointsCharWidth=0;

    char MysteriousShipChar[20];
    static int MysteriousShipCharWidth=0;

    checkDraw(tumDrawLoadedImage(MainMenuInfoBuffer.CreaturesEasyImages[ImageAnimationIndex],
                                 SCREEN_WIDTH*7/10-tumDrawGetLoadedImageWidth(MainMenuInfoBuffer.CreaturesEasyImages[ImageAnimationIndex])/2,
                                 SCREEN_HEIGHT*6/10-tumDrawGetLoadedImageHeight(MainMenuInfoBuffer.CreaturesEasyImages[ImageAnimationIndex])/2),
                                 __FUNCTION__);

    sprintf(EasyPointsChar,"= 10 Points");
    if(!tumGetTextSize((char *)EasyPointsChar,&EasyPointsCharWidth, NULL)){
                    checkDraw(tumDrawText(EasyPointsChar,
                                            SCREEN_WIDTH*4/5-EasyPointsCharWidth/2,
                                            SCREEN_HEIGHT*6/10-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }

    checkDraw(tumDrawLoadedImage(MainMenuInfoBuffer.CreaturesMediumImages[ImageAnimationIndex],
                                 SCREEN_WIDTH*7/10-tumDrawGetLoadedImageWidth(MainMenuInfoBuffer.CreaturesMediumImages[ImageAnimationIndex])/2,
                                 SCREEN_HEIGHT*7/10-tumDrawGetLoadedImageHeight(MainMenuInfoBuffer.CreaturesMediumImages[ImageAnimationIndex])/2),
                                 __FUNCTION__);

    sprintf(MediumPointsChar,"= 20 Points");
    if(!tumGetTextSize((char *)MediumPointsChar,&MediumPointsCharWidth, NULL)){
                    checkDraw(tumDrawText(MediumPointsChar,
                                            SCREEN_WIDTH*4/5-MediumPointsCharWidth/2,
                                            SCREEN_HEIGHT*7/10-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }

    checkDraw(tumDrawLoadedImage(MainMenuInfoBuffer.CreaturesHardImages[ImageAnimationIndex],
                                 SCREEN_WIDTH*7/10-tumDrawGetLoadedImageWidth(MainMenuInfoBuffer.CreaturesHardImages[ImageAnimationIndex])/2,
                                 SCREEN_HEIGHT*8/10-tumDrawGetLoadedImageHeight(MainMenuInfoBuffer.CreaturesHardImages[ImageAnimationIndex])/2),
                                 __FUNCTION__);

    sprintf(HardPointsChar,"= 30 Points");
    if(!tumGetTextSize((char *)HardPointsChar,&HardPointsCharWidth, NULL)){
                    checkDraw(tumDrawText(HardPointsChar,
                                            SCREEN_WIDTH*4/5-HardPointsCharWidth/2,
                                            SCREEN_HEIGHT*8/10-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }

    checkDraw(tumDrawLoadedImage(SaucerBoss,
                                 SCREEN_WIDTH*7/10-tumDrawGetLoadedImageWidth(SaucerBoss)/2,
                                 SCREEN_HEIGHT*9/10-tumDrawGetLoadedImageHeight(SaucerBoss)/2),
                                 __FUNCTION__);

    sprintf(MysteriousShipChar,"= ?? Points");
    if(!tumGetTextSize((char *)MysteriousShipChar,&MysteriousShipCharWidth, NULL)){
                    checkDraw(tumDrawText(MysteriousShipChar,
                                            SCREEN_WIDTH*4/5-MysteriousShipCharWidth/2,
                                            SCREEN_HEIGHT*9/10-DEFAULT_FONT_SIZE/2,
                                            White),
                                            __FUNCTION__);
                }
}

void vDrawSpaceInvadersBanner()
{
    checkDraw(tumDrawLoadedImage(TitleScreen,
                                 SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(TitleScreen)/2,
                                 SCREEN_HEIGHT*3/10-tumDrawGetLoadedImageHeight(TitleScreen)/2),
                                 __FUNCTION__);
}

void vDrawMainMenuOptions(void)
{
    static char SingleplayerChar[20];
    static char MultiplayerChar[20];

    static int SingleplayerCharWidth=0;
    static int MultiplayerCharWidth=0;   

    static char CheatsChar[20];
    static int CheatsCharWidth=0;

    static char LeaveChar[20];
    static int LeaveCharWidth=0;

    if(xSemaphoreTake(MainMenuInfoBuffer.lock, portMAX_DELAY)==pdTRUE){  

        sprintf(SingleplayerChar,"Singleplayer");
        if(!tumGetTextSize((char *)SingleplayerChar,&SingleplayerCharWidth, NULL)){
                        checkDraw(tumDrawText(SingleplayerChar,
                                              SCREEN_WIDTH*1/4-SingleplayerCharWidth/2,SCREEN_HEIGHT*6/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(MainMenuInfoBuffer.SelectedMenuOption, SinglePlayer)),
                                              __FUNCTION__);
        }

        sprintf(MultiplayerChar,"Multiplayer");
        if(!tumGetTextSize((char *)MultiplayerChar,&MultiplayerCharWidth, NULL)){
                        checkDraw(tumDrawText(MultiplayerChar,
                                              SCREEN_WIDTH*1/4-MultiplayerCharWidth/2,SCREEN_HEIGHT*7/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(MainMenuInfoBuffer.SelectedMenuOption, MultiPlayer)),
                                              __FUNCTION__);
        }

        sprintf(CheatsChar,"Cheats");
        if(!tumGetTextSize((char *)CheatsChar,&CheatsCharWidth, NULL)){
                        checkDraw(tumDrawText(CheatsChar,
                                              SCREEN_WIDTH*1/4-CheatsCharWidth/2,SCREEN_HEIGHT*8/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(MainMenuInfoBuffer.SelectedMenuOption, Cheats)),
                                              __FUNCTION__);
        }

        sprintf(LeaveChar,"Leave");
        if(!tumGetTextSize((char *)LeaveChar,&LeaveCharWidth, NULL)){
                        checkDraw(tumDrawText(LeaveChar,
                                              SCREEN_WIDTH*1/4-LeaveCharWidth/2,SCREEN_HEIGHT*9/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(MainMenuInfoBuffer.SelectedMenuOption, Leave)),
                                              __FUNCTION__);
        }

        xSemaphoreGive(MainMenuInfoBuffer.lock);
    }
}

unsigned char xCheckEnterPressed()
{
    if(xSemaphoreTake(buttons.lock, portMAX_DELAY)==pdTRUE){
        if(buttons.buttons[KEYCODE(RETURN)]){
            xSemaphoreGive(buttons.lock);
            return 1;
        }
        xSemaphoreGive(buttons.lock);
        return 0;
    }
    return 0;
}
void  xCheckMenuSelectionChange(unsigned char* UP_DEBOUNCE_STATE, 
                                unsigned char* DOWN_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {

        if (buttons.buttons[KEYCODE(DOWN)]) {
            if(buttons.buttons[KEYCODE(DOWN)]!=(*DOWN_DEBOUNCE_STATE))
                if(xSemaphoreTake(MainMenuInfoBuffer.lock,portMAX_DELAY)==pdTRUE){  
                    vDownMenuSelection(&MainMenuInfoBuffer.SelectedMenuOption);
                    xSemaphoreGive(MainMenuInfoBuffer.lock);
                }
        }                            
        else if (buttons.buttons[KEYCODE(UP)]) { 
            if(buttons.buttons[KEYCODE(UP)]!=(*UP_DEBOUNCE_STATE))
                if(xSemaphoreTake(MainMenuInfoBuffer.lock,portMAX_DELAY)==pdTRUE){  
                    vUpMenuSelection(&MainMenuInfoBuffer.SelectedMenuOption);
                    xSemaphoreGive(MainMenuInfoBuffer.lock);
                }
        }   

        (*UP_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(UP)];
        (*DOWN_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(DOWN)];
        xSemaphoreGive(buttons.lock);                                                                                                                                                                
    }   
}
void vTaskMainMenu(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xPrevAnimatedTime = 0;
    const TickType_t UpdatePeriod = 20;
    const TickType_t AnimationPeriod = 400;
   
    unsigned char UP_DEBOUNCE_STATE = 0;
    unsigned char DOWN_DEBOUNCE_STATE = 0;

    unsigned char ImageAnimationIndex=0;

    vSetMainMenuLoadedImages() ;
    vSetPlayersInfoBufferValues();
    vSetMainMenuBufferValues();


    while (1) {
        xGetButtonInput(); 
        xCheckMenuSelectionChange(&UP_DEBOUNCE_STATE, 
                                  &DOWN_DEBOUNCE_STATE);
        if(xCheckEnterPressed())
                vHandleStateMachineActivation();

        if(xTaskGetTickCount() - xPrevAnimatedTime > AnimationPeriod){
            ImageAnimationIndex=!ImageAnimationIndex; 
            xPrevAnimatedTime = xTaskGetTickCount();
        }

        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xSemaphoreTake(ScreenLock,portMAX_DELAY);

                    tumDrawClear(Black); 
                    vDrawStaticTexts();
                    vDrawPointsExplanation(ImageAnimationIndex);
                    vDrawSpaceInvadersBanner();
                    vDrawMainMenuOptions();
                    vDrawFPS();

                xSemaphoreGive(ScreenLock);
                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
                }
        }
}

void vDrawInstructionsWithinGame()
{
    char ResetGameChar[40];
    int ResetGameCharWidth=0;

    char QuitChar[20];
    int QuitCharWidth=0;

    char PauseChar[20];
    int PauseCharWidth=0;

    sprintf(QuitChar,"[Q]uit");
    if(!tumGetTextSize((char *)QuitChar,&QuitCharWidth, NULL)){
                    checkDraw(tumDrawText(QuitChar,
                                          SCREEN_WIDTH*2/4-QuitCharWidth/2 + 90,
                                          SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                                          White),
                                          __FUNCTION__);
    }

    sprintf(PauseChar,"[P]ause");
    if(!tumGetTextSize((char *)PauseChar,&PauseCharWidth, NULL)){
                    checkDraw(tumDrawText(PauseChar,
                                          SCREEN_WIDTH*2/4-PauseCharWidth/2 + 170,
                                          SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                                          White),
                                          __FUNCTION__);
    }

    sprintf(ResetGameChar,"[R]eset Game");
    if(!tumGetTextSize((char *)ResetGameChar,&ResetGameCharWidth, NULL)){
                    checkDraw(tumDrawText(ResetGameChar,
                                          SCREEN_WIDTH*2/4-ResetGameCharWidth/2 - 35,
                                          SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                                          White),
                                          __FUNCTION__);
    }
}
void vDrawLevel()
{
    static char str[20] = { 0 };
    static int strWidth = { 0 };
    if(xSemaphoreTake(PlayerInfoBuffer.lock,portMAX_DELAY)==pdTRUE){
        sprintf(str,"Level [ %d ]", PlayerInfoBuffer.Level);
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

void vDrawSingleCreature(unsigned char creatureID)
{    
    if(CreaturesBuffer.Creatures[creatureID].Position)
        checkDraw(tumDrawLoadedImage(CreaturesBuffer.Creatures[creatureID].Image_0,
                                     CreaturesBuffer.Creatures[creatureID].x_pos -CREATURE_WIDTH/2,
                                     CreaturesBuffer.Creatures[creatureID].y_pos - CREATURE_HEIGHT/2),
                                     __FUNCTION__); 
    else
        checkDraw(tumDrawLoadedImage(CreaturesBuffer.Creatures[creatureID].Image_1,
                                     CreaturesBuffer.Creatures[creatureID].x_pos -CREATURE_WIDTH/2,
                                     CreaturesBuffer.Creatures[creatureID].y_pos - CREATURE_HEIGHT/2),
                                     __FUNCTION__); 
}
void vDrawSaucer()
{
    if(xSemaphoreTake(SaucerBuffer.lock, 0)==pdTRUE){
         
        checkDraw(tumDrawLoadedImage(SaucerBoss,
                                     SaucerBuffer.saucer->x_pos-SAUCER_WIDTH/2,
                                     SaucerBuffer.saucer->y_pos-SAUCER_WIDTH/2),
                                     __FUNCTION__);
    
        xSemaphoreGive(SaucerBuffer.lock);
    }
}
void vDrawCreatures()
{
    unsigned char CreatureCountID=0;

    if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){

        for(int i=0;i<NUMB_OF_ROWS;i++){
            for(int j=0;j<NUMB_OF_COLUMNS;j++){ 
                if(CreaturesBuffer.Creatures[CreatureCountID].Alive==1)
                    vDrawSingleCreature(CreatureCountID);
                ++CreatureCountID;
            }
        }
        xSemaphoreGive(CreaturesBuffer.lock);
    }
}
void vAnimateCreatures()
{
    unsigned char creatureIDcount = 0;

    while(creatureIDcount < NUMB_OF_CREATURES){      
        if(CreaturesBuffer.Creatures[creatureIDcount].Alive == 1)
            vAlternateAnimation(&CreaturesBuffer.Creatures[creatureIDcount]);
        ++creatureIDcount;
    }

}
void vDrawBunkers()
{
    if(xSemaphoreTake(BunkersBuffer.lock, portMAX_DELAY)==pdTRUE){  

        if(BunkersBuffer.Bunkers->b1Lives>0)
            checkDraw(tumDrawFilledBox(BunkersBuffer.Bunkers->b1->x_pos - BunkersBuffer.Bunkers->b1->size/2, 
                                       BunkersBuffer.Bunkers->b1->y_pos - BunkersBuffer.Bunkers->b1->size/2,
                                       BunkersBuffer.Bunkers->b1->size, BunkersBuffer.Bunkers->b1->size,
                                       BunkersBuffer.Bunkers->b1->color),
                                       __FUNCTION__);

        if(BunkersBuffer.Bunkers->b2Lives>0)
            checkDraw(tumDrawFilledBox(BunkersBuffer.Bunkers->b2->x_pos - BunkersBuffer.Bunkers->b2->size/2, 
                                       BunkersBuffer.Bunkers->b2->y_pos - BunkersBuffer.Bunkers->b2->size/2,
                                       BunkersBuffer.Bunkers->b2->size, BunkersBuffer.Bunkers->b2->size,
                                       BunkersBuffer.Bunkers->b2->color),
                                       __FUNCTION__);

        if(BunkersBuffer.Bunkers->b3Lives>0)
            checkDraw(tumDrawFilledBox(BunkersBuffer.Bunkers->b3->x_pos - BunkersBuffer.Bunkers->b3->size/2, 
                                       BunkersBuffer.Bunkers->b3->y_pos - BunkersBuffer.Bunkers->b3->size/2,
                                       BunkersBuffer.Bunkers->b3->size, BunkersBuffer.Bunkers->b3->size,
                                       BunkersBuffer.Bunkers->b3->color),
                                       __FUNCTION__);

        if(BunkersBuffer.Bunkers->b4Lives>0)
            checkDraw(tumDrawFilledBox(BunkersBuffer.Bunkers->b4->x_pos - BunkersBuffer.Bunkers->b4->size/2, 
                                       BunkersBuffer.Bunkers->b4->y_pos - BunkersBuffer.Bunkers->b4->size/2,
                                       BunkersBuffer.Bunkers->b4->size, BunkersBuffer.Bunkers->b4->size,
                                       BunkersBuffer.Bunkers->b4->color),
                                       __FUNCTION__);

        xSemaphoreGive(BunkersBuffer.lock);
    }
}
void vDrawLowerWall()
{
    checkDraw(tumDrawLine(0, BOTTOM_WALLPOSITION,
                          SCREEN_WIDTH, BOTTOM_WALLPOSITION,
                          BOTTOM_WALLTHICKNESS,Green),
                          __FUNCTION__);
}
void vDrawShip()
{
    if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){
        checkDraw(tumDrawLoadedImage(PlayerShip, 
                                     ShipBuffer.Ship->x_pos - PLAYERSHIP_WIDTH/2,
                                     ShipBuffer.Ship->y_pos - PLAYERSHIP_HEIGHT/2),
                                     __FUNCTION__);
        xSemaphoreGive(ShipBuffer.lock);
    }
}
unsigned char xCheckShipShoot(unsigned char* SPACE_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {
        if (buttons.buttons[KEYCODE(SPACE)]) {
            if(buttons.buttons[KEYCODE(SPACE)]!=(*SPACE_DEBOUNCE_STATE)){
                (*SPACE_DEBOUNCE_STATE) = buttons.buttons[KEYCODE(SPACE)];
                xSemaphoreGive(buttons.lock);
                return 1;
            }
        }
        (*SPACE_DEBOUNCE_STATE) = buttons.buttons[KEYCODE(SPACE)];
        xSemaphoreGive(buttons.lock);
    }
    return 0;    
}
unsigned char xCheckShipMoved(void)
{
    if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {
        if (buttons.buttons[KEYCODE(LEFT)]) {
            if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
                if(ShipBuffer.Ship->x_pos >= PLAYERSHIP_WIDTH*3/4)
                    vIncrementShipLeft(ShipBuffer.Ship);
                xSemaphoreGive(ShipBuffer.lock);
                xSemaphoreGive(buttons.lock);
                return 1;
            }
            xSemaphoreGive(buttons.lock);
        }                            
        if (buttons.buttons[KEYCODE(RIGHT)]) { 
            if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
                if(ShipBuffer.Ship->x_pos <= SCREEN_WIDTH - PLAYERSHIP_WIDTH*3/4)
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

void vTriggerCreaturesBulletControl()
{
   if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){
       vDrawCreaturesBullet(&CreaturesBuffer.CreaturesBullet);
       xSemaphoreGive(CreaturesBuffer.lock);
       vTaskResume(CreaturesBulletControlTask);
       xTaskNotify(CreaturesBulletControlTask, 0x01, eSetValueWithOverwrite);
   }
}
void vTriggerShipBulletControl()
{
    if(xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY)==pdTRUE){
        vDrawShipBullet(ShipBuffer.Ship);
        vTaskResume(ShipBulletControlTask);
        xTaskNotify(ShipBulletControlTask, 0x01, eSetValueWithOverwrite);
        xSemaphoreGive(ShipBuffer.lock);
    }
    xSemaphoreGive(ShipBuffer.lock);
}

void vTaskShipShotControl(void *pvParameters)
{
    while(1){
        uint32_t ShipShotSignal;
        if(xTaskNotifyWait(0x00, 0xffffffff, &ShipShotSignal, portMAX_DELAY)==pdTRUE){
            if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                vPlayShipShotSound();
                PlayerInfoBuffer.LivesLeft-=1;
                xSemaphoreGive(PlayerInfoBuffer.lock);
            }
        }
    }
}

void vSpeedCreaturesControl()
{
    if(xSemaphoreTake(LevelModifiersBuffer.lock, 0)==pdTRUE){
        LevelModifiersBuffer.NumberOfSpeedChanges++;
        if(LevelModifiersBuffer.NumberOfSpeedChanges>4){
            LevelModifiersBuffer.AnimationPeriod-=100;
            LevelModifiersBuffer.MovingPeriod-=100;
            LevelModifiersBuffer.NumberOfSpeedChanges=0;
        }
        xSemaphoreGive(LevelModifiersBuffer.lock);
    }
}

void vCreatureScoreControl(unsigned char CreatureCollisionID)
{
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        vUpdatePlayerScoreCreatureKilled(CreaturesBuffer.Creatures[CreatureCollisionID].CreatureType);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}

void vTaskCreaturesShotControl(void *pvParameters)
{
    while(1){
        uint32_t CreatureCollisionID;
        if(xTaskNotifyWait(0x00, 0xffffffff, &CreatureCollisionID, portMAX_DELAY)==pdTRUE){
            if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){
                vKillCreature(&CreaturesBuffer.Creatures[CreatureCollisionID],
                              &CreaturesBuffer.NumbOfAliveCreatures);
                vUpdateFrontierCreaturesIDs(CreaturesBuffer.FrontierCreaturesID,
                                            CreatureCollisionID);

                vPlayDeadCreatureSound();
                vCreatureScoreControl(CreatureCollisionID);
                vSpeedCreaturesControl();
                xSemaphoreGive(CreaturesBuffer.lock);
            }
       } 
    }

}

void vTaskBunkerCreaturesCrashed(void *pvParameters)
{
    while(1){
        uint32_t BunkerCollisionID;

       if(xTaskNotifyWait(0x00, 0xffffffff, &BunkerCollisionID, portMAX_DELAY)==pdTRUE){
           if(xSemaphoreTake(BunkersBuffer.lock,portMAX_DELAY)==pdTRUE){
                vPlayBunkerShotSound();
                vKillBunker(BunkersBuffer.Bunkers, 
                            BunkerCollisionID);
                xSemaphoreGive(BunkersBuffer.lock);
           }
       }
    
    }
}
void vTaskBunkerShotControl(void *pvParameters)
{
    while(1){
        uint32_t BunkerCollisionID;

       if(xTaskNotifyWait(0x00, 0xffffffff, &BunkerCollisionID, portMAX_DELAY)==pdTRUE){
           if(xSemaphoreTake(BunkersBuffer.lock,portMAX_DELAY)==pdTRUE){
                vPlayBunkerShotSound();
                vUpdateBunkersStatus(BunkersBuffer.Bunkers, 
                                     BunkerCollisionID);
                xSemaphoreGive(BunkersBuffer.lock);
           }
       }
    
    }

}
void vSaucerScoreControl()
{
    unsigned char AddOn = xFetchSaucerValue();
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        PlayerInfoBuffer.Score+=AddOn;
        if(PlayerInfoBuffer.FreshGame==1 || PlayerInfoBuffer.Score > PlayerInfoBuffer.HiScore)
            PlayerInfoBuffer.HiScore=PlayerInfoBuffer.Score;
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}
void vTaskSaucerShotControl(void *pvParameters)
{
    while(1){
        uint32_t SaucerCollisionID;
        if(xTaskNotifyWait(0x00, 0xffffffff, &SaucerCollisionID, portMAX_DELAY)==pdTRUE){
            if(xSemaphoreTake(SaucerBuffer.lock, portMAX_DELAY)==pdTRUE){
                vKillSaucer(&SaucerBuffer.SaucerHitFlag, &SaucerBuffer.SaucerAppearsFlag);
                vSaucerScoreControl();
                vPlayDeadCreatureSound();
                xSemaphoreGive(SaucerBuffer.lock);
            }
       } 
    }
}

void vTaskShipBulletControl(void *pvParameters)
{
    static unsigned char TopWallCollisionFlag=0;
    static unsigned char BunkerCollisionFlag=0;
    static signed char CreatureCollisionFlag=0;
    static unsigned char SaucerCollisionFlag=0;

    while(1){
        uint32_t BulletLaunchSignal;

        if(xTaskNotifyWait(0x00, 0xffffffff, &BulletLaunchSignal, portMAX_DELAY) == pdTRUE){
            if(xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY)==pdTRUE){   
                if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){   
                    if(xSemaphoreTake(BunkersBuffer.lock, portMAX_DELAY)==pdTRUE){
                        if(xSemaphoreTake(SaucerBuffer.lock, portMAX_DELAY)==pdTRUE){
                            vUpdateShipBulletPos(ShipBuffer.Ship);

                            BunkerCollisionFlag=xCheckBunkersCollision(ShipBuffer.Ship->bullet->x_pos, 
                                                                       ShipBuffer.Ship->bullet->y_pos,
                                                                       (*BunkersBuffer.Bunkers));

                            TopWallCollisionFlag=xCheckShipBulletCollisionTopWall(ShipBuffer.Ship->bullet->y_pos);

                        
                            CreatureCollisionFlag=xCheckCreaturesCollision(CreaturesBuffer.Creatures,
                                                                           ShipBuffer.Ship->bullet->x_pos,
                                                                           ShipBuffer.Ship->bullet->y_pos,
                                                                           CreaturesBuffer.HorizontalDirection,
                                                                           CreaturesBuffer.FrontierCreaturesID);

                            if(SaucerBuffer.SaucerAppearsFlag && SaucerBuffer.SaucerHitFlag==0){
                                SaucerCollisionFlag=xCheckSaucerCollision(SaucerBuffer.saucer,
                                                                          SaucerBuffer.Direction,
                                                                          ShipBuffer.Ship->bullet->x_pos,
                                                                          ShipBuffer.Ship->bullet->y_pos);
                            }

                            if(SaucerCollisionFlag || BunkerCollisionFlag || TopWallCollisionFlag || (CreatureCollisionFlag >=0)){  
                                ShipBuffer.Ship->bullet->BulletAliveFlag=0;
                                ShipBuffer.BulletCrashed=1;

                                if(BunkerCollisionFlag){  
                                    vTaskResume(BunkerShotControlTask);
                                    xTaskNotify(BunkerShotControlTask, (uint32_t)BunkerCollisionFlag, eSetValueWithOverwrite);
                                }

                                else if(CreatureCollisionFlag>=0){
                                    vTaskResume(CreaturesShotControlTask);
                                    xTaskNotify(CreaturesShotControlTask, (uint32_t)CreatureCollisionFlag, eSetValueWithOverwrite);
                                }
                                else if(SaucerCollisionFlag > 0){
                                    vTaskResume(SaucerShotControlTask);
                                    xTaskNotify(SaucerShotControlTask, (uint32_t)SaucerCollisionFlag, eSetValueWithOverwrite);
                                    SaucerCollisionFlag=0;
                                }
                            }

                            xSemaphoreGive(SaucerBuffer.lock);
                        }
                        xSemaphoreGive(BunkersBuffer.lock);
                    }
                    xSemaphoreGive(CreaturesBuffer.lock);
                }
                xSemaphoreGive(ShipBuffer.lock);
            }
        }
    }

}
void vTaskCreaturesBulletControl(void *pvParameters)
{
    static unsigned char BottomWallCollisionFlag=0;
    static unsigned char BunkerCollisionFlag=0;
    static unsigned char ShipCollisonFlag=0;

    while(1){
        uint32_t CreatureBulletControlSignal;
            if(xTaskNotifyWait(0x00, 0xffffffff, &CreatureBulletControlSignal, portMAX_DELAY)==pdTRUE){
                if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){
                    if(xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY)){ 

                        vUpdateCreaturesBulletPos(&CreaturesBuffer.CreaturesBullet);

                        BottomWallCollisionFlag = xCheckCreaturesBulletCollisonBottomWall(CreaturesBuffer.CreaturesBullet.y_pos);

                        BunkerCollisionFlag = xCheckBunkersCollision(CreaturesBuffer.CreaturesBullet.x_pos,
                                                                     CreaturesBuffer.CreaturesBullet.y_pos,
                                                                     (*BunkersBuffer.Bunkers));
                        
                        ShipCollisonFlag = xCheckCreaturesBulletShipCollision(CreaturesBuffer.CreaturesBullet.x_pos,
                                                                              CreaturesBuffer.CreaturesBullet.y_pos,
                                                                              ShipBuffer.Ship);

                        if(BottomWallCollisionFlag || BunkerCollisionFlag || ShipCollisonFlag){
                            CreaturesBuffer.BulletAliveFlag=0;

                            if(BottomWallCollisionFlag)
                                vPlayBulletWallSound();

                            else if(BunkerCollisionFlag){
                                vTaskResume(BunkerShotControlTask);
                                xTaskNotify(BunkerShotControlTask, (uint32_t)BunkerCollisionFlag, eSetValueWithOverwrite);
                            }
                            else if(ShipCollisonFlag){  
                                vTaskResume(ShipShotControlTask);
                                xTaskNotify(ShipShotControlTask, (uint32_t)ShipCollisonFlag, eSetValueWithOverwrite);
                            }
                        }
                        xSemaphoreGive(ShipBuffer.lock);
                    }
                    xSemaphoreGive(CreaturesBuffer.lock);
                }
            }
    }
}

unsigned char xCheckDirectionChange(H_Movement_t* LastDirection)
{
   if((*LastDirection)!=CreaturesBuffer.HorizontalDirection)
       return 1;
   else
       return 0;
}

void vHorizontalCreatureControl(H_Movement_t* LastHorizontalDirectionOfCreatures,
                                unsigned char* NumberOfLaps)
{
    (*LastHorizontalDirectionOfCreatures) = CreaturesBuffer.HorizontalDirection; 
    vMoveCreaturesHorizontal(CreaturesBuffer.Creatures, &CreaturesBuffer.HorizontalDirection);
    if(xCheckDirectionChange(LastHorizontalDirectionOfCreatures))
        (*NumberOfLaps)++;
}

void vVerticalCreatureControl(unsigned char* NumberOfLaps)
{
    if((*NumberOfLaps) == 1){
        (*NumberOfLaps)=0;
        vMoveCreaturesVerticalDown(CreaturesBuffer.Creatures);
    }
}
void vTriggerCreaturesBunkerDestruction()
{
    static unsigned char BunkerCreatureCollisionID=0;
    if(xSemaphoreTake(BunkersBuffer.lock, 0)==pdTRUE){
        BunkerCreatureCollisionID = xCheckCreaturesTouchBunkers(CreaturesBuffer.Creatures,
                                                                CreaturesBuffer.FrontierCreaturesID,
                                                                BunkersBuffer.Bunkers);

        xSemaphoreGive(BunkersBuffer.lock);
        if(BunkerCreatureCollisionID > 0){
            vTaskResume(BunkerCreaturesCrashedTask);
            xTaskNotify(BunkerCreaturesCrashedTask, (uint32_t) BunkerCreatureCollisionID, eSetValueWithOverwrite);
        }
    }
}
void vCreaturesInitiateShoot(TickType_t* xPrevShotTime, TickType_t* ShootingPeriod)
{

    if(xTaskGetTickCount() - (*xPrevShotTime) >= (*ShootingPeriod) &&
        CreaturesBuffer.BulletAliveFlag==0 &&
        CreaturesBuffer.NumbOfAliveCreatures>0){

        vCreateCreaturesBullet(CreaturesBuffer.Creatures, 
                               &CreaturesBuffer.CreaturesBullet,
                               CreaturesBuffer.FrontierCreaturesID);
        

        CreaturesBuffer.BulletAliveFlag=1;
        vPlayBulletSound();
        (*xPrevShotTime) = xTaskGetTickCount();
    }
}
void vTaskCreaturesActionControl(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xPrevMovedTime = 0;
    TickType_t xPrevAnimatedTime = 0;
    TickType_t xPrevShotTime = 0;

    const TickType_t WakeRate = 15;
   
    static unsigned char NumberOfLaps = 0;
    static H_Movement_t LastHorizontalDirectionOfCreatures = RIGHT;

    vSetCreaturesBufferValues();
    vSetLevelModifiersValues();

    printf("Level: %d\n", PlayerInfoBuffer.Level);
    printf("MovingPeriod: %d\n", LevelModifiersBuffer.MovingPeriod);
    printf("Shooting Period in MS: %d\n", LevelModifiersBuffer.ShootingPeriod);

    while(1){
        
        if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){

            if(xTaskGetTickCount() - xPrevAnimatedTime >= LevelModifiersBuffer.AnimationPeriod){   
                vAnimateCreatures(); 
                xPrevAnimatedTime = xTaskGetTickCount();
            }
            
            if(xTaskGetTickCount() - xPrevMovedTime >= LevelModifiersBuffer.MovingPeriod){
                vHorizontalCreatureControl(&LastHorizontalDirectionOfCreatures,
                                           &NumberOfLaps);
                xPrevMovedTime = xTaskGetTickCount();
            }

            vVerticalCreatureControl(&NumberOfLaps);

            vCreaturesInitiateShoot(&xPrevShotTime, &LevelModifiersBuffer.ShootingPeriod);

            vTriggerCreaturesBunkerDestruction();


        xSemaphoreGive(CreaturesBuffer.lock);
        }

        vTaskDelayUntil(&xLastWakeTime, 
                        pdMS_TO_TICKS(WakeRate));
    }
}

void vTaskSaucerActionControl(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t WakeRate = 15;

    TickType_t xPrevAppearanceTime = 0;
    const TickType_t xSecondMeasure = 1000;
    const TickType_t AppearancePeriod = 10*xSecondMeasure;

    vSetSaucerBufferValues();

    while(1){
    
        if(xSemaphoreTake(SaucerBuffer.lock,0)==pdTRUE){
        
            if(SaucerBuffer.SaucerHitFlag==0){
                if(xTaskGetTickCount() - xPrevAppearanceTime >= AppearancePeriod){
                    SaucerBuffer.SaucerAppearsFlag = !SaucerBuffer.SaucerAppearsFlag;
                    xPrevAppearanceTime = xTaskGetTickCount();
                }

            if(SaucerBuffer.SaucerAppearsFlag==1){
                vMoveSaucerHorizontal(SaucerBuffer.saucer, &SaucerBuffer.Direction);
            }

            }
        
            xSemaphoreGive(SaucerBuffer.lock);
        }

        vTaskDelayUntil(&xLastWakeTime, 
                        pdMS_TO_TICKS(WakeRate));
    }
}

void xRetrieveSaucerAppearsFlag(unsigned char* SaucerAppearsFlag)
{
    if(xSemaphoreTake(SaucerBuffer.lock, 0)==pdTRUE){
       if(SaucerBuffer.SaucerAppearsFlag==1){
            (*SaucerAppearsFlag) = 1;
            xSemaphoreGive(SaucerBuffer.lock);
       }
       else {
            (*SaucerAppearsFlag) = 0;
            xSemaphoreGive(SaucerBuffer.lock);
       }
    }
}

void xRetrieveCreaturesBulletAliveFlag(unsigned char* CreaturesBulletOnScreenFlag)
{
    if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){
        if(CreaturesBuffer.BulletAliveFlag==1){
            xSemaphoreGive(CreaturesBuffer.lock);
            (*CreaturesBulletOnScreenFlag) = 1;
        }
        else{
            xSemaphoreGive(CreaturesBuffer.lock);
            (*CreaturesBulletOnScreenFlag) = 0;
        }
    }
}

void  xRetrieveShipBulletAliveFlag(unsigned char* ShipBulletOnScreenFlag)
{
    if(xSemaphoreTake(ShipBuffer.lock,portMAX_DELAY)==pdTRUE){  
        if(ShipBuffer.Ship->bullet->BulletAliveFlag == 1){
            xSemaphoreGive(ShipBuffer.lock);
            (*ShipBulletOnScreenFlag) = 1;
        }
        else{
            ShipBuffer.BulletCrashed=0;
            xSemaphoreGive(ShipBuffer.lock);
            (*ShipBulletOnScreenFlag) = 0;
        }
        xSemaphoreGive(ShipBuffer.lock);
    }
}

void vSetShipBulletFlags()
{
    if(xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY)==pdTRUE){
        if(ShipBuffer.BulletCrashed == 0 && ShipBuffer.Ship->bullet->BulletAliveFlag == 0){
            CreateShipBullet(ShipBuffer.Ship);
            ShipBuffer.Ship->bullet->BulletAliveFlag=1;
            vPlayBulletSound();
            xSemaphoreGive(ShipBuffer.lock);
        }
    }
}

unsigned char xCheckLivesLeft()
{
    if (xSemaphoreTake(PlayerInfoBuffer.lock, 0) == pdTRUE){
        if (PlayerInfoBuffer.LivesLeft == 0){
            xSemaphoreGive(PlayerInfoBuffer.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
                OutsideGameActionsBuffer.PlayerOutsideGameActions = LostGameAction;
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
                return 1;
            }
        }              
        xSemaphoreGive(PlayerInfoBuffer.lock);
        return 0;
    }
    return 0;
}
unsigned char xCheckPausePressed()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE){
        if (buttons.buttons[KEYCODE(P)]){
            xSemaphoreGive(buttons.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
                OutsideGameActionsBuffer.PlayerOutsideGameActions = PauseGameAction;
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
            }
            return 1;
        }              
        xSemaphoreGive(buttons.lock);
        return 0;
    }
    return 0;
}

unsigned char xCheckResetPressed()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE){
        if (buttons.buttons[KEYCODE(R)]){
            xSemaphoreGive(buttons.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
                OutsideGameActionsBuffer.PlayerOutsideGameActions = ResetGameAction;
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
            }
            return 1;
        }              
        xSemaphoreGive(buttons.lock);
        return 0;
    }
    return 0;
}

unsigned char  xCheckCreaturesLeft()
{
    if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){
        if(CreaturesBuffer.NumbOfAliveCreatures==0){
            xSemaphoreGive(CreaturesBuffer.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
                OutsideGameActionsBuffer.PlayerOutsideGameActions = WonGameAction;
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
            }
            return 1;
        }
        xSemaphoreGive(CreaturesBuffer.lock);
        return 0;
    }
    return 0;
}

unsigned char xCheckCreaturesReachedBottom()
{
    if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){
        if(xCheckFrontierReachedBottom(CreaturesBuffer.Creatures,
                                        CreaturesBuffer.FrontierCreaturesID)){
            xSemaphoreGive(CreaturesBuffer.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
                OutsideGameActionsBuffer.PlayerOutsideGameActions = LostGameAction;
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
            }
            return 1;
        }
        xSemaphoreGive(CreaturesBuffer.lock);
        return 0;
    }
    return 0;
}

void vTaskPlayingGame(void *pvParameters)
{
    PlayerShip=tumDrawLoadImage("../resources/player.bmp");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    static unsigned char SPACE_DEBOUNCE_STATE = 0;

    static unsigned char ShipBulletOnScreenFlag = 0; //if 0 -> No bullet on screen -> player allowed to shoot.
    static unsigned char CreaturesBulletOnScreenFlag = 0;
    static unsigned char SaucerAppearsFlag = 1;

    vSetOutsideGameActionsBufferValues();
    vSetShipsBufferValues();
    vSetBunkersBufferValues();

    while(1){
        xGetButtonInput();
        if(xCheckPausePressed() || xCheckLivesLeft() || xCheckResetPressed() || 
           xCheckCreaturesLeft() || xCheckCreaturesReachedBottom())
            vHandleStateMachineActivation();
        
        xCheckShipMoved();
        if(xCheckShipShoot(&SPACE_DEBOUNCE_STATE) && ShipBulletOnScreenFlag == 0)
            vSetShipBulletFlags();

        xRetrieveSaucerAppearsFlag(&SaucerAppearsFlag);
        xRetrieveShipBulletAliveFlag(&ShipBulletOnScreenFlag); 
        xRetrieveCreaturesBulletAliveFlag(&CreaturesBulletOnScreenFlag) ;

        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xSemaphoreTake(ScreenLock,portMAX_DELAY);

                    tumDrawClear(Black);
                    vDrawStaticTexts();
                    vDrawShip();                    
                    vDrawCreatures();

                    if(CreaturesBulletOnScreenFlag)
                        vTriggerCreaturesBulletControl();
                    if(ShipBulletOnScreenFlag)
                        vTriggerShipBulletControl();

                    if(SaucerAppearsFlag==1)
                        vDrawSaucer();

                    vDrawBunkers();
                    vDrawLowerWall();
                    vDrawLevel();
                    vDrawLives();
                    vDrawInstructionsWithinGame();
                    vDrawFPS();    

                xSemaphoreGive(ScreenLock);

                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
    }
}

void vDrawInstructionsPausedGame()
{
    char QuitChar[20];
    int QuitCharWidth=0;

    char ResumeChar[20];
    int ResumeCharWidth=0;

    sprintf(ResumeChar,"[R]esume");
    if(!tumGetTextSize((char *)ResumeChar,&ResumeCharWidth, NULL)){
                    checkDraw(tumDrawText(ResumeChar,
                                          SCREEN_WIDTH*2/4-ResumeCharWidth/2,
                                          SCREEN_HEIGHT*50/100 - DEFAULT_FONT_SIZE/2,
                                          White),
                                          __FUNCTION__);
    }

    sprintf(QuitChar,"[Q]uit");
    if(!tumGetTextSize((char *)QuitChar,&QuitCharWidth, NULL)){
                    checkDraw(tumDrawText(QuitChar,
                                          SCREEN_WIDTH*2/4-QuitCharWidth/2,
                                          SCREEN_HEIGHT*65/100 - DEFAULT_FONT_SIZE/2,
                                          White),
                                          __FUNCTION__);
    }
}

unsigned char xCheckResumePressed()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE){
        if (buttons.buttons[KEYCODE(R)]){
            xSemaphoreGive(buttons.lock);
            if(xSemaphoreTake(PausedGameInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                PausedGameInfoBuffer.PausedGameActions = ResumeGame;
                xSemaphoreGive(PausedGameInfoBuffer.lock);
            }
            return 1;
        }              
        xSemaphoreGive(buttons.lock);
        return 0;
    }
    return 0;
}

void vTaskPausedGame(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    while(1){
        xGetButtonInput();
        if(xCheckResumePressed())
            vHandleStateMachineActivation();

        if(DrawSignal) 
            if(xSemaphoreTake(DrawSignal, portMAX_DELAY)==pdTRUE){
                xSemaphoreTake(ScreenLock, portMAX_DELAY);
            
                    tumDrawClear(Black);
                    vDrawInstructionsPausedGame();
                    vDrawStaticTexts();
                    vDrawLevel();
                    vDrawLives();
                    vDrawFPS(); 

                xSemaphoreGive(ScreenLock);

                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
    }
}

void vDrawInstructionsGameOver()
{
    char QuitChar[20];
    int QuitCharWidth=0;

    char GoToMainMenuChar[20];
    int GoToMainMenuCharWidth=0;

    sprintf(GoToMainMenuChar,"Play Again");
    if(!tumGetTextSize((char *)GoToMainMenuChar,&GoToMainMenuCharWidth, NULL)){
                    checkDraw(tumDrawText(GoToMainMenuChar,
                                          SCREEN_WIDTH*2/4-GoToMainMenuCharWidth/2,
                                          SCREEN_HEIGHT*50/100 - DEFAULT_FONT_SIZE/2,
                                          xFetchSelectedColor(GameOverInfoBuffer.SelectedGameOverOption, PlayAgain)),
                                          __FUNCTION__);
    }

    sprintf(QuitChar,"Leave the Game");
    if(!tumGetTextSize((char *)QuitChar,&QuitCharWidth, NULL)){
                    checkDraw(tumDrawText(QuitChar,
                                          SCREEN_WIDTH*2/4-QuitCharWidth/2,
                                          SCREEN_HEIGHT*65/100 - DEFAULT_FONT_SIZE/2,
                                          xFetchSelectedColor(GameOverInfoBuffer.SelectedGameOverOption, Quit)),
                                          __FUNCTION__);
    }
}
void vDrawGameOverBanner()
{
    checkDraw(tumDrawLoadedImage(GameOver,
                                 SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(GameOver)/2 + 15,
                                 SCREEN_HEIGHT*4/10-tumDrawGetLoadedImageHeight(GameOver)/2),
                                 __FUNCTION__);
}

void  xCheckGameOverSelectionChange(unsigned char* UP_DEBOUNCE_STATE, 
                                unsigned char* DOWN_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, portMAX_DELAY) == pdTRUE) {

        if (buttons.buttons[KEYCODE(DOWN)]) {
            if(buttons.buttons[KEYCODE(DOWN)]!=(*DOWN_DEBOUNCE_STATE))
                if(xSemaphoreTake(GameOverInfoBuffer.lock,portMAX_DELAY)==pdTRUE){  
                    vDownGameOverSelection(&GameOverInfoBuffer.SelectedGameOverOption);
                    xSemaphoreGive(GameOverInfoBuffer.lock);
                }
        }                            
        else if (buttons.buttons[KEYCODE(UP)]) { 
            if(buttons.buttons[KEYCODE(UP)]!=(*UP_DEBOUNCE_STATE))
                if(xSemaphoreTake(GameOverInfoBuffer.lock,portMAX_DELAY)==pdTRUE){  
                    vDownGameOverSelection(&GameOverInfoBuffer.SelectedGameOverOption);
                    xSemaphoreGive(GameOverInfoBuffer.lock);
                }
        }   

        (*UP_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(UP)];
        (*DOWN_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(DOWN)];
        xSemaphoreGive(buttons.lock);                                                                                                                                                                
    }   
}
void vTaskGameOver(void *pvParameters)
{
    GameOver = tumDrawLoadImage("../resources/GameOver.bmp");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    vSetGameOverInfoBufferValues();

    unsigned char UP_DEBOUNCE_STATE = 0;
    unsigned char DOWN_DEBOUNCE_STATE = 0;

    while(1){
        xGetButtonInput();
        xCheckGameOverSelectionChange(&UP_DEBOUNCE_STATE, 
                                      &DOWN_DEBOUNCE_STATE);

        if(xCheckEnterPressed())
                vHandleStateMachineActivation();

        if(DrawSignal) 
            if(xSemaphoreTake(DrawSignal, portMAX_DELAY)==pdTRUE){
                xSemaphoreTake(ScreenLock, portMAX_DELAY);
            
                    tumDrawClear(Black);
                    vDrawGameOverBanner();
                    vDrawInstructionsGameOver();
                    vDrawStaticTexts();
                    vDrawLevel();
                    vDrawLives();
                    vDrawFPS(); 

                xSemaphoreGive(ScreenLock);

                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
    }
}

void vDrawCountDown()
{
    char CountDownChar[20];
    int CountDownCharWidth=0;

    sprintf(CountDownChar,"Loading...");
    if(!tumGetTextSize((char *)CountDownChar,&CountDownCharWidth, NULL)){
                    checkDraw(tumDrawText(CountDownChar,
                                          SCREEN_WIDTH*2/4-CountDownCharWidth/2,
                                          SCREEN_HEIGHT*70/100 - DEFAULT_FONT_SIZE/2,
                                          White),
                                          __FUNCTION__);
    }
}

void vDrawNextLevelBanner()
{
    checkDraw(tumDrawLoadedImage(NextLevel,
                                 SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(NextLevel)/2,
                                 SCREEN_HEIGHT*4/10-tumDrawGetLoadedImageHeight(NextLevel)/2),
                                 __FUNCTION__);
}

void vTaskNextLevel(void *pvParameters)
{
    NextLevel = tumDrawLoadImage("../resources/NextLevel.bmp");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xCountDown = 0;
    signed char CountdownInSeconds = 10;
    const TickType_t UpdatePeriod = 20;

    while(1){
        if(CountdownInSeconds==0)
            vHandleStateMachineActivation();

        if(xTaskGetTickCount() - xCountDown >= 1000){
            CountdownInSeconds--; 
            xCountDown=xTaskGetTickCount();
        }

        if(DrawSignal) 
            if(xSemaphoreTake(DrawSignal, portMAX_DELAY)==pdTRUE){
                xSemaphoreTake(ScreenLock, portMAX_DELAY);
            
                    tumDrawClear(Black);
                    vDrawStaticTexts();
                    vDrawNextLevelBanner();
                    vDrawCountDown();
                    vDrawFPS(); 

                xSemaphoreGive(ScreenLock);

                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
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
            tumEventFetchEvents(FETCH_EVENT_NONBLOCK);
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}
void vHandleNextLevelStateSM()
{
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        vPrepareGameValues(PlayerInfoBuffer.Level+1, PlayerInfoBuffer.Score);
        xSemaphoreGive(PlayerInfoBuffer.lock);
        if(StateQueue)
            xQueueSend(StateQueue,&PlayingStateSignal, 0);
    }
}
void vHandleGameOverStateSM()
{
    if(xSemaphoreTake(GameOverInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        switch(GameOverInfoBuffer.SelectedGameOverOption){
            case PlayAgain:
                xSemaphoreGive(GameOverInfoBuffer.lock);
                if(StateQueue)
                    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                        vPrepareGameValues(1,0);
                        xSemaphoreGive(PlayerInfoBuffer.lock);
                    }
                    xQueueSend(StateQueue,&MainMenuStateSignal, 0);

            case Quit:
            default:
                xSemaphoreGive(GameOverInfoBuffer.lock);
                exit(EXIT_SUCCESS);
               break; 
        }
        xSemaphoreGive(GameOverInfoBuffer.lock);
    }
}
void vHandlePausedGameStateSM()
{
    if(xSemaphoreTake(PausedGameInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        switch(PausedGameInfoBuffer.PausedGameActions){
            case ResumeGame:
                xSemaphoreGive(PausedGameInfoBuffer.lock);
                if(StateQueue)
                    xQueueSend(StateQueue,&PlayingStateSignal, 0);

            case RemainPaused:
            default:
                xSemaphoreGive(PausedGameInfoBuffer.lock);
               break; 
        }
        xSemaphoreGive(PausedGameInfoBuffer.lock);
    }
}
void vHandlePlayingGameStateSM()
{
    if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
        switch(OutsideGameActionsBuffer.PlayerOutsideGameActions){
            
            case PauseGameAction:
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
                if(StateQueue)
                    xQueueSend(StateQueue,&PausedGameStateSignal, 0);
                break;
            case LostGameAction:
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
                if(StateQueue)
                    xQueueSend(StateQueue,&GameOverStateSignal, 0);
                break;
            case WonGameAction:
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
                if(StateQueue)
                    xQueueSend(StateQueue,&NextLevelStateSignal, 0);
            case ResetGameAction:
                if (StateQueue)
                    xQueueSend(StateQueue,&ResetGameStateSignal, 0);
            case NoAction:
            default:
               break;
        }
        xSemaphoreGive(OutsideGameActionsBuffer.lock);
    }
}
void vHandleMainMenuStateSM()
{
    if(xSemaphoreTake(MainMenuInfoBuffer.lock, portMAX_DELAY)==pdTRUE){

        switch(MainMenuInfoBuffer.SelectedMenuOption){

            case SinglePlayer:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                if(StateQueue)
                    xQueueSend(StateQueue,&PlayingStateSignal, 0);
                break; 

            case Leave:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                exit(EXIT_SUCCESS);
                break;

            case MultiPlayer:
            default:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                break;
        }

        xSemaphoreGive(MainMenuInfoBuffer.lock);
    }
}
void vHandleStateMachineActivation()
{

    if(xSemaphoreTake(GameStateBuffer.lock, portMAX_DELAY)==pdTRUE){  
        switch(GameStateBuffer.GameState){
            case ResetGameState:
            case MainMenuState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandleMainMenuStateSM();
                break; 

            case PlayingState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandlePlayingGameStateSM();
                break;

            case PausedState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandlePausedGameStateSM();
                break;

            case GameOverState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandleGameOverStateSM();
                break;
            case NextLevelState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandleNextLevelStateSM();
            default:
                break; 
        
        }
        xSemaphoreGive(GameStateBuffer.lock);
    }
}

void vRecreateGame()
{
    if(CreaturesActionControlTask){ 
        vTaskSuspend(CreaturesActionControlTask);
        vTaskDelete(CreaturesActionControlTask);
        if(xTaskCreate(vTaskCreaturesActionControl, "CreaturesActionControl", mainGENERIC_STACK_SIZE*2, NULL,
                       configMAX_PRIORITIES - 3, &CreaturesActionControlTask)!=pdPASS)
            exit(EXIT_SUCCESS);
        vTaskSuspend(CreaturesActionControlTask);
    }
    if(SaucerActionControlTask){ 
        vTaskSuspend(SaucerActionControlTask);
        vTaskDelete(SaucerActionControlTask);
        if(xTaskCreate(vTaskSaucerActionControl, "SaucerActionControlTask", mainGENERIC_STACK_SIZE*2, NULL,
                       configMAX_PRIORITIES - 3, &SaucerActionControlTask)!=pdPASS)
            exit(EXIT_SUCCESS);
        vTaskSuspend(SaucerActionControlTask);
    }
    if(MainPlayingGameTask){
        vTaskSuspend(MainPlayingGameTask);
        vTaskDelete(MainPlayingGameTask);
        if(xTaskCreate(vTaskPlayingGame, "MainPlayingGameTask", mainGENERIC_STACK_SIZE * 2, NULL,
                       configMAX_PRIORITIES-4, &MainPlayingGameTask) != pdPASS) 
            exit(EXIT_SUCCESS);
        vTaskSuspend(MainPlayingGameTask);
    }
}

void vStateMachine(void *pvParameters){
    unsigned char current_state = BEGIN;
    unsigned char changed_state = 1;

    TickType_t Last_Change = 0;
    const int state_change_period = STATE_DEBOUNCE_DELAY;

    while(1){
        Last_Change=xTaskGetTickCount();
        if (changed_state) goto initial_state;

        if (StateQueue){
            if (xQueueReceive(StateQueue,&current_state,portMAX_DELAY)==pdTRUE){
                    if(xTaskGetTickCount() - Last_Change > state_change_period){
                        changed_state=1;
                        Last_Change=xTaskGetTickCount();
                    }
            }
        }

        initial_state:
            if (changed_state){
                xSemaphoreTake(GameStateBuffer.lock, portMAX_DELAY);

                    switch (current_state){

                        case MainMenuState: // Begin 

                            if(CreaturesActionControlTask) vTaskSuspend(CreaturesActionControlTask);
                            if(SaucerActionControlTask) vTaskSuspend(SaucerActionControlTask);
                            if(MainPlayingGameTask) vTaskSuspend(MainPlayingGameTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(MainMenuTask) vTaskResume(MainMenuTask);
                            break;

                        case PlayingState:

                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(CreaturesActionControlTask) vTaskResume(CreaturesActionControlTask);
                            if(SaucerActionControlTask) vTaskResume(SaucerActionControlTask);
                            if(MainPlayingGameTask) vTaskResume(MainPlayingGameTask);
                            break;

                        case PausedState:

                            if(CreaturesActionControlTask) vTaskSuspend(CreaturesActionControlTask);
                            if(SaucerActionControlTask) vTaskSuspend(SaucerActionControlTask);
                            if(MainPlayingGameTask) vTaskSuspend(MainPlayingGameTask);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(PausedGameTask) vTaskResume(PausedGameTask);
                            break;

                        case GameOverState:

                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            vRecreateGame();
                            if(GameOverTask) vTaskResume(GameOverTask);
                            break;
                        
                        case NextLevelState:

                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            vRecreateGame();
                            if(NextLevelTask) vTaskResume(NextLevelTask);
                            break;

                        case ResetGameState:

                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            vRecreateGame();
                            if(MainMenuTask) vTaskResume(MainMenuTask);
                            break;
                        default:
                            break;
                    }

                    GameStateBuffer.GameState=current_state;
                    changed_state=0; //Resets changed state

                xSemaphoreGive(GameStateBuffer.lock);
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
                    configMAX_PRIORITIES-1, &SwapBuffers) != pdPASS) {
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
    
    StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char)); 
    if(!StateQueue){
        PRINT_ERROR("Could not open State Queue.");
        goto err_statequeue;
    }
    if (xTaskCreate(vStateMachine, "StateMachine", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-2, &StateMachine) != pdPASS) {
        goto err_statemachine;
    }
    
    if (xTaskCreate(vTaskMainMenu, "MainMenuTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-4, &MainMenuTask) != pdPASS) {
        goto err_mainmenu;
    }

    if (xTaskCreate(vTaskPlayingGame, "MainPlayingGameTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-4, &MainPlayingGameTask) != pdPASS) {
        goto err_MainPlayingGameTask;
    }
    
    GameStateBuffer.lock = xSemaphoreCreateMutex();
    if(!GameStateBuffer.lock){
        PRINT_ERROR("Failed to create Game State buffer lock.");
        goto err_gamestatebuffer;
    }
    
    OutsideGameActionsBuffer.lock = xSemaphoreCreateMutex();
    if(!OutsideGameActionsBuffer.lock){
        PRINT_ERROR("Failed to create Outside actions buffer lock."); 
        goto err_outsideactionsbuffer;
    }

    MainMenuInfoBuffer.lock = xSemaphoreCreateMutex();
    if(!MainMenuInfoBuffer.lock){
        PRINT_ERROR("Failed to create Main Menu Info Buffer lock.");
        goto err_mainmenuinfobuffer;
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

    if (xTaskCreate(vTaskShipBulletControl, "ShipBulletControlTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES - 5, &ShipBulletControlTask)!= pdPASS){
        PRINT_ERROR("Failed to create ShipBulletControlTask.");
        goto err_shipbulletcontrol;
    }

    BunkersBuffer.lock = xSemaphoreCreateMutex();
    if(!BunkersBuffer.lock){
        PRINT_ERROR("Failed to create Bunkers Buffer lock.");
        goto err_bunkersbuffer;
    }

    if (xTaskCreate(vTaskBunkerShotControl, "BunkerShotControlTask", mainGENERIC_STACK_SIZE*2, NULL,
                    configMAX_PRIORITIES - 5, &BunkerShotControlTask)!=pdPASS){
        PRINT_ERROR("Failed to create BunkerShotControlTask.");
        goto err_bunkercontroltask;
    }
    
    if (xTaskCreate(vTaskCreaturesShotControl, "CreatureShotControlTask", mainGENERIC_STACK_SIZE*2, NULL,
                    configMAX_PRIORITIES - 5, &CreaturesShotControlTask)!=pdPASS){
        PRINT_ERROR("Failed to create CreatureShotControlTask.");
        goto err_creatureshotcontroltask;
    }

    CreaturesBuffer.lock = xSemaphoreCreateMutex();
    if(!CreaturesBuffer.lock){
        PRINT_ERROR("Failed to create Creatures Buffer lock.");
        goto err_creaturesbuffer;
    }

    if(xTaskCreate(vTaskCreaturesActionControl, "CreaturesActionControl", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 3, &CreaturesActionControlTask)!=pdPASS){
        PRINT_ERROR("Failed to create CreaturesMovmentControlTask.");
        goto err_creaturecontrolactiontask;
    }
    
    if(xTaskCreate(vTaskCreaturesBulletControl, "CreaturesBulletControl", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 5, &CreaturesBulletControlTask)!=pdPASS){
        PRINT_ERROR("Failed to create CreaturesBulletControlTask.");
        goto err_creaturesbulletcontroltask;
    }

    if(xTaskCreate(vTaskShipShotControl, "ShipShotControl", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 5, &ShipShotControlTask)!=pdPASS){
        PRINT_ERROR("Failed to create ShipShotControlTask.") ;
        goto err_shipshotcontroltask;
    }

    if(xTaskCreate(vTaskPausedGame, "PausedGame", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 4, &PausedGameTask)!=pdPASS) {
        PRINT_ERROR("Failed to create PausedGameTask.");
        goto err_pausedgametask;
    }
    
    PausedGameInfoBuffer.lock = xSemaphoreCreateMutex();
    if(!PausedGameInfoBuffer.lock){
        PRINT_ERROR("Failed to create Paused Game Info lock.");
        goto err_pausedgameinfolock;
    }

    if(xTaskCreate(vTaskGameOver, "GameOver", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 4, &GameOverTask)!=pdPASS){
        PRINT_ERROR("Failed to create Game Over Task.");
        goto err_gameovertask;
    }

    GameOverInfoBuffer.lock = xSemaphoreCreateMutex();
    if(!GameOverInfoBuffer.lock){
        PRINT_ERROR("Failed to create Game over info buffer lock.");
        goto err_gameoverinfolock;
    }

    if(xTaskCreate(vTaskNextLevel, "NextLevelTask", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 4, &NextLevelTask)!=pdPASS){
        PRINT_ERROR("Failed to create NextLevelTask.");
        goto err_nextleveltask;
    }

    if(xTaskCreate(vTaskBunkerCreaturesCrashed, "BunkerCreaturesCrashedTask", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 5, &BunkerCreaturesCrashedTask)!=pdPASS){
        PRINT_ERROR("Failed to create BunkerCreaturesCrashedTask.");
        goto err_bunkerscreaturescrashed;
    }
    
    LevelModifiersBuffer.lock = xSemaphoreCreateMutex();
    if(!LevelModifiersBuffer.lock){
        PRINT_ERROR("Failed to create Level modifier lock");
        goto err_levelmodifierlock;
    }

    SaucerBuffer.lock = xSemaphoreCreateMutex();
    if(!SaucerBuffer.lock){
        PRINT_ERROR("Failed to create Saucer lock");
        goto err_saucerlock;
    }

    if(xTaskCreate(vTaskSaucerActionControl, "SaucerActionControlTask", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 3, &SaucerActionControlTask)!=pdPASS){
         
        PRINT_ERROR("Failed to create SaucerActionControlTask.");
        goto err_sauceractiontask;
    }

    if(xTaskCreate(vTaskSaucerShotControl, "SaucerShotControlTask", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 5, &SaucerShotControlTask)!=pdPASS){
         
        PRINT_ERROR("Failed to create SaucerShotControlTask.");
        goto err_saucershottask;
    }


    vTaskSuspend(MainMenuTask);
    vTaskSuspend(MainPlayingGameTask);
    vTaskSuspend(PausedGameTask);
    vTaskSuspend(GameOverTask);
    vTaskSuspend(NextLevelTask);
    vTaskSuspend(ShipBulletControlTask);
    vTaskSuspend(BunkerShotControlTask);
    vTaskSuspend(BunkerCreaturesCrashedTask);
    vTaskSuspend(CreaturesShotControlTask);
    vTaskSuspend(CreaturesActionControlTask);
    vTaskSuspend(SaucerActionControlTask);
    vTaskSuspend(SaucerShotControlTask);
    vTaskSuspend(CreaturesBulletControlTask);
    vTaskSuspend(ShipShotControlTask);
    
    vTaskStartScheduler();

    return EXIT_SUCCESS;
err_saucershottask:
    vTaskDelete(SaucerActionControlTask);
err_sauceractiontask:
    vSemaphoreDelete(SaucerBuffer.lock);
err_saucerlock:
    vSemaphoreDelete(LevelModifiersBuffer.lock);
err_levelmodifierlock:
    vTaskDelete(BunkerCreaturesCrashedTask);
err_bunkerscreaturescrashed:
    vTaskDelete(NextLevelTask);
err_nextleveltask:
    vSemaphoreDelete(GameOverInfoBuffer.lock);
err_gameoverinfolock:
    vTaskDelete(GameOverTask);
err_gameovertask:
    vSemaphoreDelete(PausedGameInfoBuffer.lock);
err_pausedgameinfolock:
    vTaskDelete(PausedGameTask);
err_pausedgametask:
    vTaskDelete(ShipShotControlTask);
err_shipshotcontroltask:
    vTaskDelete(CreaturesBulletControlTask);
err_creaturesbulletcontroltask:
    vTaskDelete(CreaturesActionControlTask);        
err_creaturecontrolactiontask:
    vSemaphoreDelete(CreaturesBuffer.lock);
err_creaturesbuffer:
    vTaskDelete(CreaturesShotControlTask);
err_creatureshotcontroltask:
    vTaskDelete(BunkerShotControlTask);
err_bunkercontroltask:
    vSemaphoreDelete(BunkersBuffer.lock);
err_bunkersbuffer:
    vTaskDelete(ShipBulletControlTask);
err_shipbulletcontrol:
    vSemaphoreDelete(PlayerInfoBuffer.lock);
err_playerinfo:
    vSemaphoreDelete(ShipBuffer.lock);
err_shipbuffer:
    vSemaphoreDelete(MainMenuInfoBuffer.lock);
err_mainmenuinfobuffer:
    vSemaphoreDelete(OutsideGameActionsBuffer.lock);
err_outsideactionsbuffer:
    vSemaphoreDelete(GameStateBuffer.lock);
err_gamestatebuffer:
    vTaskDelete(MainPlayingGameTask);
err_MainPlayingGameTask:
    vTaskDelete(MainMenuTask);
err_mainmenu:
    vTaskDelete(StateMachine);
err_statemachine:
    vQueueDelete(StateQueue);
err_statequeue:
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
