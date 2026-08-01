# SESSION LOG — Garage_Drain_Defrost

> Chronologický log session (nejnovější nahoře). Append-only.

---

## S11 — 2026-08-01 (Implementer/CC) — OI1: T3/T4 komisioning

> Lubor: "Nová čidla: T3 - Outdoor unit chassis 0xa90625910004ba28 (DS18B20),
> T4 - Drain Pipe 0xb6062591abac6f28 (DS18B20). Zanes adresy čidel do
> dokumentace a uprav YAML." Stejný postup jako T1/T2 v S5 (`test01.yaml`
> bring-up, adresy přečtené z bootlogu).

**Provedeno:**
- `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml`: `t_chassis` (T3)
  `address:` `0x3`→`0xa90625910004ba28`, `t_drain` (T4) `address:` `0x4`→
  `0xb6062591abac6f28`. TODO komentáře nahrazeny komisioning poznámkou
  (konzistentně s T1/T2 stylem ze S5).
- `esphome config` — validní, 0 chyb.
- `ARCHITECTURE.md` (v1.11): §3 tabulka doplněna o reálné adresy T3/T4, §7
  bod "TODO adresy senzorů" odstraněn (zcela vyřešeno).
- `BACKLOG.md`: OI1 přesunuto z Aktivní do Done (T1/T2 S5 + T3/T4 S11).
- Lubor dokončil `TEST_PLAN.md` Fázi 4 (T3/T4 warm-up) a Fázi 7 (`err_t3`/
  `err_t4` pulzní kódy 4×/5×) bench testem — obě potvrzeny ✅. Bench test
  (S5–S11) je tímto **kompletně uzavřen**, žádný zbylý blocker.
- Následně probrán **dynamic polling** dotaz: potvrzeno chování (T1 vždy 20s;
  T2 5s kdykoli HEAT_MODE ON; T3/T4 5s jen během reálného běhu heateru, ne
  jen HEAT_MODE armed; `_used` vrstva vždy 5s napevno) s jedním upřesněním
  oproti Luborovu popisu (T3/T4 fast-tier je gated na heater running, ne na
  HEAT_MODE samotném).
- Lubor navrhl myšlenku: sladit `_used` vrstvu na stejný polling jako raw
  senzory místo plošných 5s. Zhodnoceno (bez implementace): mirror-schedule
  by mělo dvě slabiny (možný stejný-tick staleness, zpomalení Simulation Mode
  odezvy bez if/else větve navíc). Doporučen event-driven přístup (`on_value:`
  na raw senzorech + na `sim_t_*` number entitách) — to je přesně **OI17**,
  ne nová položka. Lubor souhlasil pokračovat systematicky dle BACKLOGu.
- Dohodnuté pořadí další práce: (1) uzavřít S11 doc-wise [toto], (2) OI17 +
  OI4 (event-driven `_used` refresh + přehodnocení 60s error-check intervalu),
  (3) OI13 + OI14 (drobný úklid), (4) ADR-005 execution (entity rename,
  samostatná session), (5) OI7 před Field Deployment, (6) OI15/16/19 a
  B-VALID-01/02/03 odloženy na zimní pozorování, (7) fyzická montáž (mimo
  software).

**Session S11 uzavřena.** Bench test (S5–S11) kompletní, žádný zbylý blocker.
Další session: OI17 + OI4.

---

## S10 — 2026-07-31 (Implementer/CC) — ADR-004: led_sequencer, err_t1..t4 split

> Nová, samostatná session (ne vnořená v S5, ta je uzavřená). Lubor: "Myslím, že
> se můžeme vrátit k práci na software dle backlogu." Doporučeno začít OI10
> (prerekvizita) → rovnou pokračováno implementací ADR-004 (Accepted od S2,
> nepotřebovala nové Architekt rozhodnutí, jen realizaci).

**Provedeno:**
- **OI10** — `led_sequencer` custom komponenta zkopírována z `Garage_Windows`
  (byte-identická, žádné project-specific hardcoding). Ověřena smoke-testem
  (`esphome config` s `external_components:` local path) mimo produkční YAML,
  než se do něj vůbec sáhlo.
- **ADR-004** — `led_sequencer` naportován do produkčního YAML beze změny enginu:
  - `external_components:` (`path: ../custom_components`, `components: [led_sequencer]`)
  - ERR LED (GPIO2, instance `err_led_seq`): 5 patternů (`err_wifi` 1×, `err_t1`
    2×, `err_t2` 3×, `err_t3` 4×, `err_t4` 5×, vše 300/300ms), gaps 1500/2000ms,
    číslováno dle README pořadí (ne frequency-based). Nahradil starý blokující
    `show_error_code` script (`delay:`-based sekvence).
  - WD LED (GPIO4, instance `wd_led_seq`): 4 vzájemně se vylučující continuous
    stavy (`wd_disabled`/`wd_defrost`/`wd_armed`/`wd_idle`), výběrová lambda
    přesně dle ADR-004 (`!main → disabled; else heater_on → defrost; else
    heat_mode → armed; else idle`). `wd_armed` pattern (2×150/150 + 900 off)
    zakódován jako dva kroky s posledním off_ms=1050 — Step schema nepovoluje
    on_ms=0 pro čistou "off-only" mezeru. Nahradil starý 1s heartbeat interval.
  - Oba nové 500ms intervaly volají `set_pattern(id, bool)` — stejná konvence
    jako Garage_Windows (přímé volání z lambdy, žádná ESPHome automation
    action, `__init__.py` žádnou neregistruje).
