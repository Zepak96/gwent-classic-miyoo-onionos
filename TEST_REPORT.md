# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują teraz podstawowe punktowanie, pogodę, róg, Bohaterów, Morale, Więź (3 kopie), pasowanie, remis, Nilfgaard, Szpiega, Medyka, Manekina, Zbiórkę, rozdanie i wymianę kart oraz część walidacji talii. Commit `5273ba3` został zbudowany w GitHub Actions; `ctest` zaliczył 2/2 testy (`engine_rules`, `card_catalog`). Test katalogu potwierdza 216 rekordów, w tym next-gen Roach oraz muster na Geralt/Ciri. Wciąż są to podstawowe asercje i walidacja danych; zakres nie pokrywa wszystkich kart i zasad.

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
