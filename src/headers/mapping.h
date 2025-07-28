#pragma once

#include <stdbool.h>
#include <pico/stdlib.h>

// old interface
#define ACTIONS_LEN 4

typedef uint8_t Actions[ACTIONS_LEN];


// new interface
// Action modifiers (aka how the key up and down events are called)
typedef enum _ActionModifiers {
    MOD_PRESS = 0,   // default: key down while button press
    MOD_PULSE = 1,   // Key up shortly after key down (CFG_PULSE_TIME)
    MOD_TOGGLE = 2,  // Alternate key down and key up
    MOD_KEYUP = 3,   // Send key up and clear toggles
} ActionModifier;

// Event modifiers (aka when the key presses activates)
typedef enum _EventModifiers {
    MOD_START=0,  // default : Process actions as soon as button is pressed
    MOD_TAP,      // Process actions upon the release of a short press (< CFG_HOLD[_LONG]_TIME)
    MOD_HOLD,     // Process actions after the button was held (> CFG_HOLD[_LONG]_TIME)
    MOD_RELEASE,  // Process actions upon the release of the button
    MOD_TURBO,    // Process actions periodically starting after hold time (CFG_TURBO_TIME)
} EventModifier;


typedef enum _ButtonEvent {
    NoEvent=0, // invalid value
	OnPress, 
	OnTap,
	OnHold,
	OnTurbo,
	OnRelease,
    OnHoldRelease,
    NUM_EVENTS = OnHoldRelease, // Always = last
} ButtonEvent;


// The signature of any callback actions
typedef void (*EventAction_Callback_fn)(uint8_t);

// A callback functor object with stored parameter
typedef struct EventAction_Callback_s
{
    ButtonEvent event;
    EventAction_Callback_fn func;
    Actions param;
} EventAction_Callback;

#define MAPPING_SIZE 3 // 5 is too much

// This structure handles the mapping of a button, buy processing and action
// to be done on tap, hold, turbo and others. It holds a map of actions to perform
// when a specific event happens. This replaces the old Mapping structure.
typedef struct Mapping_s
{
    // Each callback is tied to one event and one function
    // but can have up to ACTIONS_LEN keys bound to it
    EventAction_Callback event_mapping[MAPPING_SIZE];
    int count;
} Mapping;

// Initialize a mapping structure
Mapping make_mapping();

// Call all actions bound to this event
void processEvent(Mapping *self,ButtonEvent evt);

// Register actions to the proper events for the provided mapping
bool addMapping(Mapping *self, uint8_t key, EventModifier evtMod, ActionModifier actMod);
