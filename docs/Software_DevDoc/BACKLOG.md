# BACKLOG — Garage_Drain_Defrost

> **Účel:** Co je otevřené, odložené, nebo hotové — v rámci aktuální fáze.
> **Vlastník:** Architekt primárně, Implementer doplňuje.
> **Pravidlo:** Řádky se přidávají; hotové se přesouvají do Done, nemažou se.

---

## Aktivní

Naplní se výstupem code review (plánováno jako navazující session).

Předběžné položky viditelné už teď z ARCHITECTURE.md §7 (technický dluh z YAML review):

| ID | Popis | Zdroj | Priorita |
|---|---|---|---|
| OI1 | Nahradit placeholder adresy DS18B20 (`0x1`–`0x4`) reálnými adresami při komisioningu | ARCHITECTURE §7.1 | Nutné před nasazením |
| OI2 | Odstranit testovací I2C blok (VL53L0X, BME280, I2C bus) | ARCHITECTURE §7.2 | Fáze 2 cleanup |
| OI4 | Zhodnotit, zda 60s interval T3/T4 error checku není zbytečně pomalý vzhledem k přímému řízení výstupů | ARCHITECTURE §5 | Řešit spolu s OI12 (F3) |
| OI5 | run_time counter: ESP uptime parita (platform: uptime, raw sec diagnostic; d/h/m HA-side). Runtime topení = odložený nice-to-have | ADR-005/OI28 | Nice-to-have |
| OI6 | heat_mode/defrost decoupling: DEFROST_ORDERED kontroluje jen main_system_enabled, ignoruje HEAT_MODE — ověřit záměr | code review | K prověření |
| OI7 | web_server (port 80, bez auth): ponechat pro bench ladění, odstranit/zabezpečit až po HA implementaci — Windows precedent (OI26, field build vypíná web_server úplně, HA = sole control plane) | code review S3 | Před Field Deployment |
| OI8 | F1 (ADR-006, varianta B): auto-resume defrostu po rebootu je záměrný, realizovaný edge-triggerem DEFROST_ORDERED (žádný on_boot resume, žádný restore_value na defrost_running). Realizace = oprava YAML komentáře "manual start only" na pravdivý popis; závislost na OI12 (F3) ohraničuje zmeškání na ~1 poll. | ADR-006, Code_review §F1 | 🔧 Implementer (defrost impl. session) |
| OI9 | F4 (ADR-007, supersedes OI3): pevný delay v defrost_chassis_cycle/defrost_drain_cycle → wait_until(condition !bs_defrost.state, timeout chassis_time_min/drain_time_min jako bezpečnostní strop). Rename HA labelů ("...Time" → "...Max Time") odložen na ADR-005 execution session. | ADR-007, Code_review §F4 | 🔧 Implementer (defrost impl. session) |
| OI10 | Před implementační session pro ADR-004: zkopírovat custom komponentu `led_sequencer` z `Garage_Windows/firmware/custom_components/led_sequencer/` do `firmware/custom_components/` — bez toho se ADR-004 YAML nezkompiluje | Windows srovnání S3 | Prerekvizita ADR-004 session |
| OI11 | F2: detekce `err_t1_t2_fail`/`err_t3_t4_fail` čte raw senzory místo `_used`, rozbíjí Simulation Mode testování bez HW. Řešit spolu s ADR-004 (err_t1..t4 split) | Code_review_20260710.md §F2 | 🔧 Implementer, spolu s ADR-004 |
| OI12 | F3: DS18B20 median filtr — send_first_at: 1 (odkomentovat/nastavit), zkrátí ~10min boot NaN okno na ~1 poll. ZÁVISLOST OI8/ADR-006 (ohraničuje zmeškání boot-resume) — dělat přednostně, spolu s / před ověřením boot chování. | Code_review §F3, ADR-006 | 🔧 Implementer (přednostně) |
| OI13 | F5: text_sensor "Error Status" má `update_interval: 60s`, zpožděn až 60s za instant binary_sensory. Zkrátit/odstranit interval | Code_review_20260710.md §F5 | 🔧 Implementer, nízká priorita |
| OI14 | S1: přidat `password: !secret ap_password` do `wifi.ap` (Lubor spravuje secret), konzistentně s Windows. Bonus: `ota:` na list syntax jako Windows | Code_review_20260710.md §S1 | 🔧 Implementer |

---

## Odložené

*(zatím prázdné)*

---

## Hardening

*(zatím prázdné — naplní se po Field Deployment fázi)*

---

## Done

| ID | Popis | Session |
|---|---|---|
| — | Doc seed založen (VISION, ARCHITECTURE, ROADMAP) | S1 |
| OI3 | Paralelní běh chassis/drain heateru bez sync konce — rozhodnuto: nebyl to plný záměr, mění se na condition-driven s bezpečnostním stropem. Viz OI9 (F4) | S3 |

---

*Last updated: 2026-07-21 (S4)*
