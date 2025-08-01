// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once
#include <stdbool.h>
#include <pico/stdlib.h>
#include "ctrl.h"
#include "common.h"
#include "mapping.h"
#include "fsm.h"

typedef enum _ButtonMode {
    NORMAL = 1,
    HOLD = 2,
    DOUBLE = 4,
    IMMEDIATE = 8,
    LONG = 16,
    STICKY = 32,
} ButtonMode;

typedef struct Button_struct Button;
struct Button_struct {
    // Service interface
    bool (*is_pressed) (Button *self);
    void (*report) (Button *self);
    void (*reset) (Button *self);
    void (*handle_sticky) (Button *self);
    uint8_t pin;
    ButtonMode mode;
    Actions actions;
    Actions actions_secondary;
    bool state_primary;
    bool virtual_press;
    uint64_t press_timestamp;
    Fsm fsm;
};

Button Button_ (
    uint8_t pin,
    ButtonMode mode,
    Actions actions,
    Actions actions_secondary,
    Actions actions_terciary
);

Button Button_from_ctrl(
    uint8_t pin,
    CtrlSection section
);
