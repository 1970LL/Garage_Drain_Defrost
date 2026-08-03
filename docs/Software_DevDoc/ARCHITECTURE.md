# ARCHITECTURE — Garage_Drain_Defrost

> **Účel:** Snapshot současného stavu systému. Popisuje *co* je postaveno, ne *proč*.
> Pro zdůvodnění technických voleb viz `DECISIONS.md` (ADR-NNN odkazy v textu).
> **Vlastník:** Architekt.
> **Aktuální stav:** Bench-validovaný firmware (Fáze 0–10 `TEST_PLAN.md` kompletní,
> BUG-001 až BUG-007 opraveny), HA napárováno (S15, 2026-08-01). Tento dokument
> popisuje kód *tak, jak je nyní* — žádné otevřené TODO v YAML samotném; zbylé
> BACKLOG položky čekají na zimní pozorování nebo Field Deployment.
> **Phase:** 4 — Field Deployment (viz `ROADMAP.md`).

---

## 1. Účel a scope dokumentu

ARCHITECTURE.md je deskriptivní snapshot — popisuje aktuální stav systému tak, aby
nový čtenář (nebo budoucí session) získal kompletní orientaci, aniž by četl celý YAML.

**Vztah k ostatním dokumentům:**

| Dokument | Co obsahuje |
|---|---|
| `ARCHITECTURE.md` (tento) | Co je — HW, FW struktura, logika, error mapa, konfigurace |
| `DECISIONS.md` | Proč to tak je — log architektonických rozhodnutí (ADR) |
| `BACKLOG.md` | Co dál — otevřené body (naplní se code review) |
| `PROJECT_VISION.md` | Proč projekt existuje, přístup ke kvalitě (odlehčený) |

---

## 2. Hardware Overview

### 2.1 MCU & Connectivity

- **MCU:** ESP32-WROOM-32UE (38-pin module, externí anténa)
- **Framework:** ESPHome s ESP-IDF
- **Síť:** Wi-Fi
- **HA integrace:** ESPHome Native API + Web Server (port 80)
- **OTA:** ESPHome OTA platform

### 2.2 GPIO Mapa

**Analog / 1-Wire:**

| Signal | GPIO | Popis |
|---|---|---|
| 1-Wire bus | GPIO21 | Sdílená sběrnice pro 4× DS18B20 |

**Digital Inputs:**

| Signal | GPIO | Popis |
|---|---|---|
| DI1 | GPIO36 | Rezervováno, interní/log-only |
| DI2 | GPIO39 | Rezervováno, interní/log-only |

**Digital Outputs (MOSFET BSS138, open-drain, 5V/12V jumper-selectable):**

| Signal | GPIO | Funkce |
|---|---|---|
| DO1 | GPIO32 | SSR — topení šasi (230 VAC) |
| DO2 | GPIO33 | SSR — topení odtokové trubky (230 VAC) |
| DO3 | GPIO25 | Rezerva (OC 5/12 V), interní |
| DO4 | GPIO26 | Rezerva (OC 5/12 V), interní |

**Status LED (přes BSS138, HIGH = ON):**

| Signal | GPIO | Funkce |
|---|---|---|
| WD | GPIO4 | 4 stavové vzory (`led_sequencer`, ADR-004) — viz §5 |
| ERR | GPIO2 | Chybové pulzní kódy (`led_sequencer`, ADR-004) — viz §5 |

⚠ GPIO2 je strapping pin (ESP32 strapping piny: GPIO0/2/5/12/15) — bezpečné díky BSS138
driveru. GPIO4 strapping pin **není** — původní poznámka (převzatá z README/Windows) byla
fakticky nesprávná; opraveno po ověření přes `esphome config` (varování padlo jen na GPIO2).

---

## 3. Senzory (DS18B20, 1-Wire)

Čtyři DS18B20 senzory na sdílené sběrnici GPIO21:

| ID | Název (HA, skupina Servisní) | Adresa | Účel |
|---|---|---|---|
| `t_outside` | Venkovní teplota (čidlo) | `0x3400000051876f28` — komisionováno S5 (2026-07-28) | Venkovní teplota |
| `t_gas_inlet` | Teplota výměníku (čidlo) | `0xab030a9794259928` — komisionováno S5 (2026-07-28) | Teplota plynového vývodu výměníku venkovní jednotky (T2) |
| `t_chassis` | Teplota Chassis (čidlo) | `0xa90625910004ba28` — komisionováno S11 (2026-08-01) | Teplota šasi jednotky |
| `t_drain` | Teplota odpadu (čidlo) | `0xb6062591abac6f28` — komisionováno S11 (2026-08-01) | Teplota odtokové trubky |

Raw čidla jsou v HA ve skupině **Servisní** (diagnostika) — provozní hodnotu,
kterou čte řídicí logika, exponují odpovídající `_used` senzory ve skupině
**Provoz** (§3.2). Device grouping a CZ názvosloví: §9 (ADR-005).

Základní `update_interval: 20s`. **Bez median filtru** (BUG-006, S10 2026-07-31
— zakomentován, ne smazán, viz §3.1).

**Fyzické umístění (ADR-011, S9 2026-07-31):**

- **T2 (`t_gas_inlet`):** kontaktně na trubku b — plynový vývod výměníku venkovní
  jednotky (Toshiba Shorai Edge RAS-B13G3KVSG-E), Ø 9,5 mm, bez izolace, spojující
  výměník s vývodem C čtyřcestného ventilu; montáž co nejblíže k tělesu výměníku.
  Přejmenováno z `t_evap` — starý název implikoval TE-analogickou pozici (výstupní/
  kapalinová strana, trubka a), která je fyzicky nevhodná (voština nedává přístup na
  měděné potrubí) a navíc dává pozdní signál (ukončovací logika Toshiby, ne
  detekční). Plynový vývod je nejčasnější bod detekce reverzního defrost cyklu.
- **T3 (`t_chassis`):** kontaktně na vnější (spodní) stranu pánve chassis, v mezeře
  meandru topného kabelu (ne na kabel samotný — mezera čte nejstudenější kov,
  konzervativní ochrana). Vnější strana eliminuje kontakt s vodou/ledem.

### 3.1 Dynamic Polling Strategy

Dvoutier: **20s základ** (všechny 4 senzory, vždy) / **5s rychlý tier** (jen
když je aktuálně relevantní):

- Když je **HEAT_MODE aktivní** → `t_gas_inlet` se force-refreshuje každých 5 s (`component.update`)
- Když je **kterýkoli heater ON** → `t_chassis` a `t_drain` se force-refreshují každých 5 s

`t_outside` nemá vlastní rychlý tier — je vždy relevantní (krmí HEAT_MODE
přímo), takže 20s základ platí napořád.

(BUG-003, S5 2026-07-30: intervaly byly původně 1s/4s smyčky s chybným předpokladem,
že mediánový filtr efektivní periodu sám natáhne na 5s/20s — ve skutečnosti to jen
zbytečně bušilo do sběrnice. Opraveno na přímou periodu 5s/20s.)

**BUG-006 (S10, 2026-07-31):** median filtr (byl na všech 4 senzorech) explicitně
přeskakoval NaN hodnoty při výpočtu mediánu — jednotlivé výpadky se maskovaly
zbylými platnými vzorky v okně, a s `window_size:5`/`send_every:5` a (dřívějším)
120s základem trvalo až 10-20 minut, než selhání senzoru skutečně projevilo jako
`NAN` a spustilo `err_t1..t4`. Filtr zakomentován (tyhle senzory nedriftují),
základ zrychlen na 20s, rychlý tier sjednocen na 5s (T3/T4 měly dřív 20s, což
bylo po zrychlení základu bezpředmětné). Detaily: `BUGS.md` BUG-006.

