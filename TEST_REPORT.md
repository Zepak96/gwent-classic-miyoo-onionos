# Raport testów

Data raportu: 2026-11-15.

## Wynik

Testy automatyczne C++ obejmują punktowanie, pogodę (w tym Skellige Storm), róg, Bohaterów, Morale, Więź (3 kopie), kolejność Więzi i Morale, pasowanie, remis, Nilfgaard, zdolność pierwszego gracza Scoia'tael, wybór dwóch wskrzeszeń Skellige w rundzie trzeciej, Szpiega, Medyka (wymóg celu i zakaz wskrzeszania Bohatera), Manekina, Zbiórkę, Mardroeme/Berserker, Scorch, Avengera Cow/Kambiego po Manekinie, Pożodze i końcu rundy, rozdanie i wymianę kart, walidację talii, katalog, 10 talii referencyjnych, round-trip oraz odrzucenie uszkodzonego przepisu talii użytkownika, wybrane zdolności liderów, zgodność decyzji AI przy zmianie ukrytej ręki przeciwnika, początkową wymianę AI oraz pełny mecz AI kontra AI. Dawny test wymuszający wybór lidera usunięto, bo referencyjny `ControllerAI` losuje akcję proporcjonalnie do wag i nie zawsze wybiera najwyżej ważoną możliwość.

Po dodaniu przepływów Scoia'tael i Skellige workflow [desktop-build #43714840961](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43714840961) zbudował silnik oraz frontend SDL2, zaliczył 2/2 CTest w obu konfiguracjach i uruchomił test startowy SDL z atrapą wideo. Testy automatyczne obejmują wybór pierwszego gracza i wybór wskrzeszeń w rundzie trzeciej. Smoke test frontendowy nie symuluje tych wejść ani pełnego meczu; nie wykonano też testu na urządzeniu Miyoo.

