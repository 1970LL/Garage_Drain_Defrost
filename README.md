# GARAGE_DRAIN_DEFROST  
ESP32-WROOM-32UE (EBYTE) • ESPHome (ESP-IDF) • Defrost Protection System

## Overview
This project controls heating cables that prevent freezing of:
- the **outdoor AC unit chassis** (Toshiba, garage installation),  
- the **condensate drain piping**.

The system monitors several temperature sensors and detects the AC unit’s **defrost cycles** based on evaporator temperature behaviour. Heating outputs are automatically activated for configurable intervals, and all key thresholds/timers are adjustable directly from **Home Assistant** (no recompilation required).

The entire device is powered by **ESP32-WROOM-32UE** placed on a custom PCB with opto-isolated or MOSFET-driven outputs.

---

## Hardware Summary

### MCU & Connectivity
- **ESP32-WROOM-32UE** (38-pin module, external antenna)
- Wi‑Fi (ESP‑IDF)  
- ESPHome API + optional Web UI

### Sensors (all DS18B20)
Single shared 1‑Wire bus on **GPIO21**, four sensors:
- **T1 – Outside Temperature**
- **T2 – Toshiba Evaporator**
- **T3 – Toshiba Chassis**
- **T4 – Drain Pipe**
Sensor addresses are assigned during commissioning.

### Simulation mode - Sensors substitute for testing and debugging
Simulation mode allows providing temperature values from web server. 
- **Simulation mode ON/OFF** default OFF 
- **Sim Outside**
- **Sim Evaporator**
- **Sim Chassiss** option
- **Sim Drain** option

### Dynamic DS18B20 Polling Strategy
To detect defrost cycles reliably while keeping bus traffic low, we use on‑demand refresh:
- All DS18B20 sensors have a base `update_interval` of 60 s.
- When `HEAT_MODE` is **inactive**, no extra polling is performed (low bus load).
- When `HEAT_MODE` is **active**, the **evaporator** sensor (T2) is actively refreshed every **5 s** via `component.update`.
- When either heater output (DO1/DO2) is **ON**, the **chassis** (T3) and **drain** (T4) sensors are refreshed every **20 s** via `component.update`.

### Digital Inputs
- **DI1 – GPIO36** (input only, reserved)
- **DI2 – GPIO39** (input only, reserved)

### Digital Outputs (MOSFET BSS138)
All open‑drain (low-side) MOSFET drivers (BSS138)  
Selectable **5 V / 12 V** via jumper  
Max load **100 mA per channel**  
Logic: **ESP32 HIGH → output ON**

- **DO1 – GPIO32** → Heater: Chassis (230 VAC via SSR)
- **DO2 – GPIO33** → Heater: Drain Pipe (230 VAC via SSR)
- **DO3 – GPIO25** → Reserved (OC 5/12 V)
- **DO4 – GPIO26** → Reserved (OC 5/12 V)

### Power Outputs (Heaters)
- **SSR type:** Omron G3MB‑202  
- **Load:** 230 VAC / up to 2 A  
- **Protection:** Fused (T2A) per channel  
- **Driver:** SSR driven through BSS138 (low power control side)  
- **Logic:** ESP32 HIGH → SSR ON

### Status LEDs
Driven through BSS138 (open‑drain).  
Logic: **ESP32 HIGH → LED ON**

- **WD – GPIO4** (Watchdog 1 Hz blink, pulse duration depends on **MAIN_SWITCH** state)  
- **ERR – GPIO2** (Error codes; 5 s cycle with 100 ms)

⚠ **GPIO2 and GPIO4 are ESP32 strapping pins.**  
LEDs must not force incorrect boot levels → design uses MOSFET (safe).

---

## Temperature-Based Logic (Simplified)

### MAIN_SWITCH
Enables/disables the whole oparation. Allows to stop defrost activity on clima unit
maintanence or is spring/summer seaso. Main switch can be controlled from Home Assistant. 

### HEAT_MODE
Enabled/disabled based on **outside temperature** T1:
- **Enable** when T1 ≤ **+2 °C** (default)
- **Disable** when T1 ≥ **+4 °C** (default)

Both thresholds configurable from Home Assistant.

### DEFROST_ORDERED detection  
Occurs when:
1) Evaporator temperature T2 ≥ **+5 °C**  
2) ΔT = (T2 – T1) ≥ **+7 °C**

Both thresholds configurable from Home Assistant.

### Heater Activation
When `DEFROST_ORDERED == true`:
- **DO1 (Chassis heater):** ON for **default 10 min**
- **DO2 (Drain heater):** ON for **default 15 min**

All timing constants will be **HA-adjustable** (`number:` entities) and persist via `restore_value: true`.

### Notes
- Initial firmware version may use fixed timing while logic is being validated.  
- Final logic will allow full configuration via HA.

---

## Error Indication (ERR LED on GPIO2)

- LED ON for 100 ms, OFF for 100 ms (pulse). Frame for each error 1s.
- Pulses repeated **every 5 seconds**

Error codes:
- **1×** Wi‑Fi connection lost  
- **2×** T1 or T2 failure  
- **3×** T3 or T4 failure  
- **4×** Reserved  
- **5×** Reserved  

Multiple active errors are shown sequentially.

No memory function: inactive errors are not displayed.

---

## Commissioning Workflow

1. Flash firmware via USB  
2. Join Wi‑Fi and verify stability (API + ping + Web UI)  
3. Read DS18B20 addresses from logs  
4. Assign each sensor (T1–T4) in YAML  
5. Verify SSR outputs using test loads  
6. Verify error signalling (disconnect sensors → observe ERR LED)

## Repository Structure

GARAGE_DRAIN_DEFROST/
 ├─ firmware/
 │   ├─ yaml/
 │   │   ├─ ESP32-D0WD-V3_Gar_Drain_Defrost.yaml
 │   │   └─ secrets.yaml     (not tracked)
 │   ├─ .esphome/
 │   ├─ .venv/
 │   ├─ requirements.txt
 │   └─ custom_components/
 ├─ Hardware revision A/
 ├─ docs/
 ├─ .vscode/
 │   ├─ settings.json
 │   ├─ tasks.json
 ├─ .gitignore
 └─ README.md

---

## Home Assistant Integration (later step)
Entities exposed to Home Assistant will include:
- sensor.outside_temperature
- sensor.evaporator_temperature
- sensor.chassis_temperature
- sensor.drain_temperature
- binary_sensor.heat_mode
- binary_sensor.defrost_ordered
- switch.heater_chassis
- switch.heater_drain
- number.* for all thresholds & timers (with restore_value: true)

## Notes & Recommendations
- All thresholds and heater run times will be fully configurable from HA.
- Sensors must be polled frequently enough to detect defrost cycles (dynamic polling strategy)
- Consider adding optional humidity or fan status input in future revisions.
- Ensure all mains wiring complies with safety regulations.

## ESPHome (ESP‑IDF) Notes

Recommended base configuration:

```yaml
api:
  encryption:
    key: !secret api_encryption_key
  reboot_timeout: 0s

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  power_save_mode: none      # mandatory for stable logs/API

web_server:
  port: 80
  version: 2

logger:
  level: INFO

