# WORKING AGREEMENT — Garage_Drain_Defrost

> **Účel:** Pravidla spolupráce mezi Luborem, Architektem a Claude Code (CC).
> **Vlastník:** Lubor (project owner). Změny diskutovány s Architektem.
> **Předchůdce:** Adaptováno z Garage_Windows WORKING_AGREEMENT v1.2 (štíhlý seed, ADR-001).
> **v1.1 (2026-07-10):** Role Implementer a Claude Code sloučeny do jedné role —
> samostatná Sonnet-Implementer session se ukázala jako zbytečný mezikrok. Viz ADR-002.

---

## 0. Tři úrovně rozhodování

Projekt má tři odlišné rozhodovací úrovně. Každá patří jiné roli a žije v jiném tempu.

| Úroveň | Co řeší | Vlastník | Tempo |
|---|---|---|---|
| **Strategická** | Fáze projektu, milestones, priority, vize, constraints | Project Owner (Lubor) | týdny–měsíce |
| **Architektonická** | Technické řešení v rámci fáze (ADR) | Architekt | dny |
| **Implementační** | Denní práce, kód, testy, debugy | Implementer (Claude Code) | hodiny |

**Pyramida vlastnictví:**

```
Project Owner (Lubor)
    │  vlastní: PROJECT_VISION.md, ROADMAP.md
    │  rozhoduje: fáze, milestones, priority na strategické úrovni
    ▼
Architekt (Opus)
    │  vlastní: ARCHITECTURE.md, DECISIONS.md, BACKLOG.md (řazení v rámci fáze)
    │  rozhoduje: technická řešení v aktuální fázi
    ▼
Implementer = Claude Code (CC)
        vlastní: TEST_PLAN.md, BUGS.md, HANDOVER_YYYYMMDD.md, SESSION_LOG.md,
                 README.md (s instrukcí), kód, commit messages
        rozhoduje: jak konkrétně realizovat ADR, denní implementace, root cause
```

**Odlehčený režim (viz PROJECT_VISION.md):** hranice mezi úrovněmi platí, ale práh pro
formální ADR je vyšší než u Garage_Windows. Drobná implementační rozhodnutí (např. volba
konkrétní hodnoty timeoutu, drobný refactor) Implementer (CC) řeší sám a jen zaznamená do
SESSION_LOG — neeskaluje automaticky každou malou volbu.

---

## 1. Role

### 1.0 Project Owner — Lubor

**Odpovědnosti:**
- Vize a smysl projektu
- Členění projektu do fází, pořadí fází, kdy fáze končí
- Priority na strategické úrovni — must-have / nice-to-have / non-goal
- Constraints — čas, hardware, finance, kompetence
- Vlastní soubory: `PROJECT_VISION.md`, `ROADMAP.md`
- Schvaluje ADR od Architekta
- Provádí bench testy, posílá výsledky Implementer roli
- Mezi AI rolemi a Claude Code je prostředníkem (kopíruje prompty, vrací výstupy)

**Komunikační styl:** česky, krok za krokem, schvaluje malé inkrementy.

### 1.1 Architekt — Claude web, Opus (aktuálně Opus 4.8)

**Účel:** Drží projekt v hlavě v rámci aktuální fáze, rozhoduje technický směr, neřeší
detaily implementace.

**Odpovědnosti:**
- Architektonická rozhodnutí (ADR do `DECISIONS.md`)
- Aktualizace `ARCHITECTURE.md`
- Správa `BACKLOG.md` — řazení a priority v rámci aktuální fáze
- Handoff prompty pro Implementer session
- Schvaluje výstupy z Implementer session při eskalaci

**Co nedělá:**
- Nemění strategická rozhodnutí — jen je realizuje, případně navrhuje úpravy
- Nepíše prompty pro Claude Code
- Nereviewuje C++ kód řádek po řádku
- Nepíše TEST_PLAN

**Frekvence použití:** Krátké session, jen při potřebě rozhodnutí.

### 1.2 Implementer — Claude Code (CC)

**Účel:** Realizuje zadání architekta, řeší denní implementační problémy — přímo v repu.
Dřívější dělba na "Implementer (Sonnet chat)" a "Claude Code (mechanická realizace)"
byla zrušena (ADR-002): samostatná Sonnet-Implementer session byla zbytečný mezikrok,
CC zvládá roli Implementera přímo.

