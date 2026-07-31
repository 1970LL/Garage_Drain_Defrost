#include "led_sequencer.h"
#include "esphome/core/log.h"

namespace esphome {
namespace led_sequencer {

static const char* const TAG = "led_sequencer";

// ─── ESPHome lifecycle ────────────────────────────────────────────────────────

void LedSequencer::setup() {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << output_pin_,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO%u as output", output_pin_);
    }
    set_pin_(false);
    deadline_ = millis() + 50;
}

void LedSequencer::dump_config() {
    ESP_LOGCONFIG(TAG, "LedSequencer (GPIO%u):", output_pin_);
    ESP_LOGCONFIG(TAG, "  inter_pattern_gap_ms: %u", inter_pattern_gap_ms_);
    ESP_LOGCONFIG(TAG, "  end_of_frame_gap_ms:  %u", end_of_frame_gap_ms_);
    for (auto& p : patterns_) {
        ESP_LOGCONFIG(TAG, "  pattern '%s': %s, %zu step(s)",
            p.id.c_str(),
            p.continuous ? "continuous" : "default",
            p.sequence.size());
    }
}

void LedSequencer::loop() {
    if (millis() < deadline_) return;
    advance_();
}

// ─── Config builders (invoked during C++ init from __init__.py) ───────────────

void LedSequencer::begin_pattern(const char* id, bool continuous) {
    PatternDef pd;
    pd.id = id;
    pd.continuous = continuous;
    patterns_.push_back(std::move(pd));
}

void LedSequencer::add_step(uint32_t on_ms, uint32_t off_ms, uint8_t repeat) {
    if (patterns_.empty()) return;
    patterns_.back().sequence.push_back({on_ms, off_ms, repeat});
}

// ─── Public API ───────────────────────────────────────────────────────────────

void LedSequencer::set_pattern(const char* id, bool active) {
    for (auto& p : patterns_) {
        if (p.id == id) {
            p.active = active;
            return;
        }
    }
    ESP_LOGW(TAG, "set_pattern: unknown id '%s'", id);
}

void LedSequencer::clear_all() {
    for (auto& p : patterns_) p.active = false;
}

// ─── Private helpers ──────────────────────────────────────────────────────────

void LedSequencer::set_pin_(bool level) {
    gpio_set_level(static_cast<gpio_num_t>(output_pin_), level ? 1 : 0);
}

auto LedSequencer::find_continuous_active_() -> PatternDef* {
    for (auto& p : patterns_) {
        if (p.continuous && p.active) return &p;
    }
    return nullptr;
}

// Returns index of first active default pattern at index >= from, or -1.
int LedSequencer::find_next_active_default_(int from) {
    int n = static_cast<int>(patterns_.size());
    for (int i = from; i < n; i++) {
        if (!patterns_[i].continuous && patterns_[i].active) return i;
    }
    return -1;
}

// ─── State machine ────────────────────────────────────────────────────────────

void LedSequencer::advance_() {
    uint32_t now = millis();

    // ── Continuous mode — exclusive, overrides all default patterns ────────────
    PatternDef* cont = find_continuous_active_();
    if (cont) {
        int cont_idx = static_cast<int>(cont - patterns_.data());

        // Switch to this continuous pattern if not already running it.
        if ((phase_ != Phase::ON && phase_ != Phase::OFF) || pat_idx_ != cont_idx) {
            pat_idx_  = cont_idx;
            step_idx_ = 0;
            pulse_n_  = 0;
            phase_    = Phase::ON;
            set_pin_(true);
            deadline_ = now + cont->sequence[0].on_ms;
            return;
        }

        // Advance within continuous pattern.
        Step& step = cont->sequence[step_idx_];
        if (phase_ == Phase::ON) {
            phase_    = Phase::OFF;
            set_pin_(false);
            deadline_ = now + step.off_ms;
        } else {
            pulse_n_++;
            if (pulse_n_ < step.repeat) {
                phase_    = Phase::ON;
                set_pin_(true);
                deadline_ = now + step.on_ms;
            } else {
                step_idx_ = (step_idx_ + 1) % static_cast<int>(cont->sequence.size());
                pulse_n_  = 0;
                Step& next = cont->sequence[step_idx_];
                phase_    = Phase::ON;
                set_pin_(true);
                deadline_ = now + next.on_ms;
            }
        }
        return;
    }

    // ── Transition: continuous just deactivated while we were playing it ────────
    if ((phase_ == Phase::ON || phase_ == Phase::OFF) &&
        pat_idx_ < static_cast<int>(patterns_.size()) &&
        patterns_[pat_idx_].continuous) {
        phase_ = Phase::IDLE;
        set_pin_(false);
    }

    // ── Default serial mode ────────────────────────────────────────────────────

    if (phase_ == Phase::IDLE || phase_ == Phase::FRAME_GAP) {
        int first = find_next_active_default_(0);
        if (first < 0) {
            phase_    = Phase::IDLE;
            set_pin_(false);
            deadline_ = now + 50;
            return;
        }
        pat_idx_  = first;
        step_idx_ = 0;
        pulse_n_  = 0;
        phase_    = Phase::ON;
        set_pin_(true);
        deadline_ = now + patterns_[pat_idx_].sequence[0].on_ms;
        return;
    }

    if (phase_ == Phase::INTER_GAP) {
        int next = find_next_active_default_(pat_idx_ + 1);
        if (next < 0) {
            phase_    = Phase::FRAME_GAP;
            set_pin_(false);
            deadline_ = now + end_of_frame_gap_ms_;
            return;
        }
        pat_idx_  = next;
        step_idx_ = 0;
        pulse_n_  = 0;
        phase_    = Phase::ON;
        set_pin_(true);
        deadline_ = now + patterns_[pat_idx_].sequence[0].on_ms;
        return;
    }

    // ── ON / OFF phases for default patterns ──────────────────────────────────
    PatternDef& pat = patterns_[pat_idx_];
    if (pat.sequence.empty()) {
        // Degenerate pattern — skip with inter gap.
        int next = find_next_active_default_(pat_idx_ + 1);
        if (next >= 0) {
            phase_    = Phase::INTER_GAP;
            deadline_ = now + inter_pattern_gap_ms_;
        } else {
            phase_    = Phase::FRAME_GAP;
            deadline_ = now + end_of_frame_gap_ms_;
        }
        set_pin_(false);
        return;
    }
    Step& step = pat.sequence[step_idx_];

    if (phase_ == Phase::ON) {
        phase_    = Phase::OFF;
        set_pin_(false);
        deadline_ = now + step.off_ms;
        return;
    }

    // Phase::OFF — advance within pattern or finish pattern.
    pulse_n_++;
    if (pulse_n_ < step.repeat) {
        phase_    = Phase::ON;
        set_pin_(true);
        deadline_ = now + step.on_ms;
        return;
    }

    // Step done — move to next step.
    step_idx_++;
    pulse_n_ = 0;
    if (step_idx_ < static_cast<int>(pat.sequence.size())) {
        Step& next_step = pat.sequence[step_idx_];
        phase_    = Phase::ON;
        set_pin_(true);
        deadline_ = now + next_step.on_ms;
        return;
    }

    // Pattern sequence done — inter-pattern gap or end-of-frame gap.
    int next = find_next_active_default_(pat_idx_ + 1);
    if (next >= 0) {
        phase_    = Phase::INTER_GAP;
        set_pin_(false);
        deadline_ = now + inter_pattern_gap_ms_;
    } else {
        phase_    = Phase::FRAME_GAP;
        set_pin_(false);
        deadline_ = now + end_of_frame_gap_ms_;
    }
}

}  // namespace led_sequencer
}  // namespace esphome
