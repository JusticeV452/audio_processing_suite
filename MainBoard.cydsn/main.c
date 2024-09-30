/* ========================================
 *
 * Copyright MIT 6.115, 2013
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF MIT 6.115.
 *
 * This file is necessary for your project to build.
 * Please do not delete it.
 *
 * ========================================
*/

#include <stdlib.h>
#include <device.h>
#include <string.h>

#include <signal_proc.h>

// Constants
#define MAX_QUEUE_LEN 1000
#define MAX_SAMPLES 10000
#define BUFFER_LEN 512
#define PI 3.14159265
#define SAMPLE_RATE 11025
#define MAX_FILENAME_LEN 12
uint16_t SAMPLE_PERIOD = 1000000 / SAMPLE_RATE;   // In micros

// Special keys
#define UP '+' // 128
#define DOWN '-' // 129
#define LEFT 130
#define RIGHT 131
#define ENTER 13
#define BACKSPACE 8
#define ESC 27

// Interrupts
void SAMPLE_INT();

// Globals
int last_freq = 0;
int key_data = 0;
int pressed = 0;
int last_key_data = 0;
SDFile* pMainFile;
float freq_mult = 1;
int sample_ready;
uint8_t serial_data_available;

// States
int output_mode = 0;
enum states {
    MAIN_MENU,
    RECORD_AUDIO,
    LIVE_PLAYBACK, 
    PROCESS_SELECT,
    FILE_SELECT,
    PROCESS_FILES,
    FILE_PLAYBACK
};
int state = MAIN_MENU;
int last_state = MAIN_MENU;
int next_state = MAIN_MENU;

// Sub-state variables
int main_menu_state = 0;
int process_state = 0;
char process_option = 0;
int num_files_to_enter = 0;
char file_names[2][MAX_FILENAME_LEN + 1];
int file_cursor_pos = 0;
int entered_files = 0;

void set_frequency(int freq)
{
    if (freq && (freq == last_freq)) return;
    int period = freq ? (65536 / (0.005461 * freq)) : 65536 + 1;
    PWM_1_WritePeriod(period);
    PWM_1_WriteCompare(period / 2);
    last_freq = freq;
}

void set_output_mode(int mode)
{
    set_frequency(0);
    if (mode == 0) {
        sample_int_Stop();
        Sample_Player_Stop();
    } else if (mode == 1) {
        Sample_Player_Start();
        Sample_Player_SetRange(Sample_Player_RANGE_4V);
        sample_int_StartEx(SAMPLE_INT);
    }
    Audio_Mux_Select((mode >= 0 && mode <= 3) ? mode : 0);
    output_mode = mode;
}

int note_map(char key_data)
{
    int freq = key_data;
    int is_capital = (key_data >= 65 && key_data <= 90);
    if (is_capital) key_data = (key_data | 32); // Set to lowercase if letter
    switch(key_data)
    {
        case 'w': freq = 247; break;    // cb
        case 'e': freq = 262; break;    // c
        case 'r': freq = 277; break;    // c#
        case 't': freq = 294; break;    // d
        case 'y': freq = 311; break;    // eb
        case 'u': freq = 330; break;    // e
        case 'i': freq = 349; break;    // f
        case 's': freq = 370; break;    // f#
        case 'd': freq = 392; break;    // g
        case 'f': freq = 415; break;    // ab
        case 'g': freq = 440; break;    // a
        case 'h': freq = 466; break;    // bb
        case 'j': freq = 494; break;    // b
        case 'k': freq = 523; break;    // b#
        default: freq = 0;
    }
    if (is_capital) freq *= 2;          // Set to next octave if capital
    return freq * freq_mult;
}

int read_keyboard()
{
    return (
        (pow(2, 7) * Pin_1_Read()) +         
        (pow(2, 6) * Pin_2_Read()) + 
        (pow(2, 5) * Pin_3_Read()) + 
        (pow(2, 4) * Pin_4_Read()) + 
        (pow(2, 3) * Pin_5_Read()) +         
        (pow(2, 2) * Pin_6_Read()) + 
        (pow(2, 1) * Pin_7_Read()) + 
        (pow(2, 0) * Pin_8_Read())
    );
}

int mod(int n, int m)
{
  return ((n % m) >= 0) ? n % m : m + n % m;
}

void clear_file_names()
{
    memset(file_names[0], 0, MAX_FILENAME_LEN);
    memset(file_names[1], 0, MAX_FILENAME_LEN);
}

void change_state(int new_state)
{
    LCD_ClearDisplay();
    if (new_state == FILE_SELECT || state != FILE_SELECT) {
        clear_file_names();
    }
    if (new_state == MAIN_MENU || RECORD_AUDIO) {
        set_output_mode(0);
    }
    set_frequency(0);
    light_print(0);
    last_state = state;
    state = new_state;
}

