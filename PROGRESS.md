# Postęp projektu

## Aktualny audyt zgodności — 16.11.2026

Publiczna gałąź `codex/full-source-parity` zawiera niezmienione pliki źródłowe
`asundr/gwent-classic` przypięte do `08592d847f2573440a34deb9f32bc855b2544cdd`.
Mapa pełnego audytu źródło→port znajduje się w `docs/SOURCE_PARITY.md`. Port
nadal nie spełnia kryterium 1:1; paczka na SD jest wersją testową, nie
końcowym wydaniem.

- Katalog `data/cards.json` porównuje się teraz rekord po rekordzie z JSON-em
  osadzonym w upstreamowym `cards.js`: 216/216 zgodnych, pola i kolejność bez
  różnic. Ten test potwierdza zawartość katalogu, nie kompletność implementacji
  wszystkich zdolności.
- W 640×480 podglądach karty tekst oryginalnego obrazka jest oddzielony od
  portretu; nazwy i opisy są rysowane jako tekst PL/EN. Klatki inspektora,
  edytora i karuzeli obejrzano po wygenerowaniu przez SDL dummy smoke.
- Release CTest przechodzi 2/2, `--input-test` przechodzi, a `--audio-test`
  dekoduje 35 SFX i oba utwory. Test audio z SDL dummy nie potwierdza głośnika
  fizycznego Miyoo.
- Zmiany źródłowe i README są opublikowane na publicznej gałęzi; ostatnie
  poprawki obejmują źródłowy AI mulligan, 100-meczową regresję, dźwięk
  przemienionego Vildkaarla i polskie statusy meczu. Nowy ARM
  hard-float build oraz paczka OnionOS przeszły lokalnie; paczkę zainstalowano
  na karcie `E:` w `Roms/PORTS/Games/GwentClassic`, ale nie uruchomiono jej
  jeszcze na urządzeniu.
- Skrypt builda OnionOS jest uruchamialny z checkoutu Windows dzięki regule LF
  dla skryptów `.sh`. Paczka `GwentClassic-OnionOS.zip` zawiera 618 wpisów,
  SHA-256 `52cfe7b8fe18b196c8df6b516ee6eb98b4929405c6aed373856b7bd7f278045e`;
  `readelf` potwierdził ELF32 ARM i hard-float ABI. Lokalna paczka leży w
  `.work/miyoo-b9a67e8/` i nie jest jeszcze wydaniem GitHub Release.
- Po instalacji zweryfikowano 602/602 plików gry porównaniem SHA-256;
  skrót binarki na SD to
  `9b87b32b9c05d52fa6feafffe1b1d7cbad68ab8d23bf0668a09c7c37031b2a12`.
  Kopie poprzednich katalogów gry zachowano w
  `.work/sd-backup-7fca1da/GwentClassic`, `.work/sd-backup-b04f2d0/GwentClassic`
  i `.work/sd-backup-b9a67e8/GwentClassic`;
  istniejący skrót `.port` OnionOS
  nie był zmieniany.
- Poprawiono błąd mulliganu AI: źródłowe `ControllerAI.redraw()` podejmuje dwie
  kolejne decyzje, każdorazowo po natychmiastowej wymianie z wierzchem talii;
  wcześniej druga decyzja mogła wskazywać kartę już dobraną. Teraz port wykonuje
  dwie rzeczywiste podmiany w kolejności źródłowej. CTest przechodzi 2/2, w tym
  100 deterministycznych pełnych meczów z taliami i liderami źródłowymi.
- Przemienione karty Vildkaarl z tagiem `vildkarrl` odtwarzają teraz źródłowy
  efekt dźwiękowy Mardroeme zamiast standardowego dźwięku rzędu. Desktop build,
  CTest 2/2, test sterowania i dekodowanie wszystkich 37 plików audio przechodzą.
- Rozdzielono dźwięki zagrania i zdolności zgodnie z `Row.playPlacementAudio()`,
  `Weather.addCard()` oraz `Card.animate()` w źródle. Port stosuje dźwięki
  `hero`/`common_*`, wyjątki dla szpiegów, Decoya i Berserkera pod Mardroeme,
  tagi dźwięków pogody oraz osobne zaplanowane SFX animacji Horn/Spy/Morale,
  transformacji Mardroeme i wyzwolonych efektów Medic/Bond/Muster. Build i CTest
  przechodzą; fizyczny odsłuch i synchronizacja na Miyoo nadal niezweryfikowane.
- Commit `c8fb50d` zbudowano jako ELF32 ARM hard-float, a archiwum OnionOS
  zawiera 618 wpisów (SHA-256
  `ddd30aa2730122f7e2b88152a1bdd17551722938c0a3b31d0cf98fe2dc9b4dc0`).
  Zaktualizowano `E:\Roms\PORTS\Games\GwentClassic`; audyt SHA-256 potwierdził
  602/602 pliki. Hash binarki: `ff33832ddad11d9b77b26e5b3562a92d4fc160949b889994333c78419ab12ee9`.
  Poprzednie katalogi zachowano na SD i w `.work/sd-backup-c8fb50d/`; uruchomienie
  i odsłuch na urządzeniu wciąż wymagają testu fizycznego.
