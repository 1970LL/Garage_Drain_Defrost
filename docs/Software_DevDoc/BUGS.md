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

### BUG-003 — On-demand DS18B20 refresh intervaly hammerují sběrnici rychleji, než tvrdí komentář

**Nalezeno:** S5, 2026-07-30, bench test na hotovém HW (TEST_PLAN.md Fáze 4, T1/T2
warm-up krok)
**Nahlásil:** Lubor

**Příznak:** Dvě nezávislá pozorování v logu:
1. Vyhodnocení `err_t1_t2_fail` po resetu není konzistentní — někdy poruchu nahlásí,
   někdy ne (i když T1/T2 hodnoty se objeví v logu okamžitě po bootu). Pokud se
   porucha nahlásí, srovná se do ~10-20 s.
2. V logu se opakovaně (ne vždy) objevují `[W][component:395]: dallas_temp.sensor
   set Warning flag: scratch pad checksum invalid` se scratch padem `FF.FF.FF.FF.FF.FF.FF.FF.FF`,
   a `'Evaporator Temperature': Got Temperature=...` se loguje s frekvencí ~1 s,
   přestože senzor má `update_interval: 120s`.

**Root cause:**

Dva "on-demand refresh" intervaly ([`ESP32-D0WD-V3_Gar_Drain_Defrost.yaml`](../../firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml)):

```yaml
# HEAT_MODE active → refresh Evaporator (T2) every 5 s ...
- interval: 1s
  then:
  - if:
      condition: {binary_sensor.is_on: bs_heat_mode}
      then: [component.update: t_evap]

# Any heater ON → refresh Chassis (T3) & Drain (T4) every 20 s ...
- interval: 4s
  then:
  - if:
      condition: {or: [switch.is_on: sw_heater_chassis, switch.is_on: sw_heater_drain]}
      then: [component.update: t_chassis, component.update: t_drain]
```

Komentáře tvrdí kadenci 5 s / 20 s, ale samotný `interval:` byl nastaven na 1 s / 4 s
a kód nemá žádné počítání ticků — `component.update` se tedy spouští **při každém
tiku**, ne po pátém, jak komentář popisuje. V bench testu bylo `Sim Outside = 2.0 °C`
(pod `heat_on_th`) → `HEAT_MODE` ON → T2 blok reálně bušil `component.update: t_evap`
každou sekundu.

`DallasTemperatureSensor::update()` (`dallas_temp.cpp:39-56`) při zavolání pošle nový
`START_CONVERSION` a naplánuje čtení scratch padu přes `set_timeout()` po
`millis_to_wait_for_conversion_()` (750 ms při 12-bit rozlišení, `dallas_temp.cpp:15-25`).
Nekontroluje, jestli předchozí konverze ještě neběží. Když `component.update()` přijde
znovu dřív, než předchozí konverze doběhne (při 1s intervalu vs. 750ms konverzi je to
těsné a snadno se to timingem jiných komponent posune), dojde ke kolizi na sdílené
1-Wire sběrnici (GPIO21) — výsledkem je poškozený scratch pad (`FF...FF`) a neplatný
CRC (Pozorování 2). Stejná sběrnice je sdílená s T1 (`t_outside`) — kolize může
zasáhnout i jeho čtení, což vysvětluje transientní NaN hned po bootu a nekonzistentní
`err_t1_t2_fail` chování (Pozorování 1); je to race condition, ne deterministická chyba,
proto se projevuje jen "někdy".

Stejný vzor (kratší `interval:`, chybějící gating) je i v T3/T4 bloku — zatím
neškodný, protože T3/T4 mají fiktivní adresy (`0x3`/`0x4`, žádný fyzický senzor), ale
po komisioningu reálného HW (OI1) by šlo o totéž riziko.

**Oprava:** Změněna perioda obou intervalů tak, aby skutečně odpovídala kadenci
popsané v komentáři — `interval: 1s` → `interval: 5s` (T2 blok), `interval: 4s` →
`interval: 20s` (T3/T4 blok). Žádná nová abstrakce (počítadlo ticků) není potřeba,
komentář popisoval zamýšlené chování správně, jen implementace neodpovídala.

**Status:** Opraveno ve firmware YAML (S5), čeká na potvrzení re-testem (TEST_PLAN.md
Fáze 4).

**Poučení (AP kandidát?):** `interval:` blok s podmínkou uvnitř (`if: ... then:
component.update`) nemá žádnou vestavěnou ochranu proti tomu, aby perioda intervalu
neodpovídala komentáři/záměru — na rozdíl od `update_interval:` na samotném senzoru,
kde je perioda a chování svázané v jednom místě. Při psaní podobných "on-demand
refresh" bloků zkontrolovat, že `interval:` hodnota je numericky shodná s tím, co
tvrdí komentář, zvlášť když refreshovaný senzor sdílí fyzickou sběrnici (1-Wire, I2C,
UART) s jinými pravidelně pollovanými senzory — bus kolize nebývá vidět v `esphome
config`/`compile`, jen za běhu na reálném HW.

