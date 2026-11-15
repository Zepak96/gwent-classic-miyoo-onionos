# Raport testów

Data raportu: 2026-11-15.

## Mulligan, muzyka tła i aktualizacja SD — 15.11.2026

- Naprawiono obsługę Start podczas mulliganu: nie otwiera pauzy, tylko zatwierdza wybór lub pomija wymianę. A zaznacza kartę, Y zatwierdza/pomija, X otwiera inspektor. Ekran pokazuje liczbę pozostałych wymian oraz podpowiedzi przy dolnej krawędzi.
- Dwa utwory dostarczone przez właścicielkę projektu są odtwarzane w pętli: `menu.mp3` na ekranach menu i talii, `match.mp3` podczas meczu. Przełączanie ścieżek i dekodowanie obu MP3 są częścią `--audio-test`; efekty dźwiękowe nadal przechodzą dekodowanie.
- Weryfikacja lokalna: CTest 2/2, `--audio-test`, `--input-test`, smoke UI 640×480 (w tym ekran mulliganu). GitHub Actions dla commita `1df0c3740e85cca7a5e5aca826b54e7d2b9cdff8`: desktop #43753875815 i ARM package #43753875880 — sukces.
- ARM hard-float ZIP SHA-256: `273c7c80e1ee44df75b1ada8adc1519758d50816f822c933a15f53afd20855e5`. Zainstalowano na karcie `E:` bez kasowania plików; wszystkie 602 pliki gry z ZIP-a zgodne SHA-256, 0 brakujących/rozbieżnych; skrót `GwentClassic.port` zachowany. Hash zainstalowanej gry: `dda094d69a0d2afea52f5bb19a8ba4e2159a917550b299ae4b19e057cdadce5f`.
- Fizyczny start gry, przyciski Miyoo i odsłuch muzyki nadal wymagają potwierdzenia na konsoli.

## Regresja Więzi i aktualizacja karty — 15.11.2026

- Naprawiono błędne liczenie Tight Bond: zgodnie z opisem w referencyjnym `abilities.js`, dwie lub więcej kart o identycznej nazwie podwajają siłę; trzecia i kolejna kopia nie zwiększa mnożnika. Poprzedni test oczekiwał niepoprawnego mnożenia x3 i został skorygowany.
- Nowe przypadki sprawdzają trzy prawdziwe kopie „Blue Stripes Commando” z katalogu (wynik 24), brak Więzi pomiędzy różnymi nazwami mającymi tę samą metadaną grupy oraz działanie identycznej nazwy przy różnych grupach metadanych.
- Weryfikacja: desktop CMake build; CTest 2/2; `--audio-test` dekoduje 35 efektów; `--input-test` sprawdza mapę klawiszy i potwierdzone pasowanie; build ARM hard-float zakończony poprawnie.
- Przypięty SDL Mini zbudowano po zastosowaniu `patches/miyoo-sdl-keyhome.patch`; skompilowany ARM32 hard-float `libSDL2-2.0.so.0` w paczce jest identyczny z wynikiem builda. Aplikacja obsługuje `SDLK_HOME` przez zapis i wyjście.
- Paczka OnionOS SHA-256 `08c7643b372f57e46e389712ed0677888aa3734c5408605777c24cbf57a5bce4`, binarka gry SHA-256 `f37d866471e5f8d66ba2724dd290bea24fca48fe479eae1d7e5b990365afb5fb`, SDL SHA-256 `8eebe8969a25997265b35175c132fc72e66b16615a096af1cf71ec8d91db2053`. Zainstalowano na `E:`; 599 plików porównano SHA-256, 0 różnic, skrót `.port` zachowany. Fizyczny start i MENU na Miyoo nie zostały jeszcze przetestowane.

## Wynik

### Uzupełnienie modelu Mardroeme i Medyka Agile — 15.11.2026

