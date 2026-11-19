# loudmax-lv2

An LV2 wrapper for the [LoudMax](https://loudmax.blogspot.com) loudness
maximizer by Thomas Mundt. It lets LV2-only hosts load the Linux LADSPA
build of LoudMax.

> **This repository does not contain LoudMax.** LoudMax is freeware but may
> not be redistributed. Download it yourself from
> <https://loudmax.blogspot.com>. This project only contains a small adapter
> that loads your copy at runtime.

## How it works

`loudmax_lv2.so` is a tiny LV2 plugin. It opens the LADSPA binary
`la_LoudMax64.so` with `dlopen()` and forwards every LV2 call to the
matching LADSPA call. Port numbers are the same in both, so nothing is
translated. The audio processing is done entirely by the unmodified LoudMax
binary.

The wrapper looks for `la_LoudMax64.so` in these places, in order:

1. the LV2 bundle directory (`~/.lv2/loudmax-ladspa.lv2/`)
2. each directory in `$LADSPA_PATH`
3. `~/.ladspa`
4. `/usr/local/lib/ladspa`, `/usr/local/lib64/ladspa`, `/usr/lib/ladspa`, `/usr/lib64/ladspa`

## Plugins

| Name           | URI                         | Audio I/O |
|----------------|-----------------------------|-----------|
| LoudMax Mono   | `urn:ladspa-wrap:loudmax:0` | 1 → 1     |
| LoudMax Stereo | `urn:ladspa-wrap:loudmax:1` | 2 → 2     |
| LoudMax 5.1    | `urn:ladspa-wrap:loudmax:2` | 6 → 6     |

Controls:

- **Threshold** and **Output**, in dB from -30 to 0.
- **Inter-Sample Peaks**, an on/off toggle.
- **Link Mode** (5.1 only), a dropdown. The options are explained in the
  LoudMax README.

The plugin's latency is reported to the host, so hosts with delay
compensation keep tracks aligned.

## Requirements

- Linux x86_64
- A C compiler, `make`, and `python3` to generate the `.ttl` files
- LoudMax for Linux (LADSPA), version 1.47 tested

No LV2 or LADSPA development packages are needed. The LADSPA header is
included in `third_party/ladspa/` under its LGPL-2.1 licence.

## Build and install

1. Install LoudMax as its own README describes. For example, copy
   `la_LoudMax64.so` to `~/.ladspa/` or `/usr/lib/ladspa/`.
2. Build and install the wrapper into `~/.lv2`:

   ```sh
   make
   make install
   ```

   To install somewhere else, set `DEST`, e.g. `make install DEST=/usr/lib64/lv2`.

   To make the bundle self-contained, copy LoudMax into it:

   ```sh
   make install LOUDMAX_SO=/path/to/la_LoudMax64.so
   ```

3. Rescan plugins in your host.

To remove it, run `make uninstall`.

## Testing

```sh
make test       # runs audio through the stereo plugin; needs LoudMax to be installed
make validate   # checks the generated Turtle syntax (needs serdi)
```

The test sends a sine wave peaking at -0.9 dBFS through the plugin, with
the threshold at -12 dB and the output at -1 dB. It then checks that the
output peak is about -1 dBFS and that a latency is reported.

## Licence

- The wrapper code is under the MIT licence; see [LICENSE](LICENSE).
- `third_party/ladspa/ladspa.h` is under LGPL-2.1; see
  [third_party/ladspa/LICENSE](third_party/ladspa/LICENSE).
- LoudMax is © Thomas Mundt. It is not included here, and this project is
  not affiliated with or endorsed by its author.
