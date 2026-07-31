# Code Review — 2026-07-10 (S3, Implementer/CC)

> **Rozsah:** `firmware/yaml/ESP32-D0WD-V3_Gar_Drain_Defrost.yaml` — kompletní review kódu
> zděděného z ChatGPT/breadboard éry, před zahájením implementace pod Claude workflow.
> **Zaměření:** funkčnost a bezpečnost systému (dle zadání Architekta/Lubora).
> **Metoda:** line-by-line review + `esphome config` validace (vč. `--show-secrets` pro
> ověření jednoho podezřelého nálezu, hodnoty níže nejsou nikde citovány doslovně).
> **Vztah k S2:** Nálezy OI1–OI6 z `BACKLOG.md` a rozhodnutí ADR-004 (led_sequencer,
> err_t1..t4 split) se zde neopakují do detailu, jen se na ně odkazuje a doplňují se
> novým kontextem tam, kde review přinesl přesnější pochopení problému.

---

## Jak číst tento dokument

Každý nález má:
- **Závažnost:** High / Medium / Low
- **Kategorie:** 🔧 Implementer (můžu vyřešit sám) / 🏛️ Architekt (potřebuje rozhodnutí)
- **Důkaz** — co přesně v kódu/configu problém způsobuje
- **Doporučení**

Projdeme bod po bodu a společně to přetavíme na action items.

---

## A. Bezpečnostní nálezy (security)

### S1 — WiFi fallback AP je bez hesla (otevřený hotspot) 🏛️ Architekt/Lubor
**Závažnost:** Medium

Resolved config (`esphome config`) potvrzuje:
```yaml
wifi:
  ap:
    ssid: !secret 'fallback'
    ap_timeout: 90s
```
Žádný `password:` klíč. Když se ESP32 nepřipojí k domácí WiFi, po 90s spustí **otevřený,
nešifrovaný fallback AP** — kdokoli v dosahu se může připojit a dostat se na captive
portal / potenciálně k zařízení. Vyžaduje fyzickou blízkost, ale je to zero-effort exploit.

**Doporučení:** Přidat `password: !secret ap_password` (nový secret) k `wifi.ap`, nebo
vědomě potvrdit, že otevřený fallback je akceptovatelné riziko (řešení jen pro první
komisioning, pak se WiFi vždy připojí). Rozhoduje Lubor/Architekt, protože se dotýká
`secrets.yaml` a UX prvního zapojení — já to pak implementuju.

---

### S2 — Web server (port 80) bez autentizace ovládá topné výstupy 🏛️ Architekt/Lubor
**Závažnost:** Medium–High

```yaml
web_server:
  port: 80
  version: 2
  include_internal: false
```
`include_internal: false` skryje interní entity (Aux DO3/DO4), ale **"Heater Chassis"
a "Heater Drain" switche NEJSOU `internal: true`** — jsou plně viditelné a ovladatelné
z webového UI **bez jakéhokoli hesla**. Kdokoli na lokální síti (nebo zařízení, které
síť kompromituje) může přes prohlížeč zapnout/vypnout 230 VAC topné kabely, nebo
přepsat "Simulation Mode" a nasimulovat teploty. HA API je šifrované (`api.encryption`),
ale tenhle web_server vedle něj běží bez ochrany.

**Doporučení (na zvážení Architektovi):**
- (a) přidat `web_server: auth: username/password` (pokud to ESPHome 2026.1.3
  podporuje — ověřím při implementaci), nebo
- (b) zúžit `web_server` jen na diagnostiku (heater switche `internal: true`, ovládání
  jen přes HA API), nebo
- (c) web_server úplně odstranit — README ho zmiňuje jako "quick checks & simulation
  controls", ale HA API pokrývá totéž bezpečněji.

README dokumentuje web_server jako záměrnou featuru, takže je to rozhodnutí o UX, ne
jen bezpečnostní patch — proto na Architekta.

---

### S3 — `logger: level: DEBUG` — regrese oproti vlastnímu doporučení README 🔧 Implementer (po potvrzení)
**Závažnost:** Low–Medium

