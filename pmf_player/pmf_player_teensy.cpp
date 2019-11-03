//============================================================================
// PMF Player
//
// Copyright (c) 2019, Profoundic Technologies, Inc.
// All rights reserved.
//----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Profoundic Technologies nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL PROFOUNDIC TECHNOLOGIES BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#include "pmf_player.h"
#if defined(CORE_TEENSY)
#include "pmf_data.h"
// config
//#define PFC_USE_AUDIO_SHIELD_SGTL5000 // enable playback through SGTL5000-based audio shield
//---------------------------------------------------------------------------

#ifdef PFC_USE_AUDIO_SHIELD_SGTL5000
#include "output_i2s.h"
#include "control_sgtl5000.h"
//---------------------------------------------------------------------------

//===========================================================================
// mod_stream
//===========================================================================
static pmf_audio_buffer<int16_t, 2048> s_audio_buffer;
class mod_stream: public AudioStream
{
public:
  // construction
  mod_stream() :AudioStream(0,0) {}
  //-------------------------------------------------------------------------

private:
  virtual void update()
  {
    audio_block_t *block=allocate();
    int16_t *data=block->data;
    for(unsigned i=0; i<AUDIO_BLOCK_SAMPLES; ++i)
    {
      uint16_t v=s_audio_buffer.read_sample<uint16_t, 12>();
      data[i]=(v<<4)-32768;
    }
    transmit(block, 0);
    release(block);
  }
};
//---------------------------------------------------------------------------

static mod_stream s_mod_stream;
static AudioOutputI2S s_dac;
static AudioConnection s_c1(s_mod_stream, 0, s_dac, 0);
static AudioConnection s_c2(s_mod_stream, 0, s_dac, 1);
static AudioControlSGTL5000 s_sgtl5000;
//---------------------------------------------------------------------------

//===========================================================================
// pmf_player
//===========================================================================
uint32_t pmf_player::get_sampling_freq(uint32_t sampling_freq_)
{
  return uint32_t(AUDIO_SAMPLE_RATE_EXACT+0.5f);
}
//----

void pmf_player::start_playback(uint32_t sampling_freq_)
{
  // setup
  AudioMemory(2);
  s_sgtl5000.enable();
  s_sgtl5000.volume(0.6);
  s_audio_buffer.reset();
}
//----

void pmf_player::stop_playback()
{
}
//----

void pmf_player::mix_buffer(pmf_mixer_buffer &buf_, unsigned num_samples_)
{
  mix_buffer_impl(buf_, num_samples_);
}
//----

pmf_mixer_buffer pmf_player::get_mixer_buffer()
{
  return s_audio_buffer.get_mixer_buffer();
}
//---------------------------------------------------------------------------

#else // PFC_USE_AUDIO_SHIELD_SGTL5000
//===========================================================================
// pmf_player
//===========================================================================
static pmf_audio_buffer<int16_t, 2048> s_audio_buffer;
static IntervalTimer s_int_timer;
//----

void playback_interrupt()
{
  uint16_t smp=s_audio_buffer.read_sample<uint16_t, 12>();
  analogWriteDAC0(smp);
}
//----

uint32_t pmf_player::get_sampling_freq(uint32_t sampling_freq_)
{
  // round to the closest frequency representable by the bus
  float us=1000000.0f/sampling_freq_;
  uint32_t cycles=us*(F_BUS/1000000)-0.5f;
  float freq=1000000.0f*(F_BUS/1000000)/float(cycles);
  return uint32_t(freq+0.5f);
}
//----

void pmf_player::start_playback(uint32_t sampling_freq_)
{
  // enable playback interrupt at given frequency
  s_audio_buffer.reset();
  s_int_timer.begin(playback_interrupt, 1000000.0f/sampling_freq_);
}
//----

void pmf_player::stop_playback()
{
  s_int_timer.end();
}
//----

void pmf_player::mix_buffer(pmf_mixer_buffer &buf_, unsigned num_samples_)
{
  mix_buffer_impl(buf_, num_samples_);
}
//----

pmf_mixer_buffer pmf_player::get_mixer_buffer()
{
  return s_audio_buffer.get_mixer_buffer();
}
//---------------------------------------------------------------------------
#endif

//===========================================================================
#endif // CORE_TEENSY