- Rozszerzono testy zdolności liderów z repo referencyjnego o oczyszczenie
  pogody, Torrential Rain, ranged/melee Scorch, Biting Frost, Ranged Horn,
  Hope of the Aen Seidhe i Invader of the North. Pełne pokrycie wszystkich
  zdolności i wyborów lidera pozostaje otwarte.
- Dodano regresję aktywacji Eredina Dowódcy w rzędzie wręczowym oraz odrzucenia
  aktywacji, gdy slot specjalny jest już zajęty.
- Zweryfikowano też szczególną inicjalizację źródłowego Emhyra Białego Płomienia:
  wyłącza ona pasywne zdolności obu liderów, w tym dodatkową kartę Franceski,
  połowę pogody Brana, losowe wskrzeszanie Invadera i podwójne punkty szpiegów
  Zdrajcy. Cztery regresje potwierdzają brak tych efektów przy Białym Płomieniu.
- Tokeny Avengera tworzone przez Krowę/Kambiego są oznaczane prefiksem
  `summoned_`; po zniszczeniu nie pozostają w trwałym cmentarzu portu. Źródło
  usuwa Bovine Defense Force/Hemdalla z cmentarza po 1001 ms, zanim gracz może
  wykonać kolejną akcję, dlatego silnik rozstrzyga sprzątanie atomowo. Regresja
  Cow → Bovine Defense Force → Scorch potwierdza, że token nie może zostać
  wskrzeszony przez Medyka. CTest przechodzi 2/2.
- Porównanie wag Mardroeme AI z `ControllerAI.weightMardroemeRow()` nie
  wykazało błędu w istniejącym wzorze. Dodano deterministyczny test, który przy
  dwóch Berserkerach w wręcz i dwóch Młodych Berserkerach w dystansowym wybiera
  dystansowy zgodnie z wagami `10*n` oraz `8*n²-2*n`; CTest przechodzi 2/2.
- Aktualny commit `bd1e346` zbudowano jako ELF32 ARM hard-float; archiwum OnionOS
  ma 618 wpisów i SHA-256
  `6a55d9a0a2e58206f91ae01ec1797bb711f36585954cc66be49365493000af49`.
  Zainstalowano je w `E:\Roms\PORTS\Games\GwentClassic`; audyt porównał
  SHA-256 wszystkich 602/602 plików. Hash binarki:
  `52b8c386502084492648974af8aeff67d16525601caebe73f55c8254fb3a1542`.
  Poprzedni katalog gry zachowano na SD i w `.work/sd-backup-bd1e346/`.
- Przetłumaczono statusy polskiej wersji dla błędów i akcji meczu (zapis,
  mulligan, pas, cel, lider, Skellige oraz odrzucony ruch AI). CTest 2/2,
  `--input-test` i `--audio-test assets` przechodzą; pełna lokalizacja menu,
  podpowiedzi i kart pozostaje otwarta.

- Naprawiono mapowanie własnych maksimów w wyborze celu AI dla Manekina:
  wariant źródłowy losuje spośród maksimum każdego własnego rzędu, nie tylko
  globalnego maksimum planszy.
- Usunięto odzyskiwanie błędnego ruchu AI przez automatyczne pasowanie. Odrzucony
  ruch zostawia stan tury bez zmian i pokazuje diagnostykę zamiast fałszować
  zachowanie oryginalnego przeciwnika.
- Medyk wskrzeszający kartę Zręczną wymaga teraz wskazania rzędu wręcz albo
  dystansowego; wejście SDL pozwala przełączać rząd góra/dół przed zatwierdzeniem.
  Test sprawdza również odrzucenie niekompletnego wyboru bez częściowej zmiany
  stanu.
- Zachowano oddzielnie efekt Mardroeme oraz zajętość slotu specjalnego dla
  każdego rzędu. Mardroeme z Ermiona nie zajmuje slotu; specjalna karta
  Mardroeme i zdolności Rogu zajmują go. Berserker zagrany, przywołany Zbiórką
  lub obecny przy aktywacji jest transformowany. Zapis ma wersję 4 i czyta
  starsze formaty 2 oraz 3.
- Weryfikacja lokalna w kontenerze: kompilacja SDL2, CTest 2/2 oraz
  `gwent --input-test` z SDL dummy. Nie jest to test urządzenia Miyoo; nie
  zbudowano ani nie wgrano nowej paczki ARM/SD w tej gałęzi.

AI jest teraz porównywany do pojedynczego `ControllerAI` z przypiętego źródła;
trzy poziomy trudności opisane w starszym wpisie są rozbieżnością względem
aktualnie przyjętego wymagania 1:1 i zostały usunięte z tej gałęzi.

