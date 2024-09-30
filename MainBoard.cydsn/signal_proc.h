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

#include <dtft.h>
#include <sdfile.h>
#include <debug.h>
#include <kiss_fft.h>

void reverse(char* file_name)
{
	// Initialize files
    SDFile file = {0, "", -1, {}, 0, -1};
    SDFile* pfile = &file;
    SDFile out = {0, "", -1, {}, 0, -1};
    SDFile* pout = &out;
    
    uint8_t data[] = {0, 0};
    
    // Count samples
	open_file(pfile, file_name, "r");
    open_file(pout, "revres", "w");
	int num_samples = file_size(pfile);
	int count = 0;
    
    // Expand file to fit data and write to end
    FS_FSeek(pout->pFSFile, num_samples, FS_SEEK_SET);
    FS_SetEndOfFile(pout->pFSFile);
    
    int temp;	
    while ((temp = read_byte(pfile)) >= 0) {
        data[0] = temp;
        FS_Write((pout->pFSFile), data, 1);
        FS_FSeek(pout->pFSFile, num_samples - count, FS_SEEK_SET);
        show_progress(count, num_samples);
        count++;
    }
    
    close_file(pfile, 1, 10);
    close_file(pout, 1, 10);    
}

void equalize(char* file_name)
{
    SDFile readfile = {0, "", -1, {}, 0, -1};
    SDFile writefile = {0, "", -1, {}, 0, -1};
    
    SDFile* pread = &readfile;
    SDFile* pwrite = &writefile;
    
    open_file(pread, file_name, "r");
    open_file(pwrite, "eqres", "w");
    int sample;
    int counter = 0;
    while ((sample = read_byte(pread)) >= 0) {
        light_print(sample);
		
		// Shift samples down
		// Sqrt magnitude to decrease sample amp variation
        sample -= 128;
        if (sample < 0) {
            write_byte(pwrite, (uint8_t) (12 - sqrt(-sample)) * 10);
        } else {
            write_byte(pwrite, (uint8_t) (12 + sqrt(sample)) * 10);
        }
        counter++;
    }
    
    close_file(pread, 1, 10);
    close_file(pwrite, 1, 10);
}

void multi_sig(char* file_name1, char* file_name2, int operation)
{
    SDFile file1 = {0, "", -1, {}, 0, -1};
    SDFile file2 = {0, "", -1, {}, 0, -1};
    SDFile out = {0, "", -1, {}, 0, -1};
    
    SDFile* pfile1 = &file1;
    SDFile* pfile2 = &file2;
    SDFile* pout = &out;
    
    open_file(pfile1, file_name1, "r");
    open_file(pfile2, file_name2, "r");
    open_file(pout, "multires", "w");
    
    int sample1, sample2, new_sample;
    int counter = 0;
    while ((sample1 = read_byte(pfile1)) >= 0 || (sample2 = read_byte(pfile2)) >= 0) {
        switch(operation) {
			// Add sample values
            case 0: new_sample = ((sample1 >= 0) ? sample1 : 0) + ((sample2 >= 0) ? sample2 : 0); break;
			// Subtract sample1 - sample2
            case 1: 
                new_sample = ((sample1 >= 0) ? sample1 : 0) - ((sample2 >= 0) ? sample2 : 0);
                if (new_sample < 0) new_sample = 0;
                break;
			// Multiply samples
            case 2: new_sample = ((sample1 >= 0) ? sample1 : 0) * ((sample2 >= 0) ? sample2 : 0); break;
			// Divide sample1 by sample2
            case 3:
                if (sample2 > 0) new_sample = ((sample1 >= 0) ? sample1 : 0) / sample2;
                else new_sample = 255;
                break;
			// Subtract sample2 - sample1
            case -1: 
                new_sample = ((sample1 >= 0) ? sample1 : 0) - ((sample2 >= 0) ? sample2 : 0);
                new_sample *= - 1;
                if (new_sample < 0) new_sample = 0;
                break;
			// Divide sample2 by sample1
            case -3:
                if (sample1 > 0) new_sample = ((sample2 >= 0) ? sample2 : 0) / sample1;
                else new_sample = 255;
                break;                
        }
        write_byte(pout, new_sample);
        show_loading(250);
        counter++;
    }
    close_file(pfile1, 1, 10);
    close_file(pfile2, 1, 10);
    close_file(pout, 1, 10);
}

