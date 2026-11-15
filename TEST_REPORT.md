# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne zostały dodane do CMake dla podstawowych przypadków punktacji i przejścia pasowania. Desktopowy build i testy zostały uruchomione przez GitHub Actions na Ubuntu dla commita `a68363b3b6733fa06dc54f94482b773d98129cd5` — workflow `desktop-build` zakończył się powodzeniem. `ctest` raportuje 1/1 testów zaliczonych. Zakres testów jest bardzo mały i nie potwierdza kompletności zasad ani działania aplikacji.

| Obszar | Status | Uwagi |
|---|---|---|
| Budowanie desktop | Zaliczone w CI | Ubuntu, CMake; budowany tylko silnik/testy, bez SDL frontend |
| Budowanie ARM | Niewykonane | Brak uruchomionego toolchaina |
| Zasady / regresja | Wstępne | 1 test punktacji/pasowania; brak macierzy regresji |
| Interfejs 640×480 | Niewykonane | Jest tylko szkielet SDL |
| Audio | Niewykonane | Brak implementacji audio i legalnych ścieżek |
| OnionOS | Niewykonane | Brak binarki/paczki i urządzenia |
| Długi test / RAM | Niewykonane | Nie przeprowadzono |

## Test sprzętowy

Nie wykonano testu na fizycznym Miyoo Mini Plus. Zgodność pozostaje niezweryfikowana.
