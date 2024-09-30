/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#pragma once
#include <device.h>
#include <math.h>

int last_millis = 0;
int millis_past = 0;
int light_timer = 0;
uint8_t loading_counter = 0;

int millis()
{
    int current_clock = Millis_Pin_Read();
    millis_past += (current_clock && !last_millis);
    last_millis = current_clock;
    return millis_past;
}

void light_print(int data)
{
    int mask = 1;
    Pin_9_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_10_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_11_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_12_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_13_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_14_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_15_Write((data & mask) > 0);
    mask = mask << 1;
    Pin_16_Write((data & mask) > 0);
    mask = mask << 1;
}

void show_progress(int current, int final)
{
    int blink = (int) (current * 8 / (final - 1));
    //int blink = (int) (current * 8 / (final - 1)) + (current % 4) > 1 ? 0 : 1;
    //blink = (blink > 8) ? blink : 8;
    int bar_val = pow(2, blink) - 1;
    light_print(bar_val);
}

void show_loading(uint8 rate)
{
    if (millis() - light_timer >= rate) {
        light_print((loading_counter % 2) ? 85 : 170);
        loading_counter = (loading_counter + 1) % 2;
        light_timer = millis();
    }
}

/* [] END OF FILE */