## Status: aktywna implementacja — 15.11.2026

Repozytorium wejściowe było puste. Powstał szkielet C++17/CMake, model silnika i prosty frontend SDL2. To nie jest ukończona gra ani wydanie dla Miyoo.

### Zakres wersji przyjęty po doprecyzowaniu

Docelowa gra to wyłącznie Gwint jako minigra w Wiedźminie 3: Wild Hunt, z zawartością podstawki, Hearts of Stone, Blood and Wine oraz zmianami/kartami dodanymi przez aktualizację next-gen Wiedźmina 3. Nie implementujemy samodzielnej gry `GWENT: The Witcher Card Game` ani jej późniejszych zasad. Przypięte repozytorium Gwent Classic jest źródłem docelowego zachowania i zakresu portu. Celem jest przeniesienie tej gry 1:1 na Miyoo Mini Plus, a nie samodzielne korygowanie jej według innej wersji Gwinta. Różnice względem gry referencyjnej należy traktować jako braki portu.

### Etap 1 — audyt

- [x] Przejrzano źródła `asundr/gwent-classic`, commit `08592d847f2573440a34deb9f32bc855b2544cdd`.
- [x] Zebrano informacje o `ia2904/gwent-classic-v3.5`, commit `2efc94ed626c0baeb28fc5e51c0e279ed5ee51dd`.
- [x] Ustalono, że upstreamowy kod ma licencję MIT z Commons Clause; zaimportowano katalog danych kart/talii z zachowaniem licencji i manifestu.
- [x] Po zgłoszeniu przez właścicielkę projektu zgody przedstawiciela CD PROJEKT RED zaimportowano przypięte grafiki, ikony i SFX z repo źródłowego wraz z pochodzeniem i atrybucją.
- [x] Sprawdzono dokumentację OnionOS Ports Collection oraz SDL2 dla Miyoo; pakowanie wymaga launcherów i bibliotek zgodnych z docelowym wydaniem OnionOS.
- [x] Zidentyfikowano wariant next-gen Wiedźmina 3 (m.in. Roach i Muster na Geralt/Ciri); został przyjęty jako docelowy po doprecyzowaniu użytkownika.
- [x] Utworzono publiczne repozytorium GitHub i dodano workflow CI przez integrację GitHub.
- [x] Zaimportowano 216 rekordów katalogu kart z przypiętego repozytorium referencyjnego wraz z upstreamową licencją i manifestem pochodzenia.

### Etap 2 — silnik

- [~] Model danych kart i graczy; podstawowe punktowanie z pogodą, rogiem, Morale, Więzią i immunitetem Bohaterów.
- [~] Szpieg, Medyk, Manekin, Zbiórka, Pożoga, pasowanie, remis i przejście rundy. Podstawowe zdolności frakcji są przeniesione z `factions.js`: dobór Królestw Północy, wygrana Nilfgaardu przy remisie, losowy oddział Potworów pozostający po rundzie, wybór pierwszego gracza przez ludzkiego Scoia'tael oraz losowy wybór do dwóch jednostek z cmentarza Skellige na początku trzeciej rundy; gracz wybiera rząd tylko wtedy, gdy wskrzeszona karta jest Zwinna. Ostatni przypadek poprawiono w commicie `95fd156`; dalsza weryfikacja interakcji nadal trwa.
- [~] Rozdanie 10 kart z deterministycznie tasowanej talii oraz jednorazowa wymiana do 2 kart dla każdego gracza.
- [~] Zaimportowany katalog 216 rekordów; generator kompiluje go jako 258 instancji/rekordów do natywnego builda.
- [~] Zaimportowano 10 referencyjnych talii przeciwników wraz z liderami i listami kart; budowanie talii z tych przepisów jest spięte z katalogiem.
- [~] Dodatkowe zdolności z danych: row-specific Scorch, Mardroeme/Berserker, Dandelion, Avenger (Cow/Kambi), kompletne wielorzędowe efekty Skellige Storm oraz 22 liderów w katalogu.
- [~] Natywna obsługa liderów obejmuje pasywy oraz aktywacje pogody z talii, rogów, row Scorch, doboru/odrzucania i przenoszenia kart. Testy CI sprawdzają wybrane zdolności, ale komplet wszystkich 22 liderów i zgodność wszystkich interakcji z referencją pozostają do zrobienia.
- [x] CI kompiluje wygenerowany katalog i zalicza walidację wszystkich 10 talii z leaderami.
- [ ] Wczytywanie kart z katalogu w menu, pełny zestaw zdolności i wszystkich liderów.
- [~] Rozdanie 10 kart z deterministycznie tasowanej talii oraz jednorazowa wymiana do 2 kart dla każdego gracza.
- [~] Kolejność efektów Więzi i Morale oraz podwajanie Szpiegów jest porównana z kodem referencyjnym i objęta testami; weryfikacja pozostałych interakcji nadal trwa.
- [ ] Pełny katalog kart zweryfikowany z ustalonym profilem wersji.
- [ ] Wszystkie zdolności jednostek, dowódców i frakcji oraz prawidłowa kolejność ich rozstrzygania.
- [ ] Pełna walidacja talii, mulligan, medyk, manekin i zapis odrzuconych kart.
- [~] AI ma teraz poziomy Łatwy/Normalny/Trudny wybierane na ekranie talii i zapamiętywane w ustawieniach. Łatwy losuje wyłącznie ruchy legalne według silnika (z możliwością błędnego pasowania), Normalny zachowuje ważony wybór `ControllerAI` z repozytorium, a Trudny bierze legalny ruch o najwyższej wartości tej samej heurystyki. Mulligan chroni Roacha, gdy w ręce jest Geralt/Cirilla z odpowiadającą mu Zbiórką. Testy pełnych meczów AI kontra AI przechodzą dla wszystkich poziomów; trudny nie ma jeszcze wielorundowego przeszukiwania ani osobnej heurystyki planowania zasobów.
- [ ] Porównawcza weryfikacja wszystkich sekwencji względem źródła referencyjnego.

