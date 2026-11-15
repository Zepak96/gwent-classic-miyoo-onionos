# Postęp projektu

## Status: aktywna implementacja — 15.11.2026

Repozytorium wejściowe było puste. Powstał szkielet C++17/CMake, model silnika i prosty frontend SDL2. To nie jest ukończona gra ani wydanie dla Miyoo.

### Zakres wersji przyjęty po doprecyzowaniu

Docelowa gra to wyłącznie Gwint jako minigra w Wiedźminie 3: Wild Hunt, z zawartością podstawki, Hearts of Stone, Blood and Wine oraz zmianami/kartami dodanymi przez aktualizację next-gen Wiedźmina 3. Nie implementujemy samodzielnej gry `GWENT: The Witcher Card Game` ani jej późniejszych zasad. Przypięte repozytorium Gwent Classic jest źródłem docelowego zachowania i zakresu portu. Celem jest przeniesienie tej gry 1:1 na Miyoo Mini Plus, a nie samodzielne korygowanie jej według innej wersji Gwinta. Różnice względem gry referencyjnej należy traktować jako braki portu.

### Etap 1 — audyt

- [x] Przejrzano źródła `asundr/gwent-classic`, commit `08592d847f2573440a34deb9f32bc855b2544cdd`.
- [x] Zebrano informacje o `ia2904/gwent-classic-v3.5`, commit `2efc94ed626c0baeb28fc5e51c0e279ed5ee51dd`.
- [x] Ustalono, że upstreamowy kod ma licencję MIT z Commons Clause; zaimportowano katalog danych kart/talii z zachowaniem licencji i manifestu. Grafiki i audio z referencji nie zostały zaimportowane.
- [x] Ustalono, że obrazy, muzyka, fonty i część SFX z referencji nie mają wykazanych praw redystrybucji.
- [x] Sprawdzono dokumentację OnionOS Ports Collection oraz SDL2 dla Miyoo; pakowanie wymaga launcherów i bibliotek zgodnych z docelowym wydaniem OnionOS.
- [x] Zidentyfikowano wariant next-gen Wiedźmina 3 (m.in. Roach i Muster na Geralt/Ciri); został przyjęty jako docelowy po doprecyzowaniu użytkownika.
- [x] Utworzono publiczne repozytorium GitHub i dodano workflow CI przez integrację GitHub.
- [x] Zaimportowano 216 rekordów katalogu kart z przypiętego repozytorium referencyjnego wraz z upstreamową licencją i manifestem pochodzenia.

### Etap 2 — silnik

- [~] Model danych kart i graczy; podstawowe punktowanie z pogodą, rogiem, Morale, Więzią i immunitetem Bohaterów.
- [~] Szpieg, Medyk, Manekin, Zbiórka, Pożoga, pasowanie, remis, przejście rundy i wstępne zdolności frakcji.
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
- [~] AI Easy/Normal/Hard: sprawdzono brak zależności decyzji od ukrytej zawartości ręki przeciwnika, pełny mecz Hard kontra Hard oraz wybór aktywnej zdolności lidera. Strategia i obsługa doboru opcji nie pokrywają jeszcze wszystkich interakcji referencji.
- [ ] Porównawcza weryfikacja wszystkich sekwencji względem źródła referencyjnego.

### Etapy 3–8 — port, UI, lokalizacja, audio, integracja, QA i release

- [~] Prototyp SDL2 640×480 ma menu, wybór talii i poziomu AI, mecz, mulligan, planszę, rękę, cele Manekina/Medyków, lidera i pasowanie; CI buduje frontend i wykonuje startowy smoke test. Kompletność UI, lokalizacja i mapowanie fizycznych przycisków wymagają dalszej pracy/testów.
- [ ] Pełne PL/EN, tutorial, deck builder, ustawienia, zapis i odtwarzanie.
- [ ] Legalne i udokumentowane grafiki, muzyka oraz efekty dźwiękowe.
- [ ] ARM toolchain, binarka, launcher OnionOS i archiwum instalacyjne.
- [ ] Desktop/ARM build, regresja, pomiar zasobów oraz test sprzętowy.

## Rozszerzenie po ukończeniu wiernego silnika — kampania arcade

Użytkownik chce później rozbudować grę o lekką warstwę fabularną. Ten zakres jest celowo po pełnej implementacji pojedynczego meczu, żeby każdy przeciwnik używał tych samych sprawdzonych zasad.

- [ ] Mapa/drzewko kolejnych przeciwników inspirowanych postaciami z Wiedźmina 3.
- [ ] Rosnąca trudność przeciwników i talii między pojedynkami.
- [ ] Finałowy przeciwnik/boss oraz zakończenie małej kampanii.
- [ ] Menu kampanii i osobny zapis jej postępu.
- [ ] Kilka wariantów plansz/stolików z legalną, oryginalną oprawą graficzną.
- [ ] Opracować krótkie wprowadzenie fabularne oraz teksty PL/EN, bez naruszania licencji tekstów i assetów gry.

Nie zmienia to bieżącego celu: najpierw kompletna rozgrywka Gwinta z Wiedźmina 3 next-gen przeciw AI oraz stabilny port OnionOS.

## Znane blokady środowiska

- Lokalnie nie znaleziono CMake ani kompilatora C++; build desktop wykonał GitHub Actions na Ubuntu.
- Docker Desktop nie ma uruchomionego daemonu; brak lokalnego obrazu/toolchaina ARM.
- Brak potwierdzonego fizycznego Miyoo Mini Plus.
- Workflow GitHub Actions publikuje wynik kompilacji/testów przy każdym pushu.

## Zasada raportowania

Funkcja pozostaje nieukończona, dopóki nie ma implementacji i testu. Kompatybilność urządzenia nie będzie oznaczana jako potwierdzona bez testu fizycznego.
