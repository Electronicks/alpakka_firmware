#include "fsm.h"
#include "config.h"

// Identifier for State Handler functions
typedef enum _Handler {
    DO_ENTER = 0,
    DO_PRESS,
    DO_RELEASE,
    DO_EXIT,
    // DO_SYNC
    Handler_SIZE,
} Handler;

// ***********************
// INTERNAL STATE MACHINE
// ***********************

// Signature of a ActivationState Callback handler
typedef ActivationState (*ActiveMappingHandler)(Fsm *self, FsmEvent *event, Mapping *active_mapping);

// REST

ActivationState REST__OnPress(Fsm *self, FsmEvent *event, Mapping *active_mapping) {
    return ACT_START;
}

ActivationState REST__OnRelease(Fsm *self, FsmEvent *event, Mapping *active_mapping) {
    // Used to trigger delayed taps
    return ACT_TAP;
}

// START

ActivationState START__OnEntry(Fsm *self, FsmEvent *event, Mapping *active_mapping) {
    // update chord state
    processEvent(active_mapping, OnPress);
    self->timestamp = event->now; // Mark start press time
    return self->activation_state;
}

ActivationState START__OnPress(Fsm *self, FsmEvent *event, Mapping *active_mapping) {
    uint64_t hold_time = self->long_hold? CFG_HOLD_LONG_TIME : CFG_HOLD_TIME;
    if (event->now - self->timestamp > hold_time)
    {
        // Button was held for long enough
        return ACT_HOLD;
    }
    return self->activation_state;
}

ActivationState START__OnRelease(Fsm *self, FsmEvent *event, Mapping *active_mapping) {
    processEvent(active_mapping, OnRelease);
    // Delay Tap actions when a double press is
    return self->double_press_actions.count == 0 ? ACT_TAP : ACT_REST;
}

// TAP

ActivationState TAP__OnEntry(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    processEvent(active_mapping, OnTap);
    self->timestamp = event->now; // Mark start press time
    return self->activation_state;
}

ActivationState TAP__OnPress(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    // Try to catch up! new press has started already
    return ACT_REST;
}

ActivationState TAP__OnRelease(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    // Stay in this state until pulses have finished processing
    if (event->now - self->timestamp > CFG_PULSE_TIME)
    {
        return ACT_REST;
    }
    return self->current_state;
}

// HOLD

ActivationState HOLD__OnEntry(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    processEvent(active_mapping, OnHold);
    processEvent(active_mapping, OnTurbo);
    self->timestamp = event->now; // Start counting turbo press
    return self->activation_state;
}


ActivationState HOLD__OnPress(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    if (event->now - self->timestamp > CFG_TURBO_TIME) {
        processEvent(active_mapping, OnTurbo);
        self->timestamp = event->now;
    }
    return self->current_state;
}

ActivationState HOLD__OnRelease(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    processEvent(active_mapping, OnHoldRelease);
    processEvent(active_mapping, OnRelease);
    return ACT_REST;
}

// No handling Callback
ActivationState Do_Nothing_ACT(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    return self->activation_state;
}

ActivationState Fsm__call_activation_state_handler(Fsm *self, FsmEvent *event, Mapping *active_mapping, Handler handler)
{
    // Handler map for each event for each state of the internal state machine
    static ActiveMappingHandler active_state_map[ActivationState_SIZE][Handler_SIZE] = {
    //          OnEntry         OnPress         OnRelease         OnExit
    /*REST*/   {Do_Nothing_ACT, REST__OnPress,  REST__OnRelease,  Do_Nothing_ACT},
    /*START*/  {START__OnEntry, START__OnPress, START__OnRelease, Do_Nothing_ACT},
    /*TAP*/    {TAP__OnEntry,   TAP__OnPress,   TAP__OnRelease,   Do_Nothing_ACT},
    /*HOLD*/   {HOLD__OnEntry,  HOLD__OnPress,  HOLD__OnRelease,  Do_Nothing_ACT},
    };

    // beautifier function
    return active_state_map[self->activation_state][handler](self, event, active_mapping);
}


void fsm__handle_mapping_event(Fsm *self, FsmEvent *event, Mapping *active_mapping)
{
    // Call the handler for the current state when the button is pressed
    ButtonState next_state = Fsm__call_activation_state_handler(self, event,
        active_mapping, event->is_pressed? DO_PRESS : DO_RELEASE);
 
    if (next_state != self->current_state) {
        // state change requested by the handler!
        // Call the exit handler of the current state
        Fsm__call_activation_state_handler(self, event, active_mapping, DO_EXIT);
        self->current_state = next_state;
        // Call the exit handler of the current state
        Fsm__call_activation_state_handler(self, event, active_mapping, DO_ENTER);
    }
}


// ***********************
// EXTERNAL STATE MACHINE
// ***********************

// Signature of a ButtonState Callback handler
typedef ButtonState (*StateHandler)(Fsm *self, FsmEvent *event);

// NoPress