void overlay(char* file_name1, char* file_name2)
{
    SDFile file1 = {0, "", -1, {}, 0, -1};
    SDFile file2 = {0, "", -1, {}, 0, -1};
    SDFile out = {0, "", -1, {}, 0, -1};
    
    SDFile* pfile1 = &file1;
    SDFile* pfile2 = &file2;
    SDFile* pout = &out;
    
    open_file(pfile1, file_name1, "r");
    open_file(pfile2, file_name2, "r");
    open_file(pout, "ovlres", "w");
    
    int sample1, sample2, new_sample;
    int counter = 0;
    while ((sample1 = read_byte(pfile1)) >= 0 || (sample2 = read_byte(pfile2)) >= 0) {
		// Take average of samples from signals
        new_sample = ((sample1 >= 0) ? sample1: 0);
        new_sample += ((sample2 >= 0) ? sample2: 0);
        write_byte(pout, new_sample / 2);
        show_loading(250);
        counter++;
    }
    close_file(pfile1, 1, 10);
    close_file(pfile2, 1, 10);
    close_file(pout, 1, 10);
}

void fade(char* file_name, int fade_in, int fade_out)
{
    SDFile file = {0, "", -1, {}, 0, -1};
    SDFile* pfile = &file;
    SDFile out = {0, "", -1, {}, 0, -1};
    SDFile* pout = &out;
    
    open_file(pfile, file_name, "r");
    open_file(pout, "faderes", "w");
	int num_samples = file_size(pfile);    
    int data = 0;
    
	// Set range for reducing signal amplitude
    int fade_in_stop = (fade_in) ?  num_samples / 4 : 0;
    int fade_out_start = (fade_out) ? (num_samples * 3 / 4) : num_samples;
    
    int new_sample, counter = 0;
    while ((data = read_byte(pfile)) >= 0) {
        new_sample = data;
        if (counter < fade_in_stop) {
			// Increase amplitude exponentially
            new_sample *= pow(.9, (fade_in_stop - counter));
        } else if (counter >= fade_out_start) {
			// Decrease amplitude exponentially
            new_sample *= pow(.9, (counter - fade_out_start));
        }
        write_byte(pout, new_sample);
        
        show_progress(counter, num_samples);
        counter++;
    }
    close_file(pfile, 1, 10);
    close_file(pout, 1, 10);
}

int dtft_filter(char* file_name, int low_bound, int high_bound)
{
    SDFile file = {0, "", -1, {}, 0, -1};
    SDFile* pfile = &file;
    SDFile out = {0, "", -1, {}, 0, -1};
    SDFile* pout = &out;
    
    open_file(pfile, file_name, "r");
    open_file(pout, "filterres", "w");
    
    DTFT trn = {
        .freq_res = 10,
        .coeffs = 0,
        .num_samples = 0,
        .sample_idx = 0,
        .buffers = {},
        .read_idx = 0,
        .ref = {},
    };
    DTFT* ptrn = &trn;
    
    uint32_t i = 0;
    int data, update_error;
    while ((data = read_byte(pfile)) >= 0) {
        update_error = updateDTFT2(ptrn, data);
        if (update_error) {
            close_file(pfile, 1, 10);
            close_file(pout, 1, 11);
            return 1;
        }
        light_print(data);
    }
    if (high_bound == -1) high_bound = ptrn->num_samples;
    if (low_bound == -2) low_bound = ptrn->num_samples / 2;
    if (high_bound == -2) high_bound = ptrn->num_samples / 2;
    for (i = 0; i < ptrn->num_samples; i++) {
        write_byte(pout, sampleDTFT2(ptrn, low_bound, high_bound));
        show_progress(i, ptrn->num_samples);
    }
    
    close_file(pfile, 1, 10);
    close_file(pout, 1, 11);
    closeDTFT(ptrn, 0);
    return 0;
}

int kfft_filter(char* file_name, int low_bound, int high_bound)
{
    SDFile file = {0, "", -1, {}, 0, -1};
    SDFile* pfile = &file;
    SDFile out = {0, "", -1, {}, 0, -1};
    SDFile* pout = &out;
	
	// Init fft vars
    const int BUFF_SIZE = 128;
    complex float sample_buffer[BUFF_SIZE];
    complex float fft_buffer[BUFF_SIZE];
    kiss_fft_cfg fft_cfg = kiss_fft_alloc(BUFF_SIZE ,0 ,0, 0);
    kiss_fft_cfg ifft_cfg = kiss_fft_alloc(BUFF_SIZE ,1 ,0, 0);
    uint32_t buff_idx = 0;
    
    open_file(pfile, file_name, "r");
    open_file(pout, "filtres", "w");
    
    uint32_t i = 0;
    int data, update_error, sample;
    
    if (high_bound == -1) high_bound = BUFF_SIZE;
    if (low_bound == -2) low_bound = BUFF_SIZE / 2;
    if (high_bound == -2) high_bound = BUFF_SIZE / 2;
    
    while ((data = read_byte(pfile)) >= 0) {
        sample_buffer[buff_idx] = data;
        buff_idx++;
        // Filter freqs
        if (buff_idx == BUFF_SIZE) {            
            kiss_fft(fft_cfg, sample_buffer, fft_buffer);   // FFT
			
            // Frequency filter
            memset(fft_buffer + low_bound, 0, (high_bound - low_bound));
			
            kiss_fft(ifft_cfg, fft_buffer, sample_buffer);  // iFFT
            for (i = 0; i < BUFF_SIZE; i++) {
                sample = creal(sample_buffer[i] / BUFF_SIZE);
                write_byte(pout, sample); 
                light_print(data);
            }
            buff_idx = 0;   // Reload for next sample buffer
        } else light_print(data);
    }
    
    // Do transform on remaining samples
    if (buff_idx) {
        // Clear out samples after signal end
        memset(sample_buffer + buff_idx, 0, (BUFF_SIZE - buff_idx));
        
        kiss_fft(fft_cfg, sample_buffer, fft_buffer);   // FFT
		
        // Frequency filter
        memset(fft_buffer + low_bound, 0, (high_bound - low_bound));
		
        kiss_fft(ifft_cfg, fft_buffer, sample_buffer);  // iFFT
        for (i = 0; i < buff_idx; i++) {
            write_byte(pout, sample_buffer[i] / BUFF_SIZE); 
            light_print(data);
        }
        buff_idx = 0;   // Reload for next sample buffer        
    }
    
    kiss_fft_free(fft_cfg);
    kiss_fft_free(ifft_cfg);
    
    close_file(pfile, 1, 10);
    close_file(pout, 1, 11);
    return 0;
}

