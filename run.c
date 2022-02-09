#include "ca.h"

osc_p lfo, mel;
del_p d;
float dur = 0.2;
float p = 0.5;

void reload() {
  printf("reload...\n");

  num_bus(0);
  num_osc(4);
  num_buf(1);
  num_env(0);
  num_del(1);
  num_seq(0);

  print_sizes();

  lfo = osc(0);
  lfo->freq = 16;

  mel = osc(1);

  buf_resize(buf(0), 1.0);
  d = del(0);
  d->buf = buf(0);
}

void run() {
  LOOP {
    bus_t b = {0, 0};

    mel->freq = scale(osc_upd(lfo), -1, 1, 220, 880);

    if (s->T % sec2samp(dur) == 0) {
      float f[] = {1, 2, 4, 8, 16, 32, 64};
      lfo->freq = choose(f);

      wave_t waves[] = {SINE, SAW, PULSE, TRI};
      mel->wave = choose(waves);
      lfo->wave = choose(waves);
      p = frand(0, 1);
    }

    out(pan(p, osc_upd(mel)));
    if (s->T % sec2samp(1) < sec2samp(0.5))
      out(del_upd(d, get()));
  }
}
