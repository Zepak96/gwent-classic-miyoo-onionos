# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę (w tym Skellige Storm), róg, Bohaterów, Morale, Więź (3 kopie), kolejność Więzi i Morale, pasowanie, remis, Nilfgaard, Szpiega, Medyka (wymóg celu i zakaz wskrzeszania Bohatera), Manekina, Zbiórkę, Mardroeme/Berserker, Scorch, Avengera Cow/Kambiego po Manekinie, Pożodze i końcu rundy, rozdanie i wymianę kart, walidację talii, katalog, 10 talii referencyjnych, wybrane zdolności liderów, zgodność decyzji AI przy zmianie ukrytej ręki przeciwnika, początkową wymianę AI oraz pełny mecz AI kontra AI. Dawny test wymuszający wybór lidera usunięto, bo referencyjny `ControllerAI` losuje akcję proporcjonalnie do wag i nie zawsze wybiera najwyżej ważoną możliwość.

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
