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
│
├── BACKLOG.md                     open items (Aktivní / Odložené / Hardening / Done)
├── ROADMAP.md                     phase model, milestones, scheduling rationale
├── DECISIONS.md                   ADR log — ADR-001…, append-only
│
├── ARCHITECTURE.md                current system architecture snapshot
├── BUGS.md                        BUG-NNN root cause analyses + AP-NNN anti-patterns
│
├── WORKING_AGREEMENT.md           workflow rules, roles, session protocol
├── PROJECT_VISION.md              high-level project goals, constraints, quality stance
│
├── (HANDOVER_YYYYMMDD.md — not yet created; first Implementer/CC session will add it.
│    Active file already carries its creation date; superseded ones move as-is into
│    #Archive/, no rename needed.)
├── (TEST_PLAN.md — not yet created; added once code review / bench validation
│    produces concrete test cases. Owner: Implementer/CC, see WORKING_AGREEMENT §2.)
│
├── ── SUBFOLDERS ────────────────────────────────────────────────────────────
│
├── Prompts/
│   ├── PROMPT_architekt.md        start-of-session prompt for Architekt role
│   └── PROMPT_implementer.md      start-of-session prompt for Implementer role
│
├── Test Results/                  created (empty, 2026-07-10) — populated as-needed
│                                  once bench validation starts, per odlehčený/
│                                  lightweight approach — see PROJECT_VISION.md
│
└── #Archive/                      created (empty, 2026-07-10) — will hold archived
                                   HANDOVER_YYYYMMDD.md files
```

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
│  TECHNICAL REFERENCE                                                    │
│                                                                         │
│  ARCHITECTURE.md ──► component map, data flow, current implementation   │
│  BUGS.md         ──► BUG-NNN root cause analyses, AP-NNN anti-patterns  │
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
  HANDOVER archived  ──►  #Archive/HANDOVER_YYYYMMDD.md
```

---

## Append-only documents (never overwrite content)

| Document | Append unit |
|---|---|
| `SESSION_LOG.md` | One `## SN` block per session |
| `DECISIONS.md` | One `### ADR-NNN` block per decision |
| `BUGS.md` | One `### BUG-NNN` block per bug; one `### AP-NNN` per anti-pattern |
| `BACKLOG.md` | Rows added; done rows moved to Done section, not deleted |

---

## Difference from Garage_Windows structure (context for future readers)

This is a **štíhlý seed** (ADR-001), not a full clone. Notable omissions, added
as-needed rather than upfront:

- `Test Results/` — created empty (2026-07-10); full acceptance test plan content will be
  added once bench validation phase starts
- `#Archive/` — created empty (2026-07-10); will hold archived HANDOVER_YYYYMMDD.md files
  once the first one is superseded
- No CT-clamp/hypothesis-record style analysis docs — not applicable to this project's
  simpler (threshold + timer) logic
- **Role model (ADR-002):** Implementer and Claude Code are merged into one role.
  There is no separate Sonnet-Implementer chat window — Lubor copies the Architekt's
  handoff prompt directly into Claude Code, which does the implementation, testing,
  and documentation itself.

---

*Last updated: 2026-07-10 (S1, konzistenční revize) — Test Results/ a #Archive/ založeny
Luborem manuálně (prázdné), HANDOVER/TEST_PLAN doplněny do mapy dokumentů.*
