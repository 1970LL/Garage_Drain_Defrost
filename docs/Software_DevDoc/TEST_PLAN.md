# TEST PLAN — Garage_Drain_Defrost

> **Účel:** Ověření změn ze session S5/S6/S7 (OI8/ADR-006, OI9/ADR-007+008, OI12,
> OI2, OI6/ADR-009) na hotovém hardware (PCB) — první reálný test mimo breadboard.
> **Vlastník:** Implementer (CC), průběžně aktualizován.
> **Rozsah:** Firmware `ESP32-D0WD-V3_Gar_Drain_Defrost.yaml`, průběžně rozšiřováno
> o nálezy z bench testu (BUG-001..004, ADR-009).

---

## ⚠️ Bezpečnost — přečíst před zapojením

- DO1/DO2 (GPIO32/33) spínají **230 VAC přes SSR** na topné kabely. Dokud nejsou
  jednotlivé logické kroky ověřené, **nepřipojuj SSR výstupy k reálným topným
  kabelům** — použij zkušební zátěž (žárovka/LED+rezistor) nebo jen multimetr na
  výstupu SSR.
- Ověř pojistky (T2A) jsou osazené před připojením jakékoli zátěže.
- Main System Switch (`sw_main_system_enable`) vypni jako první krok, pokud
  cokoli nesedí — force-vypne oba heatery okamžitě.

---

## Fáze 0 — Pre-flight (hotové HW, poprvé mimo breadboard)

- [✅] Zapojení GPIO odpovídá README Hardware Summary / ARCHITECTURE §2.2 (DI1
      GPIO36, DI2 GPIO39, DO1 GPIO32, DO2 GPIO33, DO3 GPIO25, DO4 GPIO26, WD
      GPIO4, ERR GPIO2, 1-Wire GPIO21)
- [✅] Testovací I2C blok (GPIO22/23 SDA/SCL, GPIO27 VL53L0X enable) — ověřit, že
      tyto piny nejsou na finální desce přiřazené jinému účelu (OI2 blok je
      "delete later", ale zatím je v YAML aktivní)
- [✅] SSR/topné výstupy zatím **odpojené od reálné 230 VAC zátěže** (viz výše)
- [✅] `secrets.yaml` obsahuje platné hodnoty pro WiFi/API/OTA (Lubor spravuje sám)

## Fáze 1 — Flash & základní konektivita

- [✅] `esphome compile` proběhne bez chyb
- [✅] Flash přes USB proběhne úspěšně
- [✅] Log ukazuje úspěšné připojení k WiFi (žádné opakované reconnecty)
- [N/A] HA API se spáruje (šifrovaný, `api_encryption_key`)
  poznámka - netestováno, testy HA po dokončení firmware 
- [✅] Web UI (port 80) dostupné z prohlížeče
- [✅] Watchdog LED (GPIO4) bliká 500/500 ms (systém enabled, default stav), 100/900 ms (systém disabled)

## Fáze 2 — OI8 / ADR-006: Boot-resume přes edge-trigger

> Testuje se výhradně v **Simulation Mode** (sim entity mají `restore_value: true`,
> přežijí reboot) — nepotřebuje reálné DS18B20 senzory.

- [✅] Zapnout `Simulation Mode` (`sim_mode`)
- [ ] Nastavit `Sim Evaporator` a `Sim Outside` tak, aby platilo
      `DEFROST_ORDERED == true` (např. Sim Evaporator ≥ 5.0 °C a rozdíl
      Evaporator−Outside ≥ 7.0 °C — dle výchozích prahů)
  > ℹ️ **ADR-009 (2026-07-30):** Od tohoto rozhodnutí `DEFROST_ORDERED` navíc
  > vyžaduje `HEAT_MODE == ON`, tedy **`Sim Outside ≤ heat_on_th` (default 2.0 °C)**
  > — jinak zůstane OFF bez ohledu na Evaporator/delta. Např. Sim Outside = 0 °C,
  > Sim Evaporator = 8 °C splní všechny tři podmínky najednou. Tenhle test proběhl
  > (✅ výše) ještě před ADR-009 — při případném re-testu od nuly na to pamatuj.
