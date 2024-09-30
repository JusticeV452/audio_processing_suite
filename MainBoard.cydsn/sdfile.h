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

#include <FS.h>
#include <debug.h>

#ifndef BUFFER_LEN
#define BUFFER_LEN 512
#endif

typedef struct {
    FS_FILE* pFSFile;
    char name[12];
    int32_t curr_buffer_len;
    uint32_t buffer[BUFFER_LEN];
    uint16_t buffer_idx;
    uint8_t mode;
} SDFile;

int open_file(SDFile* pFile, char* file_name, char* mode)
{
    int Err;
    (pFile->pFSFile) = FS_FOpen(file_name, mode);
    if (!(pFile->pFSFile)) {
        Err = FS_FError((pFile->pFSFile));
        FS_FClose((pFile->pFSFile));
        while (1) light_print(Err);
    }
    pFile->curr_buffer_len = -1;
    pFile->buffer_idx = 0;
    pFile->mode = (mode[0] == 'w');
    return 1;
}

int read_byte(SDFile* pFile)
{
    if (!(pFile->pFSFile) || pFile->mode == 1) return -2;
    if (pFile->curr_buffer_len == 0) return -1; // File exhausted
    if (pFile->curr_buffer_len == -1) {
        pFile->buffer_idx = 0;
        pFile->curr_buffer_len = FS_Read((pFile->pFSFile), pFile->buffer, BUFFER_LEN);
    }
    int data = -1;
    if (pFile->curr_buffer_len) {
        data = ((uint8_t*) pFile->buffer)[pFile->buffer_idx];
        pFile->buffer_idx++;
        if (pFile->buffer_idx == pFile->curr_buffer_len) {
            pFile->curr_buffer_len = FS_Read((pFile->pFSFile), pFile->buffer, BUFFER_LEN);
            pFile->buffer_idx = 0;
        }
    }
    return data;
}

int read_data(SDFile* pFile, uint8_t size)
{
    if (!(pFile->pFSFile) || pFile->mode == 1) return -2;
    if (pFile->curr_buffer_len == 0) return -1; // File exhausted
    int read_thresh = BUFFER_LEN;
    if (pFile->curr_buffer_len == -1) {
        pFile->buffer_idx = 0;
        pFile->curr_buffer_len = FS_FRead(pFile->buffer, size, read_thresh, (pFile->pFSFile));
    }
    int data = -1;
    if (pFile->curr_buffer_len) {
        data = pFile->buffer[pFile->buffer_idx];
        pFile->buffer_idx++;
        if (pFile->buffer_idx == pFile->curr_buffer_len) {
            pFile->curr_buffer_len = FS_FRead(pFile->buffer, size, read_thresh, (pFile->pFSFile));
            pFile->buffer_idx = 0;
        }
    }
    return data;
}

int file_seek(SDFile* pFile, int position)
{
    FS_FSeek(pFile->pFSFile, position, FS_SEEK_SET);
    pFile->curr_buffer_len = FS_Read((pFile->pFSFile), pFile->buffer, BUFFER_LEN);
    pFile->buffer_idx = 0;
    return 0;
}

int file_size(SDFile* pFile)
{
    // FIX USING GENERAL POSITION NOT BUFFIDX
    // RIGHT NOW RESETS FILE POSITION
    FS_FSeek(pFile->pFSFile, 0, FS_SEEK_END);
    int num_samples = FS_FTell(pFile->pFSFile);
    file_seek(pFile, 0);
    return num_samples;
}

int write_byte(SDFile* pFile, int sample)
{
    int written = 0;
    if (!(pFile->pFSFile) || pFile->mode == 0) return -1;   // File not open / in write mode.
    ((uint8_t*) pFile->buffer)[pFile->buffer_idx] = sample;
    pFile->buffer_idx++;
    if (pFile->buffer_idx == BUFFER_LEN) {
        written = FS_Write((pFile->pFSFile), pFile->buffer, BUFFER_LEN);
        pFile->buffer_idx = 0;
        return (written == BUFFER_LEN);
    }
    return 1;
}

int write_data(SDFile* pFile, int sample, uint8_t size)
{
    int written = 0;
    int cannot_write = (!(pFile->pFSFile) || pFile->mode == 0);
    if (cannot_write) return -1;   // File not open / in write mode.
    pFile->buffer[pFile->buffer_idx] = sample;
    pFile->buffer_idx++;
    int write_thresh = BUFFER_LEN;
    if (pFile->buffer_idx == write_thresh) {
        written = FS_FWrite(pFile->buffer, size, write_thresh, (pFile->pFSFile));
        pFile->buffer_idx = 0;
        return (written == write_thresh);
    }
    return 1;
}

int close_file(SDFile* pFile, uint8_t data_size, int close_id)
{
    int success;
    int with_write = (pFile->mode == 1);
    int written = 0;
    if ((pFile->pFSFile) && with_write && (pFile->buffer_idx > 0)) {
        written = FS_FWrite(pFile->buffer, data_size, pFile->buffer_idx, (pFile->pFSFile));
        int error = FS_FError(pFile->pFSFile);
        while(written != pFile->buffer_idx) light_print(error);
    }
    pFile->buffer_idx = 0;
    int just_closed = !FS_FClose((pFile->pFSFile));
    success = (!(pFile->pFSFile) || just_closed);
    if (success) (pFile->pFSFile) = 0;
    return success;
}

int data_seek(SDFile* pFile)
{
    // Skip size
    return FS_FSeek(pFile->pFSFile, 4, FS_SEEK_SET);
}

/* [] END OF FILE */
