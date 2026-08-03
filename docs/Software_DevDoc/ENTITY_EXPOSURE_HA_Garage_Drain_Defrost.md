# ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md

> **Vlastník:** Garage_Drain_Defrost repo — toto je **autoritativní instance**
> (ADR-015 §1/§6). Ruční synced kopie do HA_RD_Jirny (worked example) dělá
> Lubor sám po review, mimo tuhle session.
> **Vznik:** S14/S15 (ADR-005 execution — rename + grouping S14, ikony S15),
> destilováno do standardního `ENTITY_EXPOSURE_HA` formátu v S16 na žádost
> HA Architekta (ADR-015).
> **Zdroj obsahu:** pracovní draft `#Archive/ADR-005_execution_draft.md`
> (Round 1 + Round 2 — old→new názvy, grouping návrh) + `firmware/yaml/
> ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` (ground truth pro `id`/`device_class`/
> `icon`/`name`).
> **Šablona:** `ENTITY_EXPOSURE_HA_Garage_Windows.md` (repo HA_RD_Jirny,
> READ-ONLY) — struktura zrcadlena 1:1, s funkční osou místo fyzické (viz
> Sub-device struktura níže).

**Session původu:** S14/S15 (Implementer/CC) — execution · S16 (Implementer/CC)
— destilace do tohoto formátu, review Luborem · **Datum:** 2026-08-01 ·
**Status:** Sub-devices finalizovány, friendly names + grouping + ikony +
Úroveň + pořadí řádků hotové, bench potvrzené a Luborem zkontrolované
(TEST_PLAN.md Fáze 10) · **Icon pass:** dokončen S15
**Zdroj:** `ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` (produkční firmware)

> Účel: rozhodnout (1) co transportovat do HA, (2) co zobrazit v základním vs.
> detailním zobrazení (helper-switched). Pořadí shora dolů = navrhovaná
> důležitost = pořadí karet v Sections view.
>
> Sloupce Úroveň a pořadí řádků navrhl Implementer heuristikou, Lubor je
> zkontroloval v S16 review (upraveno pořadí v §2/§3, zbytek potvrzen beze
> změny).

## Legenda sloupců

- **R/W** — směr vůči HA, daný typem entity (ne flag): `W` = HA smí zapisovat /
  ovládací prvek (`number`, `switch`), `R` = read-only / displej (`sensor`,
  `binary_sensor`, `text_sensor`).
- **Do HA?** — transport entity do HA vůbec (osa 1). `Ano` = nechat, `NE`
  = vyřadit, vyžaduje firmware edit (`internal: true`) → eskalace firmware
  Implementerovi.
- **Úroveň** — viditelnost v dashboardu (osa 2): `Základní` / `Detailní` / `—`
  (netransportováno nebo bench-only). Implementer heuristika: primární
  provozní stav a hlavní ovládání = Základní; diagnostika, raw čidla, sim
  entity, per-senzor error-detail, konfigurační prahy = Detailní;
  bench-only/vyloučené = `—`. Zkontrolováno a potvrzeno Luborem v S16 review.
- **Ikona (mdi:)** — `icon: mdi:...` v ESPHome yaml. Doplněno S15 (ADR-005
  Round 2), hotovo pro všechny exponované entity.

> Pořadí řádků v tabulkách shora dolů představuje důležitost pro dashboard
> (Windows konvence, Sections view pořadí karet). Implementer navrhl pořadí
> vlastním úsudkem, Lubor ho v S16 review upravil v §2 (Nastavení) a §3
> (Servisní) — zbytek potvrzen beze změny.

---

## Sub-device struktura

```
Garage Drain Defrost (node — bez entit, jen device karta)
├── Provoz      — HEAT_MODE/DEFROST_ORDERED, provozní teploty, oba heatery
├── Nastavení   — uživatelem upravitelné prahy a časovače (8× number)
├── Servisní    — raw čidla, Simulation Mode + sim entity, per-senzor error flagy
└── Globální    — hlavní vypínač, souhrnné chyby, stav systému, uptime
```

---

## 1. Sub-device: Provoz