**Odpovědnosti:**
- Realizace handoff promptu od Architekta (Lubor předá ručně, copy-paste)
- Editace kódu, root cause analýza, debug
- `TEST_PLAN.md`, `BUGS.md`, `HANDOVER_YYYYMMDD.md`, `SESSION_LOG.md`
- Aktualizace `README.md` po větších změnách
- Commit + push
- Eskalace na Architekta při architektonické otázce

**Co nedělá:**
- Nemění strategická ani architektonická rozhodnutí — jen je realizuje
- Nepíše do `ARCHITECTURE.md`, `DECISIONS.md`, `PROJECT_VISION.md`, `ROADMAP.md`
- Nevytváří nová ADR sám — eskaluje na Architekta
- Nevybírá variantní architektonická řešení sám

**Frekvence použití:** Hlavní pracovní nástroj, většina implementační práce.



---

## 2. Soubory dokumentace — kdo co píše

| Soubor | Vlastník | Frekvence změn | Úroveň |
|---|---|---|---|
| `PROJECT_VISION.md` | Lubor | zřídka | Strategická |
| `ROADMAP.md` | Lubor (Architekt navrhuje) | občas | Strategická |
| `WORKING_AGREEMENT.md` | Lubor | zřídka | Meta |
| `README.md` | CC (po milnících) | zřídka | Public |
| `ARCHITECTURE.md` | Architekt | občas | Architektonická |
| `DECISIONS.md` (ADR) | Architekt — append-only | po větší debatě | Architektonická |
| `BACKLOG.md` | Architekt primárně, Implementer (CC) doplňuje | průběžně | Architektonická |
| `BUGS.md` (BUG + AP) | Implementer (CC) — append-only | po debugu | Implementační |
| `TEST_PLAN.md` | Implementer (CC) | průběžně | Implementační |
| `HANDOVER_YYYYMMDD.md` | Implementer (CC) | každá session | Implementační |
| `SESSION_LOG.md` | Implementer (CC) nebo Architekt, kdo session končí | každá session, append-only | Implementační |

**Klíčové principy:**
- `HANDOVER_YYYYMMDD.md` (YYYY=rok, MM=měsíc, DD=den) se po ukončení session archivuje
  a nahradí novým.
- `DECISIONS.md`, `BUGS.md`, `SESSION_LOG.md` jsou append-only.
- ADR a BUG-NNN/AP-NNN jsou číslované, nikdy se nepřečíslovávají. Zrušená ADR dostanou
  status *Superseded by ADR-NNN*.
- `PROJECT_VISION.md` a `ROADMAP.md` jsou přepisovatelné — historie je v gitu.

---

## 3. Workflow

### 3.1 Strategický cyklus (mimo session)

1. Definice fáze (`PROJECT_VISION.md`)
2. Členění do milestones (`ROADMAP.md`)
3. Komunikace fáze do Architekt session

### 3.2 Denní cyklus (uvnitř fáze)

1. **Architekt session** (Opus, jen pokud potřeba) — rozhodnutí, ADR, handoff prompt pro CC
2. **Lubor** ručně předá handoff prompt do CC (Claude Code)
3. **CC (Implementer)** — implementace, root cause, debug, aktualizace TEST_PLAN/BUGS
4. **Lubor** — flash, bench testy, výsledky zpět CC (nebo Architektovi, pokud jde o
   architektonickou otázku)
5. **Konec session** — HANDOVER_YYYYMMDD.md (nový soubor s dnešním datem; předchozí
   zůstává beze změny jména a přesune se do `#Archive/`) + SESSION_LOG.md (přidat řádek) +
   příslušné append-only soubory. **CC: commit + push. Lubor ověří sync.**

### 3.3 Eskalace mezi rolemi

**Implementer (CC) → Architekt:** CC narazí na architektonickou otázku → řekne
*„toto vyžaduje Architekta"* → Lubor otevře Architekt okno → Architekt zapíše ADR-NNN →
CC pokračuje podle ADR.

**Architekt → Project Owner:** Architekt zjistí přesah scope fáze → řekne *„toto je
strategická otázka"* → Lubor rozhodne, případně aktualizuje ROADMAP/VISION.

### 3.4 Handoff prompt (Architekt → Implementer/CC)

Jeden formát, jedno předání — bez mezikroku přes samostatnou Implementer session.
Architekt píše rovnou v podobě, kterou Lubor zkopíruje do CC:

