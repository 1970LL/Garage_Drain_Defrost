# ADR-005+ — Návrh doporučení pro budoucí HA entity-exposure práci

> **Status: Resolved (S16, 2026-08-01)** → viz **ADR-015** (HA_RD_Jirny repo)
> + `ENTITY_EXPOSURE_HA_Garage_Drain_Defrost.md` (tento repo, autoritativní
> instance dle ADR-015 §1/§6). HA Architekt formalizoval standardní
> entity-exposure formát (device grouping + ikony + device_class audit jako
> standardní součást), Garage_Drain_Defrost je druhý worked example (funkční
> osa) vedle Garage_Windows (fyzická osa). **Tento draft = origin dokument**
> pro tu destilaci, ponechán jako historický záznam procesu/poučení.
>
> ---
>
> **Původní status (S15): Návrh Implementera, čeká na Architekta.** Není to
> ADR — Implementer (CC) ADR nepíše sám (`WORKING_AGREEMENT.md` §1.2). Tohle
> byl vstup pro Architekta k posouzení, jestli si zaslouží formální ADR-012
> (nebo dodatek k ADR-005), nebo stačí poznámka do `BACKLOG.md`/`ARCHITECTURE.md`.
> **Původ:** Lubor po dokončení ADR-005 execution (S14/S15, entity rename +
> device grouping + ikony) požádal o zachycení doporučení a poučení z procesu,
> než se ztratí.

---

## 1. Doporučení — co by měla budoucí "entity-exposure" ADR obsahovat rovnou

ADR-005 samo o sobě specifikovalo jen principy (CZ friendly names, grouping,
"Stav systému"/"Kód chyby" text senzory) a exekuci nechalo na Implementerovi.
V praxi se ukázalo, že tahle exekuce má víc vrstev, než ADR-005 předpokládalo
— proběhla ve **dvou kolech** (entity rename+grouping S14, ikony S15), a
druhé kolo odhalilo problém (`device_class` matoucí stav text), který mohl
být odchycen dřív, kdyby byl součástí prvního zadání.

**Doporučení: budoucí ADR pro HA entity exposure (na tomhle nebo dalším
projektu) by měla rovnou zahrnovat:**

1. **Device grouping** — tabulka `device_id` → HA název → účel, i když
   konkrétní názvy skupin budou pro každý projekt jiné (tenhle projekt zvolil
   funkční dělení Globální/Provoz/Nastavení/Servisní, protože nemá víc
   fyzických jednotek; Garage_Windows zvolil fyzické dělení Globální/Okno
   1/Okno 2, protože jich má víc). **Princip k zapsání:** zvol osu podle toho,
   čeho má projekt skutečně víc — fyzických jednotek, nebo funkčních oblastí.
   Nekopíruj strukturu referenčního projektu naslepo.
2. **Ikony (`icon: "mdi:..."`)** — specifikovat u každé entity rovnou v
   prvním kole, ne až dodatečně. Levné přidat na začátku, dražší (druhé kolo
   review) přidat později.
3. **`device_class` audit** — explicitní krok: pro každou entitu s
   `device_class:` ověřit, jaký text stavu HA skutečně zobrazí (ne jen jakou
   ikonu) — viz Poučení #1 níže.
4. **Timing doporučení:** entity rename/grouping dělat **před** prvním
   párováním s HA, ne po něm — viz Poučení #2.

---

## 2. Poučení z procesu (S14/S15)

### Poučení #1 — `device_class` řídí i text stavu, ne jen ikonu
`bs_heat_mode` mělo `device_class: cold` kvůli pohodlné výchozí ikoně
(sněhová vločka). Lubor si všiml, že HA zobrazuje stav jako "Chladno", ne
Zapnuto/Vypnuto — `device_class` v HA určuje sémantický pár on/off textů
specifický pro danou třídu (cold→"Cold"/"Normal", problem→"Problem"/"OK",
connectivity→"Connected"/"Disconnected", atd.), nejen ikonu. Pro binary_sensor,
jehož smysl je "je/není nějaký specifický stav" (porucha, konektivita),
sémantický text sedí. Pro binary_sensor, jehož smysl je spíš "je zapnuto
zařízení/mód" (i když se to *týká* chladu), sémantický text může být matoucí
— tam je lepší žádný `device_class` a vlastní `icon:`.
**Zapsáno jako BUG-007-adjacent nález, oprava viz `BUGS.md`/`ARCHITECTURE.md` §8.**

### Poučení #2 — rename `name:` po HA go-live = osiřelé entity
ESPHome odvozuje HA `unique_id` z `name:` (přes object_id hash), ne ze
stabilního YAML `id:`. Než HA vůbec poprvé zařízení spárovala, rename byl
"zdarma" — žádná stará registrace k osiření. **Po** go-live by každý další
rename vytvořil v HA entity registry ducha (stará entita zůstane
"unavailable" navždy, dokud se ručně nesmaže) a novou entitu vedle. Poučení:
dolaďovat `name:`/grouping/ikony **před** prvním párováním je jednorázová
příležitost bez nákladů — po ní se to prodraží. Tenhle projekt měl štěstí v
načasování (ADR-005 proběhlo těsně před prvním párováním); příště plánovat
vědomě.

### Poučení #3 — event-driven refresh: `binary_sensor` dedupuje, `sensor`/`text_sensor` ne
Opakovaně ověřeno v tomhle projektu (OI17, OI13, BUG-007): `binary_sensor`
publikuje/loguje jen na skutečné změně stavu, ale `sensor`/`text_sensor` s
`component.update()` vždy re-publikuje bez ohledu na to, jestli se hodnota
změnila. Napojit refresh derived entity na existující sdílený timer (i "jen
jeden řádek navíc") je bezpečné pro binary_sensor, ale u sensor/text_sensor
to vždycky vede k redundantní publikaci/log spamu — vždy navázat na skutečný
moment změny zdroje (`on_value:`/`on_state:`/přímé volání v akci uvnitř
lambdy), ne na periodický tik. BUG-007 je přesně tenhle případ.
**Doporučení pro budoucí ADR/handoffy:** kdykoli se navrhuje nová derived
`sensor`/`text_sensor` entita, rovnou specifikovat event-driven refresh zdroj,
ne "napoj na existující interval".

### Poučení #4 — dvoukolový draft-review proces se osvědčil
Metoda použitá pro ADR-005 execution (crosscheck markdown tabulka
entita-po-entitě → Lubor upraví přímo v souboru → Implementer realizuje →
bench test) fungovala dobře pro plošný zásah (~38 entit). Soubor šel
opakovaně z `#Archive/` zpátky do aktivní složky pro druhé kolo (ikony) —
nový vzor pro "pracovní draft", zapsaný do `Software_DevDoc_structure.md`.
**Doporučení:** použít stejnou metodu (ne jednorázový handoff prompt) pro
jakoukoli budoucí práci, která se dotýká víc než ~10 entit najednou.

---

## 3. Otevřená otázka pro Architekta

Stojí tohle za formální ADR-012 ("HA Entity Exposure Conventions"), který by
zafixoval body 1-4 z §1 jako standard pro tenhle i budoucí projekty (šablona
pro Garage_Windows i další)? Nebo je to dost zachycené touhle poznámkou +
`ARCHITECTURE.md` §8 a nemá smysl to formalizovat dál?

**Odkazy:** ADR-005, `BUGS.md` BUG-007, `ARCHITECTURE.md` §8,
`#Archive/ADR-005_execution_draft.md` (Round 1 + Round 2).
