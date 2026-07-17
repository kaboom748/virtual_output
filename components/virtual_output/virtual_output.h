#pragma once

#include "esphome/components/output/float_output.h"
#include "esphome/components/socket/socket.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

#include <memory>

namespace esphome::virtual_output {

/// A FloatOutput that has no pin. The browser is the buzzer.
///
/// rtttl in `output:` mode drives a passive buzzer with exactly three calls:
/// update_frequency(note), set_level(gain), set_level(0). It keeps its own tempo
/// (see the note_duration_ early return in rtttl.cpp::loop), so it never asks the
/// output for backpressure and never blocks on it. That is what makes a virtual
/// output cheap where a virtual speaker would not be: there is no PCM, no ring
/// buffer, no virtual clock -- roughly 100 bytes per second at peak instead of
/// 32000, and no ESP32-only dependency.
///
/// What travels on the wire is a timestamped transition, not a sample stream:
///
///     [0xAA][flags][ts u32 LE][freq u16 LE][duty u8]   = 9 bytes
///
/// The timestamp is taken inside update_frequency()/write_state(), on the ESP,
/// before the network can add jitter to it. The browser re-applies each
/// transition at anchor + (ts - anchor_ts) with the Web Audio sample clock, so
/// network jitter shifts the whole melody instead of deforming its rhythm.
class VirtualOutput : public output::FloatOutput, public Component {
 public:
  void set_port(uint16_t port) { this->port_ = port; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  /// PWM peripherals override this to retune the carrier; the FloatOutput base
  /// body is empty. Forgetting the override is the silent failure this whole
  /// component exists to expose: nothing warns, and every note plays at the
  /// pitch of the first one.
  void update_frequency(float frequency) override;

 protected:
  /// Called by FloatOutput::set_level() with inversion (and, when
  /// USE_OUTPUT_FLOAT_POWER_SCALING is compiled in, min_power/max_power scaling)
  /// already applied. So the browser receives the duty cycle the pin would carry,
  /// not the one rtttl asked for.
  void write_state(float state) override;

  /// One musical instant: the state of the output at millisecond `ts`.
  struct Event {
    uint32_t ts;
    uint16_t freq;
    uint8_t duty;
  };

  static constexpr uint8_t SYNC_BYTE = 0xAA;
  static constexpr uint8_t FRAME_SIZE = 9;
  static constexpr uint8_t FLAG_BEAT = 0x01;   // heartbeat: stats only, never scheduled
  static constexpr uint8_t FLAG_STATE = 0x02;  // authoritative state: apply immediately
  static constexpr uint16_t HEARTBEAT_MS = 100;
  static constexpr uint8_t QUEUE_LEN = 8;
  static constexpr uint16_t REQUEST_LINE_SIZE = 128;
  static constexpr uint16_t REQUEST_MAX_BYTES = 4096;
  static constexpr uint32_t REQUEST_TIMEOUT_MS = 2000;
  static constexpr uint32_t HTML_TIMEOUT_MS = 8000;
  static constexpr size_t HTML_CHUNK_SIZE = 256;
  static constexpr uint8_t HTML_CHUNKS_PER_LOOP = 8;

  void push_(uint16_t freq, uint8_t duty);
  void accept_client_(uint32_t now);
  void read_request_(uint32_t now);
  void handle_request_(uint32_t now);
  void start_stream_(uint32_t now);
  void pump_html_(uint32_t now);
  void drop_pending_();
  void drop_stream_();
  bool send_frame_(uint8_t flags, uint32_t ts, uint16_t freq, uint8_t duty);

  uint16_t port_{8082};

  // Mirror of what the pin would be doing right now. Kept even with nobody
  // connected: a real buzzer does not stop being driven because you left the
  // room, and a late browser needs the truth on arrival.
  uint16_t freq_{0};
  uint8_t duty_{0};

  // Transitions waiting to be written. Not a single coalesced state: rtttl
  // inserts a 10 ms gap between two identical notes with a *blocking* delay()
  // between the two set_level() calls, so both land inside one of our loop()
  // passes. Collapsing them to one state would silently merge repeated notes
  // into one long tone. Entries are fused only when they share a millisecond,
  // which is exactly the update_frequency()+set_level() pair and nothing else.
  Event queue_[QUEUE_LEN]{};
  uint8_t queue_head_{0};
  uint8_t queue_count_{0};
  bool state_dirty_{false};
  uint32_t dropped_{0};

  uint32_t last_beat_{0};

  std::unique_ptr<socket::ListenSocket> server_;
  std::unique_ptr<socket::Socket> stream_client_;
  std::unique_ptr<socket::Socket> pending_client_;

  char request_[REQUEST_LINE_SIZE]{};
  uint16_t request_len_{0};
  uint16_t request_total_{0};
  bool header_eol_{false};
  uint32_t request_start_{0};

  bool html_sending_{false};
  size_t html_offset_{0};
};

}  // namespace esphome::virtual_output