- **OI11** vyřešeno spolu s ADR-004: `err_t1_t2_fail`/`err_t3_t4_fail` (spárované
  globals) rozděleny na `err_t1`/`err_t2`/`err_t3`/`err_t4` (nezávislé), detekce
  přepnuta z raw senzorů na `_used` vrstvu — Simulation Mode teď netriggeruje
  falešnou chybu na fyzicky nepřipojeném T3/T4, dokud je `sim_mode` ON. BUG-004
  startup grace (10s) zachována beze změny na obou intervalech (20s/60s).
  Navazující spotřebitelé (`sensor_error_status` text_sensor, individuální
  binary_sensory, `bs_any_error`) přepsány na čtyři samostatné flagy.
- `ARCHITECTURE.md` §2.2 a §5 přepsány (v1.9) — nová ERR/WD tabulka dle ADR-004.
- Přidána `TEST_PLAN.md` Fáze 7 (ADR-004 ERR/WD LED). Bench test zahájen — WD LED
  potvrzeno (všechny 4 stavy), ERR LED částečně (`err_wifi` potvrzeno).
- Při ERR LED testu (`err_t1`/`err_t2`, Simulation Mode OFF→ON, T1/T2 reálně
  ~30°C, T3/T4 NaN) Lubor nahlásil **BUG-005**: přepnutí Simulation Mode nahodile
  spustilo oba heatery i s HEAT_MODE/DEFROST_ORDERED zobrazeným OFF — vypadalo
  jako recidiva BUG-002. Root-caused z logu (přesné timestampy): BUG-002 fix
  (`component.update` na čtyřech `_used` senzorech uvnitř `sim_mode`'s
  turn_on/turn_off_action) běžel *dřív*, než se `id(sim_mode).state` vůbec
  publikoval — `sim_mode` je `optimistic: false` template switch a
  `TemplateSwitch::write_state()` (`template_switch.cpp`) volá `publish_state()`
  jen pro optimistic switche; u neoptimistic se `.state` publikuje až na příští
  `loop()` iteraci. Vynucený update tak četl ještě starý režim (no-op), senzory
  se vrátily na nezávislé 5s pollery a BUG-002 mezera se znovu otevřela — log
  ukázal `Gas Inlet Temperature (used)` stale 4s za `Outside Temperature (used)`,
  což spurious-triggerlo DEFROST_ORDERED. Fix: čtyři `_used` lambdy čtou přímo
  globál `sim_mode_state` (nastavený synchronně jako první krok akce) místo
  `id(sim_mode).state`. Zkompilováno bez chyb.
- Při testu `err_t1` (T1 datový vodič fyzicky odpojen) Lubor nahlásil **BUG-006**:
  `Outside Temperature` zůstala "zamrzlá" na poslední hodnotě, nikdy nepřešla na
  `nan`, `err_t1` se nespustil. Stejný jev potvrzen i na `Gas Inlet Temperature
  (used)` mimo HEAT_MODE — Lubor se ptal, jestli `_used` vrstva vůbec dostává
  hodnoty z čidla. Root-caused ze zdroje: `MedianFilter::get_window_values_()`
  (`filter.cpp`) explicitně přeskakuje NaN vzorky při výpočtu mediánu —
  jednotlivé výpadky maskuje zbylými platnými vzorky v okně (`window_size:5`).
  Kombinace s (dřívějším) 120s `update_interval` na T1 (bez on-demand
  zrychlení, na rozdíl od T2/T3/T4) znamenala až 10-20 minut, než selhání
  senzoru vůbec projevilo jako `NAN`. `_used` vrstva sama funguje správně —
  jen věrně mirroruje pomalý raw senzor. Lubor rozhodl: `update_interval`
  120s→20s na všech 4 senzorech, rychlý on-demand tier sjednocen na 5s (T3/T4
  měly dřív 20s, bezpředmětné po zrychlení základu), a **median filtr
  zakomentován na všech 4 senzorech** (ne smazán — senzory nedriftují,
  filtrace jen zpožďovala detekci poruchy; vrátit při reálném šumu ve field
  fázi). Supersedovalo OI12 (send_first_at:1 pocházel z filtru, který teď není)
  a OI18 (souhrnná revize vzorkovacího řetězce) — obě přesunuty do Done.