**Zvážený, zamítnutý nápad — vzájemný fázový posun mezi senzory (S12, 2026-08-01):**
Log ukázal, že všechny čtyři raw senzory publikují na svém vlastním pevném
offsetu uvnitř 20s okna (např. Chassis vždy na `:X7.6`, Outside na `:X0.0`,
Gas Inlet na `:X0.7`) — dané ESPHome schedulerem (první běh dostane náhodný
posun 0-5s po bootu, pak periodicky odtud). Není to jitter měnící se cyklus od
cyklu, jen stabilní stagger. Zvažováno, jestli to představuje riziko "torn
read" tam, kde logika porovnává dva různé senzory najednou (jediné takové
místo: DEFROST_ORDERED, `t_gas_inlet_used − t_outside_used`, offset ~0,7s) —
**zamítnuto jako bezpředmětné**: tepelná setrvačnost potrubí/plechu mění
teplotu řádově °C/minutu, ne °C/sekundu, takže 0,7s posun představuje
setiny stupně, hluboko pod `accuracy_decimals: 1` rozlišením senzoru.
Vynutit skutečnou simultánnost (broadcast convert na 1-Wire) by šlo proti
poučení z BUG-003 (přerývání convert/read cyklů na sdílené sběrnici
způsobovalo checksum chyby) — nestojí to za komplikaci, fyzika defrostu to
nepotřebuje.

### 3.2 Simulation Layer ("used" sensors)

Nad reálnými senzory existuje indirection vrstva — čtyři `template` senzory
(`t_outside_used`, `t_gas_inlet_used`, `t_chassis_used`, `t_drain_used`; v HA
"Venkovní teplota" / "Teplota výměníku" / "Teplota Chassis" / "Teplota odpadu",
skupina **Provoz**, ADR-005), které přepínají mezi reálnou hodnotou a
simulovanou hodnotou podle globálního přepínače `sim_mode`:

```
sim_mode == true  → vrací sim_t_outside / sim_t_gas_inlet / sim_t_chassis / sim_t_drain
sim_mode == false → vrací t_outside / t_gas_inlet / t_chassis / t_drain
```

**Veškerá řídicí logika (HEAT_MODE, DEFROST_ORDERED) čte výhradně z `_used` senzorů**,
nikdy přímo z fyzických — to umožňuje testování bez hardwaru.

**OI17 (S12, 2026-08-01) — event-driven refresh:** `_used` senzory mají
`update_interval: never` — nepollují samy na plochém timeru. Refresh je čistě
reaktivní, přes `component.update:`:

- `on_value:` na příslušném raw `dallas_temp` senzoru (real mode zdroj) —
  spustí se přesně, když raw senzor skutečně publikuje novou hodnotu (dle
  jeho vlastního base/fast-tier intervalu popsaného v §3.1).
- `on_value:` na příslušné `sim_t_*` number entitě (sim mode zdroj) — spustí
  se okamžitě při posunu HA slideru, bez čekání na jakýkoli timer.
- `turn_on_action`/`turn_off_action` na `sim_mode` switchi (BUG-002/BUG-005
  fix, beze změny) — samotné přepnutí režimu nemění ani raw senzor, ani sim
  number, takže potřebuje svůj vlastní vynucený refresh.
- `on_boot:` (priorita -100, `esphome:` blok) — jednorázový vynucený refresh
  všech čtyř `_used` po startu, pro krajní případ přeživšího Simulation Mode
  přes reboot (`restore_value: true`), kdy obnovená hodnota number entity
  nemusí sama vyvolat `on_value:`.

Dřívější plošné `update_interval: 5s` (nezávislé na tom, jestli se zdroj pod
senzorem skutečně změnil) re-publikovalo nezměněnou hodnotu bez informační
hodnoty — zaplevňovalo log/HA traffic (OI17 nález, bench test S5). Nový
mechanismus publikuje přesně tolikrát, kolikrát se zdroj skutečně změní, a
navíc rychleji (žádné čekání na příští poll tick).

---

## 4. Řídicí logika

### 4.1 Globální stavové proměnné (`globals`)

| ID | Typ | Restore | Účel |
|---|---|---|---|
| `main_system_enabled` | bool | ano (true) | Hlavní vypínač systému |
| `sim_mode_state` | bool | ano (false) | Simulační režim |
| `defrost_running` | bool | ne | Právě probíhá defrost cyklus |
| `err_wifi_lost` | bool | ne | WiFi chyba (debounced) |
| `err_t1` | bool | ne | Chyba T1 senzoru (outside) |
| `err_t2` | bool | ne | Chyba T2 senzoru (gas inlet) |
| `err_t3` | bool | ne | Chyba T3 senzoru (chassis) |
| `err_t4` | bool | ne | Chyba T4 senzoru (drain) |
| `wifi_lost_duration_sec` | uint32 | ne | Debounce čítač pro WiFi chybu |

