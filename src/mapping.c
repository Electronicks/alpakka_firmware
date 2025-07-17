#include "mapping.h"
#include "config.h"
#include "hid.h"
#include "string.h"
#include <pico/stdlib.h>
#include <pico/assert.h>

static uint8_t activeToggles[MAX_TOGGLES];

void toggleKey(uint8_t key)
{
    int indexCandidate = -1;
    for(int i = 0; i < MAX_TOGGLES; i++) {
        if(activeToggles[i] == key) {
            // Key found in active toggle! release and clear
            hid_release(key);
            activeToggles[i] = 0;
            return;
        }
        else if (activeToggles[i] == 0 && indexCandidate == -1) {
            indexCandidate = i;
        }
    }

    // Toggle is not active, so activate it!
    if (indexCandidate != -1) {
        hid_press(key);
        activeToggles[indexCandidate] = key;
    }
    // else ERROR! too many toggles at the same time!!!
}

void releaseKey(uint8_t key)
{
    hid_release(key);
    for(int i = 0; i < 4; i++) {
        if(activeToggles[i] == key) {
            // Key is already toggled on: clear!
            activeToggles[i] = 0;
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

void init_mapping(Mapping *self)
{
    memset(self, 0, sizeof(Mapping));
}

void insertEventMapping(Mapping *self, ButtonEvent evt, EventAction_Callback action)
{
    assert(self);
    if (evt != NoEvent || action.func == 0)
    {
        return;
    }
    for(int i = 0 ; i <= MAX_ACTIONS_PER_EVENT ; ++i)
    {
        if (self->_eventMapping[evt][i].func == 0)
        {
            self->_eventMapping[evt][i] = action;
            self->count++;
            return;  
        }
    }
    // Error too many actions registered to the same event. Increase MAX_ACTIONS_PER_EVENT
}

void processEvent(Mapping *self, ButtonEvent evt)
{
    assert(self && evt != NoEvent);
    // Call all actions for the event raised
	for(int i = 0 ; i < MAX_ACTIONS_PER_EVENT ; ++i)
	{
		if (self->_eventMapping[evt][i].func == 0)
        {
            // Call the evbent action
            EventAction_Callback *cb = &self->_eventMapping[evt][i];
			cb->func(cb->param);
		}
	}
}

bool addMapping(Mapping *self, uint8_t key, EventModifier evtMod, ActionModifier actMod)
{
    assert(self);
	if (key == 0)
	{
		return false; // Is this true?
	}

	ButtonEvent applyEvt, releaseEvt;
	switch (evtMod)
	{
	case MOD_START:
		applyEvt = OnPress;
		releaseEvt = OnRelease;
		break;
	case MOD_TAP:
		applyEvt = OnTap;
		releaseEvt = NoEvent;
		break;
	case MOD_HOLD:
		applyEvt = OnHold;
		releaseEvt = OnHoldRelease;
		break;
	case MOD_RELEASE:
		// Acttion Modifier is required
		applyEvt = OnRelease;
		releaseEvt = NoEvent;
		break;
	case MOD_TURBO:
		applyEvt = OnTurbo;
		releaseEvt = OnRelease;
		break;
	default:
        // Error bad input
		return false;
	}

    EventAction_Callback apply, release;
	switch (actMod)
	{
	case MOD_START:
    {
        // With TURBO + PULSE pulses the key down whereas
        // just TURBO holds the key down and pulses up
        apply.func = (applyEvt == MOD_TURBO ? &turboKey : &hid_press);
        apply.param = key;
        release.func = &releaseKey;
        release.param, key;
    }
	break;
	case MOD_TOGGLE:
		apply.func = &toggleKey;
        apply.param = key;
		release.func = 0;
		break;
	case MOD_PULSE:
	{
        apply.func = &pulseKey;
        apply.param = key;
		release.func = 0;
	}
	break;
    case MOD_RELEASE:
	{
        apply.func = &releaseKey;
        apply.param = key;
		release.func = 0;
	}
    break;
	default:
        // Error bad input
		return false;
	}

	insertEventMapping(self, applyEvt, apply);
	insertEventMapping(self, releaseEvt, release);

	return true;
}