void main_menu(int pressed)
{
    LCD_Position(0, 0);
    LCD_PrintString("Main menu:");
    LCD_Position(1, 0);
    switch (main_menu_state)
    {
        case 0: LCD_PrintString("1: Record"); break;
        case 1: LCD_PrintString("2: Playback"); break;
        case 2: LCD_PrintString("3: Processing"); break;
        case 3: LCD_PrintString("4: Files"); break;
    }
    
    switch (pressed)
    {
        case UP: main_menu_state = mod(main_menu_state + 1, 4); 
        LCD_ClearDisplay();return;
        case DOWN: main_menu_state = mod(main_menu_state - 1, 4); 
        LCD_ClearDisplay();return;
        case LEFT: main_menu_state = mod(main_menu_state - 1, 4); 
        LCD_ClearDisplay();return;
        case RIGHT: main_menu_state = mod(main_menu_state + 1, 4); 
        LCD_ClearDisplay();return;
    }
    if (pressed != ENTER) return;
    switch (main_menu_state) {
        case 0: 
            num_files_to_enter = 1;
            change_state(FILE_SELECT);
            next_state = RECORD_AUDIO;
            break;
        case 1:
            num_files_to_enter = 1;
            change_state(FILE_SELECT);
            next_state = FILE_PLAYBACK;
            break;
        case 2: change_state(PROCESS_SELECT); break;
        case 3: change_state(MAIN_MENU); break;
    }
    main_menu_state = 0;
}

void process_select(int pressed)
{
    LCD_Position(0, 0);
    LCD_PrintString("Processing:");
    LCD_Position(1, 0);
    int num_options = 12;
    switch (process_state)
    {
        case 0: LCD_PrintString("0: Echo"); break;
        case 1: LCD_PrintString("1: Reverse"); break;
        case 2: LCD_PrintString("2: Equalize"); break;
        case 3: LCD_PrintString("3: Low Pass"); break;
        case 4: LCD_PrintString("4: High Pass"); break;
        case 5: LCD_PrintString("5: Fade"); break;
        case 6: LCD_PrintString("6: Overlay"); break;
        case 7: LCD_PrintString("7: Multiply"); break;
        case 8: LCD_PrintString("8: Subtract"); break;
        case 9: LCD_PrintString("9: Divide"); break;
        case 10: LCD_PrintString("10: FFT"); break;
        case 11: LCD_PrintString("11: Amplify (x2)"); break;
    }
    
    switch (pressed)
    {
        case UP: process_state = mod(process_state + 1, num_options);
        LCD_ClearDisplay(); break;
        case DOWN: process_state = mod(process_state - 1, num_options);
        LCD_ClearDisplay(); break;
        case LEFT: process_state = mod(process_state - 1, num_options);
        LCD_ClearDisplay(); break;
        case RIGHT: process_state = mod(process_state + 1, num_options);
        LCD_ClearDisplay(); break;
    }
    
    if (pressed == ESC) {
        process_state = 0;
        change_state(MAIN_MENU); 
        return;
    }    
    if (pressed != ENTER) return;   
    
    // Get file names to process and process
    process_option = process_state + 48;    // Convert to char    
    num_files_to_enter = (process_option < '6' || process_option > '9') ? 1 : 2;    
    process_state = 0;  //Reset sub-state
    change_state(FILE_SELECT);
    next_state = PROCESS_FILES;
}

void file_select(int pressed)
{
    LCD_Position(0, 0);
    LCD_PrintString("Enter file name:");
    LCD_Position(1, 0);
    LCD_PrintString(file_names[entered_files]);
    LCD_Position(1, file_cursor_pos);
    if (pressed == BACKSPACE) {
        file_cursor_pos -= 1;        
        file_cursor_pos = (file_cursor_pos >= 0) ? file_cursor_pos : 0;
        file_names[entered_files][file_cursor_pos] = 0;
        LCD_Position(1, file_cursor_pos);
        LCD_PutChar(' ');
        return;
    } else if (pressed == ENTER && file_cursor_pos > 0) {        
        entered_files += 1;
        if (entered_files == num_files_to_enter) {
            entered_files = 0;
            file_cursor_pos = 0;
            change_state(next_state);            
            return;
        }
        LCD_ClearDisplay();
    } else if (pressed == ESC) {
        entered_files = 0;
        file_cursor_pos = 0;
        change_state(last_state); 
        return;
    } else if (file_cursor_pos == MAX_FILENAME_LEN) {
        return;
    }
    if (pressed < ' ' || pressed > '~') return;
    char temp[] = "_";
    temp[0] = pressed;
    strcat(file_names[entered_files], temp);
    file_cursor_pos++;
}