- `esphome compile` — 0 chyb po obou opravách (Flash ~beze změny/mírně nižší po
  odstranění filtru).
- Po re-flashi Lubor dokončil Fázi 7 bench testem: WD LED (všechny 4 stavy),
  ERR LED (`err_wifi`/`err_t1`/`err_t2`, sekvenční přehrávání více chyb),
  BUG-005 fix (opakované přepínání Simulation Mode oběma směry s teplými T1/T2
  — žádný spurious heater start) i BUG-006 fix (odpojený T1 datový vodič →
  `err_t1` naskočil rychle, ne za 10-20 min) — **vše bench potvrzeno**.
  `err_t3`/`err_t4` a Fáze 4 T3/T4 zůstávají — blokováno na OI1 (fyzické
  senzory), pokračování zítra.
- **Session uzavřena.** Bench test kompletní s výjimkou T3/T4 (jediný zbylý
  blocker, hardware, ne software).

**Výstupy:**
- `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` (ADR-004 + BUG-005/006
  fixy, zkompilováno, bench potvrzeno) — commit + push na pokyn Lubora
- `firmware/custom_components/led_sequencer/` (nové, portováno z Garage_Windows)
- `ARCHITECTURE.md` (v1.10), `BACKLOG.md` (OI10/OI11/OI12/OI18→Done, +ADR-004
  řádek, bench potvrzeno), `BUGS.md` (+BUG-005, +BUG-006), `TEST_PLAN.md`
  (+Fáze 7, uzavřena), `SESSION_LOG.md` (tento blok)

**Blokující:** žádné (T3/T4 samostatný OI1 úkol na fyzický hardware, neblokuje)
**Další session:** OI1 (T3/T4 fyzické senzory) → dokončit Fázi 4 T3/T4 a Fázi 7
`err_t3`/`err_t4`. Entity rename session (ADR-005). Implementace se vrací k
BACKLOGu dle priority po OI1.

---

## S5 — 2026-07-21 až 2026-07-30 (Implementer/CC) — defrost implementace + bench test (dokončeno)

> Nejdelší session dosud, přes více dnů (Lubor testoval na hotovém HW, ne breadboardu).
> Číslování protíná S6 (Architekt, ADR-008 uprostřed), S7 (Architekt, ADR-009
> uprostřed), S8 (Architekt, ADR-010) a S9 (Architekt→Implementer, ADR-011) — viz
> bloky níže. S10 (Implementer, ADR-004) je už samostatná, nová práce po uzavření
> bench testu, ne vnořená v S5.

**Provedeno:**
- Batch 1+2 (2026-07-21): OI8 (`on_boot` no-op odstraněn, ADR-006 komentář u
  edge-triggeru), OI12 (`send_first_at: 1` na 4× DS18B20), OI9 (`wait_until`
  místo pevného `delay:` v defrost cyklech)
- Uprostřed práce Lubor identifikoval mezeru (DEFROST_ORDERED nemá hysterezi,
  čistě condition-driven cyklus zranitelný vůči šumu) → eskalace na Architekta →
  S6 (ADR-008, floor + ceiling)
- Implementace ADR-008: nové entity `chassis_time_floor`/`drain_time_floor`,
  přejmenování `_time_min`→`_time_ceiling`, dvoufázová struktura scriptů,
  retrigger bez gate na `defrost_running`
- Oprava prostředí: PlatformIO vlastní venv mělo rozbitý "editable install"
  esptool (jen metadata, chybějící modul) — force-reinstall opravil, `esphome
  compile` teď dojede až k `.bin`
- Vytvořen `ESP32-D0WD-V3_Gar_Drain_Defrost_test01.yaml` — bare I/O hardware
  bring-up test (žádná logika), pro ověření hotové desky
- Hardware ověřen funkční (test01), komisionovány reálné adresy T1/T2
  (OI1 částečně hotovo — T3/T4 zbývá)
- Bench test dle `TEST_PLAN.md` na produkčním firmware: Fáze 0–2 dokončeny
  - **BUG-001** nalezen a opraven: template switche (`sw_main_system_enable`,
    `sim_mode`) měly defaultní `restore_mode: ALWAYS_OFF`, což při každém bootu
    aktivně spustilo `turn_off_action` a přepsalo `restore_value: true` globály
    zpátky na false — root cause dohledán přímo ve zdrojáku ESPHome
    (`template_switch.cpp`). Fix: `restore_mode: DISABLED` na obou.
  - **BUG-002** nalezen a opraven: přepnutí `sim_mode` (kterýmkoli směrem) vždy
    spustilo oba heatery — race condition mezi 4 nezávisle pollovanými `_used`
    senzory (rozdílný update_interval timing). Fix: `component.update` na všech
    čtyřech, synchronně po přepnutí.
