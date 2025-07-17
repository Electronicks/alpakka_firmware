// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#include <stdio.h>
#include <string.h>
#include <pico/time.h>
#include <hardware/gpio.h>
#include "button.h"
#include "config.h"
#include "hid.h"
#include "bus.h"
#include "pin.h"
#include "common.h"

bool Button__is_pressed(Button *self) {
    bool is_pressed = false;

    if (self->pin == PIN_NONE) return false;
    // Virtual buttons.
    else if (self->pin == PIN_VIRTUAL) {
        if (self->virtual_press) {
            self->virtual_press = false;
            return true;
        } else {
            return false;
        }
    }
    // Buttons connected directly to Pico.
    else if (is_between(self->pin, PIN_GROUP_BOARD, PIN_GROUP_BOARD_END)) {
        is_pressed =  !gpio_get(self->pin);
    }
    // Buttons connected to 1st IO expander.
    else if (is_between(self->pin, PIN_GROUP_IO_0, PIN_GROUP_IO_0_END)) {
        is_pressed =  bus_i2c_io_cache_read(0, self->pin - PIN_GROUP_IO_0);
    }
    // Buttons connected to 2nd IO expander.
    else if (is_between(self->pin, PIN_GROUP_IO_1, PIN_GROUP_IO_1_END)) {
        is_pressed =  bus_i2c_io_cache_read(1, self->pin - PIN_GROUP_IO_1);
    }

    // debounce processing
    if (self->press_timestamp == 0 && is_pressed)
    {
        // start of debounce process
        self->press_timestamp = time_us_64();
    }
    else if (self->press_timestamp > 0 && time_us_64()-self->press_timestamp < CFG_PRESS_DEBOUNCE*1000)
    {
        // is_pressed doesn't matter: we're debouncing
        is_pressed = true;
    }
    else if (self->press_timestamp > 0 && !is_pressed)
    {
        // We're out of the debounce window : clear timestamp
        self->press_timestamp = 0;
    }
    return is_pressed;
}

void Button__report(Button *self) {
    if (self->mode == STICKY)
    {
        self->handle_sticky(self);
    }
    else
    {
        // Process latest input
        FsmEvent evt;
        evt.is_pressed = Button__is_pressed(self);
        evt.now = time_us_64();
        fsm__handle_event(&self->fsm, &evt);
    }
}

void Button__handle_sticky(Button *self) {
    bool pressed = self->is_pressed(self);
    if(pressed && !self->state_primary) {
        self->state_primary = true;
        hid_press_multiple(self->actions);
        hid_press_multiple(self->actions_secondary);
        return;
    }
    if((!pressed) && self->state_primary) {
        self->state_primary = false;
        hid_release_multiple(self->actions_secondary);
        return;
    }
}

void Button__reset(Button *self) {
    self->fsm.current_state = BTN_NO_PRESS;
    self->fsm.activation_state = ACT_REST;
    self->press_timestamp = 0;
}

// Init.
Button Button_ (
    uint8_t pin,
    ButtonMode mode,
    Actions actions,
    Actions actions_secondary,
    Actions actions_terciary
) {
    if (pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }
    Button button;
    memcpy(button.actions, actions, 4);
    memcpy(button.actions_secondary, actions_secondary, 4);
    button.is_pressed = Button__is_pressed;
    button.report = Button__report;
    button.reset = Button__reset;
    button.handle_sticky = Button__handle_sticky;
    button.pin = pin;
    button.mode = mode;
    button.state_primary = false;
    button.virtual_press = false;
    button.press_timestamp = 0;
    button.fsm = make_fsm();
    button.fsm.long_hold = (mode & LONG) != 0;

    // Translation layer from old to new
    bool immediate = (mode & IMMEDIATE) != 0;
    bool hold = (mode & HOLD) != 0;
    bool dbl = (mode & DOUBLE) != 0;
    if (mode == NORMAL)
    {
        for(int i = 0 ; i < 4 ; ++i)
        {
            addMapping(&button.fsm.press_actions, actions[i], MOD_START, MOD_PRESS);
        }
    }
    else if(hold)
    {
        for(int i = 0 ; i < 4 ; ++i)
        {
            addMapping(&button.fsm.press_actions, actions[i], MOD_START, immediate ? MOD_PRESS : MOD_TAP);
            addMapping(&button.fsm.press_actions, actions_secondary[i], MOD_START, MOD_HOLD);
            if (dbl)
            {
                addMapping(&button.fsm.double_press_actions, actions_terciary[i], MOD_START, MOD_PRESS);
            }
        }
    }
    else if(dbl /*&& !hold is logically implicit*/)
    {
        for(int i = 0 ; i < 4 ; ++i)
        {
            addMapping(&button.fsm.press_actions, actions[i], MOD_START, immediate ? MOD_PRESS : MOD_TAP);
            addMapping(&button.fsm.double_press_actions, actions_secondary[i], MOD_START, MOD_PRESS);
        }
    }
    // STICKY still has old handling
    return button;
}

// Alternative init.
Button Button_from_ctrl(uint8_t pin, CtrlSection section) {
    return Button_(
        pin,
        section.button.mode,
        section.button.actions,
        section.button.actions_secondary,
        section.button.actions_terciary
    );
}
