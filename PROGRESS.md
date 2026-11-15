# Postęp projektu

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
- [~] Zbudowano frontend armhf oficjalnym toolchainem Miyoo GCC 8.2.1 i przypiętym SDL2; po poprawce czytelności ZIP OnionOS z commita `d4f4e6c` (workflow ARM #43740551240) skopiowano na kartę SD OnionOS v4.3.1-1. Audyt plików: 598/598 plików gry i skrót `.notfound` zgodne SHA-256; binarka na karcie ma hash `d57e6584fe75ed2c8abb7a01240b832324ee86e8eb9ba05f147519bc698b5844`. Skrót wymaga uruchomienia `~Import ports.miyoocmd`; start gry, wejście, audio i wydajność czekają na fizyczny test.
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