void amplify(char* file_name, int factor)
{
    SDFile file = {0, "", -1, {}, 0, -1};
    SDFile* pfile = &file;
    SDFile out = {0, "", -1, {}, 0, -1};
    SDFile* pout = &out;
    
    open_file(pfile, file_name, "r");
    open_file(pout, "ampres", "w");
    int num_samples = file_size(pfile);
    
    int sample, counter = 0;
    while ((sample = read_byte(pfile)) >= 0) {
        write_byte(pout, factor * sample);        
        show_progress(counter, num_samples);
        counter++;
    }
    close_file(pfile, 1, 10);
    close_file(pout, 1, 10);
}

void echo(char* file_name)
{
    SDFile file = {0, "", -1, {}, 0, -1};
    SDFile* pfile = &file;
    SDFile out = {0, "", -1, {}, 0, -1};
    SDFile* pout = &out;
    
    open_file(pfile, file_name, "r");
    open_file(pout, "echores", "w");
	int num_samples = file_size(pfile);
	
	// Init fft vars
	const int BUFF_SIZE = 128;
    complex float sample_buffer[BUFF_SIZE];
    complex float fft_buffer[BUFF_SIZE];	
    kiss_fft_cfg fft_cfg = kiss_fft_alloc(BUFF_SIZE ,0 ,0, 0);
    kiss_fft_cfg ifft_cfg = kiss_fft_alloc(BUFF_SIZE ,1 ,0, 0);
    uint32_t buff_idx = 0;
	
	complex float time_box[BUFF_SIZE];
	complex float box_fft[BUFF_SIZE];
    memset(time_box, 0, 8 * BUFF_SIZE);
	int s;	// Make box in time domain
	for (s = 0; s < 4; s++) time_box[s] = 1;
	kiss_fft(fft_cfg, time_box, box_fft);   // FFT
	
    uint8_t i;
    int data, counter = 0;
    while ((data = read_byte(pfile)) >= 0) {
        sample_buffer[buff_idx] = data;
        buff_idx++;
        // Filter freqs
        if (buff_idx == BUFF_SIZE) {            
            kiss_fft(fft_cfg, sample_buffer, fft_buffer);   // FFT
		
			// Convolution in time domain (produce echo)
			for (i = 0; i < buff_idx; i++) fft_buffer[i] *= box_fft[i];	
			
			kiss_fft(ifft_cfg, fft_buffer, sample_buffer);  // iFFT
			for (i = 0; i < buff_idx; i++) {
				write_byte(pout, sample_buffer[i] / BUFF_SIZE);
			}
			buff_idx = 0;   // Reload for next sample buffer        
        }
		show_progress(counter, num_samples);
		counter++;
    }
	
	// Do transform on remaining samples
    if (buff_idx) {
        // Clear out samples after signal end
        memset(sample_buffer + buff_idx, 0, (BUFF_SIZE - buff_idx));
        
        kiss_fft(fft_cfg, sample_buffer, fft_buffer);   // FFT
		
		// Convolution in time domain (produce echo)
		for (i = 0; i < buff_idx; i++) fft_buffer[i] *= box_fft[i];	
		
        kiss_fft(ifft_cfg, fft_buffer, sample_buffer);  // iFFT
        for (i = 0; i < buff_idx; i++) {
            write_byte(pout, sample_buffer[i] / BUFF_SIZE);
			show_loading(250);
        }
        buff_idx = 0;   // Reload for next sample buffer  		
    }
    
    kiss_fft_free(fft_cfg);
    kiss_fft_free(ifft_cfg);
	
    close_file(pfile, 1, 10);
    close_file(pout, 1, 11);    
}

/* [] END OF FILE */