### 4.2 HEAT_MODE (binary_sensor, hystereze)

Aktivní, když venkovní teplota (`t_outside_used`) klesne pod `heat_on_th` (default 2,0 °C);
deaktivuje se, až teplota vystoupá nad `heat_off_th` (default 4,0 °C). Dvojitý práh brání
oscilaci. Pokud je `main_system_enabled == false`, HEAT_MODE je vždy false.

### 4.3 DEFROST_ORDERED (binary_sensor, AND logika)

Signalizuje probíhající odmrazovací cyklus venkovní jednotky. Podmínka (všechny tři musí platit):

1. **Absolutní práh:** `t_gas_inlet_used ≥ def_abs_th` (default 5,0 °C)
2. **Delta práh:** `(t_gas_inlet_used − t_outside_used) ≥ def_dt_th` (default 7,0 °C)
3. **HEAT_MODE armed:** `bs_heat_mode` je aktivní (ochrana se nařizuje jen v topné
   sezóně, ADR-009)

Pokud je systém disabled nebo HEAT_MODE OFF, DEFROST_ORDERED je vždy false.

### 4.4 Defrost cyklus (scripty)

Na vzestupnou hranu `DEFROST_ORDERED` (false→true), pokud systém je enabled a defrost
zrovna neběží (`defrost_running == false`), se spustí `run_defrost`:

```
run_defrost (mode: restart)
  ├─ defrost_running = true
  ├─ script.execute: defrost_chassis_cycle   ─┐
  ├─ script.execute: defrost_drain_cycle      ├─ běží paralelně (fire-and-forget spuštění)
  ├─ wait_until: obě dokončeny                ┘
  └─ defrost_running = false

defrost_chassis_cycle: heater_chassis ON → delay(chassis_time_floor)
                       → wait_until(!DEFROST_ORDERED, timeout = chassis_time_ceiling − chassis_time_floor) → OFF
defrost_drain_cycle:   heater_drain   ON → delay(drain_time_floor)
                       → wait_until(!DEFROST_ORDERED, timeout = drain_time_ceiling − drain_time_floor)     → OFF
```

Každý cyklus má dvě meze (ADR-008). `chassis_time_floor` / `drain_time_floor`
(default 2 / 3 min) je garantovaná minimální doba topení — běží bez ohledu na stav
podmínky. Chrání proti short-cyklování: `DEFROST_ORDERED` nemá hysterezi, takže krátké
zakolísání kolem prahu by heater vypnulo dřív, než topný kabel při tepelné setrvačnosti
šasi/trubky cokoli ohřeje. `chassis_time_ceiling` / `drain_time_ceiling` (default 20 min)
je bezpečnostní strop proti zaseknuté podmínce (ADR-007). Timeout druhé fáze je
`ceiling − floor`, takže celkový strop cyklu je `ceiling`.

Mezi floor a stropem je heater řízen reálným trváním podmínky: krátký defrost skončí
na floor, delší na poklesu `DEFROST_ORDERED`, selhání vyhodnocení na stropu.

**Retrigger:** edge-trigger `DEFROST_ORDERED` není gatovaný na `defrost_running` —
nová vzestupná hrana restartuje `run_defrost` i oba nested scripty (`mode: restart`).
`defrost_running` je čistý stavový příznak, ne interlock. Známé omezení: retrigger
resetuje rozpočet stropu, takže kmitající podmínka může topení protahovat (ADR-008
bod 8, k pozorování v zimní sezóně).

Oba nested scripty startují současně a běží nezávisle na svých mezích — chassis a drain
heater tedy nejsou synchronně vypnuté.

