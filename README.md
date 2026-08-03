# GARAGE_DRAIN_DEFROST  
ESP32-WROOM-32UE (EBYTE) • ESPHome (ESP-IDF) • Defrost Protection System

## Overview
This project controls heating cables that prevent freezing of:
- the **outdoor AC unit chassis** (Toshiba, garage installation),  
- the **condensate drain piping**.

The system monitors several temperature sensors and detects the AC unit's **defrost cycles** based on gas-inlet temperature behaviour. Heating outputs are automatically activated for a condition-driven duration (bounded by a guaranteed minimum and a safety-ceiling maximum), and all key thresholds/timers are adjustable directly from **Home Assistant** (no recompilation required).

The entire device is powered by **ESP32-WROOM-32UE** placed on a custom PCB with opto-isolated or MOSFET-driven outputs.

> For full architectural detail (data flow, error handling internals, HA entity/device grouping, decision rationale), see `docs/Software_DevDoc/ARCHITECTURE.md` and `DECISIONS.md`. This README stays at overview level.

---

## Hardware Summary

### MCU & Connectivity
- **ESP32-WROOM-32UE** (38-pin module, external antenna)
- Wi‑Fi (ESP‑IDF)  
- ESPHome API + Web UI (port 80, bench use — planned removal before field deployment)

### Sensors (all DS18B20)
Single shared 1‑Wire bus on **GPIO21**, four sensors:
- **T1 – Outside Temperature**
- **T2 – Gas Inlet Temperature** (gas outlet tube of the outdoor unit's heat exchanger — see ADR-011 for exact placement rationale; historically called "Evaporator", renamed since the old name implied the wrong physical position)
- **T3 – Chassis Temperature**
- **T4 – Drain Pipe Temperature**

Sensor addresses are commissioned once per physical unit (see Commissioning Workflow below). Each raw sensor feeds a corresponding "(used)" template sensor that the control logic actually reads — this indirection layer is what Simulation Mode overrides (see below).

### Simulation mode — sensor substitution for testing/debugging
Simulation Mode lets you drive the control logic from manually-entered values (Web UI/HA) instead of the physical sensors, without recompiling:
- **Simulation Mode** switch, default OFF
- **Sim Outside / Sim Gas Inlet / Sim Chassis / Sim Drain** — manual temperature entities, only take effect while Simulation Mode is ON

### Dynamic DS18B20 Polling Strategy
Two-tier: **20 s base** (all four sensors, always) / **5 s fast tier** (only while actually relevant):
- `HEAT_MODE` active → **Gas Inlet** (T2) is force-refreshed every 5 s
- Either heater output is **ON** → **Chassis** (T3) and **Drain** (T4) are force-refreshed every 5 s
- **Outside** (T1) has no fast tier — it's always relevant (feeds `HEAT_MODE` directly), so the 20 s base always applies

No median filter (removed — these sensors don't drift/spike; filtering only delayed genuine-failure detection, see `BUGS.md` BUG-006). The "(used)" layer that logic reads is event-driven, not independently polled — it refreshes exactly when its underlying raw sensor or simulation value actually changes.

### Digital Inputs
- **DI1 – GPIO36** (input only, reserved, not exposed to HA)
- **DI2 – GPIO39** (input only, reserved, not exposed to HA)

### Digital Outputs (MOSFET BSS138)
All open‑drain (low-side) MOSFET drivers (BSS138)  
Selectable **5 V / 12 V** via jumper  
Max load **100 mA per channel**  
Logic: **ESP32 HIGH → output ON**

- **DO1 – GPIO32** → Heater: Chassis (230 VAC via SSR)
- **DO2 – GPIO33** → Heater: Drain Pipe (230 VAC via SSR)
- **DO3 – GPIO25** → Reserved (OC 5/12 V, not exposed to HA)
- **DO4 – GPIO26** → Reserved (OC 5/12 V, not exposed to HA)

### Power Outputs (Heaters)
- **SSR type:** Omron G3MB‑202  
- **Load:** 230 VAC / up to 2 A  
- **Protection:** Fused (T2A) per channel  
- **Driver:** SSR driven through BSS138 (low power control side)  
- **Logic:** ESP32 HIGH → SSR ON
- **Heating cable:** self-regulating type (samoregulační), fail-safe against overheat by design — see ADR-010

### Status LEDs
Driven through BSS138 (open‑drain), via the `led_sequencer` custom component (ADR-004).  
Logic: **ESP32 HIGH → LED ON**

- **WD – GPIO4** — 4 mutually-exclusive continuous patterns, selects by priority (disabled > defrost > armed > idle):

  | State | Condition | Pattern |
  |---|---|---|
  | Disabled | Main switch OFF | 100 ms on / 900 ms off |
  | Defrosting | Either heater ON | 100 ms on / 100 ms off |
  | Armed | `HEAT_MODE` ON, no heater running | 2×(150/150) then 900 ms off |
  | Idle | Enabled, `HEAT_MODE` OFF | 500 ms on / 500 ms off |

- **ERR – GPIO2** — pulse-count error codes, see below.

⚠ **GPIO2 is an ESP32 strapping pin** (GPIO4 is not).  
LED must not force an incorrect boot level → design uses MOSFET (safe).

---

## Temperature-Based Logic (Simplified)

### MAIN_SWITCH
Enables/disables the whole operation. Allows stopping defrost activity during clima unit maintenance or spring/summer season. Controlled from Home Assistant/Web UI (Hlavní vypínač).

### HEAT_MODE
Enabled/disabled based on **outside temperature** T1 (hysteresis):
- **Enable** when T1 ≤ **+2 °C** (default)
- **Disable** when T1 ≥ **+4 °C** (default)

Both thresholds configurable from Home Assistant.

### DEFROST_ORDERED detection
Occurs when **all three** conditions hold:
1) Gas Inlet temperature T2 ≥ **+5 °C** (default)
2) ΔT = (T2 − T1) ≥ **+7 °C** (default)
3) `HEAT_MODE` is armed (freeze-relevance gate — a reverse-defrost signal only matters while frost is actually a risk)

