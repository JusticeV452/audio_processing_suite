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

#include "project.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define _USE_MATH_DEFINES
#include <complex.h>

#include <sdfile.h>

typedef struct {
    uint32_t freq_res;
    complex float* coeffs;
    uint32_t num_samples;
    uint32_t sample_idx;
    SDFile buffers[2];
    uint8_t read_idx;
    char ref[8];
} DTFT;

void updateDTFT(DTFT* pdtft, int sample)
{
    uint32_t i;
    complex float last_k, Omega, k_delta;
    SDFile* pread_buff;
    SDFile* pwrite_buff;
    
    // Open read buffer
    if (pdtft->num_samples) {
        pread_buff = &pdtft->buffers[pdtft->read_idx];
        open_file(pread_buff, (pdtft->read_idx) ? "ref2" : "ref1", "r");
		
        // Skip header info (res, num_samples), freeze if error
        int error = FS_FSeek(pread_buff->pFSFile, 8, FS_SEEK_SET);
        while (error) light_print(85);
    }
    
    // Open write buffer
    pwrite_buff = &pdtft->buffers[(pdtft->read_idx + 1) % 2];
    open_file(pwrite_buff, (pdtft->read_idx) ? "ref1" : "ref2", "w");
    
    // Write header (res, num_samples)
    int write_res = write_data(pwrite_buff, pdtft->freq_res, 4);
    int success = (write_res > 0);
    while (!success) light_print(9);
    success = write_data(pwrite_buff, pdtft->num_samples + 1, 4) > 0;
    while(!success) light_print(55);
    
    for (i = 0; i < pdtft->freq_res; i++) {
        Omega = (2 * M_PI * (double) i / pdtft->freq_res);
        k_delta = sample * cexp(-1 * _Complex_I * Omega * pdtft->num_samples);
        last_k = 0;
        if (pdtft->num_samples) {
            last_k += read_data(pread_buff, 4);
            last_k += _Complex_I * read_data(pread_buff, 4);
        }
        last_k += k_delta;
        success = write_data(pwrite_buff, (float) creal(last_k), 4) > 0;
        while(!success) light_print(170);
        success = write_data(pwrite_buff, (float) cimag(last_k), 4) > 0;
        while(!success) light_print(171);
    }
    
    // Close files
    int closed = close_file(&pdtft->buffers[0], 4, 60);
    while(!closed);
    closed = close_file(&pdtft->buffers[1], 4, 90);
    while(!closed);
    
    pdtft->num_samples++;
    pdtft->read_idx = (pdtft->read_idx + 1) % 2;
}

int updateDTFT2(DTFT* pdtft, int sample)
{
    // Return of function indicates an error
    
    uint32_t i;
    complex float last_k, Omega, k_delta;
    if (!pdtft->coeffs) {
        pdtft->coeffs = (complex float*)malloc(pdtft->freq_res * sizeof(complex float));
        memset(pdtft->coeffs, 0, pdtft->freq_res * sizeof(complex float));
        if (!pdtft->coeffs) return 1;
    }
    
    for (i = 0; i < pdtft->freq_res; i++) {
        Omega = (2 * M_PI * (double) i / pdtft->freq_res);
        k_delta = sample * cexp(-1 * _Complex_I * Omega * pdtft->num_samples);
        last_k = pdtft->coeffs[i];
        pdtft->coeffs[i] = last_k + k_delta;
    }
    pdtft->num_samples++;
    return 0;
}

void closeDTFT(DTFT* pdtft, int backup)
{
    if (backup) {
        // Open file to save in
        SDFile* pfile = &pdtft->buffers[0];
        uint8_t backup_success = 0;
        
        open_file(pfile, "coeff.ft", "w");
        backup_success = (write_data(pfile, pdtft->freq_res, 4) > 0);
        while (!backup_success) light_print(9);
        backup_success = write_data(pfile, pdtft->num_samples, 4) > 0;
        while(!backup_success) light_print(55);
    
        uint32_t i;
        complex float k;
        for (i = 0; i < pdtft->freq_res; i++) {
            k = pdtft->coeffs[i];
            backup_success = write_data(pfile, creal(k), 4) > 0;
            while(!backup_success) light_print(56);
            backup_success = write_data(pfile, cimag(k), 4) > 0;
            while(!backup_success) light_print(56);
        }
        
        // Close file
        int closed = close_file(pfile, 4, 67);
        while (!closed);
    }
    
    pdtft->read_idx = 0;
    pdtft->num_samples = 0;
    pdtft->sample_idx = 0;
    free(pdtft->coeffs);    
}

void endDTFT(DTFT* pdtft)
{
    // Delete temp buffer
    while (FS_Remove((pdtft->read_idx) ? "ref1" : "ref2")) light_print (92);
	
    // Rename final file
    while (FS_Rename((pdtft->read_idx) ? "ref2" : "ref1", "coeff.ft")) light_print (170);
    
    // Close files
    int closed = close_file(&pdtft->buffers[0], 4, 67);
    while (!closed);
    closed = close_file(&pdtft->buffers[1], 4, 97);
    while (!closed);
    
    pdtft->read_idx = 0;
	pdtft->num_samples = 0;
}

uint8_t sampleDTFT(DTFT* pdtft)
{
    SDFile* pfile = &pdtft->buffers[0];
    if (!(pfile->pFSFile)) open_file(pfile, "coeff.ft", "r");
    
    int num_coeffs = read_data(pfile, 4);
    uint32_t num_samples = read_data(pfile, 4);
    if (pdtft->sample_idx == num_samples) {
        close_file(pfile, 4, 13);
        return -1;
    }
    
    int i = 0;
    float Omega, real, imag;
    complex float F_Omega, exp_val;
    complex float result = 0;
    for (i = 0; i < num_coeffs; i++) {
        Omega = (2 * M_PI * (double) i / num_coeffs);
        exp_val = cexp(_Complex_I * Omega * pdtft->sample_idx);
        real = read_data(pfile, 4);
        imag = read_data(pfile, 4);
        F_Omega = real + _Complex_I * imag;
        result += F_Omega * exp_val;
    }
    pdtft->sample_idx++;
    return creal(result / (2 * M_PI));
}

int sampleDTFT2(DTFT* pdtft, uint32_t low_bound, uint32_t high_bound)
{
    if (!(pdtft->coeffs)) return -2;
    if (pdtft->sample_idx == pdtft->num_samples) {
        return -1;
    }
    
    float Omega;
    complex float F_Omega, exp_val;
    uint32_t i = 0;
    complex float result = 0;
    
    for (i = 0; i < pdtft->freq_res; i++) {
        if (i < low_bound || i >= high_bound) continue;
        Omega = (2 * M_PI * (double) i / pdtft->freq_res);
        exp_val = cexp(_Complex_I * Omega * pdtft->sample_idx);
        F_Omega = pdtft->coeffs[i];
        result += F_Omega * exp_val;
    }
    pdtft->sample_idx++;
    return creal(result / (2 * M_PI));
}

/* [] END OF FILE */