### Etapy 3–8 — port, UI, lokalizacja, audio, integracja, QA i release

- [~] Wierny interfejs referencyjny jest w trakcie portowania: ekran startowy edycji talii, proporcje trzech paneli meczu, rozmieszczenie rzędów oraz podgląd dużej karty z nazwą/opisem zdolności odwzorowują część układu `index.html`/`style.css`. Edytor i mecz nadal mają różnice wizualne i brakujące elementy: oryginalny HUD/stany menu, karuzela, pop-upy, powiadomienia, wynik, typografia i część ikon. Mapa elementów i skalowania jest w `docs/REFERENCE_UI_PORT.md`; obecny stan nie jest zgodny 1:1.
- [~] Prototyp silnika/UI posiada mecz, mulligan, cele Manekina/Medyków, lidera i pasowanie, ale kod nie jest kompletnym portem zachowania referencyjnego; zachowane różnice należy weryfikować jako braki względem `gwent.js` i plików składowych.
- [~] Zapis i odtwarzanie stanu meczu: dodano wersjonowany, sumowany kontrolnie format binarny, atomowy zapis oraz bezpieczne odrzucanie uszkodzonych plików; UI ma ręczny zapis/wczytanie i wznowienie z menu. Autosave i test na Miyoo pozostają do zrobienia; przepis talii ma już zapis silnikowy, ale nie jest jeszcze podłączony do UI.
- [~] Dodano format talii użytkownika `.gwd` i podłączono go do SDL: trzy sloty, edycja puli frakcji/neutrali, lidera i kopii, walidacja, zapis atomowy z sumą kontrolną oraz użycie talii przy starcie meczu. Silnik testuje round-trip i odrzucanie uszkodzeń. CI #43717386389 zbudował frontend, zaliczył CTest i wyrenderował ekrany edytora i meczu; brakuje zmiany nazwy talii, automatycznej QA sekwencji przycisków i testu na urządzeniu.
- [~] Edytor talii jest wstępnie podłączony w PL/EN; pełne PL/EN całego UI, tutorial i ustawienia nadal do zrobienia.
- [~] Dodano tryb inspekcji zaznaczonej karty (`I`/przytrzymanie SELECT przez 500 ms) z powiększoną oryginalną grafiką i większym opisem. To ułatwienie odczytu dla 640×480; nie zastępuje pełnej korekty typografii, wyrównania liczb, geometrii wszystkich okien ani animacji źródłowych.
- [~] Poprawiono czytelność inspektora: smoke pokazuje pełnowymiarową Cirillę z widocznym cytatem na karcie oraz pełnym opisem Muster obok. W zwykłym podglądzie meczu zasady są teraz większe (2×); długie opisy pokazują cztery wiersze i wskazówkę otwarcia pełnego tekstu klawiszem `I`. Lokalny build SDL2, CTest 2/2 i smoke działają; test fizyczny oraz dalsze porównanie wszystkich napisów i medalionów nadal otwarte.
- [~] Przeniesiono podstawowy lot zagrywanej karty z ręki do rzędu/pogody w 500 ms dla gracza i AI. Audyt CSS/JS wskazał też kaskadowe przesunięcia karuzeli i przejścia/fade; te animacje oraz zmiany układu są nadal niezaimplementowane.
- [~] Przypięte grafiki kart, plansza, ikony SVG i SFX z repo są w `assets/gwent-classic/`; sześć SVG ma rastrowe odpowiedniki PNG dla SDL. Dodano dekodowanie 35 efektów MP3 oraz zapętlane tło menu i meczu z dwóch ścieżek dostarczonych przez właścicielkę projektu. Lokalny smoke dekoduje efekty i muzykę oraz przełącza odtwarzanie; fizyczny test audio na Miyoo pozostaje otwarty.
- [x] Zgodność pogody: ponowne zagranie już aktywnej karty pogodowej odrzuca duplikat na cmentarz właściciela bez ponownego aktywowania efektu. Dodano regresję dla duplikatu Biting Frost i późniejszego Czystego Nieba; lokalne CTest 2/2 oraz dekodowanie 35 MP3 przechodzą.
- [~] Zbudowano frontend armhf oficjalnym toolchainem Miyoo GCC 8.2.1 i przypiętym SDL2. Pakiet dla commita `1df0c37` zawiera poprawkę mulliganu i dwa zapętlane utwory (menu/mecz); workflow desktop #43753875815 i ARM #43753875880 zakończyły się sukcesem. Zainstalowano na karcie SD `E:`. Audyt SHA-256 porównał 602/602 pliki z katalogu gry bez różnic; istniejący skrót `.port` pozostał zachowany. Hash binarki: `dda094d69a0d2afea52f5bb19a8ba4e2159a917550b299ae4b19e057cdadce5f`. Fizyczne uruchomienie, wejście i audio na Miyoo nadal czekają na test użytkowniczki.
- [~] Desktop/ARM build i smoke; regresja silnika istnieje dla wybranych reguł. Pełne pokrycie, pomiar zasobów i test uruchomienia na sprzęcie nadal otwarte.

