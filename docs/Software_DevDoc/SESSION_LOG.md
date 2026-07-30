# SESSION LOG — Garage_Drain_Defrost

> Chronologický log session (nejnovější nahoře). Append-only.

---

## S5 — 2026-07-21 až 2026-07-30 (Implementer/CC) — defrost implementace + bench test (dokončeno)

> Nejdelší session dosud, přes více dnů (Lubor testoval na hotovém HW, ne breadboardu).
> Číslování protíná S6 (Architekt, ADR-008 uprostřed) a S7 (Architekt, ADR-009
> uprostřed) — viz oba bloky níže pro kontext.

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

**Blokující:** žádné — čeká se na pokyn k commitu (T3/T4 samo o sobě neblokuje,
je to samostatný OI1 úkol na fyzický hardware)
**Další session:** Commit + push celého rozsahu S5/S7 (na pokyn). Poté: OI1 (T3/T4
fyzické senzory) až budou k dispozici; implementační session pro ADR-004 (vyžaduje
OI10); entity rename session (ADR-005).

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
