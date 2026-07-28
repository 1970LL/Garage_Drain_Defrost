# ARCHITECTURE — Garage_Drain_Defrost

> **Účel:** Snapshot současného stavu systému. Popisuje *co* je postaveno, ne *proč*.
> Pro zdůvodnění technických voleb viz `DECISIONS.md` (ADR-NNN odkazy v textu).
> **Vlastník:** Architekt.
> **Aktuální stav:** Restart pod standardizovaným workflow. Firmware pochází z
> ChatGPT/breadboard éry (pre-review) — tento dokument popisuje kód *tak, jak je nyní*,
> včetně známých dočasných/testovacích bloků a otevřených TODO.
> **Phase:** 1 — Doc-sync & Code Review.

---

## 1. Účel a scope dokumentu

ARCHITECTURE.md je deskriptivní snapshot — popisuje aktuální stav systému tak, aby
nový čtenář (nebo budoucí session) získal kompletní orientaci, aniž by četl celý YAML.

**Vztah k ostatním dokumentům:**

| Dokument | Co obsahuje |
|---|---|
| `ARCHITECTURE.md` (tento) | Co je — HW, FW struktura, logika, error mapa, konfigurace |
| `DECISIONS.md` | Proč to tak je — log architektonických rozhodnutí (ADR) |
| `BACKLOG.md` | Co dál — otevřené body (naplní se code review) |
| `PROJECT_VISION.md` | Proč projekt existuje, přístup ke kvalitě (odlehčený) |

---

## 2. Hardware Overview

### 2.1 MCU & Connectivity

- **MCU:** ESP32-WROOM-32UE (38-pin module, externí anténa)
- **Framework:** ESPHome s ESP-IDF
- **Síť:** Wi-Fi
- **HA integrace:** ESPHome Native API + Web Server (port 80)
- **OTA:** ESPHome OTA platform

### 2.2 GPIO Mapa

**Analog / 1-Wire:**

| Signal | GPIO | Popis |
|---|---|---|
| 1-Wire bus | GPIO21 | Sdílená sběrnice pro 4× DS18B20 |

**Digital Inputs:**

| Signal | GPIO | Popis |
|---|---|---|
| DI1 | GPIO36 | Rezervováno, interní/log-only |
| DI2 | GPIO39 | Rezervováno, interní/log-only |

**Digital Outputs (MOSFET BSS138, open-drain, 5V/12V jumper-selectable):**

| Signal | GPIO | Funkce |
|---|---|---|
| DO1 | GPIO32 | SSR — topení šasi (230 VAC) |
| DO2 | GPIO33 | SSR — topení odtokové trubky (230 VAC) |
| DO3 | GPIO25 | Rezerva (OC 5/12 V), interní |
| DO4 | GPIO26 | Rezerva (OC 5/12 V), interní |

**Status LED (přes BSS138, HIGH = ON):**

| Signal | GPIO | Funkce |
|---|---|---|
| WD | GPIO4 | Watchdog heartbeat 1 Hz — 500/500 ms (systém enabled) nebo 100/900 ms (disabled) |
| ERR | GPIO2 | Chybové pulzní kódy, 10s cyklus (viz §5) |

⚠ GPIO2 je strapping pin (ESP32 strapping piny: GPIO0/2/5/12/15) — bezpečné díky BSS138
driveru. GPIO4 strapping pin **není** — původní poznámka (převzatá z README/Windows) byla
fakticky nesprávná; opraveno po ověření přes `esphome config` (varování padlo jen na GPIO2).

### 2.3 Testovací blok — k odstranění (TODO)

YAML aktuálně obsahuje HW/kód, který slouží jen k testování a je označen
`# delete later on, only for testing`:

- **VL53L0X** distance senzor (I2C adresa 0x41, enable GPIO27)
- **BME280** senzor (I2C adresa 0x76)
- **I2C bus** (GPIO22=SDA, GPIO23=SCL) — existuje jen kvůli výše uvedeným testovacím senzorům

Toto je hlavní kandidát na vyčištění ve Fázi 2 (Firmware Cleanup). Žádná část
produkční defrost logiky na I2C senzorech nezávisí.

