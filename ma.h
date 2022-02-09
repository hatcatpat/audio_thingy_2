#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void perform(float *out_l, float *out_r, const float *in_l, const float *in_r,
             uint frames);

#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"
#define FORMAT ma_format_f32
#define CHANNELS 2
ma_device device;
ma_decoder_config decoder_config;

static void callback(ma_device *dev, void *out, const void *in,
                     ma_uint32 frames) {
  memset(out, 0, frames * 2 * sizeof(float));
  perform(out, NULL, NULL, NULL, frames);
}

int audio_init() {
  ma_device_config config = ma_device_config_init(ma_device_type_playback);

  //// cant get duplex to work with pulse :(
  // ma_device_config config = ma_device_config_init(ma_device_type_duplex);
  // config.capture.pDeviceID = NULL;
  // config.capture.format = FORMAT;
  // config.capture.channels = CHANNELS;
  // config.capture.shareMode = ma_share_mode_shared;

  config.playback.pDeviceID = NULL;
  config.playback.format = FORMAT;
  config.playback.channels = CHANNELS;
  config.sampleRate = 0; // use default

  config.dataCallback = callback;

  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    fprintf(stderr, "[ma] init audio: failed!\n");
    return -1;
  }

  decoder_config = ma_decoder_config_init(
      device.playback.format, device.playback.channels, device.sampleRate);

  printf("[ma] init audio: successful!\n");
  return device.sampleRate;
}

int audio_start() {
  ma_device_start(&device);
  return 0;
}

int audio_cleanup() {
  ma_device_stop(&device);
  ma_device_uninit(&device);

  printf("cleaned up audio\n");
  return 0;
}