- Test přerušen 2026-07-28 (pokračování příště) — Fáze 3–6 zbývají
- Pokračování 2026-07-30: Fáze 3 (floor/ceiling scénáře 3a-3d) i Fáze 4 T1/T2 dokončeny
  a bench-potvrzeny. Na žádost Lubora kompletně odstraněn testovací I2C blok (VL53L0X,
  BME280, I2C bus) — OI2, ne jen zakomentováno
- **BUG-003** nalezen a opraven: on-demand DS18B20 refresh intervaly (T2 při HEAT_MODE,
  T3/T4 při běžícím heateru) měly `interval: 1s`/`4s` místo komentářem tvrzené kadence
  5s/20s — hammerovaly sdílenou 1-Wire sběrnici rychleji, než DS18B20 stihne dokončit
  konverzi (~750ms). Fix: perioda intervalu opravena na 5s/20s.
- **BUG-004** nalezen a opraven (po BUG-003, T1/T2 stále nekonzistentně hlásily poruchu
  po bootu): root cause dohledán ve zdroji ESPHome (`scheduler.cpp` — náhodný 0-5s offset
  prvního spuštění, nezávisle pro senzor i error-check; `filter.cpp` — `send_first_at:1`
  publikuje i defaultní `NAN`, `send_every:5` pak drží tuhle hodnotu dalších 5 cyklů).
  3-vodičové zapojení DS18B20 (potvrzeno Luborem) vyloučilo parazitní napájení jako
  příčinu. Fix: 10s startup grace před vyhodnocením `err_t1_t2_fail`/`err_t3_t4_fail`.
- Fáze 5 (regrese): Lubor našel, že `DEFROST_ORDERED` sepne heatery i s `HEAT_MODE`
  OFF (Sim Outside 4°C/HEAT_MODE OFF + Sim Evaporator 17°C) — konkretizace už dřív
  otevřené OI6. Eskalováno na Architekta → S7 (ADR-009, DEFROST_ORDERED gated na
  HEAT_MODE)
- Implementace ADR-009: `bs_defrost` lambda +`&& id(bs_heat_mode).state`,
  `ARCHITECTURE.md §4.3` (3. podmínka), `DECISIONS.md` (+ADR-009 verbatim),
  `BACKLOG.md` (OI6→Done, +OI16 per-surface freeze gate hardening item)
- Fáze 5 (zbytek regresních bodů) a 5a (ADR-009 validace) dokončeny a bench-potvrzeny
- Lubor si všiml, že `_used` senzory pollují na plošných 5s bez ohledu na HEAT_MODE
  nebo skutečnou změnu zdroje (zaplevňuje log) — zapsáno jako **OI17** (event-driven
  refresh, stejný vzor jako BUG-002 fix), odloženo záměrně, aby nezpomalovalo
  rozjetý bench test. Zároveň zapsána **OI18** — souhrnná revize celého řetězce
  vzorkování → filtr → publikace (ne jen po jednotlivých článcích) před field
  nasazením.
- Fáze 6 (reálný SSR výstup se zkušební zátěží) dokončena a bench-potvrzena
- **Bench test uzavřen: OK s výjimkou T3/T4** (očekávané, blokováno na OI1 — chybí
  fyzické senzory). Všechny ostatní fáze (0-6, vč. 5a) bench-potvrzeny

**Výstupy:**
- `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` (ADR-008/009, BUG-001-004 fixy,
  T1/T2 adresy, I2C blok odstraněn) — **necommitnuto**, čeká na pokyn k commitu
- `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost_test01.yaml` (bring-up nástroj)
- `docs/Software_DevDoc/TEST_PLAN.md` (uzavřen), `BUGS.md` (+BUG-001..004),
  `DECISIONS.md` (+ADR-009), `ARCHITECTURE.md` (v1.7), `BACKLOG.md` (OI1/OI2/OI6/
  OI8/OI9/OI12 update, +OI15/OI16/OI17/OI18), `HANDOVER_20260730.md`

**Blokující:** žádné (T3/T4 samostatný OI1 úkol na fyzický hardware, neblokuje)
**Další session:** Commitnuto a pushnuto (`c6032fe`). Fronta: OI1 (T3/T4 fyzické
senzory) až budou k dispozici; entity rename session (ADR-005). ADR-004
(implementační session, vyžadovala OI10) proběhla jako **S10** — viz blok nahoře.
Mezitím proběhly i S8 (Architekt, doc-only, ADR-010 — výběr topných kabelů) a S9
(Architekt+Implementer, ADR-011 — umístění čidel, rename t_evap→t_gas_inlet).

---

## S9 — 2026-07-31 (Architekt → Implementer) — ADR-011: umístění čidel, t_evap→t_gas_inlet

