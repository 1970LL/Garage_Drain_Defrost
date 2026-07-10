# ROADMAP — Garage_Drain_Defrost

> **Účel:** Pořadí fází, milestones, rationale plánování.
> **Vlastník:** Lubor (Architekt navrhuje úpravy).
> **Pravidlo:** PŘEPISUJE se. Historie je v gitu.

---

## Fáze

| Fáze | Cíl | Stav | Hlavní výstup |
|---|---|---|---|
| **0 — Concept & Breadboard** | Zadání, HW scope, funkční prototyp s ChatGPT-asistovaným kódem | ✅ Done | Breadboard prototyp, funkční defrost logika (nevalidovaná formálně) |
| **1 — Doc-sync & Code Review** | Nastartovat standardizovanou dokumentaci (tento seed), provést review kódu, vytěžit BACKLOG | 🔄 Active | Doc seed committnutý, code review nález → BACKLOG naplněn |
| **HW Finalizace** | Dokončení a osazení custom PCB | ⏳ Planned | Osazená finální deska |
| **2 — Firmware Cleanup & Hardening** | Odstranění testovacích bloků (I2C/VL53L0X/BME280), doladění nálezů z code review, reálné adresy DS18B20 | ⏳ Planned | Firmware bez technického dluhu z §7 ARCHITECTURE.md |
| **3 — Bench Validation** | Ověření defrost/heat logiky na finálním HW se simulačním režimem i reálnými senzory | ⏳ Planned | Funkční potvrzení na bench, lehký test checklist |
| **4 — Field Deployment** | Nasazení v garáži, ověření přes zimní sezónu | ⏳ Planned | Provozní instalace |

**Pozn.:** Fáze jsou orientační a budou upřesněny/přeřazeny podle výsledků code review
(fáze 1). Vzhledem k odlehčenému přístupu ke kvalitě (viz PROJECT_VISION.md) očekáváme
kratší a méně formalizované fáze než u Garage_Windows — bez samostatné "Breadboard →
HW → Dual" struktury, protože zde není multi-instance komplexita.

---

## Aktuální fáze: 1 — Doc-sync & Code Review

**Cíl fáze:** Založit doc seed (WORKING_AGREEMENT, PROJECT_VISION, ARCHITECTURE, ROADMAP,
BACKLOG, DECISIONS, BUGS, SESSION_LOG, Prompts), commitnout, a v navazující session
provést review existujícího YAML/kódu. Review výstup se promítne do BACKLOG.md jako
konkrétní dílčí úkoly.

**Definition of Done pro Fázi 1:**
- [x] PROJECT_VISION.md
- [x] ARCHITECTURE.md
- [x] ROADMAP.md
- [x] BACKLOG.md (skeleton — naplní se v code review)
- [x] DECISIONS.md (seed — ADR-001, ADR-002)
- [x] BUGS.md (skeleton)
- [x] WORKING_AGREEMENT.md
- [x] Prompts/PROMPT_architekt.md, PROMPT_implementer.md
- [x] SESSION_LOG.md (S1)
- [x] Software_DevDoc_structure.md
- [x] Commit + push do repa
- [ ] Code review provedeno (nová session) → BACKLOG naplněn

---

## Verzování dokumentu

| Verze | Datum | Změna |
|---|---|---|
| 1.0 | 2026-07-10 | První verze. Fázový model pro restart projektu pod standardizovaným workflow. |
| 1.1 | 2026-07-10 | Definition of Done pro Fázi 1 odškrtnuta dle skutečného stavu (doc seed hotový, jen commit/push a code review zbývaly). Nalezeno při konzistenční revizi doc seedu (S1). |
