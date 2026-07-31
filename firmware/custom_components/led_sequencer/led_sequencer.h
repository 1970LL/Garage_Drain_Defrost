#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "driver/gpio.h"

namespace esphome {
namespace led_sequencer {

struct Step {
    uint32_t on_ms;
    uint32_t off_ms;
    uint8_t  repeat;  // number of on/off pulses for this step
};

struct PatternDef {
    std::string       id;
    std::vector<Step> sequence;
    bool              continuous{false};
    bool              active{false};
};

class LedSequencer : public Component {
 public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::LATE; }

    // ── Config setters (called from __init__.py code gen) ─────────────────────
    void set_output_pin(uint8_t pin)          { output_pin_ = pin; }
    void set_inter_pattern_gap_ms(uint32_t v) { inter_pattern_gap_ms_ = v; }
    void set_end_of_frame_gap_ms(uint32_t v)  { end_of_frame_gap_ms_ = v; }
    void begin_pattern(const char* id, bool continuous);
    void add_step(uint32_t on_ms, uint32_t off_ms, uint8_t repeat);

    // ── Public API (called from YAML lambdas) ─────────────────────────────────
    void set_pattern(const char* id, bool active);
    void clear_all();

 private:
    enum class Phase { IDLE, ON, OFF, INTER_GAP, FRAME_GAP };

    void advance_();
    void set_pin_(bool level);
    PatternDef* find_continuous_active_();
    int         find_next_active_default_(int from);

    uint8_t  output_pin_{2};
    uint32_t inter_pattern_gap_ms_{1500};
    uint32_t end_of_frame_gap_ms_{2000};
    std::vector<PatternDef> patterns_;

    Phase    phase_{Phase::IDLE};
    int      pat_idx_{0};   // index of currently playing pattern in patterns_
    int      step_idx_{0};  // index of current step in pattern.sequence
    uint8_t  pulse_n_{0};   // current pulse count within step (0-based)
    uint32_t deadline_{0};
};

}  // namespace led_sequencer
}  // namespace esphome
