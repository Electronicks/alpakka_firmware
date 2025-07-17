#pragma once

#include <stdbool.h>
#include <pico/stdlib.h>

#define MAX_ACTIONS_PER_EVENT 4 
#define MAX_TOGGLES 4

// The list of different function that can be bound in the mapping
typedef void (*EventAction_Callback_fn)(uint8_t);
typedef struct EventAction_Callback_s
{
    EventAction_Callback_fn func;
    uint8_t param;
} EventAction_Callback;

// new interface
// Key modifiers
typedef enum _ActionModifiers {
    MOD_PRESS = 0,   // default: key down while button press
    MOD_PULSE = 1,   // Key up shortly after key down (CFG_PULSE_TIME)
    MOD_TOGGLE = 2,  // Alternate key down and key up
    MOD_KEYUP = 3,   // Send key up and clear toggles
} ActionModifier;

typedef enum _EventModifiers {
    MOD_START=0,  // default : Process actions as soon as button is pressed
    MOD_TAP,      // Process actions upon the release of a short press (< CFG_HOLD_TIME)
    MOD_HOLD,     // Process actions after the button was held (> CFG_HOLD_TIME)
    MOD_RELEASE,  // Process actions upon the release of the button
    MOD_TURBO,    // Process actions periodically starting after hold time (CFG_TURBO_TIME)
} EventModifier;


typedef enum _ButtonEvent {
    NoEvent=-1,
	OnPress,
	OnTap,
	OnHold,
	OnTurbo,
	OnRelease,
    OnHoldRelease,
    // Always last
    NUM_EVENTS,
} ButtonEvent;


// This structure handles the mapping of a button, buy processing and action
// to be done on tap, hold, turbo and others. It holds a map of actions to perform
// when a specific event happens. This replaces the old Mapping structure.
typedef struct Mapping_s
{
    EventAction_Callback _eventMapping[NUM_EVENTS][MAX_ACTIONS_PER_EVENT];
    int count;
} Mapping;

// public
void init_mapping(Mapping *self);

void processEvent(Mapping *self,ButtonEvent evt);

bool addMapping(Mapping *self, uint8_t key, EventModifier evtMod, ActionModifier actMod);
