import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@local"]
AUTO_LOAD = []
DEPENDENCIES = []

led_sequencer_ns = cg.esphome_ns.namespace("led_sequencer")
LedSequencer = led_sequencer_ns.class_("LedSequencer", cg.Component)

# ─── Config key constants ─────────────────────────────────────────────────────

CONF_OUTPUT_PIN         = "output_pin"
CONF_INTER_PATTERN_GAP  = "inter_pattern_gap_ms"
CONF_END_OF_FRAME_GAP   = "end_of_frame_gap_ms"
CONF_PATTERNS           = "patterns"
CONF_SEQUENCE           = "sequence"
CONF_MODE               = "mode"
CONF_ON_MS              = "on_ms"
CONF_OFF_MS             = "off_ms"
CONF_REPEAT             = "repeat"

# ─── Sub-schemas ──────────────────────────────────────────────────────────────

STEP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ON_MS):                cv.int_range(min=1),
        cv.Required(CONF_OFF_MS):               cv.int_range(min=0),
        cv.Optional(CONF_REPEAT, default=1):    cv.int_range(min=1, max=255),
    }
)

PATTERN_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID):                                   cv.string,
        cv.Required(CONF_SEQUENCE):                             cv.ensure_list(STEP_SCHEMA),
        cv.Optional(CONF_MODE, default="default"):              cv.one_of("default", "continuous", lower=True),
    }
)

INSTANCE_SCHEMA = cv.Schema(
    {
        cv.GenerateID():                                        cv.declare_id(LedSequencer),
        cv.Required(CONF_OUTPUT_PIN):                           cv.int_range(min=0, max=48),
        cv.Optional(CONF_INTER_PATTERN_GAP, default=1500):     cv.int_range(min=0),
        cv.Optional(CONF_END_OF_FRAME_GAP,  default=2000):     cv.int_range(min=0),
        cv.Optional(CONF_PATTERNS):                             cv.ensure_list(PATTERN_SCHEMA),
    }
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.ensure_list(INSTANCE_SCHEMA)

# ─── Code generation ──────────────────────────────────────────────────────────

async def to_code(configs):
    for config in configs:
        var = cg.new_Pvariable(config[CONF_ID])
        await cg.register_component(var, config)

        cg.add(var.set_output_pin(config[CONF_OUTPUT_PIN]))
        cg.add(var.set_inter_pattern_gap_ms(config[CONF_INTER_PATTERN_GAP]))
        cg.add(var.set_end_of_frame_gap_ms(config[CONF_END_OF_FRAME_GAP]))

        for pattern in config.get(CONF_PATTERNS, []):
            cg.add(var.begin_pattern(
                pattern[CONF_ID],
                pattern[CONF_MODE] == "continuous",
            ))
            for step in pattern[CONF_SEQUENCE]:
                cg.add(var.add_step(
                    step[CONF_ON_MS],
                    step[CONF_OFF_MS],
                    step[CONF_REPEAT],
                ))
