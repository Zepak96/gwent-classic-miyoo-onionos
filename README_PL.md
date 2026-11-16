# Gwent Classic dla Miyoo Mini Plus / OnionOS

Natywny port 1:1 Gwinta jako minigry z **Wiedźmina 3: Dziki Gon** na Miyoo Mini Plus — bazujący na wskazanym repozytorium Gwent Classic oraz obejmujący docelowo podstawkę, Hearts of Stone, Blood and Wine i zmiany/karty z aktualizacji next-gen Wiedźmina 3. Repo referencyjne określa zachowanie gry; nie zastępujemy go zasadami samodzielnej gry `GWENT: The Witcher Card Game`. Prototyp SDL 640×480 ma mecz AI, talie referencyjne, mulligan i edytor talii z zapisem trzech slotów. Port OnionOS i kompletna zgodność z grą pozostają w trakcie prac.

## Stan

Aktualny postęp, zakres audytu i braki opisuje [PROGRESS.md](PROGRESS.md). Obecny kod nie jest kompletną implementacją gry ani gotowym portem OnionOS.

Aktualne klatki smoke 640×480: [edytor talii](artifacts/qa/deck-builder-card-preview.png), [wybór talii](artifacts/qa/deckselect-menu.png), [karuzela wyboru karty](artifacts/qa/target-carousel.png) i [inspektor karty](artifacts/qa/card-details-polish.png). To zrzuty desktopowego smoke testu, nie z fizycznego Miyoo.

## Budowanie

Wymagania i polecenia opisuje [BUILD.md](BUILD.md). Projekt wymaga CMake oraz kompilatora C++17; frontend wymaga SDL2.

## Licencje i zasoby

Repozytorium zawiera grafiki, ikony i 35 SFX z przypiętej wersji repozytorium wzorcowego oraz dwa utwory MP3 zapętlane jako tło menu i meczu. Oryginalne ilustracje są już wyświetlane na ekranach gry i w edytorze talii. Pliki muzyki: [menu.mp3](assets/gwent-classic/music/menu.mp3) i [match.mp3](assets/gwent-classic/music/match.mp3). Zobacz [instrukcję folderu muzyki](assets/gwent-classic/music/README_PL.md), [LICENSES.md](LICENSES.md) i `assets/gwent-classic/ASSET_ORIGIN.md`.

## Sterowanie

Program startuje w edytorze talii, zgodnie z ekranem `DeckMaker` w repozytorium źródłowym. Ekran wyboru talii otwiera się z edytora przez Escape/B. Tam lewo/prawo zmienia talię, Y zmienia język, A/Start rozpoczyna mecz, R wczytuje ostatni zapis, X/B otwiera edytor, P wybiera talię referencyjną, a [ / ] zmienia slot własnej talii. Edytor pokazuje karty wybranej frakcji i neutralne: lewo/prawo przełącza bank i talię, góra/dół wybiera kartę, A dodaje lub usuwa kopię zależnie od panelu, B w panelu talii usuwa kopię, L wybiera lidera, Y zmienia frakcję, Select zapisuje poprawną talię, a Start rozpoczyna mecz. X otwiera tekstowy podgląd karty. Własne talie zapisują się w trzech slotach.

AI używa jednego ważonego modelu `ControllerAI` z repozytorium referencyjnego; gra nie oferuje osobnych poziomów trudności. Przy zdolności Scoia'tael lewo/prawo wybiera rozpoczynającego, A zatwierdza. Podczas mulliganu lewo/prawo wybiera kartę, A zaznacza, Start/Y zatwierdza albo pomija wymianę, a X otwiera podgląd. Licznik pokazuje pozostałe wymiany z dwóch. W meczu lewo/prawo wybiera kartę, góra/dół rząd, A zagrywa, Y używa lidera, Select prosi o potwierdzenie pasowania, a Start otwiera pauzę. X pokazuje opis karty. W pauzie można wznowić, zapisać lub wczytać mecz, zmienić język, wyłączyć dźwięk albo wrócić do wyboru talii. Szczegóły mapowania przycisków Miyoo są w [docs/MIYOO_INPUT.md](docs/MIYOO_INPUT.md).

SDL Mini dla Miyoo jest budowany z poprawką mapującą fizyczne **MENU** (`KEY_HOME`) na `SDLK_HOME`; aplikacja zapisuje mecz i kończy działanie. Fizyczne działanie przycisków i audio nadal wymagają potwierdzenia na konsoli.