Aktuální YAML má `logger: level: DEBUG`. README §"ESPHome (ESP-IDF) Notes" (vlastní
doporučená base konfigurace projektu) říká `level: INFO`. DEBUG je žvatlavější,
zvyšuje seriové/API zatížení a v produkčním nasazení nemá důvod být zapnutý.

**Doporučení:** Přepnout na `INFO` pro normální provoz. Neměním to rovnou sám, protože
DEBUG může být záměrně zapnuté kvůli aktuální fázi bench-testování — potvrďte a
provedu.

---

### S4 — Ověřeno, není to bug (transparentnost) ✅
`esphome config` dump ukazoval podezřelé `gateway: !secret 'dns1'` místo
`!secret 'gateway'`. Prošetřeno: `secrets.yaml` má `gateway` a `dns1` jako nezávislé
klíče (žádné YAML anchors/aliases), a `--show-secrets` potvrdil, že **obě secret
hodnoty jsou shodné** — běžná a validní situace (router bývá zároveň gateway i DNS
server). Jde jen o kosmetickou zvláštnost `esphome config`, které při shodných
hodnotách dvou secrets špatně popisuje, který secret k položce patří. Žádná akce
potřeba, uvádím jen pro záznam, že to bylo prověřeno.

---

## B. Funkční nálezy

### F1 — Boot-time auto-restart defrostu nedělá to, co komentář slibuje 🏛️ Architekt
**Závažnost:** High (design-intent otázka)

```yaml
esphome:
  on_boot:
    priority: -10
    then:
    - script.stop: run_defrost
```
Komentář: *"Prevent auto-running the defrost script after each reboot (manual start
only)"*. Ve skutečnosti to nefunguje takto:

1. `on_boot` (priority -10) proběhne prakticky okamžitě po startu — v tu chvíli
   `run_defrost` ještě nikdy neběžel, takže `script.stop` nemá co zastavit (no-op).
2. Edge-trigger interval (`- interval: 1s`, řádky 686–699) má vlastní `static bool last
   = false`, který se resetuje při každém rebootu. Pokud je `DEFROST_ORDERED` (tj.
   `bs_defrost.state`) v okamžiku prvního tiku **už true** — typicky proto, že ESP32
   restartoval uprostřed reálného odmrazovacího cyklu jednotky (OTA, WiFi glitch, crash)
   — vyhodnotí se to jako "rising edge" a `run_defrost` se spustí automaticky.
3. `on_boot` guard tohle nezachytí, protože `script.stop` proběhl už dřív a nemá vazbu
   na budoucí spuštění.

V praxi tomu částečně (nezáměrně) brání to, že DS18B20 senzory po bootu chvíli
nehlásí platná data (viz F3 níže) — takže `bs_defrost.state` je při prvním tiku skoro
jistě `false`. Ale jakmile senzory po pár minutách "naskočí" a reálná defrost podmínka
je pořád aktivní, spustí se to i tak — jen o pár minut později, ne přesně na boot.

