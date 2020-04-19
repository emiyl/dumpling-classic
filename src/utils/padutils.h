/*
    padutils.h
    utils to support all wii u controller
*/

#ifndef _PAD_UTILS_H_
#define _PAD_UTILS_H_

#include <stdint.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/padscore_functions.h"
#include "dynamic_libs/vpad_functions.h"

uint32_t buttons_hold; //Held buttons
uint32_t buttons_pressed; //Pressed buttons
uint32_t buttons_released; //Released buttons
KPADData kpad[4]; // kpad data for all 4 controllers

void updateKpad(void);
void updatePressedButtons();
void updateHeldButtons();
void updateReleasedButtons();
void updatePad(void);
bool vpadstickPos(u8 stick, f32 value);
bool wpadsticknunchuckPos(int i, u8 stick, f32 value);
bool wpadstickclassicPos(int i, u8 stick, f32 value);
bool wpadstickproPos(int i, u8 stick, f32 value);
bool wpadstickPos(int i, u8 stick, f32 value);
bool stickPos(u8 stick, f32 value);
int vpadtowpad(int button);
int vpadtowpadclassic(int button);
int vpadtowpadpro(int button);
int kpadpressed(int i, int button);
int kpadheld(int i, int button);
int kpadreleased(int i, int button);
int isPressed(int button);
int isHeld(int button);
int isReleased(int button);
void padInit(void);

#endif