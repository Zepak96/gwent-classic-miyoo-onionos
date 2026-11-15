# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę, róg, Bohaterów, Morale, Więź (3 kopie), pasowanie, remis, Nilfgaard, Szpiega, Medyka, Manekina, Zbiórkę, Mardroeme/Berserker, Scorch, rozdanie i wymianę kart, walidację talii, katalog oraz 10 talii referencyjnych z liderami. Commit `7e41eff` został zbudowany w GitHub Actions i zaliczył 2/2 testy (`engine_rules`, `card_catalog`). Testy nadal nie pokrywają wszystkich kart, AI, UI ani wydania ARM.

| Obszar | Status | Uwagi |
|---|---|---|
| Budowanie desktop | Zaliczone w CI | Ubuntu, CMake; budowany tylko silnik/testy, bez SDL frontend |
| Budowanie ARM | Niewykonane | Brak uruchomionego toolchaina |
| Zasady / regresja | W trakcie | Rozbudowano lokalne testy; czekają na CI |
| Interfejs 640×480 | Niewykonane | Jest tylko szkielet SDL |
| Audio | Niewykonane | Brak implementacji audio i legalnych ścieżek |
| OnionOS | Niewykonane | Brak binarki/paczki i urządzenia |
| Długi test / RAM | Niewykonane | Nie przeprowadzono |

## Test sprzętowy

Nie wykonano testu na fizycznym Miyoo Mini Plus. Zgodność pozostaje niezweryfikowana.