- [✅] Ověřit v HA/logu, že `DEFROST_ORDERED` binary_sensor je `ON` a defrost
      cyklus reálně běží (heater switche `ON`)
- [✅] **Restartovat ESP32 (power-cycle nebo reset), zatímco podmínka stále platí**
  > 🐛 **BUG-001 nalezen zde (2026-07-28):** Main System Enable i Simulation Mode se
  > po restartu resetovaly na OFF (default `restore_mode: ALWAYS_OFF` u template
  > switchů přepsal `restore_value: true` globály). Opraveno v YAML
  > (`restore_mode: DISABLED` na obou switchích) — vyžaduje re-flash a opakování
  > tohoto kroku od začátku (Simulation Mode i podmínku nastavit znovu po flashi).
  > Detaily: `BUGS.md` BUG-001.
- [✅] Po bootu (během ~1 poll intervalu po startu) ověřit: `run_defrost` se spustí
      znovu automaticky (heater switche znovu `ON` po krátké prodlevě) — toto je
      **záměrné chování** dle ADR-006, ne bug
- [✅] Zkontrolovat log — měl by obsahovat `show_error_code`/edge-trigger hlášení,
      žádné odkazy na starý (odstraněný) `on_boot: script.stop`
- [✅] Vypnout Simulation Mode po testu, ověřit, že se vrátí na reálné senzory
  > 🐛 **BUG-002 nalezen zde (2026-07-28):** Přepnutí Simulation Mode (kterýmkoli
  > směrem) vždy spustilo oba heatery — race condition mezi čtyřmi nezávisle
  > pollovanými `_used` senzory (viz `BUGS.md` BUG-002). Opraveno v YAML
  > (`component.update` na všech čtyřech, synchronně po přepnutí) — vyžaduje
  > re-flash. Po re-flashi ověř: přepnutí sim_mode už nespustí heater, pokud
  > podmínka (real i sim) reálně DEFROST_ORDERED nesplňuje.

## Fáze 3 — OI9 / ADR-007+ADR-008: Defrost cyklus floor + ceiling

> Také přes Simulation Mode. Čtyři scénáře — floor garance, condition-driven pokles,
> ceiling cutoff, a retrigger.
>
> ℹ️ **ADR-009 (2026-07-30):** stejně jako u Fáze 2 — `DEFROST_ORDERED == true`
> teď vyžaduje i `Sim Outside ≤ heat_on_th` (HEAT_MODE armed). Scénáře 3a-3d níže
> proběhly (✅) před ADR-009; při re-testu od nuly na tuhle podmínku pamatuj.

**3a — Floor garance (podmínka spadne hned na začátku):**
- [✅] Simulation Mode ON, nastavit hodnoty pro `DEFROST_ORDERED == true`
- [✅] Ověřit heater ON (chassis i drain)
- [✅] Okamžitě (do pár sekund) změnit sim hodnoty tak, aby `DEFROST_ORDERED`
      spadlo na `false`
- [✅] Ověřit: heater **zůstane ON** po celou dobu `chassis_time_floor`/
      `drain_time_floor` (default 2/3 min), i když podmínka už neplatí
- [✅] Ověřit: heater se vypne krátce po uplynutí floor (podmínka je stále false)

**3b — Ukončení na poklesu podmínky (po floor, před ceiling):**
- [✅] Simulation Mode ON, nastavit `DEFROST_ORDERED == true`, nechat běžet přes floor
- [✅] Po uplynutí floor, ale dřív než uplyne `chassis_time_ceiling`/`drain_time_ceiling`,
      změnit sim hodnoty tak, aby `DEFROST_ORDERED` spadlo na `false`
- [✅] Ověřit: heater se vypne **okamžitě** (do ~1s), ne až na plný ceiling

**3c — Ukončení na bezpečnostním stropu (podmínka trvá déle než ceiling):**
- [✅] Dočasně snížit `chassis_time_ceiling`/`drain_time_ceiling` na nízkou hodnotu
      (min. povolené 15 min, nebo dočasně accept delší test) pro rychlejší test
- [✅] Simulation Mode ON, nastavit `DEFROST_ORDERED == true` a **nechat platit**
      po celou dobu testu
