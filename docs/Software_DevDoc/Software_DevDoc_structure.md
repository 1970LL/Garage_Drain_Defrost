# Software_DevDoc — Structure & Document Map

> Quick orientation for every new session. Read this first.

---

## Directory Tree

```
docs/Software_DevDoc/
│
├── Software_DevDoc_structure.md   ← this file
│
├── ── ACTIVE DOCUMENTS ──────────────────────────────────────────────────────
│
├── SESSION_LOG.md                 chronological session history (newest-first, S1…)
├── HANDOVER_20260801.md           dated snapshot, current — superseded copies move
│                                  as-is (same filename) into #Archive/ once replaced
│
├── BACKLOG.md                     open items (Aktivní / Odložené / Hardening / Done)
├── ROADMAP.md                     phase model, milestones, scheduling rationale
├── DECISIONS.md                   ADR log — ADR-001…, append-only
│
├── ARCHITECTURE.md                current system architecture snapshot
├── BUGS.md                        BUG-NNN root cause analyses + AP-NNN anti-patterns
├── TEST_PLAN.md                   bench test checklist (Fáze 0…), owner: Implementer/CC
│
├── WORKING_AGREEMENT.md           workflow rules, roles, session protocol
├── PROJECT_VISION.md              high-level project goals, constraints, quality stance
│
├── ── SUBFOLDERS ────────────────────────────────────────────────────────────
│
├── Prompts/
│   ├── PROMPT_architekt.md        start-of-session prompt for Architekt role
│   └── PROMPT_implementer.md      start-of-session prompt for Implementer role
│
├── Test Results/                  still empty — lightweight approach (PROJECT_VISION.md);
│                                  TEST_PLAN.md itself has carried all bench-test content
│                                  so far, no separate result artifacts needed yet
│
└── #Archive/                      superseded snapshots, same filename as when active:
    ├── Code_review_20260710.md
    ├── HANDOVER_20260710.md
    ├── HANDOVER_20260722.md
    ├── HANDOVER_20260728.md
    ├── HANDOVER_20260730.md
    └── HANDOVER_20260731.md
```

**Sibling reference material (outside Software_DevDoc/, under `docs/`):** not part
of this doc set's workflow, just source material cited from `DECISIONS.md`/
`ARCHITECTURE.md` — `docs/Toshiba/Outdoor Unit/` (Toshiba Shorai Edge service
manual excerpts, ADR-011 sensor-placement rationale), `docs/Thermal Cable/`
(samoregulační kabel datasheets, ADR-010), `docs/GPIO_Desc_Functionality.xlsx`.

---

## Active Document Roles

```
┌─────────────────────────────────────────────────────────────────────────┐
│  PER-SESSION WORKFLOW                                                   │
│                                                                         │
│  HANDOVER_YYYYMMDD.md ──► what happened last, what to do next           │
│  SESSION_LOG.md        ──► short block per session, append-only        │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│  PLANNING & TRACKING                                                    │
│                                                                         │
│  BACKLOG.md   ──► what is open (Aktivní), deferred, done                │
│  ROADMAP.md   ──► phase sequence, milestones                            │
│  DECISIONS.md ──► ADR-NNN records, append-only                          │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│  TECHNICAL REFERENCE & VALIDATION                                       │
│                                                                         │
│  ARCHITECTURE.md ──► component map, data flow, current implementation   │
│  BUGS.md         ──► BUG-NNN root cause analyses, AP-NNN anti-patterns  │
│  TEST_PLAN.md    ──► bench test checklist, Lubor checks boxes off       │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│  PROCESS & GOVERNANCE                                                   │
│                                                                         │
│  WORKING_AGREEMENT.md ──► workflow rules, roles, session protocol       │
│  PROJECT_VISION.md    ──► goals, non-goals, quality stance              │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Document Lifecycle

```
Created this session
        │
        ▼
  ACTIVE (root)  ──────────────────────────────────► never deleted
        │                                             (SESSION_LOG,
        │  next session creates new HANDOVER          DECISIONS, BUGS)
        │
        ▼
  HANDOVER archived  ──►  #Archive/HANDOVER_YYYYMMDD.md (same filename, unchanged)
```

---

## Append-only documents (never overwrite content)

| Document | Append unit |
|---|---|
| `SESSION_LOG.md` | One `## SN` block per session |
| `DECISIONS.md` | One `### ADR-NNN` block per decision |
| `BUGS.md` | One `### BUG-NNN` block per bug; one `### AP-NNN` per anti-pattern |
| `BACKLOG.md` | Rows added; done rows moved to Done section, not deleted |

`ARCHITECTURE.md` is the one exception to "append-only": it's a **snapshot**,
rewritten in place per section as the system changes, with a version table
(§ "Verzování dokumentu") tracking each revision instead of accumulating history
inline. `TEST_PLAN.md` is a **living checklist** — new Fáze N sections get
appended as new work needs verification, but checkboxes within existing phases
are updated in place (unchecked → ✅) as Lubor completes bench tests, not
re-appended as new rows.

---

## Difference from Garage_Windows structure (context for future readers)

This is a **štíhlý seed** (ADR-001), not a full clone. Notable omissions, added
as-needed rather than upfront:

- `Test Results/` — created empty (2026-07-10); still empty as of S13
  (2026-08-01) — `TEST_PLAN.md` has absorbed all bench-test content directly
  (findings, 🐛 inline notes, per-phase status), no separate artifact folder
  has been needed so far
- No CT-clamp/hypothesis-record style analysis docs — not applicable to this
  project's simpler (threshold + timer) logic
- **Role model (ADR-002):** Implementer and Claude Code are merged into one role.
  There is no separate Sonnet-Implementer chat window — Lubor copies the Architekt's
  handoff prompt directly into Claude Code, which does the implementation, testing,
  and documentation itself.

---

*Last updated: 2026-08-01 (S13, Implementer) — TEST_PLAN.md and HANDOVER now
active/in regular use (both were still-empty placeholders as of the 2026-07-10
version of this map); #Archive/ populated (5 HANDOVER snapshots + one archived
code review); sibling reference-material folders (Toshiba, Thermal Cable) noted.*
