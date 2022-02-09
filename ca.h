#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846264338327950288419716939937510582
#define TAU 6.283185307179586476925286766559005768394338798750211642
#define PI2 0.636619772367581343075535053490057448137838582961825795

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)
#define CLAMP(v, a, b) (v)<(a) ? (a) : (v)>(b) ? (b) : (v)
#define SQR(a) (a) * (a)
#define MOD(a, b) ((((a) % (b)) + (b)) % (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define LEN(arr) (sizeof(arr) / sizeof(*arr))

typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned char id;
typedef unsigned char chan;

#define FREE(p)                                                                \
  if (p) {                                                                     \
    free(p);                                                                   \
    p = NULL;                                                                  \
  }

#define APPLY(OBJ, FUNC)                                                       \
  for (chan c = 0; c < 2; ++c) {                                               \
    float v = OBJ[c];                                                          \
    v = FUNC;                                                                  \
    OBJ[c] = v;                                                                \
  }

float frand(float lo, float hi) {
  return ((float)rand() / (float)RAND_MAX) * (hi - lo) + lo;
}
float norm_rand() { return frand(0.0, 1.0); }
float bi_rand() { return frand(-1.0, 1.0); }

float scale(float x, float inlo, float inhi, float outlo, float outhi) {
  return (x - inlo) * (outhi - outlo) / (inhi - inlo) + outlo;
}

float bi2norm(float b) { return (b + 1.0) * 0.5; }
float norm2bi(float n) { return n * 2.0 - 1.0; }

float norm_scale(float n, float lo, float hi) { return n * (hi - lo) + lo; }
float bi_scale(float b, float lo, float hi) {
  return norm_scale(bi2norm(b), lo, hi);
}

typedef enum { SINE, SAW, PULSE, TRI, NUM_WAVES } wave_t;
float sine(float p) { return cos(p * TAU); }
float saw(float p) { return p; }
float pulse(float p, float w) { return p < w ? -1.0 : 1.0; }
float tri(float p) { return p < 0.5 ? p / 0.5 : 1 - (p - 0.5) / 0.5; }

