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

*Poslední ADR: ADR-005. Příští číslo: ADR-006.*
