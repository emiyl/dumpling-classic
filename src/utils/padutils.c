#include "padutils.h"
#include <string.h>

int vpadError = -1;
VPADData vpad;

void updateKpad(void)
{
    for (int i = 0; i < 4; i++)
    {
        u32 controllerType;
        // check if the controller is connected
        if (WPADProbe(i, &controllerType) != 0)
            continue;

        KPADRead(i, &(kpad[i]), 1);
    }
}

void updatePressedButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_pressed = vpad.btns_d;
}

void updateHeldButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_hold = vpad.btns_h;
}

void updateReleasedButtons() {
	VPADRead(0, &vpad, 1, &vpadError);
	buttons_released = vpad.btns_r;
}

void updatePad(void)
{
    updateKpad();
    updatePressedButtons();
    updateHeldButtons();
    updateReleasedButtons();
}

bool vpadstickPos(u8 stick, f32 value)
{
    switch(stick) {
        case 0 :
            return (value > 0) ? (vpad.lstick.x > value): (vpad.lstick.x < value);
        case 1 :
            return (value > 0) ? (vpad.lstick.y > value): (vpad.lstick.y < value);
        case 2 :
            return (value > 0) ? (vpad.rstick.x > value): (vpad.rstick.x < value);
        case 3 :
            return (value > 0) ? (vpad.rstick.y > value): (vpad.rstick.y < value);
        case 4 :
            return ((vpad.lstick.x > value) || (vpad.lstick.x < -value)) || \
                   ((vpad.lstick.y > value) || (vpad.lstick.y < -value)) || \
                   ((vpad.rstick.x > value) || (vpad.rstick.x < -value)) || \
                   ((vpad.rstick.y > value) || (vpad.rstick.y < -value));

        default :
            return 0;
    }
}

bool wpadsticknunchuckPos(int i, u8 stick, f32 value)
{
    switch(stick) {
        case 0 :
            return (value > 0) ? (kpad[i].nunchuck.stick_x > value): (kpad[i].nunchuck.stick_x < value);
        case 1 :
            return (value > 0) ? (kpad[i].nunchuck.stick_y > value): (kpad[i].nunchuck.stick_y < value);
        case 2 :
            return 0; // nunchuck only has one stick
        case 3 :
            return 0; // nunchuck only has one stick
        case 4 :
            return ((kpad[i].nunchuck.stick_x > value) || (kpad[i].nunchuck.stick_x < -value)) || \
                   ((kpad[i].nunchuck.stick_y > value) || (kpad[i].nunchuck.stick_y < -value));

        default :
            return 0;
    }
}

bool wpadstickclassicPos(int i, u8 stick, f32 value)
{
    switch(stick) {
        case 0 :
            return (value > 0) ? (kpad[i].classic.lstick_x > value): (kpad[i].classic.lstick_x < value);
        case 1 :
            return (value > 0) ? (kpad[i].classic.lstick_y > value): (kpad[i].classic.lstick_y < value);
        case 2 :
            return (value > 0) ? (kpad[i].classic.rstick_x > value): (kpad[i].classic.rstick_x < value);
        case 3 :
            return (value > 0) ? (kpad[i].classic.rstick_y > value): (kpad[i].classic.rstick_y < value);
        case 4 :
            return ((kpad[i].classic.lstick_x > value) || (kpad[i].classic.lstick_x < -value)) || \
                   ((kpad[i].classic.lstick_y > value) || (kpad[i].classic.lstick_y < -value)) || \
                   ((kpad[i].classic.rstick_x > value) || (kpad[i].classic.rstick_x < -value)) || \
                   ((kpad[i].classic.rstick_y > value) || (kpad[i].classic.rstick_y < -value));

        default :
            return 0;
    }
}

bool wpadstickproPos(int i, u8 stick, f32 value)
{
    switch(stick) {
        case 0 :
            return (value > 0) ? (kpad[i].pro.lstick_x > value): (kpad[i].pro.lstick_x < value);
        case 1 :
            return (value > 0) ? (kpad[i].pro.lstick_y > value): (kpad[i].pro.lstick_y < value);
        case 2 :
            return (value > 0) ? (kpad[i].pro.rstick_x > value): (kpad[i].pro.rstick_x < value);
        case 3 :
            return (value > 0) ? (kpad[i].pro.rstick_y > value): (kpad[i].pro.rstick_y < value);
        case 4 :
            return ((kpad[i].pro.lstick_x > value) || (kpad[i].pro.lstick_x < -value)) || \
                   ((kpad[i].pro.lstick_y > value) || (kpad[i].pro.lstick_y < -value)) || \
                   ((kpad[i].pro.rstick_x > value) || (kpad[i].pro.rstick_x < -value)) || \
                   ((kpad[i].pro.rstick_y > value) || (kpad[i].pro.rstick_y < -value));

        default :
            return 0;
    }
}