> Handoff prompt referoval sám sebe jako "Session: S7" — Architekt window k tomu
> nemá živý přístup do SESSION_LOG.md (ADR-003) a S7 i S8 už byly obsazené (ADR-009
> 2026-07-30, ADR-010 2026-07-30). Implementer přečísloval na **S9**, ADR-011 text
> samotný zůstal beze změny (verbatim požadavek, vč. "Session: S7" uvnitř).

**Provedeno:**
- Reverse engineering venkovní jednotky Toshiba Shorai Edge (RAS-B13G3KVSG-E) —
  servisní manuál + principiální schéma (`Principle_Schematic.pdf`,
  `Defrost_Operation.pdf`) — zmapoval topologii 4-way valve reverse defrost cyklu
  a odhalil, že původně plánovaná pozice čidla výparníku (vnější strana voštiny)
  je fyzicky nevhodná.
- ADR-011 — čidlo T2 přemístěno (koncepčně) na plynový vývod výměníku (trubka b,
  Ø 9,5 mm, vývod C čtyřcestného ventilu) — nejčasnější bod detekce defrostu,
  na rozdíl od TE-analogické pozice (trubka a), která by dávala pozdní
  (ukončovací) signál. Čidlo T3 (chassis) umístěno na vnější stranu pánve,
  v mezeře meandru topného kabelu.
- Rename `t_evap` → `t_gas_inlet` proveden ve firmware YAML (id, name, lambda
  reference, komentáře) — cascadovalo i na `t_evap_used`→`t_gas_inlet_used` a
  `sim_t_evap`→`sim_t_gas_inlet` (validace handoffu `grep t_evap → 0 výskytů`
  by jinak selhala na těchto odvozených id). Čistě textový rename, žádná změna
  logiky/prahů. `esphome compile` bez chyb.
- `ARCHITECTURE.md` §3/§3.1/§3.2/§4.3/§6 aktualizovány — přidán i popis fyzického
  umístění T2/T3. Mimochodem opravena i stará chybná poznámka u §3.1 (tvrdila, že
  1s/4s intervaly jsou záměr kvůli mediánovému filtru — ve skutečnosti to byl
  BUG-003, opraveno S5 2026-07-30, ale komentář v dokumentu na to nebyl navázán).
- `BACKLOG.md`: B-VALID-01/02/03 vloženy verbatim. B-VALID-03 (per-surface freeze
  gate) obsahově duplicitní s existující **OI16** (ADR-009, Architekt window o ní
  nevěděla) — okomentováno, ne sloučeno (zachování verbatim požadavku).

**Výstupy:**
- `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` (rename, zkompilováno)
- `DECISIONS.md` (+ADR-011, patička → ADR-012), `ARCHITECTURE.md` (v1.8),
  `BACKLOG.md` (+B-VALID-01/02/03), `SESSION_LOG.md` (tento blok)
- **Necommitnuto** — Lubor avizoval ještě jeden zápis (nejasné zda k tomuto tématu
  nebo novému), čeká se na pokyn.

**Blokující:** žádné
**Další session:** Commit + push (na pokyn). Fyzická montáž čidel dle ADR-011 při
instalaci; B-VALID-01/02/03 vyhodnotit po 1. zimní sezóně.

---

## S8 — 2026-07-30 (Architekt) — ADR-010: topné kabely (samoregulační 10/20 W/m)

> Doc-only session, žádná změna firmware/YAML. Handoff předpokládal další volné OI
> číslo "OI17" — v BACKLOGu už bylo obsazené (OI17/OI18 vznikly týž den ve
> společné bench-test session S5, Architekt window o nich nevěděl, viz ADR-003).
> Implementer přečísloval novou položku na **OI19**.

**Provedeno:**
- Rozvahová session (výběr topného kabelu, ne firmware). Rešerší potvrzeno: sériové
  konstantní kabely nelze řezat a nesmí se smotávat; samoregulační jsou řezatelné
  a self-limiting.
- ADR-010 — samoregulační kabel na oba okruhy: odtok 10 W/m / 7 m (DN30, spirála +
  rezerva pro přípoj do svodu), chassis 20 W/m / ~2,5 m. Klíčová vazba: časové řízení
  bez teplotní zpětné vazby je bezpečné jen proto, že kabel je fail-safe (self-limiting)
  → invariant „neměnit typ kabelu bez revize řízení".
- Field data 1. sezóny (mrzla jednotka + napojení na odpad, ne potrubí) potvrzuje
  těžiště na chassis/junction.
- OI15 (absolutní max-on-time guard) — priorita snížena z "Hardening" na
  "Nice-to-have", kabel je fail-safe proti přehřátí; guard zůstává jen jako
  pojistka proti plýtvání energií.
- ARCHITECTURE.md nemá dedikovanou HW/BOM sekci pro topné okruhy (jen GPIO tabulka
  DO1/DO2 = SSR výstupy) — dle handoff instrukce (Krok 3, podmíněně) vynecháno,
  poznamenáno zde.

