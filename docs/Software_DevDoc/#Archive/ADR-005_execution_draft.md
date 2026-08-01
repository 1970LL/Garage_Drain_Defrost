# ADR-005 Execution — Draft pro schválení (entity rename + device grouping)

> **Účel:** Pracovní návrh k projetí položku po položce, ne finální dokument.
> Po schválení/úpravách promítnu do YAML a smažu/archivuju tenhle soubor.
> **Šablona:** `Garage_Windows/firmware/yaml/ESP32-D0WD-V3_Gar_Windows_02.yaml`
> (device grouping syntax, CZ názvosloví styl — "Kód chyby", "Stav X", "Hlavní
> vypínač", "Porucha WiFi", "Čas chodu ESP32 (s)" apod.)

---

## Rozsah — co se mění a co ne

- **Mění se:** `name:` (CZ friendly name) → tím se automaticky změní i odvozený
  HA `entity_id` (ESPHome ho odvozuje z `name:`, ne z YAML `id:`). Nulová cena
  téhle změny teď, dokud HA neběží — žádné dashboardy/automatizace na starých
  entity_id zatím nezávisí.
- **Přidává se:** `esphome: devices:` blok (4 skupiny) + `device_id:` na
  každé entitě.
- **Nová entita:** text_sensor "Stav systému" (ADR-005 bod 3) — dosud
  neexistuje, jen zrcadlila WD LED selector lambdu bez vlastní HA expozice.
  Potřebuje novou C++ lambdu (viz níže).
- **NEmění se:** YAML `id:` (interní reference v lambdách/akcích) — přejmenovat
  i tyhle by bylo čistě kosmetické riziko (překlep ve ~40 odkazech) bez
  uživatelského přínosu. Necháváme anglické, neviditelné z HA.

---

## 1. Device grouping — návrh

Windows šablona: Globální + per-fyzická-jednotka (Okno 1/Okno 2). Tady není
víc fyzických jednotek, takže navrhuju funkční rozdělení (tvůj návrh
Provoz/Nastavení/Servisní):

| `id` | `name` | Účel |
|---|---|---|
| `globalni` | Globální | Celý systém, nevázáno na konkrétní subsystém (hlavní vypínač, WiFi chyba, souhrnná porucha, uptime, stav systému) |
| `provoz` | Provoz | Živý provozní stav — co sleduješ běžně (teploty v aktivním použití, HEAT_MODE, DEFROST_ORDERED, oba heatery) |
| `nastaveni` | Nastavení | Uživatelem upravitelné prahy a časy (8× `number`) |
| `servisni` | Servisní | Diagnostika, bench/simulace nástroje (raw čidla, Simulation Mode + 4 sim entity, per-senzor error flagy, rezervní I/O) |

**Otázka k rozhodnutí:** souhlasíš s tímhle funkčním dělením, nebo bys některé
položky přesunul jinam (viz tabulky níže, sloupec Skupina — uprav přímo)?

---

## 2. Entity crosscheck — projdi řádek po řádku

Formát: **Současný název (EN)** | `id` (nezměněno) | **Návrh CZ název** |
Skupina | Exponovat do HA? | Poznámka

### Globální

| Současný název | `id` | Návrh CZ název | Skupina | Exponovat | Poznámka |
|---|---|---|---|---|---|
| Main System Enable | `sw_main_system_enable` | Hlavní vypínač | globalni | ano | dle Windows přesně |
| Error: WiFi Lost | `bs_err_wifi_lost` | Porucha WiFi | globalni | ano | dle Windows přesně |
| System Error Active | `bs_any_error` | Aktivní porucha | globalni | ano | |
| Error Status | `sensor_error_status` | Kód chyby | globalni | ano | dle Windows přesně (ADR-005 bod 3) |
| Uptime (s) | `uptime_sensor` | Čas chodu ESP32 (s) | globalni | ano | dle Windows přesně |
| *(nová entita)* | `sensor_system_state` (nový id) | Stav systému | globalni | ano | **ADR-005 bod 3** — text_sensor, zrcadlí WD selector lambdu: `OFF` (main off) / `IDLE` (idle) / `ARMED` (armed) / `Heater ON` (heater on). Potřebuje novou C++ lambdu, viz §3. |

### Provoz

| Současný název | `id` | Návrh CZ název | Skupina | Exponovat | Poznámka |
|---|---|---|---|---|---|
| HEAT_MODE | `bs_heat_mode` | Protimrazový režim | provoz | ano | |
| DEFROST_ORDERED | `bs_defrost` | Odmrazení aktivní | provoz | ano | |
| Outside Temperature (used) | `t_outside_used` | Venkovní teplota | provoz | ano | "(used)" odpadá z názvu — je to *ta* provozní hodnota |
| Gas Inlet Temperature (used) | `t_gas_inlet_used` | Teplota výměníku | provoz | ano | |
| Chassis Temperature (used) | `t_chassis_used` | Teplota Chassis | provoz | ano | |
| Drain Pipe Temperature (used) | `t_drain_used` | Teplota odpadu | provoz | ano | |
| Heater Chassis | `sw_heater_chassis` | Topení Chassis | provoz | ano | |
| Heater Drain | `sw_heater_drain` | Topení odpadu | provoz | ano | |

### Nastavení

| Současný název | `id` | Návrh CZ název | Skupina | Exponovat | Poznámka |
|---|---|---|---|---|---|
| Heat Enable Threshold (°C) | `heat_on_th` | Zapnout protimrazový režim při (°C) | nastaveni | ano | |
| Heat Disable Threshold (°C) | `heat_off_th` | Vypnout protimrazový režim při (°C) | nastaveni | ano | |
| Defrost Evap Abs Threshold (°C) | `def_abs_th` | Práh teploty výměníku (°C) | nastaveni | ano | |
| Defrost Delta Threshold (°C) | `def_dt_th` | Práh rozdílu teplot výměník - venkovní (°C) | nastaveni | ano | |
| Chassis Heater Min Time (min) | `chassis_time_floor` | Chassis: min čas odmrazování (min) | nastaveni | ano | |
| Drain Heater Min Time (min) | `drain_time_floor` | Odpad: min čas odmrazování (min) | nastaveni | ano | |
| Chassis Heater Time (min) | `chassis_time_ceiling` | Chassis: max čas odmrazování (min) | nastaveni | ano | |
| Drain Heater Time (min) | `drain_time_ceiling` | Odpad: max čas odmrazování (min) | nastaveni | ano | |

### Servisní

| Současný název | `id` | Návrh CZ název | Skupina | Exponovat | Poznámka |
|---|---|---|---|---|---|
| Outside Temperature | `t_outside` | Venkovní teplota (čidlo) | servisni | ano | raw hodnota — redundantní s "Venkovní teplota" v Provoz mimo Sim mode, viz otázka níže |
| Gas Inlet Temperature | `t_gas_inlet` | Teplota výměníku (čidlo) | servisni | ano | |
| Chassis Temperature | `t_chassis` | Teplota Chassis (čidlo) | servisni | ano | |
| Drain Pipe Temperature | `t_drain` | Teplota odpadu (čidlo) | servisni | ano | |
| Simulation Mode | `sim_mode` | Simulační režim | servisni | ano | |
| Sim Outside (°C) | `sim_t_outside` | Simulovaná venkovní teplota (°C) | servisni | ano | |
| Sim Gas Inlet (°C) | `sim_t_gas_inlet` | Simulovaná teplota výměník (°C) | servisni | ano | |
| Sim Chassis (°C) | `sim_t_chassis` | Simulovaná teplota Chassis (°C) | servisni | ano | |
| Sim Drain (°C) | `sim_t_drain` | Simulovaná teplota odpad (°C) | servisni | ano | |
| Error: T1 Sensor Fail (Outside) | `bs_err_t1` | Porucha čidla T1 (venkovní) | servisni | ano | souhrn už je v "Kód chyby" (Globální) — tohle je detail pro dohledání KTERÉ čidlo |
| Error: T2 Sensor Fail (Gas Inlet) | `bs_err_t2` | Porucha čidla T2 (výměnik) | servisni | ano | |
| Error: T3 Sensor Fail (Chassis) | `bs_err_t3` | Porucha čidla T3 (Chassis) | servisni | ano | |
| Error: T4 Sensor Fail (Drain) | `bs_err_t4` | Porucha čidla T4 (odpad) | servisni | ano | |

### Rezervované (nevyužité piny)

| Současný název | `id` | Návrh | Poznámka |
|---|---|---|---|
| DI1 (Reserved) | `di1_in` | beze změny, `internal: true` | neexponovat |
| DI2 (Reserved) | `di2_in` | beze změny, `internal: true` | " |
| Aux DO3 | `sw_do3` | beze změny, `internal: true` | " |
| Aux DO4 | `sw_do4` | beze změny, `internal: true` | " |


---

## 3. Otevřené otázky (potřebuju tvoje rozhodnutí)

1. **Device grouping (§1)** — souhlas s Globální/Provoz/Nastavení/Servisní
   rozdělením, nebo přesunout některé položky? - souhlas
2. **Raw vs. (used) teploty** — obě zůstávají exponované (raw v Servisní,
   used v Provoz)? V reálném provozu (Simulation Mode OFF) mají identickou
   hodnotu — je ti to OK jako "raw = diagnostika, used = to co se skutečně
   používá", nebo chceš raw sensory nakonec z HA úplně schovat? - exponovat, kdyžtak vyhodím z dashboardu
3. **Rezervované piny (DI1/DI2/DO3/DO4)** — exponovat i tyhle, i když nemají
   funkci? - neexponovat
4. **"Stav systému" text_sensor** — potvrzuješ přesně tyhle čtyři stavové
   texty: `OFF` / `IDLE` / `ARMED` / `Heater ON` (dle ADR-005 bod 3
   a existující WD LED selector logiky)? - anglické názvy - konzistentně s Garage Windows
5. **CZ názvy samotné** — uprav/oprav cokoliv v tabulkách výše přímo v tomhle
   souboru, ať to mám v jednom místě k realizaci. - opraveno přímo v dokumentu

Až budou body 1-5 vyřešené, promítnu vše do YAML (`esphome: devices:` blok,
`device_id:` na každé entitě, `name:` rename, nová "Stav systému" lambda),
zkompiluju, aktualizuju `ARCHITECTURE.md` a `DECISIONS.md` (poznámka o
provedené exekuci k ADR-005), a smažu tenhle draft soubor. - tento soubor nemaž, přesuň do Archive

---
---

## Round 2 (S15, 2026-08-01) — Ikony entit (`icon:`)

> Round 1 (výše) je hotová a zrealizovaná (S14) — tabulky nechávám beze změny
> jako záznam. Tohle je nové kolo: žádná entita zatím nemá `icon:` nastavené,
> všechny běží na HA/ESPHome default (odvozeno z domény / `device_class`, kde
> je nastaven). Stejný postup — projeď a uprav/oprav přímo v tabulce.

**Poznámka k entitám s `device_class:`** — `device_class` v HA neřídí jen
výchozí ikonu, ale i **text zobrazovaného stavu** (sémantický, ne obecné
Zapnuto/Vypnuto). `bs_heat_mode` (`cold`) tímhle způsobem zobrazoval "Chladno"
— Lubor si toho všiml, `device_class: cold` odebrán, nahrazeno explicitní
ikonou (viz tabulka Provoz níže). Pět error binary_sensorů má `device_class:
problem` → HA zobrazuje "Problem"/"OK", což sémanticky sedí na "je porucha/
není porucha", takže **ponecháno beze změny** — flag, kdyby se i tohle chtělo
přehodnotit později.

### Globální

| Entita (CZ název) | `id` | Návrh ikona (`mdi:`) | Poznámka |
|---|---|---|---|
| Hlavní vypínač | `sw_main_system_enable` | `mdi:power` | |
| Porucha WiFi | `bs_err_wifi_lost` | `mdi:wifi-alert` | dle Windows přesně |
| Aktivní porucha | `bs_any_error` | `mdi:alert-circle-outline` | dle Windows (`Čítač chyb I2C`/`Stav I2C`) |
| Kód chyby | `sensor_error_status` | `mdi:alert-circle` | |
| Čas chodu ESP32 (s) | `uptime_sensor` | `mdi:clock-outline` | Windows tuhle entitu nemá ikonovanou, návrh nový |
| Stav systému | `sensor_system_state` |`mdi:led-on` | (zrcadlí WD LED) |

### Provoz

| Entita (CZ název) | `id` | Návrh ikona (`mdi:`) | Poznámka |
|---|---|---|---|
| Protimrazový režim | `bs_heat_mode` | ✅ `mdi:snowflake-alert` | **Vyřešeno (S15):** `device_class: cold` odebrán — způsoboval, že HA místo Zapnuto/Vypnuto zobrazoval sémantický stav "Chladno" (device_class řídí i text stavu, ne jen ikonu). Nahrazeno explicitní ikonou. |
| Odmrazení aktivní | `bs_defrost` | `mdi:snowflake-melt` | |
| Venkovní teplota | `t_outside_used` | `mdi:thermometer` | HA numeric+°C obvykle auto-ikonuje teploměrem i bez tohohle |
| Teplota výměníku | `t_gas_inlet_used` | `mdi:thermometer` | |
| Teplota Chassis | `t_chassis_used` | `mdi:thermometer` | |
| Teplota odpadu | `t_drain_used` | `mdi:thermometer` | |
| Topení Chassis | `sw_heater_chassis` | `mdi:heating-coil` | |
| Topení odpadu | `sw_heater_drain` | `mdi:radiator` | |

### Nastavení

| Entita (CZ název) | `id` | Návrh ikona (`mdi:`) | Poznámka |
|---|---|---|---|
| Zapnout protimrazový režim při (°C) | `heat_on_th` | `mdi:thermometer-chevron-up` | |
| Vypnout protimrazový režim při (°C) | `heat_off_th` | `mdi:thermometer-chevron-down` | |
| Práh teploty výměníku (°C) | `def_abs_th` | `mdi:thermometer-alert` | |
| Práh rozdílu teplot výměník - venkovní (°C) | `def_dt_th` | `mdi:delta` | MDI má přímo "delta" ikonu, sedí na rozdílový práh |
| Chassis: min čas odmrazování (min) | `chassis_time_floor` | `mdi:timer-outline` | |
| Odpad: min čas odmrazování (min) | `drain_time_floor` | `mdi:timer-outline` | |
| Chassis: max čas odmrazování (min) | `chassis_time_ceiling` | `mdi:timer-alert-outline` | odlišeno od floor (bezpečnostní strop) |
| Odpad: max čas odmrazování (min) | `drain_time_ceiling` | `mdi:timer-alert-outline` | |

### Servisní

| Entita (CZ název) | `id` | Návrh ikona (`mdi:`) | Poznámka |
|---|---|---|---|
| Venkovní teplota (čidlo) | `t_outside` | `mdi:thermometer-lines` | odlišná varianta od Provoz (`mdi:thermometer`), ať jde na první pohled poznat raw vs. used |
| Teplota výměníku (čidlo) | `t_gas_inlet` | `mdi:thermometer-lines` | |
| Teplota Chassis (čidlo) | `t_chassis` | `mdi:thermometer-lines` | |
| Teplota odpadu (čidlo) | `t_drain` | `mdi:thermometer-lines` | |
| Simulační režim | `sim_mode` | `mdi:flask-outline` | |
| Simulovaná venkovní teplota (°C) | `sim_t_outside` | `mdi:thermometer-outline` | |
| Simulovaná teplota výměník (°C) | `sim_t_gas_inlet` | `mdi:thermometer-outline` | |
| Simulovaná teplota Chassis (°C) | `sim_t_chassis` | `mdi:thermometer-outline` | |
| Simulovaná teplota odpad (°C) | `sim_t_drain` | `mdi:thermometer-outline` | |
| Porucha čidla T1 (venkovní) | `bs_err_t1` | *(ponechat default)* | `device_class: problem` |
| Porucha čidla T2 (výměník) | `bs_err_t2` | *(ponechat default)* | |
| Porucha čidla T3 (Chassis) | `bs_err_t3` | *(ponechat default)* | |
| Porucha čidla T4 (odpad) | `bs_err_t4` | *(ponechat default)* | |

**Otázky k rozhodnutí:**
1. Souhlas s návrhy výše, nebo uprav/oprav přímo v tabulkách? - drobné úpravy přímo v tabulkách
2. Servisní teploty — návrh odlišuje raw (`mdi:thermometer-lines`) od used
   (`mdi:thermometer`) čistě vizuálně v HA. Chceš tohle rozlišení, nebo
   sjednotit na stejnou ikonu všude? - ano rozlišit
3. Entity s `device_class:` (Protimrazový režim, 4× Porucha čidla) — nechat na
   defaultu, nebo přesto chceš vlastní ikonu? - protimrazový režim - výše v tabulce, porucha čidla `mdi:thermometer-alert`

Až schválíš/upravíš, promítnu `icon:` do YAML, zkompiluju a smažu/archivuju
tenhle soubor znovu.
