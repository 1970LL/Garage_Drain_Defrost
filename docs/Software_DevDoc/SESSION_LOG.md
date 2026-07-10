# SESSION LOG — Garage_Drain_Defrost

> Chronologický log session (nejnovější nahoře). Append-only.

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