**Chování po rebootu (ADR-006, varianta B):** auto-resume defrostu je záměrný a
realizuje ho výhradně edge-trigger `DEFROST_ORDERED` — jeho `static bool last` se při
bootu inicializuje na `false`, takže první `DEFROST_ORDERED == true` přečtené po bootu
je vzestupná hrana → `run_defrost` fírne. Žádný explicitní `on_boot` resume ani
`restore_value` na `defrost_running` se nepoužívá. Nejhorší případ zmeškání (reboot
uprostřed reálného cyklu jednotky) je ohraničen dobou, než DS18B20 senzory nahlásí
platná data — s `send_first_at: 1` (F3/OI12) ~1 poll interval místo původních ~10 min.
Heatery jsou `restore_mode: ALWAYS_OFF`, takže toto okno je bezpečné (jednotka krátce
bez asistence, ne nechtěné topení).

### 4.5 Main System Switch

Globální vypínač (`sw_main_system_enable`). Vypnutí:
- nastaví `main_system_enabled = false`
- force-stopne všechny tři scripty (`run_defrost`, `defrost_chassis_cycle`, `defrost_drain_cycle`)
- vynutí okamžité vypnutí obou heaterů

---

## 5. Error Handling & Indikace

**ADR-004 (S10, 2026-07-31):** Pět nezávislých, souběžně platných error flagů —
původní spárované `err_t1_t2_fail`/`err_t3_t4_fail` rozděleny na per-senzor
`err_t1..err_t4` (OI11), aby měl každý svůj vlastní ERR LED kód a aby detekce
mohla číst `_used` vrstvu místo raw senzoru (Simulation Mode už netriggeruje
falešnou chybu na fyzicky nepřipojeném T3/T4, dokud je `sim_mode` ON):

| Flag | Detekce | Debounce |
|---|---|---|
| `err_wifi_lost` | `wifi.connected` == false | 300 s (5 min) souvislého výpadku |
| `err_t1` | `isnan()` na `t_outside_used` | kontrola každých 20 s |
| `err_t2` | `isnan()` na `t_gas_inlet_used` | kontrola každých 20 s |
| `err_t3` | `isnan()` na `t_chassis_used` | kontrola každých 20 s, informativní (ne kritické) |
| `err_t4` | `isnan()` na `t_drain_used` | kontrola každých 20 s, informativní (ne kritické) |

Obě kontroly mají 10s startup grace po bootu (BUG-004). T3/T4 interval
sjednocen z 60s na 20s (OI4, S12 2026-08-01) pro konzistenci s T1/T2, po
OI17/BUG-006 už není důvod, aby zaostávaly.

**ERR LED (GPIO2)** — `led_sequencer` (ADR-004, portováno z Garage_Windows beze
změny enginu, OI10), instance `err_led_seq`. Deklarativní patterny, "default"
mode — víc aktivních chyb se přehraje sekvenčně (gaps 1500/2000 ms), pořadí dle
README/tabulky (ne frequency-based jako Windows ADR-010):

| Kód | Chyba | Pattern |
|---|---|---|
| `err_wifi` | WiFi lost | 1×(300/300) |
| `err_t1` | T1 outside fail | 2×(300/300) |
| `err_t2` | T2 gas inlet fail | 3×(300/300) |
| `err_t3` | T3 chassis fail | 4×(300/300) |
| `err_t4` | T4 drain fail | 5×(300/300) |

Interval (500 ms) volá `set_pattern(id, bool)` pro všech pět kódů — engine si
sám drží, které patterny jsou aktivní a přehrává je v kole.

**WD LED (GPIO4)** — `led_sequencer` instance `wd_led_seq`, čtyři vzájemně se
vylučující `continuous` stavy (jen jeden aktivní, priorita shora):

| Stav | Podmínka | Pattern |
|---|---|---|
| `wd_disabled` | MAIN_SWITCH off | 100/900 continuous |
| `wd_defrost` | kterýkoli heater ON | 100/100 continuous |
| `wd_armed` | HEAT_MODE on, žádný heater | 2×(150/150)+900 off continuous |
| `wd_idle` | enabled, HEAT_MODE off | 500/500 continuous |

Výběrová lambda (500 ms interval): `!main → disabled; else heater_on → defrost;
else heat_mode → armed; else idle`.

**HA-facing** (názvy dle ADR-005 execution, S14):
- `bs_err_wifi_lost` ("Porucha WiFi", skupina Globální), `bs_err_t1..t4`
  ("Porucha čidla T1-T4 ...", skupina Servisní) — jednotlivé binary_sensory
  (device_class: problem)