#define OBJ_GET(OBJ)                                                           \
  if (s->num_##OBJ - 1 < i)                                                    \
    num_##OBJ(i + 1);                                                          \
  return &s->OBJ[i];

#define OBJ_NUM(OBJ, DESTRUCTOR)                                               \
  if (num == s->num_##OBJ)                                                     \
    return;                                                                    \
  if (num == 0) {                                                              \
    for (int i = 0; i < s->num_##OBJ; ++i)                                     \
      DESTRUCTOR;                                                              \
    FREE(s->OBJ);                                                              \
    s->num_##OBJ = 0;                                                          \
    return;                                                                    \
  }                                                                            \
  num = MIN(num, OBJ##_max);                                                   \
  uint8 num_ = s->num_##OBJ;                                                   \
  s->num_##OBJ = num;                                                          \
  for (int i = num; i < num_; ++i)                                             \
    DESTRUCTOR;                                                                \
  if (s->OBJ)                                                                  \
    s->OBJ = realloc(s->OBJ, num * sizeof(OBJ##_t));                           \
  else                                                                         \
    s->OBJ = calloc(num, sizeof(OBJ##_t));                                     \
  if (num_ < num)                                                              \
    for (int i = num_; i < num; ++i)                                           \
      OBJ##_init(OBJ(i));

#define OBJ_MAX 64

#define bus_max OBJ_MAX
typedef struct {
  float value[2];
} bus_t;
typedef bus_t *bus_p;
bus_t *bus(id i);
void num_bus(uint8 num);
void bus_init(bus_t *b);
void bus_print(bus_t *b);
void bus_set(bus_t *b, bus_t v);
void bus_set1(bus_t *b, chan c, float v);
void bus_set2(bus_t *b, float v);
bus_t bus_zero() { return (bus_t){0, 0}; }
bus_t bus_one() { return (bus_t){1, 1}; }

#define BUS_BINOP(NAME, OP)                                                    \
  void bus_##NAME(bus_t *b, bus_t *c) {                                        \
    if (!b || !c)                                                              \
      return;                                                                  \
    b->value[0] = b->value[0] OP(c->value[0]);                                 \
    b->value[1] = b->value[1] OP(c->value[1]);                                 \
  }                                                                            \
  void bus_##NAME##_s(bus_t *b, float v) {                                     \
    if (!b)                                                                    \
      return;                                                                  \
    b->value[0] = b->value[0] OP v;                                            \
    b->value[1] = b->value[1] OP v;                                            \
  }
BUS_BINOP(add, +)
BUS_BINOP(sub, -)
BUS_BINOP(mul, *)
BUS_BINOP(div, /)

#define osc_max OBJ_MAX
typedef struct {
  wave_t wave;
  float freq, width, phase, value;
} osc_t;
typedef osc_t *osc_p;
osc_t *osc(id i);
void num_osc(uint8 num);
void osc_init(osc_t *o);
void osc_print(osc_t *o);
void osc_set(osc_t *o, float freq, float width);
float osc_upd(osc_t *o);

#define env_max OBJ_MAX
typedef struct {
  uint phase;
  float value, attack, release;
  bool active;
} env_t;
typedef env_t *env_p;
env_t *env(id i);
void num_env(uint8 num);
void env_init(env_t *e);
void env_print(env_t *e);
void env_set(env_t *e, float attack, float release);
void env_trig(env_t *e);
float env_upd(env_t *e);

#define buf_max OBJ_MAX
typedef struct {
  float *data;
  uint length;
} buf_t;
typedef buf_t *buf_p;
buf_t *buf(id i);
void num_buf(uint8 num);
void buf_init(buf_t *b);
void buf_print(buf_t *b);
void buf_resize(buf_t *b, float d);
float buf_read1(buf_t *b, uint p, chan c);
bus_t buf_read(buf_t *b, uint p);
void buf_write1(buf_t *b, uint p, chan c, float v);
void buf_write(buf_t *b, uint p, bus_t *v);

#define del_max OBJ_MAX
typedef struct {
  buf_t *buf;
  bus_t bus;
  uint write;
  int read;
  bool record;
  float delay, feedback;
} del_t;
typedef del_t *del_p;
del_t *del(id i);
void num_del(uint8 num);
void del_init(del_t *d);
void del_print(del_t *d);
void del_set(del_t *d, buf_t *buf, float delay, float feedback);
void del_upd_(del_t *d, buf_t *b);
float del_upd1(del_t *d, chan c, float v);
bus_t del_upd(del_t *d, bus_t v);

#define seq_max OBJ_MAX
typedef struct {
  void *data;
  int step;
  uint pos, length;
} seq_t;
typedef seq_t *seq_p;
seq_t *seq(id i);
void num_seq(uint8 num);
void seq_init(seq_t *q);
void seq_print(seq_t *q);
void seq_set(seq_t *q, void *data, uint length);
void seq_upd_(seq_t *q);
float seq_upd_f(seq_t *q);
int seq_upd_i(seq_t *q);

#define fil_max OBJ_MAX
typedef struct {
  float y0, y1, y2;
  float x1, x2;
  float b0, b1, b2;
  float a0, a1, a2;
  float freq, res, gain;
} fil_t;
typedef fil_t *fil_p;
fil_t *fil(id i);
void num_fil(uint8 num);
void fil_init(fil_t *f);
void fil_print(fil_t *f);
void fil_set(fil_t *f, void *data, uint length);
void fil_upd_(fil_t *f);

typedef struct {
  bus_t *bus;
  osc_t *osc;
  env_t *env;
  buf_t *buf;
  del_t *del;
  seq_t *seq;
  uint num_bus, num_osc, num_env, num_buf, num_del, num_seq;
  float *out[2];
  const float *in[2];
  double rate, inv_rate, t;
  uint frames, pos, T;
} S;

S *s;
void update(S *s_) { s = s_; }

void print_sizes() {
  printf("num_bus: %i\n", s->num_bus);
  printf("num_osc: %i\n", s->num_osc);
  printf("num_env: %i\n", s->num_env);
  printf("num_buf: %i\n", s->num_buf);
  printf("num_del: %i\n", s->num_del);
}

float wave(wave_t i, float p, float w) {
  switch (i) {
  case SINE:
    return sine(p);
  case SAW:
    return saw(p);
  case PULSE:
    return pulse(p, w);
  case TRI:
    return tri(p);
  default:
    return 0.0;
  }
}

#define LOOP                                                                   \
  for (s->pos = 0; s->pos < s->frames; ++s->pos, s->t += s->inv_rate, ++s->T)

#ifdef CA_JACK
float in_get_(chan c) { return s->in[c][s->pos]; }
float out_get_(chan c) { return s->out[c][s->pos]; }
void out_set_(chan c, float v) { s->out[c][s->pos] = v; }
#else
#ifdef CA_INPUT
float in_get_(chan c) { return s->in[0][s->pos * 2 + c]; }
#else
float in_get_(chan c) { return 0.0; }
#endif
float out_get_(chan c) { return s->out[0][s->pos * 2 + c]; }
void out_set_(chan c, float v) { s->out[0][s->pos * 2 + c] = v; }
#endif

void set1(chan c, float v) { out_set_(c, v); }
void set(bus_t v) { set1(0, v.value[0]), set1(1, v.value[1]); }
void set2(float v) { set1(0, v), set1(1, v); }

void out1(chan c, float v) { out_set_(c, out_get_(c) + v); }
void out(bus_t v) { out1(0, v.value[0]), out1(1, v.value[1]); }
void out2(float v) { out1(0, v), out1(1, v); }

float in1(chan c) { return in_get_(c); }
bus_t in() { return (bus_t){in1(0), in1(1)}; }

float get1(chan c) { return out_get_(c); }
bus_t get() { return (bus_t){get1(0), get1(1)}; }

int sec2samp(float v) { return (int)floor(v * s->rate); }
float samp2sec(uint v) { return (float)v * s->inv_rate; }

bus_t flip(bus_t *v) { return (bus_t){v->value[1], v->value[0]}; }

bus_t fill(float a) { return (bus_t){a, a}; }

bus_t pan(float p, float v) { return (bus_t){v * (1 - p), v * p}; }

bool chance(float p) { return frand(0.0, 1.0) < p; }

#define choose(f) f[rand() % LEN(f)]

// bus
bus_t *bus(id i) { OBJ_GET(bus); }
void num_bus(uint8 num) { OBJ_NUM(bus, {}); }

void bus_init(bus_t *b) {
  if (!b)
    return;

  *b = (bus_t){0.0, 0.0};
}

void bus_print(bus_t *b) {
  if (!b)
    return;

  printf("[%f, %f]\n", b->value[0], b->value[1]);
}

void bus_set(bus_t *b, bus_t v) {
  if (!b)
    return;

  *b = v;
}
void bus_set1(bus_t *b, chan c, float v) {
  if (!b)
    return;

  b->value[c] = v;
}
void bus_set2(bus_t *b, float v) {
  if (!b)
    return;

  *b = (bus_t){v, v};
}

// osc
osc_t *osc(id i) { OBJ_GET(osc); }
void num_osc(uint8 num) { OBJ_NUM(osc, {}); }

void osc_init(osc_t *o) {
  if (!o)
    return;

  o->wave = SINE;
  o->phase = o->value = 0.0;
  o->freq = 440;
  o->width = 0.5;
}

void osc_print(osc_t *o) {
  if (!o)
    return;

  printf("wave %i, value %f, phase %f, freq %f\n", o->wave, o->value, o->phase,
         o->freq);
}

void osc_set(osc_t *o, float freq, float width) {
  if (!o)
    return;

  o->freq = freq, o->width = width;
}

float osc_upd(osc_t *o) {
  if (!o)
    return 0.0;

  o->value = wave(o->wave, o->phase, o->width);

  o->phase += o->freq * s->inv_rate;
  while (o->phase >= 1.0)
    o->phase -= 1.0;

  return o->value;
}

// env
env_t *env(id i) { OBJ_GET(env); }
void num_env(uint8 num) { OBJ_NUM(env, {}); }

void env_init(env_t *e) {
  if (!e)
    return;
  e->phase = 0;
  e->attack = e->value = 0.0;
  e->release = 0.1;
  e->active = false;
}

void env_print(env_t *e) {
  if (!e)
    return;

  printf("value %f, phase %i, attack %f, release %f, active %i\n", e->value,
         e->phase, e->attack, e->release, e->active);
}

void env_set(env_t *e, float attack, float release) {
  if (!e)
    return;

  e->attack = attack, e->release = release;
}

float env_upd(env_t *e) {
  if (!e)
    return 0.0;

  if (e->active) {
    uint attack = sec2samp(e->attack), release = sec2samp(e->release);

    if (e->phase < attack + release) {

      if (attack && e->phase < attack)
        e->value = (float)e->phase / attack;
      else
        e->value = 1.0 - (float)(e->phase - attack) / release;

      e->value = SQR(SQR(e->value));

      ++e->phase;
    } else {
      e->phase = 0;
      e->active = 0;
    }
  }

  return e->value;
}

void env_trig(env_t *e) {
  if (!e)
    return;

  e->phase = 0;
  e->value = 0.0;
  e->active = 1;
}

// buf
buf_t *buf(id i) { OBJ_GET(buf); }
void num_buf(uint8 num) {
  OBJ_NUM(buf, { FREE(buf(i)->data); });
}

void buf_init(buf_t *b) {
  if (!b)
    return;

  b->length = 0;
  FREE(b->data);
}

void buf_resize(buf_t *b, float d) {
  if (!b)
    return;

  if (d) {
    uint l = sec2samp(d);
    if (b->length == l)
      return;

    if (b->data)
      b->data = realloc(b->data, 2 * l * sizeof(float));
    else
      b->data = calloc(2 * l, sizeof(float));

    if (b->data)
      b->length = l;
  } else {
    b->length = 0;
    FREE(b->data);
  }
}

void buf_print(buf_t *b) {
  if (!b)
    return;

  printf("length %i, valid %c\n", b->length / 2, b->data ? 'y' : 'n');
}

float buf_read1(buf_t *b, uint p, chan c) {
  if (!b || !b->data)
    return 0.0;

  return b->data[p * 2 + c];
}
bus_t buf_read(buf_t *b, uint p) {
  if (!b || !b->data)
    return (bus_t){0, 0};

  bus_t v = {0, 0};

  for (chan c = 0; c < 2; ++c)
    v.value[c] = buf_read1(b, p, c);

  return v;
}

void buf_write1(buf_t *b, uint p, chan c, float v) {
  if (!b || !b->data)
    return;

  b->data[p * 2 + c] = v;
}
void buf_write(buf_t *b, uint p, bus_t *v) {
  if (!b || !b->data)
    return;

  for (chan c = 0; c < 2; ++c)
    b->data[p * 2 + c] = v->value[c];
}

// del
del_t *del(id i) { OBJ_GET(del); }
void num_del(uint8 num) { OBJ_NUM(del, {}); }

void del_init(del_t *d) {
  if (!d)
    return;

  d->buf = NULL;
  d->write = d->read = 0;
  d->delay = 0;
  d->record = 1;
  d->feedback = 0.5;
  bus_set2(&d->bus, 0.0);
}

void del_print(del_t *d) {
  if (!d)
    return;

  printf("buf %p, write %i, read %i, delay %f, feedback %f, value ",
         (void *)d->buf, d->write, d->read, d->delay, d->feedback);
  bus_print(&d->bus);
}

void del_set(del_t *d, buf_t *buf, float delay, float feedback) {
  if (!d)
    return;

  d->buf = buf, d->delay = delay, d->feedback = feedback;
}

void del_upd_(del_t *d, buf_t *b) {
  if (d->write > b->length - 1)
    d->write = 0;

  d->read = (d->write - sec2samp(d->delay)) % b->length;
  while (d->read > b->length - 1)
    d->read -= b->length;
  while (d->read < 0)
    d->read += b->length;
}

float del_upd1(del_t *d, chan c, float v) {
  if (!d || !d->buf || !d->buf->data)
    return 0.0;

  del_upd_(d, d->buf);

  d->bus.value[c] = buf_read1(d->buf, d->read, c);
  if (d->record)
    buf_write1(d->buf, d->write, c, v + d->bus.value[c] * d->feedback);

  ++d->write;

  return d->bus.value[c];
}
bus_t del_upd(del_t *d, bus_t v) {
  if (!d || !d->buf || !d->buf->data)
    return bus_zero();

  del_upd_(d, d->buf);

  for (chan c = 0; c < 2; ++c) {
    d->bus.value[c] = buf_read1(d->buf, d->read, c);
    if (d->record)
      buf_write1(d->buf, d->write, c,
                 v.value[c] + d->bus.value[c] * d->feedback);
  }

  ++d->write;

  return d->bus;
}

// seq
seq_t *seq(id i) { OBJ_GET(seq); }
void num_seq(uint8 num) { OBJ_NUM(seq, {}); }

void seq_init(seq_t *q) {
  if (!q)
    return;
  q->data = NULL;
  q->pos = 0;
  q->length = 0;
  q->step = 1;
}

void seq_print(seq_t *q) {
  if (!q)
    return;
  printf("data %c, %i pos, %i length, %i step\n", q->data ? 'y' : 'n', q->pos,
         q->length, q->step);
}

void seq_set(seq_t *q, void *data, uint length) {
  if (!q)
    return;
  q->data = data;
  if (q->length != length) {
    q->length = length;
    q->pos %= q->length;
    if (q->pos < 0)
      q->pos += q->length;
  }
}

void seq_upd_(seq_t *q) {
  q->pos = (q->pos + q->step) % q->length;
  if (q->pos < 0)
    q->pos += q->length - 1;
}

#define SEQ_UPD(TYPE, DEFAULT)                                                 \
  TYPE seq_upd_##TYPE(seq_t *q) {                                              \
    if (!q || !q->data)                                                        \
      return DEFAULT;                                                          \
    TYPE v = (TYPE)((TYPE *)q->data)[q->pos];                                  \
    seq_upd_(q);                                                               \
    return v;                                                                  \
  }

SEQ_UPD(float, 0.0)
SEQ_UPD(int, 0)
SEQ_UPD(uint, 0)

#undef SEQ_UPD

// misc
#define PRINT(OBJ)                                                             \
  printf("\nOBJ\n");                                                           \
  for (int i = 0; i < s->num_##OBJ; ++i) {                                     \
    printf("%i: ", i);                                                         \
    OBJ##_print(OBJ(i));                                                       \
  }

void print_all() {
  PRINT(bus);
  PRINT(osc);
  PRINT(env);
  PRINT(buf);
  PRINT(del);
  PRINT(seq);
}
