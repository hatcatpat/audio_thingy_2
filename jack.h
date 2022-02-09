#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int uint;
typedef unsigned char chan;

void perform(float *out_l, float *out_r, const float *in_l, const float *in_r,
             uint frames);

#include <jack/jack.h>

jack_client_t *client;
jack_port_t *port_in[2], *port_out[2];
const float *buf_in[2];
float *buf_out[2];

static int callback(jack_nframes_t frames, void *arg) {
  for (int c = 0; c < 2; ++c) {
    buf_in[c] = jack_port_get_buffer(port_in[c], frames);
    buf_out[c] = jack_port_get_buffer(port_out[c], frames);
    memset(buf_out[c], 0, frames * sizeof(float));
  }

  perform(buf_out[0], buf_out[1], buf_in[0], buf_in[1], frames);

  return 0;
}

int audio_cleanup();
void jack_shutdown(void *arg) { audio_cleanup(); }

int audio_init() {
  const char *client_name = "ca";
  const char *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;

  client = jack_client_open(client_name, options, &status, server_name);

  if (!client) {
    fprintf(stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
    if (status & JackServerFailed)
      fprintf(stderr, "Unable to connect to JACK server\n");

    return -1;
  }

  if (status & JackServerStarted)
    fprintf(stderr, "JACK server started\n");

  if (status & JackNameNotUnique) {
    client_name = jack_get_client_name(client);
    fprintf(stderr, "unique name `%s' assigned\n", client_name);
  }

  jack_set_process_callback(client, callback, 0);

  jack_on_shutdown(client, jack_shutdown, 0);

  printf("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate(client));

  for (int c = 0; c < 2; ++c) {
    port_in[c] =
        jack_port_register(client, c == 0 ? "in_l" : "in_r",
                           JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    port_out[c] =
        jack_port_register(client, c == 0 ? "out_l" : "out_r",
                           JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (!port_in[c] || !port_out[c]) {
      fprintf(stderr, "no more JACK ports available\n");
      return -1;
    }
  }

  printf("init audio jack: successful!\n");
  return jack_get_sample_rate(client);
}

int audio_start() {
  if (jack_activate(client))
    return -1;

  const char **ports;

  // input
  {
    ports = jack_get_ports(client, NULL, NULL,
                           JackPortIsPhysical | JackPortIsOutput);
    if (!ports) {
      fprintf(stderr, "no physical capture ports\n");
      return -1;
    }

    for (int c = 0; c < 2; ++c)
      if (jack_connect(client, ports[c], jack_port_name(port_in[c])))
        fprintf(stderr, "cannot connect input port %i\n", c);

    free(ports);
  }

  // output
  {
    ports = jack_get_ports(client, NULL, NULL,
                           JackPortIsPhysical | JackPortIsInput);
    if (!ports) {
      fprintf(stderr, "no physical playback ports\n");
      return -1;
    }

    for (int c = 0; c < 2; ++c)
      if (jack_connect(client, jack_port_name(port_out[c]), ports[c]))
        fprintf(stderr, "cannot connect output port %i\n", c);

    free(ports);
  }

  return 0;
}

int audio_cleanup() {
  jack_client_close(client);

  printf("cleaned up audio\n");
  return 0;
}
