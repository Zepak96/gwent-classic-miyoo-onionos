# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę (w tym Skellige Storm), róg, Bohaterów, Morale, Więź (3 kopie), kolejność Więzi i Morale, pasowanie, remis, Nilfgaard, Szpiega, Medyka (wymóg celu i zakaz wskrzeszania Bohatera), Manekina, Zbiórkę, Mardroeme/Berserker, Scorch, Avengera Cow/Kambiego po Manekinie, Pożodze i końcu rundy, rozdanie i wymianę kart, walidację talii, katalog, 10 talii referencyjnych, wybrane zdolności liderów oraz sprawdzenie, że AI wybiera lidera, gdy jego efekt jest lepszy od zagrania karty. Test AI sprawdza też odporność na zmianę ukrytej ręki przeciwnika i rozgrywa mecz Hard kontra Hard bez przypisanych liderów. Workflow [desktop-build #43712449387](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43712449387) zbudował silnik, testy i frontend SDL2 na Ubuntu, zaliczył 2/2 CTest oraz uruchomił ekran tytułowy, wybór talii i mecz w trybie `SDL_VIDEODRIVER=dummy`. Artefakt `frontend-smoke-frame` z tego przebiegu został obejrzany; potwierdza układ 640×480 w stanie początkowego meczu, ale nie wejście gracza, pełny mecz ani sprzęt Miyoo.

| Obszar | Status | Uwagi |
|---|---|---|
| Budowanie desktop | Zaliczone w CI | Ubuntu, CMake, SDL2; budowany frontend oraz silnik/testy |
| Budowanie ARM | Niewykonane | Brak uruchomionego toolchaina |
| Zasady / regresja | Częściowo zaliczone w CI | Testy jednostkowe istnieją dla wybranych reguł, liderów i AI; kompletne pokrycie gry nadal nie istnieje |
| Interfejs 640×480 | Wstępny smoke zaliczony | Prototyp buduje się, uruchamia i generuje oglądany kadr; brak testu interakcji i pełnej QA wizualnej |
| Audio | Niewykonane | Brak implementacji audio i legalnych ścieżek |
| OnionOS | Niewykonane | Brak binarki/paczki i urządzenia |
| Długi test / RAM | Niewykonane | Nie przeprowadzono |

## Test sprzętowy

Nie wykonano testu na fizycznym Miyoo Mini Plus. Zgodność pozostaje niezweryfikowana.