**Výstupy:**
- `DECISIONS.md` (+ADR-010, patička → ADR-011), `BACKLOG.md` (nová OI19 thaw mode,
  OI15 priorita → nice-to-have), `SESSION_LOG.md` (tento blok).
- `docs/Thermal Cable/` — datasheety (K&V srKABEL, srKABEL PRO) přiloženy Luborem,
  commitnuty v S5/S7 rozsahu (`c6032fe`) ještě před touto session.

**Blokující:** žádné
**Další session:** Implementační dle BACKLOG (ADR-004 led_sequencer port / entity
rename ADR-005). Finální délka chassis kabelu se potvrdí po fyzické montáži.

---

## S7 — 2026-07-30 (Architekt) — ADR-009: DEFROST_ORDERED gated na HEAT_MODE

> Handoff prompt referoval sám sebe jako "Session: S6" — Architekt window k tomu
> nemá živý přístup do SESSION_LOG.md (viz ADR-003, sync je Luborova ruční role),
> takže neví, že S6 je už obsazené (ADR-008, 2026-07-22). Implementer přečísloval
> na S7 (další volné číslo), ADR-009 text samotný zůstal beze změny (verbatim
> požadavek) — jen tenhle SESSION_LOG blok má správné číslo.

**Provedeno:**
- Podnět z S5 (Implementer/Lubor bench test, Fáze 5 regrese): `DEFROST_ORDERED`
  kontroluje jen `main_system_enabled`, na `HEAT_MODE` se vůbec neptá — Sim Outside
  4°C (HEAT_MODE OFF) + Sim Evaporator 17°C přesto spustí oba heatery. Konkretizace
  už dřív otevřené OI6 (code review S3, "K prověření").
- ADR-009 — `DEFROST_ORDERED` gated na `HEAT_MODE`: do AND podmínky `bs_defrost`
  přidáno `&& id(bs_heat_mode).state`. Fyzikální zdůvodnění: DEFROST_ORDERED
  detekuje produkci kondenzátu (jednotka odmrazuje), ne riziko zamrznutí —
  HEAT_MODE je proxy pro to druhé a obě podmínky musí platit současně, aby topení
  dávalo smysl. Gate umístěn uvnitř `bs_defrost` (jedno místo pro policy), ne na
  edge-triggeru — floor (ADR-008) doběhne i při poklesu HEAT_MODE uprostřed cyklu.
- Známé omezení zapsáno do ADR-009: HEAT_MODE prahy (2/4°C) jsou proxy "chladno",
  ne měřený bod mrazu povrchu — přesnější per-surface gate odložen do BACKLOGu
  (nová OI16, k pozorování v zimní sezóně).

**Výstupy:**
- DECISIONS.md (+ADR-009), ARCHITECTURE.md (§4.3, v1.7), BACKLOG.md (OI6→Done,
  nová OI16), SESSION_LOG.md (tento blok).
- Handoff prompt pro CC (implementace ADR-009 — jednořádková změna v `bs_defrost`
  + doc-commit).

**Blokující:** žádné
**Další session:** Implementační pokračování S5 (Implementer/CC) — ADR-009 do
firmware YAML, bench potvrzení (Sim Outside/Evaporator scénář z podnětu).

---

## S6 — 2026-07-22 (Architekt) — ADR-008: minimální doba topení (floor)

**Provedeno:**
- Podnět z S5 (Implementer): DEFROST_ORDERED nemá hysterezi (na rozdíl od HEAT_MODE),
  takže čistě condition-driven cyklus z ADR-007 je zranitelný vůči šumu kolem prahu —
  krátké zakolísání by vypnulo heater dřív, než topný kabel při tepelné setrvačnosti
  šasi/trubky cokoli ohřeje.
- ADR-008 (rozšiřuje ADR-007, nesuperseduje) — dva prahy místo jednoho:
  garantovaná minimální doba (floor) + bezpečnostní strop (ceiling). Dvoufázová
  struktura `turn_on → delay(floor) → wait_until(!DEFROST_ORDERED, timeout=ceiling−floor)
  → turn_off`.
- Nové entity `chassis_time_floor`/`drain_time_floor` (1–10 min, default 2/3).
  Přejmenováno `chassis_time_min`→`chassis_time_ceiling`/`drain_time_min`→
  `drain_time_ceiling` (15–60 min, default 20/20) — `_time_min` bylo po zavedení
  floor zavádějící (označovalo maximum).
- Retrigger: gate `defrost_running == false` na edge-triggeru zrušen — nová hrana
  restartuje cyklus přes `mode: restart` místo aby byla spolknuta, dokud běží pomalejší
  z dvojice chassis/drain. `defrost_running` degraduje na čistý stavový příznak.
- Akceptované omezení (bod 8): retrigger resetuje rozpočet stropu, kmitající podmínka
  může topení protahovat — ceiling chrání jen proti zaseknuté, ne kmitající podmínce.
  Řešení (nezávislý max on-time, Windows ERR8/OI25 precedent) odloženo do BACKLOGu,
  k pozorování v zimní sezóně.