## Rozszerzenie po ukończeniu wiernego silnika — kampania arcade

Użytkownik chce później rozbudować grę o lekką warstwę fabularną. Ten zakres jest celowo po pełnej implementacji pojedynczego meczu, żeby każdy przeciwnik używał tych samych sprawdzonych zasad.

- [ ] Mapa/drzewko kolejnych przeciwników inspirowanych postaciami z Wiedźmina 3.
- [ ] Rosnąca trudność przeciwników i talii między pojedynkami.
- [ ] Finałowy przeciwnik/boss oraz zakończenie małej kampanii.
- [ ] Menu kampanii i osobny zapis jej postępu.
- [ ] Po wydaniu bieżącej wersji odświeżyć menu główne w kierunku atmosfery z referencyjnego zrzutu ekranu dostarczonego przez użytkowniczkę.
- [ ] Kilka wariantów plansz/stolików z legalną, oryginalną oprawą graficzną.
- [ ] Opracować krótkie wprowadzenie fabularne oraz teksty PL/EN, bez naruszania licencji tekstów i assetów gry.

Nie zmienia to bieżącego celu: najpierw kompletna rozgrywka Gwinta z Wiedźmina 3 next-gen przeciw AI oraz stabilny port OnionOS.

## Znane blokady środowiska

- Lokalnie kompilator C++ działa w Dockerze Ubuntu; build desktop i ARM wykonują też workflow GitHub Actions.
- Karta OnionOS jest widoczna w Windows jako `E:` (`MIYOO MINI`, FAT32); aktualna paczka armhf jest na karcie, ale nie została jeszcze uruchomiona na Miyoo.
- Urządzenie/karta nie udostępnia z Windows kanału do automatycznego sterowania konsolą; fizyczny test startu, przycisków, audio i wydajności pozostaje niewykonany.
- Workflow GitHub Actions publikuje wynik kompilacji/testów przy każdym pushu.

## Zasada raportowania

Funkcja pozostaje nieukończona, dopóki nie ma implementacji i testu. Kompatybilność urządzenia nie będzie oznaczana jako potwierdzona bez testu fizycznego.
- [~] Po ponownym zgłoszeniu czytelności powiększono podgląd kart na stole i grafikę w inspektorze (w tym cytat Cirilli), zmieniono kolor oraz centrowanie liczb siły i zwiększono kontrast panelu opisu. Lokalny frontend SDL zbudował się i wygenerował cztery klatki smoke 640×480; fizyczny test Miyoo pozostaje otwarty.

## Renderer i mapowanie fizycznego wejścia — kontynuacja 15.11.2026

