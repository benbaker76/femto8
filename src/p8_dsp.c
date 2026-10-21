/*
 * p8_dsp.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "p8_audio.h"
#include "p8_dsp.h"

int random_range(int min, int max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

void dsp_square_wave(uint32_t frequency, int16_t amplitude, int16_t offset, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int period_length = (float)SAMPLE_RATE / frequency;
    if (period_length == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int half_period = period_length / 2;
    for (int i = 0; i < dest_length; i++)
    {
        int sample_in_period = position % period_length;
        dest[dest_offset + i] += (offset + sample_in_period < half_period ? -amplitude : amplitude);
        position++;
    }
}

void dsp_pulse_wave(uint32_t frequency, int16_t amplitude, int16_t offset, float duty_cycle, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int period_length = (float)SAMPLE_RATE / frequency;
    if (period_length == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int duty_on_length = duty_cycle * period_length;
    for (int i = 0; i < dest_length; i++)
    {
        int sample_in_period = position % period_length;
        dest[dest_offset + i] += (offset + sample_in_period < duty_on_length ? amplitude : -amplitude);
        position++;
    }
}

void dsp_triangle_wave(uint32_t frequency, int16_t amplitude, int16_t offset, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int period_length = (float)SAMPLE_RATE / frequency;
    if (period_length == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    for (int i = 0; i < dest_length; i++)
    {
        int repetitions = position / period_length;
        float p = position / (float)period_length - repetitions;
        if (p < 0.50f)
            dest[dest_offset + i] += (offset + amplitude - amplitude * 2 * (p / 0.5f));
        else
            dest[dest_offset + i] += (offset - amplitude + amplitude * 2 * ((p - 0.5f) / 0.5f));
        position++;
    }
}

void dsp_sawtooth_wave(uint32_t frequency, int16_t amplitude, int16_t offset, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int period_length = (float)SAMPLE_RATE / frequency;
    if (period_length == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    for (int i = 0; i < dest_length; i++)
    {
        int repetitions = position / period_length;
        float p = position / (float)period_length - repetitions;
        dest[dest_offset + i] += (offset - amplitude + amplitude * 2 * p);
        position++;
    }
}

void dsp_tilted_sawtooth_wave(uint32_t frequency, int16_t amplitude, int16_t offset, float duty_cycle, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int period_length = (float)SAMPLE_RATE / frequency;
    if (period_length == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    for (int i = 0; i < dest_length; i++)
    {
        int repetitions = position / period_length;
        float p = position / (float)period_length - repetitions;
        if (p < duty_cycle)
            dest[dest_offset + i] += (offset - amplitude + amplitude * 2 * (p / duty_cycle));
        else
        {
            float op = (p - duty_cycle) / (1.0f - duty_cycle);
            dest[dest_offset + i] += (offset + amplitude - amplitude * 2 * op);
        }
        position++;
    }
}

void dsp_organ_wave(uint32_t frequency, int16_t amplitude, int16_t offset, float coefficient, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    int period_length = (float)SAMPLE_RATE / frequency;
    if (period_length == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += offset;
        return;
    }
    for (int i = 0; i < dest_length; i++)
    {
        int repetitions = position / period_length;
        float p = position / (float)period_length - repetitions;
        if (p < 0.25f)
            dest[dest_offset + i] += (offset + amplitude - amplitude * 2 * (p / 0.25f));
        else if (p < 0.50f)
            dest[dest_offset + i] += (offset - amplitude + amplitude * (1.0f + coefficient) * (p - 0.25f) / 0.25f);
        else if (p < 0.75f)
            dest[dest_offset + i] += (offset + amplitude * coefficient - amplitude * (1.0f + coefficient) * (p - 0.50f) / 0.25f);
        else
            dest[dest_offset + i] += (offset - amplitude + amplitude * 2 * (p - 0.75f) / 0.25f);
        position++;
    }
}

void dsp_noise(uint32_t frequency, int16_t amplitude, int position, int dest_offset, int dest_length, int16_t *dest)
{
    if (frequency == 0) {
        for (int i = 0; i < dest_length; i++)
            dest[dest_offset + i] += 0;
        return;
    }
    for (int i = 0; i < dest_length; i++)
    {
        dest[dest_offset + i] += (int16_t)random_range(-amplitude / 2, amplitude / 2);
        position++;
    }
}

void dsp_fade_in(int16_t amplitude, int dest_offset, int dest_length, int16_t *dest)
{
    (void)amplitude;  // unused parameter - kept for API compatibility
    float incr = 1.0f / dest_length;
    for (int i = 0; i < dest_length; i++)
    {
        float gain = incr * i;
        dest[dest_offset + i] = (int16_t)(dest[dest_offset + i] * gain);
    }
}

void dsp_fade_out(int16_t amplitude, int dest_offset, int dest_length, int16_t *dest)
{
    (void)amplitude;  // unused parameter - kept for API compatibility
    float incr = 1.0f / dest_length;
    for (int i = 0; i < dest_length; i++)
    {
        float gain = incr * (dest_length - 1 - i);
        dest[dest_offset + i] = (int16_t)(dest[dest_offset + i] * gain);
    }
}
