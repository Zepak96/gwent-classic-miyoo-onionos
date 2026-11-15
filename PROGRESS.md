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
- [~] Szpieg, Medyk, Manekin, Zbiórka, Pożoga, pasowanie, remis i przejście rundy. Podstawowe zdolności frakcji są przeniesione z `factions.js`: dobór Królestw Północy, wygrana Nilfgaardu przy remisie, losowy oddział Potworów pozostający po rundzie, wybór pierwszego gracza przez ludzkiego Scoia'tael oraz wskrzeszenie dwóch jednostek Skellige na początku trzeciej rundy (wybór gracza zwinnej jednostki, AI losowo wybiera i ustawia ją na lepszym rzędzie). Weryfikacja szczegółowych interakcji nadal trwa.
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
- [~] AI „Classic”: zastąpiono sztuczne poziomy Easy/Normal/Hard pojedynczym wyborem ważonym opartym na `ControllerAI` z przypiętego repozytorium. Obejmuje ważenie kart, pasowania i liderów, wybór części celów/odrzuceń oraz początkowe wymiany kart; porównanie wszystkich wag i interakcji z referencją nadal trwa.
- [ ] Porównawcza weryfikacja wszystkich sekwencji względem źródła referencyjnego.

### Etapy 3–8 — port, UI, lokalizacja, audio, integracja, QA i release

- [~] Wierny interfejs referencyjny jest w trakcie portowania: ekran startowy edycji talii, proporcje trzech paneli meczu, rozmieszczenie rzędów oraz podgląd dużej karty z nazwą/opisem zdolności odwzorowują część układu `index.html`/`style.css`. Edytor i mecz nadal mają różnice wizualne i brakujące elementy: oryginalny HUD/stany menu, karuzela, pop-upy, powiadomienia, wynik, typografia i część ikon. Mapa elementów i skalowania jest w `docs/REFERENCE_UI_PORT.md`; obecny stan nie jest zgodny 1:1.
- [~] Prototyp silnika/UI posiada mecz, mulligan, cele Manekina/Medyków, lidera i pasowanie, ale kod nie jest kompletnym portem zachowania referencyjnego; zachowane różnice należy weryfikować jako braki względem `gwent.js` i plików składowych.
- [~] Zapis i odtwarzanie stanu meczu: dodano wersjonowany, sumowany kontrolnie format binarny, atomowy zapis oraz bezpieczne odrzucanie uszkodzonych plików; UI ma ręczny zapis/wczytanie i wznowienie z menu. Autosave i test na Miyoo pozostają do zrobienia; przepis talii ma już zapis silnikowy, ale nie jest jeszcze podłączony do UI.
- [~] Dodano format talii użytkownika `.gwd` i podłączono go do SDL: trzy sloty, edycja puli frakcji/neutrali, lidera i kopii, walidacja, zapis atomowy z sumą kontrolną oraz użycie talii przy starcie meczu. Silnik testuje round-trip i odrzucanie uszkodzeń. CI #43717386389 zbudował frontend, zaliczył CTest i wyrenderował ekrany edytora i meczu; brakuje zmiany nazwy talii, automatycznej QA sekwencji przycisków i testu na urządzeniu.
- [~] Edytor talii jest wstępnie podłączony w PL/EN; pełne PL/EN całego UI, tutorial i ustawienia nadal do zrobienia.
- [~] Przypięte grafiki kart, plansza, ikony SVG i SFX z repo są w `assets/gwent-classic/`; obrazy kart i UI są użyte w prototypowych ekranach, a sześć SVG ma rastrowe odpowiedniki PNG dla SDL. Muzyki nie ma w repo wzorcowym. Odtwarzanie audio w porcie nie jest zaimplementowane.
- [~] Przygotowano pliki launchera/pakowania OnionOS; ARM binarka i instalacja na karcie czekają na wierne UI oraz build docelowy.
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

- Lokalnie nie znaleziono CMake ani kompilatora C++; build desktop został wykonany w Dockerze Ubuntu.
- Karta OnionOS jest widoczna w Windows jako `E:` (`MIYOO MINI`, FAT32); nie ma jeszcze binarki ARM do instalacji ani uruchomienia.
- Samo Miyoo nie było testowane bezpośrednio po USB; test konsoli pozostaje niewykonany.
- Workflow GitHub Actions publikuje wynik kompilacji/testów przy każdym pushu.

## Zasada raportowania

Funkcja pozostaje nieukończona, dopóki nie ma implementacji i testu. Kompatybilność urządzenia nie będzie oznaczana jako potwierdzona bez testu fizycznego.