- `bs_any_error` ("Aktivní porucha", Globální) — souhrnný OR všech pěti
- `sensor_error_status` ("Kód chyby", Globální, text_sensor) — čitelný
  comma-separated seznam aktivních chyb ("OK" když žádná); event-driven refresh
  (OI13, §3.2 vzor) místo dřívějšího plochého 60s pollu
- `sensor_system_state` ("Stav systému", Globální, text_sensor, nová entita
  ADR-005 bod 3) — zrcadlí stejnou WD selector lambdu jako čtyři stavy výše,
  jako čitelný text: `OFF` / `IDLE` / `ARMED` / `Heater ON` (hodnoty anglicky,
  konzistentně s Garage_Windows precedentem — CZ jen v `name:`, ne ve
  vypsaných stavech). **BUG-007:** původně navázáno na 500ms WD LED interval —
  `text_sensor::publish_state()` nededupuje (na rozdíl od `binary_sensor`), takže
  to re-publikovalo/logovalo 2×/s napořád. Opraveno na event-driven refresh:
  `sw_main_system_enable` turn_on/turn_off_action, `sw_heater_chassis`/
  `sw_heater_drain` on_turn_on/on_turn_off, `bs_heat_mode` on_state:, plus
  `on_boot:` safety net.

---

## 6. Konfigurace (HA-editable `number` entity)

Všechny persistují přes `restore_value: true`.

| Entita | Default | Rozsah | Účel |
|---|---|---|---|
| `heat_on_th` | 2.0 °C | −20 až 10 | HEAT_MODE zapnutí |
| `heat_off_th` | 4.0 °C | −10 až 15 | HEAT_MODE vypnutí |
| `def_abs_th` | 5.0 °C | −10 až 30 | Defrost absolutní práh (gas inlet) |
| `def_dt_th` | 7.0 °C | 0 až 20 | Defrost delta práh (gas inlet − outside) |
| `chassis_time_floor` | 2 min | 1–10 | Garantovaná min. doba běhu chassis heateru (ADR-008) |
| `drain_time_floor` | 3 min | 1–10 | Garantovaná min. doba běhu drain heateru (ADR-008) |
| `chassis_time_ceiling` | 20 min | 15–60 | Max. doba běhu chassis heateru (bezpečnostní strop, ADR-007/008) |
| `drain_time_ceiling` | 20 min | 15–60 | Max. doba běhu drain heateru (bezpečnostní strop, ADR-007/008) |
| `sim_t_outside/gas_inlet/chassis/drain` | 10/5/5/5 °C | dle senzoru | Manuální hodnoty pro simulační režim |

---

## 7. Známý technický dluh (vstup pro BACKLOG)

Toto jsou položky viditelné přímo z YAML — code review pravděpodobně přidá další:

1. **DI1/DI2 jsou "Reserved"** — nevyužité vstupy, jen logují press/release, bez funkce.
2. **DO3/DO4 jsou "Reserved"** — nevyužité výstupy (interní, skryté z HA).

---

## 8. HA Entity Exposure & Device Grouping (ADR-005, S14-S16)

ADR-005 execution (S14, 2026-08-01): CZ friendly names + `esphome: devices:`
grouping, šablona `Garage_Windows` (tam Globální + per-fyzická-jednotka Okno
1/Okno 2 — tady žádná druhá fyzická jednotka není, takže funkční rozdělení).

**Živý zdroj pravdy pro entity-by-entity mapu (id/`device_class`/`icon`/
grouping/dashboard Úroveň):** `ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md`
(S16, ADR-015 — standardní entity-exposure formát, autoritativní instance v
tomto repu). Pracovní draft `#Archive/ADR-005_execution_draft.md` zůstává
jako historický kontext rozhodování (Round 1 rename+grouping, Round 2 ikony),
ne jako primární odkaz.

**Skupiny (`esphome: devices:`):**