- ARCHITECTURE.md §4.4 a §6 přepsány (v1.4). Rename friendly names odložen na
  ADR-005 execution session (spolu s ADR-007's odloženým rename).

**Výstupy:**
- DECISIONS.md (+ADR-008, cross-ref u ADR-007), ARCHITECTURE.md (§4.4, §6, v1.4),
  BACKLOG.md (OI9 rozšířena, nová OI15), SESSION_LOG.md (tento blok).
- Handoff prompt pro CC (implementace ADR-008 ve firmware + OI12 v jednom kroku).

**Blokující:** žádné
**Další session:** Implementační pokračování S5 (Implementer/CC) — ADR-008 do
firmware YAML, dokončení, bench test (Simulation Mode) dle TEST_PLAN.md.

---

## S4 — 2026-07-21 (Architekt) — formální ADR pro F1/F4 + doc-commit

**Provedeno:**
- ADR-006 (OI8/F1) — boot-resume varianta B: resume přes edge-trigger, bez on_boot
  resume, bez restore_value na defrost_running; F3/OI12 (send_first_at) jako pojmenovaná
  závislost ohraničující zmeškání na ~1 poll.
- ADR-007 (OI9/F4, supersedes OI3) — defrost condition-driven: pevný delay →
  wait_until(!DEFROST_ORDERED, timeout = *_time_min jako strop). Rename labelů odložen
  na ADR-005 execution session.
- ARCHITECTURE.md §4.4 přepsán dle ADR-006/007.
- BACKLOG: OI8/OI9 → ADR Accepted / 🔧 realizace; OI12 povýšeno na závislost OI8.

**Výstupy:**
- DECISIONS.md (+ADR-006, +ADR-007), ARCHITECTURE.md (§4.4), BACKLOG.md (OI8/OI9/OI12),
  SESSION_LOG.md (tento blok).

**Blokující:** žádné
**Další session:** Implementační session — defrost změny (wait_until + timeout, F3
send_first_at, oprava YAML komentáře "manual start only") + ADR-004 led_sequencer port
dle BACKLOG. Poté entity rename (ADR-005), kde padne i label rename z ADR-007.

---

## S3 — 2026-07-10 (Implementer/CC) — Code Review firmware YAML

**Provedeno:**
- Kompletní line-by-line review `ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` (funkčnost +
  bezpečnost) → `docs/Software_DevDoc/Code_review_20260710.md` (8 nálezů: F1–F5, S1–S4)
- Křížové srovnání s `ESP32-D0WD-V3_Gar_Windows_02.yaml` (Lubor dočasně vložil,
  po review smazáno) — potvrdilo/posílilo více nálezů, odhalilo `led_sequencer`
  jako custom komponentu (prerekvizita pro ADR-004) a rukopisné konvence k převzetí
- Společný průchod s Luborem bod po bodu, všechny nálezy rozhodnuty:
  - F1 (boot auto-restart defrostu) → obnovit stav před rebootem (restore_value +
    on_boot resume), ne "manual start only"
  - S1 (otevřený fallback AP) → přidat heslo, konzistentně s Windows
  - S2 (web_server bez auth) → ponechat pro bench, řešit před Field Deployment
  - F4 (defrost timer vs. trvání podmínky) → `wait_until` s timeout stropem místo
    pevného `delay:`; naming HA labelů odloženo na ADR-005 execution session
  - Zbytek (F2/F3/F5, S3-logger, S4) beze změny v kategorizaci — 🔧 Implementer,
    žádné ADR nutné
- `BACKLOG.md` naplněn: OI8–OI14 (nové), OI3 vyřešena → Done, OI4 přeformulována

**Výstupy:**
- `Code_review_20260710.md` (nový, vč. sekce F — resolutions)
- `BACKLOG.md` (+OI8–OI14, OI3→Done, OI4 update)

**Blokující:** žádné
**Další session:** Formální ADR zápis Architektem pro F1 (OI8) a F4 (OI9) — obě mění
zdokumentované chování v `ARCHITECTURE.md` §4.4, rozhodnutí je ale už hotové (jen
schválení). Poté implementační session(y) — viz `BACKLOG.md` OI8–OI14.

---

## S2 — 2026-07-10 (Architekt) — improvement review vs Garage_Windows

Rozhodovací session. Bez zásahu do firmwaru.

**Provedeno:**
- ADR-003 sync-first + revision header = SESSION_LOG.
- ADR-004 port led_sequencer: ERR 5 separátních kódů (wifi/T1/T2/T3/T4), WD coarse
  (disabled/defrost/armed/idle, 1 aktivní stav).
- ADR-005 HA-boundary: ESP derivuje vše, CZ názvy, device_id grouping, "Stav systému"
  + "Kód chyby" text senzory; rename = samostatná session.
- Review nálezy → BACKLOG: OI5 (uptime), OI6 (heat_mode/defrost decoupling).
- Doc fix: GPIO4 není strapping pin (jen GPIO2) — README + ARCHITECTURE §2.2.

**Fronta:** (1) tento doc-commit; (2) implementační session (led_sequencer port, ERR
split, WD lambda, uptime); (3) entity rename session (ADR-005).

**Výstupy:**
- `DECISIONS.md` (+ADR-003, +ADR-004, +ADR-005), `BACKLOG.md` (+OI5, +OI6),
  `README.md` (strapping pin fix, error-code tabulka na 5 kódů),
  `HANDOVER_20260710.md` (nový)

**Blokující:** žádné
**Další session:** Implementační session (led_sequencer port, ERR split na
err_t1..t4, WD výběrová lambda, uptime senzor) — viz HANDOVER_20260710.md.

---

## S1 — 2026-07-10

**Role:** Architekt (kick-off)
**Kontext využit:** N/A (kick-off session)

**Provedeno:**
- Rozhodnutí o restartu projektu pod standardizovaným AI workflow (ADR-001) — štíhlý
  seed z Garage_Windows, ne plné klonování
- Rozhodnutí o odlehčeném přístupu ke kvalitě (good-enough, rychlejší iterace)
- Založen doc seed: `PROJECT_VISION.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `BACKLOG.md`
  (skeleton s předběžnými položkami z ARCHITECTURE review), `DECISIONS.md` (ADR-001),
  `BUGS.md` (skeleton), `WORKING_AGREEMENT.md`, `Prompts/PROMPT_architekt.md`,
  `Prompts/PROMPT_implementer.md`, `SESSION_LOG.md` (tento), `Software_DevDoc_structure.md`
- `ARCHITECTURE.md` sestaven na základě čtení existujícího YAML — zdokumentována HW mapa,
  senzorová vrstva (vč. simulační "used" indirection), řídicí logika (HEAT_MODE,
  DEFROST_ORDERED, defrost scripty), error handling, konfigurace, a identifikován
  předběžný technický dluh (TODO adresy senzorů, testovací I2C blok, otázka
  synchronizace konce defrost cyklu, asymetrie error-check intervalů)
- Rozhodnutí o sloučení rolí Implementer a Claude Code do jedné (ADR-002) — samostatná
  Sonnet-Implementer session se v praxi ukázala jako zbytečný mezikrok, CC realizuje
  roli Implementera přímo. Potvrzeno s Ownerem: Lubor stále ručně kopíruje handoff
  prompt z Architekt okna do CC (jen odpadlo Sonnet okno mezi nimi)
- V návaznosti aktualizovány: `WORKING_AGREEMENT.md` (v1.0 → v1.1: pyramida, role,
  tabulka souborů, workflow, jazyk, volba modelu, AP-COM-002), `Prompts/PROMPT_implementer.md`
  (přepsán pro CC), `Prompts/PROMPT_architekt.md` (handoff cílí přímo na CC),
  `DECISIONS.md` (+ADR-002), `Software_DevDoc_structure.md` (poznámka o roli modelu)

**Výstupy:**
- Celý doc seed vč. následných úprav (viz výše) — připraven ke commitu

**Pokračování S1 (role Implementer/CC) — konzistenční revize doc seedu:**
- Lubor ručně založil prázdné `Test Results/` a `#Archive/` (structure.md aktualizován,
  přestal tvrdit "not yet created")
