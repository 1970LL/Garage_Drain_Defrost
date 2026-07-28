# BUGS — Garage_Drain_Defrost

> **Účel:** Root cause analýzy bugů (BUG-NNN) a obecné anti-patterny (AP-NNN).
> **Vlastník:** Implementer.
> **Pravidlo:** Append-only, nikdy se nepřepisuje. Čísla se nerecyklují.

---

## Bugs

### BUG-001 — Main System Enable / Simulation Mode se resetují na OFF po každém rebootu

**Nalezeno:** S5, 2026-07-28, bench test na hotovém HW (TEST_PLAN.md Fáze 2, krok
"Restartovat ESP32... zatímco podmínka stále platí")
**Nahlásil:** Lubor

**Příznak:** Po restartu ESP32 se `Main System Enable` switch vrátí na OFF, i když
byl před restartem ON. Stejný problém má `Simulation Mode`.

**Root cause:**

`switch: platform: template` (base `switch` schema, `esphome/components/switch/__init__.py`)
má defaultní `restore_mode: ALWAYS_OFF`, pokud není explicitně přepsán — netýká se
jen `platform: output` switchů (kde je to běžně vidět), ale i template switchů.

`TemplateSwitch::setup()` (`esphome/components/template/switch/template_switch.cpp:37-52`)
na základě `restore_mode` zjistí `initial_state` a pokud má hodnotu, **aktivně
zavolá `turn_on()`/`turn_off()`** — což spustí `turn_on_action`/`turn_off_action`,
úplně stejně, jako by to udělal uživatel ručně. S `ALWAYS_OFF` se tedy při každém
bootu zavolá `turn_off()` → `turn_off_action` → `id(main_system_enabled) = false;`
(u Main System Enable) resp. `id(sim_mode_state) = false;` (u Simulation Mode) —
**přepíše to hodnotu, kterou příslušná globální proměnná mezitím správně obnovila
ze svého vlastního `restore_value: true`.**

Mechanismus je nezávislý na tom, že switch má `lambda:` pro zobrazovaný stav — ten
lambda běží až v `loop()`, `setup()` proběhne dřív a `turn_off_action` už mezitím
stihne globální proměnnou přepsat.

**Vedlejší dopad:** Tenhle bug navíc **blokoval smysluplné otestování OI8/ADR-006**
(boot-resume defrostu) — `DEFROST_ORDERED` lambda má `if (!id(main_system_enabled))
return false;`, takže s `main_system_enabled` resetovaným na `false` při každém
bootu by edge-trigger nikdy nedetekoval rising edge, bez ohledu na senzorová data.

**Oprava:** Přidat `restore_mode: DISABLED` na oba template switche
(`sw_main_system_enable`, `sim_mode`). S `DISABLED` `get_initial_state_with_restore_mode()`
vrátí prázdný optional, `setup()` nezavolá žádnou akci, a zobrazovaný stav se
odvozuje čistě z `lambda:` (která už čte správně obnovenou globální proměnnou).

**Status:** Opraveno ve firmware YAML (S5), čeká na potvrzení re-testem.

**Poučení (AP kandidát?):** Jakýkoli `platform: template` switch, který zrcadlí
`restore_value: true` globál přes `lambda:` a mění ho v `turn_on_action`/
`turn_off_action`, MUSÍ mít `restore_mode: DISABLED` — jinak defaultní `ALWAYS_OFF`
tiše přepíše globál při každém bootu. Zkontrolovat, jestli tenhle vzor není i jinde
v budoucích switchích (žádný další v aktuálním YAML tenhle pattern nemá — jediné
dva byly `sw_main_system_enable` a `sim_mode`).

---

### BUG-002 — Přepnutí Simulation Mode (kterýmkoli směrem) vždy spustí oba heatery

**Nalezeno:** S5, 2026-07-28, bench test na hotovém HW (TEST_PLAN.md Fáze 2, řádek
"Vypnout Simulation Mode po testu")
**Nahlásil:** Lubor

**Příznak:** Přepnutí `Simulation Mode` ON→OFF nebo OFF→ON vždy spustí Heater
Chassis i Heater Drain, i když reálné teploty (~27,5 °C) ani simulované hodnoty
(`Sim Chassis`/`Sim Outside` ~5 °C) samy o sobě podmínku `DEFROST_ORDERED`
nesplňují.

**Root cause:**

`t_outside_used`, `t_evap_used`, `t_chassis_used`, `t_drain_used` (template senzory,
`update_interval: 5s`) jsou čtyři **nezávislé** `PollingComponent` instance — každá
má vlastní interní časovač, nesynchronizovaný ani mezi sebou, ani s přepnutím
`sim_mode` switche. Po přepnutí `sim_mode` tedy vznikne okno (až 5 s), kdy některé
`_used` senzory už čtou podle nového režimu a jiné ještě podle starého.

`DEFROST_ORDERED` (`bs_defrost`) porovnává **dva** takové senzory najednou
(`t_evap_used` vs. `t_outside_used`). Při reálné hodnotě ~27,5 °C a simulované
~5 °C je rozdíl (~22,5 °C) hluboko nad `def_dt_th` (7,0 °C) — pokud se aktualizuje
jen jeden z páru dřív než druhý (typicky ano, nejsou synchronní), objeví se
kombinace "jeden starý, jeden nový" hodnoty, jejíž rozdíl podmínku AND splní, i
když ani čistě reálná, ani čistě simulovaná dvojice by ji nesplnila. Funguje to
symetricky oběma směry přepnutí, a s tak velkým rozdílem real/sim hodnot prakticky
jistě (proto "vždy").

Edge-trigger to zachytí jako vzestupnou hranu → `run_defrost` → a díky floor logice
(ADR-008) topení běží garantovaně dál i poté, co se `_used` senzory srovnají zpátky
na konzistentní (a podmínku nesplňující) hodnoty.

**Oprava:** `sim_mode` switch (`turn_on_action`/`turn_off_action`) po nastavení
`sim_mode_state` vynutí `component.update` na všech čtyřech `_used` senzorech —
recompute proběhne synchronně, hned po přepnutí, prakticky simultánně, čímž se
okno zúží na nulu pro přepínání přes tenhle switch (nemění dokumentovanou
architekturu simulační vrstvy z ARCHITECTURE.md §3.2, jen ji dělá atomickou při
přepnutí).

**Status:** Opraveno ve firmware YAML (S5), čeká na potvrzení re-testem.

**Poučení:** Řídicí logika, která porovnává dva nezávisle pollované template
senzory najednou (ne jen čte jeden), je zranitelná vůči podobné race condition
kdykoli se jejich společná vstupní podmínka (zde `sim_mode`) mění skokově. Pokud se
v budoucnu přidá další podobná víceparametrová podmínka, zvážit stejný vzor
(explicitní `component.update` při přepnutí zdrojového switche) nebo přesunout
srovnání přímo do jedné lambdy bez mezikroku přes nezávisle pollované senzory.

---

## Anti-patterny (AP-NNN)

*(zatím žádné, projektově specifické — obecné AP-COM-* pro komunikaci s AI viz
WORKING_AGREEMENT.md §6)*

---

*Poslední BUG: BUG-002. Příští číslo: BUG-003.*
*Poslední AP: žádný. Příští číslo: AP-001.*
