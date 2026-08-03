# BACKLOG — Garage_Drain_Defrost

> **Účel:** Co je otevřené, odložené, nebo hotové — v rámci aktuální fáze.
> **Vlastník:** Architekt primárně, Implementer doplňuje.
> **Pravidlo:** Řádky se přidávají; hotové se přesouvají do Done, nemažou se.

---

## Aktivní

Vše aktivní k 2026-08-01 (S16) je vázané na Field Deployment (OI7) nebo na
zimní pozorování reálného provozu (OI15/16/19/20, B-VALID-01/02/03) — nic
z toho není akcionovatelné dřív, viz `ROADMAP.md` Fáze 4.

| ID | Popis | Zdroj | Priorita |
|---|---|---|---|
| OI7 | web_server (port 80, bez auth): ponechat pro bench ladění, odstranit/zabezpečit až po HA implementaci — Windows precedent (OI26, field build vypíná web_server úplně, HA = sole control plane) | code review S3 | Před Field Deployment |
| OI15 | ADR-008 bod 8 (akceptované omezení): retrigger resetuje rozpočet stropu — podmínka kmitající rychleji než floor může topení protahovat neomezeně, ceiling chrání jen proti zaseknuté podmínce, ne kmitající. Řešení až při výskytu: nezávislý absolutní max on-time neresetovaný retriggerem (precedent Garage_Windows ERR8/OI25, `max_on_time_ms`). **ADR-010:** samoregulační kabel je fail-safe proti přehřátí; guard už není ochrana proti přehřátí, jen proti plýtvání energií — priorita snížena. | ADR-008 bod 8, ADR-010 | Nice-to-have (bylo: Hardening) |
| OI16 | Per-surface freeze gate: zvážit gate topení na měřený povrch (t_chassis_used/t_drain_used ≤ th) místo/vedle HEAT_MODE proxy — přesnější, ale +entita a NaN fail-safe policy. | ADR-009 | Odloženo — po zimním field pozorování |
| OI20 | Heater runtime counter (kumulativní doba běhu topných kabelů) — vyděleno z OI5 při realizaci S13: uptime parita hotová (Done), tahle část zůstává odložený nice-to-have (chybí jasný use-case bez field dat). | OI5 | Nice-to-have |
| OI19 | Thaw mode (nouzové rozmrazování): jednotka odstavena (ventilátor stojí, přirozená konvekce), časově neomezený běh topných kabelů pro roztání ledu při selhání prevence; detekci dokončení lze opřít o informativní čidla T3/T4. Ne provozní režim, jen nouzový fallback. Trigger realizace: dle zkušenosti z 1. zimní sezóny. | ADR-010 | Nice-to-have |

### B-VALID-01 — Validace defrost detekce zimním pozorováním
Po první zimní sezóně ověřit:
- Délka DEFROST_ORDERED == true per cyklus: očekáváno ≤ 10 min (manuálový strop).
  Výrazně delší → podezření na zaseknutý senzor nebo kmitající podmínku (ADR-008 bod 8).
- Rozestup cyklů: očekáváno ≥ 31 min (manuálová Table 2, model 13k).
  Kratší → podezření na spurious trigger.
- Pokud data sedí → prahy def_abs_th/def_dt_th a floor hodnoty potvrzeny.
  Pokud ne → doladit.

### B-VALID-02 — Zvážit snížení *_time_ceiling po zimním ověření
Pokud reálné cykly nepřekračují 10–12 min, stáhnout ceiling z 20 → 15 min.
Jednotka fyzicky nepřekročí 10 min (manuálový strop). Neblokující.
Trigger: B-VALID-01 done.

### B-VALID-03 — Per-surface freeze gate (odloženo z ADR-009)
Zvážit t_chassis ≤ th / t_drain ≤ th jako doplňkový gate DEFROST_ORDERED
místo samotného HEAT_MODE proxy.
Trigger: zimní pozorování ukáže falešné triggery v pásmu 2–4 °C.

