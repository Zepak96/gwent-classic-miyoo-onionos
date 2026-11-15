# Gwent Classic dla Miyoo Mini Plus / OnionOS

Natywny port 1:1 Gwinta jako minigry z **Wiedźmina 3: Dziki Gon** na Miyoo Mini Plus — bazujący na wskazanym repozytorium Gwent Classic oraz obejmujący docelowo podstawkę, Hearts of Stone, Blood and Wine i zmiany/karty z aktualizacji next-gen Wiedźmina 3. Repo referencyjne określa zachowanie gry; nie zastępujemy go zasadami samodzielnej gry `GWENT: The Witcher Card Game`. Prototyp SDL 640×480 ma mecz AI, talie referencyjne, mulligan i edytor talii z zapisem trzech slotów. Port OnionOS i kompletna zgodność z grą pozostają w trakcie prac.

## Stan

Aktualny postęp, zakres audytu i braki opisuje [PROGRESS.md](PROGRESS.md). Obecny kod nie jest kompletną implementacją gry ani gotowym portem OnionOS.

Aktualne klatki smoke 640×480: [menu tytułowe](docs/screenshots/current-title-smoke.png), [wybór talii i poziomu AI](docs/screenshots/current-deck-select-smoke.png), [mecz](docs/screenshots/current-match-smoke.png), [pauza i ustawienia](docs/screenshots/current-pause-smoke.png), [wybór karty lidera](docs/screenshots/current-leader-choice-smoke.png), [ujawnione karty Emhyra](docs/screenshots/current-leader-reveal-smoke.png), [inspektor karty](docs/screenshots/current-card-details-smoke.png), [edytor talii](docs/screenshots/current-deck-smoke.png) i [animacja zagrania](docs/screenshots/current-card-flight-smoke.png). To zrzuty z desktopowego smoke testu, nie z fizycznego Miyoo.

## Budowanie

Wymagania i polecenia opisuje [BUILD.md](BUILD.md). Projekt wymaga CMake oraz kompilatora C++17; frontend wymaga SDL2.

## Licencje i zasoby

Repozytorium zawiera grafiki, ikony i 35 SFX z przypiętej wersji repozytorium wzorcowego oraz dwa utwory MP3 zapętlane jako tło menu i meczu. Oryginalne ilustracje są już wyświetlane na ekranach gry i w edytorze talii. Pliki muzyki: [menu.mp3](assets/gwent-classic/music/menu.mp3) i [match.mp3](assets/gwent-classic/music/match.mp3). Zobacz [instrukcję folderu muzyki](assets/gwent-classic/music/README_PL.md), [LICENSES.md](LICENSES.md) i `assets/gwent-classic/ASSET_ORIGIN.md`.

## Sterowanie

Na ekranie tytułowym Enter/X otwiera wybór talii. W wyborze talii lewo/prawo zmienia talię, Z język, Enter/X rozpoczyna mecz, R wczytuje ostatni zapis, B otwiera edytor, P przełącza na talię referencyjną, a [ / ] wybiera slot własnej talii. Edytor pokazuje karty frakcji i neutralne; góra/dół wybiera kartę, Enter/A dodaje kopię, Backspace/B usuwa, L/X przełącza wybór lidera, F/Y zmienia frakcję, S/SELECT zapisuje poprawną talię, START/Escape wraca bez zapisu. Własne talie zapisują się w trzech slotach.

AI ma trzy poziomy wybierane strzałkami góra/dół na ekranie wyboru talii: **Łatwy** wybiera losowo spośród legalnych ruchów i może popełniać błędy strategiczne; **Normalny** używa ważonego AI z repozytorium referencyjnego; **Trudny** wybiera legalny ruch o najwyższej wartości heurystycznej. Wybór poziomu i języka zapisuje się w ustawieniach. Przy zdolności Scoia'tael lewo/prawo wybiera rozpoczynającego, Enter zatwierdza. Podczas mulliganu lewo/prawo wybiera kartę; A ją zaznacza, Start lub Y zatwierdza wymianę albo pomija ją, a X otwiera powiększony podgląd. Licznik pokazuje pozostałe wymiany z dwóch. W meczu lewo/prawo wybiera kartę, góra/dół rząd, Enter/X zagrywa, L używa lidera, Backspace pasuje; dla Manekina/Medyków strzałki i Enter wybierają cel. W menu pauzy (P/Start) można wznowić grę, zapisać/wczytać mecz, zmienić poziom AI i język, wyłączyć SFX albo wrócić do menu talii. Pasowanie z przycisku Select wymaga potwierdzenia A; B anuluje. Na początku trzeciej rundy Skellige losuje dwie jednostki z cmentarza; dla wybranej Zwinnej gracz wybiera rząd.

SDL Mini dla Miyoo jest budowany z małą poprawką mapującą fizyczne **MENU** (`KEY_HOME`) na `SDLK_HOME`; aplikacja zapisuje mecz i kończy działanie. **A** zatwierdza i zaznacza kartę do wymiany, **B** wraca/anuluje, **X** otwiera podgląd, **Y** aktywuje lidera lub zatwierdza/pomija mulligan, **L/R** zmienia fokus, **L2/R2** przewija, **Start** otwiera pauzę lub zatwierdza/pomija mulligan, a **Select** prosi o potwierdzenie pasowania. Zmodyfikowany SDL i aplikacja budują się dla ARM; fizyczne działanie przycisków wymaga jeszcze testu na konsoli. Szczegóły są w [docs/MIYOO_INPUT.md](docs/MIYOO_INPUT.md).
