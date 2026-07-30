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

> **Částečně odblokováno (S5, 2026-07-28):** T1/T2 mají reálné adresy (test01 HW
> bring-up), takže tahle část je teď testovatelná na reálném HW. T3/T4 zůstávají
> placeholder — čekají na fyzické senzory, ta část zůstává blokovaná na OI1.

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

**T3/T4 (blokováno na OI1 — chybí fyzické senzory/adresy):**
- [ ] (Po OI1) Po rebootu sledovat log/HA — "Chassis Temperature" (T3) a "Drain Pipe
      Temperature" (T4) by měly přestat být `unavailable` do ~1 poll intervalu
- [ ] (Po OI1) Ověřit, že `err_t3_t4_fail` se po bootu nespustí falešně

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

---

## Výsledek session (dokončeno, s výjimkou T3/T4)

**Datum testu:** 2026-07-28 až 2026-07-30
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
- OI6/ADR-009 — `DEFROST_ORDERED` sepnul heatery i s HEAT_MODE OFF (nalezeno
  náhodou při regresi Fáze 5). Eskalováno na Architekta, opraveno gatem na
  HEAT_MODE, **bench potvrzeno** (Fáze 5a).
- OI17, OI18 — zapsány jako nové backlog položky (`_used` senzory plošný 5s poll,
  a souhrnná revize celého vzorkování/filtr/publish řetězce před field nasazením)
  — odloženo, nezpomalovalo probíhající test.
- Detaily bugů: `BUGS.md`. Detaily ADR-009: `DECISIONS.md`.

**Stav fází:**
- Fáze 0–1: ✅ hotovo (HA API test odložen na po dokončení firmware — N/A, ne blokující)
- Fáze 2 (OI8/ADR-006): ✅ bench potvrzeno, vč. BUG-001/002
- Fáze 3 (OI9/ADR-007+008): ✅ bench potvrzeno (scénáře 3a-3d)
- Fáze 4 (OI12): ✅ T1/T2 bench potvrzeno, vč. BUG-003/004. **T3/T4 zbývá — blokováno
  na OI1** (chybí fyzické senzory/adresy), jediná neuzavřená část testu
- Fáze 5 (regrese) + 5a (ADR-009): ✅ bench potvrzeno
- Fáze 6 (reálný SSR výstup se zkušební zátěží): ✅ bench potvrzeno

**Závěr:** Bench test dokončen — **OK s výjimkou T3/T4** (očekávané, čeká na
fyzické senzory, OI1). Firmware s opravami BUG-001..004 a ADR-009 zkompilovaný,
zatím **necommitnuto**.

---

*Založeno: S5, 2026-07-21 (Implementer/CC).*
