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
#include <project.h>
#include <math.h>
#include <helpers.h>

void drive_row()
{
    Pin_1_Write(1);
    Pin_2_Write(1);
    Pin_3_Write(1);
    Pin_4_Write(1);
    Pin_5_Write(0);
    Pin_6_Write(0);
    Pin_7_Write(0);
    Pin_8_Write(0);
    
    Pin_9_Write(1);
    Pin_10_Write(1);
    Pin_11_Write(1);
    Pin_12_Write(1);
    Pin_13_Write(0);
    Pin_14_Write(0);
    Pin_15_Write(0);
    Pin_16_Write(0);
    
    Pin_17_Write(1);
    Pin_18_Write(1);
    Pin_19_Write(1);
    Pin_20_Write(1);
    Pin_21_Write(0);
    Pin_22_Write(0);
    Pin_23_Write(0);
}

void drive_col()
{
    Pin_1_Write(0);
    Pin_2_Write(0);
    Pin_3_Write(0);
    Pin_4_Write(0);
    Pin_5_Write(1);
    Pin_6_Write(1);
    Pin_7_Write(1);
    Pin_8_Write(1);
    
    Pin_9_Write(0);
    Pin_10_Write(0);
    Pin_11_Write(0);
    Pin_12_Write(0);
    Pin_13_Write(1);
    Pin_14_Write(1);
    Pin_15_Write(1);
    Pin_16_Write(1);
    
    Pin_17_Write(0);
    Pin_18_Write(0);
    Pin_19_Write(0);
    Pin_20_Write(0);
    Pin_21_Write(1);
    Pin_22_Write(1);
    Pin_23_Write(1);
}

int read_row()
{
    return (
        (pow(2, 11) * Pin_1_Read()) + 
        (pow(2, 10) * Pin_2_Read()) + 
        (pow(2, 9) * Pin_3_Read()) + 
        (pow(2, 8) * Pin_4_Read()) + 
        
        (pow(2, 7) * Pin_9_Read()) + 
        (pow(2, 6) * Pin_10_Read()) + 
        (pow(2, 5) * Pin_11_Read()) + 
        (pow(2, 4) * Pin_12_Read()) + 
        
        (pow(2, 3) * Pin_17_Read()) + 
        (pow(2, 2) * Pin_18_Read()) + 
        (pow(2, 1) * Pin_19_Read()) + 
        (pow(2, 0) * Pin_20_Read())
    );
}

int read_col()
{
    return (
        (pow(2, 10) * Pin_5_Read()) + 
        (pow(2, 9) * Pin_6_Read()) + 
        (pow(2, 8) * Pin_7_Read()) + 
        (pow(2, 7) * Pin_8_Read()) + 
        
        (pow(2, 6) * Pin_13_Read()) + 
        (pow(2, 5) * Pin_14_Read()) + 
        (pow(2, 4) * Pin_15_Read()) + 
        (pow(2, 3) * Pin_16_Read()) + 
        
        (pow(2, 2) * Pin_21_Read()) + 
        (pow(2, 1) * Pin_22_Read()) + 
        (pow(2, 0) * Pin_23_Read())
    );
}

int data_available = 0;
int last_data_available = 0;
unsigned long CAPS_LOCK_KEY = 0b01000110;
unsigned long SHIFT_KEY = 0b01101010;
int caps_lock_on = 0;
int caps_lock_last_pressed = 0;

int bits_set(int num)
{
    int mask = 1;
    int num_bits = 14;
    int result = 0;
    int i;
    for (i = 0; i < num_bits; i++)
    {
        result += (int) ((num & mask) > 0);
        mask = mask << 1;
    }
    return result;
}

int SHIFT_ROW_READ = 256;   // 8th row
int SHIFT_COL_READ = 1024;  // 10th column

int shift_pressed_with_key(long row_read, long col_read)
{
    // Check if shift key pressed with single other key
    int row_bits, col_bits;
    
    row_bits = bits_set(row_read);
    col_bits = bits_set(col_read);
    if (row_bits > 2 || col_bits > 2) return 0;
    if (row_read == SHIFT_ROW_READ && col_read == SHIFT_COL_READ) return 0;
    // Check if shift key in row_bits    
    int row_sub_shift = (row_read & (~SHIFT_ROW_READ));
    int in_row_bits = (row_sub_shift != row_read);
    // Check if shift key in col_bits
    int col_sub_shift = (col_read & (~SHIFT_COL_READ));
    int in_col_bits = (col_sub_shift != col_read);
    
    return (in_row_bits && in_col_bits);
}

int shift_with_char = 0;
int shift_was_pressed = 0;
int shift_current_char = 0;

unsigned long keypad_scan(void)
{
    long row_num, row_read = 0;
    long col_num, col_read = 0;
    int only_one_key;
    unsigned long result;
    
    /* Drive rows, read columns */
    drive_row();
    CyDelay(10);    // Wait for value to stabilize
    col_read = read_col();
    col_num = logb2(col_read);
    
    /* Drive columns, read rows*/
    drive_col();
    CyDelay(10);    // Wait for value to stabilize
    row_read = read_row();
    row_num = logb2(row_read);
    
    /* combine results */
    result = 12 * row_num + col_num;
    shift_with_char = shift_pressed_with_key(row_read, col_read);
    shift_current_char = shift_was_pressed && shift_with_char;
    only_one_key = (bits_set(row_read) == 1) && (bits_set(col_read) == 1);
    
    if (result == CAPS_LOCK_KEY && !caps_lock_last_pressed && only_one_key) {
        caps_lock_on = !caps_lock_on;
        Caps_LED_Write(caps_lock_on);
        data_available = 0;   
    } else if (shift_current_char) {
        // Get key pressed without shift key
        row_num = logb2(row_read & (~SHIFT_ROW_READ));
        col_num = logb2(col_read & (~SHIFT_COL_READ));
        result = 12 * row_num + col_num;
    } else if (result != CAPS_LOCK_KEY) {
        data_available = (col_num != -1 && row_num != -1);        
    }
    
    if (result == SHIFT_KEY || shift_current_char) {
        shift_was_pressed = 1;
        data_available = shift_current_char && (col_num != -1 && row_num != -1);
    } else shift_was_pressed = 0;
    
    DA_Pin_Write(data_available);
    Shift_LED_Write(shift_was_pressed || shift_current_char);
    caps_lock_last_pressed = (result == CAPS_LOCK_KEY) && only_one_key;
    result = data_available ? result : 0;    
    return result;
}

unsigned long key_idx = 0;
unsigned long out = 0;
unsigned long mask = 1;
int lower_case_letters = 1;
unsigned long last_out;

int main()
{
    CyGlobalIntEnable; /* Enable global interrupts. */

    for(;;)
    {
        mask = 1;
        data_available = 0;
        key_idx = keypad_scan();
        out = char_lookup(key_idx);
        lower_case_letters = !caps_lock_on;
        
        if (shift_current_char) lower_case_letters = !lower_case_letters;
        
        // Make lowercase if caps lock is off or shift not held
        if (out >= 65 && out <= 90 && lower_case_letters)
        {
            out = out | 0b100000;
        }
        
        // Write value to output pins and latch on rising edge of data available
        // or change of value
        if (data_available && (!last_data_available || out != last_out))        
        {
            Pin_25_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_26_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_27_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_28_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_29_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_30_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_31_Write((out & mask) > 0);
            mask = mask << 1;
            Pin_32_Write((out & mask) > 0);
            mask = mask << 1;
        }
        last_data_available = data_available;
        last_out = out;
    }
}

/* [] END OF FILE */
