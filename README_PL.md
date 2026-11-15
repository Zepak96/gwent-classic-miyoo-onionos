# Gwent Classic dla Miyoo Mini Plus / OnionOS

Natywny port Gwinta jako minigry z **Wiedźmina 3: Dziki Gon** — obejmujący podstawkę, Hearts of Stone, Blood and Wine oraz dodatki/zmiany Gwinta z aktualizacji next-gen Wiedźmina 3. Projekt nie odtwarza samodzielnej gry `GWENT: The Witcher Card Game` ani jej zasad online. Projekt jest na wczesnym etapie: obecny kod to szkielet silnika i prosty frontend SDL2, a nie kompletna grywalna implementacja.

## Stan

Aktualny postęp, zakres audytu i braki opisuje [PROGRESS.md](PROGRESS.md). Nie ma jeszcze binarki ARM, paczki ZIP ani potwierdzenia działania na Miyoo. Zasady i karty są w trakcie implementacji; nie traktuj obecnego kodu jako wiernej kompletnej wersji gry.

## Budowanie

Wymagania i polecenia opisuje [BUILD.md](BUILD.md). Projekt wymaga CMake oraz kompilatora C++17; frontend wymaga SDL2.

## Licencje i zasoby

Ten projekt nie dołącza kodu ani zasobów z repozytoriów referencyjnych. Zobacz [LICENSES.md](LICENSES.md). Nie są dołączone materiały CD Projekt RED.

## Sterowanie

Frontend SDL2 nie ma jeszcze interfejsu gry ani kompletnego mapowania przycisków Miyoo. Samo zamknięcie okna działa przez SDL_QUIT/Escape w środowisku desktopowym; sterowanie docelowe nie zostało zweryfikowane.
