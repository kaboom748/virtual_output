# virtual_output

[![CI](https://github.com/kaboom748/virtual_output/actions/workflows/ci.yml/badge.svg)](https://github.com/kaboom748/virtual_output/actions/workflows/ci.yml)

An ESPHome `output:` platform with no pin. **Your browser is the buzzer.**

Point `rtttl:` at it instead of `esp8266_pwm` or `ledc`, open
`http://<ip>:8082/`, and the ringtone plays in a tab — next to a scope showing
exactly what the pin would have been doing. ESP8266 **and** ESP32. No extra
hardware, no speaker, no wiring.

```yaml
output:
  - platform: virtual_output
    id: buzzer
    port: 8082

rtttl:
  id: ringtone
  output: buzzer
  gain: 60%
```

## Why

Because a buzzer is a rehearsal you cannot repeat at your desk. You solder,
flash, listen, and start again. This component moves the rehearsal into a
browser tab — and, importantly, it is **as unforgiving as the real thing**. If
your ESP stutters, this stutters. If `gain: 100%` is silent on the bench, it is
silent here. A rehearsal that only reproduced the successes would be worth
nothing.

What travels on the wire is not audio. It is a timestamped transition:

```
[0xAA][flags][ts uint32 LE][freq uint16 LE][duty uint8]   = 9 bytes
```

Roughly 100 bytes per second at peak. The timestamp is taken **on the ESP**,
inside `update_frequency()` / `write_state()`, before the network can add jitter
to it. The browser replays each transition on the Web Audio sample clock, so
network jitter shifts the whole melody instead of deforming its rhythm.

## Install

```yaml
external_components:
  - source: github://kaboom748/virtual_output
    components: [virtual_output]
    refresh: 1d
```

Then browse to `http://<ip-of-your-esp>:8082/`.

## Configuration

| Option | Default | Notes |
|---|---|---|
| `id` | *required* | |
| `port` | `8082` | The only setting this component owns. |
| `inverted` | `false` | From `FLOAT_OUTPUT_SCHEMA`. |
| `min_power` / `max_power` | | From `FLOAT_OUTPUT_SCHEMA`. |
| `zero_means_zero` | `false` | From `FLOAT_OUTPUT_SCHEMA`. |

Everything but `port` comes from the base schema for free, and
`FloatOutput::set_level()` applies it **before** `write_state()` is called. The
browser therefore receives the duty cycle the pin would carry, not the one
`rtttl` asked for.

## An expected compile warning

```
WARNING Component rtttl is not known to work with the selected output type.
```

Harmless. `rtttl/__init__.py` carries a `FINAL_VALIDATE_SCHEMA` with an
allowlist, `PWM_GOOD = ["esp8266_pwm", "ledc"]`; anything else warns. Only an
upstream change removes it.

## Reading the panel

The heartbeat (10/s, continuously, whether or not music is playing) is the
metronome. Every beat is stamped **on the ESP**, before the network can touch it,
which lets the panel separate the only two things that can go wrong instead of
blaming whichever is nearer to hand:

| Line | Means |
|---|---|
| `Beat rate / target` | Must read `10.0 / 10.0`. Any sag is real. |
| `ESP loop worst` | Largest gap between beats **as the ESP lived it**. ~110 ms is healthy. |
| `Link jitter` | Arrival spread **minus** the ESP's own period. The network, and nothing else. |
| `ESP repairs` | A transition overflowed the queue and was dropped. Must stay 0. |
| `Resyncs` | Abnormal here; the ESP closes on a short write rather than shift a frame. |
| `Late events` | Play-out shorter than the link jitter. Raise the slider; `Margin min` says by how much. |
| `Duty / amplitude` | At `gain: 100%` you should read `100% / 0%` and hear nothing. |

## "My notes are held too long"

Look at **`ESP loop worst`** before anything else. If it reads several hundred
milliseconds, your ESP's main loop is freezing — a poor WiFi link is the usual
cause on ESP8266. `rtttl` derives its tempo from that same loop, so it freezes
too and holds the note.

**A real buzzer on a GPIO does exactly the same thing.** This component is not
adding the fault; it is showing it to you. Fixing it here would mean lying about
what the pin is doing.

## Moving to real hardware

Replace the `output:` block. Nothing else changes — not `rtttl:`, not `gain:`,
not your automations:

```yaml
output:
  - platform: esp8266_pwm     # or: ledc, on ESP32
    id: buzzer
    pin: D5                   # or: GPIO19
```

Moving to a **real speaker** (`rtttl: speaker:`) is *not* this migration. That is
rtttl's other mode — 16 kHz PCM instead of a PWM square — and it drops ESP8266.
This component does not cover it.

## Design notes

- **No undefined behaviour.** `output::FloatOutput` is a public API, and this
  component owns its own state. Nothing is reached by casting to a class the
  object is not an instance of.
- **One socket path for both targets.** ESPHome's `socket` component covers
  ESP8266 through its `lwip_tcp` implementation, so there is no
  `WiFiServer` / `socket::Socket` split. The only `#ifdef` left picks a string for
  the `X-Platform` header. Note that on `lwip_tcp`, `ListenSocket` and `Socket`
  are distinct types: `socket_ip()` cannot `listen()` there.
- **A queue, not a coalesced state.** For two identical consecutive notes,
  `rtttl` inserts a 10 ms gap with a *blocking* `delay()` between the two
  `set_level()` calls, so both land inside one of our `loop()` passes. Entries
  are fused only when they share a millisecond — which is exactly the
  `update_frequency()` + `set_level()` pair, and nothing else.
- **`TCP_NODELAY` on both sides.** Every frame is 9 bytes, so all of them are
  sub-MSS; Nagle would hold each note until the previous ACK returned.
- **The browser is a delayed mirror**, so its eyes ride the same delayed clock as
  its ears: the panel and the scope are scheduled at play-out time, not painted
  on arrival.

## Browser autoplay

Browsers require a user gesture before audio can start. The page asks at load —
Chrome grants it once this origin's Media Engagement Index has been crossed, and
Web Audio use feeds that index, so it earns itself with use. Until then, the
`LISTEN` button (or a click anywhere on the page) opens it. `MUTE` silences the
ear, not the instrument: the scope keeps scrolling and the panel keeps counting.

## License

GPLv3 for the C++, MIT for the Python — ESPHome's own model. GitHub will show
`NOASSERTION`; so does `esphome/esphome`.