---

## 3. Senzory (DS18B20, 1-Wire)

Čtyři DS18B20 senzory na sdílené sběrnici GPIO21:

| ID | Název | Adresa | Účel |
|---|---|---|---|
| `t_outside` | Outside Temperature | `0x3400000051876f28` — komisionováno S5 (2026-07-28) | Venkovní teplota |
| `t_evap` | Evaporator Temperature | `0xab030a9794259928` — komisionováno S5 (2026-07-28) | Teplota výparníku (Toshiba) |
| `t_chassis` | Chassis Temperature | `0x3` — **TODO: placeholder, nutno nahradit reálnou adresou** | Teplota šasi jednotky |
| `t_drain` | Drain Pipe Temperature | `0x4` — **TODO** | Teplota odtokové trubky |

Základní `update_interval: 120s`, filtr `median` (window 5, send_every 5).

### 3.1 Dynamic Polling Strategy

Aby detekce defrostu byla rychlá, ale bus zátěž nízká:

- Když je **HEAT_MODE aktivní** → `t_evap` se force-refreshuje každou 5 s (`component.update`)
- Když je **kterýkoli heater ON** → `t_chassis` a `t_drain` se force-refreshují každých 20 s

(Intervaly v kódu jsou 1s/4s smyčky kvůli mediánovému filtru — potřebují 5 vzorků, proto
efektivní perioda 5s/20s.)

### 3.2 Simulation Layer ("used" sensors)

Nad reálnými senzory existuje indirection vrstva — čtyři `template` senzory
(`t_outside_used`, `t_evap_used`, `t_chassis_used`, `t_drain_used`), které přepínají
mezi reálnou hodnotou a simulovanou hodnotou podle globálního přepínače `sim_mode`:

```
sim_mode == true  → vrací sim_t_outside / sim_t_evap / sim_t_chassis / sim_t_drain
sim_mode == false → vrací t_outside / t_evap / t_chassis / t_drain
```

**Veškerá řídicí logika (HEAT_MODE, DEFROST_ORDERED) čte výhradně z `_used` senzorů**,
nikdy přímo z fyzických — to umožňuje testování bez hardwaru.

---

## 4. Řídicí logika

### 4.1 Globální stavové proměnné (`globals`)

| ID | Typ | Restore | Účel |
|---|---|---|---|
| `main_system_enabled` | bool | ano (true) | Hlavní vypínač systému |
| `sim_mode_state` | bool | ano (false) | Simulační režim |
| `defrost_running` | bool | ne | Právě probíhá defrost cyklus |
| `err_wifi_lost` | bool | ne | WiFi chyba (debounced) |
| `err_t1_t2_fail` | bool | ne | Chyba T1/T2 senzoru |
| `err_t3_t4_fail` | bool | ne | Chyba T3/T4 senzoru |
| `wifi_lost_duration_sec` | uint32 | ne | Debounce čítač pro WiFi chybu |

### 4.2 HEAT_MODE (binary_sensor, hystereze)

Aktivní, když venkovní teplota (`t_outside_used`) klesne pod `heat_on_th` (default 2,0 °C);
deaktivuje se, až teplota vystoupá nad `heat_off_th` (default 4,0 °C). Dvojitý práh brání
oscilaci. Pokud je `main_system_enabled == false`, HEAT_MODE je vždy false.

### 4.3 DEFROST_ORDERED (binary_sensor, AND logika)

Signalizuje probíhající odmrazovací cyklus venkovní jednotky. Podmínka (obě musí platit):

1. **Absolutní práh:** `t_evap_used ≥ def_abs_th` (default 5,0 °C)
2. **Delta práh:** `(t_evap_used − t_outside_used) ≥ def_dt_th` (default 7,0 °C)

Pokud je systém disabled, DEFROST_ORDERED je vždy false.

### 4.4 Defrost cyklus (scripty)

Na vzestupnou hranu `DEFROST_ORDERED` (false→true), pokud systém je enabled a defrost
zrovna neběží (`defrost_running == false`), se spustí `run_defrost`:

```
run_defrost (mode: restart)
  ├─ defrost_running = true
  ├─ script.execute: defrost_chassis_cycle   ─┐
  ├─ script.execute: defrost_drain_cycle      ├─ běží paralelně (fire-and-forget spuštění)
  ├─ wait_until: obě dokončeny                ┘
  └─ defrost_running = false

defrost_chassis_cycle: heater_chassis ON → delay(chassis_time_floor)
                       → wait_until(!DEFROST_ORDERED, timeout = chassis_time_ceiling − chassis_time_floor) → OFF
defrost_drain_cycle:   heater_drain   ON → delay(drain_time_floor)
                       → wait_until(!DEFROST_ORDERED, timeout = drain_time_ceiling − drain_time_floor)     → OFF
```

Každý cyklus má dvě meze (ADR-008). `chassis_time_floor` / `drain_time_floor`
(default 2 / 3 min) je garantovaná minimální doba topení — běží bez ohledu na stav
podmínky. Chrání proti short-cyklování: `DEFROST_ORDERED` nemá hysterezi, takže krátké
zakolísání kolem prahu by heater vypnulo dřív, než topný kabel při tepelné setrvačnosti
šasi/trubky cokoli ohřeje. `chassis_time_ceiling` / `drain_time_ceiling` (default 20 min)
je bezpečnostní strop proti zaseknuté podmínce (ADR-007). Timeout druhé fáze je
`ceiling − floor`, takže celkový strop cyklu je `ceiling`.

Mezi floor a stropem je heater řízen reálným trváním podmínky: krátký defrost skončí
na floor, delší na poklesu `DEFROST_ORDERED`, selhání vyhodnocení na stropu.

**Retrigger:** edge-trigger `DEFROST_ORDERED` není gatovaný na `defrost_running` —
nová vzestupná hrana restartuje `run_defrost` i oba nested scripty (`mode: restart`).
`defrost_running` je čistý stavový příznak, ne interlock. Známé omezení: retrigger
resetuje rozpočet stropu, takže kmitající podmínka může topení protahovat (ADR-008
bod 8, k pozorování v zimní sezóně).

Oba nested scripty startují současně a běží nezávisle na svých mezích — chassis a drain
heater tedy nejsou synchronně vypnuté.

**Chování po rebootu (ADR-006, varianta B):** auto-resume defrostu je záměrný a
realizuje ho výhradně edge-trigger `DEFROST_ORDERED` — jeho `static bool last` se při
bootu inicializuje na `false`, takže první `DEFROST_ORDERED == true` přečtené po bootu
je vzestupná hrana → `run_defrost` fírne. Žádný explicitní `on_boot` resume ani
`restore_value` na `defrost_running` se nepoužívá. Nejhorší případ zmeškání (reboot
uprostřed reálného cyklu jednotky) je ohraničen dobou, než DS18B20 senzory nahlásí
platná data — s `send_first_at: 1` (F3/OI12) ~1 poll interval místo původních ~10 min.
Heatery jsou `restore_mode: ALWAYS_OFF`, takže toto okno je bezpečné (jednotka krátce
bez asistence, ne nechtěné topení).

### 4.5 Main System Switch

Globální vypínač (`sw_main_system_enable`). Vypnutí:
- nastaví `main_system_enabled = false`
- force-stopne všechny tři scripty (`run_defrost`, `defrost_chassis_cycle`, `defrost_drain_cycle`)
- vynutí okamžité vypnutí obou heaterů

---

## 5. Error Handling & Indikace

Tři nezávislé, souběžně platné error flagy (na rozdíl od Windows tu není hierarchie
ERR1–ERR10, jen tři ploché stavy):

| Flag | Detekce | Debounce |
|---|---|---|
| `err_wifi_lost` | `wifi.connected` == false | 300 s (5 min) souvislého výpadku |
| `err_t1_t2_fail` | `isnan()` na T1 nebo T2 | kontrola každých 20 s |
| `err_t3_t4_fail` | `isnan()` na T3 nebo T4 | kontrola každých 60 s, informativní (ne kritické) |