Po dodaniu zapisu i wczytania workflow [desktop-build #43716080347](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43716080347) zbudował silnik i frontend, zaliczył testy oraz uruchomił smoke test SDL. Test C++ zapisuje i odczytuje pełny stan meczu, a następnie sprawdza odrzucenie pliku ze zmienioną sumą kontrolną bez naruszenia stanu gry w pamięci. Smoke test nie ćwiczy klawiszy zapisu/wczytania. Test sprzętowy nadal nie jest wykonany: podczas tej sesji Windows nie wykrył nowego dysku, urządzenia przenośnego ani interfejsu USB sieciowego po podłączeniu Miyoo.

Po dodaniu API talii użytkownika pierwsze uruchomienie CI ujawniło błąd dekodowania pary identyfikatora/liczby kopii. Poprawka została zbudowana i zweryfikowana przez [desktop-build #43716630159](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43716630159): oba zadania (silnik i frontend) zakończyły się powodzeniem, testy CTest przeszły, a frontendowy smoke test SDL zakończył się poprawnie. Test obejmuje poprawny zapis/odczyt talii, materializację jej kart z katalogu oraz odrzucenie uszkodzonego pliku bez nadpisania przepisu w pamięci. Nie obejmuje jeszcze edytora talii w UI ani uruchomienia gry na Miyoo.

Po podłączeniu edytora do menu workflow [desktop-build #43717386389](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43717386389) przeszedł dla silnika i frontendu. CTest zakończył się powodzeniem, smoke test SDL wyrenderował mecz i ekran edycji talii, a workflow zapisał oba obrazy jako artefakt `frontend-smoke-frames`. Ten test sprawdza renderowanie, nie automatyzuje sekwencji przycisków w edytorze ani nie zastępuje testu na fizycznym Miyoo.

| Obszar | Status | Uwagi |
|---|---|---|
| Budowanie desktop | Zaliczone w CI | Ubuntu, CMake, SDL2; budowany frontend oraz silnik/testy |
| Budowanie ARM | Niewykonane | Brak uruchomionego toolchaina |
| Zasady / regresja | Częściowo zaliczone w CI | Testy jednostkowe istnieją dla wybranych reguł, liderów i AI; kompletne pokrycie gry nadal nie istnieje |
| Interfejs 640×480 | Wstępny smoke zaliczony | Prototyp buduje się, uruchamia i generuje oglądany kadr; brak testu interakcji i pełnej QA wizualnej |
| Audio | Częściowo zaliczone desktopowo | `dr_mp3` dekoduje wszystkie 35 MP3; SDL Audio miksuje dźwięki zdarzeń. Brak muzyki w repo źródłowym i testu dźwięku na Miyoo. |
| OnionOS | Niewykonane | Brak binarki/paczki i urządzenia |
| Długi test / RAM | Niewykonane | Nie przeprowadzono |

## Test renderowania assetów i UI

W tej sesji dołączono 531 plików obrazów, 35 SFX MP3 i 6 SVG z przypiętego repo. Test katalogu sprawdza istnienie małej i dużej ilustracji dla każdego z 216 rekordów. Build SDL2 w Dockerze Ubuntu zakończył się powodzeniem, CTest zaliczył 2/2, a smoke test utworzył kadr meczu i edytora z wczytanymi ilustracjami. Oględziny potwierdziły ładowanie plików, ale wykazały, że kompozycja menu/HUD/stołu jest autorskim prototypem i nie odpowiada layoutowi `style.css`; kadry nie są dowodem zgodności 1:1 i wymagają zastąpienia wiernym portem.

## Test sprzętowy

Nie uruchomiono jeszcze portu na fizycznym Miyoo Mini Plus. Po podłączeniu adaptera karta microSD została wykryta jako `E:` z etykietą `MIYOO MINI`, FAT32, z katalogami `Roms`, `Emu`, `Saves` i `Themes`. Do chwili uzyskania wiernego ekranu i binarki ARM karta nie została zmodyfikowana. Samo wykrycie karty nie potwierdza działania gry na urządzeniu.

## Ponowna kontrola zgodności układu źródłowego

Po porównaniu renderera SDL z `style.css` poprawiono położenie liczników/pól rzędów i początek układania kart w środkowym panelu, bazując na oryginalnym podziale 26.5% / 52.5% / 21% i proporcjach rzędów. Dodano też kartę dużego podglądu i panel opisu zdolności w pozycjach źródłowych oraz zaimportowano 44 wpisy nazw/opisów zdolności z przypiętego `abilities.js`. Build desktop SDL2 w Ubuntu 24.04 zakończył się powodzeniem; CTest: 2/2. Smoke test wyrenderował edytor i mecz; obrazy sprawdzono wizualnie. Klatki nadal pokazują odstępstwa od źródła, m.in. nieodtworzone panele pozostałych akcji/karty, oryginalną typografię i zachowanie nakładek. Zmiana geometrii i dodanie podglądu nie są dowodem portu 1:1; zgodność wizualna i sprzętowa pozostaje niezaliczona.

Następnie sześć źródłowych ikon SVG zrasteryzowano do PNG 256×256 przez CairoSVG 2.9.1. Renderer meczu przełączono na PNG dla muzyki, efektów i powiadomień; wizualny smoke test potwierdził widoczne ikony. Build i CTest (2/2) przeszły ponownie. Pozostałe UI i rzeczywiste interakcje nie zostały przez tę kontrolę zaakceptowane jako zgodne 1:1.

Dodano własny dekoder MP3 `dr_mp3` (Unlicense/public domain) oraz odtwarzanie wielokanałowych efektów przez SDL Audio. Desktopowy build zakończył się powodzeniem; CTest 2/2, a `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy gwent --audio-test` pomyślnie zdekodował wszystkie 35 plików. To weryfikuje dekodowanie i otwarcie urządzenia SDL dummy, nie fizyczny głośnik Miyoo. Renderowane klatki edytora i meczu zapisano w `docs/screenshots/current-deck-smoke.png` oraz `docs/screenshots/current-match-smoke.png`; są to zrzuty desktopowego smoke testu, nie test sprzętowy.

Po zgłoszeniu niskiej czytelności dodano fullscreenowy inspektor wybranej karty z dużą grafiką źródłową oraz większym polem nazwy, siły, rzędu i opisu. Zrzut `docs/screenshots/current-card-details-smoke.png` sprawdzono wizualnie w 640×480; opis można odczytać, a tło inspektora jest nieprzezroczyste. Otwieranie: `I` lub przytrzymanie SELECT 500 ms; zamykanie: `B`/ESC. To rozwiązuje odczyt szczegółu wybranej karty, ale nie stanowi jeszcze akceptacji wszystkich napisów, liczników, wyrównań ani animacji względem źródła. Pełny desktop build, CTest, dekoder audio i trzyklatkowy smoke przebiegły poprawnie; testu sprzętowego nadal brak.

Mały panel opisu przeniesiono z jednopikselowego, sztucznie ściśniętego tekstu na czytelniejsze pełnej szerokości glify 5×7 z zawijaniem po 29 znaków; liczba kopii jest centrowana względem ikony znacznika. Dodano lot karty z ręki do rzędu/pogody dla człowieka i AI (500 ms, źródłowy JS czeka 499 ms) oraz smoke klatkę w połowie przejścia. Build, CTest 2/2, dekodowanie 35 MP3 i czteroklatkowy smoke przechodzą. Kaskadowe przejścia karuzeli, fade paneli i pozostałe animacje SDL nadal nie implementuje; liczniki, wyrównania i wszystkie ekrany wymagają dalszej porównawczej korekty.

## Regresja duplikatów pogody

Porównanie z `Weather.addCard` w przypiętym `gwent.js` wykazało, że aktywny może pozostać tylko jeden egzemplarz danego rodzaju karty pogodowej; kolejny trafia do cmentarza bez zmiany aktywnego efektu. `applyWeather` teraz zachowuje tę regułę, a test silnika zagrywa Biting Frost po obu stronach, potwierdza pojedynczy efekt i odrzucenie drugiej karty, a następnie sprawdza usunięcie pogody przez Czyste Niebo. Build SDL2 w Ubuntu 24.04, CTest (2/2) i dekodowanie wszystkich 35 SFX zakończyły się powodzeniem. Zmiana nie stanowi potwierdzenia pełnej zgodności zasad ani testu Miyoo.