- Renderer Mini zgłasza nazwę `Miyoo Mini`; jego SDL wymaga kopiowania tekstur i nie wykonuje rysowania prymitywów. Frontend składa teraz całą scenę w SDL software 640×480, a renderer sprzętowy otrzymuje jedną teksturę. Oversized textures są skalowane przed utworzeniem, a cache ma limit 48 tekstur.
- `SDL_event_mini.c` odpytuje `/dev/input/event0` i mapuje linuksowe klawisze na SDL keyboard events. Wdrożono translację A=Left Ctrl, B=Left Alt, X=Left Shift, Y=Space, Start=Return, Select=Escape, L=Tab, R=Backspace, L2/R2=PageDown/PageUp. Dodano `patches/miyoo-sdl-keyhome.patch`, zbudowano przypięty SDL Mini z mapowaniem MENU=`KEY_HOME`→`SDLK_HOME` i dołączono patch do paczki.
- Dodano ekran pauzy z kontynuacją, ręcznym save/load, poziomem AI, wyborem PL/EN, przełącznikiem SFX i powrotem do menu talii. Select wymaga potwierdzenia przed pasowaniem; przytrzymanie Select otwiera inspektor. Zdarzenie Home zapisuje mecz i kończy proces. Sterowanie fizyczne nadal wymaga testu na konsoli.
- Lokalny build SDL2 i CTest (2/2) przeszły; smoke dekoduje 35/35 SFX, sprawdza translację fizycznych klawiszy, tryb pauzy, potwierdzanie pasowania i ochronę aktywnego wyboru celu. Wygenerowano oraz obejrzano klatki w trybie compositingu 640×480. Pełny cross-build i paczka ARM przechodzą w tym przebiegu; nową paczkę należy jeszcze skopiować i porównać z kartą.
- Weryfikacja na urządzeniu nadal otwarta: fizyczne przyciski, start, dźwięk, RAM, stabilność i prawdziwa wydajność nie są potwierdzone. Referencyjny frontend 1:1, tutorial, pełne PL/EN i kompletna zgodność reguł również pozostają do wdrożenia. Rozbudowane menu i kampania arcade są zaplanowane po wydaniu tej wersji.

## Regresja punktowania Więzi — 15.11.2026

- Audyt wykazał błąd przeniesiony do testu: silnik mnożył siłę przez liczbę kopii i porównywał metadane `group`, a nie nazwę karty. Referencyjny opis zdolności mówi o podwojeniu siły obu kart przy co najmniej dwóch kartach o tej samej nazwie.
- Punktowanie zmieniono na mnożnik ×2 niezależny od liczby nadmiarowych kopii; dopasowanie odbywa się po nazwie karty. Test korzysta z prawdziwego katalogowego „Blue Stripes Commando”: 3 egzemplarze mają razem 24 punkty (3 × 4 × 2), różne nazwy ze wspólną grupą nie łączą się, a ta sama nazwa działa przy różnych grupach metadanych. Kolejność Więzi przed Morale pozostaje pokryta osobnym przypadkiem.
- Desktop build, CTest (2/2), smoke translacji wejścia, dekodowanie 35 SFX, cross-build zmodyfikowanego SDL i aplikacji ARM hard-float zakończone powodzeniem. Paczka SHA-256 `08c7643b372f57e46e389712ed0677888aa3734c5408605777c24cbf57a5bce4` została skopiowana na kartę `E:`; 599/599 plików zgodnych, skrót OnionOS zachowany, hash gry `f37d866471e5f8d66ba2724dd290bea24fca48fe479eae1d7e5b990365afb5fb`, hash SDL `8eebe8969a25997265b35175c132fc72e66b16615a096af1cf71ec8d91db2053`.
- To naprawia jeden rozpoznany błąd zasad. Pełne porównanie wszystkich efektów, sekwencji i rozszerzeń next-gen oraz test fizyczny nadal są otwarte.

## Animacje komunikatów z repo źródłowego — 15.11.2026

- Przeniesiono cykl powiadomień z `ui.notification()` wraz z oryginalnymi ikonami `notif_*.png`: kolejka zdarzeń, baner na środku planszy oraz 150 ms wygaszania, 900 ms ekspozycji i 150 ms zanikania (łącznie 1200 ms).
- Podłączono komunikat rzutu monetą, początek rundy, zmianę tury, spasowanie i wynik rundy; dodano tekst PL/EN oraz zapamiętywany przełącznik powiadomień.
- Desktop build, CTest (2/2), `--input-test` oraz SDL smoke test przeszły; kadr komunikatu sprawdzono wizualnie. Nie budowano ARM i nie zapisywano tej zmiany na SD.
- To nadal częściowy port animacji. Karuzela kart, fade podglądu i ekranu końca gry, dokładna kolejność wszystkich animacji oraz test fizyczny pozostają do zrobienia.

## Animacja ekranu wyniku — 15.11.2026

- Zgodnie z `showEndScreen()` w źródłowym `gwent.js` ekran końca rundy/meczu pojawia się przez fade-in 300 ms. Przeniesiono animację na przyciemnienie planszy, grafikę wyniku i tekst; czas startuje przy wejściu w fazę wyniku i przy wczytaniu zapisu z takim ekranem.
- Dodano test alpha dla 0/150/300/600 ms oraz osobny kadr smoke w połowie przejścia. Build desktop, CTest (2/2), `--input-test` i pełny SDL smoke przechodzą; kadr sprawdzono wizualnie.
- Pozostałe animacje źródłowe nadal są otwarte; animacja wyniku nie zamyka audytu 1:1.

## Krzywa skalowania nakładek zdolności — 15.11.2026

