#include "fsm.h"
#include "config.h"

typedef enum _HandlerIndex {
    DO_ENTER = 0,
    DO_PRESS = 1,
    DO_RELEASE = 2,
    DO_EXIT = 3,
    HandlerIndex_SIZE = 4,
} HandlerIndex;




typedef MappingState (*ActiveMappingHandler)(Fsm *self, FsmEvent *event, Mapping *activeMapping);

typedef ButtonState (*StateHandler)(Fsm *self, FsmEvent *event);


MappingState Fsm__call_mapping_state_handler(Fsm *self, FsmEvent *event, HandlerIndex handler, Mapping *activeMapping);

void fsm__handle_mapping_event(Fsm *self, FsmEvent *event, Mapping *activeMapping)
{
    // Call the handler for the current state when the button is pressed
    ButtonState next_state = Fsm__call_mapping_state_handler(self, event,
        event->is_pressed? DO_PRESS : DO_RELEASE, activeMapping);
 
    if (next_state != self->current_state) {
        // state change requested by the handler!
        // Call the exit handler of the current state
        Fsm__call_mapping_state_handler(self, event, DO_EXIT, activeMapping);
        self->current_state = next_state;
        // Call the exit handler of the current state
        Fsm__call_mapping_state_handler(self, event, DO_ENTER, activeMapping);
    }
}

// No handling Callback

MappingState No_Mapping_Handling(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    return self->mapping_state;
}

ButtonState No_Button_Handling(Fsm *self, FsmEvent *event) {
    return self->current_state;
}

// REST

MappingState REST__OnPress(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    return STATE_START;
}

MappingState REST__OnRelease(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    // Used to trigger delayed taps
    return STATE_TAP;
}
// START

MappingState START__OnEntry(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    // update chord state
    Fsm__call_mapping_state_handler(self, event, OnPress, &self->press_actions);
    self->timestamp = event->now; // Mark start press time
    return self->mapping_state;
}

MappingState START__OnPress(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    uint64_t hold_time = self->long_hold? CFG_HOLD_LONG_TIME : CFG_HOLD_TIME;
    if (event->now - self->timestamp > hold_time)
    {
        return STATE_HOLD;
    }
    return self->current_state;
}

MappingState START__OnRelease(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    Fsm__call_mapping_state_handler(self, event, OnRelease, &self->press_actions);
    // Delay Tap actions when a double press is 
    return self->double_press_actions.count == 0 ? STATE_TAP : STATE_REST;
}

// TAP

MappingState TAP__OnEntry(Fsm *self, FsmEvent *event, Mapping *activeMapping)
{
    Fsm__call_mapping_state_handler(self, event, OnTap, &self->press_actions);
    self->timestamp = event->now; // Mark start press time
    return self->mapping_state;
}

MappingState TAP__OnPress(Fsm *self, FsmEvent *event, Mapping *activeMapping)
{
    // Try to catch up! new press has started already
    return STATE_REST;
}

MappingState TAP__OnRelease(Fsm *self, FsmEvent *event, Mapping *activeMapping)
{
    if (event->now - self->timestamp > CFG_PULSE_TIME)
    {
        return STATE_REST;
    }
    return self->current_state;
}

// HOLD

MappingState HOLD__OnEntry(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    Fsm__call_mapping_state_handler(self, event, OnHold, &self->press_actions);
    Fsm__call_mapping_state_handler(self, event, OnTurbo, &self->press_actions);
    self->timestamp = time_us_64(); // Start counting turbo press
}


MappingState HOLD__OnPress(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    uint64_t now = time_us_64();
    if (now - self->timestamp > CFG_TURBO_TIME) {
        Fsm__call_mapping_state_handler(self, event, OnTurbo, &self->press_actions);
        self->timestamp = now;
    }
    return self->current_state;
}

MappingState HOLD__OnRelease(Fsm *self, FsmEvent *event, Mapping *activeMapping) {
    Fsm__call_mapping_state_handler(self, event, OnHoldRelease, &self->press_actions);
    Fsm__call_mapping_state_handler(self, event, OnRelease, &self->press_actions);
    return STATE_REST;
}

// NoPress

ButtonState NoPress__OnEntry(Fsm *self, FsmEvent *event)
{
    // pop chord stack
}

ButtonState NoPress__OnPress(Fsm *self, FsmEvent *event)
{
    // check for sim mapping: WaitSim
    if (self->double_press_actions.count > 0)
    {
        return STATE_DblPressStart;
    }
    // check for diag mapping: DiagPressMaster/Slave
    return STATE_BtnPress;
}

ButtonState NoPress_OnExit(Fsm *self, FsmEvent *event)
{
    // push chord stack
}

