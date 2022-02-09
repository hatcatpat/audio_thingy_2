#include <dlfcn.h>
#include <time.h>
#include <unistd.h>

#include "ca.h"

#ifdef CA_JACK
#include "jack.h"
#else
#include "ma.h"
#endif

typedef void reload_t();
typedef void run_t();
typedef void update_t(S *);
run_t *run;
void dl_reload();
bool dl_reload_flag = 0;

void perform(float *out_l, float *out_r, const float *in_l, const float *in_r,
             uint frames) {
  s->frames = frames;
  s->out[0] = out_l, s->out[1] = out_r, s->in[0] = in_l, s->in[1] = in_r;

  if (run)
    run();

  if (dl_reload_flag)
    dl_reload();
}

int init() {
  printf("init...\n");

  int rate = audio_init();
  if (rate <= 0)
    return -1;

  s = malloc(sizeof(S));

  s->rate = (double)rate, s->inv_rate = 1.0 / s->rate;
  s->t = 0.0, s->T = 0;
  s->frames = 0;

  s->bus = NULL, s->osc = NULL, s->env = NULL, s->buf = NULL, s->del = NULL,
  s->seq = NULL;
  s->num_bus = s->num_osc = s->num_env = s->num_buf = s->num_del = s->num_seq =
      0;

  printf("init s: successful!\n");
  return 0;
}

char *lib_names[2] = {"./run0.so", "./run1.so"};
uint8 lib_curr = 0;

void build() {
#ifdef CA_JACK
  if (system("make run_jack") == -1)
    return;
#else
  if (system("make run") == -1)
    return;
#endif

  if (lib_curr == 0)
    system("cp run.so run0.so");
  else
    system("cp run.so run1.so");
}

void *lib;
void dl_reload() {
  void *lib_ = dlopen(lib_names[lib_curr], RTLD_NOW);

  if (lib_) {
    printf("dl loaded\n");

    update_t *update = (update_t *)dlsym(lib_, "update");
    if (update)
      update(s);
    else
      fprintf(stderr, "dl did not load 'update': %s\n", dlerror());

    reload_t *reload = (reload_t *)dlsym(lib_, "reload");
    if (reload)
      reload();
    else
      fprintf(stderr, "dl did not load 'reload': %s\n", dlerror());

    run_t *run_ = (run_t *)dlsym(lib_, "run");
    if (run_) {
      run = run_;
      if (lib)
        dlclose(lib);
      lib = lib_;
      lib_curr = !lib_curr;
    } else {
      fprintf(stderr, "dl did not load 'run': %s\n", dlerror());
      dlclose(lib_);
    }
  } else {
    if (lib)
      dlclose(lib);
    fprintf(stderr, "dl did not load: %s\n", dlerror());
  }

  dl_reload_flag = 0;
}

int cleanup() {
  audio_cleanup();

  while (dl_reload_flag)
    ;

  if (lib)
    dlclose(lib);

  FREE(s->bus);
  FREE(s->osc);
  FREE(s->env);
  if (s->buf) {
    for (int i = 0; i < s->num_buf; ++i)
      FREE(buf(i)->data);
    free(s->buf);
  }
  FREE(s->del);
  FREE(s->seq);

  free(s);

  system("rm run.so");
  system("rm run0.so");
  system("rm run1.so");

  printf("cleaned up\n");
  return 0;
}

int main(void) {
  srand(time(NULL));

  if (init())
    return -1;

  build();
  dl_reload();

  if (audio_start())
    return cleanup();

  while ((getc(stdin) != 'q')) {
    build();
    dl_reload_flag = 1;
  }

  return cleanup();
}
