#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include <SDL2/SDL_scancode.h>

/** FreeRTOS related */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/** TUM library related*/
#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"

/** AsyncIO related */
#include "AsyncIO.h"
#define UDP_BUFFER_SIZE 1024
#define UDP_RECEIVE_PORT 1234
#define UDP_TRANSMIT_PORT 1235

/**Game related */
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

///Tasks
static TaskHandle_t MainMenuTask = NULL;
static TaskHandle_t MainPlayingGameTask = NULL;
static TaskHandle_t PausedGameTask = NULL;
static TaskHandle_t GameOverTask = NULL;
static TaskHandle_t NextLevelTask = NULL;
static TaskHandle_t CheatsTask = NULL;
static TaskHandle_t ShipBulletControlTask = NULL;
static TaskHandle_t BunkerShotControlTask = NULL;
static TaskHandle_t BunkerCreaturesCrashedTask = NULL;
static TaskHandle_t CreaturesShotControlTask = NULL;
static TaskHandle_t CreaturesActionControlTask = NULL;
static TaskHandle_t CreaturesBulletControlTask = NULL;
static TaskHandle_t ShipShotControlTask = NULL;
static TaskHandle_t SaucerActionControlTask = NULL;
static TaskHandle_t SaucerShotControlTask = NULL;

static TaskHandle_t UDPControlTask = NULL;
static TaskHandle_t SaucerAIControlTask = NULL;
static TaskHandle_t SwapBuffers = NULL;
static TaskHandle_t StateMachine = NULL;

#define STATE_DEBOUNCE_DELAY 250
#define STATE_QUEUE_LENGTH 1
///Queues
static QueueHandle_t StateQueue = NULL;
static QueueHandle_t NextKEYQueue = NULL;
static QueueHandle_t PauseResumeAIQueue = NULL;
static QueueHandle_t ShipPosQueue = NULL;
static QueueHandle_t SaucerPosQueue = NULL;

///Images
static image_handle_t TitleScreen = NULL;
static image_handle_t GameOver = NULL;
static image_handle_t NextLevel = NULL;
static image_handle_t PlayerShip = NULL;
static image_handle_t Bunker = NULL;
static image_handle_t BunkerHit1 = NULL;
static image_handle_t BunkerHit2 = NULL;
static image_handle_t BunkerHit3 = NULL;
static image_handle_t BunkerHit4 = NULL;
static image_handle_t CreatureMEDIUM_0 = NULL;
static image_handle_t CreatureMEDIUM_1 = NULL;
static image_handle_t CreatureEASY_0 = NULL;
static image_handle_t CreatureEASY_1 = NULL;
static image_handle_t CreatureHARD_0 = NULL;
static image_handle_t CreatureHARD_1 = NULL;
static image_handle_t SaucerBoss = NULL;
static image_handle_t SaucerAIBoss = NULL;
static image_handle_t CreaturesEasy_MenuScaled_0 = NULL;
static image_handle_t CreaturesEasy_MenuScaled_1 = NULL;
static image_handle_t CreaturesMedium_MenuScaled_0 = NULL;
static image_handle_t CreaturesMedium_MenuScaled_1 = NULL;
static image_handle_t CreaturesHard_MenuScaled_0 = NULL;
static image_handle_t CreaturesHard_MenuScaled_1 = NULL;
static image_handle_t CreatureShotAnimation = NULL;
static image_handle_t SaucerShotAnimation = NULL;
static image_handle_t WallShotAnimation = NULL;

///Semaphore/Mutexs
static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t HandleUDP = NULL;

///UDP Sockets
aIO_handle_t UDP_SOC_RECEIVE = NULL;
aIO_handle_t UDP_SOC_TRANSMIT = NULL;  

///Signals used to change states.
const unsigned char MainMenuStateSignal=MainMenuState;
const unsigned char SinglePlayingStateSignal=SinglePlayingState;;
const unsigned char MultiPlayingStateSignal=MultiPlayingState;;
const unsigned char PausedGameStateSignal=PausedState;
const unsigned char GameOverStateSignal=GameOverState;
const unsigned char NextLevelStateSignal=NextLevelState;
const unsigned char ResetGameStateSignal=ResetGameState;
const unsigned char CheatsStateSignal=CheatsState;

///Global Structs with protective locks.
typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    unsigned char ENTER_DEBOUNCE_STATE;
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
    unsigned short ImageIndex;
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

    unsigned int NewLivesAddedThreshold;
    unsigned short FreshGame;
    GameState_t PlayerChosenMode;
    SemaphoreHandle_t lock;
}PlayerBuffer_t;
static PlayerBuffer_t PlayerInfoBuffer = { 0 };

typedef struct BunkersBuffer_t{
    bunkers_t* Bunkers;
    image_handle_t ImagesCatalog[5];
    SemaphoreHandle_t lock;
}BunkersBuffer_t;
static BunkersBuffer_t BunkersBuffer = { 0 };

typedef struct CreaturesBuffer_t{
    creature_t* Creatures;
    signed short FrontierCreaturesID[8];
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
    SelectedPausedGameOption_t SelectedPausedGameOption;
    SemaphoreHandle_t lock;
}PausedGameInfoBuffer_t;
static PausedGameInfoBuffer_t PausedGameInfoBuffer = { 0 };

typedef struct CheatsInfoBuffer_t{
    SelectedCheatsOption_t SelectedCheatsOption;
    unsigned int StartingScoreValue;
    unsigned int StartingLevelValue;
    SemaphoreHandle_t lock;
}CheatsInfoBuffer_t;
static CheatsInfoBuffer_t CheatsInfoBuffer = { 0 };

typedef struct GameOverInfoBuffer_t{
    SelectedGameOverOption_t SelectedGameOverOption;
    SemaphoreHandle_t lock;
}GameOverInfoBuffer_t;
static GameOverInfoBuffer_t GameOverInfoBuffer = { 0 };

typedef struct AnimationsBuffer_t{
    TickType_t LivesColorRedTimer;
    LivesAnimation_t LivesCondition;
    unsigned char LivesTimerSet;

    TickType_t CreatureShotAnimationTimer;
    unsigned char CreatureShot;
    unsigned char CreatureShotAnimationTimerSet;
    signed short CreatureShotX;
    signed short CreatureShotY;
    
    TickType_t SaucerShotAnimationTimer;
    unsigned char SaucerShotAnimationTimerSet;
    unsigned char SaucerShot;
    signed short SaucerShotX;
    signed short SaucerShotY;

    TickType_t WallShotAnimationTimer;
    unsigned char WallShotAnimationTimerSet;
    unsigned char WallShot;
    signed short WallShotX;

    SemaphoreHandle_t lock;
}AnimationsBuffer_t;
AnimationsBuffer_t AnimationsBuffer = { 0 };

//Retrieves the latest button entries.
void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

//Draw wrapper function that throws a message in case of drawing errors.
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

//Draws FPS on the screen.
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
//Sound playing functions.
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
    tumSoundPlaySample(d5);
}
void vPlayBulletSound()
{
    tumSoundPlaySample(a3);
}
/** Set-Functions */
///  These functions are usually declared in the code block previous to the running loops of tasks.
void vSetMainMenuBufferValues() ///Ex: Initialize values within the MainMenuInfoBuffer global struct.
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
    SaucerAIBoss=tumDrawLoadImage("../resources/AIsaucer.bmp");
}
void vSetPlayersInfoBufferValues()
{
    xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY);

        PlayerInfoBuffer.HiScore=0;
        PlayerInfoBuffer.Score=0;
        PlayerInfoBuffer.LivesLeft = 3;
        PlayerInfoBuffer.Level = 1;    
        PlayerInfoBuffer.FreshGame=1;
        PlayerInfoBuffer.NewLivesAddedThreshold=INITIAL_POINTS_THRESHOLD;

        PlayerInfoBuffer.PlayerChosenMode = SinglePlayingState;

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
    PlayerShip=tumDrawLoadImage("../resources/player.bmp");
    xSemaphoreTake(ShipBuffer.lock, portMAX_DELAY);

        ShipBuffer.Ship = CreateShip(SCREEN_WIDTH/2,PLAYERSHIP_Y_BEGIN,
                                     SHIPSPEED,
                                     Green, 
                                     SHIPSIZE);
        ShipBuffer.BulletCrashed=0;

    xSemaphoreGive(ShipBuffer.lock);
}
void vSetSaucerBufferValues()
{

    xSemaphoreTake(SaucerBuffer.lock, portMAX_DELAY);
        SaucerBuffer.saucer = CreateSaucer();
        vPrepareImageSaucer(&SaucerBuffer.ImageIndex);
        SaucerBuffer.saucer->Images[0]= SaucerBoss;
        SaucerBuffer.saucer->Images[1] = SaucerAIBoss;
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
        vAssignFrontierCreatures(CreaturesBuffer.FrontierCreaturesID);
        vAssignCreaturesImages(CreaturesBuffer.Creatures,
                               CreaturesBuffer.ImagesCatalog);

    xSemaphoreGive(CreaturesBuffer.lock);
}

void vSetBunkersBufferValues()
{
    xSemaphoreTake(BunkersBuffer.lock, portMAX_DELAY);

        Bunker = tumDrawLoadImage("../resources/bunker.bmp");
        BunkerHit1 = tumDrawLoadImage("../resources/bunker_hit1.bmp");
        BunkerHit2 = tumDrawLoadImage("../resources/bunker_hit2.bmp");
        BunkerHit3 = tumDrawLoadImage("../resources/bunker_hit3.bmp");
        BunkerHit4 = tumDrawLoadImage("../resources/bunker_hit4.bmp");

        BunkersBuffer.ImagesCatalog[0]=Bunker;
        BunkersBuffer.ImagesCatalog[1]=BunkerHit1;
        BunkersBuffer.ImagesCatalog[2]=BunkerHit2;
        BunkersBuffer.ImagesCatalog[3]=BunkerHit3;
        BunkersBuffer.ImagesCatalog[4]=BunkerHit4;

        BunkersBuffer.Bunkers = CreateBunkers();

    xSemaphoreGive(BunkersBuffer.lock);
}

void vSetPausedGameInfoBufferValues()
{
    xSemaphoreTake(PausedGameInfoBuffer.lock, portMAX_DELAY);
    
        PausedGameInfoBuffer.SelectedPausedGameOption = Resume;

    xSemaphoreGive(PausedGameInfoBuffer.lock);
}

void vSetGameOverInfoBufferValues()
{
    xSemaphoreTake(GameOverInfoBuffer.lock, portMAX_DELAY);

        GameOverInfoBuffer.SelectedGameOverOption = PlayAgain;

    xSemaphoreGive(GameOverInfoBuffer.lock);
}

void vSetCheatsInfoBufferValues()
{
    xSemaphoreTake(CheatsInfoBuffer.lock, portMAX_DELAY);

        CheatsInfoBuffer.SelectedCheatsOption = InfiniteLives;
        CheatsInfoBuffer.StartingLevelValue=1;
        CheatsInfoBuffer.StartingScoreValue=0;

    xSemaphoreGive(CheatsInfoBuffer.lock);
}

void vSetLevelModifiersValues()
{
    xSemaphoreTake(LevelModifiersBuffer.lock, portMAX_DELAY);
        xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY);

            if(PlayerInfoBuffer.Level<3){
                LevelModifiersBuffer.NumberOfSpeedChanges = 0;
                LevelModifiersBuffer.SpeedChangeCount = 5 - 1 * (PlayerInfoBuffer.Level - 1);
                LevelModifiersBuffer.MovingPeriod = 700 - 50*(PlayerInfoBuffer.Level - 1);
                LevelModifiersBuffer.AnimationPeriod = LevelModifiersBuffer.MovingPeriod;
                LevelModifiersBuffer.ShootingPeriod = 2000 - 250* (PlayerInfoBuffer.Level - 1);
            }
            else {
                LevelModifiersBuffer.NumberOfSpeedChanges = 0;
                LevelModifiersBuffer.SpeedChangeCount = 4;
                LevelModifiersBuffer.MovingPeriod = 600;
                LevelModifiersBuffer.AnimationPeriod = LevelModifiersBuffer.MovingPeriod;
                LevelModifiersBuffer.ShootingPeriod = 2000 - 250* (PlayerInfoBuffer.Level - 1);
            }

        xSemaphoreGive(PlayerInfoBuffer.lock);
    xSemaphoreGive(LevelModifiersBuffer.lock);
}