> ℹ️ Implementer poznámka: obsahově překrývá **OI16** (stejná myšlenka, zapsána
> S7/ADR-009 — Architekt window o ní nevěděl, bez live přístupu k repu). Ponecháno
> verbatim dle handoffu, ale při realizaci řešit jako jednu položku, ne dvě.

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
| OI2 | Testovací I2C blok (VL53L0X, BME280, I2C bus) kompletně odstraněn z YAML (ne jen zakomentován) — na žádost Lubora během bench testu, zbytečně nafukoval seznam entit ve Web UI. | S5 |
| OI6 | heat_mode/defrost decoupling vyřešeno ADR-009: DEFROST_ORDERED nyní gated na HEAT_MODE (freeze-relevance gate) — bench test (Sim Outside 4°C/HEAT_MODE OFF + Sim Evaporator 17°C) potvrdil, že šlo o reálný problém, ne jen teoretickou mezeru. | S7 |
| OI9 | F4 (ADR-007) + ADR-008 (floor): dvoufázová struktura defrost cyklů (floor→ceiling), nové entity `chassis_time_floor`/`drain_time_floor`, rename `_time_min`→`_time_ceiling`, retrigger bez gate na `defrost_running`. **Bench potvrzeno** (TEST_PLAN.md Fáze 3, scénáře 3a-3d: floor garance, condition-driven pokles, ceiling cutoff, retrigger). | S5 |
| OI10 | Custom komponenta `led_sequencer` zkopírována z `Garage_Windows/firmware/custom_components/led_sequencer/` do `firmware/custom_components/` (byte-identická, žádné project-specific hardcoding v komponentě). Ověřeno `esphome config` (smoke-test YAML, `external_components:` local path) — validuje bez chyb. Prerekvizita pro implementaci ADR-004 splněna. | Implementer, 2026-07-31 |
| OI11 | F2 vyřešeno spolu s ADR-004: `err_t1_t2_fail`/`err_t3_t4_fail` rozděleny na per-senzor `err_t1..err_t4`, detekce nově čte `_used` vrstvu místo raw senzoru — Simulation Mode už netriggeruje falešnou chybu na fyzicky nepřipojeném T3/T4. | S10 |
| ADR-004 | `led_sequencer` naportován (ERR LED 5 kódů, WD LED 4 stavy), nahradil ruční `show_error_code` script + WD 1s heartbeat interval. **Bench potvrzeno kompletně** (TEST_PLAN.md Fáze 7) — WD 4 stavy, ERR všech 5 kódů vč. `err_t3`/`err_t4`. | S10, S11 |
| OI12 | F3/send_first_at:1 supersedováno BUG-006: median filtr (odkud send_first_at pocházel) na všech 4 senzorech zakomentován, takže boot-NaN-okno i tahle otázka odpadají zcela — raw senzor publikuje první čtení okamžitě, žádný filtr window k naplnění. T3/T4 mechanismus stejný jako T1/T2, čeká už jen na fyzické senzory (OI1), ne na software. | S10 |
| OI18 | Souhrnná revize vzorkovacího řetězce vyřešena BUG-006 (vynuceno reálným nálezem při bench testu, ne jen preventivně): `update_interval` sjednocen na 20s/5s, median filtr zakomentován na všech 4 senzorech. | S10 |
| OI1 | Placeholder adresy DS18B20 nahrazeny reálnými adresami při komisioningu — **kompletně hotovo**. T1/T2 (S5, 2026-07-28): T1=`0x3400000051876f28`, T2=`0xab030a9794259928`. T3/T4 (S11, 2026-08-01, test01 HW bring-up): T3=`0xa90625910004ba28`, T4=`0xb6062591abac6f28`. | S5, S11 |
| OI13 | F5: text_sensor "Error Status" měl `update_interval: 60s`, zpožděn až 60s za instant binary_sensory. **Vyřešeno event-driven refresh přístupem** (stejný vzor jako OI17): `update_interval: never` + `on_state:` na pěti error binary_sensorech (`bs_err_wifi_lost`/`bs_err_t1..4`), volající `component.update` na `sensor_error_status`. `esphome compile` čistý. | S13 |
| OI14 | `password: !secret ap_password` doplněn do `wifi.ap` (Lubor přidal secret sám). Bonus: `ota:` převeden na list syntax (`- platform: esphome`) dle Windows precedentu. **Vynecháno záměrně:** heslo na `web_server` — Lubor upozornil, že se to u Garage_Windows nepodařilo rozjet a stejně se web_server před field deployment odstraní (OI7), nemá smysl na to teď pálit čas. | S13 |
| OI4 | 60s interval T3/T4 error checku posouzen a zkrácen na 20s (sjednoceno s T1/T2) — konzistence, po OI17/BUG-006 už není důvod, aby T3/T4 zaostávaly za rychlejšími raw senzory. Lubor rozhodl pro tuto variantu (2026-08-01) z nabídnutých tří (ponechat 60s / zkrátit na 20s / plně event-driven `on_value:` na `_used`). **Bench potvrzeno** (TEST_PLAN.md Fáze 8) — `err_t3`/`err_t4` do ~20-25s. | S12 |
| ADR-005 | Execution (entity rename + device grouping): `esphome: devices:` (Globální/Provoz/Nastavení/Servisní, šablona Garage_Windows), CZ `name:` na všech exponovaných entitách, nová entita `sensor_system_state` ("Stav systému", zrcadlí WD selector: `OFF`/`IDLE`/`ARMED`/`Heater ON`). Rezervované piny (DI1/DI2/DO3/DO4) zůstávají neexponované. Draft s crosscheck tabulkou schválen Luborem po úpravách, archivováno: `#Archive/ADR-005_execution_draft.md`. **BUG-007** nalezen bench testem (log spam na `sensor_system_state`) a opraven (event-driven refresh místo 500ms interval). **Round 2 (S15):** `icon:` doplněn na všech 35 entitách; vedlejší nález — `device_class: cold` na `bs_heat_mode` zobrazoval v HA "Chladno" místo Zapnuto/Vypnuto, odebráno, nahrazeno `icon: "mdi:snowflake-alert"`. `esphome compile` čistý (`config_hash=0xb87510a3`). | S14, S15 |
| OI5 | ESP uptime parita — `sensor: platform: uptime` (`uptime_sensor`, "Uptime (s)"), Windows OI28 precedent (ADR-005 bod 4: ESP exponuje raw sekundy, d/h/m formátování zůstává HA-side, ne firmware). Standalone, nezávislé na ADR-005 execution (entity rename) — jen nová entita v současné konvenci. "Runtime topení" část OI5 vyděleno jako **OI20** (zůstává odložený nice-to-have). `esphome compile` čistý (`config_hash=0xabae8da0`). | S13 |
| OI17 | `_used` senzory (`t_outside_used`/`t_gas_inlet_used`/`t_chassis_used`/`t_drain_used`) měly plošný `update_interval: 5s` nezávislý na HEAT_MODE/skutečné změně zdroje. **Vyřešeno event-driven refresh přístupem:** `update_interval: never` + `on_value:` na raw senzorech (real mode) + `on_value:` na `sim_t_*` number entitách (sim mode), volající `component.update` na příslušný `_used`; `on_boot:` safety net pro sim-mode-přes-reboot edge case. Zvažován i "mirror raw polling schedule" přístup, zamítnut (stejný-tick staleness riziko, zpomalil by Simulation Mode odezvu). **Bench potvrzeno** (TEST_PLAN.md Fáze 8) — sim mode instant refresh, real mode HEAT_MODE/heater fast tiery, BUG-002/005 regrese OK. Vedlejší nález při testu: AP-001 (tvrdý HW reset a `flash_write_interval`, viz `BUGS.md`) — systémové chování, ne bug, ponecháno beze změny. | S12 |
| — | **ADR-005+ návrh** — vyřešeno S16 → **ADR-015** (HA_RD_Jirny), formalizováno jako standardní entity-exposure formát. Výstup: `ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md` (autoritativní instance v tomto repu, ADR-015 §1/§6). Draft archivován: `#Archive/ADR-005-plus_proposal_draft.md`. | S15, S16 |

---

*Last updated: 2026-08-01 (S16) — ADR-005+ vyřešeno (ADR-015, `ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md`). Aktivní BACKLOG je teď 100% vázán na Field Deployment/zimní pozorování, žádná výjimka. Projekt ve Fázi 4 (Field Deployment) dle `ROADMAP.md`.*