All thresholds configurable from Home Assistant.

### Heater Activation
When `DEFROST_ORDERED` rises (false→true), both heaters start together and each follows its own two-phase timing (condition-driven, not a fixed duration):
- **Floor** — guaranteed minimum run time regardless of condition (protects against short-cycling): Chassis 2 min / Drain 3 min (default)
- Then runs until `DEFROST_ORDERED` drops, **or**
- **Ceiling** — safety-cap max run time if the condition gets stuck: 20 min (default), both channels

All floor/ceiling values are HA-adjustable (`number:` entities) and persist via `restore_value: true`. A fresh rising edge restarts both cycles even mid-run (retrigger).

---

## Error Indication (ERR LED on GPIO2)

Declarative pulse patterns via `led_sequencer` (ADR-004): each code is `300 ms on / 300 ms off`, repeated N times per its number below, with a 1500 ms gap between codes and a 2000 ms gap after the last one before the sequence repeats. Multiple active errors play **sequentially**, not overlapped.

Error codes (repeat count, numbered by this list's order — not by frequency):
- **1×** Wi‑Fi connection lost (5 min debounce)
- **2×** T1 (Outside) sensor failure
- **3×** T2 (Gas Inlet) sensor failure
- **4×** T3 (Chassis) sensor failure
- **5×** T4 (Drain) sensor failure

Failure detection reads the "(used)" sensor layer (not the raw sensor directly) so Simulation Mode doesn't trip a false fail on a physically disconnected sensor while simulating. No memory function: inactive errors stop displaying immediately.

---

## Commissioning Workflow

1. Flash firmware via USB
2. Join Wi‑Fi and verify stability (API + ping + Web UI)
3. Bring up each DS18B20 individually (`firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost_test01.yaml`), read its address from the boot log
4. Assign each sensor (T1–T4) address in the production YAML
5. Verify SSR outputs using test loads (never real 230 VAC heating cables until logic is bench-confirmed)
6. Verify error signalling (disconnect sensors → observe ERR LED / HA "Kód chyby")
7. Pair with Home Assistant (ESPHome native API integration) — entity names/grouping are already final (see below), no rename expected after go-live

## Repository Structure

```
GARAGE_DRAIN_DEFROST/
 ├─ firmware/
 │   ├─ yaml/
 │   │   ├─ ESP32-D0WD-V3_Gar_Drain_Defrost.yaml       (production)
 │   │   ├─ ESP32-D0WD-V3_Gar_Drain_Defrost_test01.yaml (sensor bring-up scratch)
 │   │   └─ secrets.yaml     (not tracked)
 │   ├─ custom_components/   (led_sequencer, ported from Garage_Windows)
 │   ├─ scripts/
 │   ├─ .esphome/, .venv/, build/, requirements.txt
 ├─ Hardware revision A/     (Gerbers, 3D packages)
 ├─ docs/
 │   ├─ Software_DevDoc/     (ADR log, architecture snapshot, backlog, test plan, session log — see Software_DevDoc_structure.md)
 │   ├─ Toshiba/             (outdoor unit service manual excerpts — ADR-011 reference)
 │   ├─ Thermal Cable/       (heating cable datasheets — ADR-010 reference)
 │   └─ GPIO_Desc_Functionality.xlsx
 ├─ .vscode/
 └─ README.md
```

---

## Home Assistant Integration

Live — paired via ESPHome's native API integration, imported cleanly (S15/2026-08-01). All entities use **Czech friendly names** and are grouped into four HA sub-devices (ADR-005):

| Group | Purpose |
|---|---|
| Globální | Whole-system state — main switch, WiFi/aggregate error, "Kód chyby", "Stav systému", uptime |
| Provoz | Live operational state — HEAT_MODE/DEFROST_ORDERED, the temperatures actually driving logic, both heaters |
| Nastavení | User-adjustable thresholds and timers |
| Servisní | Diagnostics — raw sensors, Simulation Mode + its manual entities, per-sensor error flags |

Full entity-by-entity list, icons, and dashboard grouping: `docs/Software_DevDoc/ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md` (authoritative, ADR-015). Background/rationale: `ARCHITECTURE.md` §8.

## Notes & Recommendations
- All thresholds and heater run times are fully configurable from HA.
- Sensors are polled frequently enough to catch defrost cycles via the dynamic (base/fast-tier) polling strategy above.
- Ensure all mains wiring complies with safety regulations.
- Reserved pins (DI1/DI2, DO3/DO4) have no function yet — kept hidden from HA until they do.

## ESPHome (ESP‑IDF) Notes

`logger: level: DEBUG` during bench/development; the actual current base config (API encryption, OTA, WiFi incl. `wifi.ap` fallback password, web_server) lives in `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` — that file is the single source of truth, not a duplicated sample here.
