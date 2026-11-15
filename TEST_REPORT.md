# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę (w tym Skellige Storm), róg, Bohaterów, Morale, Więź (3 kopie), kolejność Więzi i Morale, pasowanie, remis, Nilfgaard, zdolność pierwszego gracza Scoia'tael, wybór dwóch wskrzeszeń Skellige w rundzie trzeciej, Szpiega, Medyka (wymóg celu i zakaz wskrzeszania Bohatera), Manekina, Zbiórkę, Mardroeme/Berserker, Scorch, Avengera Cow/Kambiego po Manekinie, Pożodze i końcu rundy, rozdanie i wymianę kart, walidację talii, katalog, 10 talii referencyjnych, wybrane zdolności liderów, zgodność decyzji AI przy zmianie ukrytej ręki przeciwnika, początkową wymianę AI oraz pełny mecz AI kontra AI. Dawny test wymuszający wybór lidera usunięto, bo referencyjny `ControllerAI` losuje akcję proporcjonalnie do wag i nie zawsze wybiera najwyżej ważoną możliwość.

Po dodaniu przepływów Scoia'tael i Skellige workflow [desktop-build #43714840961](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43714840961) zbudował silnik oraz frontend SDL2, zaliczył 2/2 CTest w obu konfiguracjach i uruchomił test startowy SDL z atrapą wideo. Testy automatyczne obejmują wybór pierwszego gracza i wybór wskrzeszeń w rundzie trzeciej. Smoke test frontendowy nie symuluje tych wejść ani pełnego meczu; nie wykonano też testu na urządzeniu Miyoo.

Po dodaniu zapisu i wczytania workflow [desktop-build #43716080347](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43716080347) zbudował silnik i frontend, zaliczył testy oraz uruchomił smoke test SDL. Test C++ zapisuje i odczytuje pełny stan meczu, a następnie sprawdza odrzucenie pliku ze zmienioną sumą kontrolną bez naruszenia stanu gry w pamięci. Smoke test nie ćwiczy klawiszy zapisu/wczytania. Test sprzętowy nadal nie jest wykonany: podczas tej sesji Windows nie wykrył nowego dysku, urządzenia przenośnego ani interfejsu USB sieciowego po podłączeniu Miyoo.

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

Nie wykonano testu na fizycznym Miyoo Mini Plus. Zgodność pozostaje niezweryfikowana. Połączenie USB nie udostępniło konsoli w Windows; następną drogą testu będzie karta microSD w czytniku albo dostęp przez sieć OnionOS (np. SSH/SFTP lub Samba), jeśli usługi zostaną włączone.
