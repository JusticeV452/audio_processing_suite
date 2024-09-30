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

int logb2(int x) {
  if (x <= 0) return -1;
  int result = 0;
  while (x > 1) {
    x /= 2;
    result++;
  }
  return result;
}

int char_lookup(int idx)
{
    int result = idx;
    // 0 - 9
    if (idx == 0b01110001) {
        result = 48;
    } else if (idx == 0b01000100) {
        result = 49;
    } else if (idx == 0b01100111) {
        result = 50;
    } else if (idx == 0b10001011) {
        result = 51;
    } else if (idx == 0b01000001) {
        result = 52;
    } else if (idx == 0b01000101) {
        result = 53;
    } else if (idx == 0b01011101) {
        result = 54;
    } else if (idx == 0b01110110) {
        result = 55;
    } else if (idx == 0b00001111) {
        result = 56;
    } else if (idx == 0b00000110) {
        result = 57;
    // A - Z
    } else if (idx == 0b01101100) {
        result = 65;
    } else if (idx == 0b01011110) {
        result = 66;
    } else if (idx == 0b01101000) {
        result = 67;
    } else if (idx == 0b00100101) {
        result = 68;
    } else if (idx == 0b00110100) {
        result = 69;
    } else if (idx == 0b00001100) {
        result = 70;
    } else if (idx == 0b00111111) {
        result = 71;
    } else if (idx == 0b10000000) {
        result = 72;
    } else if (idx == 0b01000000) {
        result = 73;
    } else if (idx == 0b00111100) {
        result = 74;
    } else if (idx == 0b00101011) {
        result = 75;
    } else if (idx == 0b01101110) {
        result = 76;
    } else if (idx == 0b00001101) {
        result = 77;
    } else if (idx == 0b00111000) {
        result = 78;
    } else if (idx == 0b01110101) {
        result = 79;
    } else if (idx == 0b00101100) {
        result = 80;
    } else if (idx == 0b01111111) {
        result = 81;
    } else if (idx == 0b01011100) {
        result = 82;
    } else if (idx == 0b00000010) {
        result = 83;
    } else if (idx == 0b01001101) {
        result = 84;
    } else if (idx == 0b00001010) {
        result = 85;
    } else if (idx == 0b01010111) {
        result = 86;
    } else if (idx == 0b00011100) {
        result = 87;
    } else if (idx == 0b01001111) {
        result = 88;
    } else if (idx == 0b01001011) {
        result = 89;
    } else if (idx == 0b01010001) {
        result = 90;
    // Special
    } else if (idx == 0b00011011) {
        result = 13;    // '\r'
    } else if (idx == 0b00010011) {
        result = 10;    // '\n'
    } else if (idx == 0b01001110) {
        result = 32;    // ' '
    } else if (idx == 0b00100001) {
        result = 8;    // '<-'
    } else if (idx == 0b01111010) {
        result = 27;    // 'ESC'
    } else if (idx == 0b01111000) {
        result = 128;    // 'UP'
    } else if (idx == 0b00111101) {
        result = 129;    // 'DOWN'
    } else if (idx == 0b00101110) {
        result = 130;    // 'LEFT'
    } else if (idx == 0b00010101) {
        result = 131;    // 'RIGHT'
    } else if (idx == 0b00110101) {
        result = 132;    // 'WIN'
    } else if (idx == 0b01110010) {
        result = 127;    // 'DEL'
    }
    return result;
}

/* [] END OF FILE */
