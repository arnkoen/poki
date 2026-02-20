/*
tmixer - tiny audio mixer

This is a C-language rewrite of the original C++ library tinymixer
(https://github.com/mendsley/tinymixer), originally authored by
Mathew Endsley, licensed under the BSD 2-Clause License.

All credit goes to the original author.

-------------------------------------------------------------------------------

Original copyright:

Copyright (c) 2011-2013, Mathew Endsley
All rights reserved.

C rewrite and modifications:

Copyright (c) 2025, Arne König
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TINY_MIXER_H
#define TINY_MIXER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct tm_buffer tm_buffer;

typedef struct tm_channel {
	int index;
} tm_channel;

typedef struct tm_callbacks {
	void* udata;
	void* (*allocate)(void* opaque, int bytes);
	void  (*free)(void* opaque, void* pointer);
	void (*pre_effects)(void* opaque, float* samples, int nsamples, float gain);
	void (*channel_complete)(void* opaque, void* channel_opaque, tm_channel channel);
} tm_callbacks;

typedef struct tm_buffer_callbacks {
	void (*on_destroy)(void* buffer_opqaue);
	void* (*start_source)(void* buffer_opaque);
	void (*end_source)(void* buffer_opaque, void* source_opaque);
	int (*request_samples)(void* buffer_opaque, void* source_opaque, const float** left, const float** right, int nsamples);
} tm_buffer_callbacks;

typedef struct tm_resampler {
	float ideal_rate;
	float prev_samples[2];
} tm_resampler;

typedef struct tm_lowpass_filter {
	float cutoff_frequency; // 1250.0 is a decent starting point
	float sample_rate;
	float channel_history[2];
} tm_lowpass_filter;

void tm_init(tm_callbacks callbacks, int sample_rate);
void tm_shutdown(void);
void tm_getsamples(float* samples, int nsamples);
void tm_set_mastergain(float gain);

void tm_create_buffer_interleaved_s16le(int channels, const int16_t* pcm_data, int pcm_data_size, const tm_buffer** handle);
void tm_create_buffer_interleaved_float(int channels, const float* pcm_data, int pcm_data_size, const tm_buffer** handle);
void tm_create_buffer_vorbis_stream(const void* data, int ndata, void* opaque, void (*closed)(void*), const tm_buffer** handle);
void tm_create_buffer_custom_stream(void* opaque, tm_buffer_callbacks callbacks, const tm_buffer** buffer);
int tm_get_buffer_size(const tm_buffer* handle);
void tm_release_buffer(const tm_buffer* handle);

void tm_update_listener(const float* position, const float* forward);
void tm_set_base_gain(int index, float gain);
void tm_set_callback_gain(float gain);
void tm_effects_compressor(const float thresholds[2], const float multipliers[2], float attack_seconds, float release_seconds);

bool tm_add(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel);
bool tm_add_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, tm_channel* channel);
bool tm_add_spatial(const tm_buffer* handle, int gain_index, float gain, float pitch, const float* position, float distance_min, float distance_max, tm_channel* channel);
bool tm_add_spatial_loop(const tm_buffer* handle, int gain_index, float gain, float pitch, const float* position, float distance_min, float distance_max, tm_channel* channel);

void tm_channel_set_opaque(tm_channel channel, void* opaque);
void tm_channel_stop(tm_channel channel);
void tm_channel_set_position(tm_channel channel, const float* position);
void tm_channel_fadeout(tm_channel channel, float seconds);
void tm_channel_set_gain(tm_channel channel, float gain);
void tm_channel_set_frequency(tm_channel channel, float frequency);

float tm_channel_get_gain(tm_channel channel);
void tm_stop_all_sources();

bool tm_channel_isplaying(tm_channel channel);
static inline bool tm_channel_isvalid(tm_channel channel) { return channel.index != 0; }
static inline bool tm_channel_equals(tm_channel lhs, tm_channel rhs) { return lhs.index == rhs.index; }

void tm_resampler_init(tm_resampler* resampler, int input_sample_rate, int output_sample_rate);
void tm_resampler_init_rate(tm_resampler* resampler, float ideal_rate);
int tm_resampler_calculate_input_samples(const tm_resampler* resampler, int output_samples);
int tm_resampler_calculate_output_samples(const tm_resampler* resampler, int input_samples);
void tm_resample_stereo(tm_resampler* resampler, const float* input, int num_input_samples, float* output, int num_output_samples);
void tm_resample_mono(tm_resampler* resampler, const float* input, int num_input_samples, float* output, int num_output_samples);

void tm_lowpass_filter_init(tm_lowpass_filter* filter, float cutoff_frequency, float sample_rate);
void tm_lowpass_filter_apply(tm_lowpass_filter* filter, float* output, float* input, int num_samples, int num_channels);

#endif //TINY_MIXER_H