---

### BUG-004 — `err_t1_t2_fail` po bootu občas nahlásí poruchu, i když senzory jsou v pořádku

**Nalezeno:** S5, 2026-07-30, bench test na hotovém HW (TEST_PLAN.md Fáze 4, T1/T2
warm-up krok), po opravě BUG-003
**Nahlásil:** Lubor
**HW kontext:** DS18B20 T1-T4 potvrzeno 3-vodičově (VCC/GND/DATA, externí napájení,
ne parazitní) — vyloučilo to hypotézu o kolapsu "strong pull-up" při parazitním
napájení jako příčinu.

**Příznak:** Po restartu `err_t1_t2_fail` nekonzistentně (ne vždy) nahlásí poruchu,
přestože syrová čtení T1/T2 v logu vypadají v pořádku (`Got Temperature=...` s platným
checksumem hned od prvního čtení). Pokud se porucha objeví, sama zmizí do ~18-20 s.

**Root cause:**

Dva nezávislé zdroje náhodnosti se v prvních sekundách po bootu potkávají:

1. `App.scheduler`/`Scheduler::set_interval` (`scheduler.cpp:161-166`) spouští **první**
   provedení KAŽDÉHO `interval:`/`PollingComponent` s náhodným offsetem
   `0 až min(interval/2, 5000ms)` po jeho registraci. To platí jak pro senzory
   (`t_outside`, `t_evap`, `update_interval: 120s` → offset 0-5000ms), tak pro sám
   error-check (`interval: 20s` → offset 0-5000ms) — navzájem nezávisle.
2. `MedianFilter`/`SlidingWindowFilter` (`filter.cpp:35-36,41-63`) s `send_first_at: 1`
   publikuje výsledek hned po **prvním** přijatém vzorku (`send_at_ = send_every_ -
   send_first_at_` → po prvním `new_value()` už `send_at_ >= send_every_`). Pokud je
   ovšem tenhle první vzorek sám o sobě `NAN` (senzor v tu chvíli ještě nic
   nepublikoval — `Sensor::state` je defaultně `NAN`, dokud neproběhne první
   `publish_state()`), publikuje se okamžitě `NAN` — a další publikace přijde až po
   **5 dalších** vzorcích (`send_every: 5`), ne dřív.

Kombinace: `err_t1_t2_fail`'s vlastní první kontrola může (díky svému nezávislému
náhodnému offsetu) proběhnout dřív, než T1 nebo T2 vůbec stihnou dokončit svou úplně
první konverzi (750 ms při 12-bit rozlišení) a publikovat cokoliv — v tu chvíli vidí
defaultní `NAN` a **správně** (dle svého kódu) nahlásí `should_error = true`. Jakmile
senzor tuhle "prázdnou" hodnotu jednou publikuje (nebo pokud check proběhne až po ní),
filtr díky `send_first_at: 1` čeká dalších 5 refresh cyklů (5 s dle BUG-003 fixu = až
25 s), než publikuje reálnou teplotu a chybu smaže. Je to čistě timing race dvou
nezávisle náhodně naplánovaných komponent — proto "někdy ano, někdy ne" — **ne**
skutečná porucha senzoru ani kolize na sběrnici.

**Oprava:** `err_t1_t2_fail` a `err_t3_t4_fail` intervaly nevyhodnocují stav prvních
10 s po bootu (`lambda: 'return millis() >= 10000;'` jako podmínka kolem těla obou
intervalů). 10 s bezpečně pokrývá nejhorší případ (5 s scheduler offset + 750 ms
konverze + rezerva) bez zpomalení detekce skutečné poruchy za běhu (grace platí jen
jednou, hned po bootu, ne opakovaně).

**Status:** Opraveno ve firmware YAML (S5), čeká na potvrzení re-testem (TEST_PLAN.md
Fáze 4).

**Poučení (AP kandidát?):** Kombinace "publikuj hned po prvním vzorku" (`send_first_at:
1`, žádoucí pro rychlé zveřejnění po bootu, viz OI12) a "vyhodnocuj chybu okamžitě bez
debounce" je zranitelná vůči startovní timing race, pokud obě strany (senzor i error
check) mají svůj vlastní nezávislý náhodný start. Řešení není vracet se k pomalejšímu
filtru (to by vrátilo starý ~10min problém, co řešilo OI12), ale dát error-checku
krátkou "startup grace" — vyhodnocovat chybu, až senzory měly reálnou šanci se poprvé
ozvat. Zvážit stejný vzor u budoucích rychlých (bez WiFi-style dlouhého debounce)
error-checků nad `PollingComponent` senzory.

---

## Anti-patterny (AP-NNN)

*(zatím žádné, projektově specifické — obecné AP-COM-* pro komunikaci s AI viz
WORKING_AGREEMENT.md §6)*

---

*Poslední BUG: BUG-004. Příští číslo: BUG-005.*
*Poslední AP: žádný. Příští číslo: AP-001.*
