# BACKLOG — Garage_Drain_Defrost

> **Účel:** Co je otevřené, odložené, nebo hotové — v rámci aktuální fáze.
> **Vlastník:** Architekt primárně, Implementer doplňuje.
> **Pravidlo:** Řádky se přidávají; hotové se přesouvají do Done, nemažou se.

---

## Aktivní

Naplní se výstupem code review (plánováno jako navazující session).

Předběžné položky viditelné už teď z ARCHITECTURE.md §7 (technický dluh z YAML review):

| ID | Popis | Zdroj | Priorita |
|---|---|---|---|
| OI1 | Nahradit placeholder adresy DS18B20 (`0x1`–`0x4`) reálnými adresami při komisioningu | ARCHITECTURE §7.1 | Nutné před nasazením |
| OI2 | Odstranit testovací I2C blok (VL53L0X, BME280, I2C bus) | ARCHITECTURE §7.2 | Fáze 2 cleanup |
| OI3 | Ověřit/rozhodnout, zda paralelní běh chassis/drain heateru bez společné synchronizace konce je záměr | ARCHITECTURE §7.3 | K prověření v code review |
| OI4 | Zhodnotit, zda 60s interval T3/T4 error checku není zbytečně pomalý vzhledem k přímému řízení výstupů | ARCHITECTURE §5 | K prověření v code review |

---

## Odložené

*(zatím prázdné)*

---

## Hardening

*(zatím prázdné — naplní se po Field Deployment fázi)*

---

## Done

| ID | Popis | Session |
|---|---|---|
| — | Doc seed založen (VISION, ARCHITECTURE, ROADMAP) | S1 |

---

*Last updated: 2026-07-10 (S1)*