```markdown
# Implementer Session — [téma]

**Cíl:** [1 věta]
**Kontext:** [2–3 věty, odkaz na ADR-NNN pokud relevantní]
**Soubory k načtení:** [max 3]
**První krok:** [co udělat hned]
**Constraints:**
- Modify ONLY [files]
- Do NOT modify [files]
**Kdy eskalovat zpět na Architekta:** [konkrétní triggery]
**Validation:** [compile / test / chování — co ověřit před commitem]
**Výstup session:** [co má být hotové, vč. TEST_PLAN/BUGS/HANDOVER/SESSION_LOG update]
```

---

## 4. Komunikační pravidla

### 4.1 Jazyk

- Lubor ↔ Architekt: **česky**
- Lubor ↔ Implementer (CC): **česky** (kontext) nebo **anglicky** (konkrétní kódové
  zadání/constraints) — CC zvládá obojí, volí se podle povahy zadání
- ADR / BUG / dokumentace v repu: **česky** popisné texty, **anglicky** kód

### 4.2 Styl konverzace

- Pomalu, krok za krokem — žádné dlouhé monology na první výzvu
- Jedna otázka per zpráva, max tři, pokud jsou nezávislé
- Krátké odpovědi; u rozsáhlých analýz postupovat po částech

### 4.3 Schvalování

- Lubor schvaluje malé inkrementy
- Architekt navrhuje ADR jako draft → Lubor schvaluje → commit
- Architekt navrhuje handoff prompt → Lubor schvaluje → odešle do CC
- Strategické změny (ROADMAP, VISION) → Lubor rozhoduje sám

---

## 5. Kontextová ekonomie

### 5.1 Hlášení kontextu

- **0–40 %:** v pohodě, pokračujte
- **40–60 %:** hotovo, poslední logický celek
- **60–80 %:** napsat handover a zavřít
- **80 %+:** stop, jen handover

### 5.2 Lehký start session

- Architekt: nejnovější HANDOVER_YYYYMMDD.md + relevantní sekce BACKLOG.md + PROJECT_VISION pro orientaci
- Implementer (CC): nejnovější HANDOVER_YYYYMMDD.md + handoff prompt od Architekta + 1 specifický kódový soubor
- Nečíst všechny MD soubory automaticky ani staré archivované handovery

### 5.3 Krátká okna

- Víc kratších session místo 1–2 dlouhých
- Po dokončení logického celku zavřít okno

### 5.4 Volba modelu

- **Opus** — jen pro Architekta, jen pro rozhodnutí
- **Claude Code (CC)** — pro Implementer, veškerá implementační práce (kód, testy,
  dokumentace, commit/push)

### 5.5 Past chats / Memory

- **NEZAPÍNAT.** Jeden zdroj pravdy je v repu.

---

## 6. Anti-patterny v komunikaci s AI

Převzato z Garage_Windows (obecně platné, projektově neutrální):

### AP-COM-001 — Velké zadání naráz
**Pravidlo:** Žádej jeden krok, schval, žádej další. Iteruj.

### AP-COM-002 — Smíšené role v jednom okně
**Pravidlo:** Jedna role per okno/nástroj. Opus jen na Architekta, CC na Implementer.

### AP-COM-003 — Handover jako jediná paměť
**Pravidlo:** Handover je jen aktuální stav (1 strana). Decisions, bugs, backlog mají
vlastní append-only soubory.

### AP-COM-004 — Zpětný refaktoring historické dokumentace
**Pravidlo:** Refaktoruje se jen poslední 1–2 handovery. Starší se ponechají jako archiv.

### AP-COM-005 — AI rozhoduje o úrovni nad svojí
**Pravidlo:** Každá role rozhoduje jen na své úrovni. Když narazí na vyšší úroveň, eskaluje.

---

## 7. Verzování dokumentu

| Verze | Datum | Změna |
|---|---|---|
| 1.0 | 2026-07-10 | První verze. Adaptováno z Garage_Windows WORKING_AGREEMENT v1.2 pro restart Garage_Drain_Defrost (ADR-001, štíhlý seed, odlehčený režim kvality). |
| 1.1 | 2026-07-10 | Sloučení rolí Implementer a Claude Code do jedné (ADR-002) — samostatná Sonnet-Implementer session odpadá jako zbytečný mezikrok. Aktualizovány §0, §1, §2, §3, §4, §5, §6. |
| 1.2 | 2026-07-10 | Oprava: §3.2 krok 5 sjednocen s §2 — HANDOVER soubor používá jednotně datovaný název (`HANDOVER_YYYYMMDD.md`), ne "přepsat HANDOVER.md". Nalezeno při konzistenční revizi doc seedu (S1). |
