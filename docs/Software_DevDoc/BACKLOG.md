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
| OI4 | Zhodnotit, zda 60s interval T3/T4 error checku není zbytečně pomalý vzhledem k přímému řízení výstupů. Senzor samotný je po BUG-006 rychlý (20s/5s) — zbývá posoudit, jestli má smysl zkrátit i samotný isnan() check interval (dnes 60s) | ARCHITECTURE §5 | K posouzení |
| OI5 | run_time counter: ESP uptime parita (platform: uptime, raw sec diagnostic; d/h/m HA-side). Runtime topení = odložený nice-to-have | ADR-005/OI28 | Nice-to-have |
| OI7 | web_server (port 80, bez auth): ponechat pro bench ladění, odstranit/zabezpečit až po HA implementaci — Windows precedent (OI26, field build vypíná web_server úplně, HA = sole control plane) | code review S3 | Před Field Deployment |
| OI13 | F5: text_sensor "Error Status" má `update_interval: 60s`, zpožděn až 60s za instant binary_sensory. Zkrátit/odstranit interval | Code_review_20260710.md §F5 | 🔧 Implementer, nízká priorita |
| OI14 | S1: přidat `password: !secret ap_password` do `wifi.ap` (Lubor spravuje secret), konzistentně s Windows. Bonus: `ota:` na list syntax jako Windows | Code_review_20260710.md §S1 | 🔧 Implementer |
| OI15 | ADR-008 bod 8 (akceptované omezení): retrigger resetuje rozpočet stropu — podmínka kmitající rychleji než floor může topení protahovat neomezeně, ceiling chrání jen proti zaseknuté podmínce, ne kmitající. Řešení až při výskytu: nezávislý absolutní max on-time neresetovaný retriggerem (precedent Garage_Windows ERR8/OI25, `max_on_time_ms`). **ADR-010:** samoregulační kabel je fail-safe proti přehřátí; guard už není ochrana proti přehřátí, jen proti plýtvání energií — priorita snížena. | ADR-008 bod 8, ADR-010 | Nice-to-have (bylo: Hardening) |
| OI16 | Per-surface freeze gate: zvážit gate topení na měřený povrch (t_chassis_used/t_drain_used ≤ th) místo/vedle HEAT_MODE proxy — přesnější, ale +entita a NaN fail-safe policy. | ADR-009 | Odloženo — po zimním field pozorování |
| OI17 | `_used` senzory (`t_outside_used`/`t_gas_inlet_used`/`t_chassis_used`/`t_drain_used`) mají plošný `update_interval: 5s` nezávislý na HEAT_MODE/skutečné změně zdroje — re-publikují nezměněnou hodnotu i když se zdroj pod nimi nezměnil. Zaplevňuje log/HA traffic bez informační hodnoty (méně palčivé po BUG-006 — raw senzory teď 20s/5s, ne 120s, ale princip pořád platí). Navrhovaná oprava: event-driven refresh (`component.update` po publikaci reálného senzoru + `on_value:` na `sim_t_*` number entitách) místo plošného pollu — stejný vzor jako BUG-002 fix. | Bench test S5, 2026-07-30 | 🔧 Implementer, úklid |
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
| ADR-004 | `led_sequencer` naportován (ERR LED 5 kódů, WD LED 4 stavy), nahradil ruční `show_error_code` script + WD 1s heartbeat interval. **Bench potvrzeno** (TEST_PLAN.md Fáze 7) — WD kompletně, ERR `err_wifi`/`err_t1`/`err_t2` + sekvenční přehrávání. `err_t3`/`err_t4` zbývají, blokováno na OI1. | S10 |
| OI12 | F3/send_first_at:1 supersedováno BUG-006: median filtr (odkud send_first_at pocházel) na všech 4 senzorech zakomentován, takže boot-NaN-okno i tahle otázka odpadají zcela — raw senzor publikuje první čtení okamžitě, žádný filtr window k naplnění. T3/T4 mechanismus stejný jako T1/T2, čeká už jen na fyzické senzory (OI1), ne na software. | S10 |
| OI18 | Souhrnná revize vzorkovacího řetězce vyřešena BUG-006 (vynuceno reálným nálezem při bench testu, ne jen preventivně): `update_interval` sjednocen na 20s/5s, median filtr zakomentován na všech 4 senzorech. | S10 |

---

*Last updated: 2026-07-31 (S10) — bench test uzavřen s výjimkou T3/T4 (OI1, pokračování zítra); BUG-005/006 opraveny, ADR-004 bench potvrzeno, OI12/OI18 hotovo*