- [✅] Ověřit: heater se vypne přesně na nastaveném ceilingu (±pár sekund), i když
      `DEFROST_ORDERED` je stále `true`
- [✅] Ověřit: `defrost_running` global se vrátí na `false` po dokončení obou
      nested scriptů
- [✅] Vrátit `chassis_time_ceiling`/`drain_time_ceiling` na provozní hodnoty po testu

**3d — Retrigger (ADR-008 bod 6):**
- [✅] Spustit defrost cyklus (podmínka true), nechat běžet částečně
- [✅] Zatímco cyklus běží, vyvolat novou vzestupnou hranu `DEFROST_ORDERED`
      (spadnout na false a znovu na true)
- [✅] Ověřit: cyklus se restartuje (heater zůstává ON, časový rozpočet floor/ceiling
      se resetuje od nuly) — to je nové, záměrné chování ADR-008, dřív by hrana
      byla ignorována, dokud `defrost_running == true`

## Fáze 4 — OI12: Sensor warm-up window

> **Plně odblokováno (S11, 2026-08-01):** T1/T2 mají reálné adresy od S5
> (2026-07-28, test01 HW bring-up). T3/T4 nyní také mají reálné adresy (S11,
> test01 HW bring-up: T3=`0xa90625910004ba28`, T4=`0xb6062591abac6f28`) — OI1
> kompletně vyřešeno, tahle část je teď testovatelná na reálném HW.

**T1/T2 (testovatelné teď):**
- [✅] Po rebootu sledovat log/HA — "Outside Temperature" (T1) a "Evaporator
      Temperature" (T2) by měly přestat být `unavailable` do ~1 poll intervalu
      (dle `update_interval`), ne až po ~10 minutách
- [✅] Ověřit, že `err_t1_t2_fail` se po bootu **nespustí falešně** (žádné zbytečné
      ERR LED pulzy hned po startu)
  > 🐛 **BUG-003 nalezen zde (2026-07-30):** `err_t1_t2_fail` po bootu hlásil poruchu
  > nekonzistentně (někdy ano, srovná se do ~10-20s, někdy ne), a v logu se objevovalo
  > `scratch pad checksum invalid` na 1-Wire sběrnici. Root cause: on-demand refresh
  > intervaly pro T2/T3/T4 (`component.update`) hammerovaly sběrnici mnohem rychleji,
  > než tvrdil komentář (1s/4s místo 5s/20s), a kolidovaly s T1 čtením. Opraveno v
  > YAML (perioda intervalu opravena na 5s/20s) — vyžaduje re-flash a opakování
  > tohoto kroku od začátku. Detaily: `BUGS.md` BUG-003.
  > 🐛 **BUG-004 nalezen zde (2026-07-30, po BUG-003 fixu):** Nekonzistentní
  > `err_t1_t2_fail` po bootu přetrvávalo i po opravě BUG-003 (3-vodičové zapojení
  > DS18B20 vyloučilo parazitní napájení jako příčinu). Skutečná příčina: timing race
  > mezi náhodným prvním spuštěním error-checku a náhodnou první konverzí senzoru
  > (oboje nezávisle 0-5s po bootu dle ESPHome scheduleru) — check někdy vyhodnotí
  > defaultní `NAN` dřív, než senzor poprvé publikuje, a medián filtr pak drží tuhle
  > "chybnou" hodnotu dalších ~5 refresh cyklů. Opraveno přidáním 10s startup grace
  > před vyhodnocením obou error-checků (T1/T2 i T3/T4) — vyžaduje re-flash a
  > opakování tohoto kroku od začátku. Detaily: `BUGS.md` BUG-004.

**T3/T4 (testovatelné teď, S11):**
- [✅] Po rebootu sledovat log/HA — "Chassis Temperature" (T3) a "Drain Pipe
      Temperature" (T4) by měly přestat být `unavailable` do ~1 poll intervalu
- [✅] Ověřit, že `err_t3`/`err_t4` se po bootu **nespustí falešně** (žádné zbytečné
      ERR LED pulzy hned po startu)

## Fáze 5 — Regrese (sousední systémy nedotčené touto session)

