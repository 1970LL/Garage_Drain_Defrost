# DECISIONS — Garage_Drain_Defrost

> **Účel:** Log architektonických rozhodnutí (ADR). Append-only, nikdy se nepřepisuje.
> **Vlastník:** Architekt.
> **Pravidlo:** ADR jsou číslované, nikdy se nepřečíslovávají. Zrušená rozhodnutí dostanou
> status *Superseded by ADR-NNN*, nemažou se.

---

### ADR-001 — Restart projektu pod standardizovaným AI workflow (štíhlý seed)

**Status:** Accepted
**Datum:** 2026-07-10
**Kontext:**

Projekt Garage_Drain_Defrost byl původně vyvíjen s pomocí ChatGPT (Ctrl+C/Ctrl+V),
ladění probíhalo na breadboardu, bez formální dokumentační struktury. Hardware je nyní
téměř hotový (custom PCB s ESP32-WROOM-32UE), struktura projektu existuje ve VS Code,
Git sync je funkční.

Paralelně byl dokončen projekt Garage_Windows se zavedeným standardem AI-asistovaného
vývoje (role Owner/Architekt/Implementer/CC, dokumentační aparát v `docs/Software_DevDoc/`).

**Rozhodnutí:**

Restartovat Drain_Defrost pod stejným standardem, ale jako **štíhlý seed**, ne plnou
kopii Garage_Windows dokumentace:

- Přenést beze změny: role, workflow pravidla (`WORKING_AGREEMENT.md`), starterové
  prompty (`Prompts/`)
- Založit od nuly, štíhlé: `PROJECT_VISION.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
  `BACKLOG.md`, `DECISIONS.md` (tento soubor), `BUGS.md`, `SESSION_LOG.md`
- Vynechat prozatím: `Test Results/` aparát, CT/hypothesis analýzy (Windows-specifické,
  nerelevantní), archiv

**Zdůvodnění:**

Drain_Defrost je funkčně jednodušší než Garage_Windows (prahová logika + časovače,
žádná uzavřená regulační smyčka, žádný pohyblivý motor pod napětím). Plné klonování
40-session dokumentačního aparátu by vytvořilo prázdná místa bez obsahu a zbytečnou
režii. Zvolen byl také **odlehčený přístup ke kvalitě** (viz PROJECT_VISION.md §Přístup
ke kvalitě) — rychlejší iterace, méně formální ADR/test aparát než u Windows.

**Důsledky:**

- Dokumentace poroste organicky podle potřeby, ne preventivně do hloubky
- Eskalace na plnou ADR analýzu s variantami A/B/C není nutná pro drobná rozhodnutí
- Testovací aparát bude checklist, ne vícebodová akceptační sada
- Následující krok: code review existujícího YAML v nové session, výstup naplní BACKLOG.md

---

### ADR-002 — Sloučení rolí Implementer a Claude Code

**Status:** Accepted
**Datum:** 2026-07-10
**Kontext:**

WORKING_AGREEMENT (v1.0, ADR-001) přenesl z Garage_Windows model se čtyřmi vrstvami:
Project Owner → Architekt → Implementer (samostatná Sonnet chat session) → Claude Code
(mechanická realizace). V praxi se ukázalo, že samostatná Implementer session je
zbytečný mezikrok — Lubor stejně ručně kopíroval handoff prompt dál mezi okny a
Implementer sám nepsal kód, jen přeformulovával zadání pro CC.

**Rozhodnutí:**

Role Implementer a Claude Code se slučují do jedné role — **Implementer = Claude Code**.
CC nyní přímo:
- realizuje handoff prompt od Architekta (Lubor ho ručně kopíruje z Architekt okna do CC)
- dělá root cause analýzu a debug
- píše a aktualizuje `TEST_PLAN.md`, `BUGS.md`, `HANDOVER_YYYYMMDD.md`, `SESSION_LOG.md`
- commituje a pushuje

Handoff prompt (Architekt → Implementer) a dřívější CC prompt (Implementer → CC) se
slučují do jednoho formátu — žádný dvoustupňový překlad zadání.

**Zdůvodnění:**

Osvědčeno v praxi po zrušení Sonnet-Implementer session — CC zvládá roli implementátora
bez ztráty kvality, ušetří se jeden krok ruční relace promptů a jeden kontext-okno cyklus.

**Důsledky:**

- `WORKING_AGREEMENT.md` aktualizován na v1.1 (pyramida, role §1, tabulka souborů §2,
  workflow §3, jazyk §4, model §5, AP-COM-002)
- `Prompts/PROMPT_implementer.md` přepsán — cílí na CC, ne na Sonnet okno
- `Prompts/PROMPT_architekt.md` upraven — handoff jde přímo pro CC
- Beze změny zůstává: Lubor je stále ruční prostředník mezi Architekt oknem a CC
  (žádná automatizace předání zatím není zavedena)

---

### ADR-003 — Sync-first rituál + revision header = SESSION_LOG

**Status:** Accepted
**Datum:** 2026-07-10
**Session:** S2 (Architekt)
**Kontext:**

Doc-sync disciplína zděděna z Windows (ADR-019), ale bez vlastní ADR kotvy a bez
definovaného anchoru. Review kick-off narazil na nemožnost ověřit čerstvost před
Sync Now. Windows navíc ukázal, že ROADMAP-patička driftuje (S36: ROADMAP S25 vs.
reálné S33) → nespolehlivý anchor.

**Rozhodnutí:**

1. Každý start session = Sync Now + ověření revision headeru PŘED čtením obsahu.
2. Revision header = poslední `## SN` blok v `SESSION_LOG.md` (jediný anchor).
   ROADMAP se pro freshness nepoužívá.

