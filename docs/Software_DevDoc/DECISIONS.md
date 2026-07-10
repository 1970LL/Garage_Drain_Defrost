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

*Poslední ADR: ADR-002. Příští číslo: ADR-003.*