- [✅] Main System Switch OFF → oba heatery okamžitě OFF, scripty zastavené
- [✅] HEAT_MODE hystereze funguje (zapne/vypne kolem prahů, bez oscilace)
- [✅] Error detekce WiFi (5min debounce), T1/T2, T3/T4 pořád hlásí správně mimo
      boot-warm-up okno
- [✅] ERR LED pulzní patterny (1×/2×/3×) pořád fungují pro existující 3 error
      kategorie
- [✅] Watchdog LED přepíná 500/500 ↔ 100/900 podle Main Switch stavu

**5a — OI6/ADR-009: DEFROST_ORDERED gated na HEAT_MODE**

> 📐 **Nález zde (2026-07-30, náhodou při běžné regresi):** `DEFROST_ORDERED` sepnul
> oba heatery i s `HEAT_MODE == OFF` (Sim Outside 4 °C, Sim Evaporator 17 °C) —
> otevřená otázka OI6 z code review S3 (nikdy nerozhodnuto, jestli je to záměr).
> Eskalováno na Architekta → **ADR-009**: `DEFROST_ORDERED` nově vyžaduje i
> `HEAT_MODE == ON` (freeze-relevance gate). `bs_defrost` lambda: `+ && id(bs_heat_mode).state`.
> Detaily: `DECISIONS.md` ADR-009. Tenhle sub-blok testuje přímo tu opravu — nebyl
> v původním plánu, protože o mezeře nikdo nevěděl, dokud se náhodně neprojevila.

- [✅] Simulation Mode ON, `Sim Outside = 4°C` (nad `heat_on_th` = 2.0°C → HEAT_MODE
      OFF), `Sim Evaporator = 17°C` (abs i delta prahy splněny) → ověřit
      `DEFROST_ORDERED == OFF` a oba heatery zůstávají OFF
- [✅] Se stejným Sim Evaporator = 17°C změnit `Sim Outside = 0°C` (pod `heat_on_th`
      → HEAT_MODE ON) → ověřit `DEFROST_ORDERED == ON` a heatery naskočí
- [✅] Zatímco defrost cyklus běží (po floor, před ceiling — stejné okno jako 3b),
      zvednout `Sim Outside` nad `heat_off_th` (4.0°C), aby HEAT_MODE spadlo na OFF
      → ověřit: heater se vypne **okamžitě** (stejný mechanismus jako 3b, jen
      spouštěč je teď HEAT_MODE místo evap/outside prahů), `defrost_running` se
      vrátí na `false`
- [✅] Vrátit Sim hodnoty na neutrální stav po testu (mimo obě podmínky)

## Fáze 6 — Reálný výstup (až po logických testech výše)

- [✅] Připojit zkušební zátěž na DO1/DO2 (ne ještě reálné topné kabely)
- [✅] Ověřit SSR spíná správně (multimetr/vizuálně) při heater ON/OFF z fází 2–3
- [ ] Teprve po úspěšném ověření připojit reálné topné kabely

## Fáze 7 — ADR-004: ERR/WD LED (`led_sequencer`)

> Nové (S10, 2026-07-31). Nahrazuje starý `show_error_code` script a WD 1s
> heartbeat interval — vizuální ověření timingu, stopkami/okem, žádné HW
> potřeba mimo desku samotnou.

**WD LED (GPIO4) — 4 vzájemně se vylučující stavy:**
- [✅] Main Switch OFF → `wd_disabled` (100ms ON / 900ms OFF, krátký záblesk)
- [✅] Main Switch ON, HEAT_MODE OFF, žádný heater → `wd_idle` (500/500, pomalý tep)
- [✅] Simulation Mode: vynutit HEAT_MODE ON (Sim Outside ≤ heat_on_th), žádný
      heater běžící → `wd_armed` (2× rychlý dvojzáblesk 150/150, pak ~900ms pauza)
- [✅] Spustit defrost cyklus (heater ON) → `wd_defrost` (100/100, rychlý tep)
- [✅] Ověřit přechody jsou okamžité (do ~500ms) a nikdy nejsou aktivní dva vzory
      současně

