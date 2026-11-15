# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę, róg, Bohaterów, Morale, Więź (3 kopie), pasowanie, remis, Nilfgaard, Szpiega, Medyka, Manekina, Zbiórkę, rozdanie i wymianę kart oraz walidację talii. Commit `5273ba3` został zbudowany w GitHub Actions i zaliczył 2/2 testy (`engine_rules`, `card_catalog`). Bieżące lokalne zmiany dodają build-time generator katalogu i dodatkowe testy; czekają na kolejny wynik CI. Test katalogu sprawdza 216 rekordów źródłowych oraz next-gen Roach i muster na Geralt/Ciri. Testy nadal nie pokrywają wszystkich kart i zasad.

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