bool wpadstickPos(int i, u8 stick, f32 value)
{
    u32 controllerType;
    // check if the controller is connected
    if (WPADProbe(i, &controllerType) != 0)
        return 0;

    switch (controllerType)
    {
    case WPAD_EXT_NUNCHUK:
        return wpadsticknunchuckPos(i, stick, value);
    case WPAD_EXT_CLASSIC:
        return wpadstickclassicPos(i, stick, value);
    case WPAD_EXT_PRO_CONTROLLER:
        return wpadstickproPos(i, stick, value);
    }

    return 0;
}

bool stickPos(u8 stick, f32 value) {
    return vpadstickPos(stick, value) || wpadstickPos(0, stick, value) || wpadstickPos(1, stick, value) || wpadstickPos(2, stick, value) || wpadstickPos(3, stick, value);
}

// converts a vpad mapping to a wpad mapping
// returns 0 if there is no mapping for it
int vpadtowpad(int button)
{
    switch (button)
    {
    case VPAD_BUTTON_A: return WPAD_BUTTON_A;
    case VPAD_BUTTON_B: return WPAD_BUTTON_B;
    case VPAD_BUTTON_X: return WPAD_BUTTON_1;
    case VPAD_BUTTON_Y: return WPAD_BUTTON_2;
    case VPAD_BUTTON_LEFT: return WPAD_BUTTON_LEFT;
    case VPAD_BUTTON_RIGHT: return WPAD_BUTTON_RIGHT;
    case VPAD_BUTTON_UP: return WPAD_BUTTON_UP;
    case VPAD_BUTTON_DOWN: return WPAD_BUTTON_DOWN;
    case VPAD_BUTTON_ZL: return WPAD_BUTTON_MINUS;
    case VPAD_BUTTON_ZR: return 0;
    case VPAD_BUTTON_L: return 0;
    case VPAD_BUTTON_R: return 0;
    case VPAD_BUTTON_PLUS: return WPAD_BUTTON_PLUS;
    case VPAD_BUTTON_MINUS: return WPAD_BUTTON_MINUS;
    case VPAD_BUTTON_HOME: return WPAD_BUTTON_HOME;
    case VPAD_BUTTON_SYNC: return 0;
    case VPAD_BUTTON_STICK_R: return 0;
    case VPAD_BUTTON_STICK_L: return 0;
    case VPAD_BUTTON_TV: return 0;
    }

    return 0;
}

// converts a vpad mapping to a wpad classic mapping
// returns 0 if there is no mapping for it
int vpadtowpadclassic(int button)
{
    switch (button)
    {
    case VPAD_BUTTON_A: return WPAD_CLASSIC_BUTTON_A;
    case VPAD_BUTTON_B: return WPAD_CLASSIC_BUTTON_B;
    case VPAD_BUTTON_X: return WPAD_CLASSIC_BUTTON_X;
    case VPAD_BUTTON_Y: return WPAD_CLASSIC_BUTTON_Y;
    case VPAD_BUTTON_LEFT: return WPAD_CLASSIC_BUTTON_LEFT;
    case VPAD_BUTTON_RIGHT: return WPAD_CLASSIC_BUTTON_RIGHT;
    case VPAD_BUTTON_UP: return WPAD_CLASSIC_BUTTON_UP;
    case VPAD_BUTTON_DOWN: return WPAD_CLASSIC_BUTTON_DOWN;
    case VPAD_BUTTON_ZL: return WPAD_CLASSIC_BUTTON_ZL;
    case VPAD_BUTTON_ZR: return WPAD_CLASSIC_BUTTON_ZR;
    case VPAD_BUTTON_L: return WPAD_CLASSIC_BUTTON_L;
    case VPAD_BUTTON_R: return WPAD_CLASSIC_BUTTON_R;
    case VPAD_BUTTON_PLUS: return WPAD_CLASSIC_BUTTON_PLUS;
    case VPAD_BUTTON_MINUS: return WPAD_CLASSIC_BUTTON_MINUS;
    case VPAD_BUTTON_HOME: return WPAD_CLASSIC_BUTTON_HOME;
    case VPAD_BUTTON_SYNC: return 0;
    case VPAD_BUTTON_STICK_R: return 0;
    case VPAD_BUTTON_STICK_L: return 0;
    case VPAD_BUTTON_TV: return 0;
    }

    return 0;
}