**Důsledky:**

- Freshness z jednoho vysokofrekvenčního zdroje; odpadá false-stale šum.

**Odkazy:** ADR-019 (Windows), WORKING_AGREEMENT §5.

---

### ADR-004 — Port led_sequencer + DD pattern sada

**Status:** Accepted
**Datum:** 2026-07-10
**Session:** S2 (Architekt)
**Kontext:**

ERR signalizace dnes ruční YAML script se spárovanými globals; WD LED nesignalizuje
provozní stav topení. Windows má univerzální `led_sequencer` (ADR-010) psaný pro
reuse — dvě instance, deklarativní patterny, `set_pattern(id, bool)`.

**Rozhodnutí:**

Portovat `led_sequencer` beze změny enginu.

ERR LED (GPIO2), sériově (F3, gaps 1500/2000):

| Kód | Chyba | Pattern |
|---|---|---|
| `err_wifi` | WiFi lost | 1×(300/300) |
| `err_t1` | T1 outside fail | 2×(300/300) |
| `err_t2` | T2 evaporator fail | 3×(300/300) |
| `err_t3` | T3 chassis fail | 4×(300/300) |
| `err_t4` | T4 drain fail | 5×(300/300) |

WD LED (GPIO4), mutually-exclusive stavy, všechny continuous, engine drží jeden
aktivní (priorita shora):

| Stav | Podmínka | Pattern |
|---|---|---|
| `wd_disabled` | MAIN_SWITCH off | 100/900 continuous |
| `wd_defrost` | heater ON / defrost_running | 100/100 continuous |
| `wd_armed` | HEAT_MODE on, žádný heater | 2×(150/150)+900 off continuous |
| `wd_idle` | enabled, HEAT_MODE off | 500/500 continuous |

Výběrová lambda: `!main → disabled; else heater_on → defrost; else heat_mode → armed; else idle`.

Detekční vrstva se MUSÍ rozdělit ze spárovaných globals (`err_t1_t2_fail` /
`err_t3_t4_fail`) na per-senzor `err_t1..t4` — patří do implementační session.

**Zdůvodnění:**

Vědomá volba: ERR číslování dle README pořadí (ne frequency-based jako Windows ADR-006).

**Odkazy:** Windows ADR-010, ARCHITECTURE §8.3/§10.

---

### ADR-005 — HA-boundary + entity-expozice konvence

**Status:** Accepted
**Datum:** 2026-07-10
**Session:** S2 (Architekt)
**Kontext:**

HA je tenká vrstva bez transformace; DD dnes exponuje entity vývojářsky (EN +
kódové názvy), ploše bez device_id, bez derivovaného stavu.

**Rozhodnutí:**

ESP = veškerá derivace; HA jen zobrazuje.

1. CZ friendly names.
2. Grouping přes device_id sub-devices.
3. ESP-side "Stav systému" (Vypnuto/Klid/Naarmováno/Odmrazování — zrcadlí WD
   výběrovou lambdu) + "Kód chyby" text senzory.