| # | Funkce | Entita (id) | Friendly name old | Friendly name new | R/W | Do HA? | Úroveň | Ikona (mdi:) |
|---|---|---|---|---|---|---|---|---|
| 1 | Stav mrazového režimu | `bs_heat_mode` | HEAT_MODE | Protimrazový režim | R | Ano | Základní | `snowflake-alert` |
| 2 | Stav odmrazování | `bs_defrost` | DEFROST_ORDERED | Odmrazení aktivní | R | Ano | Základní | `snowflake-melt` |
| 3 | Topení šasi (ovládání) | `sw_heater_chassis` | Heater Chassis | Topení Chassis | W | Ano | Základní | `heating-coil` |
| 4 | Topení odtoku (ovládání) | `sw_heater_drain` | Heater Drain | Topení odpadu | W | Ano | Základní | `radiator` |
| 5 | Venkovní teplota (provozní) | `t_outside_used` | Outside Temperature (used) | Venkovní teplota | R | Ano | Základní | `thermometer` |
| 6 | Teplota výměníku (provozní) | `t_gas_inlet_used` | Gas Inlet Temperature (used) | Teplota výměníku | R | Ano | Základní | `thermometer` |
| 7 | Teplota šasi (provozní) | `t_chassis_used` | Chassis Temperature (used) | Teplota Chassis | R | Ano | Základní | `thermometer` |
| 8 | Teplota odtoku (provozní) | `t_drain_used` | Drain Pipe Temperature (used) | Teplota odpadu | R | Ano | Základní | `thermometer` |

---

## 2. Sub-device: Nastavení

| # | Funkce | Entita (id) | Friendly name old | Friendly name new | R/W | Do HA? | Úroveň | Ikona (mdi:) |
|---|---|---|---|---|---|---|---|---|
| 1 | Práh zapnutí mrazového režimu | `heat_on_th` | Heat Enable Threshold (°C) | Zapnout protimrazový režim při (°C) | W | Ano | Detailní | `thermometer-chevron-up` |
| 2 | Práh vypnutí mrazového režimu | `heat_off_th` | Heat Disable Threshold (°C) | Vypnout protimrazový režim při (°C) | W | Ano | Detailní | `thermometer-chevron-down` |
| 3 | Absolutní práh odmrazování | `def_abs_th` | Defrost Evap Abs Threshold (°C) | Práh teploty výměníku (°C) | W | Ano | Detailní | `thermometer-alert` |
| 4 | Delta práh odmrazování | `def_dt_th` | Defrost Delta Threshold (°C) | Práh rozdílu teplot výměník - venkovní (°C) | W | Ano | Detailní | `delta` |
| 5 | Šasi: garantovaná min. doba | `chassis_time_floor` | Chassis Heater Min Time (min) | Chassis: min čas odmrazování (min) | W | Ano | Detailní | `timer-outline` |
| 6 | Šasi: bezpečnostní strop | `chassis_time_ceiling` | Chassis Heater Time (min) | Chassis: max čas odmrazování (min) | W | Ano | Detailní | `timer-alert-outline` |
| 7 | Odtok: garantovaná min. doba | `drain_time_floor` | Drain Heater Min Time (min) | Odpad: min čas odmrazování (min) | W | Ano | Detailní | `timer-outline` |
| 8 | Odtok: bezpečnostní strop | `drain_time_ceiling` | Drain Heater Time (min) | Odpad: max čas odmrazování (min) | W | Ano | Detailní | `timer-alert-outline` |

---

## 3. Sub-device: Servisní