- Zgodnie z `Row.updateState` w przypiętym `gwent.js`, Mardroeme jest trwałym
  efektem rzędu w tej rundzie. Silnik rozróżnia ten efekt od zajętego slotu
  specjalnego, co pozwala poprawnie odwzorować Mardroeme z Ermiona. Test sprawdza
  transformację istniejącego i później zagranego Berserkera, Zbiórkę, ponowne
  użycie slotu oraz zapis obu stanów.
- Zapis stanu używa wersji 4; loader akceptuje formaty 2 i 3.
- Desktop SDL2 build, CTest 2/2 i `--input-test` pod SDL dummy przechodzą.
  Nie wykonano ARM cross-builda: Docker Desktop przerwał wcześniejszą próbę i
  nie powstał z niej pakiet. Nie instalowano zmian na SD ani nie testowano ich
  na fizycznym Miyoo.

### Najnowsza weryfikacja — 15.11.2026

- Po ponownym audycie wizualnym podgląd zdolności w meczu zmieniono na czcionkę 2×, ograniczono do czterech wierszy, a dłuższy tekst otrzymuje widoczną wskazówkę `I - PELNY OPIS` / `I - FULL TEXT`. Pełny inspektor pokazuje cały opis Cirilli i cytat. Świeży render 640×480 obejrzano w natywnej skali; testy CTest 2/2 i dekodowanie 35 SFX przeszły. Długie opisy pozostają w inspektorze, a wizualna zgodność 1:1 i test na fizycznym Miyoo nadal są otwarte.
- Lokalny frontend SDL2 i silnik skompilowały się w Ubuntu 24.04/Dockerze. CTest: 2/2; dodatkowo dekoder audio sprawdził wszystkie 35 MP3, a smoke test zapisał sześć obrazów 640×480 w `docs/screenshots/`. Planszę meczu wycentrowano w ekranie 4:3, zachowując jej źródłowe 16:9; inspektor karty Cirilli pokazuje cały opis i cytat.
- Regresja Zbiórki sprawdza Geralta i Roacha. Test AI uruchamia pełne mecze AI kontra AI na poziomach Łatwy, Normalny i Trudny; osobny przypadek sprawdza, że trudny wybiera silniejszy ruch w prostym stanie.
- Desktop CI #43740551176 oraz ARM CI #43740551240 zakończyły się sukcesem dla commita `d4f4e6c`; przeszły testy silnika i katalogu, dekodowanie 35 SFX, smoke SDL i kontrola archiwum OnionOS. SHA-256 ZIP: `012eaaf45416f60e17162c69ec2d8d3d421198a9ae713f39afd5b0c98efa46e4`.
- Paczkę z tego commita zainstalowano na karcie `E:` (`MIYOO MINI`, OnionOS v4.3.1-1). Porównanie: 598 plików gry bez braków, nadmiarów ani różnic, a skrót `.notfound` zgodny; hash binarki na karcie: `d57e6584fe75ed2c8abb7a01240b832324ee86e8eb9ba05f147519bc698b5844`. Fizyczny start, przyciski, długi test pamięci/wydajności i odsłuch na Miyoo nie zostały jeszcze sprawdzone. Skrót wymaga uruchomienia `~Import ports.miyoocmd` na urządzeniu.

Testy automatyczne C++ obejmują punktowanie, pogodę (w tym Skellige Storm), róg, Bohaterów, Morale, Więź (3 kopie), kolejność Więzi i Morale, pasowanie, remis, Nilfgaard, zdolność pierwszego gracza Scoia'tael, wybór dwóch wskrzeszeń Skellige w rundzie trzeciej, Szpiega, Medyka (wymóg celu i zakaz wskrzeszania Bohatera), Manekina, Zbiórkę Geralta/Roacha oraz AI, Mardroeme/Berserker, Scorch, Avengera Cow/Kambiego po Manekinie, Pożodze i końcu rundy, rozdanie i wymianę kart, walidację talii, katalog, 10 talii referencyjnych, round-trip oraz odrzucenie uszkodzonego przepisu talii użytkownika, wybrane zdolności liderów, zgodność decyzji AI przy zmianie ukrytej ręki przeciwnika, ochronę Roacha w mulliganie, wszystkie trzy poziomy trudności oraz pełny mecz AI kontra AI na każdym poziomie. Dawny test wymuszający wybór lidera usunięto, bo referencyjny `ControllerAI` losuje akcję proporcjonalnie do wag i nie zawsze wybiera najwyżej ważoną możliwość.

