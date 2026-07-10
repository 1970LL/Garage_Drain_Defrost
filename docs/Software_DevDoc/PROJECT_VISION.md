# PROJECT VISION — Garage_Drain_Defrost

> **Účel:** Strategický rámec projektu — co projekt dělá a proč, jaké jsou fáze, hranice (scope, non-goals), constraints.
> **Vlastník:** Lubor (Project Owner).
> **Pravidlo:** PŘEPISUJE se. Aktuální verze je platná. Historie je v gitu.

---

## Co projekt dělá

Projekt **Garage_Drain_Defrost** je ochranný topný systém proti zamrzání pro **venkovní
jednotku klimatizace (Toshiba)** instalovanou v garáži. Systém chrání dvě kritická místa:

- **šasi venkovní jednotky** (proti zamrznutí kondenzátu na povrchu),
- **odtokové potrubí kondenzátu** (proti ucpání ledem a přetečení).

Systém sleduje čtyři teplotní čidla (DS18B20): venkovní teplotu, teplotu výparníku,
teplotu šasi a teplotu odtokové trubky. Z chování teploty výparníku **detekuje odmrazovací
cyklus** jednotky (defrost) a v reakci na něj i na obecné mrazové podmínky aktivuje topné
kabely na šasi a odtoku po konfigurovatelnou dobu. Prahy i časovače jsou nastavitelné přímo
z **Home Assistant** bez nutnosti rekompilace.

Řídicí jednotka je postavena na **ESP32-WROOM-32UE** s firmwarem **ESPHome (ESP-IDF
backend)** na custom PCB s MOSFET/SSR výstupy pro 230 VAC topné kabely.

## Proč ho dělám

Projekt je **privátní automatizační projekt**, navazující na zkušenost z Garage_Windows.
Cíl je praktický — ochránit klimatizační jednotku před zamrznutím v zimním provozu — a
zároveň pokračovat ve stejném AI-asistovaném vývojovém modelu (Architekt / Implementer /
Claude Code) osvědčeném na předchozím projektu.

Na rozdíl od Garage_Windows, kde byl software hlavní učební oblastí a hardware rutinou,
je u tohoto projektu **těžiště jinde**: funkční logika je jednodušší (prahová logika,
časovače, jednoduchý stavový automat — žádná uzavřená polohová smyčka, žádný motor pod
napětím v pohybu). Hlavní přínos standardizovaného workflow je tu **rychlost a pořádek**,
ne hluboké technické učení.

## Cílový uživatel

**Já + rodina.** Provoz je většinou neviditelný — systém běží na pozadí a zasahuje jen
v mrazu. Důsledky pro design:

- **HA dashboard** ukazuje stav (topení aktivní/neaktivní, chyby), ale nevyžaduje časté
  zásahy uživatele
- **Bez nutnosti lokálního ovládání** — na rozdíl od oken tu není potřeba fyzické tlačítko
  pro běžný provoz
- **Spolehlivost je důležitá, ale ne kriticky bezpečnostní** — selhání znamená riziko
  poškození AC jednotky nebo ucpání odtoku, ne fyzické ohrožení osob nebo nekontrolovaný
  pohyb mechaniky

## Přístup ke kvalitě

**Odlehčený (good-enough)** — rychlejší iterace, méně formální dokumentace a testovací
aparát než u Garage_Windows.

Důvod tohoto postoje:

1. **Nižší riziko selhání.** Nezamrzlé topení vede k postupnému poškození (koroze,
   ucpání), ne k náhlé bezpečnostní události. Je čas reagovat (vizuální kontrola, HA
   notifikace), než dojde ke škodě.
2. **Jednodušší systém.** Bez uzavřené regulační smyčky, bez pohyblivých částí, bez
   komplexní interlock logiky mezi kanály — menší prostor pro subtilní bugy vyžadující
   hloubkovou root-cause analýzu.
3. **Hardware je již téměř hotový** — hlavní práce zbývá na softwarové straně, kde chceme
   projít review existujícího kódu (ChatGPT/breadboard éra) a doladit ho, ne stavět od nuly
   s plnou formální rigorózností.

**Praktický důsledek pro AI role:** Architekt a Implementer nemusí eskalovat každou
drobnost k plné ADR analýze s variantami A/B/C. Rychlé rozhodnutí + poznámka v BACKLOG/
DECISIONS stačí. Testovací aparát je checklist, ne 30+ bodová akceptační sada. Dokumentace
roste podle potřeby, ne preventivně do hloubky.

## Fáze projektu

| Fáze | Cíl | Stav |
|---|---|---|
| **0 — Concept & Breadboard** | Zadání, HW scope, funkční prototyp na breadboardu s ChatGPT-asistovaným kódem | ✅ Done |
| **1 — Doc-sync & Code Review** | Nastartovat standardizovanou dokumentaci, provést review existujícího kódu, vytěžit dílčí úkoly | 🔄 Active |
| **HW Finalizace** | Dokončení a osazení custom PCB, ověření na finálním HW | ⏳ Planned |
| **2 — Firmware Cleanup & Hardening** | Odstranění testovacích/dočasných bloků z kódu (TODO adresy čidel, testovací VL53L0X/BME280 blok), doladění defrost logiky podle review | ⏳ Planned |
| **3 — Bench Validation** | Ověření defrost/heat logiky na finálním HW se simulačním režimem i reálnými senzory | ⏳ Planned |
| **4 — Field Deployment** | Nasazení v garáži, ověření přes zimní sezónu | ⏳ Planned |

*(Detailní fázový model — cíle, stav, hlavní výstupy — vede `ROADMAP.md`; tato tabulka
je jen orientační zrcadlo pro rychlou orientaci a musí s ním zůstat v souladu.)*

---

## Verzování dokumentu

| Verze | Datum | Změna |
|---|---|---|
| 1.0 | 2026-07-10 | První verze. Kick-off restartu projektu pod standardizovaným workflow (štíhlý seed z Garage_Windows). |
| 1.1 | 2026-07-10 | Fázová tabulka sladěna s `ROADMAP.md` (doplněna fáze "3 — Bench Validation", Field Deployment posunut na fázi 4). Nalezeno při konzistenční revizi doc seedu (S1). |
