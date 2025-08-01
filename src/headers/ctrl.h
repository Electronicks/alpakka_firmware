// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once
#include "common.h"

#define CTRL_MSG_SIZE 64
#define CTRL_NON_PAYLOAD_SIZE 4
#define CTRL_MAX_PAYLOAD_SIZE (CTRL_MSG_SIZE - CTRL_NON_PAYLOAD_SIZE)

typedef enum _Ctrl_protocol_flags {
    CTRL_FLAG_NONE = 1,
    CTRL_FLAG_WIRELESS,
} Ctrl_protocol_flags;

typedef enum _Ctrl_device {
    ALPAKKA = 1,
    KAPYBARA,
} Ctrl_device;

typedef enum Ctrl_msg_type_enum {
    LOG = 1,
    PROC,
    CONFIG_GET,
    CONFIG_SET,
    CONFIG_SHARE,
    SECTION_GET,
    SECTION_SET,
    SECTION_SHARE,
    STATUS_GET,
    STATUS_SET,
    STATUS_SHARE,
    PROFILE_OVERWRITE,
} Ctrl_msg_type;

typedef enum Ctrl_cfg_type_enum {
    PROTOCOL = 1,
    SENS_TOUCH,
    SENS_MOUSE,
    DEADZONE,
    LOG_MASK,
    LONG_CALIBRATION,
    SWAP_GYROS,
    TOUCH_INVERT_POLARITY,
    GYRO_USER_OFFSET,
    THUMBSTICK_SMOOTH_SAMPLES,
} Ctrl_cfg_type;

typedef enum CtrlSectionType_enum {
    // Unsorted indexes to keep backwards compatibility.
    SECTION_META = 1,
    SECTION_A,
    SECTION_B,
    SECTION_X,
    SECTION_Y,
    SECTION_DPAD_LEFT,
    SECTION_DPAD_RIGHT,
    SECTION_DPAD_UP,
    SECTION_DPAD_DOWN,
    SECTION_SELECT_1,
    SECTION_START_1,
    SECTION_SELECT_2,
    SECTION_START_2,
    SECTION_L1,
    SECTION_R1,
    SECTION_L2,
    SECTION_R2,
    SECTION_L4,
    SECTION_R4,
    SECTION_ROTARY_UP = 29,
    SECTION_ROTARY_DOWN,
    SECTION_LSTICK_SETTINGS = 31,
    SECTION_LSTICK_LEFT,
    SECTION_LSTICK_RIGHT,
    SECTION_LSTICK_UP,
    SECTION_LSTICK_DOWN,
    SECTION_LSTICK_UL = 55,
    SECTION_LSTICK_UR,
    SECTION_LSTICK_DL,
    SECTION_LSTICK_DR,
    SECTION_LSTICK_PUSH = 36,
    SECTION_LSTICK_INNER,
    SECTION_LSTICK_OUTER,
    SECTION_RSTICK_SETTINGS = 59,
    SECTION_RSTICK_LEFT = 20,
    SECTION_RSTICK_RIGHT,
    SECTION_RSTICK_UP,
    SECTION_RSTICK_DOWN,
    SECTION_RSTICK_UL,
    SECTION_RSTICK_UR,
    SECTION_RSTICK_DL,
    SECTION_RSTICK_DR,
    SECTION_RSTICK_PUSH,
    SECTION_RSTICK_INNER = 60,
    SECTION_RSTICK_OUTER,
    SECTION_GLYPHS_0 = 39,
    SECTION_GLYPHS_1,
    SECTION_GLYPHS_2,
    SECTION_GLYPHS_3,
    SECTION_DAISY_0,
    SECTION_DAISY_1,
    SECTION_DAISY_2,
    SECTION_DAISY_3,
    SECTION_GYRO_SETTINGS,
    SECTION_GYRO_X,
    SECTION_GYRO_Y,
    SECTION_GYRO_Z,
    SECTION_MACRO_1,
    SECTION_MACRO_2,
    SECTION_MACRO_3,
    SECTION_MACRO_4,
} CtrlSectionType;