4. Human-readable formátování (uptime d/h/m) HA-side = jediná posvěcená výjimka
   (Windows OI28 precedent).

Exekuce (per-entity rename + grouping) = samostatná session.

**Odkazy:** Windows ARCHITECTURE §10, ADR-017.

---

### ADR-006 — Boot-resume defrostu přes edge-trigger (varianta B)

**Status:** Accepted
**Datum:** 2026-07-21
**Session:** S4 (Architekt)
**Nález:** F1 / OI8 (Code_review_20260710.md)

**Kontext:**

YAML komentář slibuje "manual start only", reálné chování je ale auto-resume defrostu
po rebootu: edge-trigger `DEFROST_ORDERED` má `static bool last`, který se při bootu
inicializuje na `false` → první `DEFROST_ORDERED == true` po bootu je vzestupná hrana
→ `run_defrost` fírne. `on_boot` `script.stop` tomu budoucímu spuštění nebrání (ověřeno,
Windows TC-6.1). F3 (DS18B20 ~10 min NaN po bootu) tomu accidentally brání a posouvá
resume o ~10 min. Zvažovány dvě cesty: A (explicitní `on_boot` resume, plný strop bez
podmínky) vs B (resume ponechán na edge-triggeru + condition-gated cyklus dle ADR-007).

**Rozhodnutí:**

Varianta B. Auto-resume po rebootu je **záměrné** a realizuje ho výhradně existující
edge-trigger `DEFROST_ORDERED` (strukturální záruka: `static bool last = false` při
bootu → první přečtené true = rising edge → `run_defrost`).

- `on_boot` explicitní resume se **nepřidává** (pod B by fizznul — `!bs_defrost.state`
  je při NaN senzorech true → `wait_until` v cyklu skončí okamžitě; byl by to zavádějící
  dead-code).
- `restore_value: true` na `defrost_running` se **nepřidává** (pod B nic neřídí).
- **Závislost:** F3 fix (`send_first_at: 1`, OI12) ohraničuje nejhorší případ zmeškání
  z ~10 min na ~1 poll interval. Bez něj B funguje, jen s větším oknem.
- YAML komentář "manual start only" se opraví na pravdivý popis.

**Zdůvodnění:**

Promeškání max jednoho defrost cyklu je akceptovatelné (jednotka krátce bez asistence,
ne nechtěné topení — heatery `ALWAYS_OFF`). Záruku "další už nepromeškáme" dává
strukturálně edge-trigger; F3 fix zmenší i to jediné okno. Leanness: žádný redundantní
boot kód, žádný NVS zápis, dokumentace odpovídá realitě.

**Důsledky:**

- `ARCHITECTURE.md §4.4` — doplněno boot chování.
- OI8 → 🔧 Implementer (oprava komentáře + závislost OI12).
- OI12 (F3) povýšeno z "kdykoli" na přednostní závislost boot-resume.

**Odkazy:** Code_review_20260710.md §F1/§F3, ADR-007, ADR-004.

---

### ADR-007 — Defrost cyklus condition-driven s bezpečnostním stropem

**Status:** Accepted
**Datum:** 2026-07-21
**Session:** S4 (Architekt)
**Nález:** F4 / OI9 (Code_review_20260710.md) — **supersedes OI3**
**Rozšířeno:** ADR-008

**Kontext:**

`defrost_chassis_cycle`/`defrost_drain_cycle` běží po pevný `delay:`
(`chassis_time_min`/`drain_time_min`) nezávisle na tom, jak dlouho reálná podmínka
`DEFROST_ORDERED` trvá. Pokud odmrazovací cyklus jednotky trvá déle, topení zhasne na
časovači, i když jednotka ještě odmrazuje; edge-triggered → re-trigger až po poklesu a
novém náběhu podmínky. Přesnější formulace stávajícího OI3.

**Rozhodnutí:**

Nahradit pevný `delay:` za `wait_until` s `condition: !bs_defrost.state` a `timeout:`
= `chassis_time_min`/`drain_time_min`. Topení běží **dokud `DEFROST_ORDERED` trvá**,
s min-časem jako **bezpečnostním stropem** (maximální doba topení), ne pevnou dobou.

- Sémantika HA number entit `chassis_time_min`/`drain_time_min` se mění z "pevná doba"
  na "maximální doba (strop)".
- **Rename HA labelů** ("...Time (min)" → "...Max Time (min)") se **odkládá** na
  ADR-005 execution session (entity rename) — nepředbíhat, riziko dvojí změny.