| `device_id` | HA název | Účel |
|---|---|---|
| `globalni` | Globální | Celý systém — hlavní vypínač, WiFi/souhrnná porucha, "Kód chyby", "Stav systému", uptime |
| `provoz` | Provoz | Živý provozní stav — HEAT_MODE/DEFROST_ORDERED, `_used` teploty, oba heatery |
| `nastaveni` | Nastavení | Uživatelem upravitelné prahy a časy (8× `number`) |
| `servisni` | Servisní | Diagnostika — raw čidla, Simulation Mode + 4 sim entity, per-senzor error flagy |

**Škálování:** mění se pouze `name:` (→ odvozený HA `entity_id`) a nový
`device_id:` atribut. Interní YAML `id:` (lambda/action reference) beze
změny — zůstávají anglické, v HA neviditelné.

**Rozhodnutí (Lubor, 2026-08-01):**
- Raw i `_used` teplotní senzory zůstávají obě exponované (raw = Servisní/
  diagnostika, used = Provoz/skutečně používaná hodnota) — v reálném provozu
  (Simulation Mode OFF) mají identickou hodnotu; zvažováno schovat raw úplně,
  zatím ponechány, lze vyřadit z dashboardu na HA straně později.
- Rezervované piny (DI1/DI2/DO3/DO4) zůstávají `internal: true`, neexponovány
  — nemají žádnou funkci.

**Ikony (Round 2, S15, 2026-08-01):** všech 35 exponovaných entit má
explicitní `icon: "mdi:..."` (draft: `#Archive/ADR-005_execution_draft.md`,
sekce "Round 2"). Poznámka k `device_class:` — v HA řídí nejen výchozí ikonu,
ale i **text zobrazovaného stavu** (sémantický, ne obecné Zapnuto/Vypnuto).
`bs_heat_mode` mělo `device_class: cold`, což HA zobrazovalo jako "Chladno" —
Lubor si toho všiml (BUG-podobný nález, ne funkční regrese, jen matoucí
prezentace), `device_class` odebrán, nahrazen `icon: "mdi:snowflake-alert"`.
Pět error binary_sensorů (`device_class: problem` → HA "Problem"/"OK")
ponecháno beze změny — sémanticky sedí.

---

## 9. Verzování dokumentu

