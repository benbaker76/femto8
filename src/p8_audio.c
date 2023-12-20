/*
 * p8_audio.c
 *
 *  Created on: Dec 13, 2023
 *      Author: bbaker
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "p8_audio.h"
#include "p8_dsp.h"
#include "p8_emu.h"
#include "queue.h"

#ifdef SDL
#include "SDL.h"
#endif

#define EFFECT_MASK 0x7000
#define VOLUME_MASK 0x0E00
#define WAVEFORM_MASK 0x01C0
#define PITCH_MASK 0x003F

#define EFFECT_SHIFT 12
#define VOLUME_SHIFT 9
#define WAVEFORM_SHIFT 6

enum
{
    SOUNDMODE_NONE,
    SOUNDMODE_SOUND,
    SOUNDMODE_MUSIC
};

typedef struct
{
    int pattern;
    uint8_t channel_mask;
} musicstate_t;

typedef struct
{
    int sound_mode;
    int sound_index;
    int sample;
    int position;
    int end;
} soundstate_t;

typedef struct
{
    int32_t index;
    int32_t channel;
    uint32_t start;
    uint32_t end;
} sound_t;

typedef struct
{
    int32_t index;
    int32_t fadems;
    int32_t mask;
} music_t;

typedef struct
{
    int sound_mode;

    union
    {
        sound_t sound;
        music_t music;
    };
} soundcommand_t;

enum
{
    TONE_C,
    TONE_CS,
    TONE_D,
    TONE_DS,
    TONE_E,
    TONE_F,
    TONE_FS,
    TONE_G,
    TONE_GS,
    TONE_A,
    TONE_AS,
    TONE_B
};

enum
{
    WAVEFORM_TRIANGLE,
    WAVEFORM_TILTEDSAW,
    WAVEFORM_SAW,
    WAVEFORM_SQUARE,
    WAVEFORM_PULSE,
    WAVEFORM_ORGAN,
    WAVEFORM_NOISE,
    WAVEFORM_PHASER
};

enum
{
    EFFECT_NONE,
    EFFECT_SLIDE,
    EFFECT_VIBRATO,
    EFFECT_DROP,
    EFFECT_FADEIN,
    EFFECT_FADEOUT,
    EFFECT_ARPEGGIOFAST,
    EFFECT_ARPEGGIOSLOW
};

const char *m_tone_map[] = {
    "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

const float m_tone_frequencies[] = {
    130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.0f, 233.08f, 246.94f};

void render_sounds(int16_t *buffer, int total_samples);

bool m_music_enabled = true;
bool m_sound_enabled = true;

soundcommand_t m_sound_buffer[SOUND_QUEUE_SIZE];

queue_t m_sound_queue = {
    .data_buf = m_sound_buffer,
    .elements_num_max = SOUND_QUEUE_SIZE,
    .elements_size = sizeof(soundcommand_t),
};

pthread_mutex_t m_sound_queue_mutex;

soundstate_t m_channels[CHANNEL_COUNT];
musicstate_t m_music_state;

#ifdef SDL
SDL_AudioSpec m_audio_spec;
#endif

void audio_callback(void *userdata, uint8_t *cbuffer, int length)
{
    render_sounds((int16_t *)cbuffer, length / sizeof(int16_t));
}

void audio_init()
{
    _queue_init(&m_sound_queue);

#ifdef SDL
    m_audio_spec.freq = SAMPLE_RATE;
    m_audio_spec.format = AUDIO_S16SYS;
    m_audio_spec.channels = 1;
    m_audio_spec.samples = SOUND_BUFFER_SIZE;
    m_audio_spec.userdata = NULL;
    m_audio_spec.callback = audio_callback;

    int ret = SDL_OpenAudio(&m_audio_spec, &m_audio_spec);

    if (ret != 0)
    {
        printf("Error on SDL_OpenAudio()\n");
    }

    SDL_PauseAudio(0);
#endif
}

void audio_resume()
{
#ifdef SDL
    SDL_PauseAudio(0);
#endif
}

void audio_pause()
{
#ifdef SDL
    SDL_PauseAudio(1);
#endif
}

void audio_close()
{
#ifdef SDL
    SDL_CloseAudio();
#endif
}

void audio_sound(int32_t index, int32_t channel, uint32_t start, uint32_t end)
{
    soundcommand_t sound_command;
    sound_command.sound_mode = SOUNDMODE_SOUND;
    sound_command.sound.index = index;
    sound_command.sound.channel = channel;
    sound_command.sound.start = start;
    sound_command.sound.end = end;

    pthread_mutex_lock(&m_sound_queue_mutex);
    queue_add_back(&m_sound_queue, &sound_command);
    pthread_mutex_unlock(&m_sound_queue_mutex);
}

void audio_music(int32_t index, int32_t fadems, int32_t mask)
{
    soundcommand_t sound_command;
    sound_command.sound_mode = SOUNDMODE_MUSIC;
    sound_command.music.index = index;
    sound_command.music.fadems = fadems;
    sound_command.music.mask = mask;

    pthread_mutex_lock(&m_sound_queue_mutex);
    queue_add_back(&m_sound_queue, &sound_command);
    pthread_mutex_unlock(&m_sound_queue_mutex);
}

void update_channel(soundstate_t *channel)
{
    if (channel->sound_mode == SOUNDMODE_NONE)
        return;

    if (channel->sound_mode == SOUNDMODE_MUSIC)
    {
        if (channel->sample >= channel->end)
        {
            bool is_loop_begin = m_memory[MEMORY_MUSIC + 4 * m_music_state.pattern] & (1 << 7);
            bool is_loop_end = m_memory[MEMORY_MUSIC + 4 * m_music_state.pattern + 1] & (1 << 7);
            bool is_stop = m_memory[MEMORY_MUSIC + 4 * m_music_state.pattern + 2] & (1 << 7);
            if (is_stop)
            {
                return;
            }
            m_music_state.pattern++;
            if (is_loop_end || m_music_state.pattern == MUSIC_COUNT)
            {
                int i = m_music_state.pattern - 1;
                while (i >= 0)
                {
                    is_loop_begin = (m_memory[MEMORY_MUSIC + 4 * i] & (1 << 7));
                    if (is_loop_begin || i == 0)
                        break;
                    i--;
                }
                m_music_state.pattern = i;
            }
            for (int i = 0; i < CHANNEL_COUNT; i++)
            {
                uint8_t channel_data = m_memory[MEMORY_MUSIC + 4 * m_music_state.pattern + i];
                bool enabled = (channel_data & (1 << 6)) == 0;
                if (enabled)
                {
                    m_channels[i].sound_mode = SOUNDMODE_MUSIC;
                    m_channels[i].sound_index = channel_data & 0x7F;
                    m_channels[i].sample = 0;
                    m_channels[i].position = 0;
                    m_channels[i].end = 31;
                }
                else
                    m_channels[i].sound_mode = SOUNDMODE_NONE;
            }
        }
    }
    else if (channel->sound_mode == SOUNDMODE_SOUND)
    {
        if (channel->sample >= channel->end)
            channel->sound_mode = SOUNDMODE_NONE;
    }
}

void update_sound_queue()
{
    pthread_mutex_lock(&m_sound_queue_mutex);

    soundcommand_t sound_command;

    while (queue_get_front(&m_sound_queue, &sound_command))
    {
        if (sound_command.sound_mode == SOUNDMODE_SOUND)
        {
            sound_t *sound = &sound_command.sound;

            if (sound->index == -1)
            {
                if (sound->channel >= 0 && sound->channel <= CHANNEL_COUNT)
                    m_channels[sound->channel].sound_mode = SOUNDMODE_NONE;
                continue;
            }
            else if (sound->index == -2)
                continue;
            else if (sound->channel == -2)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    soundstate_t *channel = &m_channels[i];

                    if (channel->sound_index == sound->index)
                        channel->sound_mode = SOUNDMODE_NONE;
                }
                continue;
            }
            else if (sound->channel == -1)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    if (m_channels[i].sound_mode == SOUNDMODE_NONE)
                    {
                        sound->channel = i;
                        break;
                    }
                }
            }
            if (sound->channel >= 0 && sound->channel < CHANNEL_COUNT && sound->index >= 0 && sound->index <= SOUND_COUNT)
            {
                soundstate_t *channel = &m_channels[sound->channel];
                uint8_t speed = m_memory[MEMORY_SFX + 68 * sound->index + 64 + 1];
                int sample_per_tick = (SAMPLE_RATE / 128) * (speed + 1);
                channel->sound_mode = SOUNDMODE_SOUND;
                channel->sound_index = sound->index;
                channel->end = sound->end;
                channel->sample = sound->start;
                channel->position = sound->start * sample_per_tick;
            }
        }
        else if (sound_command.sound_mode == SOUNDMODE_MUSIC)
        {
            music_t *music = &sound_command.music;

            if (music->index == -1)
            {
                for (int i = 0; i < CHANNEL_COUNT; i++)
                    m_channels[i].sound_mode = SOUNDMODE_NONE;
            }
            else
            {
                m_music_state.pattern = music->index;
                m_music_state.channel_mask = music->mask;
                for (int i = 0; i < CHANNEL_COUNT; i++)
                {
                    uint8_t channel_data = m_memory[MEMORY_MUSIC + 4 * m_music_state.pattern + i];
                    bool enabled = (channel_data & (1 << 6)) == 0;
                    if (enabled)
                    {
                        m_channels[i].sound_mode = SOUNDMODE_MUSIC;
                        m_channels[i].sound_index = channel_data & 0x7F;
                        m_channels[i].sample = 0;
                        m_channels[i].position = 0;
                        m_channels[i].end = 31;
                    }
                    else
                    {
                        m_channels[i].sound_mode = SOUNDMODE_NONE;
                    }
                }
            }
        }
    }

    pthread_mutex_unlock(&m_sound_queue_mutex);
}

float get_frequency(int pitch)
{
    return m_tone_frequencies[pitch % 12] / 2 * (1 << (pitch / 12));
}

void render_sound(int waveform, int pitch, int volume, int position, int offset, int length, int16_t *buffer)
{
    int16_t amplitude = (int16_t)((MAX_VOLUME / 8) * volume);
    unsigned int frequency = (unsigned int)get_frequency(pitch);
    switch (waveform)
    {
    case WAVEFORM_TRIANGLE:
        dsp_triangle_wave(frequency, amplitude, 0, position, offset, length, buffer);
        break;
    case WAVEFORM_TILTEDSAW:
        dsp_tilted_sawtooth_wave(frequency, amplitude, 0, 0.85f, position, offset, length, buffer);
        break;
    case WAVEFORM_SAW:
        dsp_sawtooth_wave(frequency, amplitude, 0, position, offset, length, buffer);
        break;
    case WAVEFORM_SQUARE:
        dsp_square_wave(frequency, amplitude, 0, position, offset, length, buffer);
        break;
    case WAVEFORM_PULSE:
        dsp_pulse_wave(frequency, amplitude, 0, 1.0f / 3.0f, position, offset, length, buffer);
        break;
    case WAVEFORM_ORGAN:
        dsp_organ_wave(frequency, amplitude, 0, 0.5f, position, offset, length, buffer);
        break;
    case WAVEFORM_NOISE:
        dsp_noise(frequency, amplitude, position, offset, length, buffer);
        break;
    case WAVEFORM_PHASER:
        break;
    }
}

void render_sounds(int16_t *buffer, int total_samples)
{
    update_sound_queue();

    memset(buffer, 0, sizeof(int16_t) * total_samples);

    for (int i = 0; i < CHANNEL_COUNT; i++)
    {
        soundstate_t *channel = &m_channels[i];

        if ((channel->sound_mode == SOUNDMODE_MUSIC && m_music_enabled) || (channel->sound_mode == SOUNDMODE_SOUND && m_sound_enabled))
        {
            // uint8_t editor_mode = m_memory[MEMORY_SFX + 68 * channel->sound_index + 64];
            uint8_t speed = m_memory[MEMORY_SFX + 68 * channel->sound_index + 64 + 1];
            // uint8_t loop_start = m_memory[MEMORY_SFX + 68 * channel->sound_index + 64 + 2];
            // uint8_t loop_end = m_memory[MEMORY_SFX + 68 * channel->sound_index + 64 + 3];
            int sample_per_tick = (SAMPLE_RATE / 128) * (speed + 1);
            int index = 0;

            while (index < total_samples)
            {
                uint8_t data_lo = m_memory[MEMORY_SFX + 68 * channel->sound_index + channel->sample * 2];
                uint8_t data_hi = m_memory[MEMORY_SFX + 68 * channel->sound_index + channel->sample * 2 + 1];
                uint16_t data = (uint16_t)((data_hi << 8) | data_lo);
                // bool use_sfx = data & 0x8000;
                //  uint8_t effect = (data & EFFECT_MASK) >> EFFECT_SHIFT;
                uint8_t volume = (data & VOLUME_MASK) >> VOLUME_SHIFT;
                uint8_t waveform = (data & WAVEFORM_MASK) >> WAVEFORM_SHIFT;
                uint8_t pitch = data & PITCH_MASK;

                int length = MIN(total_samples - index, sample_per_tick - (channel->position % sample_per_tick));

                render_sound(waveform, pitch, volume, channel->position, index, length, buffer);

                index += length;
                channel->position += length;
                channel->sample = channel->position / sample_per_tick;

                update_channel(channel);
            }
        }
    }
}
