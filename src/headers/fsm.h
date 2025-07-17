#pragma once

#include "mapping.h"

#include <stdbool.h>

// Internal state machine
typedef enum _MappingState {
    STATE_REST = 0,     // Button at rest, no active mappings
    STATE_START,    // Buttoon has just been pressed
    STATE_TAP,
    STATE_HOLD,     // Button has been held
    MappingState_SIZE,
} MappingState;

// external state machine
typedef enum _ButtonState {
    STATE_NoPress = 0,          // Button is not pressed
    STATE_BtnPress,             // Button is pressed, basic mappings are active
    // STATE_TapPress, // not necessary anymore
    STATE_DblPressStart,        // Start of first tap of a double press
    STATE_DblPressNoPress,      // After the first tap of a double press
    // STATE_DblPressNoPressTap,   // After the first tap when a tap is active
    STATE_DblPressPress,        // Double press mapping active
    // STATE_WaitSim,
    // STATE_SimPressMaster,
    // STATE_SimPressSlave,
    // STATE_SimRelease,
    // STATE_DiagPressSlave,
    // STATE_DiagRelease,
    ButtonState_SIZE,
} ButtonState;

typedef struct Fsm_s
{
    ButtonState current_state; // selects the row from the state_map
    MappingState mapping_state;
    Mapping press_actions;
    Mapping double_press_actions;
    uint64_t timestamp;
    bool long_hold;
} Fsm;

Fsm make_fsm();

typedef struct FsmEvent_s
{
    bool is_pressed;
    uint64_t now;
} FsmEvent;

void fsm__handle_event(Fsm *fsm, FsmEvent *event);