// converts a vpad mapping to a wpad pro mapping
// returns 0 if there is no mapping for it
int vpadtowpadpro(int button)
{
    switch (button)
    {
    case VPAD_BUTTON_A: return WPAD_PRO_BUTTON_A;
    case VPAD_BUTTON_B: return WPAD_PRO_BUTTON_B;
    case VPAD_BUTTON_X: return WPAD_PRO_BUTTON_X;
    case VPAD_BUTTON_Y: return WPAD_PRO_BUTTON_Y;
    case VPAD_BUTTON_LEFT: return WPAD_PRO_BUTTON_LEFT;
    case VPAD_BUTTON_RIGHT: return WPAD_PRO_BUTTON_RIGHT;
    case VPAD_BUTTON_UP: return WPAD_PRO_BUTTON_UP;
    case VPAD_BUTTON_DOWN: return WPAD_PRO_BUTTON_DOWN;
    case VPAD_BUTTON_ZL: return WPAD_PRO_TRIGGER_ZL;
    case VPAD_BUTTON_ZR: return WPAD_PRO_TRIGGER_ZR;
    case VPAD_BUTTON_L: return WPAD_PRO_TRIGGER_L;
    case VPAD_BUTTON_R: return WPAD_PRO_TRIGGER_R;
    case VPAD_BUTTON_PLUS: return WPAD_PRO_BUTTON_PLUS;
    case VPAD_BUTTON_MINUS: return WPAD_PRO_BUTTON_MINUS;
    case VPAD_BUTTON_HOME: return WPAD_PRO_BUTTON_HOME;
    case VPAD_BUTTON_SYNC: return 0;
    case VPAD_BUTTON_STICK_R: return WPAD_PRO_BUTTON_STICK_R;
    case VPAD_BUTTON_STICK_L: return WPAD_PRO_BUTTON_STICK_L;
    case VPAD_BUTTON_TV: return 0;
    }

    return 0;
}

int kpadpressed(int i, int button)
{
    u32 controllerType;
    // check if the controller is connected
    if (WPADProbe(i, &controllerType) != 0)
        return 0;

    switch (controllerType)
    {
    case WPAD_EXT_CORE:
        return kpad[i].btns_d & vpadtowpad(button);
    case WPAD_EXT_CLASSIC:
        return kpad[i].classic.btns_d & vpadtowpadclassic(button);
    case WPAD_EXT_PRO_CONTROLLER:
        return kpad[i].pro.btns_d & vpadtowpadpro(button);
    }

    return 0;
}

int kpadheld(int i, int button)
{
    u32 controllerType;
    // check if the controller is connected
    if (WPADProbe(i, &controllerType) != 0)
        return 0;

    switch (controllerType)
    {
    case WPAD_EXT_CORE:
        return kpad[i].btns_h & vpadtowpad(button);
    case WPAD_EXT_CLASSIC:
        return kpad[i].classic.btns_h & vpadtowpadclassic(button);
    case WPAD_EXT_PRO_CONTROLLER:
        return kpad[i].pro.btns_h & vpadtowpadpro(button);
    }

    return 0;
}

int kpadreleased(int i, int button)
{
    u32 controllerType;
    // check if the controller is connected
    if (WPADProbe(i, &controllerType) != 0)
        return 0;

    switch (controllerType)
    {
    case WPAD_EXT_CORE:
        return kpad[i].btns_r & vpadtowpad(button);
    case WPAD_EXT_CLASSIC:
        return kpad[i].classic.btns_r & vpadtowpadclassic(button);
    case WPAD_EXT_PRO_CONTROLLER:
        return kpad[i].pro.btns_r & vpadtowpadpro(button);
    }

    return 0;
}

int isPressed(int button) {
	return (buttons_pressed&button) || kpadpressed(0, button) || kpadpressed(1, button) || kpadpressed(2, button) || kpadpressed(3, button);
}

int isHeld(int button) {
	return (buttons_hold&button) || kpadheld(0, button) || kpadheld(1, button) || kpadheld(2, button) || kpadheld(3, button);
}

int isReleased(int button) {
	return (buttons_released&button) || kpadreleased(0, button) || kpadreleased(1, button) || kpadreleased(2, button) || kpadreleased(3, button);
}

void padInit(void)
{
    InitPadScoreFunctionPointers();
    InitVPadFunctionPointers();

    VPADInit();
    KPADInit();
    memset(kpad, 0, sizeof(kpad)); // Init to 0
}