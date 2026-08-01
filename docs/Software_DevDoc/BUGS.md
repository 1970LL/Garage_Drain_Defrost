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

**Status:** Opraveno ve firmware YAML (S5), **bench potvrzeno** (TEST_PLAN.md Fáze 2).

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

**Status:** Opraveno ve firmware YAML (S5), **bench potvrzeno** (TEST_PLAN.md Fáze 2)
— následně nahrazeno hlubší opravou BUG-005 (stejná symptomatika, jiný root cause).

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

**Status:** Opraveno ve firmware YAML (S5), **bench potvrzeno** (TEST_PLAN.md
Fáze 4) — fast tier hodnoty (5s/20s) později upraveny BUG-006 (T3/T4 sjednoceno
na 5s, base interval 120s→20s), stejný fix princip.

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

**Status:** Opraveno ve firmware YAML (S5), **bench potvrzeno** (TEST_PLAN.md
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

### BUG-005 — Přepnutí Simulation Mode nahodile spustí oba heatery (BUG-002 fix nedovřen)

**Nalezeno:** S10, 2026-07-31, bench test ADR-004 (Fáze 7, ERR LED test v reálném
režimu, T1/T2 ~30°C reálné, T3/T4 NaN — nepřipojené)
**Nahlásil:** Lubor

**Příznak:** Přepnutí `Simulation Mode` (kterýmkoli směrem) nahodile ("chybuje
oběma směry") spustí oba heatery, i když `HEAT_MODE`/`DEFROST_ORDERED` čtené
z HA/dashboardu ukazují OFF. Po uplynutí floor doby (ADR-008) se heatery samy
vypnou. Vypadá jako recidiva BUG-002, i když BUG-002 fix (synchronní
`component.update` na všech čtyřech `_used` senzorech v `turn_on_action`/
`turn_off_action`) je stále v kódu.

**Root cause:**

Log (Sim ON, 21:20:31–21:20:36) ukázal přesný mechanismus:

```
21:20:31.428  Simulation Mode Turning ON.
21:20:31.428  Outside Temperature (used) >> 29.8 °C   ← STÁLE reálná hodnota!
21:20:31.449  Gas Inlet Temperature (used) >> 30.2 °C ← STÁLE reálná hodnota!
21:20:31.461  Simulation Mode >> ON                    ← publish_state až TEĎ
21:20:35.454  Outside Temperature (used) >> 1.5 °C     ← vlastní 5s poll, sim hodnota
21:20:35.473  HEAT_MODE >> ON
21:20:35.497  DEFROST_ORDERED >> ON                    ← Gas Inlet ještě STALE 30.2°C!
21:20:35.677  Heater Chassis Turning ON.
21:20:36.309  Gas Inlet Temperature (used) >> 2.0 °C   ← až teď dorazí sim hodnota
21:20:36.309  DEFROST_ORDERED >> OFF
```

`sim_mode` je `platform: template` switch s `optimistic: false` a `lambda:`.
`TemplateSwitch::write_state()` (`template_switch.cpp:16-31`) při zapnutí/vypnutí
**synchronně** spustí `turn_on_action`/`turn_off_action`, ale `publish_state()`
zavolá jen pokud je switch `optimistic: true` — u neoptimistického switche se
`.state` publikuje až při **příštím** `loop()` průchodu, kdy se znovu vyhodnotí
jeho vlastní `lambda: return id(sim_mode_state);`.

BUG-002 fix (`component.update:` na čtyřech `_used` senzorech) běží jako součást
`turn_on_action`/`turn_off_action` — tedy **dřív**, než se `id(sim_mode).state`
vůbec stihne publikovat. Jejich lambdy (`if (id(sim_mode).state) ... else ...`)
proto v tu chvíli ještě čtou **starou** hodnotu switche → vynucený update je
fakticky no-op (přečte znovu starý režim). Všechny čtyři `_used` senzory se pak
vrátí na svoje nezávislé, vzájemně neseřízené 5s pollery — přesně ten samý
mechanismus, který měl BUG-002 fix odstranit, se znovu otevře. Log to potvrzuje:
`Outside Temperature (used)` dostane fresh sim hodnotu (1.5°C) o 4s dřív než
`Gas Inlet Temperature (used)` (2.0°C) — během té mezery `DEFROST_ORDERED`
vyhodnotí `abs_ok`/`dt_ok` proti STALE reálnému Gas Inlet (~30°C) vs. FRESH sim
Outside (~1.5°C), rozdíl ~28°C hluboko nad `def_dt_th` → spurious trigger.
Nahodilost (`chybuje oběma směry`) odpovídá tomu, který ze čtyř senzorů se
náhodou stihne aktualizovat první.

**Oprava:** Čtyři `_used` lambdy (`t_outside_used`, `t_gas_inlet_used`,
`t_chassis_used`, `t_drain_used`) čtou místo `id(sim_mode).state` přímo globál
`id(sim_mode_state)` — ten se nastavuje synchronně jako úplně první krok
`turn_on_action`/`turn_off_action`, tedy dřív, než na něj kterýkoli ze čtyř
`component.update:` volání narazí. Vynucený update tak poprvé skutečně čte NOVÝ
režim, ne starý. Switch samotný (`sim_mode`) svoje zobrazované UI `state`
zveřejní beze změny, jen s obvyklým jednomístným zpožděním `loop()` — na to nic
jiného v řídicí logice není navázané.

**Status:** Opraveno ve firmware YAML (S10), **bench potvrzeno** (opakované
přepínání Simulation Mode oběma směry z reálného stavu s T1/T2 teplými, žádný
spurious heater start).

**Poučení (AP kandidát?):** `optimistic: false` `platform: template` switch
publikuje svůj vlastní `.state` až s jednou `loop()` iterací zpoždění za
`turn_on_action`/`turn_off_action` (protože `write_state()` volá
`publish_state()` jen v optimistic větvi — viz `template_switch.cpp`). Kód
spuštěný **uvnitř** téhle akce nesmí spoléhat na `id(<ten samý switch>).state`
jako na "už platí nová hodnota" — musí číst přímo řídicí globál/proměnnou, kterou
akce sama nastavuje. Stejný vzor (`id(main_system_enabled)` vs. hypotetický
`id(sw_main_system_enable).state`) zkontrolovat, pokud v budoucnu přibude další
logika uvnitř `sw_main_system_enable`'s turn_on/turn_off_action.

---

### BUG-006 — Selhaný senzor se v `_used`/error detekci projeví až za 10-20 minut

**Nalezeno:** S10, 2026-07-31, bench test ADR-004 (Fáze 7, `err_t1` — fyzicky
odpojen datový vodič T1)
**Nahlásil:** Lubor

**Příznak:** Po odpojení datového vodiče T1 zůstala `Outside Temperature`
"zamrzlá" na poslední platné hodnotě (30,1 °C), nikdy nepřešla na `nan`, `err_t1`
se nespustil. Následně stejný jev pozorován i na `Gas Inlet Temperature (used)`
(nová platná hodnota z rawlogu se do `used` nepropsala) — vypadalo to jako
regrese v `_used` indirection vrstvě, ale šlo o hlubší, obecnější problém.

**Root cause:**

Dvě věci dohromady:

1. `MedianFilter::get_window_values_()` (`filter.cpp`) **explicitně přeskakuje
   NaN hodnoty** v okně při výpočtu mediánu — publikuje medián ze zbylých
   *platných* vzorků, dokud nejsou NaN úplně všechny (`window_size: 5`).
   Jednotlivé/pár výpadků se tak potichu maskují zbylými dobrými vzorky.
2. `t_outside` neměl žádné on-demand zrychlení (na rozdíl od T2/T3/T4) — běžel
   čistě na `update_interval: 120s`. T2/T3/T4 měly zrychlení jen podmíněně
   (HEAT_MODE/běžící heater) — mimo tenhle stav byly na stejném pomalém
   120s základu.

Kombinace: s `window_size: 5` a `send_every: 5` (stejná hodnota) filtr publikuje
jen jednou za 5 syrových vzorků, a při 120s intervalu potřebuje **5 po sobě
jdoucích neúspěšných čtení**, než se v okně nenajde jediná platná hodnota a
filtr konečně publikuje `NAN` — to je `5 × 120s = 10 minut` v nejlepším případě,
až ~20 minut, pokud odpojení nastane uprostřed 5-vzorkového cyklu. `_used`
vrstva přitom funguje správně — věrně mirroruje `t_X.state`, jen ten sám je
takhle pomalý.

**Oprava:** Na všech čtyřech raw DS18B20 senzorech (T1-T4):
- `update_interval` snížen ze 120s na **20s** (základní/pomalý tier)
- on-demand zrychlení (T2 při HEAT_MODE, T3/T4 při běžícím heateru) sjednoceno
  na **5s** (rychlý tier) — u T3/T4 to bylo dřív 20s, tedy stejně jako nový
  základ, takže bezpředmětné; zvýšeno na 5s aby mělo smysl
- median filtr na všech čtyřech **zakomentován** (ne smazán) — tyhle senzory
  (venkovní/HVAC/šasi/odtok teploty) nedriftují ani nemají rázové špičky,
  filtrace jen zpožďovala detekci selhání bez reálného přínosu. Lze vrátit
  (odkomentovat), pokud by se ve field fázi ukázal reálný šum na čtení.

**Status:** Opraveno ve firmware YAML (S10), **bench potvrzeno** (odpojen T1
datový vodič, `err_t1` naskočil rychle, ne za 10-20 min).

**Poučení (AP kandidát?):** Filtr, který explicitně ignoruje NaN při výpočtu
výsledku (běžný a žádoucí vzor pro potlačení šumu), má vedlejší efekt: maskuje
i *skutečné, trvalé* selhání zdroje po dobu `window_size × update_interval`.
U pomalu se měnících, nešumících veličin (venkovní teplota apod.) filtrace
nepřidává hodnotu a jen prodlužuje detekci poruchy — zvážit u budoucích senzorů,
jestli filtr vůbec dává smysl, než ho přidat "pro jistotu".

---

## Anti-patterny (AP-NNN)

### AP-001 — Tvrdý HW reset obchází graceful NVS sync, `restore_value` může ztratit poslední změnu

**Nalezeno:** S12, 2026-08-01, bench test OI17/OI4 (boot chování se Simulation Mode)
**Nahlásil:** Lubor

**Kontext:** `esp32: preferences:` (auto-loaded komponenta, `flash_write_interval`
default **60s** — `esphome/components/preferences/__init__.py:16`, YAML tuhle
hodnotu nikde nepřepisuje) ukládá `restore_value: true` entity/globály jen do
RAM cache (`ESP32PreferenceBackend::save()`, `esp32/preferences.cpp:40-54`).
Fyzický zápis do NVS flash provede až `sync()` — buď periodicky (každých 60s),
nebo při **graceful** shutdownu (`on_shutdown()`: OTA reflash, HA "Restart"
tlačítko, `api.reboot`). Fyzické EN/RESET tlačítko na ESP32 devkitu (mimo GPIO
mapu projektu — přímo na chip reset lince, ne ESPHome `button:`) je elektricky
identické s výpadkem napájení: žádný firmware kód neproběhne, `on_shutdown()`
nemá šanci se zavolat. Pokud se stav (Simulation Mode, prahy, sim slidery, ...)
změní méně než `flash_write_interval` před tvrdým resetem/výpadkem, změna se
ztratí — zařízení po bootu ukáže poslední **skutečně do flash zapsanou**
hodnotu, ne poslední nastavenou.

**Ověřeno na logu:** `'Simulation Mode' >> OFF` v `11:27:46.619`, tvrdý reset
(EN tlačítko) v `11:28:02.193` — odstup jen ~15,5s, dobře pod 60s oknem. Po
bootu Simulation Mode = ON (stará hodnota, ne OFF). Lubor potvrdil: reset
tlačítkem **několik minut** po přepnutí → OFF správně přežije (sync stihl
proběhnout mezitím).

**Rozhodnutí (Lubor, 2026-08-01):** Ponecháno beze změny — `flash_write_interval:
60s` je pro reálný provoz v pořádku (tvrdé resety/výpadky napájení jsou v poli
vzácné). Při bench testování je třeba tohle chování respektovat: buď počkat
60s+ po změně stavu před tvrdým resetem, nebo použít graceful reboot (HA/API
"Restart"), pokud má stav přežít okamžitě.

**Poučení:** `restore_value: true` negarantuje přežití *poslední* změny přes
*jakýkoli* restart — jen přes restarty, které buď proběhly graceful, nebo
nastaly po uplynutí `flash_write_interval` od poslední změny. Týká se všech
restore_value entit v projektu (Main System Enable, Simulation Mode, prahové
hodnoty, sim slidery), ne jen jedné — obecná vlastnost mechanismu, ne
lokalizovaná chyba.

---

*Poslední BUG: BUG-006. Příští číslo: BUG-007.*
*Poslední AP: AP-001. Příští číslo: AP-002.*
*Poslední AP: žádný. Příští číslo: AP-001.*
