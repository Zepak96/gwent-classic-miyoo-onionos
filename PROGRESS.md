# Postęp projektu

## Status: początek implementacji — 15.11.2026

Repozytorium wejściowe było puste. Powstał szkielet C++17/CMake, minimalny model silnika oraz pierwsze testy reguł. To nie jest ukończona gra ani wydanie dla Miyoo.

### Zakres wersji przyjęty po doprecyzowaniu

Docelowa gra to wyłącznie Gwint jako minigra w Wiedźminie 3: Wild Hunt, z zawartością podstawki, Hearts of Stone, Blood and Wine oraz zmianami/kartami dodanymi przez aktualizację next-gen Wiedźmina 3. Nie implementujemy samodzielnej gry `GWENT: The Witcher Card Game` ani jej późniejszych zasad. Repozytoria Gwent Classic służą jako punkt odniesienia dla mechaniki i pokrycia kart, po sprawdzeniu różnic względem gry Wiedźmin 3.

### Etap 1 — audyt

- [x] Przejrzano źródła `asundr/gwent-classic`, commit `08592d847f2573440a34deb9f32bc855b2544cdd`.
- [x] Zebrano informacje o `ia2904/gwent-classic-v3.5`, commit `2efc94ed626c0baeb28fc5e51c0e279ed5ee51dd`.
- [x] Ustalono, że oba projekty mają kod MIT z Commons Clause. Kod i zasoby nie są dołączane do tego projektu.
- [x] Ustalono, że obrazy, muzyka, fonty i część SFX z referencji nie mają wykazanych praw redystrybucji.
- [x] Sprawdzono dokumentację OnionOS Ports Collection oraz SDL2 dla Miyoo; pakowanie wymaga launcherów i bibliotek zgodnych z docelowym wydaniem OnionOS.
- [x] Zidentyfikowano wariant next-gen Wiedźmina 3 (m.in. Roach i Muster na Geralt/Ciri); został przyjęty jako docelowy po doprecyzowaniu użytkownika.

### Etap 2 — silnik

- [~] Model danych kart i graczy oraz początkowe liczenie punktów, pogoda, róg, scorch, pasowanie i przejście rundy.
- [ ] Pełny katalog kart zweryfikowany z ustalonym profilem wersji.
- [ ] Wszystkie zdolności jednostek, dowódców i frakcji oraz prawidłowa kolejność ich rozstrzygania.
- [ ] Pełna walidacja talii, mulligan, medyk, manekin i zapis odrzuconych kart.
- [ ] AI Easy/Normal/Hard bez informacji ukrytej.
- [ ] Porównawcze testy sekwencji ze źródłem referencyjnym.

### Etapy 3–8 — port, UI, lokalizacja, audio, integracja, QA i release

- [ ] SDL2 UI dopasowany do 640×480 i sterowanie fizycznymi przyciskami.
- [ ] Pełne PL/EN, tutorial, deck builder, ustawienia, zapis i odtwarzanie.
- [ ] Legalne i udokumentowane grafiki, muzyka oraz efekty dźwiękowe.
- [ ] ARM toolchain, binarka, launcher OnionOS i archiwum instalacyjne.
- [ ] Desktop/ARM build, regresja, pomiar zasobów oraz test sprzętowy.

## Znane blokady środowiska

- Na tym komputerze nie znaleziono CMake ani kompilatora C++; jest Docker, ale nie potwierdzono obrazu/toolchaina ARM.
- Brak potwierdzonego fizycznego Miyoo Mini Plus.
- Brak aktywnej autoryzacji GitHub CLI może uniemożliwić stworzenie zdalnego repozytorium.

## Zasada raportowania

Funkcja pozostaje nieukończona, dopóki nie ma implementacji i testu. Kompatybilność urządzenia nie będzie oznaczana jako potwierdzona bez testu fizycznego.
