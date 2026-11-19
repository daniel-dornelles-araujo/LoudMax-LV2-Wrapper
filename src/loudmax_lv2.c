/* loudmax-lv2: thin LV2 wrapper around the LoudMax LADSPA plugin.
 *
 * LV2 port indices are identical to the LADSPA port indices, so every call
 * is forwarded 1:1. The proprietary LADSPA binary (la_LoudMax64.so) is NOT
 * part of this project; it is located at runtime in, in order:
 *   1. the LV2 bundle directory
 *   2. each directory in $LADSPA_PATH
 *   3. ~/.ladspa, /usr/local/lib/ladspa, /usr/local/lib64/ladspa,
 *      /usr/lib/ladspa, /usr/lib64/ladspa
 *
 * SPDX-License-Identifier: MIT
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "ladspa.h"

#define LOUDMAX_SO "la_LoudMax64.so"
#define URI_BASE   "urn:ladspa-wrap:loudmax:"
#define NPLUG      3

/* Minimal LV2 core ABI (stable since LV2 1.0), so no LV2 headers are needed. */
typedef void *LV2_Handle;
typedef struct { const char *URI; void *data; } LV2_Feature;
typedef struct LV2_Descriptor {
    const char *URI;
    LV2_Handle (*instantiate)(const struct LV2_Descriptor *, double, const char *, const LV2_Feature *const *);
    void (*connect_port)(LV2_Handle, uint32_t, void *);
    void (*activate)(LV2_Handle);
    void (*run)(LV2_Handle, uint32_t);
    void (*deactivate)(LV2_Handle);
    void (*cleanup)(LV2_Handle);
    const void *(*extension_data)(const char *);
} LV2_Descriptor;

typedef struct {
    const LADSPA_Descriptor *ld;
    LADSPA_Handle h;
    void *lib;
} Wrap;

static void *try_dir(const char *dir, size_t len)
{
    char path[4096];
    if (len == 0 || len > sizeof path - sizeof LOUDMAX_SO - 2) return NULL;
    snprintf(path, sizeof path, "%.*s/%s", (int)len, dir, LOUDMAX_SO);
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

static void *open_loudmax(const char *bundle)
{
    void *lib;
    if ((lib = try_dir(bundle, strlen(bundle)))) return lib;

    const char *lp = getenv("LADSPA_PATH");
    while (lp && *lp) {
        const char *end = strchrnul(lp, ':');
        if ((lib = try_dir(lp, (size_t)(end - lp)))) return lib;
        lp = *end ? end + 1 : end;
    }

    const char *home = getenv("HOME");
    if (home) {
        char d[4096];
        snprintf(d, sizeof d, "%s/.ladspa", home);
        if ((lib = try_dir(d, strlen(d)))) return lib;
    }

    static const char *const sys[] = {
        "/usr/local/lib/ladspa", "/usr/local/lib64/ladspa",
        "/usr/lib/ladspa", "/usr/lib64/ladspa",
    };
    for (size_t i = 0; i < sizeof sys / sizeof *sys; i++)
        if ((lib = try_dir(sys[i], strlen(sys[i])))) return lib;

    fprintf(stderr, "loudmax-lv2: could not find " LOUDMAX_SO
            " (install LoudMax to ~/.ladspa or /usr/lib/ladspa, or set LADSPA_PATH)\n");
    return NULL;
}

static LV2_Handle instantiate(const LV2_Descriptor *d, double rate,
                              const char *bundle, const LV2_Feature *const *f)
{
    (void)f;
    void *lib = open_loudmax(bundle);
    if (!lib) return NULL;

    LADSPA_Descriptor_Function fn =
        (LADSPA_Descriptor_Function)dlsym(lib, "ladspa_descriptor");
    unsigned long idx = (unsigned long)(d->URI[sizeof URI_BASE - 1] - '0');
    const LADSPA_Descriptor *ld = fn ? fn(idx) : NULL;
    LADSPA_Handle h = ld ? ld->instantiate(ld, (unsigned long)rate) : NULL;
    Wrap *w = h ? calloc(1, sizeof *w) : NULL;
    if (!w) {
        if (h) ld->cleanup(h);
        dlclose(lib);
        return NULL;
    }
    w->ld = ld; w->h = h; w->lib = lib;
    return w;
}

static void connect_port(LV2_Handle p, uint32_t port, void *data)
{ Wrap *w = p; w->ld->connect_port(w->h, port, (LADSPA_Data *)data); }
static void activate(LV2_Handle p)
{ Wrap *w = p; if (w->ld->activate) w->ld->activate(w->h); }
static void run(LV2_Handle p, uint32_t n)
{ Wrap *w = p; w->ld->run(w->h, n); }
static void deactivate(LV2_Handle p)
{ Wrap *w = p; if (w->ld->deactivate) w->ld->deactivate(w->h); }
static void cleanup(LV2_Handle p)
{ Wrap *w = p; w->ld->cleanup(w->h); dlclose(w->lib); free(w); }
static const void *extension_data(const char *uri) { (void)uri; return NULL; }

#define DESC(i) { URI_BASE #i, instantiate, connect_port, activate, run, deactivate, cleanup, extension_data }
static const LV2_Descriptor descs[NPLUG] = { DESC(0), DESC(1), DESC(2) };

__attribute__((visibility("default")))
const LV2_Descriptor *lv2_descriptor(uint32_t i) { return i < NPLUG ? &descs[i] : NULL; }
