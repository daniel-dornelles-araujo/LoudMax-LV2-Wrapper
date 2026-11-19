/* Smoke test: load the stereo plugin from the built bundle, push a 0.9-peak
 * sine through it with threshold -12 dB / output -1 dB, and check the output
 * ceiling and reported latency. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <dlfcn.h>

typedef struct LV2_Descriptor {
    const char *URI;
    void *(*instantiate)(const struct LV2_Descriptor *, double, const char *, const void *const *);
    void (*connect_port)(void *, uint32_t, void *);
    void (*activate)(void *);
    void (*run)(void *, uint32_t);
    void (*deactivate)(void *);
    void (*cleanup)(void *);
    const void *(*extension_data)(const char *);
} LV2_Descriptor;

#define N 4096

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s BUNDLE_DIR\n", argv[0]); return 2; }
    char so[4096];
    snprintf(so, sizeof so, "%s/loudmax_lv2.so", argv[1]);
    void *h = dlopen(so, RTLD_NOW);
    if (!h) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    const LV2_Descriptor *(*get)(uint32_t) = (const LV2_Descriptor *(*)(uint32_t))dlsym(h, "lv2_descriptor");
    const LV2_Descriptor *d = get(1);
    void *p = d->instantiate(d, 48000, argv[1], NULL);
    if (!p) { fprintf(stderr, "FAIL: instantiate (is la_LoudMax64.so installed?)\n"); return 1; }

    float thr = -12, out = -1, isp = 1, lat = -1;
    static float il[N], ir[N], ol[N], orr[N];
    for (int i = 0; i < N; i++) il[i] = ir[i] = 0.9f * sinf(i * 0.05f);
    d->connect_port(p, 0, &thr); d->connect_port(p, 1, &out);
    d->connect_port(p, 2, &isp); d->connect_port(p, 3, &lat);
    d->connect_port(p, 4, il);   d->connect_port(p, 5, ol);
    d->connect_port(p, 6, ir);   d->connect_port(p, 7, orr);

    d->activate(p);
    float pk = 0;
    for (int k = 0; k < 20; k++) {
        d->run(p, N);
        for (int i = 0; i < N; i++) if (fabsf(ol[i]) > pk) pk = fabsf(ol[i]);
    }
    d->deactivate(p);
    d->cleanup(p);

    float db = 20 * log10f(pk);
    printf("latency=%g frames, output peak=%.2f dBFS\n", lat, db);
    if (lat <= 0 || db > -0.9f || db < -1.5f) { puts("FAIL"); return 1; }
    puts("PASS");
    return 0;
}