| # | Funkce | Entita (id) | Friendly name old | Friendly name new | R/W | Do HA? | Úroveň | Ikona (mdi:) |
|---|---|---|---|---|---|---|---|---|
| 1 | Přepínač simulačního režimu | `sim_mode` | Simulation Mode | Simulační režim | W | Ano | Detailní | `flask-outline` |
| 2 | Sim. venkovní teplota | `sim_t_outside` | Sim Outside (°C) | Simulovaná venkovní teplota (°C) | W | Ano | Detailní | `thermometer-outline` |
| 3 | Sim. teplota výměníku | `sim_t_gas_inlet` | Sim Gas Inlet (°C) | Simulovaná teplota výměník (°C) | W | Ano | Detailní | `thermometer-outline` |
| 4 | Sim. teplota šasi | `sim_t_chassis` | Sim Chassis (°C) | Simulovaná teplota Chassis (°C) | W | Ano | Detailní | `thermometer-outline` |
| 5 | Sim. teplota odtoku | `sim_t_drain` | Sim Drain (°C) | Simulovaná teplota odpad (°C) | W | Ano | Detailní | `thermometer-outline` |
| 6 | Porucha čidla T1 | `bs_err_t1` | Error: T1 Sensor Fail (Outside) | Porucha čidla T1 (venkovní) | R | Ano | Detailní | `thermometer-alert` |
| 7 | Porucha čidla T2 | `bs_err_t2` | Error: T2 Sensor Fail (Gas Inlet) | Porucha čidla T2 (výměník) | R | Ano | Detailní | `thermometer-alert` |
| 8 | Porucha čidla T3 | `bs_err_t3` | Error: T3 Sensor Fail (Chassis) | Porucha čidla T3 (Chassis) | R | Ano | Detailní | `thermometer-alert` |
| 9 | Porucha čidla T4 | `bs_err_t4` | Error: T4 Sensor Fail (Drain) | Porucha čidla T4 (odpad) | R | Ano | Detailní | `thermometer-alert` |
| 10 | Venkovní teplota (raw čidlo) | `t_outside` | Outside Temperature | Venkovní teplota (čidlo) | R | Ano | Detailní | `thermometer-lines` |
| 11 | Teplota výměníku (raw čidlo) | `t_gas_inlet` | Gas Inlet Temperature | Teplota výměníku (čidlo) | R | Ano | Detailní | `thermometer-lines` |
| 12 | Teplota šasi (raw čidlo) | `t_chassis` | Chassis Temperature | Teplota Chassis (čidlo) | R | Ano | Detailní | `thermometer-lines` |
| 13 | Teplota odtoku (raw čidlo) | `t_drain` | Drain Pipe Temperature | Teplota odpadu (čidlo) | R | Ano | Detailní | `thermometer-lines` |

> Raw čidla (#10-13) jsou v reálném provozu (Simulation Mode OFF) hodnotově
> identická s Provoz #5-8 — Servisní kopie je diagnostická (raw vs. used
> odchylka je viditelná jen během Simulation Mode). Rozhodnuto ponechat obě
> exponované (S14, viz draft §3 otázka 2), ne skrýt raw z HA.

---

## 4. Sub-device: Globální

| # | Funkce | Entita (id) | Friendly name old | Friendly name new | R/W | Do HA? | Úroveň | Ikona (mdi:) |
|---|---|---|---|---|---|---|---|---|
| 1 | Hlavní vypínač systému | `sw_main_system_enable` | Main System Enable | Hlavní vypínač | W | Ano | Základní | `power` |
| 2 | Souhrnný stav systému | `sensor_system_state` | *(nová entita, ADR-005 bod 3)* | Stav systému | R | Ano | Základní | `led-on` |
| 3 | Seznam aktivních chyb | `sensor_error_status` | Error Status | Kód chyby | R | Ano | Základní | `alert-circle` |
| 4 | Souhrnný poruchový flag | `bs_any_error` | System Error Active | Aktivní porucha | R | Ano | Základní | `alert-circle-outline` |
| 5 | Stav WiFi spojení | `bs_err_wifi_lost` | Error: WiFi Lost | Porucha WiFi | R | Ano | Detailní | `wifi-alert` |
| 6 | Doba běhu ESP32 | `uptime_sensor` | Uptime (s) | Čas chodu ESP32 (s) | R | Ano | Detailní | `clock-outline` |

---

## 5. Node: Garage Drain Defrost (bez entit)

Hlavní ESPHome device karta. Obsahuje pouze integrační metadata: fw verze,
IP adresa, stav připojení. Žádné uživatelské entity.

---

## 6. Vyloučené z dashboardu

| Funkce | Entita (id) | Friendly name | R/W | Do HA? | Pozn. |
|---|---|---|---|---|---|
| Rezervovaný vstup | `di1_in` | DI1 (Reserved) | R | NE | GPIO36, bez funkce, jen log press/release (`internal: true`) |
| Rezervovaný vstup | `di2_in` | DI2 (Reserved) | R | NE | GPIO39, bez funkce, jen log press/release (`internal: true`) |
| Rezervovaný výstup | `sw_do3` | Aux DO3 | W | NE | GPIO25 (OC 5/12V), bez funkce (`internal: true`) |
| Rezervovaný výstup | `sw_do4` | Aux DO4 | W | NE | GPIO26 (OC 5/12V), bez funkce (`internal: true`) |

> Na rozdíl od Windows (lokální tlačítka DI4-DI7, SIM switche bench-only)
> tenhle projekt nemá žádné bench-only entity mimo dashboard — Simulation
> Mode + 4 sim entity jsou záměrně exponované ve Servisní skupině (§3), ne
> vyřazené, protože se plánuje jejich použití i po field deploymentu (ladění
> bez fyzického přístupu k jednotce). Vyloučené jsou tu jen skutečně
> nefunkční rezervované piny.