Po dodaniu przepływów Scoia'tael i Skellige workflow [desktop-build #43714840961](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43714840961) zbudował silnik oraz frontend SDL2, zaliczył 2/2 CTest w obu konfiguracjach i uruchomił test startowy SDL z atrapą wideo. Testy automatyczne obejmują wybór pierwszego gracza i wybór wskrzeszeń w rundzie trzeciej. Smoke test frontendowy nie symuluje tych wejść ani pełnego meczu; nie wykonano też testu na urządzeniu Miyoo.

Po dodaniu zapisu i wczytania workflow [desktop-build #43716080347](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43716080347) zbudował silnik i frontend, zaliczył testy oraz uruchomił smoke test SDL. Test C++ zapisuje i odczytuje pełny stan meczu, a następnie sprawdza odrzucenie pliku ze zmienioną sumą kontrolną bez naruszenia stanu gry w pamięci. Smoke test nie ćwiczy klawiszy zapisu/wczytania. Test sprzętowy nadal nie jest wykonany: podczas tej sesji Windows nie wykrył nowego dysku, urządzenia przenośnego ani interfejsu USB sieciowego po podłączeniu Miyoo.

Po dodaniu API talii użytkownika pierwsze uruchomienie CI ujawniło błąd dekodowania pary identyfikatora/liczby kopii. Poprawka została zbudowana i zweryfikowana przez [desktop-build #43716630159](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43716630159): oba zadania (silnik i frontend) zakończyły się powodzeniem, testy CTest przeszły, a frontendowy smoke test SDL zakończył się poprawnie. Test obejmuje poprawny zapis/odczyt talii, materializację jej kart z katalogu oraz odrzucenie uszkodzonego pliku bez nadpisania przepisu w pamięci. Nie obejmuje jeszcze edytora talii w UI ani uruchomienia gry na Miyoo.

Po podłączeniu edytora do menu workflow [desktop-build #43717386389](https://github.com/Zepak96/gwent-classic-miyoo-onionos/actions/runs/43717386389) przeszedł dla silnika i frontendu. CTest zakończył się powodzeniem, smoke test SDL wyrenderował mecz i ekran edycji talii, a workflow zapisał oba obrazy jako artefakt `frontend-smoke-frames`. Ten test sprawdza renderowanie, nie automatyzuje sekwencji przycisków w edytorze ani nie zastępuje testu na fizycznym Miyoo.

| Obszar | Status | Uwagi |
|---|---|---|
| Budowanie desktop | Zaliczone w CI | Ubuntu, CMake, SDL2; budowany frontend oraz silnik/testy |
| Budowanie ARM | Zaliczone w CI | Miyoo GCC 8.2.1/armhf, SDL2 z przypiętego portu; paczka `d4f4e6c` skopiowana i porównana z kartą. |
| Zasady / regresja | Częściowo zaliczone lokalnie i w CI | Wybrane reguły, liderzy, Roach/Muster i pełne mecze AI×AI dla trzech poziomów; kompletne porównanie wszystkich sekwencji z referencją nadal nie istnieje. |
| Interfejs 640×480 | Smoke wizualny zaliczony | Obejrzano cztery klatki 640×480; plansza jest wycentrowana, opis Ciri czytelny. Brakuje automatyzacji interakcji, porównania wszystkich ekranów 1:1 i testu fizycznego. |
| Audio | Częściowo zaliczone desktopowo | `dr_mp3` dekoduje wszystkie 35 MP3; SDL Audio miksuje dźwięki zdarzeń. Brak muzyki w repo źródłowym i testu dźwięku na Miyoo. |
| OnionOS | Paczka na SD; start fizyczny niezweryfikowany | 598 plików gry i skrót zgodne SHA-256 z paczką `d4f4e6c`; import skrótu i uruchomienie gry pozostają do testu na Miyoo. |
| Długi test / RAM | Niewykonane | Nie przeprowadzono |

## Test renderowania assetów i UI

W tej sesji dołączono 531 plików obrazów, 35 SFX MP3 i 6 SVG z przypiętego repo. Test katalogu sprawdza istnienie małej i dużej ilustracji dla każdego z 216 rekordów. Build SDL2 w Dockerze Ubuntu zakończył się powodzeniem, CTest zaliczył 2/2, a smoke test utworzył kadr meczu i edytora z wczytanymi ilustracjami. Oględziny potwierdziły ładowanie plików, ale wykazały, że kompozycja menu/HUD/stołu jest autorskim prototypem i nie odpowiada layoutowi `style.css`; kadry nie są dowodem zgodności 1:1 i wymagają zastąpienia wiernym portem.

## Test sprzętowy

Port nie został jeszcze uruchomiony na fizycznym Miyoo Mini Plus. Karta microSD jest wykryta jako `E:` (`MIYOO MINI`, FAT32); paczka z commita `d4f4e6c` znajduje się w `Roms/PORTS/Games/GwentClassic`. Porównano SHA-256 598 plików gry i skrótu `.notfound`; skrót wymaga uruchomienia `~Import ports.miyoocmd` na urządzeniu. Zgodność plików nie potwierdza startu, wejścia, audio ani wydajności na konsoli.

## Ponowna kontrola zgodności układu źródłowego

Po porównaniu renderera SDL z `style.css` poprawiono położenie liczników/pól rzędów i początek układania kart w środkowym panelu, bazując na oryginalnym podziale 26.5% / 52.5% / 21% i proporcjach rzędów. Dodano też kartę dużego podglądu i panel opisu zdolności w pozycjach źródłowych oraz zaimportowano 44 wpisy nazw/opisów zdolności z przypiętego `abilities.js`. Build desktop SDL2 w Ubuntu 24.04 zakończył się powodzeniem; CTest: 2/2. Smoke test wyrenderował edytor i mecz; obrazy sprawdzono wizualnie. Klatki nadal pokazują odstępstwa od źródła, m.in. nieodtworzone panele pozostałych akcji/karty, oryginalną typografię i zachowanie nakładek. Zmiana geometrii i dodanie podglądu nie są dowodem portu 1:1; zgodność wizualna i sprzętowa pozostaje niezaliczona.

Następnie sześć źródłowych ikon SVG zrasteryzowano do PNG 256×256 przez CairoSVG 2.9.1. Renderer meczu przełączono na PNG dla muzyki, efektów i powiadomień; wizualny smoke test potwierdził widoczne ikony. Build i CTest (2/2) przeszły ponownie. Pozostałe UI i rzeczywiste interakcje nie zostały przez tę kontrolę zaakceptowane jako zgodne 1:1.

Dodano własny dekoder MP3 `dr_mp3` (Unlicense/public domain) oraz odtwarzanie wielokanałowych efektów przez SDL Audio. Desktopowy build zakończył się powodzeniem; CTest 2/2, a `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy gwent --audio-test` pomyślnie zdekodował wszystkie 35 plików. To weryfikuje dekodowanie i otwarcie urządzenia SDL dummy, nie fizyczny głośnik Miyoo. Renderowane klatki edytora i meczu zapisano w `docs/screenshots/current-deck-smoke.png` oraz `docs/screenshots/current-match-smoke.png`; są to zrzuty desktopowego smoke testu, nie test sprzętowy.

Po zgłoszeniu niskiej czytelności dodano fullscreenowy inspektor wybranej karty z dużą grafiką źródłową oraz większym polem nazwy, siły, rzędu i opisu. Zrzut `docs/screenshots/current-card-details-smoke.png` sprawdzono wizualnie w 640×480; opis można odczytać, a tło inspektora jest nieprzezroczyste. Otwieranie: `I` lub przytrzymanie SELECT 500 ms; zamykanie: `B`/ESC. To rozwiązuje odczyt szczegółu wybranej karty, ale nie stanowi jeszcze akceptacji wszystkich napisów, liczników, wyrównań ani animacji względem źródła. Pełny desktop build, CTest, dekoder audio i trzyklatkowy smoke przebiegły poprawnie; testu sprzętowego nadal brak.

Mały panel opisu przeniesiono z jednopikselowego, sztucznie ściśniętego tekstu na czytelniejsze pełnej szerokości glify 5×7 z zawijaniem po 29 znaków; liczba kopii jest centrowana względem ikony znacznika. Dodano lot karty z ręki do rzędu/pogody dla człowieka i AI (500 ms, źródłowy JS czeka 499 ms) oraz smoke klatkę w połowie przejścia. Build, CTest 2/2, dekodowanie 35 MP3 i czteroklatkowy smoke przechodzą. Kaskadowe przejścia karuzeli, fade paneli i pozostałe animacje SDL nadal nie implementuje; liczniki, wyrównania i wszystkie ekrany wymagają dalszej porównawczej korekty.

## Regresja duplikatów pogody

Porównanie z `Weather.addCard` w przypiętym `gwent.js` wykazało, że aktywny może pozostać tylko jeden egzemplarz danego rodzaju karty pogodowej; kolejny trafia do cmentarza bez zmiany aktywnego efektu. `applyWeather` teraz zachowuje tę regułę, a test silnika zagrywa Biting Frost po obu stronach, potwierdza pojedynczy efekt i odrzucenie drugiej karty, a następnie sprawdza usunięcie pogody przez Czyste Niebo. Build SDL2 w Ubuntu 24.04, CTest (2/2) i dekodowanie wszystkich 35 SFX zakończyły się powodzeniem. Zmiana nie stanowi potwierdzenia pełnej zgodności zasad ani testu Miyoo.

## Korekta klatek i czytelności liczb/kart

Oględziny smoke frame meczu ujawniły, że renderer nie czyścił bufora po edytorze talii; w dolnej części kadru pozostawała stara grafika karty. Dodano czyszczenie renderera w pętli gry i przed każdą klatką smoke. Dwucyfrowa siła mieści się teraz w power medallion, a wynik rundy jest wycentrowany. Panel opisu karty pokazuje pełny tekst (bez limitu trzech linii) w wyższej czcionce. Klatka inspektora została ustawiona na Cirillę, by sprawdzić długą nazwę, cytat w oryginalnej ilustracji i pełny opis Muster. Build SDL2 Ubuntu 24.04, CTest (2/2) i czteroklatkowy smoke test zakończone powodzeniem; kadry sprawdzono wizualnie. Nadal wymagane są test na Miyoo i porównanie całego UI ze źródłem.

## Build ARM i paczka OnionOS

Frontend skompilowano dla ELF32 ARM EABI hard-float oficjalnym `mini_toolchain-v1.0` (GCC 8.2.1) i przypiętą biblioteką SDL2 dla Miyoo. Pierwszy cross-link wykazał brak `std::filesystem` z GCC 8 oraz nierozwiązane symbole MI/GFX z referencyjnego SDL; dodano `stdc++fs` oraz odpowiednią flagę linkerową dla zależności dostarczanych przez OnionOS. Skrypt `tools/build_onion.sh` odtwarza konfigurację, sprawdza SHA-256 SDK i rewizję SDL, pakuje licencje oraz uruchamia kontrolę formatu i hard-float ELF. Pełny build skryptem zakończył się sukcesem, ZIP przeszedł `unzip -t`, a SHA-256 paczki to `bc7ae158a545182160c8cecc1e48a259155dadc3b32200c5dd7375ed3c52cc87`. Paczkę skopiowano do nowych ścieżek `Roms/PORTS` na podłączoną kartę z OnionOS v4.3.1-1; suma binarki na karcie zgadza się z buildem. Nie uruchomiono jeszcze gry na urządzeniu, więc start SDL, zachowanie przycisków, audio i wydajność nie są potwierdzone.

Następnie ustandaryzowano ścieżki źródeł kompilatora, końce linii notice licencyjnych, uprawnienia, kolejność plików i czasy ZIP. Dwa lokalne przebiegi pakowania dały ten sam wynik; SHA-256 paczki `b93ea51d4ff998919e014035cf21d4306d254740b532c84630c0e0272ed9db77` jest też zgodna z artefaktem z GitHub Actions #43735411242. Binarka wewnątrz obu paczek jest identyczna, a jej SHA-256 `8e5c007861d5d859d8d17746e00fe0e1b09f61bff472dc95799c50b8a493cc77`. Zaktualizowaną binarkę skopiowano na kartę i odczytano zgodną sumę. Uruchomienie na Miyoo nadal nie jest potwierdzone.

Po kolejnym commitcie workflow #43735744971 ponownie zbudował ARM i przesłał artefakt. Paczka z tego runu ma ten sam SHA-256 `b93ea51d4ff998919e014035cf21d4306d254740b532c84630c0e0272ed9db77`. Sprawdzono ZIP, skład wszystkich plików i licencje GCC. Po instalacji najnowszego artefaktu porównano SHA-256 każdego z 599 plików na karcie z wpisami w ZIP-ie: 0 brakujących, 0 różniących się i 0 dodatkowych. Ścieżka OnionOS `~Import ports.miyoocmd`, standalone launcher i wersja `v4.3.1-1` są obecne. Składnia `launch.sh` i `.notfound` jest poprawna. Gra nie została jeszcze uruchomiona; użytkownik przeprowadzi fizyczny test po zaimportowaniu skrótu.

Po kolejnym zgłoszeniu problemu z czytelnością UI powiększono podgląd ilustracji kart na planszy oraz kartę w inspektorze; karta Cirilli zajmuje teraz niemal całą dostępną wysokość, dzięki czemu jej oryginalny cytat i nazwa są wyraźniejsze. Moc na zwykłych kartach jest rysowana standardowym, kontrastowym krojem i wycentrowana w medalionie; poprawiono też kontrast/nieprzezroczystość panelu opisu. Build SDL2 w Ubuntu 24.04 i czteroklatkowy smoke render zakończyły się poprawnie. Kadry meczu i inspektora ponownie obejrzano w 640×480. To desktopowy render, a nie weryfikacja na fizycznym Miyoo.
## Audyt źródłowy i wybór rzędu Medyka Agile — 15.11.2026

Na gałęzi `codex/full-source-parity` wykonano w kontenerze `gwent-miyoo-verify`:

- konfigurację i kompilację CMake desktop SDL2 z `GWENT_BUILD_TESTS=ON`;
- CTest: `engine_rules` i `card_catalog`, oba zaliczone (2/2);
- test sterowania `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy gwent --input-test`,
  który przeszedł na mapowaniu wejścia Miyoo dla wymiany kart, potwierdzenia pass
  oraz wyboru rzędu dla wskrzeszanej karty Agile.

Nie wykonano w tej gałęzi cross-builda ARM, kontroli paczki OnionOS, odczytu SD
ani testu na fizycznym Miyoo. Testy nie dowodzą jeszcze parytetu pełnego zestawu
kart, liderów, interfejsu ani animacji. Lista elementów otwartych jest w
`docs/SOURCE_PARITY.md`.

## Source animation port — 15.11.2026

- Added use of the upstream `anim_<ability>.png` overlays for source ability
  visuals, using the reference 50/300/1000/300 ms sequence; Scorch uses its
  reference 50/300/1300/300 ms sequence.
- Native played-card movement now follows the source `translate()` duration
  (499 ms). Weather row overlays now fade over 500 ms.
- Desktop CMake build succeeded in `gwent-miyoo-verify`; CTest passed 2/2 and
  `gwent --input-test` passed with dummy SDL video/audio.
- This is a partial animation port. `docs/REFERENCE_ANIMATIONS.md` records the
  remaining source behaviors; no ARM build, SD copy, or physical Miyoo check was
  made for this change.

## Random source opponent setup — 15.11.2026

`DeckMaker.constructOpponentDeck()` in the pinned reference chooses a random premade deck and then a random legal leader from that deck's faction. Replaced the port's predictable "next recipe" opponent with that behavior. `makeReferenceOpponentDeck(seed)` uses a local deterministic PRNG so it can be tested and replayed; the SDL front end supplies a per-match seed. CTest verifies 500 seeds, all ten premade recipes, faction-legal leaders, valid card lists, and repeatability. Desktop build and CTest passed (2/2); ARM/device verification remains open.

## Source automatic passing — 15.11.2026

Ported the reference `Player.canPlay()`, `Player.endTurn()`, and `Game.startRound()` rule: a player with no cards and no available leader action passes automatically after their action; players unable to act are also passed as a round starts. Round start deliberately does not refresh a leader already used earlier in the match. Reworked engine fixtures to retain spare actions where a test needs to continue, and added explicit last-card and empty-opening cases. Desktop build and CTest passed (2/2).

## Reference ability animation targets — 15.11.2026

- Compared `abilities.js` and `Card.animate()` in pinned upstream commit `08592d847f2573440a34deb9f32bc855b2544cdd`.
- Extended the original PNG/timing overlays to each matching Bond unit, newly summoned Muster unit, revived Medic target, and each Scorch target at its pre-action board location.
- Desktop build succeeded; CTest passed 2/2; dummy SDL `--input-test` passed.
- This remains partial animation parity. Cross-build, physical device, and SD copy were not performed. See `docs/REFERENCE_ANIMATIONS.md` for open animation behavior.

## Polish and English catalog rendering — 15.11.2026

- Updated CMake catalog generation to include `data/card-names-pl.json` and `data/ability-text-pl.json`.
- Added locale-aware card names, ability titles and descriptions; the UI uses localized names in its detail/preview, deck and match labels and adds a localized name strip over enlarged source artwork. Added bitmap glyphs for Polish diacritics.
- Catalog validation checks complete name-map and ability-map coverage; engine tests check Geralt's English/Polish name and translated ability description across the catalog.
- Fresh desktop configure/build passed; CTest passed 2/2; SDL `--input-test` passed; 640×480 Polish detail/deck smoke frames were generated and visually inspected.
- This does not complete UI localization: several static prompts and labels remain English, official Polish text still requires a source-by-source review, and no ARM/SD/device validation was performed.

## Start on the source DeckMaker screen — 15.11.2026

- Removed the invented title screen and made the deck editor the app's initial screen, matching the source's immediate `new DeckMaker()` initialization.
- The auxiliary deck selector's Escape path returns to the deck editor. Startup-screen checks run in `--input-test` and `--smoke-test`.
- Desktop build, CTest 2/2, `--input-test`, and a six-frame SDL dummy smoke run passed; the initial DeckMaker frame is saved at `artifacts/qa/deckmaker-start.png`.
- Only the initial-screen flow is aligned here; DeckMaker geometry and interaction parity remain open. No ARM, SD, or physical-device validation was done.

## Timed source notifications — 15.11.2026

- Ported the reference notification lifecycle to the SDL frontend: source notification PNGs, a center-screen banner, a queued event sequence, and 150 ms fade-in + 900 ms hold + 150 ms fade-out (1200 ms total).
- Wired initial coin result, round start, turn handoff, pass, and round result events; added PL/EN labels and a persisted notification toggle. Existing settings remain readable.
- Extended `--input-test` with notification alpha boundaries, queue ordering, disabled behavior, and gameplay transition cases. Desktop build and CTest (2/2), `--input-test`, and the SDL dummy smoke run passed; the notification screenshot was generated and visually inspected.
- This is one slice of the animation port, not full animation parity. Carousel/preview/end-screen fades, source-exact sequencing and physical Miyoo behavior remain open. No ARM build or SD write was made in this slice.