- Oprava vnitřního rozporu v pojmenování HANDOVER souboru — sjednoceno na jednu
  konvenci (`HANDOVER_YYYYMMDD.md`, aktivní soubor už nese datum, archivuje se beze
  změny jména): `WORKING_AGREEMENT.md` (v1.1 → v1.2, §3.2 + §2 notace), oba
  `Prompts/PROMPT_*.md`, `Software_DevDoc_structure.md`
- Oprava chybného cross-ref v `BACKLOG.md` (OI4: ARCHITECTURE §7.4 → §5)
- Sladění fázového modelu `PROJECT_VISION.md` (v1.0 → v1.1) s `ROADMAP.md` (doplněna
  fáze 3 — Bench Validation)
- `ROADMAP.md` (v1.0 → v1.1): Definition of Done pro Fázi 1 odškrtnuta dle reálného stavu
- Ponecháno pro společnou revizi s Architektem: faktická chyba v `ARCHITECTURE.md` §2.2
  (GPIO4 mylně označen jako strapping pin — jen GPIO2 jím skutečně je)

**Blokující:** žádné
**Další session:** Code review existujícího firmware (nová session, role Implementer/CC) →
naplnění BACKLOG.md konkrétními nálezy. Dále: společná revize ARCHITECTURE.md §2.2 s
Architektem (GPIO4 strapping pin tvrzení).
