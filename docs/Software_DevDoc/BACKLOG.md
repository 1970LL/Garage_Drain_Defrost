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
| OI1 | Nahradit placeholder adresy DS18B20 reálnými adresami při komisioningu. **T1/T2 hotovo** (S5, 2026-07-28, test01 HW bring-up: T1=`0x3400000051876f28`, T2=`0xab030a9794259928`). **T3/T4 zbývá** — čeká na fyzické senzory. | ARCHITECTURE §7.1 | Nutné před nasazením (T3/T4) |
| OI2 | Odstranit testovací I2C blok (VL53L0X, BME280, I2C bus) | ARCHITECTURE §7.2 | Fáze 2 cleanup |
| OI4 | Zhodnotit, zda 60s interval T3/T4 error checku není zbytečně pomalý vzhledem k přímému řízení výstupů | ARCHITECTURE §5 | Řešit spolu s OI12 (F3) |
| OI5 | run_time counter: ESP uptime parita (platform: uptime, raw sec diagnostic; d/h/m HA-side). Runtime topení = odložený nice-to-have | ADR-005/OI28 | Nice-to-have |
| OI6 | heat_mode/defrost decoupling: DEFROST_ORDERED kontroluje jen main_system_enabled, ignoruje HEAT_MODE — ověřit záměr | code review | K prověření |
| OI7 | web_server (port 80, bez auth): ponechat pro bench ladění, odstranit/zabezpečit až po HA implementaci — Windows precedent (OI26, field build vypíná web_server úplně, HA = sole control plane) | code review S3 | Před Field Deployment |
| OI9 | F4 (ADR-007, supersedes OI3) + ADR-008 (floor): defrost_chassis_cycle/defrost_drain_cycle nyní dvoufázové — `delay(floor)` → `wait_until(!bs_defrost, timeout = ceiling−floor)`. Nové entity `chassis_time_floor`/`drain_time_floor` (1–10 min, default 2/3), přejmenováno `chassis_time_min`→`chassis_time_ceiling`/`drain_time_min`→`drain_time_ceiling` (15–60 min, default 20/20). Retrigger již negatován na `defrost_running` (ADR-008 bod 6). Realizováno S5. Rename HA friendly names odložen na ADR-005 execution session. Čeká na bench potvrzení (TEST_PLAN.md Fáze 3). | ADR-007, ADR-008, Code_review §F4 | 🔧 Implementováno, čeká na bench |
| OI10 | Před implementační session pro ADR-004: zkopírovat custom komponentu `led_sequencer` z `Garage_Windows/firmware/custom_components/led_sequencer/` do `firmware/custom_components/` — bez toho se ADR-004 YAML nezkompiluje | Windows srovnání S3 | Prerekvizita ADR-004 session |
| OI11 | F2: detekce `err_t1_t2_fail`/`err_t3_t4_fail` čte raw senzory místo `_used`, rozbíjí Simulation Mode testování bez HW. Řešit spolu s ADR-004 (err_t1..t4 split) | Code_review_20260710.md §F2 | 🔧 Implementer, spolu s ADR-004 |
| OI12 | F3: DS18B20 median filtr — send_first_at: 1 (odkomentovat/nastavit), zkrátí ~10min boot NaN okno na ~1 poll. ZÁVISLOST OI8/ADR-006 (ohraničuje zmeškání boot-resume). Realizováno S5 na všech 4 senzorech. Čeká na bench potvrzení — **blokováno na OI1** (bez reálných adres senzorů nelze přesně ověřit zkrácení okna, viz TEST_PLAN.md Fáze 4). | Code_review §F3, ADR-006 | 🔧 Implementováno, ověření blokováno na OI1 |
| OI13 | F5: text_sensor "Error Status" má `update_interval: 60s`, zpožděn až 60s za instant binary_sensory. Zkrátit/odstranit interval | Code_review_20260710.md §F5 | 🔧 Implementer, nízká priorita |
| OI14 | S1: přidat `password: !secret ap_password` do `wifi.ap` (Lubor spravuje secret), konzistentně s Windows. Bonus: `ota:` na list syntax jako Windows | Code_review_20260710.md §S1 | 🔧 Implementer |
| OI15 | ADR-008 bod 8 (akceptované omezení): retrigger resetuje rozpočet stropu — podmínka kmitající rychleji než floor může topení protahovat neomezeně, ceiling chrání jen proti zaseknuté podmínce, ne kmitající. Řešení až při výskytu: nezávislý absolutní max on-time neresetovaný retriggerem (precedent Garage_Windows ERR8/OI25, `max_on_time_ms`). | ADR-008 bod 8 | Hardening, k pozorování v zimní sezóně |

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
| OI8 | F1 (ADR-006, varianta B): auto-resume defrostu po rebootu je záměrný, realizovaný edge-triggerem DEFROST_ORDERED. **Bench potvrzeno** (TEST_PLAN.md Fáze 2, 2026-07-28) — po cestě odhaleny a opraveny BUG-001 (restore_mode) a BUG-002 (sim_mode race), viz BUGS.md. | S5 |

---

*Last updated: 2026-07-28 (S5)*
