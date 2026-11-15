# Gwent Classic dla Miyoo Mini Plus / OnionOS

Natywny port 1:1 Gwinta jako minigry z **Wiedźmina 3: Dziki Gon** na Miyoo Mini Plus — bazujący na wskazanym repozytorium Gwent Classic oraz obejmujący docelowo podstawkę, Hearts of Stone, Blood and Wine i zmiany/karty z aktualizacji next-gen Wiedźmina 3. Repo referencyjne określa zachowanie gry; nie zastępujemy go zasadami samodzielnej gry `GWENT: The Witcher Card Game`. Jest już grywalny prototyp meczu SDL 640×480 z taliami referencyjnymi, mulliganem i AI, ale projekt nie jest kompletną implementacją ani gotowym portem OnionOS.

## Stan

Aktualny postęp, zakres audytu i braki opisuje [PROGRESS.md](PROGRESS.md). Nie ma jeszcze binarki ARM, paczki ZIP ani potwierdzenia działania na Miyoo. Zasady i karty są w trakcie implementacji; nie traktuj obecnego kodu jako wiernej kompletnej wersji gry.

## Budowanie

Wymagania i polecenia opisuje [BUILD.md](BUILD.md). Projekt wymaga CMake oraz kompilatora C++17; frontend wymaga SDL2.

## Licencje i zasoby

Ten projekt nie dołącza kodu ani zasobów z repozytoriów referencyjnych. Zobacz [LICENSES.md](LICENSES.md). Nie są dołączone materiały CD Projekt RED.

## Sterowanie

Na ekranie tytułowym Enter/X otwiera wybór talii. W wyborze talii lewo/prawo zmienia talię, Z język, Enter/X rozpoczyna mecz. Przeciwnik używa jednego trybu AI „Classic”, opartego na ważonych decyzjach z repozytorium referencyjnego; repozytorium nie definiuje poziomów Easy/Normal/Hard. Podczas mulliganu lewo/prawo wybiera kartę, spacja/X ją zaznacza, Enter zatwierdza. W meczu lewo/prawo wybiera kartę, góra/dół rząd, Enter/X zagrywa, L używa lidera, Backspace pasuje; dla Manekina/Medyków strzałki i Enter wybierają cel. Escape wraca do menu. Są to skróty SDL; mapowanie fizycznych przycisków Miyoo i działanie na urządzeniu pozostają do sprawdzenia.
