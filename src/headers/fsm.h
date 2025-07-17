#pragma once

#include "mapping.h"
#include <stdbool.h>

// Internal state machine states following activation of a mapping
typedef enum _ActivationState {
    ACT_REST = 0, // No active mappings
    ACT_START,    // Mapping calls OnPress actions
    ACT_TAP,      // Mapping calls OnTap actions
    ACT_HOLD,     // Mapping calls On Hold and Turbo events
    ActivationState_SIZE,
} ActivationState;

// External state machine states tracks the buttons state and enables correct mapping
typedef enum _ButtonState {
    BTN_NO_PRESS = 0,           // Button is not pressed
    BTN_PRESS,                  // Button is pressed, basic mappings are active
    BTN_DBL_PRESS_START,        // Start of first tap of a double press
    BTN_DBL_PRESS_NO_PRESS,     // After the first tap of a double press
    BTN_DBL_PRESS_PRESS,        // Double Press mappings are activated
    ButtonState_SIZE,
} ButtonState;

// Finite State Machine object.
typedef struct Fsm_s
{
    // Current State of the external state machine
    ButtonState current_state;

    // Current State of the internal state machine
    ActivationState activation_state;

    // Basic Mappings
    Mapping press_actions;

    // Double Press Mappings
    Mapping double_press_actions;

    // Advanced Mappings Press Mappings // Can be layer, chord, or something else
    //Mapping advanced_press_actions;

    uint64_t timestamp; // Internal timestamp to track time
    bool long_hold;     // Use long hold parameter
} Fsm;

// Initialize an FSM object
Fsm make_fsm();

// FSM event to process
typedef struct FsmEvent_s
{
    // Current state of the button
    bool is_pressed;

    // Very recent timestamp
    uint64_t now;
} FsmEvent;

// Get the FSM to process an event, possibly changing states, triggering events and calling actions
void fsm__handle_event(Fsm *fsm, FsmEvent *event);