| Verze | Datum | Změna |
|---|---|---|
| 1.0 | 2026-07-10 | První verze. Snapshot stavu firmwaru z ChatGPT/breadboard éry, zdroj: YAML review před formálním code review. |
| 1.1 | 2026-07-10 | Oprava §2.2: GPIO4 mylně označen jako strapping pin, opraveno na jen GPIO2. Schváleno Architektem po konzistenční revizi doc seedu (S1). |
| 1.2 | 2026-07-21 | §4.4 přepsán dle ADR-006 (boot-resume varianta B) a ADR-007 (defrost condition-driven, supersedes OI3). S4 (Architekt), doc-commit S4. |
| 1.3 | 2026-07-21 | Konzistenční čištění po ADR-007: §7 bod "paralelní defrost bez sync konce" odstraněn (vyřešeno, viz §4.4), zbylé body přečíslovány; §6 tabulka `chassis_time_min`/`drain_time_min` popis změněn z "doba běhu" na "max. doba (bezpečnostní strop)". |
| 1.4 | 2026-07-22 | §4.4 a §6 přepsány dle ADR-008 (floor + rename _time_min → _time_ceiling). S5 (Architekt). |
| 1.5 | 2026-07-28 | §3: reálné adresy T1 (`t_outside`)/T2 (`t_evap`) komisionovány na hotovém HW (test01 bring-up), TODO odstraněno pro obě. T3/T4 zůstávají placeholder. S5 (Implementer). |
| 1.6 | 2026-07-30 | §2.3 (Testovací blok) odstraněna — VL53L0X, BME280 a I2C bus kompletně vyřazeny z YAML (OI2, na žádost Lubora během bench testu S5). §7 bod 2 odstraněn (vyřešeno), zbylé body přečíslovány. S5 (Implementer). |
| 1.7 | 2026-07-30 | §4.3 přepsán dle ADR-009 — DEFROST_ORDERED nově gated na HEAT_MODE (3. podmínka), OI6 vyřešeno. S7 (Implementer, doc-commit ADR-009 handoff). |
| 1.8 | 2026-07-31 | §3/§3.1/§3.2/§4.3/§6: `t_evap`→`t_gas_inlet` (ADR-011, reverse-engineering Toshiba Shorai Edge servisního manuálu — starý název implikoval fyzicky nesprávnou/pozdní pozici čidla), přidán popis fyzického umístění T2/T3. Zároveň opravena stará chybná poznámka u §3.1 (1s/4s intervaly, BUG-003 — viz S5 2026-07-30). S9 (Implementer, doc-commit ADR-011 handoff). |
| 1.9 | 2026-07-31 | §2.2/§5 přepsány dle ADR-004 — `led_sequencer` portován (OI10), nahradil ruční `show_error_code` script a WD 1s heartbeat interval; `err_t1_t2_fail`/`err_t3_t4_fail` rozděleny na per-senzor `err_t1..err_t4` čtoucí `_used` vrstvu (OI11 vyřešeno). S10 (Implementer). |
| 1.10 | 2026-07-31 | §3/§3.1 přepsány dle BUG-006 — median filtr zakomentován na všech 4 DS18B20 senzorech (maskoval selhání senzoru až 10-20 min), základní `update_interval` 120s→20s, rychlý tier sjednocen na 5s (T3/T4 z dřívějších 20s). S10 (Implementer). |
| 1.11 | 2026-08-01 | §3/§7: reálné adresy T3 (`t_chassis`) = `0xa90625910004ba28`, T4 (`t_drain`) = `0xb6062591abac6f28` komisionovány na hotovém HW (test01 bring-up), TODO/placeholder odstraněno pro obě — OI1 kompletně vyřešeno. S11 (Implementer). |
| 1.12 | 2026-08-01 | §3.2 přepsáno dle OI17 — `_used` senzory `update_interval: never`, refresh čistě event-driven (`on_value:` na raw senzorech/sim number entitách + `on_boot:` safety net), nahradilo plošné `update_interval: 5s`. §4.1 tabulka globálů opravena (`err_t1_t2_fail`/`err_t3_t4_fail` byly stale od ADR-004/OI11 splitu, teď `err_t1..t4`). §5 tabulka opravena dle OI4 (T3/T4 error-check 60s→20s). S12 (Implementer). |
| 1.13 | 2026-08-01 | §3.1 doplněna poznámka o zváženém a zamítnutém nápadu (fázový posun mezi senzory na sdíleném 1-Wire, "torn read" riziko v DEFROST_ORDERED — zamítnuto, tepelná setrvačnost dominuje). S12 (Implementer). |
| 1.14 | 2026-08-01 | **ADR-005 execution** — nová §8 (HA Entity Exposure & Device Grouping), §3/§3.2/§5 přepsány na CZ `name:` a `device_id:` grouping (Globální/Provoz/Nastavení/Servisní), nová entita `sensor_system_state` ("Stav systému"). §6 (`number` tabulka) beze změny — sloupec uvádí interní `id:`, ne `name:`, ty se nemění. S14 (Implementer). |
| 1.15 | 2026-08-01 | §5: BUG-007 — `sensor_system_state` refresh mechanismus opraven z 500ms interval (log spam) na event-driven (viz BUGS.md). S14 (Implementer). |
| 1.16 | 2026-08-01 | §8 doplněno o ADR-005 Round 2 (ikony) — všech 35 entit má `icon:`, `bs_heat_mode` ztratilo `device_class: cold` (matoucí "Chladno" stav), nahrazeno explicitní ikonou. S15 (Implementer). |
| 1.17 | 2026-08-01 | Hlavičkové "Aktuální stav"/"Phase" opraveny ze stale "Phase 1 — Doc-sync & Code Review, ChatGPT/pre-review éra" na aktuální stav (bench-validováno, HA napárováno, Phase 4 — Field Deployment dle `ROADMAP.md` v1.2). Nalezeno při dokumentačním křížovém úklidu (S15, Implementer). |
| 1.18 | 2026-08-01 | §8: primární odkaz na entity-by-entity mapu přepnut z archivovaného draftu na živý `ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md` (S16, ADR-015) — draft zůstává jen jako historický kontext. Nalezeno v pre-field-deployment dokumentačním průchodu (S16, Implementer). |