**Zdůvodnění:**

Topení kopíruje skutečné trvání odmrazovacího cyklu → lepší ochrana i úspora; strop je
fail-safe max on-time proti zaseknuté podmínce.

**Důsledky:**

- `ARCHITECTURE.md §4.4` — přepsán popis defrost cyklů.
- OI3 → Done (supersedes). OI9 → 🔧 Implementer (realizace).
- Naming poznámka do ADR-005 fronty.
- Interakce s ADR-006/B: boot-resumnutý cyklus používá stejný `wait_until`; při NaN
  senzorech (před F3 fixem) fizzne — pod variantou B akceptováno.

**Odkazy:** Code_review_20260710.md §F4, OI3, ADR-006, ADR-005 (deferred rename).

---

### ADR-008 — Minimální doba topení (floor) vedle bezpečnostního stropu

**Status:** Accepted
**Datum:** 2026-07-22
**Session:** S5 (Architekt)
**Rozšiřuje:** ADR-007 (nesuperseduje — mění počet mezí, ne princip)

**Kontext:**

ADR-007 udělal defrost cyklus čistě condition-driven: heater běží, dokud trvá
`DEFROST_ORDERED`, se stropem jako fail-safe. Na rozdíl od HEAT_MODE ale
`DEFROST_ORDERED` nemá hysterezi — je to prostá AND podmínka (`t_evap_used ≥ def_abs_th`
a zároveň `(t_evap_used − t_outside_used) ≥ def_dt_th`) bez dvojitého prahu. Krátké
zakolísání kolem prahu (šum senzoru, přirozené kolísání defrostu jednotky) proto vypne
heater téměř okamžitě — dřív, než topný kabel při tepelné setrvačnosti šasi/trubky
stihne cokoli reálně ohřát.

Vlastní defrost cyklus jednotky odhadován na ~1–5 min; dynamika náběhu topných kabelů
zatím není změřená.

**Rozhodnutí:**

1. **Dva prahy místo jednoho** pro `defrost_chassis_cycle` i `defrost_drain_cycle`:
   garantovaná minimální doba (floor) + bezpečnostní strop (ceiling, princip beze změny
   z ADR-007). Dvoufázová struktura:
   `turn_on → delay(floor) → wait_until(!DEFROST_ORDERED, timeout = ceiling − floor) → turn_off`

2. **Invariant:** timeout druhé fáze = `ceiling − floor`, nikdy `ceiling`. Celkový strop
   cyklu zůstává `ceiling` — floor je jeho součást, ne přídavek.

3. **Nové entity:** `chassis_time_floor` / `drain_time_floor`, rozsah **1–10 min**,
   defaulty 2 / 3 min.

4. **Přejmenování stropů:** `chassis_time_min` → `chassis_time_ceiling`,
   `drain_time_min` → `drain_time_ceiling`. Rozsah **1–60 → 15–60 min**,
   defaulty **2 / 3 → 20 / 20 min**.
   *Důvod přejmenování:* `_time_min` je po zavedení floor aktivně zavádějící — označuje
   maximum a čte se jako „minimum" i jako jednotka „minuty". Pár floor/ceiling odpovídá
   slovníku dokumentace („podlaha"/„strop"). Jednotka patří do `unit_of_measurement:`
   a §6 tabulky, ne do ID.
   *Důvod zvednutí rozsahu:* bez rozestoupení mezí by `wait_until` okno vyšlo nulové
   a condition-driven chování z ADR-007 by zaniklo.

5. **Nepřekrývající se rozsahy** (floor ≤ 10, ceiling ≥ 15) činí `floor < ceiling`
   strukturálně nesplnitelným → podtečení `uint32_t` v timeout výpočtu nemůže nastat.
   Defenzivní clamp v lambdě se přesto ponechá jako levná pojistka pro případ budoucí
   změny rozsahů.

6. **Retrigger:** gate `defrost_running == false` na edge-triggeru `DEFROST_ORDERED`
   se **ruší**. Nová vzestupná hrana restartuje cyklus přes `mode: restart`
   (`switch.turn_on` na již zapnutém heateru je idempotentní). `defrost_running`
   degraduje z interlocku na čistý stavový příznak — což je zároveň to, co potřebuje
   ADR-004 pro WD stav „defrost".
   *Důvod:* chassis a drain mají různé stropy a končí nesoučasně; dokud běžel ten
   pomalejší, byla nová hrana spolknutá a rychlejší heater se znovu nenastartoval.