ButtonState NO_PRESS__OnEntry(Fsm *self, FsmEvent *event)
{
    // pop chord stack
    return self->current_state;
}

ButtonState NO_PRESS__OnPress(Fsm *self, FsmEvent *event)
{
    // check for sim mapping: WaitSim
    if (self->double_press_actions.count > 0)
    {
        return BTN_DBL_PRESS_START;
    }
    // check for diag mapping: DiagPressMaster/Slave
    return BTN_PRESS;
}

ButtonState NO_PRESS__OnExit(Fsm *self, FsmEvent *event)
{
    // push chord stack
    return self->current_state;
}

ButtonState PRESS__OnEvent(Fsm *self, FsmEvent *event)
{
    fsm__handle_mapping_event(self, event, &self->press_actions);
    if (self->activation_state == ACT_REST)
    {
        return BTN_NO_PRESS;
    }
    return self->current_state;
}

ButtonState DBL_PRESS_START_ON_EVENT(Fsm *self, FsmEvent *event)
{
    // Process any OnPress actions
    fsm__handle_mapping_event(self, event, &self->press_actions);
    if (self->activation_state == ACT_HOLD)
    {
        // If the first tap was so long that it triggers the long press,
        // Abandon double press possibility
        return BTN_PRESS;
    }
    if (self->activation_state == ACT_REST)
    {
        return event->now - self->timestamp < CFG_DOUBLE_PRESS_TIME ? BTN_DBL_PRESS_NO_PRESS : BTN_NO_PRESS;
    }
    return self->current_state;
}


ButtonState DBL_PRESS_NO_PRESS__OnPress(Fsm *self, FsmEvent *event)
{
    return BTN_DBL_PRESS_PRESS;
}

ButtonState DBL_PRESS_NO_PRESS__OnRelease(Fsm *self, FsmEvent *event)
{
    if (event->now - self->timestamp > CFG_DOUBLE_PRESS_TIME)
    {
        // Process any Tap actions now that the double press window has passed
        fsm__handle_mapping_event(self, event, &self->press_actions);
        if (self->activation_state == ACT_REST)
        {
            return BTN_NO_PRESS;
        }
    }
    return self->current_state;
}


ButtonState DBL_PRESS_PRESS__OnEvent(Fsm *self, FsmEvent *event)
{
    // Process any double press mappings actions
    fsm__handle_mapping_event(self, event, &self->double_press_actions);
    if (self->activation_state == ACT_REST)
    {
        return BTN_NO_PRESS;
    }
    return self->current_state;
}

// No handling Callback
ButtonState Do_Nothing_BTN(Fsm *self, FsmEvent *event) {
    return self->current_state;
}

ButtonState Fsm__call_button_state_handler(Fsm *self, FsmEvent *event, Handler handler)
{
    // Handler map for each event for each state of the external state machine
    static StateHandler Button_state_map[ButtonState_SIZE][Handler_SIZE] = {
    //                   OnEntry            OnPress                      OnRelease                      OnExit
    /*NoPress*/         {NO_PRESS__OnEntry, NO_PRESS__OnPress,           Do_Nothing_BTN,                NO_PRESS__OnExit},
    /*BtnPress*/        {Do_Nothing_BTN,    PRESS__OnEvent,              PRESS__OnEvent,                Do_Nothing_BTN},
    /*DblPressStart*/   {Do_Nothing_BTN,    DBL_PRESS_START_ON_EVENT,    DBL_PRESS_START_ON_EVENT,      Do_Nothing_BTN},
    /*DblPressNoPress*/ {Do_Nothing_BTN,    DBL_PRESS_NO_PRESS__OnPress, DBL_PRESS_NO_PRESS__OnRelease, Do_Nothing_BTN},
    /*DblPressPress*/   {Do_Nothing_BTN,    DBL_PRESS_PRESS__OnEvent,    DBL_PRESS_PRESS__OnEvent,      Do_Nothing_BTN},
    // ...
    };

    // Call correct handler for current state and handler
    return Button_state_map[self->current_state][handler](self, event);
}


void fsm__handle_event(Fsm *self, FsmEvent *event)
{
    // Call the handler for the current state when the button is pressed
    ButtonState next_state = Fsm__call_button_state_handler(self, event,
        event->is_pressed? DO_PRESS : DO_RELEASE);
 
    if (next_state != self->current_state) {
        // state change requested by the handler!
        // Call the exit handler of the current state
        Fsm__call_button_state_handler(self, event, DO_EXIT);
        self->current_state = next_state;
        // Call the exit handler of the current state
        Fsm__call_button_state_handler(self, event, DO_ENTER);
    }
}

Fsm make_fsm()
{
    Fsm fsm;
    fsm.current_state = BTN_NO_PRESS;
    fsm.activation_state = ACT_REST;
    fsm.press_actions = make_mapping();
    fsm.double_press_actions = make_mapping();
    fsm.timestamp = 0;
    fsm.long_hold = false;
    return fsm;
}