**ERR LED (GPIO2) — 5 pulzních kódů, sekvenčně při více současných chybách:**
- [✅] Vynutit `err_wifi` (odpojit WiFi/router) → 1× pulz (300/300), pak gap
- [✅] Vynutit `err_t1` (Simulation Mode ON, ale sledovat že se týká reálného T1 —
      viz pozn. níže) → 2× pulz
- [✅] Vynutit `err_t2` → 3× pulz
- [✅] Vynutit `err_t3`/`err_t4` → 4×/5× pulz
- [✅] Vyvolat 2+ chyby současně → přehrají se sekvenčně za sebou (gap 1500ms
      mezi kódy, 2000ms po posledním před opakováním cyklu), ne najednou/překryté
  > ℹ️ OI11 fix: `err_t1..t4` teď čtou `_used` vrstvu — v Simulation Mode (real
  > sensor NaN, ale `sim_mode` ON) by se chyba **neměla** spustit. Pro vynucení
  > `err_t1`/`err_t2` reálně je potřeba buď vypnout Simulation Mode s odpojeným
  > senzorem, nebo dočasně odpojit T1/T2 fyzicky.
  > 🐛 **BUG-005 nalezen zde (2026-07-31):** Při přepnutí Simulation Mode OFF→ON
  > (T1/T2 reálně ~30°C, T3/T4 NaN) se nahodile spustily oba heatery i s
  > HEAT_MODE/DEFROST_ORDERED zobrazeným OFF. Root cause: BUG-002 fix (synchronní
  > `component.update` na čtyřech `_used` senzorech) běžel *dřív*, než se
  > `id(sim_mode).state` vůbec publikoval (neoptimistic template switch —
  > `publish_state()` až v příští `loop()`), takže vynucený update fakticky
  > no-op a senzory se vrátily na nezávislé 5s pollery — recidiva BUG-002. Log
  > ukázal `Outside Temperature (used)` s fresh sim hodnotou 4s dřív než `Gas
  > Inlet Temperature (used)`, což během té mezery spurious-triggerlo
  > `DEFROST_ORDERED`. Opraveno: čtyři `_used` lambdy čtou přímo `sim_mode_state`
  > (globál, nastavený synchronně jako první krok akce) místo `id(sim_mode).state`.
  > Vyžaduje re-flash. Detaily: `BUGS.md` BUG-005.
- [✅] Po re-flashi: přepnout Simulation Mode ON→OFF→ON několikrát za sebou s T1/T2
      teplými (real ~20-30°C) a T3/T4 NaN (nepřipojené) — ověřit, že se **ani
      jednou** nespustí heater, pokud reálně DEFROST_ORDERED nesplňuje podmínku
  > 🐛 **BUG-006 nalezen zde (2026-07-31):** Při testu `err_t1` (T1 datový vodič
  > fyzicky odpojen) zůstala `Outside Temperature` "zamrzlá" na poslední hodnotě,
  > nikdy nepřešla na `nan`, `err_t1` se nespustil. Root cause: median filtr
  > (`filter.cpp`, `get_window_values_()`) přeskakuje NaN vzorky při výpočtu
  > mediánu — jednotlivé výpadky maskuje zbylými platnými vzorky v okně. S
  > `window_size:5`/`send_every:5` a (dřívějším) 120s intervalem to trvalo až
  > 10-20 minut, než se selhání vůbec projevilo. Stejný jev potvrzen i na T2
  > (Gas Inlet) mimo HEAT_MODE. Opraveno: `update_interval` 120s→20s na všech 4
  > senzorech, rychlý tier sjednocen na 5s, median filtr zakomentován (senzory
  > nedriftují). Vyžaduje re-flash. Detaily: `BUGS.md` BUG-006.
- [✅] Po re-flashi zopakovat `err_t1` (odpojit T1 datový vodič) — ověřit, že
      `Outside Temperature` přejde na `nan` a `err_t1` se spustí **do ~20-25s**
      (ne 10-20 min)
- [✅] Zkontrolovat log — žádné odkazy na starý (odstraněný) `show_error_code`

---

## Výsledek session (S5–S11, kompletní vč. T3/T4)