- Porównanie `Card.animate()` z `.card > div:nth-child(4)` wykazało, że SDL zmieniał rozmiar grafiki skokowo. Dodano źródłową krzywą CSS `ease` i przejścia tła 250 ms: 40→100%, 100→80% i 80→40%, przy zachowaniu fade-in/out oraz hold.
- `--input-test` sprawdza granice faz i wartości skali; smoke generuje klatkę Morale w trakcie pierwszego przejścia. Desktop build, CTest (2/2), `--input-test`, pełny smoke, ARM GCC 8.2 hard-float i ZIP OnionOS przeszły. ZIP SHA-256: `e9b8fbebbc730a58fa94e87e41caebdb6bc7bdedb235db9af6d9dfd78cca1046`. Nie skopiowano go na SD ani nie testowano na urządzeniu.

## Układ rzędów i animacja odstępów — 15.11.2026

- Wyrównano pozycje kart w rzędzie do `row-cards` w źródłowym CSS: karty są centrowane, a odstępy/optyczne nakładanie wynikają ze źródłowej szerokości kart i `resizeCardContainer(10, .075, .00325)`.
- Po zmianie liczby kart pozycje i odstępy interpolują przez 250 ms krzywą CSS `ease` odpowiadającą przejściu marginesów w źródle. Lot karty i nakładki zdolności korzystają z nowych docelowych pozycji rzędów.
- Test wejścia sprawdza pozycję dla 1/9 kart, zagęszczanie przy 10/20 oraz interpolację 250 ms; smoke renderuje 10 kart w połowie przejścia. Desktop build, CTest (2/2), `--input-test` i pełny smoke przeszły. Cross-build GCC 8.2 zweryfikował ELF32 ARM hard-float; ZIP OnionOS przeszedł `unzip -t`, SHA-256 `3dc8fc01a09a3c6d418f57fbac3862d595392ede2d57574040c15a2f6b646f44`; hash binarki w ZIP zgadza się z ARM buildem (`a0d2d056205cb0ba380365317f5400656797b347606c0254f2a7f7ca1196f8d3`). Nie wgrywano na SD.

## Animacje z repozytorium źródłowego — 15.11.2026

Przeniesiono do SDL wybrane istniejące animacje źródłowe: lot karty trwa 499 ms, oryginalne grafiki `anim_*.png` są używane dla efektów kart, a nakładka pogody pojawia się i znika w 500 ms. Szczegóły timingów i lista efektów nadal wymagających portu są w `docs/REFERENCE_ANIMATIONS.md`. To jest częściowy postęp, nie ukończony port animacji 1:1. Desktop build, CTest 2/2 i test wejścia SDL przeszły; ta zmiana nie była cross-buildowana dla Miyoo, kopiowana na kartę SD ani sprawdzana na konsoli.

## Losowanie przeciwnika zgodne ze źródłem — 15.11.2026

Źródłowy `DeckMaker.constructOpponentDeck()` wybiera losową talię z puli startowej, a potem losowego lidera należącego do frakcji tej talii. Port nie wybiera już deterministycznie „następnej” talii. Dodano `makeReferenceOpponentDeck(seed)` z lokalnym, odtwarzalnym PRNG oraz test 500 ziaren obejmujący wszystkie dziesięć talii, legalność liderów i poprawność składów. Build desktop i CTest 2/2 przeszły; kompilacja ARM i test Miyoo pozostają otwarte.

## Automatyczne pasowanie zgodne ze źródłem — 15.11.2026

Przeniesiono reguły `Player.canPlay()`, `Player.endTurn()` i `Game.startRound()`: gracz bez kart i dostępnej zdolności lidera pasuje po swojej akcji, a przed rozpoczęciem rundy automatycznie pasują strony, które nie mogą wykonać ruchu. Zdolność lidera użyta wcześniej w meczu nie odnawia się na początku kolejnej rundy. Dodano testy ostatniej karty i pustego rozpoczęcia; desktop build i CTest 2/2 przeszły.

## Uzupełnienie celów animacji kart — 15.11.2026

Porównanie z `abilities.js` i `Card.animate()` wykazało, że nakładki muszą pojawiać się na kartach dotkniętych zdolnością, a nie wyłącznie na karcie zagranej. Renderer obsługuje teraz nakładkę Więzi na wszystkie pasujące karty w rzędzie, Muster na każdej nowo przywołanej kopii, Medyka na wskrzeszonej jednostce oraz Pożogę równolegle na jednostkach usuwanych z planszy, zachowując ich pozycje sprzed zmiany stanu. Build, CTest (2/2) i `--input-test` przechodzą. To nadal częściowy port animacji; pełne przejścia, kolejność ruchu, karuzela, komunikaty i ekrany końca gry są otwarte. Nie robiono builda ARM ani zapisu na SD.

## Lokalizacja katalogu kart PL/EN — 15.11.2026

