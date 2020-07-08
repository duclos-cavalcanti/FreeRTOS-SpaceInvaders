#ifndef __main_H__
#define __main_H__

#define INITIAL_POINTS_THRESHOLD 1080


#define PLAYERSHIP_HEIGHT 10
#define PLAYERSHIP_WIDTH 30
#define PLAYERSHIP_Y_BEGIN SCREEN_HEIGHT*90/100

#define CREATURE_HEIGHT 21
#define CREATURE_WIDTH 27

#define BOTTOM_WALLTHICKNESS 3
#define BOTTOM_WALLPOSITION SCREEN_HEIGHT*95/100

#define CREATURE_SHOT_ANIMATION_W 25
#define CREATURE_SHOT_ANIMATION_H 24

#define SAUCER_SHOT_ANIMATION_W 31
#define SAUCER_SHOT_ANIMATION_H 30

void checkDraw(unsigned char status, const char *msg);

typedef enum Columns_t{
    Column_1,
    Column_2,
    Column_3,
    Column_4,
    Column_5,
    Column_6,
    Column_7,
    Column_8
}Columns_t;

typedef enum Rows_t{
    Row_1,
    Row_2,
    Row_3,
    Row_4,
    Row_5
}Rows_t;

#define BEGIN MainMenuState
typedef enum GameState_t{
    MainMenuState,
    PlayingState,
    NextLevelState,
    PausedState,
    GameOverState,
    ResetGameState
}GameState_t;

typedef enum SelectedMenuOption_t{
    SinglePlayer,
    MultiPlayer,
    Cheats,
    Leave
}SelectedMenuOption_t;

typedef enum PlayerOutsideGameActions_t{
    NoAction,
    PauseGameAction,
    LostGameAction,
    WonGameAction,
    ResetGameAction
}PlayerOutsideGameActions_t;

typedef enum SelectedPausedGameOption_t{
    Resume,
    RestartReset
}SelectedPausedGameOption_t;

typedef enum SelectedGameOverOption_t{
    PlayAgain,
    Quit
}SelectedGameOverOption_t;

typedef enum LivesAnimation_t{
    LivesIntact,
    LivesLost,
    LivesGained
}LivesAnimation_t;

typedef enum TypesOfNewGames_t{
    NewGameFromScratch,
    NewGameNextLevel
}TypesOfNewGames_t;


void vHandleStateMachineActivation();
#endif 