typedef struct _Ctrl {
    Ctrl_protocol_flags protocol_flags;
    Ctrl_device device_id;
    Ctrl_msg_type message_type;
    uint8_t len;
    uint8_t payload[CTRL_MAX_PAYLOAD_SIZE];
} Ctrl;

typedef struct __packed _CtrlProfileMeta {
    // Must be packed (58 bytes).
    char name[24];
    uint8_t control_byte;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t version_patch;
    uint8_t _padding[30];
} CtrlProfileMeta;

typedef struct __packed _CtrlButton {
    // Must be packed (58 bytes).
    uint8_t mode;
    uint8_t actions[4];
    uint8_t actions_secondary[4];
    uint8_t actions_terciary[4];
    uint8_t hint[14];
    uint8_t hint_secondary[14];
    uint8_t hint_terciary[14];
    uint8_t _padding[3];
} CtrlButton;

typedef struct __packed _CtrlRotary {
    // Must be packed (58 bytes).
    uint8_t actions_0[4];
    uint8_t actions_1[4];
    uint8_t actions_2[4];
    uint8_t actions_3[4];
    uint8_t actions_4[4];
    uint8_t hint_0[14];
    uint8_t hint_1[6];
    uint8_t hint_2[6];
    uint8_t hint_3[6];
    uint8_t hint_4[6];
} CtrlRotary;

typedef struct __packed _CtrlThumbstick {
    // Must be packed (58 bytes).
    uint8_t mode;
    uint8_t distance_mode;
    uint8_t deadzone;
    uint8_t overlap;
    uint8_t deadzone_override;
    uint8_t antideadzone;
    uint8_t saturation;
    uint8_t _padding[51];
} CtrlThumbstick;

typedef struct __packed _CtrlGlyph {
    uint8_t actions[4];
    uint8_t glyph;
} CtrlGlyph;

typedef struct __packed _CtrlGlyphs {
    // Must be packed (58 bytes).
    CtrlGlyph glyphs[11];
    uint8_t padding[3];
} CtrlGlyphs;

typedef struct __packed _CtrlDaisyGroup {
    uint8_t actions_a[4];
    uint8_t actions_b[4];
    uint8_t actions_x[4];
    uint8_t actions_y[4];
} CtrlDaisyGroup;

typedef struct __packed _CtrlDaisy {
    // Must be packed (58 bytes).
    CtrlDaisyGroup groups[2];
    uint8_t _padding[26];
} CtrlDaisy;

typedef struct __packed _CtrlGyro {
    // Must be packed (58 bytes).
    uint8_t mode;
    uint8_t engage;
    uint8_t _padding[56];
} CtrlGyro;

typedef struct __packed _CtrlGyroAxis {
    // Must be packed (58 bytes).
    uint8_t actions_neg[4];
    uint8_t actions_pos[4];
    uint8_t angle_min;
    uint8_t angle_max;
    uint8_t hint_neg[14];
    uint8_t hint_pos[14];
    uint8_t _padding[20];
} CtrlGyroAxis;

typedef struct __packed _CtrlMacro {
    // Must be packed (58 bytes).
    uint8_t macro[2][28];
    uint8_t _padding[2];
} CtrlMacro;

typedef union _CtrlSection {
    CtrlProfileMeta meta;
    CtrlButton button;
    CtrlRotary rotary;
    CtrlThumbstick thumbstick;
    CtrlGlyphs glyphs;
    CtrlDaisy daisy;
    CtrlGyro gyro;
    CtrlGyroAxis gyro_axis;
    CtrlMacro macro;
} CtrlSection;

typedef struct _CtrlProfile {
    CtrlSection sections[64];
} CtrlProfile;

Ctrl ctrl_empty();
Ctrl ctrl_log(uint8_t* offset_ptr, uint8_t len);
Ctrl ctrl_status_share();
Ctrl ctrl_config_share(uint8_t index);
Ctrl ctrl_section_share(uint8_t profile_index, uint8_t section_index);

void ctrl_config_set(Ctrl_cfg_type key, uint8_t preset, uint8_t values[5]);