Generator katalogu kompiluje teraz osobne nazwy kart oraz tytuły/opisy zdolności po polsku i angielsku; UI wybiera tekst według ustawienia języka, a wybrane nazwy są nakładane nad oryginalną ilustracją w podglądzie i inspektorze. Dodano polskie glify bitmapowe i kontrolę, że każda nazwa katalogowa oraz każda zdolność źródłowa ma wpis PL. Test sprawdza Geralta w obu językach i obecność tłumaczeń dla wszystkich 258 instancji kart. Desktop build, CTest (2/2), `--input-test` oraz zrzut smoke 640×480 przeszły. Lokalizacja statycznych komunikatów UI nie jest jeszcze pełna, a tłumaczenia nazw wymagają weryfikacji względem oficjalnej polskiej wersji gry; parytet i build ARM pozostają otwarte.

## Start z ekranem DeckMaker jak w źródle — 15.11.2026

Pin `gwent.js` kończy inicjalizację przez `let dm = new DeckMaker()`, bez ekranu tytułowego. SDL uruchamia się teraz od razu w edytorze talii; usunięto nieźródłowy ekran tytułowy, a powrót z pomocniczego selektora talii prowadzi z powrotem do edytora. Test wejścia sprawdza domyślny ekran, a smoke przetestował render DeckMakera oraz powrót selektora. Build, CTest (2/2), `--input-test` i pełny smoke z sześcioma zrzutami zakończyły się poprawnie. Edytor nie odtwarza jeszcze całej geometrii i wszystkich operacji DeckMakera, dlatego pozycja parytetu pozostaje otwarta. Bez cross-builda i zapisu na SD.

## Przepływ zdolności Eredina — 16.11.2026

- Zgodnie z kodem źródłowym, Eredin odrzuca do dwóch kart z ręki (zależnie od liczby dostępnych), a następnie pokazuje wybór jednej karty z talii z możliwością pominięcia dobierania. Po rozpoczęciu zdolności anulowanie wyboru odrzucenia jest zablokowane, a anulowanie wyboru dobierania kończy zdolność bez pobrania karty.
- Uzupełniono przypadki: ręka z jedną kartą, pusta ręka i wybór tylko dobierania, pusta talia i samo odrzucanie oraz całkowicie pusta ręka i talia. Dodano regresje silnika i testu wejścia SDL.
- Desktop CTest (2/2), `--input-test` i zrzuty smoke 640×480 przechodzą. Fizyczne testy na Miyoo nadal są otwarte.

## Animacja wskrzeszenia Medyka — 16.11.2026

- Nakładka `anim_medic` pojawia się raz na wskrzeszanej karcie w cmentarzu; usunięto błędną drugą nakładkę z zagrywanej karty Medyka.
- Lot cmentarz → rząd zaczyna się dokładnie po sekwencji `Card.animate()` trwającej 1650 ms. Test SDL sprawdza pozycję w cmentarzu, jeden efekt oraz granicę nakładka/lot.
- CTest (2/2) i `--input-test` przechodzą. Inne animacje źródłowe i test sprzętowy pozostają otwarte.

## Boardowe liczenie kart przez AI — 16.11.2026

- `ControllerAI.getBoardData()` zlicza Szpiegów, Medyków i karty Pożogi ze wszystkich sześciu rzędów. Natywne AI uwzględniało tylko swoje rzędy, co zmieniało wagę Manekina, gdy na planszy przeciwnika leżał Szpieg, Medyk lub Pożoga.
- Wagi obejmują teraz obie strony; wybór celu Manekina nadal wskazuje legalną kartę po stronie AI. Test 256 deterministycznych decyzji potwierdza wzrost prawdopodobieństwa zagrania Manekina przy wrogim Szpiegu.
- CTest (2/2) przechodzi. Pełny rozkład wag i fizyczny test urządzenia pozostają otwarte.

## Korekta: wagi Manekina AI — 16.11.2026

- Ponowna kontrola przypiętego `gwent.js` wykazała, że `ControllerAI.getBoardData()` liczy zdolności z `board.row[0..2]`, czyli trzech rzędów kontrolowanych przez AI. Poprzednia notatka o wszystkich sześciu rzędach była niezgodna ze źródłem.
- Waga Manekina używa własnych list AI Szpiegów, Medyków i Pożogi; przeciwnik nie zmienia tej wagi. Test 256 decyzji z tymi samymi ziarnami sprawdza równość wyników z i bez wrogiego Szpiega. Dodatkowo regresja weryfikuje wybór własnego Szpiega, kiedy po obu stronach leżą Szpiedzy.
- Desktop CTest: 2/2 zaliczone; `--input-test` zaliczył sterowanie Miyoo, blokadę akcji, podgląd/lot kart, efekty zdolności, mulligan i Medyka; `--audio-test assets` zdekodował 35 efektów oraz oba utwory tła.
- Zbudowano pakiet ARM hard-float dla Cortex-A7. ZIP przechodzi test CRC, zawiera 604 pliki razem ze skrótem OnionOS, a wszystkie 603 pliki gry na karcie SD odpowiadają paczce SHA-256. Miyoo Mini Plus nie uruchomiono fizycznie w tej walidacji.
