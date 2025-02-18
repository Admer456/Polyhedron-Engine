/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// snd_dma.c -- main control for any streaming sound output device

#include "sound.h"

dma_t       dma;

cvar_t      *s_khz;
cvar_t      *s_testsound;
#if USE_DSOUND
static cvar_t       *s_direct;
#endif
static cvar_t       *s_mixahead;

static snddmaAPI_t snddma;

void DMA_SoundInfo(void)
{
    Com_Printf("%5d channels\n", dma.channels);
    Com_Printf("%5d samples\n", dma.samples);
    Com_Printf("%5d samplepos\n", dma.samplepos);
    Com_Printf("%5d samplebits\n", dma.samplebits);
    Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
    Com_Printf("%5d speed\n", dma.speed);
    Com_Printf("%p dma buffer\n", dma.buffer);
}

qboolean DMA_Init(void)
{
    sndinitstat_t ret = SIS_FAILURE;

	Com_WPrintf("WARNING: Sound is running in software mode!\nPlease set sound renderer to OpenAL for optimal experience!\n");

    s_khz = Cvar_Get("s_khz", "44", CVAR_ARCHIVE | CVAR_SOUND);
    s_mixahead = Cvar_Get("s_mixahead", "0.1", CVAR_ARCHIVE);
    s_testsound = Cvar_Get("s_testsound", "0", 0);

#if USE_DSOUND
    s_direct = Cvar_Get("s_direct", "1", CVAR_SOUND);
    if (s_direct->integer) {
        DS_FillAPI(&snddma);
        ret = snddma.Init();
        if (ret != SIS_SUCCESS) {
            Cvar_Set("s_direct", "0");
        }
    }
#endif
    if (ret != SIS_SUCCESS) {
        WAVE_FillAPI(&snddma);
        ret = snddma.Init();
        if (ret != SIS_SUCCESS) {
            return false;
        }
    }

    S_InitScaletable();

    s_numchannels = MAX_CHANNELS;

    Com_Printf("sound sampling rate: %i\n", dma.speed);

    return true;
}

void DMA_Shutdown(void)
{
    snddma.Shutdown();
    s_numchannels = 0;
}

void DMA_Activate(void)
{
    if (snddma.Activate) {
        S_StopAllSounds();
        snddma.Activate(s_active);
    }
}

int DMA_DriftBeginofs(float timeofs)
{
    static int  s_beginofs;
    int         start;

    // drift s_beginofs
    start = cl.serverTime * 0.001 * dma.speed + s_beginofs;
    if (start < paintedtime) {
        start = paintedtime;
        s_beginofs = start - (cl.serverTime * 0.001 * dma.speed);
    } else if (start > paintedtime + 0.3 * dma.speed) {
        start = paintedtime + 0.1 * dma.speed;
        s_beginofs = start - (cl.serverTime * 0.001 * dma.speed);
    } else {
        s_beginofs -= 10;
    }

    return timeofs ? start + timeofs * dma.speed : paintedtime;
}

void DMA_ClearBuffer(void)
{
    int     clear;

    if (dma.samplebits == 8)
        clear = 0x80;
    else
        clear = 0;

    snddma.BeginPainting();
    if (dma.buffer)
        memset(dma.buffer, clear, dma.samples * dma.samplebits / 8);
    snddma.Submit();
}

static int DMA_GetTime(void)
{
    static  int     buffers;
    static  int     oldsamplepos;
    int fullsamples = dma.samples / dma.channels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
    if (dma.samplepos < oldsamplepos) {
        buffers++;                  // buffer wrapped
        if (paintedtime > 0x40000000) {
            // time to chop things off to avoid 32 bit limits
            buffers = 0;
            paintedtime = fullsamples;
            S_StopAllSounds();
        }
    }
    oldsamplepos = dma.samplepos;

    return buffers * fullsamples + dma.samplepos / dma.channels;
}

void DMA_Update(void)
{
    int soundtime, endTime;
    int samps;

    snddma.BeginPainting();

    if (!dma.buffer)
        return;

// Updates DMA time
    soundtime = DMA_GetTime();

// check to make sure that we haven't overshot
    if (paintedtime < soundtime) {
        Com_DPrintf("S_Update_ : overflow\n");
        paintedtime = soundtime;
    }

// mix ahead of current position
    endTime = soundtime + s_mixahead->value * dma.speed;
//endTime = (soundtime + 4096) & ~4095;

    // mix to an even submission block size
    endTime = (endTime + dma.submission_chunk - 1)
              & ~(dma.submission_chunk - 1);
    samps = dma.samples >> (dma.channels - 1);
    if (endTime - soundtime > samps)
        endTime = soundtime + samps;

    S_PaintChannels(endTime);

    snddma.Submit();
}