7. **Floor není bezpečnostní override.** `script.stop` z hlavního vypínače (§4.5)
   zabíjí i běžící `delay:` — nouzová cesta zůstává nedotčená.

8. **Akceptované omezení:** retrigger resetuje rozpočet stropu. Podmínka kmitající
   s periodou kratší než floor tedy může topení protahovat neomezeně. Strop chrání
   proti *zaseknuté* podmínce, ne proti *kmitající*. Vědomě akceptováno v odlehčeném
   režimu — chování nelze rozumně odhadnout ani nasimulovat na bench, závisí na šumu
   DS18B20 v reálné instalaci a na dynamice konkrétní jednotky.
   **Re-trigger podmínka:** pozorování v zimní sezóně; při výskytu → nezávislý
   absolutní max on-time neresetovaný retriggerem (precedent Garage_Windows ERR8 /
   OI25, `max_on_time_ms` v C++ driveru). BACKLOG položka, ne blokující.

9. **Rename HA labelů (friendly names) odložen** na ADR-005 execution session — celá
   čtveřice (floor + ceiling, chassis + drain) najednou, ne po kouskách. Přejmenování
   `id:` v bodě 4 je interní a na HA `entity_id` nemá vliv (ten se odvozuje z `name:`).

**Zdůvodnění:**

Minimální doba je analogií dvojitého prahu u HEAT_MODE, jen realizovaná časem místo
teploty. Bez ní je condition-driven cyklus zranitelný vůči šumu právě proto, že
`DEFROST_ORDERED` hysterezi nemá. Není to revert ADR-007 — strop zůstává potřebný
beze změny, přidává se dolní mez vedle horní.

**Důsledky:**

- `ARCHITECTURE.md §4.4` přepsán, `§6` tabulka přepsána (v1.4).
- Změna `id:` u stropů mění hash NVS preference → uložené hodnoty 2/3 min se neobnoví
  a entity nastartují na novém defaultu 20 min. Přejmenování je tedy zároveň čistou
  migrací defaultů mimo starý rozsah; žádný explicitní NVS zásah není potřeba.
- ADR-007 zůstává `Accepted`, dostává křížový odkaz „*Rozšířeno: ADR-008*".
- ADR-006 beze změny — při NaN po bootu je `bs_defrost` false, žádná hrana,
  `run_defrost` nefírne, floor se nespustí. Žádná kolize.
- BACKLOG: OI9 rozšířena o floor + rename; nová hardening položka dle bodu 8.

**Odkazy:** ADR-007, ADR-006, ADR-004, ADR-005 (odložený rename friendly names),
Windows OI25/ERR8 (precedent absolutního max on-time).

---

### ADR-009 — DEFROST_ORDERED gated na HEAT_MODE (freeze-relevance gate)

**Status:** Accepted
**Datum:** 2026-07-30
**Session:** S6 (Architekt)
**Nález:** OI6 (code review S3), konkretizováno bench testem F5/regrese

**Kontext:**

`DEFROST_ORDERED` (bs_defrost) dnes kontroluje jen `main_system_enabled` a vlastní dva
prahy (`def_abs_th`, `def_dt_th`); na `HEAT_MODE` (bs_heat_mode) se neptá. Bench test
(Sim Mode): Outside = 4 °C → HEAT_MODE OFF (nad heat_off_th), Evaporator = 17 °C →
DEFROST_ORDERED ON → oba heatery sepnou, ačkoli mráz nehrozí. Chování odpovídá kódu i
ARCHITECTURE §4.3, není to regrese ze S5. Od ADR-008 (garantovaný floor 2/3 min) má
spurious trigger konkrétní cenu: heater poběží nejméně floor dobu, i když je +4 °C.

Fyzika: `DEFROST_ORDERED` detekuje *produkci kondenzátu* (jednotka odmrazuje), nikoli
*riziko zamrznutí* (povrch chassis/drain u bodu mrazu). Obě podmínky jsou nezávislé a
pro užitečnost topení musí platit obě. Navíc dynamic polling (ARCHITECTURE §3.1) už je
na HEAT_MODE navázaný — mimo HEAT_MODE běží t_evap na 120 s, takže DEFROST_ORDERED je
tam už dnes degradovaný.