**Datum testu:** 2026-07-28 až 2026-07-31
**Nálezy:**
- BUG-001 — Main System Enable / Simulation Mode se resetovaly na OFF po každém
  rebootu (default `restore_mode: ALWAYS_OFF` u template switchů). Opraveno,
  bench potvrzeno.
- BUG-002 — přepnutí Simulation Mode (kterýmkoli směrem) vždy spustilo oba
  heatery (race condition mezi 4 nezávisle pollovanými `_used` senzory). Opraveno,
  bench potvrzeno.
- BUG-003 — on-demand DS18B20 refresh intervaly (T2/T3/T4) hammerovaly 1-Wire
  sběrnici rychleji, než tvrdil komentář (1s/4s místo 5s/20s). Opraveno,
  bench potvrzeno.
- BUG-004 — timing race mezi error-checkem a první konverzí senzoru způsoboval
  falešný T1/T2 fail po bootu. Opraveno (10s startup grace), bench potvrzeno.
- BUG-005 — přepnutí Simulation Mode nahodile spustilo oba heatery (recidiva
  BUG-002: vynucený `component.update` běžel dřív, než se `id(sim_mode).state`
  publikoval). Opraveno (čtení `sim_mode_state` globálu přímo), bench potvrzeno.
- BUG-006 — median filtr maskoval selhání senzoru až 10-20 minut (NaN-skip v
  `get_window_values_()` + pomalý 120s interval). Opraveno (`update_interval`
  20s/5s, filtr zakomentován na všech 4 senzorech), bench potvrzeno.
- OI6/ADR-009 — `DEFROST_ORDERED` sepnul heatery i s HEAT_MODE OFF (nalezeno
  náhodou při regresi Fáze 5). Eskalováno na Architekta, opraveno gatem na
  HEAT_MODE, bench potvrzeno (Fáze 5a).
- ADR-004 — `led_sequencer` (WD 4 stavy, ERR 5 kódů) naportován, nahradil ruční
  `show_error_code` script a WD heartbeat interval. Bench potvrzeno **kompletně**
  (Fáze 7) — WD 4 stavy, ERR všech 5 kódů (`err_wifi`/`err_t1`/`err_t2`/`err_t3`/
  `err_t4`) vč. sekvenčního přehrávání více chyb najednou.
- OI17 — zapsána jako backlog položka (`_used` senzory plošný 5s poll) — probrána
  s Luborem 2026-08-01, dohodnut event-driven refresh přístup, realizace
  navazující session.
- Detaily bugů: `BUGS.md`. Detaily ADR-004/009: `DECISIONS.md`.

**Stav fází:**
- Fáze 0–1: ✅ hotovo (HA API test odložen na po dokončení firmware — N/A, ne blokující)
- Fáze 2 (OI8/ADR-006): ✅ bench potvrzeno, vč. BUG-001/002
- Fáze 3 (OI9/ADR-007+008): ✅ bench potvrzeno (scénáře 3a-3d)
- Fáze 4 (OI12): ✅ bench potvrzeno kompletně, T1-T4, vč. BUG-003/004
- Fáze 5 (regrese) + 5a (ADR-009): ✅ bench potvrzeno
- Fáze 6 (reálný SSR výstup se zkušební zátěží): ✅ bench potvrzeno
- Fáze 7 (ADR-004, ERR/WD LED): ✅ bench potvrzeno kompletně, všech 5 ERR kódů
  vč. `err_t3`/`err_t4`, vč. BUG-005/006

**Závěr:** Bench test **kompletně dokončen** (S5–S11) — všechny fáze ✅, žádný
zbylý blocker. OI1 vyřešeno (T1-T4 reálné adresy, S5+S11). Firmware s
BUG-001..006 a ADR-004/009 zkompilovaný a plně bench potvrzený.

> **S11 dodatek (2026-08-01):** OI1 vyřešeno — T3=`0xa90625910004ba28`,
> T4=`0xb6062591abac6f28` (test01 HW bring-up). Fáze 4 (T3/T4 warm-up) a Fáze 7
> (`err_t3`/`err_t4`) doběhnuty na reálném HW — Lubor potvrdil. Bench test
> kompletní.

---

*Založeno: S5, 2026-07-21 (Implementer/CC).*
