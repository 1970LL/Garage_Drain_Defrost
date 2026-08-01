# ROADMAP — Garage_Drain_Defrost

> **Účel:** Pořadí fází, milestones, rationale plánování.
> **Vlastník:** Lubor (Architekt navrhuje úpravy).
> **Pravidlo:** PŘEPISUJE se. Historie je v gitu.

---

## Fáze

| Fáze | Cíl | Stav | Hlavní výstup |
|---|---|---|---|
| **0 — Concept & Breadboard** | Zadání, HW scope, funkční prototyp s ChatGPT-asistovaným kódem | ✅ Done | Breadboard prototyp, funkční defrost logika (nevalidovaná formálně) |
| **1 — Doc-sync & Code Review** | Nastartovat standardizovanou dokumentaci (tento seed), provést review kódu, vytěžit BACKLOG | ✅ Done | Doc seed committnutý (S1), code review provedeno (`Code_review_20260710.md`, archivováno), BACKLOG naplněn |
| **HW Finalizace** | Dokončení a osazení custom PCB | ✅ Done | Osazená finální deska (Hardware revision A), bench test S5+ probíhal na hotovém HW |
| **2 — Firmware Cleanup & Hardening** | Odstranění testovacích bloků (I2C/VL53L0X/BME280), doladění nálezů z code review, reálné adresy DS18B20 | ✅ Done | OI2 (testovací blok odstraněn), OI1 (T1-T4 reálné adresy komisionovány, S5+S11), technický dluh z §7 ARCHITECTURE.md vyřešen |
| **3 — Bench Validation** | Ověření defrost/heat logiky na finálním HW se simulačním režimem i reálnými senzory | ✅ Done | `TEST_PLAN.md` Fáze 0–10 kompletní, BUG-001 až BUG-007 nalezeny a opraveny, ADR-004/005/006/007/008/009 bench potvrzeny, HA napárováno (S15, 2026-08-01) |
| **4 — Field Deployment** | Fyzická montáž (topné kabely ADR-010, finální umístění čidel T2/T3 ADR-011), nasazení v garáži, ověření přes zimní sezónu | 🔄 Active | Provozní instalace, zimní pozorování (B-VALID-01/02/03, OI16/OI19) |

**Pozn.:** Vzhledem k odlehčenému přístupu ke kvalitě (viz PROJECT_VISION.md) proběhly
fáze 1-3 rychleji a méně formalizovaně než u Garage_Windows — bez samostatné
"Breadboard → HW → Dual" struktury, protože zde není multi-instance komplexita.
Softwarová práce pro fáze 1-3 je kompletní; zbylé aktivní BACKLOG položky
(OI7, OI15/16/19/20, B-VALID-01/02/03) jsou všechny buď vázané na Field
Deployment samotný, nebo na zimní pozorování — žádná není akcionovatelná dřív.

---

## Aktuální fáze: 4 — Field Deployment

**Cíl fáze:** Fyzická montáž topných kabelů (ADR-010) a finální umístění čidel
T2 (plynový vývod výměníku)/T3 (mezera meandru chassis kabelu) dle ADR-011,
uvedení do provozu, a validace defrost detekce/timingů přes první reálnou
zimní sezónu.

**Definition of Done pro Fázi 4:**
- [ ] Topné kabely namontovány (ADR-010: odtok 10 W/m/7m, chassis 20 W/m/~2,5m)
- [ ] T2 čidlo přemístěno na plynový vývod výměníku (ADR-011)
- [ ] T3 čidlo umístěno v mezeře meandru topného kabelu chassis (ADR-011)
- [ ] OI7 (web_server auth/odstranění) vyřešeno před ostrým provozem
- [ ] B-VALID-01/02/03 vyhodnoceny po první zimní sezóně
- [ ] OI16/OI19 (per-surface freeze gate, thaw mode) rozhodnuty dle zimních dat

---

## Verzování dokumentu

| Verze | Datum | Změna |
|---|---|---|
| 1.0 | 2026-07-10 | První verze. Fázový model pro restart projektu pod standardizovaným workflow. |
| 1.1 | 2026-07-10 | Definition of Done pro Fázi 1 odškrtnuta dle skutečného stavu (doc seed hotový, jen commit/push a code review zbývaly). Nalezeno při konzistenční revizi doc seedu (S1). |
| 1.2 | 2026-08-01 | Fáze 1-3 označeny Done (code review, HW finalizace, firmware hardening, bench validace S5-S15 kompletní vč. HA párování). Aktuální fáze přepnuta na 4 — Field Deployment, nová Definition of Done. Dokumentační úklid po uzavření bench-test rozsahu (S15, Implementer). |