ButtonState BtnPress__OnEvent(Fsm *self, FsmEvent *event)
{
    fsm__handle_mapping_event(self, event, &self->press_actions);
    if (self->mapping_state == STATE_REST)
    {
        return STATE_NoPress;
    }
    return self->current_state;
}

ButtonState DblPressStart__OnEvent(Fsm *self, FsmEvent *event)
{
    // Process any OnPress actions
    fsm__handle_mapping_event(self, event, &self->press_actions);
    if (self->mapping_state == STATE_HOLD)
    {
        // If the first tap was so long that it triggers the long press,
        // Abandon double press possibility
        return STATE_BtnPress;
    }
    if (self->mapping_state == STATE_REST)
    {
        return event->now - self->timestamp < CFG_DOUBLE_PRESS_TIME ? STATE_DblPressNoPress : STATE_NoPress;
    }
    return self->current_state;
}


ButtonState DblPressNoPress__OnPress(Fsm *self, FsmEvent *event)
{
    return STATE_DblPressPress;
}

ButtonState DblPressNoPress__OnRelease(Fsm *self, FsmEvent *event)
{
    if (event->now - self->timestamp > CFG_DOUBLE_PRESS_TIME)
    {
        // Process any Tap actions now that the double press window has passed
        fsm__handle_mapping_event(self, event, &self->press_actions);
        if (self->mapping_state == STATE_REST)
        {
            return STATE_NoPress;
        }
    }
    return self->current_state;
}


ButtonState DblPressPress__OnEvent(Fsm *self, FsmEvent *event)
{
    // Process any double press mappings actions
    fsm__handle_mapping_event(self, event, &self->double_press_actions);
    if (self->mapping_state == STATE_REST)
    {
        return STATE_NoPress;
    }
    return self->current_state;
}

ButtonState Fsm__call_button_state_handler(Fsm *self, FsmEvent *event, HandlerIndex handler)
{
    // Handler map for each event for each state of the external state machine
    static StateHandler Button_state_map[ButtonState_SIZE][HandlerIndex_SIZE] = {
    //                   OnEntry             OnPress                   OnRelease                   OnExit
    /*NoPress*/         {NoPress__OnEntry,   NoPress__OnPress,         No_Button_Handling,         NoPress_OnExit  },
    /*BtnPress*/        {No_Button_Handling, BtnPress__OnEvent,        BtnPress__OnEvent,          No_Button_Handling},
    /*DblPressStart*/   {No_Button_Handling, DblPressStart__OnEvent,   DblPressStart__OnEvent,     No_Button_Handling},
    /*DblPressNoPress*/ {No_Button_Handling, DblPressNoPress__OnPress, DblPressNoPress__OnRelease, No_Button_Handling},
    /*DblPressPress*/   {No_Button_Handling, DblPressPress__OnEvent,   DblPressPress__OnEvent,     No_Button_Handling},
    // ...
    };

    // beautifier function
    return Button_state_map[self->current_state][handler](self, event);
}

MappingState Fsm__call_mapping_state_handler(Fsm *self, FsmEvent *event, HandlerIndex handler, Mapping *activeMapping)
{
    // Handler map for each event for each state of the internal state machine
    static ActiveMappingHandler active_state_map[MappingState_SIZE][HandlerIndex_SIZE] = {
    //          OnEntry         OnPress         OnRelease         OnExit
    /*REST*/   {No_Mapping_Handling,    REST__OnPress,  REST__OnRelease,      No_Mapping_Handling  },
    /*START*/  {START__OnEntry, START__OnPress, START__OnRelease, No_Mapping_Handling},
    /*TAP*/    {TAP__OnEntry, TAP__OnPress, TAP__OnRelease, No_Mapping_Handling},
    /*HOLD*/   {HOLD__OnEntry,  HOLD__OnPress,  HOLD__OnRelease,  No_Mapping_Handling },
    };

    // beautifier function
    return active_state_map[self->mapping_state][handler](self, event, activeMapping);
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


void Fsm__raiseEvent(Fsm *self, Mapping *activeMapping, ButtonEvent event) {
    assert(self && activeMapping && event != NoEvent);
    for (int i = 0 ; i < MAX_ACTIONS_PER_EVENT ; ++i)
    {
        EventAction_Callback cb = activeMapping->_eventMapping[event][i];
        if (cb.func != 0) {
            // debug log
            cb.func(cb.param); // Called the mapped action
        }
    }
}

Fsm make_fsm()
{
    Fsm fsm;
    fsm.current_state = STATE_NoPress;
    fsm.mapping_state = STATE_REST;
    init_mapping(&fsm.press_actions);
}
