# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują teraz podstawowe punktowanie, pogodę, róg, Bohaterów, Morale, Więź (3 kopie), pasowanie, remis, Nilfgaard, Szpiega, Medyka, Manekina, Zbiórkę i część walidacji talii. Zmiany są jeszcze lokalne i czekają na wynik kolejnego GitHub Actions CI. Ostatni zweryfikowany build (`a68363b3b6733fa06dc54f94482b773d98129cd5`) zakończył się sukcesem z 1/1 testów, ale dotyczył starszej wersji kodu. Zakres nadal nie pokrywa wszystkich kart i zasad.

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