void vSetAnimationsBufferValues()
{
    CreatureShotAnimation = tumDrawLoadImage("../resources/destroyedv2.bmp");
    SaucerShotAnimation = tumDrawLoadImage("../resources/destroyedv2.bmp");
    WallShotAnimation = tumDrawLoadImage("../resources/ShipBulletTopDMGv2.bmp");

    xSemaphoreTake(AnimationsBuffer.lock, portMAX_DELAY);

        AnimationsBuffer.LivesCondition=LivesIntact;
        AnimationsBuffer.LivesColorRedTimer=0;
        AnimationsBuffer.LivesTimerSet=0;

        AnimationsBuffer.CreatureShotAnimationTimer=0;
        AnimationsBuffer.CreatureShotAnimationTimerSet=0;
        AnimationsBuffer.CreatureShot=0;
        AnimationsBuffer.CreatureShotX=0;
        AnimationsBuffer.CreatureShotY=0;

        AnimationsBuffer.SaucerShotAnimationTimer=0;
        AnimationsBuffer.SaucerShotAnimationTimerSet=0;
        AnimationsBuffer.SaucerShot=0;
        AnimationsBuffer.SaucerShotX=0;
        AnimationsBuffer.SaucerShotY=0;

        AnimationsBuffer.WallShotAnimationTimer=0;
        AnimationsBuffer.WallShotAnimationTimerSet=0;
        AnimationsBuffer.WallShot=0;
        AnimationsBuffer.WallShotX=0;


    xSemaphoreGive(AnimationsBuffer.lock);
}