**Toto je rozhodnutí, ne jen bug:** je "auto-resume defrostu po rebootu, pokud podmínka
stále platí" vlastně žádoucí bezpečnostní chování (chrání to jednotku, což je smysl
celého systému), nebo má být striktně "manual start only", jak říká komentář? Pokud
druhé, potřebuje to jiný mechanismus (např. persistent flag "byl jsem právě rebootnut,
ignoruj první edge").

---

### F2 — Detekce chyb T1/T2 a T3/T4 ignoruje Simulation Mode 🔧 Implementer
**Závažnost:** High (rozbíjí vlastní testovací funkci)

```yaml
- interval: 20s
  then:
  - lambda: |-
      bool t1_valid = !isnan(id(t_outside).state);   // RAW senzor, ne _used
      bool t2_valid = !isnan(id(t_evap).state);
```
Detekční intervaly (20s pro T1/T2, 60s pro T3/T4) čtou přímo fyzické `dallas_temp`
senzory (`t_outside`, `t_evap`, `t_chassis`, `t_drain`), ne `_used` vrstvu, která
správně přepíná mezi reálnými a simulovanými hodnotami. Řídicí logika (HEAT_MODE,
DEFROST_ORDERED) tuhle abstrakci respektuje — error detekce ne.

**Důsledek:** Když je `Simulation Mode` zapnutý (typicky při bench testu bez
připojených čidel), fyzické senzory reálně vrací NaN → systém **neustále hlásí
T1/T2 a T3/T4 selhání**, i když je to naprosto očekávané a simulace jinak funguje
správně. Rozbíjí to smysl simulačního režimu jako testovacího nástroje bez hardwaru.

**Doporučení:** Detekce má buď číst `_used` senzory, nebo se má při `sim_mode == true`
úplně vypnout. Tohle přímo zapadá do ADR-004 (split na `err_t1..t4`) — navrhuju
vyřešit rovnou v té implementační session, ne jako izolovaný patch teď.

---

### F3 — Studený start: ~10minutové okno falešných chyb po každém rebootu 🔧 Implementer
**Závažnost:** Medium–High

Všechny 4 DS18B20 senzory mají:
```yaml
filters:
  median:
    window_size: 5
    send_every: 5
  #  send_first_at: 3
```
`send_first_at` je zakomentované. Median filtr nic nepublikuje, dokud se nenaplní
celé okno (5 vzorků) — při základním `update_interval: 120s` je to **až 10 minut**
po každém startu, než senzor poprvé nahlásí hodnotu (do té doby `state` = NaN).

Zatímco řídicí logika (HEAT_MODE/DEFROST_ORDERED) na to reaguje bezpečně (`isnan()`
check → vrací `{}`/false, nic nezapne), **error-detekční intervaly to bezpečně
neřeší** — čtou `isnan()` na stejných raw senzorech a do 20–60s po bootu correctně,
ale předčasně, nahlásí `err_t1_t2_fail`/`err_t3_t4_fail` = true. Tedy: **po každém
restartu zařízení systém sám sobě nahlásí falešnou poruchu senzorů na ~10 minut**,
včetně ERR LED pulzů a HA problem-entit.

**Doporučení:** Odkomentovat/nastavit `send_first_at: 1` (nebo 2–3), případně přidat
boot-time grace period před prvním vyhodnocením error-check intervalů. Malá,
nekontroverzní implementační volba — vyřeším sám, jen navrhnu přesné parametry k
potvrzení.

---

### F4 — Defrost heater běží na pevný časovač, ne na trvání skutečné podmínky (rozšíření OI3) 🏛️ Architekt
**Závažnost:** Medium (přesnější formulace existujícího OI3)

`defrost_chassis_cycle`/`defrost_drain_cycle` běží po pevně nastavenou dobu
(`chassis_time_min`/`drain_time_min`), nezávisle na tom, jestli `DEFROST_ORDERED`
mezitím zůstává true. Pokud reálný odmrazovací cyklus jednotky trvá déle než
nastavený čas, topení se vypne na časovači, i když jednotka ještě odmrazuje — a
protože je to edge-triggered (ne level-triggered), nedojde k re-triggeru, dokud
`DEFROST_ORDERED` nejdřív neklesne na false a znovu nevystoupá.

**Otázka pro Architekta:** má být doba topení vázaná na trvání podmínky (s nějakým
bezpečnostním stropem), místo slepého pevného časovače? Souvisí s OI3 v BACKLOG.md —
navrhuju to řešit spolu.

---

### F5 — Textový "Error Status" senzor zpožděn až 60s za binary_sensory 🔧 Implementer
**Závažnost:** Low

```yaml
text_sensor:
- platform: template
  name: "Error Status"
  update_interval: 60s
```
`bs_any_error`/`bs_err_*` binary sensory se přepočítávají prakticky okamžitě (žádný
`update_interval`), ale text senzor "Error Status" má vlastní 60s cyklus. Uživatel
může chvíli vidět "System Error Active: ON" a "Error Status: OK" současně.

**Doporučení:** Zkrátit interval nebo ho úplně odstranit (defaultní kontinuální
vyhodnocení). Drobná, bezpečná oprava — vyřeším sám.

---

## C. Již trackované v BACKLOG (jen doplněný kontext, ne nové OI)

| ID | Stav po review |
|---|---|
| OI1 (placeholder adresy senzorů) | Beze změny, stále blokuje reálné nasazení. |
| OI2 (testovací I2C blok) | Beze změny. |
| OI3 (paralelní defrost bez sync konce) | Viz F4 výše — přesnější formulace stejného problému. |
| OI4 (60s interval T3/T4) | Souvisí s F3 — stejný mechanismus (raw isnan check), řešit spolu. |
| OI5 (uptime) | Beze změny. |
| OI6 (heat_mode/defrost decoupling) | Potvrzeno v aktuálním kódu — `DEFROST_ORDERED` lambda kontroluje jen `main_system_enabled`, `HEAT_MODE`/`bs_heat_mode` se nikde nekontroluje. Pořád otevřené. |

---

## D. Drobnosti / hardening (nízká priorita, můžu kdykoliv) 🔧

- **H1** — DI1/DI2 (GPIO36/39) nemají definovaný pull-up/pull-down a jsou to ADC1
  piny bez HW pull podpory. Pokud zůstanou nezapojené, čekejte šumový
  press/release log spam (jsou `internal: true`, takže bez dopadu na HA). Bez akce,
  dokud nebudou fyzicky využité.
- **H2** — `esp32: framework: type: esp-idf` nemá pinovanou verzi frameworku —
  drobné riziko nereprodukovatelnosti buildu v čase. `requirements.txt` pinuje aspoň
  ESPHome (`==2026.1.3`).
- **H3** — Komentáře u dallas senzorů (T2/T3/T4) mají neobvyklé odsazení (kosmetické,
  zmíněno už v S1 kick-off review, funkčně bez dopadu).

---

## E. Co funguje dobře (pro rovnováhu)

- Sekrety správně přes `!secret`, nic citlivého není v gitu (ověřeno i strojově).
- `Main System Enable` switch má důkladný "safe state" turn_off_action — force-stopne
  všechny 3 scripty A vynutí OFF na obou heaterech, ne jen jedno nebo druhé.
- API šifrování + OTA heslo jsou nastavené.
- `restore_mode: ALWAYS_OFF` na obou heater switchích — po jakémkoli rebootu (i mimo
  scénář F1) start vždy s vypnutými topeními, ne s posledním stavem.
- Median filtr + dynamický polling (5s/20s refresh) je chytře navržený kompromis
  mezi rychlostí detekce a zátěží 1-Wire sběrnice — jen s vedlejším efektem F3.

---

## Shrnutí pro rozhodování

| # | Nález | Závažnost | Kdo |
|---|---|---|---|
| S1 | Otevřený fallback AP | Medium | 🏛️ Architekt/Lubor |
| S2 | Web server bez autentizace ovládá heatery | Medium–High | 🏛️ Architekt/Lubor |
| S3 | `logger: DEBUG` vs. README doporučení `INFO` | Low–Medium | 🔧 po potvrzení |
| F1 | Boot auto-restart defrostu neodpovídá komentáři | High | 🏛️ Architekt |
| F2 | Error detekce ignoruje Simulation Mode | High | 🔧 (spojit s ADR-004) |
| F3 | ~10min falešná chyba senzorů po každém bootu | Medium–High | 🔧 |
| F4 | Defrost timer neváže se na trvání podmínky | Medium | 🏛️ Architekt (= OI3) |
| F5 | Text sensor 60s lag za binary_sensory | Low | 🔧 |

---

## F. Výsledky společného průchodu (S3, Lubor + CC) — Resolutions

Všech 8 nálezů prošlo bod po bodu. Nic nezůstává otevřené k rozhodnutí — zbývá jen
formální ADR zápis (Architekt) pro dva body, které mění zdokumentované chování.

| # | Rozhodnutí | Vyžaduje |
|---|---|---|
| **F1** | Boot obnoví stav před rebootem, ne "manual start only". `defrost_running: restore_value: true`; `on_boot` (priority `-100`, ne `-10`) po restore explicitně zavolá `run_defrost`, pokud `defrost_running && main_system_enabled`. Zjednodušení: cyklus se po rebootu spustí **od začátku** (plná `chassis_time_min`/`drain_time_min`), ne s dopočítaným zbytkem — přesný zbytek by vyžadoval sekundové zápisy do flash (opotřebení NVS). Vzor ověřený ve Windows (`on_state`/`on_value` nefunguje pro obnovený/počáteční stav po bootu, jen na budoucí změny — Windows TC-6.1). | 🏛️ Formální ADR + update ARCHITECTURE.md §4.4/boot chování |
| **S1** | Přidat heslo na fallback AP: `wifi.ap.password: !secret ap_password` (nový secret, Lubor si ho spravuje sám). Konzistentně s Windows (`ESP32-D0WD-V3_Gar_Windows_02.yaml`). Bonus tidy-up: `ota:` na list syntax jako Windows; `wifi.id` NEpřidávat (u nás nepoužité). | 🔧 Implementer, žádné ADR nutné |
| **S2** | `web_server` zůstává (bez auth) pro bench ladění; odstranit/zabezpečit až po HA implementaci — Windows OI26 precedent (field build ho vypíná úplně, HA = sole control plane). Zapsáno jako **OI7** v BACKLOG.md. | ✅ Hotovo (BACKLOG) |
| **F4** | Nahradit pevný `delay:` za `wait_until` s `timeout:` — topení běží dokud `bs_defrost.state`, s `chassis_time_min`/`drain_time_min` jako bezpečnostní strop, ne pevná doba. **Naming HA labelů ("...Time (min)" → možná "...Max Time (min)") odloženo na ADR-005 execution session** — nepředbíhat rename, riziko dvojí změny (teď EN, pak CZ). | 🏛️ Formální ADR + update ARCHITECTURE.md §4.4; naming poznámka pro budoucí ADR-005 session |
| S3 (logger) | Zachovat `DEBUG` globálně; tlumit konkrétní tagy přes `logs: {tag: WARN}` až/pokud budou otravné (Windows vzor), ne plošně na `INFO`. | 🔧 Implementer, žádné ADR nutné |
| S4 | Ověřeno, není bug (viz výše). | ✅ Uzavřeno |
| F2, F3, F5 | Beze změny — zůstávají 🔧 Implementer, F2 spojit s ADR-004 implementační session (err_t1..t4 split). | 🔧 Implementer |
| H1–H3 | Beze změny, nízká priorita. | 🔧 Implementer, kdykoli |

### Nové body zjištěné při Windows srovnání

- **`led_sequencer` je custom C++ komponenta**, ne vestavěná ESPHome platforma —
  před implementační session pro ADR-004 nutno zkopírovat zdroják z
  `Garage_Windows/firmware/custom_components/led_sequencer/` do našeho
  `firmware/custom_components/`. Bez toho se ADR-004 YAML blok nezkompiluje.
- Rukopisné konvence k převzetí ze Windows (potvrzeno, aplikuju při příští úpravě
  dotčených sekcí): section dividery (`# ── Název ──...`), hlavičkový komentář
  "README.md is the single source of truth for pin assignments" (ověřeno — README
  je už teď přesné pro všech 9 produkčních pinů, viz tabulka výše), zarovnané klíče
  v `manual_ip`, `on_boot priority: -100`.

---

*Review provedl: Implementer/CC, session S3, 2026-07-10. Firmware YAML nebyl v této
session měněn — čistě analytická/dokumentační session. Výsledky prošly společně s
Luborem bod po bodu (viz sekce F výše). Zbývá: formální ADR zápis Architektem pro
F1 a F4, pak implementační session(y).*
