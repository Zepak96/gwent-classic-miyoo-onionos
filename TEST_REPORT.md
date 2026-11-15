# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę, róg, Bohaterów, Morale, Więź (3 kopie), kolejność Więzi i Morale, pasowanie, remis, Nilfgaard, Szpiega, Medyka, Manekina, Zbiórkę, Mardroeme/Berserker, Scorch, rozdanie i wymianę kart, walidację talii, katalog, 10 talii referencyjnych oraz wybrane pasywne i aktywne zdolności liderów. Commit `edd2bf1` został zbudowany w GitHub Actions i zaliczył 2/2 testy CTest (`engine_rules`, `card_catalog`), workflow [desktop-build #43710397110](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43710397110). To potwierdza build silnika/testów na Ubuntu, ale nie pełną zgodność wszystkich kart, pełnego AI, UI SDL, wydania ARM ani działania na Miyoo.

| Obszar | Status | Uwagi |
|---|---|---|
| Budowanie desktop | Zaliczone w CI | Ubuntu, CMake; budowany tylko silnik/testy, bez SDL frontend |
| Budowanie ARM | Niewykonane | Brak uruchomionego toolchaina |
| Zasady / regresja | Częściowo zaliczone w CI | Testy jednostkowe istnieją dla wybranych reguł i liderów; kompletne pokrycie gry nadal nie istnieje |
| Interfejs 640×480 | Niewykonane | Jest tylko szkielet SDL |
| Audio | Niewykonane | Brak implementacji audio i legalnych ścieżek |
| OnionOS | Niewykonane | Brak binarki/paczki i urządzenia |
| Długi test / RAM | Niewykonane | Nie przeprowadzono |

## Test sprzętowy

Nie wykonano testu na fizycznym Miyoo Mini Plus. Zgodność pozostaje niezweryfikowana.
