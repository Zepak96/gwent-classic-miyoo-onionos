# Gwent Classic dla Miyoo Mini Plus / OnionOS

Natywny port 1:1 Gwinta jako minigry z **Wiedźmina 3: Dziki Gon** na Miyoo Mini Plus — bazujący na wskazanym repozytorium Gwent Classic oraz obejmujący docelowo podstawkę, Hearts of Stone, Blood and Wine i zmiany/karty z aktualizacji next-gen Wiedźmina 3. Repo referencyjne określa zachowanie gry; nie zastępujemy go zasadami samodzielnej gry `GWENT: The Witcher Card Game`. Projekt jest na wczesnym etapie: obecny kod nie jest kompletną grywalną implementacją ani gotowym portem OnionOS.

## Stan

Aktualny postęp, zakres audytu i braki opisuje [PROGRESS.md](PROGRESS.md). Nie ma jeszcze binarki ARM, paczki ZIP ani potwierdzenia działania na Miyoo. Zasady i karty są w trakcie implementacji; nie traktuj obecnego kodu jako wiernej kompletnej wersji gry.

## Budowanie

Wymagania i polecenia opisuje [BUILD.md](BUILD.md). Projekt wymaga CMake oraz kompilatora C++17; frontend wymaga SDL2.

## Licencje i zasoby

Ten projekt nie dołącza kodu ani zasobów z repozytoriów referencyjnych. Zobacz [LICENSES.md](LICENSES.md). Nie są dołączone materiały CD Projekt RED.

## Sterowanie

Frontend SDL2 nie ma jeszcze interfejsu gry ani kompletnego mapowania przycisków Miyoo. Samo zamknięcie okna działa przez SDL_QUIT/Escape w środowisku desktopowym; sterowanie docelowe nie zostało zweryfikowane.