**Rozhodnutí:**

`DEFROST_ORDERED` se váže na `HEAT_MODE`. Do AND podmínky bs_defrost se přidá
`&& id(bs_heat_mode).state`. Ochrana chassis/drain se nařizuje jen v topné sezóně
(HEAT_MODE armed) — mimo ni je DEFROST_ORDERED vždy false.

Umístění gate uvnitř bs_defrost (ne na edge-triggeru run_defrost): policy na jednom
místě; pokles HEAT_MODE během cyklu ukončí ceiling fázi přes stávající
wait_until(!DEFROST_ORDERED). Floor (ADR-008) doběhne.

**Zdůvodnění:**

- Freeze protection běží jen když mráz reálně hrozí → konec plýtvání energií a opotřebení
  SSR/kabelů mimo topnou sezónu; méně matoucí Web UI/HA.
- Zarovnání s polling subsystémem (§3.1), který HEAT_MODE gate už předpokládá.
- Sémantika: "ORDERED" = nařízená reakce; HEAT_MODE = freeze-relevance. Žádné přetížení.

**Důsledky:**

- `bs_defrost` lambda: +`&& id(bs_heat_mode).state`.
- `ARCHITECTURE.md §4.3` — přidat 3. podmínku (HEAT_MODE armed).
- OI6 → Done.
- Známé omezení: HEAT_MODE prahy (2/4 °C) jsou proxy „chladno", ne měřený bod mrazu
  povrchu; hystereze může nechat úzké pásmo 3–4 °C, kde by marginální zamrznutí bylo
  fyzikálně možné, ale HEAT_MODE latched OFF. Přesnější per-surface gate (t_chassis/
  t_drain ≤ th) → BACKLOG, odloženo do zimního field pozorování (viz OI-nové).
- NaN/boot: HEAT_MODE je při NaN false → DEFROST_ORDERED false; konzistentní s
  ALWAYS_OFF fail-safe a ADR-006 boot chováním.

**Odkazy:** OI6, ADR-008, ADR-006, ARCHITECTURE §3.1/§4.3.

---

### ADR-010 — Topné kabely: samoregulační, 10 W/m odtok / 20 W/m chassis

**Status:** Accepted
**Datum:** 2026-07-30
**Session:** S8 (Architekt)
**Podnět:** Rozvahová session — výběr topného kabelu (typ + výkon + délky).

**Kontext:**

Původně zvažován konstantní odporový kabel. Rešerší zjištěno, že sériové konstantní
kabely (levné hotové sety) nelze řezat na míru a nesmí se smotávat/překrývat. Zásadní
je ale interakce s řízením: regulace je **časová**, čidla chassis a drain jsou pouze
informativní (drain navíc jen v jednom bodě) → topné okruhy nemají teplotní zpětnou
vazbu. Konstantní kabel by pod 9mm PE izolací na plastu neměl žádnou ochranu proti
přehřátí kromě časového stropu. Samoregulační kabel je řezatelný na místě a konstrukčně
self-limiting (nepřehřeje se ani při křížení/dotyku).

**Rozhodnutí:**

1. Použít **samoregulační** kabel (230 V, opletené stínění → připojit na PE) na oba okruhy.
   Volba typu: K&V thermo srKABEL (10 W/m) a srKABEL PRO (20 W/m) nebo ekvivalent.
2. Výkon a délka:
   - **Odtok: 10 W/m, 7 m** na trubce DN30 (rezerva pro přípoj do okapového svodu).
   - **Chassis: 20 W/m, ~2,5 m** (finální délka dle montáže — profilované dno
     s držáky a revizními otvory).
3. Instalovat **maximální délku, která fyzicky sedí** — výkonová rezerva pro prevenci
   i pro případný nouzový thaw.
4. Řízení zůstává **časové**; delší časové stropy (ADR-008 ceiling) jsou nově bezpečné,
   protože kabel je fail-safe.

**Zdůvodnění:**

- Řezatelnost na místě řeší kusé délky (7 m / ~2,5 m).
- Self-limiting je **jediná** ochrana proti přehřátí při časovém řízení bez teplotní
  zpětné vazby → bezpečnost pod izolací na plastu i při smotání/křížení.
- Odtok zaizolovaný, malá ztráta (~7 W/m při −15 °C) → 10 W/m pokrývá s rezervou;
  trubka je ve spádu, vyprazdňuje se během/krátce po defrostu → žádná stojatá voda
  k roztání, burst-topení stačí.