//Changes which MotherShip Image being used depending on Single or Multi- Player
void vPrepareImageSaucer(unsigned short* ImageSaucerIndex)
{
    xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY);
        if(PlayerInfoBuffer.PlayerChosenMode == SinglePlayingState)
            (*ImageSaucerIndex) = 0;
        else
            (*ImageSaucerIndex) = 1;
    xSemaphoreGive(PlayerInfoBuffer.lock);
}
//Resets Player Values depending on the type of game being created
void vPrepareGameValues(TypesOfNewGames_t TypeOfNewGame)
{ 
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){

        switch(TypeOfNewGame){
            case(NewGameFromScratch):
                PlayerInfoBuffer.Score=0;
                PlayerInfoBuffer.Level=1;
                PlayerInfoBuffer.FreshGame=0;
                PlayerInfoBuffer.LivesLeft=3;
                PlayerInfoBuffer.NewLivesAddedThreshold = INITIAL_POINTS_THRESHOLD;
                xSemaphoreGive(PlayerInfoBuffer.lock);
                break;

            case(NewGameNextLevel):
                PlayerInfoBuffer.Level++;
                PlayerInfoBuffer.FreshGame=0;
                PlayerInfoBuffer.Score=PlayerInfoBuffer.Score;
                xSemaphoreGive(PlayerInfoBuffer.lock);
                break;
        
            case(InfiniteLivesCheat):
                PlayerInfoBuffer.LivesLeft=6000;
                xSemaphoreGive(PlayerInfoBuffer.lock);
                break;

            case(ChooseStartingScoreCheat):
                if(xSemaphoreTake(CheatsInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                    PlayerInfoBuffer.Score=CheatsInfoBuffer.StartingScoreValue;
                    xSemaphoreGive(CheatsInfoBuffer.lock);
                }
                vSetCheatsInfoBufferValues();
                xSemaphoreGive(PlayerInfoBuffer.lock);
                break;

            case(ChooseStartingLevelCheat):
                if(xSemaphoreTake(CheatsInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                    PlayerInfoBuffer.Level=CheatsInfoBuffer.StartingLevelValue;
                    xSemaphoreGive(CheatsInfoBuffer.lock);
                }
                vSetCheatsInfoBufferValues();
                xSemaphoreGive(PlayerInfoBuffer.lock);
                break;

            default:
                xSemaphoreGive(PlayerInfoBuffer.lock);
                break;
        }
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}

//Draws Player Score and High Score
void vDrawStaticTexts(void)
{
    char Score_1[20];   
    char Score_2[20];   
    char Hi_Score[20];   
    
    static int Score1_Width,Score2_Width,HiScoreWidth=0;
    
    if(xSemaphoreTake(PlayerInfoBuffer.lock, 0)==pdTRUE){

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

//Draws the animated creatures and their respective point values on the screen
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

//Draws Space Invaders Image on the screen
void vDrawSpaceInvadersBanner()
{
    checkDraw(tumDrawLoadedImage(TitleScreen,
                                 SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(TitleScreen)/2,
                                 SCREEN_HEIGHT*3/10-tumDrawGetLoadedImageHeight(TitleScreen)/2),
                                 __FUNCTION__);
}

//Draws Main Menu Options on the screen
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

    if(xSemaphoreTake(MainMenuInfoBuffer.lock, 0)==pdTRUE){  

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

//Checks for User's Enter Input
unsigned char xCheckEnterPressed()
{
    if(xSemaphoreTake(buttons.lock, 0)==pdTRUE){
        if(buttons.buttons[KEYCODE(RETURN)]){
            if(buttons.buttons[KEYCODE(RETURN)]!=buttons.ENTER_DEBOUNCE_STATE){
                buttons.ENTER_DEBOUNCE_STATE=buttons.buttons[KEYCODE(RETURN)];
                xSemaphoreGive(buttons.lock);
                return 1;
            }
        }
        buttons.ENTER_DEBOUNCE_STATE=buttons.buttons[KEYCODE(RETURN)];
        xSemaphoreGive(buttons.lock);
        return 0;
    }
    return 0;
}
//Checks for User's UP/DOWN Input, changing highlighted displayed options in Menu
void  xCheckMenuSelectionChange(unsigned char* UP_DEBOUNCE_STATE, 
                                unsigned char* DOWN_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

        if (buttons.buttons[KEYCODE(DOWN)]) {
            if(buttons.buttons[KEYCODE(DOWN)]!=(*DOWN_DEBOUNCE_STATE))
                if(xSemaphoreTake(MainMenuInfoBuffer.lock,0)==pdTRUE){  
                    vDownMenuSelection(&MainMenuInfoBuffer.SelectedMenuOption);
                    xSemaphoreGive(MainMenuInfoBuffer.lock);
                }
        }                            
        else if (buttons.buttons[KEYCODE(UP)]) { 
            if(buttons.buttons[KEYCODE(UP)]!=(*UP_DEBOUNCE_STATE))
                if(xSemaphoreTake(MainMenuInfoBuffer.lock,0)==pdTRUE){  
                    vUpMenuSelection(&MainMenuInfoBuffer.SelectedMenuOption);
                    xSemaphoreGive(MainMenuInfoBuffer.lock);
                }
        }   

        (*UP_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(UP)];
        (*DOWN_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(DOWN)];
        xSemaphoreGive(buttons.lock);                                                                                                                                                                
    }   
}

/**BEGIN - Main Menu Task */
void vTaskMainMenu(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xPrevAnimatedTime = 0;
    const TickType_t UpdatePeriod = 20;
    const TickType_t AnimationPeriod = 400;
   
    //Debouncing variables
    unsigned char UP_DEBOUNCE_STATE = 0;
    unsigned char DOWN_DEBOUNCE_STATE = 0;

    //Index used to toggle images for animation purposes
    unsigned char ImageAnimationIndex=0;

    //Set functions used to initialize relevant global structs
    vSetMainMenuLoadedImages() ;
    vSetPlayersInfoBufferValues();
    vSetMainMenuBufferValues();


    while (1) {
        xGetButtonInput(); 
        xCheckMenuSelectionChange(&UP_DEBOUNCE_STATE,   //Moves highlighting of current users selection 
                                  &DOWN_DEBOUNCE_STATE);

        if(xCheckEnterPressed()) //if Enter is pressed, user chose an option
                vHandleStateMachineActivation(); //!Goes to a pre-handler of the State Machine task.

        if(xTaskGetTickCount() - xPrevAnimatedTime > AnimationPeriod){ //Controls Animation of figures on screen
            ImageAnimationIndex=!ImageAnimationIndex; 
            xPrevAnimatedTime = xTaskGetTickCount();
        }

        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xSemaphoreTake(ScreenLock,portMAX_DELAY);

                    tumDrawClear(Black); 
                    vDrawStaticTexts(); //Draws Scores
                    vDrawPointsExplanation(ImageAnimationIndex);//Draws Animated enemies and their point values
                    vDrawSpaceInvadersBanner();
                    vDrawMainMenuOptions();//Draws players options
                    vDrawFPS();

                xSemaphoreGive(ScreenLock);
                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
        }
}

//Draws possible keyboard inputs in the game
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

//Draws current level on the screen
void vDrawLevel()
{
    static char str[20] = { 0 };
    static int strWidth = { 0 };
    if(xSemaphoreTake(PlayerInfoBuffer.lock,0)==pdTRUE){
        sprintf(str,"Level [ %d ]", PlayerInfoBuffer.Level);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
    if(xSemaphoreTake(AnimationsBuffer.lock, 0)==pdTRUE){
        if(!tumGetTextSize((char*)str, &strWidth, NULL))
            checkDraw(tumDrawText(str,155- strWidth/2,
                                  SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                                  White), 
                                  __FUNCTION__);
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Draws players lives on the screen
void vDrawLives()
{   
    static char str[100] = { 0 };
    static int strWidth;
    
    if(xSemaphoreTake(PlayerInfoBuffer.lock,0)==pdTRUE){
        sprintf(str,"Lives: [ %d ]", PlayerInfoBuffer.LivesLeft);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }

    if(!tumGetTextSize((char*)str, &strWidth, NULL))
        checkDraw(tumDrawText(str, 55 - strWidth/2, 
                              SCREEN_HEIGHT*97/100 - DEFAULT_FONT_SIZE/2,
                              xFetchAnimationColor(AnimationsBuffer.LivesCondition)),
                              __FUNCTION__);
}

//Draws a single creature, will be called within a loop to draw all creatures
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

//Draws Saucer-Shot-Animation on screen
void vDrawSaucerDestruction()
{
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        checkDraw(tumDrawLoadedImage(SaucerShotAnimation, 
                                     AnimationsBuffer.SaucerShotX - SAUCER_SHOT_ANIMATION_W/2,
                                     AnimationsBuffer.SaucerShotY - SAUCER_SHOT_ANIMATION_H/2 - SAUCER_HEIGHT/2),
                                     __FUNCTION__);
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Draws the Saucer/Mothership
void vDrawSaucer()
{
    if(xSemaphoreTake(SaucerBuffer.lock, 0)==pdTRUE){
        checkDraw(tumDrawLoadedImage(SaucerBuffer.saucer->Images[SaucerBuffer.ImageIndex],
                                     SaucerBuffer.saucer->x_pos-SAUCER_WIDTH/2,
                                     SaucerBuffer.saucer->y_pos-SAUCER_WIDTH/2),
                                     __FUNCTION__);
        xSemaphoreGive(SaucerBuffer.lock);
    }
}

//Draws Wall-Shot-Animation on screen
void vDrawWallShot()
{
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        checkDraw(tumDrawLoadedImage(WallShotAnimation, 
                                     AnimationsBuffer.WallShotX - WALL_SHOT_ANIMATION_W/2,
                                     UPPER_WALL_LIMIT - WALL_SHOT_ANIMATION_H/2),
                                     __FUNCTION__);
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Draws Creatures-Shot-Animation on screen
void vDrawCreatureDestruction()
{
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        checkDraw(tumDrawLoadedImage(CreatureShotAnimation, 
                                     AnimationsBuffer.CreatureShotX - CREATURE_SHOT_ANIMATION_W/2,
                                     AnimationsBuffer.CreatureShotY - CREATURE_SHOT_ANIMATION_H/2),
                                     __FUNCTION__);
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Draws Creatures on screen
void vDrawCreatures()
{
    unsigned char CreatureCountID=0;
    if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){
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

//Alternates Creatures images for animation purposes
void vAnimateCreatures()
{
    unsigned char creatureIDcount = 0;

    while(creatureIDcount < NUMB_OF_CREATURES){      
        if(CreaturesBuffer.Creatures[creatureIDcount].Alive == 1)
            vAlternateAnimation(&CreaturesBuffer.Creatures[creatureIDcount]);
        ++creatureIDcount;
    }

}

//Draws all bunkers
void vDrawBunkers()
{
    if(xSemaphoreTake(BunkersBuffer.lock, 0)==pdTRUE){  

        if(BunkersBuffer.Bunkers->b1Lives>0)
            checkDraw(tumDrawLoadedImage(BunkersBuffer.ImagesCatalog[5 - BunkersBuffer.Bunkers->b1Lives], //Each image in image catalog represents a state of the bunker
                                         BunkersBuffer.Bunkers->b1->x_pos - BunkersBuffer.Bunkers->b1->size/2,
                                         BunkersBuffer.Bunkers->b1->y_pos - BunkersBuffer.Bunkers->b1->size/2),
                                         __FUNCTION__);

        if(BunkersBuffer.Bunkers->b2Lives>0)
            checkDraw(tumDrawLoadedImage(BunkersBuffer.ImagesCatalog[5 - BunkersBuffer.Bunkers->b2Lives],
                                         BunkersBuffer.Bunkers->b2->x_pos - BunkersBuffer.Bunkers->b2->size/2,
                                         BunkersBuffer.Bunkers->b2->y_pos - BunkersBuffer.Bunkers->b2->size/2),
                                         __FUNCTION__);

        if(BunkersBuffer.Bunkers->b3Lives>0)
            checkDraw(tumDrawLoadedImage(BunkersBuffer.ImagesCatalog[5 - BunkersBuffer.Bunkers->b3Lives],
                                         BunkersBuffer.Bunkers->b3->x_pos - BunkersBuffer.Bunkers->b3->size/2,
                                         BunkersBuffer.Bunkers->b3->y_pos - BunkersBuffer.Bunkers->b3->size/2),
                                         __FUNCTION__);

        if(BunkersBuffer.Bunkers->b4Lives>0)
            checkDraw(tumDrawLoadedImage(BunkersBuffer.ImagesCatalog[5 - BunkersBuffer.Bunkers->b4Lives],
                                         BunkersBuffer.Bunkers->b4->x_pos - BunkersBuffer.Bunkers->b4->size/2,
                                         BunkersBuffer.Bunkers->b4->y_pos - BunkersBuffer.Bunkers->b4->size/2),
                                         __FUNCTION__);

        xSemaphoreGive(BunkersBuffer.lock);
    }
}

//Draws lower green wall
void vDrawLowerWall()
{
    checkDraw(tumDrawLine(0, BOTTOM_WALLPOSITION,
                          SCREEN_WIDTH, BOTTOM_WALLPOSITION,
                          BOTTOM_WALLTHICKNESS,Green),
                          __FUNCTION__);
}

//Draws Players Ship
void vDrawShip()
{
    if(xSemaphoreTake(ShipBuffer.lock,0)==pdTRUE){
        checkDraw(tumDrawLoadedImage(PlayerShip, 
                                     ShipBuffer.Ship->x_pos - PLAYERSHIP_WIDTH/2,
                                     ShipBuffer.Ship->y_pos - PLAYERSHIP_HEIGHT/2),
                                     __FUNCTION__);
        xSemaphoreGive(ShipBuffer.lock);
    }
}

//Check for Space Input
unsigned char xCheckShipShoot(unsigned char* SPACE_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
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

//Check for left and right arrow key inputs
unsigned char xCheckShipMoved(signed short* LatestShipX)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if (buttons.buttons[KEYCODE(LEFT)]) {
            if(xSemaphoreTake(ShipBuffer.lock,0)==pdTRUE){  
                if(ShipBuffer.Ship->x_pos >= PLAYERSHIP_WIDTH*3/4)
                    vIncrementShipLeft(ShipBuffer.Ship);
                (*LatestShipX)=ShipBuffer.Ship->x_pos;
                xSemaphoreGive(ShipBuffer.lock);
                xSemaphoreGive(buttons.lock);
                return 1;
            }
            xSemaphoreGive(buttons.lock);
        }                            
        if (buttons.buttons[KEYCODE(RIGHT)]) { 
            if(xSemaphoreTake(ShipBuffer.lock,0)==pdTRUE){  
                if(ShipBuffer.Ship->x_pos <= SCREEN_WIDTH - PLAYERSHIP_WIDTH*3/4)
                    vIncrementShipRight(ShipBuffer.Ship); 
                (*LatestShipX)=ShipBuffer.Ship->x_pos;
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

//Checks for Creatures Bullet still being "alive" and wakes/triggers the task responsible for its control
void vTriggerCreaturesBulletControl()
{
   if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){
       vDrawCreaturesBullet(&CreaturesBuffer.CreaturesBullet);
       xSemaphoreGive(CreaturesBuffer.lock);
       vTaskResume(CreaturesBulletControlTask);
       xTaskNotify(CreaturesBulletControlTask, 0x01, eSetValueWithOverwrite);
   }
}

//Checks for Players Bullet still being "alive" and wakes/triggers the task responsible for its control
void vTriggerShipBulletControl()
{
    if(xSemaphoreTake(ShipBuffer.lock, 0)==pdTRUE){
        vDrawShipBullet(ShipBuffer.Ship);
        vTaskResume(ShipBulletControlTask);
        xTaskNotify(ShipBulletControlTask, 0x01, eSetValueWithOverwrite);
        xSemaphoreGive(ShipBuffer.lock);
    }
    xSemaphoreGive(ShipBuffer.lock);
}

//Activates/Begins/Puts in motion the Animation of a life being lost
void vActivateLivesAnimationState()
{
    if(xSemaphoreTake(AnimationsBuffer.lock, portMAX_DELAY)==pdTRUE){
        AnimationsBuffer.LivesCondition=LivesLost;
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

/**Being-Shot Control Task - Player Ship*/
void vTaskShipShotControl(void *pvParameters)
{
    while(1){
        uint32_t ShipShotSignal;
        if(xTaskNotifyWait(0x00, 0xffffffff, &ShipShotSignal, portMAX_DELAY)==pdTRUE){
            if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                vPlayShipShotSound();
                if(PlayerInfoBuffer.LivesLeft > 0)
                    PlayerInfoBuffer.LivesLeft-=1;
                vActivateLivesAnimationState();
                xSemaphoreGive(PlayerInfoBuffer.lock);
            }
        }
    }
}

//Controls Creatures speed after killing one of them
void vSpeedCreaturesControl(unsigned char* NumberOfCreaturesKilled)
{
    if(xSemaphoreTake(LevelModifiersBuffer.lock, 0)==pdTRUE){
        (*NumberOfCreaturesKilled)++;

        if((*NumberOfCreaturesKilled) >= LevelModifiersBuffer.SpeedChangeCount &&
           LevelModifiersBuffer.NumberOfSpeedChanges < 6){ 

            LevelModifiersBuffer.NumberOfSpeedChanges++;
            LevelModifiersBuffer.AnimationPeriod-=100;
            LevelModifiersBuffer.MovingPeriod-=100;
            (*NumberOfCreaturesKilled)=0;
        }
        xSemaphoreGive(LevelModifiersBuffer.lock);
    }
}

//Updates Player Score after hitting Creature
void vUpdatePlayerScoreCreatureKilled(unsigned char CreatureID)
{
    static unsigned char AddOn=0;
    AddOn = xFetchCreatureValue(CreatureID);
    PlayerInfoBuffer.Score+=AddOn;
    if(PlayerInfoBuffer.FreshGame==1 || PlayerInfoBuffer.Score > PlayerInfoBuffer.HiScore)
        PlayerInfoBuffer.HiScore=PlayerInfoBuffer.Score;
}

//Controls Player Score after hitting Creature
void vCreatureScoreControl(unsigned char CreatureCollisionID)
{
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        vUpdatePlayerScoreCreatureKilled(CreaturesBuffer.Creatures[CreatureCollisionID].CreatureType);
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}

//Activates/Begins/Puts in motion the Animation of a creature being shot
void vActivateCreatureDestroyedAnimationState(unsigned char CreatureCollisionID, creature_t* Creatures)
{
    signed short DeadCreatureX,DeadCreatureY;
    if(xSemaphoreTake(AnimationsBuffer.lock, portMAX_DELAY)==pdTRUE){
        vRetrieveDeadCreatureXY(&DeadCreatureX, &DeadCreatureY, CreaturesBuffer.Creatures[CreatureCollisionID]);
        AnimationsBuffer.CreatureShotX=DeadCreatureX;
        AnimationsBuffer.CreatureShotY=DeadCreatureY;
        AnimationsBuffer.CreatureShot=1;
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

/**Being-Shot Control Task - Creatures*/
void vTaskCreaturesShotControl(void *pvParameters)
{
    unsigned char NumberOfCreaturesKilled=0;
    while(1){
        uint32_t CreatureCollisionID;
        if(xTaskNotifyWait(0x00, 0xffffffff, &CreatureCollisionID, portMAX_DELAY)==pdTRUE){
            if(xSemaphoreTake(CreaturesBuffer.lock, portMAX_DELAY)==pdTRUE){ vKillCreature(&CreaturesBuffer.Creatures[CreatureCollisionID],
                              &CreaturesBuffer.NumbOfAliveCreatures);
                if(xCheckKilledCreatureWithinFrontier(CreatureCollisionID, CreaturesBuffer.FrontierCreaturesID))
                    vUpdateFrontierCreaturesIDs(CreaturesBuffer.FrontierCreaturesID,
                                                CreatureCollisionID,
                                                CreaturesBuffer.Creatures);

                vPlayDeadCreatureSound();
                vCreatureScoreControl(CreatureCollisionID); //Updates Score depending on creature-type shot
                vControlNewLivesAddition();//Check if player "deserves" new lives
                vSpeedCreaturesControl(&NumberOfCreaturesKilled);//Adjusts new speed setting given killed creature
                vActivateCreatureDestroyedAnimationState(CreatureCollisionID, CreaturesBuffer.Creatures);
                xSemaphoreGive(CreaturesBuffer.lock);
            }
       } 
    }
}

/**Creatures Crashed-into-Bunker Control Task*/
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

/**Being-Shot Control Task - Bunker*/
void vTaskBunkerShotControl(void *pvParameters)
{
    while(1){
        uint32_t BunkerCollisionID;

       if(xTaskNotifyWait(0x00, 0xffffffff, &BunkerCollisionID, portMAX_DELAY)==pdTRUE){
           if(xSemaphoreTake(BunkersBuffer.lock,portMAX_DELAY)==pdTRUE){
                vPlayBunkerShotSound();
                vUpdateBunkersStatus(BunkersBuffer.Bunkers, //Reduces "life points" of hit bunker
                                     BunkerCollisionID);
                xSemaphoreGive(BunkersBuffer.lock);
           }
       }
    
    }
}

//Activates/Begins/Puts in motion the Animation of the Wall being shot
void vActivateWallShotAnimationState()
{
    if(xSemaphoreTake(AnimationsBuffer.lock,portMAX_DELAY)==pdTRUE){
        AnimationsBuffer.WallShotX=ShipBuffer.Ship->bullet->x_pos;
        AnimationsBuffer.WallShot=1;
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Activates/Begins/Puts in motion the Animation of the Saucer being shot
void vActivateSaucerShotAnimationState(saucer_t* saucer)
{
    signed short DeadSaucerX, DeadSaucerY;
    if(xSemaphoreTake(AnimationsBuffer.lock,portMAX_DELAY)==pdTRUE){
        vRetrieveDeadSaucerXY(&DeadSaucerX, &DeadSaucerY, saucer);
        AnimationsBuffer.SaucerShotX=DeadSaucerX;
        AnimationsBuffer.SaucerShotY=DeadSaucerY;
        AnimationsBuffer.SaucerShot=1;
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Updates Player Score after hitting Saucer
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

//Suspends Saucer tasks in order to reduce load after saucer is killed
void vSuspendSaucerTasks()
{
    if(SaucerActionControlTask) vTaskSuspend(SaucerActionControlTask);
    if(UDPControlTask) vTaskSuspend(UDPControlTask);
    if(SaucerAIControlTask) vTaskSuspend(SaucerAIControlTask);
}

/**Being-Shot Control Task - Saucer/Mothership*/
void vTaskSaucerShotControl(void *pvParameters)
{
    while(1){
        uint32_t SaucerCollisionID;
        if(xTaskNotifyWait(0x00, 0xffffffff, &SaucerCollisionID, portMAX_DELAY)==pdTRUE){
            if(xSemaphoreTake(SaucerBuffer.lock, portMAX_DELAY)==pdTRUE){

                vKillSaucer(&SaucerBuffer.SaucerHitFlag, &SaucerBuffer.SaucerAppearsFlag); //Update Flags to indicate saucer has been shot
                vSuspendSaucerTasks();
                vSaucerScoreControl();//Add Corresponding score increase with saucer shot
                vPlayDeadCreatureSound();
                vActivateSaucerShotAnimationState(SaucerBuffer.saucer);

                xSemaphoreGive(SaucerBuffer.lock);
            }
       } 
    }
}


/**Bullet Control Task - Ship*/
void vTaskShipBulletControl(void *pvParameters)
{
    static unsigned char TopWallCollisionFlag=0;
    static unsigned char BunkerCollisionFlag=0;
    static signed char CreatureCollisionFlag=0;
    static unsigned char SaucerCollisionFlag=0;
    static unsigned char ShipBulletwCreatureBulletCollisionFlag=0;

    while(1){
        uint32_t BulletLaunchSignal;

        if(xTaskNotifyWait(0x00, 0xffffffff, &BulletLaunchSignal, portMAX_DELAY) == pdTRUE){
            if(xSemaphoreTake(ShipBuffer.lock, 0)==pdTRUE){   
                if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){   
                    if(xSemaphoreTake(BunkersBuffer.lock, 0)==pdTRUE){
                        if(xSemaphoreTake(SaucerBuffer.lock, 0)==pdTRUE){
                            vUpdateShipBulletPos(ShipBuffer.Ship);


                            if(CreaturesBuffer.BulletAliveFlag==1)
                                ShipBulletwCreatureBulletCollisionFlag=xCheckBullet2BulletCollision(ShipBuffer.Ship->bullet->x_pos,
                                                                                                    ShipBuffer.Ship->bullet->y_pos,
                                                                                                    CreaturesBuffer.CreaturesBullet.x_pos,
                                                                                                    CreaturesBuffer.CreaturesBullet.y_pos);

                            BunkerCollisionFlag=xCheckBunkersCollision(ShipBuffer.Ship->bullet->x_pos, 
                                                                       ShipBuffer.Ship->bullet->y_pos,
                                                                       (*BunkersBuffer.Bunkers));

                            TopWallCollisionFlag=xCheckShipBulletCollisionTopWall(ShipBuffer.Ship->bullet->y_pos);

                        
                            CreatureCollisionFlag=xCheckCreaturesCollision(CreaturesBuffer.Creatures,
                                                                           ShipBuffer.Ship->bullet->x_pos,
                                                                           ShipBuffer.Ship->bullet->y_pos,
                                                                           CreaturesBuffer.HorizontalDirection);

                            if(SaucerBuffer.SaucerAppearsFlag && SaucerBuffer.SaucerHitFlag==0){
                                SaucerCollisionFlag=xCheckSaucerCollision(SaucerBuffer.saucer,
                                                                          SaucerBuffer.Direction,
                                                                          ShipBuffer.Ship->bullet->x_pos,
                                                                          ShipBuffer.Ship->bullet->y_pos);
                            }

                            if(SaucerCollisionFlag || BunkerCollisionFlag || TopWallCollisionFlag || 
                              (CreatureCollisionFlag >=0) || ShipBulletwCreatureBulletCollisionFlag){

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
                                else if(ShipBulletwCreatureBulletCollisionFlag){
                                    CreaturesBuffer.BulletAliveFlag=0;
                                    ShipBulletwCreatureBulletCollisionFlag=0;
                                }
                                else if(TopWallCollisionFlag){
                                    vActivateWallShotAnimationState(); 
                                    TopWallCollisionFlag=0;
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
/**Bullet Control Task - Creature*/
void vTaskCreaturesBulletControl(void *pvParameters)
{
    //Flags used to store values that indicate possible collision with game objects
    static unsigned char BottomWallCollisionFlag=0;
    static unsigned char BunkerCollisionFlag=0;
    static unsigned char ShipCollisonFlag=0;

    while(1){
        uint32_t CreatureBulletControlSignal;
            if(xTaskNotifyWait(0x00, 0xffffffff, &CreatureBulletControlSignal, portMAX_DELAY)==pdTRUE){
                if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){
                    if(xSemaphoreTake(ShipBuffer.lock, 0)){ 

                        vUpdateCreaturesBulletPos(&CreaturesBuffer.CreaturesBullet); //Updates Creatures Bullet Position according to its current speed

                        BottomWallCollisionFlag = xCheckCreaturesBulletCollisonBottomWall(CreaturesBuffer.CreaturesBullet.y_pos);

                        BunkerCollisionFlag = xCheckBunkersCollision(CreaturesBuffer.CreaturesBullet.x_pos,
                                                                     CreaturesBuffer.CreaturesBullet.y_pos,
                                                                     (*BunkersBuffer.Bunkers));
                        
                        ShipCollisonFlag = xCheckCreaturesBulletShipCollision(CreaturesBuffer.CreaturesBullet.x_pos,
                                                                              CreaturesBuffer.CreaturesBullet.y_pos,
                                                                              ShipBuffer.Ship);

                        if(BottomWallCollisionFlag || BunkerCollisionFlag || ShipCollisonFlag){ //In case of any positive outcome -> there was collision
                            CreaturesBuffer.BulletAliveFlag=0;

                            if(BottomWallCollisionFlag)
                                vPlayBulletWallSound();

                            else if(BunkerCollisionFlag){
                                vTaskResume(BunkerShotControlTask); //Wakes and notifies task responsible for Bunker-being-Shot control
                                xTaskNotify(BunkerShotControlTask, (uint32_t)BunkerCollisionFlag, eSetValueWithOverwrite); //Sends Flag value to said task, as it contains which Bunker was shot
                            }
                            else if(ShipCollisonFlag){  
                                vTaskResume(ShipShotControlTask); //Wakes and notifies task responsible for Ship-being-Shot control
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

//Control Left and Right movement and also updates if creatures have reached an edge(number of laps)
void vHorizontalCreatureControl(H_Movement_t* LastHorizontalDirectionOfCreatures,
                                unsigned char* NumberOfLaps)
{
    (*LastHorizontalDirectionOfCreatures) = CreaturesBuffer.HorizontalDirection; 
    vMoveCreaturesHorizontal(CreaturesBuffer.Creatures, &CreaturesBuffer.HorizontalDirection);
    if(xCheckDirectionChange(LastHorizontalDirectionOfCreatures, CreaturesBuffer.HorizontalDirection))
        (*NumberOfLaps)++;
}

//Checks if creatures have already reached an edge, which in that case will order a vertical shift down
void vVerticalCreatureControl(unsigned char* NumberOfLaps)
{
    if((*NumberOfLaps) == 1){
        (*NumberOfLaps)=0;
        vMoveCreaturesVerticalDown(CreaturesBuffer.Creatures);
    }
}

//Checks for Creatures reaching bunker height and wakes/triggers the task responsible for it 
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

//Controls when creatures are allowed to shoot and initiates their bullet creation
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

/**CreatureControl Task*/
void vTaskCreaturesActionControl(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xPrevMovedTime = 0;
    TickType_t xPrevAnimatedTime = xTaskGetTickCount();
    TickType_t xPrevShotTime = 0;

    const TickType_t WakeRate = 15;
   
    static unsigned char NumberOfLaps = 0;
    static H_Movement_t LastHorizontalDirectionOfCreatures = RIGHT;

    vSetCreaturesBufferValues();
    vSetLevelModifiersValues();

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

/**SaucerControl Tasks - SinglePlayer */
void vTaskSaucerActionControl(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t WakeRate = 15;

    TickType_t xPrevAppearanceTime = xTaskGetTickCount();
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

                if(SaucerBuffer.SaucerAppearsFlag==1)
                    vMoveSaucerHorizontal(SaucerBuffer.saucer, &SaucerBuffer.Direction);
            }
            xSemaphoreGive(SaucerBuffer.lock);
        }

        vTaskDelayUntil(&xLastWakeTime, 
                        pdMS_TO_TICKS(WakeRate));
    }
}

/**SaucerControl Tasks - AI */
void vTaskSaucerAIControl(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t WakeRate = 15;
    const TickType_t xSecondMeasure = 1000;
   
    //These variables control the disappearance and appearance times, which here are different
    TickType_t xPrevAppearanceTime = xTaskGetTickCount(); 
    TickType_t xPrevDisAppearanceTime = xTaskGetTickCount(); 
    const TickType_t xDisappearancePeriod = 10*xSecondMeasure;
    const TickType_t AppearancePeriod = 20*xSecondMeasure;

    //Thse make possible the counting of the timers to be done in series, not simulataneously
    unsigned char AppearPeriodDone=1;
    unsigned char DisAppearPeriodDone=0;

    unsigned short LatestSaucerX=0;

    vSetSaucerBufferValues();

    while(1){
        if(xSemaphoreTake(SaucerBuffer.lock,0)==pdTRUE){
            if(SaucerBuffer.SaucerHitFlag==0){ //If Saucer has not been hit
                if(AppearPeriodDone==1 && 
                   DisAppearPeriodDone==0 && 
                   xTaskGetTickCount() - xPrevDisAppearanceTime >= xDisappearancePeriod){

                    AppearPeriodDone=0;
                    DisAppearPeriodDone=1;
                    SaucerBuffer.SaucerAppearsFlag = 1;
                    xPrevAppearanceTime = xTaskGetTickCount();
                }

                if(AppearPeriodDone==0 &&
                   DisAppearPeriodDone==1 && 
                   xTaskGetTickCount() - xPrevAppearanceTime >= AppearancePeriod){

                    AppearPeriodDone=1;
                    DisAppearPeriodDone=0;
                    SaucerBuffer.SaucerAppearsFlag = 0;
                    xPrevDisAppearanceTime = xTaskGetTickCount();
                }


                if(SaucerBuffer.SaucerAppearsFlag==1){
                    xCheckUDPInput(&SaucerBuffer.saucer->x_pos);//Check for UDP-Incoming commands to move AI Saucer
                    xCheckAISaucerBorder(SaucerBuffer.saucer);//If Saucer stuck at border/edge move it around the screen
                    LatestSaucerX=SaucerBuffer.saucer->x_pos;
                    if(SaucerPosQueue)
                        xQueueSend(SaucerPosQueue, &LatestSaucerX, 0);
                }
            }

            xSemaphoreGive(SaucerBuffer.lock);
        }

        vTaskDelayUntil(&xLastWakeTime, 
                        pdMS_TO_TICKS(WakeRate));
    }
}

/**Retrieve Functions */
///Check and make a copy of the latest flags/values of certain global structs to then trigger necessary in-game events.
void xRetrieveAIModeStatus(unsigned char* AIModeFlag)
{
    if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        if(PlayerInfoBuffer.PlayerChosenMode == SinglePlayingState) 
            (*AIModeFlag) = 0;
        else
            (*AIModeFlag) = 1;
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}

void xRetrieveSaucerDestroyedAnimationFlag(unsigned char* SaucerDestroyedAnimationFlag)
{
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if(AnimationsBuffer.SaucerShot==1) 
            (*SaucerDestroyedAnimationFlag)=1;
        else
            (*SaucerDestroyedAnimationFlag)=0;
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

void xRetrieveCreatureDestroyedAnimationFlag(unsigned char* CreatureDestroyedAnimationFlag)
{
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if(AnimationsBuffer.CreatureShot==1) 
            (*CreatureDestroyedAnimationFlag)=1;
        else
            (*CreatureDestroyedAnimationFlag)=0;
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

void xRetrieveWallShotAnimationFlag(unsigned char* WallShotAnimationFlag)
{
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if(AnimationsBuffer.WallShot==1) 
            (*WallShotAnimationFlag)=1;
        else
            (*WallShotAnimationFlag)=0;
        xSemaphoreGive(AnimationsBuffer.lock);
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
    if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){
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

void xRetrieveShipBulletAliveFlag(unsigned char* ShipBulletOnScreenFlag)
{
    if(xSemaphoreTake(ShipBuffer.lock,0)==pdTRUE){  
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

//Checks if player is allowed to shoot, creates player bullet if condition is met
void vActivateShipBulletFlags()
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

// Controls the Saucer/Mothership has-been-shot animation
void vControlSaucerShotAnimation()
{
    const TickType_t SaucerShotAnimationTime = 250;
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if(AnimationsBuffer.SaucerShot==1 && AnimationsBuffer.SaucerShotAnimationTimerSet==0){
            AnimationsBuffer.SaucerShotAnimationTimer = xTaskGetTickCount();
            AnimationsBuffer.SaucerShotAnimationTimerSet=1;
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        else if(AnimationsBuffer.SaucerShotAnimationTimerSet==1){
            if(xTaskGetTickCount() - AnimationsBuffer.SaucerShotAnimationTimer >= SaucerShotAnimationTime){
                AnimationsBuffer.SaucerShot=0; 
                AnimationsBuffer.SaucerShotAnimationTimerSet=0;
                AnimationsBuffer.SaucerShotAnimationTimer=0; 
            }
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Controls the bullet-hit-top-wall animation
void vControlTopWallShotAnimation()
{
    const TickType_t WallShotAnimationTime = 130;
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if(AnimationsBuffer.WallShot==1 && AnimationsBuffer.WallShotAnimationTimerSet==0){
            AnimationsBuffer.WallShotAnimationTimer = xTaskGetTickCount();
            AnimationsBuffer.WallShotAnimationTimerSet=1;
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        else if(AnimationsBuffer.WallShotAnimationTimerSet==1){
            if(xTaskGetTickCount() - AnimationsBuffer.WallShotAnimationTimer >= WallShotAnimationTime){
                AnimationsBuffer.WallShot=0; 
                AnimationsBuffer.WallShotAnimationTimerSet=0;
                AnimationsBuffer.WallShotAnimationTimer=0; 
            }
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Controls the lives lost animation when player has been shot
void vControlLivesRedAnimation()
{
    const TickType_t LivesAnimationsTime = 1200;
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if((AnimationsBuffer.LivesCondition==LivesLost || AnimationsBuffer.LivesCondition==LivesGained) && AnimationsBuffer.LivesTimerSet==0){
            AnimationsBuffer.LivesColorRedTimer = xTaskGetTickCount();
            AnimationsBuffer.LivesTimerSet=1;
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        else if(AnimationsBuffer.LivesTimerSet==1){ 
            if(xTaskGetTickCount() - AnimationsBuffer.LivesColorRedTimer>=LivesAnimationsTime){
                AnimationsBuffer.LivesCondition=LivesIntact;
                AnimationsBuffer.LivesColorRedTimer=0;
                AnimationsBuffer.LivesTimerSet=0;    
            }
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Controls the creature-has-been-shot animation on screen
void vControlCreaturesShotAnimation()
{
    const TickType_t DeadCreatureAnimationTime = 250;
    if(xSemaphoreTake(AnimationsBuffer.lock,0)==pdTRUE){
        if(AnimationsBuffer.CreatureShot==1 && AnimationsBuffer.CreatureShotAnimationTimerSet==0){
            AnimationsBuffer.CreatureShotAnimationTimer=xTaskGetTickCount(); 
            AnimationsBuffer.CreatureShotAnimationTimerSet=1;
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        else if(AnimationsBuffer.CreatureShotAnimationTimerSet==1){
            if(xTaskGetTickCount() - AnimationsBuffer.CreatureShotAnimationTimer >= DeadCreatureAnimationTime){
                AnimationsBuffer.CreatureShot=0; 
                AnimationsBuffer.CreatureShotAnimationTimerSet=0;
                AnimationsBuffer.CreatureShotAnimationTimer=0; 
            }
            xSemaphoreGive(AnimationsBuffer.lock);
        }
        xSemaphoreGive(AnimationsBuffer.lock);
    }
}

//Controls the new lifes given to player and its corresponding animaton
void vControlNewLivesAddition()
{
    if(xSemaphoreTake(PlayerInfoBuffer.lock,0)==pdTRUE){
        if(PlayerInfoBuffer.LivesLeft < 3 && PlayerInfoBuffer.Score >= PlayerInfoBuffer.NewLivesAddedThreshold){
            PlayerInfoBuffer.LivesLeft++; 
            PlayerInfoBuffer.NewLivesAddedThreshold+=PlayerInfoBuffer.NewLivesAddedThreshold;
            AnimationsBuffer.LivesCondition = LivesGained;
        }
        xSemaphoreGive(PlayerInfoBuffer.lock);
    }
}

//Check if player has lost all his lives
unsigned char xCheckLivesLeft()
{
    if (xSemaphoreTake(PlayerInfoBuffer.lock, 0) == pdTRUE){
        if (PlayerInfoBuffer.LivesLeft == 0){
            xSemaphoreGive(PlayerInfoBuffer.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, 0)==pdTRUE){
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
//
//Check for Pause Input
unsigned char xCheckPausePressed()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE){
        if (buttons.buttons[KEYCODE(P)]){
            xSemaphoreGive(buttons.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, 0)==pdTRUE){
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

//Check for Reset Input
unsigned char xCheckResetPressed()
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE){
        if (buttons.buttons[KEYCODE(R)]){
            xSemaphoreGive(buttons.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, 0)==pdTRUE){
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

//Check if all creatures are dead
unsigned char  xCheckCreaturesLeft()
{
    if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){
        if(CreaturesBuffer.NumbOfAliveCreatures==0){
            xSemaphoreGive(CreaturesBuffer.lock);
            if(xSemaphoreTake(OutsideGameActionsBuffer.lock, portMAX_DELAY)==pdTRUE){
                OutsideGameActionsBuffer.PlayerOutsideGameActions = WonGameAction;
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
            }
            return 1;//Won Level
        }
        xSemaphoreGive(CreaturesBuffer.lock);
        return 0;
    }
    return 0;
}

//Checks if any of the bottom-most creatures have reached the bottom of the screen
unsigned char xCheckCreaturesReachedBottom()
{
    if(xSemaphoreTake(CreaturesBuffer.lock, 0)==pdTRUE){
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

//Pauses and Resumes AI communication
void vToggleAICommunication(unsigned char* SaucerAppearsFlag, unsigned char* LastSaucerAppearsFlag)
{
    if((*SaucerAppearsFlag)!=(*LastSaucerAppearsFlag)){ //If Last state of flag is different than the newest value -> Toggle
        if(PauseResumeAIQueue)
            xQueueSend(PauseResumeAIQueue, SaucerAppearsFlag, 0);
        (*LastSaucerAppearsFlag) = (*SaucerAppearsFlag);
    }
}

/**Main Playing Game Task*/
void vTaskPlayingGame(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    static unsigned char SPACE_DEBOUNCE_STATE = 0; //Debounces Shooting

    static unsigned char ShipBulletOnScreenFlag = 0; //0 -> No bullet on screen -> player allowed to shoot.
    static unsigned char CreaturesBulletOnScreenFlag = 0;

    static unsigned char SaucerAppearsFlag = 0;
    static unsigned char LastSaucerAppearsFlag = 1;//Used as memory of the SaucerAppearsFlag

    static unsigned char CreatureDestroyedAnimationFlag = 0; //Animation Flags 1->Initiate corresponding Animation
    static unsigned char SaucerDestroyedAnimationFlag = 0;
    static unsigned char WallShotAnimationFlag = 0;

    static unsigned char AIModeFlag=0; //1 -> Player is playing Multiplayer mode, 0-> Singleplayer mode
    static signed short LatestShipX = 0; //Latest Ship X Coordinate

    xRetrieveAIModeStatus(&AIModeFlag); //Retrieves value that indicates which mode player has chosen

    //Set functions used to initialize relevant global structs
    vSetOutsideGameActionsBufferValues();
    vSetShipsBufferValues();
    vSetBunkersBufferValues();
    vSetAnimationsBufferValues();

    while(1){
        xGetButtonInput();

        //Checks for player inputs or game events that require a state change
        if(xCheckPausePressed() || xCheckLivesLeft() || xCheckResetPressed() || 
           xCheckCreaturesLeft() || xCheckCreaturesReachedBottom())
            vHandleStateMachineActivation();  //!Goes to a pre-handler of the State Machine task.
        
        xCheckShipMoved(&LatestShipX);
        if(AIModeFlag && SaucerAppearsFlag==1) //If Saucer appeared and Multiplayer mode is on -> send latest Ship X to UDP Task
            xQueueSend(ShipPosQueue, &LatestShipX, 0);

        if(AIModeFlag)//If Multiplayer mode is on, toggle (pause/resume) AI communication when saucer appears/disappears
            vToggleAICommunication(&SaucerAppearsFlag, &LastSaucerAppearsFlag);

        if(xCheckShipShoot(&SPACE_DEBOUNCE_STATE) && ShipBulletOnScreenFlag == 0)
            vActivateShipBulletFlags();

        //Retrieve functions - check and make a copy of the latest flags/values of certain
        //global structs to then trigger necessary in-game events.
        xRetrieveSaucerAppearsFlag(&SaucerAppearsFlag);
        xRetrieveShipBulletAliveFlag(&ShipBulletOnScreenFlag); 
        xRetrieveCreaturesBulletAliveFlag(&CreaturesBulletOnScreenFlag) ;
        xRetrieveCreatureDestroyedAnimationFlag(&CreatureDestroyedAnimationFlag);
        xRetrieveSaucerDestroyedAnimationFlag(&SaucerDestroyedAnimationFlag);
        xRetrieveWallShotAnimationFlag(&WallShotAnimationFlag);


        //Control Animations of certain events in game.
        vControlLivesRedAnimation();
        vControlCreaturesShotAnimation();
        vControlSaucerShotAnimation();
        vControlTopWallShotAnimation();

        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xSemaphoreTake(ScreenLock,portMAX_DELAY);

                    tumDrawClear(Black);
                    vDrawStaticTexts();//Draws Scores
                    vDrawShip();//Draws player ship 
                    vDrawCreatures();//Draws the enemy creatures

                    if(CreaturesBulletOnScreenFlag) //if there is a creature bullet alive/on screen -> Wake up Creature bullet Control task
                        vTriggerCreaturesBulletControl();
                    if(ShipBulletOnScreenFlag) //Same for PlayerShip Bullets
                        vTriggerShipBulletControl();
                    if(SaucerAppearsFlag)// Only draw saucer if the SaucerAppearFlag has been set by the SaucerControlTask
                        vDrawSaucer();

                    if(CreatureDestroyedAnimationFlag) //Only draw animations if their respective flags have been set
                        vDrawCreatureDestruction();
                    if(SaucerDestroyedAnimationFlag)
                        vDrawSaucerDestruction();
                    if(WallShotAnimationFlag)
                        vDrawWallShot();

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

//Draws possible Paused Game menu options
void vDrawInstructionsPausedGame()
{
    char RestartChar[20];
    int RestartCharWidth=0;

    char ResumeChar[30];
    int ResumeCharWidth=0;

    if(xSemaphoreTake(PausedGameInfoBuffer.lock, 0)==pdTRUE){
        sprintf(ResumeChar,"Resume Game");
        if(!tumGetTextSize((char *)ResumeChar,&ResumeCharWidth, NULL)){
                        checkDraw(tumDrawText(ResumeChar,
                                              SCREEN_WIDTH*2/4-ResumeCharWidth/2,
                                              SCREEN_HEIGHT*50/100 - DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(PausedGameInfoBuffer.SelectedPausedGameOption, Resume)),
                                              __FUNCTION__);
        }

        sprintf(RestartChar,"Restart Game");
        if(!tumGetTextSize((char *)RestartChar,&RestartCharWidth, NULL)){
                        checkDraw(tumDrawText(RestartChar,
                                              SCREEN_WIDTH*2/4-RestartCharWidth/2,
                                              SCREEN_HEIGHT*65/100 - DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(PausedGameInfoBuffer.SelectedPausedGameOption, RestartReset)),
                                              __FUNCTION__);
        }
        xSemaphoreGive(PausedGameInfoBuffer.lock);
    }
}

//Checks for User's UP/DOWN Input, changing highlighted displayed options in Game Over Menu
void  xCheckPauseSelectionChange(unsigned char* UP_DEBOUNCE_STATE, 
                                unsigned char* DOWN_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

        if (buttons.buttons[KEYCODE(DOWN)]) {
            if(buttons.buttons[KEYCODE(DOWN)]!=(*DOWN_DEBOUNCE_STATE))
                if(xSemaphoreTake(PausedGameInfoBuffer.lock,0)==pdTRUE){  
                    vDownPausedSelection(&PausedGameInfoBuffer.SelectedPausedGameOption);
                    xSemaphoreGive(PausedGameInfoBuffer.lock);
                }
        }                            
        else if (buttons.buttons[KEYCODE(UP)]) { 
            if(buttons.buttons[KEYCODE(UP)]!=(*UP_DEBOUNCE_STATE))
                if(xSemaphoreTake(PausedGameInfoBuffer.lock,0)==pdTRUE){  
                    vUpPausedSelection(&PausedGameInfoBuffer.SelectedPausedGameOption);
                    xSemaphoreGive(PausedGameInfoBuffer.lock);
                }
        }   

        (*UP_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(UP)];
        (*DOWN_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(DOWN)];
        xSemaphoreGive(buttons.lock);                                                                                                                                                                
    }   
}

/**Paused Game Task*/
void vTaskPausedGame(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    unsigned char UP_DEBOUNCE_STATE = 0; //Up and Down Arrow key debouncing states
    unsigned char DOWN_DEBOUNCE_STATE = 0;

    while(1){
        xGetButtonInput();
        xCheckPauseSelectionChange(&UP_DEBOUNCE_STATE, //Moves highlighting of current users selection
                                  &DOWN_DEBOUNCE_STATE);
        if(xCheckEnterPressed())
            vHandleStateMachineActivation(); //!Goes to a pre-handler of the State Machine task.

        if(DrawSignal) 
            if(xSemaphoreTake(DrawSignal, portMAX_DELAY)==pdTRUE){
                xSemaphoreTake(ScreenLock, portMAX_DELAY);
            
                    tumDrawClear(Black);
                    vDrawInstructionsPausedGame();//Draws possible paused menu options
                    vDrawStaticTexts();//Draws Scores
                    vDrawLevel();
                    vDrawLives();
                    vDrawFPS(); 

                xSemaphoreGive(ScreenLock);

                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
    }
}

//Draws possible Game over menu options
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

//Draws Game Ove Sign/Banner
void vDrawGameOverBanner()
{
    checkDraw(tumDrawLoadedImage(GameOver,
                                 SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(GameOver)/2 + 15,
                                 SCREEN_HEIGHT*4/10-tumDrawGetLoadedImageHeight(GameOver)/2),
                                 __FUNCTION__);
}

//Checks for User's UP/DOWN Input, changing highlighted displayed options in Game Over Menu
void  xCheckGameOverSelectionChange(unsigned char* UP_DEBOUNCE_STATE, 
                                unsigned char* DOWN_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

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

/**Game Over Task*/
void vTaskGameOver(void *pvParameters)
{
    GameOver = tumDrawLoadImage("../resources/GameOver.bmp");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    vSetGameOverInfoBufferValues();

    unsigned char UP_DEBOUNCE_STATE = 0; //Up and Down Arrow key debouncing states
    unsigned char DOWN_DEBOUNCE_STATE = 0;

    while(1){
        xGetButtonInput();
        xCheckGameOverSelectionChange(&UP_DEBOUNCE_STATE, //Moves highlighting of current users selection 
                                      &DOWN_DEBOUNCE_STATE);

        if(xCheckEnterPressed())
                vHandleStateMachineActivation(); //!Goes to a pre-handler of the State Machine task.

        if(DrawSignal) 
            if(xSemaphoreTake(DrawSignal, portMAX_DELAY)==pdTRUE){
                xSemaphoreTake(ScreenLock, portMAX_DELAY);
            
                    tumDrawClear(Black);
                    vDrawGameOverBanner();
                    vDrawInstructionsGameOver();//Draws possible Game Over Menu options
                    vDrawStaticTexts();//Draws Scores
                    vDrawLevel();
                    vDrawLives();
                    vDrawFPS(); 

                xSemaphoreGive(ScreenLock);

                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
    }
}

//Draws "Loading..." on screen
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

//Draws next level Sign/Banner
void vDrawNextLevelBanner()
{
    checkDraw(tumDrawLoadedImage(NextLevel,
                                 SCREEN_WIDTH*2/4-tumDrawGetLoadedImageWidth(NextLevel)/2,
                                 SCREEN_HEIGHT*4/10-tumDrawGetLoadedImageHeight(NextLevel)/2),
                                 __FUNCTION__);
}

/**Go-To-Next-Level Task*/
void vTaskNextLevel(void *pvParameters)
{
    NextLevel = tumDrawLoadImage("../resources/NextLevel.bmp");

    TickType_t xLastWakeTime = xTaskGetTickCount();

    TickType_t xCountDown = 0; //These variables control the countdown, this task only appeards for 10 seconds and then changes to next level
    signed char CountdownInSeconds = 10;

    const TickType_t UpdatePeriod = 20;

    while(1){
        if(CountdownInSeconds==0){
            CountdownInSeconds=10;
            vHandleStateMachineActivation();//!Goes to a pre-handler of the State Machine task.
        }

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

//Draws Cheats Menu options
void vDrawCheatOptions()
{
    static char InfiniteLivesChar[20];
    static char ChooseStartingScoreChar[35];
    static char ChooseStartingLevelChar[35];

    static int InfiniteLivesCharWidth=0;
    static int ChooseStartingScoreCharWidth=0;   
    static int ChooseStartingLevelCharWidth=0;

    static char StartingScoreValueChar[20];
    static int StartingScoreValueCharWidth=0;

    static char StartingLevelValueChar[20];
    static int StartingLevelValueCharWidth=0;

    if(xSemaphoreTake(CheatsInfoBuffer.lock, 0)==pdTRUE){  

        sprintf(InfiniteLivesChar,"Infinite Lives");
        if(!tumGetTextSize((char *)InfiniteLivesChar,&InfiniteLivesCharWidth, NULL)){
                        checkDraw(tumDrawText(InfiniteLivesChar,
                                              SCREEN_WIDTH*1/4-InfiniteLivesCharWidth/2,SCREEN_HEIGHT*6/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(CheatsInfoBuffer.SelectedCheatsOption, InfiniteLives)),
                                              __FUNCTION__);
        }

        sprintf(ChooseStartingScoreChar,"Choose Starting Score");
        if(!tumGetTextSize((char *)ChooseStartingScoreChar,&ChooseStartingScoreCharWidth, NULL)){
                        checkDraw(tumDrawText(ChooseStartingScoreChar,
                                              SCREEN_WIDTH*1/4-ChooseStartingScoreCharWidth/2,SCREEN_HEIGHT*7/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(CheatsInfoBuffer.SelectedCheatsOption, ChooseStartingScore)),
                                              __FUNCTION__);
        }

        sprintf(StartingScoreValueChar,"Value: [  %d  ]", CheatsInfoBuffer.StartingScoreValue);
        if(!tumGetTextSize((char *)StartingScoreValueChar,&StartingScoreValueCharWidth, NULL)){
                        checkDraw(tumDrawText(StartingScoreValueChar,
                                              SCREEN_WIDTH*3/4-StartingScoreValueCharWidth/2,SCREEN_HEIGHT*7/10-DEFAULT_FONT_SIZE/2,
                                              Green),
                                              __FUNCTION__);
        }
        


        sprintf(ChooseStartingLevelChar,"Choose Starting Level");
        if(!tumGetTextSize((char *)ChooseStartingLevelChar,&ChooseStartingLevelCharWidth, NULL)){
                        checkDraw(tumDrawText(ChooseStartingLevelChar,
                                              SCREEN_WIDTH*1/4-ChooseStartingLevelCharWidth/2,SCREEN_HEIGHT*8/10-DEFAULT_FONT_SIZE/2,
                                              xFetchSelectedColor(CheatsInfoBuffer.SelectedCheatsOption, ChooseStartingLevel)),
                                              __FUNCTION__);
        }

        sprintf(StartingLevelValueChar,"Value: [  %d  ]", CheatsInfoBuffer.StartingLevelValue);
        if(!tumGetTextSize((char *)StartingLevelValueChar,&StartingLevelValueCharWidth, NULL)){
                        checkDraw(tumDrawText(StartingLevelValueChar,
                                              SCREEN_WIDTH*3/4-StartingLevelValueCharWidth/2,SCREEN_HEIGHT*8/10-DEFAULT_FONT_SIZE/2,
                                              Green),
                                              __FUNCTION__);
        }
        xSemaphoreGive(CheatsInfoBuffer.lock);
    }
}

//Moves highlighting of current users selection
void xCheckCheatsSelectionChange(unsigned char* UP_DEBOUNCE_STATE,
                                 unsigned char* DOWN_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

        if (buttons.buttons[KEYCODE(DOWN)]) {
            if(buttons.buttons[KEYCODE(DOWN)]!=(*DOWN_DEBOUNCE_STATE))
                if(xSemaphoreTake(CheatsInfoBuffer.lock,0)==pdTRUE){  
                    vDownCheatsSelection(&CheatsInfoBuffer.SelectedCheatsOption);
                    xSemaphoreGive(CheatsInfoBuffer.lock);
                }
        }                            
        else if (buttons.buttons[KEYCODE(UP)]) { 
            if(buttons.buttons[KEYCODE(UP)]!=(*UP_DEBOUNCE_STATE))
                if(xSemaphoreTake(CheatsInfoBuffer.lock,0)==pdTRUE){  
                    vUpCheatsSelection(&CheatsInfoBuffer.SelectedCheatsOption);
                    xSemaphoreGive(CheatsInfoBuffer.lock);
                }
        }   

        (*UP_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(UP)];
        (*DOWN_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(DOWN)];
        xSemaphoreGive(buttons.lock);                                                                                                                                                                
    }
}

//Increases/Decreases Values shown on the screen, regarding cheat being chosen
void xCheckLeftRightIncrement(unsigned char* RIGHT_DEBOUNCE_STATE,
                              unsigned char* LEFT_DEBOUNCE_STATE)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

        if (buttons.buttons[KEYCODE(LEFT)]) {
            if(buttons.buttons[KEYCODE(LEFT)]!=(*LEFT_DEBOUNCE_STATE))
                if(xSemaphoreTake(CheatsInfoBuffer.lock,0)==pdTRUE){  
                    vDecrementValue(CheatsInfoBuffer.SelectedCheatsOption,
                                    &CheatsInfoBuffer.StartingScoreValue,
                                    &CheatsInfoBuffer.StartingLevelValue);
                    xSemaphoreGive(CheatsInfoBuffer.lock);
                }
        }                            
        else if (buttons.buttons[KEYCODE(RIGHT)]) { 
            if(buttons.buttons[KEYCODE(RIGHT)]!=(*RIGHT_DEBOUNCE_STATE))
                if(xSemaphoreTake(CheatsInfoBuffer.lock,0)==pdTRUE){  
                    vIncrementValue(CheatsInfoBuffer.SelectedCheatsOption,
                                    &CheatsInfoBuffer.StartingScoreValue,
                                    &CheatsInfoBuffer.StartingLevelValue);
                    xSemaphoreGive(CheatsInfoBuffer.lock);
                }
        }   

        (*RIGHT_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(RIGHT)];
        (*LEFT_DEBOUNCE_STATE)=buttons.buttons[KEYCODE(LEFT)];
        xSemaphoreGive(buttons.lock);                                                                                                                                                                
    }
}

/**Cheats Task*/
void vTaskCheats(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t UpdatePeriod = 20;

    TickType_t xInitialDebouncePeriodReference = xTaskGetTickCount(); //Debounces Enter being pressed from Main Menu to Cheats Task
    const TickType_t InitialDebouncePeriod = 600;
   
    unsigned char UP_DEBOUNCE_STATE = 0;
    unsigned char DOWN_DEBOUNCE_STATE = 0;

    unsigned char RIGHT_DEBOUNCE_STATE = 0;
    unsigned char LEFT_DEBOUNCE_STATE = 0;

    vSetCheatsInfoBufferValues();

    while(1){
        xGetButtonInput(); 
         
        xCheckCheatsSelectionChange(&UP_DEBOUNCE_STATE, //Moves highlighting of current users selection
                                   &DOWN_DEBOUNCE_STATE);

        xCheckLeftRightIncrement(&RIGHT_DEBOUNCE_STATE, //Increases/Decreases Values shown on the screen, regarding cheat being chosen
                                 &LEFT_DEBOUNCE_STATE);

        if(xTaskGetTickCount() - xInitialDebouncePeriodReference > InitialDebouncePeriod)
            if(xCheckEnterPressed()){ //Player has chosen a cheat!
                xInitialDebouncePeriodReference = xTaskGetTickCount();
                vHandleStateMachineActivation();//!Goes to a pre-handler of the State Machine task.
            }
    
        if(DrawSignal)
            if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE){    
                xSemaphoreTake(ScreenLock,portMAX_DELAY);

                    tumDrawClear(Black); 
                    vDrawSpaceInvadersBanner();//Draws Spacer Invaders Sign/Banner
                    vDrawCheatOptions();//Draws Cheat Menu options


                xSemaphoreGive(ScreenLock);
                vTaskDelayUntil(&xLastWakeTime, 
                                pdMS_TO_TICKS(UpdatePeriod));
            }
    }
}

//Listens on UDP Receive Port and decodes commands sent from the AI
void UDPhandler(size_t read_size, char *buffer, void *args)
{
    OpponentCommands_t NextKEY = NONE;

    BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken2 = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken3 = pdFALSE;

    if (xSemaphoreTakeFromISR(HandleUDP, &xHigherPriorityTaskWoken1) ==pdTRUE) {
        char SendCommandFlag = 0;

        if (strncmp(buffer, "INC", (read_size < 3) ? read_size : 3) ==0) {
            NextKEY = INC;
            SendCommandFlag = 1;
        }
        else if (strncmp(buffer, "DEC",(read_size < 3) ? read_size : 3) == 0) {
            NextKEY = DEC;
            SendCommandFlag = 1;
        }

        else if (strncmp(buffer, "NONE",(read_size < 4) ? read_size : 4) == 0) {
            NextKEY = NONE;
            SendCommandFlag = 1;
        }

        if (NextKEYQueue && SendCommandFlag) {
            xQueueSendFromISR(NextKEYQueue, 
                             (void *)&NextKEY,
                             &xHigherPriorityTaskWoken2);
        }
        xSemaphoreGiveFromISR(HandleUDP, &xHigherPriorityTaskWoken3);


        portYIELD_FROM_ISR(xHigherPriorityTaskWoken1 |
                           xHigherPriorityTaskWoken2 |
                           xHigherPriorityTaskWoken3);
    }
    else {
        fprintf(stderr, "[ERROR] Overlapping UDPHandler call\n");
    }
}

//Reads from the NextKEYQueue where Decoded AI commands are sent to from the UDP Handler function
unsigned char xCheckUDPInput(signed short* SaucerX)
{
    static OpponentCommands_t CurrentKEY = { 0 };
    if(NextKEYQueue){
        xQueueReceive(NextKEYQueue, &CurrentKEY, 0);
    }
    if(CurrentKEY == INC){
        vMoveSaucerRight(SaucerBuffer.saucer);
    }
    else if(CurrentKEY == DEC){
        vMoveSaucerLeft(SaucerBuffer.saucer);
    }
        
    return 0; 
}

/**UDP-CONTROL Task*/
void vTaskUDPControl(void *pvParameters)
{
    const TickType_t UpdatePeriod=15;

    //Buffer, string variable used to send messages to the AI
    static char buffer[50];

    char *addr = NULL; // Loopback
    in_port_t UDPport = UDP_RECEIVE_PORT; //UDP receive port

    signed short CurrentSaucerX=0; //Variable that stores current Saucer X
    signed short CurrentShipX=0; //Variable that stores current Ship X

    unsigned char PauseResumeSignal;//Flag used to control the sending of Pause and Resume to the AI
    int RelativePosDifference=0; //Relative position difference between AI and Ship
    unsigned char ShipBulletOnScreenFlag=0;//Flag that stores flag regarding players bullet being alive or not

    unsigned char LastShipBulletOnScreenFlagCondition=ShipBulletOnScreenFlag;


    UDP_SOC_RECEIVE = aIOOpenUDPSocket(addr, //! Instantiating UDP Handler Function to UDP Receiving Port where AI sends messages to
                                       UDPport,
                                       UDP_BUFFER_SIZE,
                                       UDPhandler, 
                                       NULL);

    printf("UDP socket opened on port %d\n", UDPport); 

    while(1){
        vTaskDelay(pdMS_TO_TICKS(UpdatePeriod)); 

        while(xQueueReceive(ShipPosQueue, &CurrentShipX, 0)==pdTRUE){}//If there is a value in the ShipPosQueue, continue executing
        while(xQueueReceive(SaucerPosQueue, &CurrentSaucerX, 0)==pdTRUE) {}

            xRetrieveShipBulletAliveFlag(&ShipBulletOnScreenFlag);//Retrieves Bullet on Screen Flag -> Means Attacking if Bullet is Alive 

            RelativePosDifference = (int)CurrentShipX - (int)CurrentSaucerX; //Calculates relative position difference

            if(RelativePosDifference>0){
                sprintf(buffer, "+%d", RelativePosDifference);
            }
            else
                sprintf(buffer, "-%d", -RelativePosDifference);


            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buffer, strlen(buffer));//Places on the transmit port the message above


            if(ShipBulletOnScreenFlag!=LastShipBulletOnScreenFlagCondition){ //Only Sends these values if there was a change between agressiveness/passiveness
                if(ShipBulletOnScreenFlag==1){
                    sprintf(buffer, "ATTACKING"); 
                }
                else {
                    sprintf(buffer, "PASSIVE"); 
                }

                aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buffer, strlen(buffer));
                LastShipBulletOnScreenFlagCondition = ShipBulletOnScreenFlag;
            }

            if(xQueueReceive(PauseResumeAIQueue, &PauseResumeSignal, 0)==pdTRUE){ //If There is a Pause/Resume value
                if(PauseResumeSignal==1)
                    sprintf(buffer, "RESUME"); 
                else
                    sprintf(buffer, "PAUSE"); 

                aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, buffer, strlen(buffer)); //Places on the transmit port the message above
            }
    }
}

/**Swap Buffers Task*/
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

/**HANDLE State Machine Functions */
///Depending on the Game State different handles are taken, which each of whom 
///also display different possibilities of state changes.
///Important: vHandleStateMachineActivation is the Main Handle, begin reading through it
void vHandleNextLevelStateSM()
{
        vPrepareGameValues(NewGameNextLevel);
        if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
            if(StateQueue && PlayerInfoBuffer.PlayerChosenMode==SinglePlayingState){
                xSemaphoreGive(PlayerInfoBuffer.lock);
                xQueueSend(StateQueue,&SinglePlayingStateSignal, 0);
            }
            else if(StateQueue && PlayerInfoBuffer.PlayerChosenMode==MultiPlayingState){
                xSemaphoreGive(PlayerInfoBuffer.lock);
                xQueueSend(StateQueue,&MultiPlayingStateSignal, 0);
            }
        }
}
void vHandleGameOverStateSM()
{
    if(xSemaphoreTake(GameOverInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        switch(GameOverInfoBuffer.SelectedGameOverOption){
            case PlayAgain:
                xSemaphoreGive(GameOverInfoBuffer.lock);
                vPrepareGameValues(NewGameFromScratch);
                if(StateQueue)
                    xQueueSend(StateQueue,&MainMenuStateSignal, 0);
                break; 

            case Quit:
                xSemaphoreGive(GameOverInfoBuffer.lock);
                exit(EXIT_SUCCESS);
                break; 
            default:
                xSemaphoreGive(GameOverInfoBuffer.lock);
                break; 
        }
        xSemaphoreGive(GameOverInfoBuffer.lock);
    }
}
void vHandlePausedGameStateSM()
{
    if(xSemaphoreTake(PausedGameInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        switch(PausedGameInfoBuffer.SelectedPausedGameOption){
            case Resume:
                xSemaphoreGive(PausedGameInfoBuffer.lock);
                if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                    if(StateQueue && PlayerInfoBuffer.PlayerChosenMode==SinglePlayingState){
                        xSemaphoreGive(PlayerInfoBuffer.lock);
                        xQueueSend(StateQueue,&SinglePlayingStateSignal, 0);
                    }
                    else if(StateQueue && PlayerInfoBuffer.PlayerChosenMode==MultiPlayingState){
                        xSemaphoreGive(PlayerInfoBuffer.lock);
                        xQueueSend(StateQueue,&MultiPlayingStateSignal, 0);
                    }
                    xSemaphoreGive(PlayerInfoBuffer.lock);
                }
                break; 
            case RestartReset:
                xSemaphoreGive(PausedGameInfoBuffer.lock);
                vPrepareGameValues(NewGameFromScratch);
                if(StateQueue)
                    xQueueSend(StateQueue,&ResetGameStateSignal, 0);
                break; 
            default:
                xSemaphoreGive(PausedGameInfoBuffer.lock);
               break; 
        }
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
                break;
            case ResetGameAction:
                vPrepareGameValues(NewGameFromScratch);
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
                if (StateQueue)
                    xQueueSend(StateQueue,&ResetGameStateSignal, 0);
                break;
            case NoAction:
            default:
                xSemaphoreGive(OutsideGameActionsBuffer.lock);
                break;
        }
    }
}
void vHandleMainMenuStateSM()
{
    if(xSemaphoreTake(MainMenuInfoBuffer.lock, portMAX_DELAY)==pdTRUE){

        switch(MainMenuInfoBuffer.SelectedMenuOption){

            case SinglePlayer:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                    PlayerInfoBuffer.PlayerChosenMode=SinglePlayingState; //Necessary for future functions to know which mode the game is in
                    xSemaphoreGive(PlayerInfoBuffer.lock);
                }
                if(StateQueue)
                    xQueueSend(StateQueue,&SinglePlayingStateSignal, 0);
                break; 

            case Leave:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                exit(EXIT_SUCCESS);
                break;

            case Cheats:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                if(StateQueue)
                    xQueueSend(StateQueue,&CheatsStateSignal, 0);
                break;

            case MultiPlayer:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                if(xSemaphoreTake(PlayerInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
                    PlayerInfoBuffer.PlayerChosenMode=MultiPlayingState; //Necessary for future functions to know which mode the game is in
                    xSemaphoreGive(PlayerInfoBuffer.lock);
                }
                if(StateQueue)
                    xQueueSend(StateQueue,&MultiPlayingStateSignal, 0);
                break; 
            default:
                xSemaphoreGive(MainMenuInfoBuffer.lock);
                break;
        }

        xSemaphoreGive(MainMenuInfoBuffer.lock);
    }
}

void vHandleCheatsStateSM()
{
    if(xSemaphoreTake(CheatsInfoBuffer.lock, portMAX_DELAY)==pdTRUE){
        switch(CheatsInfoBuffer.SelectedCheatsOption){
            case InfiniteLives:
                xSemaphoreGive(CheatsInfoBuffer.lock);
                vPrepareGameValues(InfiniteLivesCheat);
                if(StateQueue)
                    xQueueSend(StateQueue,&MainMenuStateSignal, 0);
                break; 

            case ChooseStartingScore:
                xSemaphoreGive(CheatsInfoBuffer.lock);
                vPrepareGameValues(ChooseStartingScoreCheat);
                if(StateQueue)
                    xQueueSend(StateQueue,&MainMenuStateSignal, 0);
                break; 

            case ChooseStartingLevel:
                xSemaphoreGive(CheatsInfoBuffer.lock);
                vPrepareGameValues(ChooseStartingLevelCheat);
                if(StateQueue)
                    xQueueSend(StateQueue,&MainMenuStateSignal, 0);
                break; 
            default:
                xSemaphoreGive(CheatsInfoBuffer.lock);
                break; 
        }
        xSemaphoreGive(GameOverInfoBuffer.lock);
    }
}

//Main Handle - This is the point of entry where the appropiate "vHandle" function will be chosen according to the game state
void vHandleStateMachineActivation()
{
    if(xSemaphoreTake(GameStateBuffer.lock, portMAX_DELAY)==pdTRUE){  
        switch(GameStateBuffer.GameState){
            case ResetGameState:
            case MainMenuState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandleMainMenuStateSM();
                break; 

            case MultiPlayingState:
            case SinglePlayingState:
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
                break;
            case CheatsState:
                xSemaphoreGive(GameStateBuffer.lock);
                vHandleCheatsStateSM();
            default:
                break; 
        
        }
        xSemaphoreGive(GameStateBuffer.lock);
    }
}

//Function that deletes and creates important Tasks relevant to a new game situation
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
    if(SaucerAIControlTask){ 
        vTaskSuspend(SaucerAIControlTask);
        vTaskDelete(SaucerAIControlTask);
        if(xTaskCreate(vTaskSaucerAIControl, "SaucerAIControlTask", mainGENERIC_STACK_SIZE*2, NULL,
                       configMAX_PRIORITIES - 3, &SaucerAIControlTask)!=pdPASS)
            exit(EXIT_SUCCESS);
        vTaskSuspend(SaucerAIControlTask);
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

/**State Machine Task */
void vTaskStateMachine(void *pvParameters){
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
                taskENTER_CRITICAL();
                xSemaphoreTake(GameStateBuffer.lock, portMAX_DELAY);

                    switch (current_state){

                        case MainMenuState: // Begin 

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(CheatsTask) vTaskSuspend(CheatsTask);
                            if(UDPControlTask) vTaskSuspend(UDPControlTask);
                            if(CreaturesActionControlTask) vTaskSuspend(CreaturesActionControlTask);
                            if(SaucerActionControlTask) vTaskSuspend(SaucerActionControlTask);
                            if(SaucerAIControlTask) vTaskSuspend(SaucerAIControlTask);
                            if(MainPlayingGameTask) vTaskSuspend(MainPlayingGameTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(MainMenuTask) vTaskResume(MainMenuTask);
                            break;

                        case SinglePlayingState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(UDPControlTask) vTaskSuspend(UDPControlTask);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(CreaturesActionControlTask) vTaskResume(CreaturesActionControlTask);
                            if(SaucerActionControlTask) vTaskResume(SaucerActionControlTask);
                            if(MainPlayingGameTask) vTaskResume(MainPlayingGameTask);
                            break;

                        case MultiPlayingState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(CreaturesActionControlTask) vTaskResume(CreaturesActionControlTask);
                            if(SaucerAIControlTask) vTaskResume(SaucerAIControlTask);
                            if(UDPControlTask) vTaskResume(UDPControlTask);
                            if(MainPlayingGameTask) vTaskResume(MainPlayingGameTask);
                            break;

                            case PausedState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(UDPControlTask) vTaskSuspend(UDPControlTask);
                            if(SaucerAIControlTask) vTaskSuspend(SaucerAIControlTask);
                            if(CreaturesActionControlTask) vTaskSuspend(CreaturesActionControlTask);
                            if(SaucerActionControlTask) vTaskSuspend(SaucerActionControlTask);
                            if(MainPlayingGameTask) vTaskSuspend(MainPlayingGameTask);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(PausedGameTask) vTaskResume(PausedGameTask);
                            break;

                        case GameOverState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(UDPControlTask) vTaskSuspend(UDPControlTask);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            vRecreateGame();
                            if(GameOverTask) vTaskResume(GameOverTask);
                            break;
                        
                        case NextLevelState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(UDPControlTask) vTaskSuspend(UDPControlTask);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            vRecreateGame();
                            if(NextLevelTask) vTaskResume(NextLevelTask);
                            break;

                        case CheatsState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(MainMenuTask) vTaskSuspend(MainMenuTask);
                            if(CheatsTask) vTaskResume(CheatsTask);

                            break;
                        case ResetGameState:

                            xSemaphoreGive(GameStateBuffer.lock);
                            if(UDPControlTask) vTaskSuspend(UDPControlTask);
                            if(GameOverTask) vTaskSuspend(GameOverTask);
                            if(PausedGameTask) vTaskSuspend(PausedGameTask);
                            if(NextLevelTask) vTaskSuspend(NextLevelTask);
                            vRecreateGame();
                            if(MainMenuTask) vTaskResume(MainMenuTask);
                            break;
                        default:
                            xSemaphoreGive(GameStateBuffer.lock);
                            break;
                    }

                    GameStateBuffer.GameState=current_state;
                    changed_state=0; //Resets changed state
                    taskEXIT_CRITICAL();
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
    if (xTaskCreate(vTaskStateMachine, "StateMachine", mainGENERIC_STACK_SIZE * 2, NULL,
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
    
    PlayerInfoBuffer.lock = xSemaphoreCreateMutex();
    if(!PlayerInfoBuffer.lock){
        PRINT_ERROR("Failed to create Player info buffer lock.");
        goto err_playerinfo;
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

    AnimationsBuffer.lock = xSemaphoreCreateMutex();
    if(!AnimationsBuffer.lock){
        PRINT_ERROR("Failed to create Animatons Buffer lock.");
        goto err_animationsbuffer;
    }

    if(xTaskCreate(vTaskUDPControl, "UDP Control Task", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 3, &UDPControlTask)!=pdPASS){
         
        PRINT_ERROR("Failed to create UDP Control Task.");
        goto err_udpcontroltask;
    }

    HandleUDP = xSemaphoreCreateMutex();
    if(!HandleUDP){
        PRINT_ERROR("Could not create UDP Semaphore handle.");
        goto err_udphandlesemaphore;
    }

    NextKEYQueue = xQueueCreate(1, sizeof(OpponentCommands_t));
    if (!NextKEYQueue) {
        exit(EXIT_FAILURE);
        goto err_nextkeyqueue;
    }


    PauseResumeAIQueue = xQueueCreate(1, sizeof(unsigned char));
    if (!PauseResumeAIQueue) {
        exit(EXIT_FAILURE);
        goto err_pauseresumequeue;
    }

    ShipPosQueue = xQueueCreate(1, sizeof(unsigned short));
    if(!ShipPosQueue){
        exit(EXIT_FAILURE);
        goto err_shipposqueue;
    }

    if(xTaskCreate(vTaskSaucerAIControl, "AI Control Task", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 3, &SaucerAIControlTask)!=pdPASS){
        PRINT_ERROR("Failed to create UDP Control Task.");
        goto err_saucerAIcontroltask;
    }

    if(xTaskCreate(vTaskCheats, "Cheats Task", mainGENERIC_STACK_SIZE*2, NULL,
                   configMAX_PRIORITIES - 4, &CheatsTask)!=pdPASS){
        PRINT_ERROR("Failed to create Cheats Task.");
        goto err_cheatstask;
    }

    CheatsInfoBuffer.lock = xSemaphoreCreateMutex();
    if(!CheatsInfoBuffer.lock){
        PRINT_ERROR("Failed to create Cheats Buffer lock.");
        goto err_cheatsbufferlock;
    }

    SaucerPosQueue = xQueueCreate(1, sizeof(unsigned short));
    if(!SaucerPosQueue){
        exit(EXIT_FAILURE);
        goto err_saucerposqueue;
    }

    vTaskSuspend(MainMenuTask);
    vTaskSuspend(MainPlayingGameTask);
    vTaskSuspend(PausedGameTask);
    vTaskSuspend(GameOverTask);
    vTaskSuspend(NextLevelTask);
    vTaskSuspend(MainMenuTask);
    vTaskSuspend(MainPlayingGameTask);
    vTaskSuspend(PausedGameTask);
    vTaskSuspend(GameOverTask);
    vTaskSuspend(CheatsTask);
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
    

    vTaskSuspend(UDPControlTask);
    vTaskSuspend(SaucerAIControlTask);

    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_saucerposqueue:
    vSemaphoreDelete(CheatsInfoBuffer.lock);
err_cheatsbufferlock:
    vTaskDelete(CheatsTask);
err_cheatstask:
    vTaskDelete(SaucerAIControlTask);
err_saucerAIcontroltask:
    vQueueDelete(ShipPosQueue);
err_shipposqueue:
    vQueueDelete(PauseResumeAIQueue);
err_pauseresumequeue:    
    vQueueDelete(NextKEYQueue);
err_nextkeyqueue:
    vTaskDelete(UDPControlTask);
err_udphandlesemaphore:
    vTaskDelete(UDPControlTask);
err_udpcontroltask:
    vSemaphoreDelete(AnimationsBuffer.lock);
err_animationsbuffer:
    vTaskDelete(SaucerShotControlTask);
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
    vSemaphoreDelete(ShipBuffer.lock);
err_shipbuffer:
    vSemaphoreDelete(MainMenuInfoBuffer.lock);
err_mainmenuinfobuffer:
    vSemaphoreDelete(OutsideGameActionsBuffer.lock);
err_outsideactionsbuffer:
    vSemaphoreDelete(PlayerInfoBuffer.lock);
err_playerinfo:
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
    vTaskDelete(SwapBuffers);
err_swapbuffers:
    vSemaphoreDelete(buttons.lock);
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
