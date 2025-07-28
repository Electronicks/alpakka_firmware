#include "mapping.h"
#include "config.h"
#include "hid.h"
#include "string.h"
#include <pico/stdlib.h>
#include <pico/assert.h>

// Make a circular buffer?
static uint8_t s_active_toggles[ACTIONS_LEN] = {0, 0, 0, 0};


// Callable action:

// void press_key(uint8_t key) { hid_press(key); }; // Useless wrapper

void toggleKey(uint8_t key)
{
    int i_candidate = -1;
    for(int i = 0; i < ACTIONS_LEN; i++) {
        if(s_active_toggles[i] == key) {
            // Key found in active toggle! release and clear
            hid_release(key);
            s_active_toggles[i] = 0;
            return;
        }
        else if (s_active_toggles[i] == 0 && i_candidate == -1) {
            i_candidate = i;
        }
    }

    // Toggle is not active, so activate it!
    if (i_candidate != -1) {
        hid_press(key);
        s_active_toggles[i_candidate] = key;
    }
    // else ERROR! too many toggles at the same time!!!
}

void releaseKey(uint8_t key)
{
    hid_release(key);
    for(int i = 0; i < 4; i++) {
        if(s_active_toggles[i] == key) {
            // Key is already toggled on: clear!
            s_active_toggles[i] = 0;
            return;
        }
    }
}

void pulseKey(uint8_t key)
{
    hid_press(key);
    hid_release_later(key, CFG_PULSE_TIME); // I wish I could clear later too
}

void turboKey(uint8_t key)
{
    hid_release(key);
    hid_press_later(key, CFG_TURBO_TIME);
}

Mapping make_mapping()
{
    Mapping map;
    map.count = 0;
    for (int i = 0 ; i < MAPPING_SIZE ; ++i)
    {
        map.event_mapping[i].event = NoEvent;
        map.event_mapping[i].func = 0;
        for(int j = 0 ; j < ACTIONS_LEN ; ++j)
        {
            map.event_mapping[i].param[j] = KEY_NONE;
        }
    }
    return map;
}

void insertEventMapping(Mapping *self, EventAction_Callback action)
{
    assert(self);
    if (action.event != NoEvent || action.func == 0)
    {
        return;
    }

    int i_candidate = -1;
    for(int i = 0 ; i <= MAPPING_SIZE ; ++i)
    {
        if (self->event_mapping[i].event == action.event && self->event_mapping[i].func == action.func)
        {
            for(int j = 0 ; j < ACTIONS_LEN ; ++j)
            {
                if (self->event_mapping[i].param[j] == KEY_NONE)
                {
                    // Add key press to existing mapping on same action and event
                    self->event_mapping[i].param[j] = action.param[0];
                    self->count++;
                    return;  
                }
            }
        }
        else if (self->event_mapping[i].event ==NoEvent)
        {
            i_candidate = i;
        }
    }

    if (i_candidate != -1)
    {
        self->event_mapping[i_candidate] = action;
        self->count++;   
    }
    // Error actions registered on too many different events. Increase MAPPING_SIZE
}

void processEvent(Mapping *self, ButtonEvent evt)
{
    assert(self && evt != NoEvent);
    // Call all actions for the event raised
	for(int i = 0 ; i < MAPPING_SIZE ; ++i)
	{
		if (self->event_mapping[i].event == evt && self->event_mapping[evt].func != 0)
        {
            EventAction_Callback *cb = &self->event_mapping[evt];
            for(int j = 0 ; j < ACTIONS_LEN ; ++j)
            {
                info("BTN: Calling action %c for event %d\n", cb->param[j], evt);
                // Call the event actions
                cb->func(cb->param[j]);
            }
		}
	}
}

bool addMapping(Mapping *self, uint8_t key, EventModifier evtMod, ActionModifier actMod)
{
    assert(self);
    info("Adding %d for %c on %d\n", actMod, key, evtMod);

	EventAction_Callback apply, release;
	switch (evtMod)
	{
	case MOD_START:
		apply.event = OnPress;
		release.event = OnRelease;
		break;
	case MOD_TAP:
		apply.event = OnTap;
		release.event = NoEvent;
		break;
	case MOD_HOLD:
		apply.event = OnHold;
		release.event = OnHoldRelease;
		break;
	case MOD_RELEASE:
		// Acttion Modifier is required
		apply.event = OnRelease;
		release.event = NoEvent;
		break;
	case MOD_TURBO:
		apply.event = OnTurbo;
		release.event = OnRelease;
		break;
	default:
        // Error bad input
		return false;
	}

	switch (actMod)
	{
	case MOD_START:
    {
        // With TURBO + PULSE pulses the key down whereas
        // just TURBO holds the key down and pulses up
        apply.func = (evtMod == MOD_TURBO ? &turboKey : &hid_press);
        apply.param[0] = key;
        release.func = &releaseKey;
        release.param[0] = key;
    }
	break;
	case MOD_TOGGLE:
		apply.func = &toggleKey;
        apply.param[0] = key;
		release.func = 0;
		break;
	case MOD_PULSE:
	{
        apply.func = &pulseKey;
        apply.param[0] = key;
		release.func = 0;
	}
	break;
    case MOD_RELEASE:
	{
        apply.func = &releaseKey;
        apply.param[0] = key;
		release.func = 0;
	}
    break;
	default:
        // Error bad input
		return false;
	}

	insertEventMapping(self, apply);
	insertEventMapping(self, release);

	return true;
}