- Chassis holý plech, exponovaný větru v pergole (venkovní prostředí, ne garáž) →
  20 W/m = max dostupná autorita; navíc dostává teplo i ze samotného defrostu jednotky.
- Field data (1. sezóna): mrzla **jednotka a její napojení na odpad, ne vlastní
  potrubí** — potvrzuje těžiště na chassis/junction.

**Důsledky:**

- **Invariant (dependency):** časové řízení je bezpečné **pouze** se samoregulačním
  kabelem. Případný přechod na konstantní kabel by vyžadoval povinný max-on-time guard
  (OI15) + teplotní zpětnou vazbu. Neměnit typ kabelu bez revize řízení.
- OI15 (absolutní max-on-time guard, BACKLOG) klesá z „nutný" na „nice-to-have" — kabel
  je fail-safe; guard zůstává jen jako pojistka proti plýtvání energií, ne proti přehřátí.
- Elektrika: i při max délce (odtok 7 m @10 W/m ≈ 70 W ≈ 0,30 A; chassis 2,5 m @20 W/m
  ≈ 50 W ≈ 0,22 A) zůstává proud i se studeným náběhem (~1–1,5 A na 1–2 s) hluboko pod
  T2A (pomalá) i SSR G3MB-202 (2 A/kanál) → délka není HW-omezena.
- **Thaw mode (rozmrazování při selhání prevence) = obsluha NOUZOVÉHO stavu, ne
  provozní režim:** jednotka odstavena (ventilátor stojí → přirozená konvekce, příznivé
  podmínky), čas neomezen (rozmrazení řádově hodiny je akceptovatelné). Extra délka
  kabelu dodá výkon; delší/neomezený běh dodá čas. → BACKLOG, nice-to-have; detekci
  dokončení lze opřít o informativní čidla.
- **Klíčová pojistka prevence je EMPIRICKÁ:** scénář (prahy, časová okna, doběh/floor)
  se musí odladit praktickým pozorováním v 1. sezóně. Při dobrém odladění k zamrznutí
  nedojde; thaw mode je fallback, ne primární obrana.

**Odkazy:** rozvaha 2026-07-30 (S8), ADR-007 (ceiling), ADR-008 (floor / OI15
max-on-time), ADR-009 (HEAT_MODE gate).

---

### ADR-011 — Umístění teplotních čidel: výparník a chassis

**Status:** Accepted
**Datum:** 2026-07-31
**Session:** S7 (Architekt)

**Kontext:**

Projekt původně předpokládal umístění čidla výparníku (t_evap, DS18B20) na vnější stranu voštiny výměníku venkovní jednotky. Reverse engineering jednotky Toshiba Shorai Edge (RAS-B13G3KVSG-E) provedený v S7 odhalil, že tato pozice je nevhodná — voština nedává přístup na měděné potrubí a tepelná vazba by byla špatná. Studiem servisního manuálu a principiálního schématu (Principle_Schematic.pdf) bylo plně zmapováno zapojení chladicího okruhu, topologie čtyřcestného ventilu a logika odmrazovacího cyklu jednotky. Čidlo chassis (t_chassis) bylo původně nespecifikováno co do strany montáže.

**Defrost mechanismus venkovní jednotky Toshiba (referenční popis):**

Jednotka používá 4-way valve reverse defrost: v topném režimu pracuje venkovní výměník jako výparník. Při akumulaci námrazy řídicí elektronika detekuje potřebu odmrazení přes senzor TE (Heat Exchange Temp. Sensor, CN61) a zahájí cyklus.

Detekce potřeby odmrazení (tabulka zón, servisní manuál str. 40):
Baseline TE0 a TO0 se ukládají jako minimum prvních 10–15 min topného provozu. Odmrazení se iniciuje, když teplotní pokles výměníku TE0 − TE ≥ 2–3 °C a zároveň superheat SH − SH0 ≤ 2 (zóna A/B), nebo TE ≤ −25 °C (zóna C), nebo po ≥ 90 min provozu s TE ≤ −2 °C (zóna D, časový fallback).

Průběh cyklu:

1. Kompresor se zastaví na ~40 s.
2. Čtyřcestný ventil překlopí (40 s po zastavení kompresoru) — přepne výtlačný plyn z vnitřní jednotky na venkovní výměník.
3. Ventilátor venkovní jednotky se zastaví (chlazení pouze konvekcí).
4. Horký výtlačný plyn vstupuje plynovým vývodem výměníku (trubka b, vývod C ventilu, Ø 9,5 mm) a rozpouští námrazu na lamelách.
5. Roztátá námraza odtéká jako kondenzát přes spodní vývod výměníku (trubka a, přes PMV, Ø 6 mm), kde sedí senzor TE.

Ukončení cyklu — splněna první z podmínek:

- Teplota výměníku na TE stoupne na ≥ +8 °C po dobu 3 s, nebo
- Teplota na TE drží ≥ +7 °C po dobu 60 s, nebo
- Cyklus trvá max. 10 min (tvrdý strop).

Návrat do topení:

1. Kompresor stojí ~40 s.
2. Čtyřcestný ventil překlopí zpět (~30 s po zastavení).
3. Venkovní ventilátor spustí s kompresorem.

Typický rozestup mezi defrosty: 31–51 min (dle modelu, Table 2 manuálu; model 13k = 31 min minimum).

Klíčový poznatek: senzor TE je záměrně umístěn na výstupní/kapalinové straně výměníku (trubka a) — funguje jako ukončovací indikátor (zahřeje se teprve po roztátí námrazy po celé délce výměníku). Naopak plynový vývod (trubka b, vývod C čtyřcestného ventilu) se zahřeje jako první bezprostředně po překlopení ventilu.

**Rozhodnutí:**

1. Čidlo výparníku — rename t_evap → t_gas_inlet:

Čidlo DS18B20 se umístí kontaktně na trubku b — plynový vývod výměníku, Ø 9,5 mm, bez izolace, spojující výměník s vývodem C čtyřcestného ventilu. Montáž co nejblíže k tělesu výměníku. Uchycení hliníkovou páskou pro tepelný kontakt; doporučení — čidlo izolovat od okolního prostředí vhodnou izolací.

Tato poloha poskytuje nejčasnější možnou detekci defrostu. Je fyzicky přístupná po demontáži čelního krytu vnější jednotky.

Název t_evap implikuje měření na výstupní/výparníkové straně analogicky k TE. Naše čidlo záměrně sedí na plynovém vstupu — opačný konec. Rename se provede okamžitě (fáze tvorby software, žádná NVS ani entity_id zpětná závislost). Prahy def_abs_th/def_dt_th zůstávají beze změny.

2. Čidlo chassis (t_chassis):

DS18B20 se umístí kontaktně na vnější (spodní) stranu pánve chassis, v mezeře meandru topného kabelu (ne na samotný kabel — kabel je lokální hot spot, mezera čte nejstudenější kov = konzervativní ochrana). Přesná poloha se určí při montáži dle kritického bodu (nejnižší bod / odtokový roh). Uchycení hliníkovou páskou; doporučení — vhodná izolace od okolního vzduchu.

Vnější strana eliminuje kontakt s vodou/ledem (koroze, zkreslení latentním teplem při 0 °C). Tepelný spád přes 1 mm pozinkovaného plechu je zanedbatelný (~0,003 K), informační hodnota obou stran je identická.

**Zdůvodnění:**

Plynový vývod (trubka b) je fyzikálně ověřený nejčasnější bod detekce reverzu; konzistentní s průběhem cyklu z manuálu a topologií venkovní jednotky ze schématu. TE-analogické umístění (trubka a) bylo explicitně zavrženo — dává pozdní signál (ukončovací logika Toshiby). Vnější strana chassis eliminuje vodní médium jako zdroj chyb bez ztráty informace.

**Důsledky:**

- ARCHITECTURE.md §3: aktualizovat název t_evap → t_gas_inlet, popsat fyzické umístění.
- firmware/yaml/: rename všech výskytů t_evap / id: t_evap → t_gas_inlet; name: dle ADR-005 konvence.
- ARCHITECTURE.md §4.3: aktualizovat název senzoru v popisu DEFROST_ORDERED.
- BACKLOG.md: přidat B-VALID-01, B-VALID-02, B-VALID-03 (viz níže).

**Odkazy:** Principle_Schematic.pdf, Defrost_Operation.pdf (servisní manuál str. 40), ADR-008, ADR-009, ADR-005.

---

*Poslední ADR: ADR-011. Příští číslo: ADR-012.*