**ERR LED** (`show_error_code` script, spouštěný intervalem 10s): sekvenčně přehraje pulzní
patterny pro každou aktivní chybu — WiFi 1 pulz, T1/T2 2 pulzy, T3/T4 3 pulzy — s mezerami
mezi patterny. Více chyb současně = zobrazí se všechny za sebou v jednom 10s cyklu.

**HA-facing:**
- `bs_err_wifi_lost`, `bs_err_t1_t2`, `bs_err_t3_t4` — jednotlivé binary_sensory (device_class: problem)
- `bs_any_error` — souhrnný OR všech tří
- `sensor_error_status` (text_sensor) — čitelný comma-separated seznam aktivních chyb ("OK" když žádná)

---

## 6. Konfigurace (HA-editable `number` entity)

Všechny persistují přes `restore_value: true`.

| Entita | Default | Rozsah | Účel |
|---|---|---|---|
| `heat_on_th` | 2.0 °C | −20 až 10 | HEAT_MODE zapnutí |
| `heat_off_th` | 4.0 °C | −10 až 15 | HEAT_MODE vypnutí |
| `def_abs_th` | 5.0 °C | −10 až 30 | Defrost absolutní práh (evaporátor) |
| `def_dt_th` | 7.0 °C | 0 až 20 | Defrost delta práh (evap − outside) |
| `chassis_time_floor` | 2 min | 1–10 | Garantovaná min. doba běhu chassis heateru (ADR-008) |
| `drain_time_floor` | 3 min | 1–10 | Garantovaná min. doba běhu drain heateru (ADR-008) |
| `chassis_time_ceiling` | 20 min | 15–60 | Max. doba běhu chassis heateru (bezpečnostní strop, ADR-007/008) |
| `drain_time_ceiling` | 20 min | 15–60 | Max. doba běhu drain heateru (bezpečnostní strop, ADR-007/008) |
| `sim_t_outside/evap/chassis/drain` | 10/5/5/5 °C | dle senzoru | Manuální hodnoty pro simulační režim |

---

## 7. Známý technický dluh (vstup pro BACKLOG)

Toto jsou položky viditelné přímo z YAML — code review pravděpodobně přidá další:

1. **TODO adresy senzorů** — `t_outside`/`t_evap`/`t_chassis`/`t_drain` mají placeholder
   adresy `0x1`–`0x4`, nutno nahradit reálnými adresami při komisioningu.
2. **Testovací I2C blok** — VL53L0X + BME280 + I2C bus, explicitně označeno "delete later".
3. **DI1/DI2 jsou "Reserved"** — nevyužité vstupy, jen logují press/release, bez funkce.
4. **DO3/DO4 jsou "Reserved"** — nevyužité výstupy (interní, skryté z HA).

---

## 8. Verzování dokumentu

| Verze | Datum | Změna |
|---|---|---|
| 1.0 | 2026-07-10 | První verze. Snapshot stavu firmwaru z ChatGPT/breadboard éry, zdroj: YAML review před formálním code review. |
| 1.1 | 2026-07-10 | Oprava §2.2: GPIO4 mylně označen jako strapping pin, opraveno na jen GPIO2. Schváleno Architektem po konzistenční revizi doc seedu (S1). |
| 1.2 | 2026-07-21 | §4.4 přepsán dle ADR-006 (boot-resume varianta B) a ADR-007 (defrost condition-driven, supersedes OI3). S4 (Architekt), doc-commit S4. |
| 1.3 | 2026-07-21 | Konzistenční čištění po ADR-007: §7 bod "paralelní defrost bez sync konce" odstraněn (vyřešeno, viz §4.4), zbylé body přečíslovány; §6 tabulka `chassis_time_min`/`drain_time_min` popis změněn z "doba běhu" na "max. doba (bezpečnostní strop)". |
| 1.4 | 2026-07-22 | §4.4 a §6 přepsány dle ADR-008 (floor + rename _time_min → _time_ceiling). S5 (Architekt). |
| 1.5 | 2026-07-28 | §3: reálné adresy T1 (`t_outside`)/T2 (`t_evap`) komisionovány na hotovém HW (test01 bring-up), TODO odstraněno pro obě. T3/T4 zůstávají placeholder. S5 (Implementer). |