---

## 7. Rozpory v názvech (k eskalaci firmware Implementerovi)

| # | Problém | Návrh |
|---|---|---|
| 1 | `sim_mode` nemá `sw_` prefix jako ostatní switche (`sw_main_system_enable`, `sw_heater_chassis`, `sw_heater_drain`, `sw_do3`, `sw_do4`) | Sjednotit na `sw_sim_mode` |
| 2 | Text_sensor id konvence koliduje vizuálně se `sensor:` doménou — `sensor_error_status`/`sensor_system_state` (text_sensor) vs. `t_outside`/`uptime_sensor` (sensor) oba používají/naznačují "sensor" bez rozlišení domény | Zvážit `ts_error_status`/`ts_system_state` (analogie k Windows `w1_ERR10` konvenci), nebo ponechat — dopad jen na čitelnost YAML, ne na funkci |

> Obě položky jsou kosmetické (YAML `id:` je HA neviditelné, viz Rozsah v
> `#Archive/ADR-005_execution_draft.md`) — žádná nemění chování ani HA
> expozici. Poslat firmware Implementerovi k rozhodnutí, jestli stojí za
> samostatnou dávku, nebo zůstanou tak jak jsou.

---

## 8. Device_class audit — výsledky (S15)

### `bs_heat_mode` (Protimrazový režim) — `device_class: cold` odebrán

**Stav před S15:** `device_class: cold` nastaven kvůli výchozí ikoně (sněhová
vločka).

**Nalezený problém:** HA `device_class: cold` neřídí jen ikonu, ale i
sémantický text zobrazeného stavu — místo obecného Zapnuto/Vypnuto HA
zobrazovalo **"Chladno"**. Entita `bs_heat_mode` reprezentuje "je aktivní
protimrazový režim" (mód/stav systému), ne "je fyzicky chladno" — sémantický
text byl matoucí vzhledem k významu entity.

**Rozhodnutí S15:** `device_class: cold` odebrán, nahrazen explicitní
`icon: "mdi:snowflake-alert"`. HA teď zobrazuje generické Zapnuto/Vypnuto.
Bench potvrzeno (TEST_PLAN.md Fáze 10).

> **Warning pro budoucí refaktoring:** stejné pravidlo jako Windows §7
> `end_stop_closed` — `device_class` se hodí jen tam, kde jeho vestavěná
> sémantika (on/off text) skutečně odpovídá významu entity. Než přiřadit
> `device_class` jen kvůli pohodlné výchozí ikoně, ověřit v HA UI, jaký text
> stavu to reálně vyprodukuje.

### `bs_err_wifi_lost`, `bs_err_t1..t4`, `bs_any_error` — `device_class: problem` ponecháno

Pět binary_sensorů s `device_class: problem` (HA zobrazuje "Problem"/"OK")
— sémanticky sedí na "je/není porucha", **beze změny, audit uzavřen bez
akce.**

### `sensor_error_status`, `sensor_system_state` — bez `device_class` (text_sensor)

ESPHome `text_sensor` nepodporuje `device_class` (stejná situace jako Windows
§7 `w1_ERR10`). Nic k řešení.

---

## 9. Změny oproti pracovnímu draftu

| Změna | Detail |
|---|---|
| Formát → `ENTITY_EXPOSURE_HA` standard | Destilace z `#Archive/ADR-005_execution_draft.md` (2 kola, chronologický pracovní formát) do sub-device tabulek dle Windows šablony (ADR-015) |
| Osa grouping | Funkční (Provoz/Nastavení/Servisní/Globální), ne fyzická jako Windows (Okno 1/Okno 2/Globální) — tenhle projekt nemá víc fyzických jednotek |
| Sloupce Úroveň + pořadí řádků | Nové oproti draftu (draft je neměl) — Implementer heuristika, zkontrolováno a upraveno Luborem v S16 review (pořadí v §2/§3) |
| `id`/`device_class`/`icon` | Převzato z YAML (ground truth) k datu S16, ne z draftu — draft je pracovní historie rozhodování, ne finální zdroj |
| Rozpory v názvech, Device_class audit | Nové sekce oproti draftu (draft měl jen "Otevřené otázky", jinak strukturované) |

---

*Založeno: S16, 2026-08-01 (Implementer/CC), na žádost HA Architekta (ADR-015).*