void process_files()
{
    // Uses process_option + file_names
    LCD_Position(0, 0);
    LCD_PrintString("Processing...");
    switch (process_option)
    {
        case '0': echo(file_names[0]); break;
        case '1': reverse(file_names[0]); break;
        case '2': equalize(file_names[0]); break;
        case '3': kfft_filter(file_names[0], 0, -2); break;
        case '4': kfft_filter(file_names[0], -2, -1); break;
        case '5': fade(file_names[0], 1, 1); break;
        case '6': overlay(file_names[0], file_names[1]); break;
        case '7': multi_sig(file_names[0], file_names[1], 2); break;
        case '8': multi_sig(file_names[0], file_names[1], 1); break;
        case '9': multi_sig(file_names[0], file_names[1], 3); break;
        case  58: kfft_filter(file_names[0], 0, -1); break;
        case  59: amplify(file_names[0], 2); break;
    }
    LCD_ClearDisplay();
    LCD_Position(0, 0);
    LCD_PrintString("Processing complete!");
    LCD_Position(1, 0);
    LCD_PrintString("Returning to main menu...");
    CyDelay(500);
    // Clear process settings
    process_option = 0;
    change_state(MAIN_MENU);
}

CY_ISR(SAMPLE_INT)
{
    int sample = 0;
    
    sample = read_byte(pMainFile);
    if (sample >= 0) {        
        Sample_Player_SetValue(sample);
        light_print(sample);
    } else if (sample == -1) {
        close_file(pMainFile, 1, 10);
    }
    if (state == RECORD_AUDIO && (pMainFile->pFSFile) && sample_ready) {
        write_byte(pMainFile, Mic_ADC_GetResult16());
    }
    
    DAC_Pin_ClearInterrupt();
}

CY_ISR(RX_INT)
{
    key_data = UART_ReadRxData();
    pressed = key_data;
    serial_data_available = 1;
}

void main()
{	
    int data_available, last_data_available;
    
    FS_Init();
    PWM_1_Start();
    set_output_mode(0);
    SDFile File = {0, "main", -1, {}, 0, -1};
    SDFile* pFile = &File;
    pMainFile = pFile;
    
    CyGlobalIntEnable;
    Mic_ADC_Start();				// start the ADC_DelSig_1
	Mic_ADC_StartConvert();		// start the ADC_DelSig_1 conversion
    
    LCD_Start();					    // initialize lcd
	LCD_ClearDisplay();
    rx_int_StartEx(RX_INT);
    UART_Start();                       // initialize UART
    UART_ClearRxBuffer();
    
    clear_file_names();

    for(;;)
    {
        data_available = serial_data_available || DA_Read();
        key_data = (serial_data_available) ? key_data : read_keyboard();        
        
        // Check if button just pushed
        if ((data_available && !last_data_available) || serial_data_available) {
            pressed = key_data;
            if (serial_data_available) serial_data_available = 0;
        } else pressed = -1;
        last_data_available = data_available;
        
        // Set keyboard play mode
        switch (pressed)
        {
            case '0': set_output_mode(0); freq_mult = 1; break;
            case '1': set_output_mode(1); freq_mult = 1; break;
            case '2': freq_mult = 2.0;
            case '3': freq_mult = 3.0;
            case '4': freq_mult = 4.0;
            case '5': freq_mult = 0.5;
        }
        
        // Enable notes in Main menu or recording menu
        if (data_available && (state == MAIN_MENU || state == RECORD_AUDIO)) {
            int note = note_map(key_data);
            set_frequency(note);
        } else if (state == MAIN_MENU || state == RECORD_AUDIO) {
            set_frequency(0);
        }
        
        switch (state)
        {
            case MAIN_MENU: main_menu(pressed); break;
            case RECORD_AUDIO:
                if (!(pMainFile->pFSFile)) {
                    open_file(pMainFile, file_names[0], "w"); 
                    light_print(1);
                }
                sample_ready = Mic_ADC_IsEndConversion(Mic_ADC_RETURN_STATUS);
                if ((pMainFile->pFSFile) && sample_ready) {
                    write_byte(pMainFile, Mic_ADC_GetResult16());
                }
                if (pressed == ESC) {                    
                    int success = close_file(pMainFile, 1, 10);
                    while (!success) light_print(16);
                    change_state(MAIN_MENU);
                }
                break;
            case PROCESS_SELECT: process_select(pressed); break;
            case FILE_SELECT: file_select(pressed); break;
            case PROCESS_FILES: process_files(); break;
            case FILE_PLAYBACK:
                if (!(pMainFile->pFSFile) && strlen(file_names[0])) {
                    open_file(pMainFile, file_names[0], "r");
                    clear_file_names();
                    set_output_mode(1);
                } else if (!(pMainFile->pFSFile)) change_state(MAIN_MENU);
                if (pressed == ESC) {                    
                    int success = close_file(pMainFile, 1, 10);
                    while (!success) light_print(16);
                    change_state(MAIN_MENU);
                }
                set_frequency(SAMPLE_RATE);                
                break;
        }
    }
}

/* [] END OF FILE */