# Licencje i pochodzenie

## Ten projekt

Oryginalny kod tego projektu jest objęty dołączoną licencją MIT. Nie obejmuje ona cudzych zasobów, znaków towarowych ani materiałów gry. Nazwy kart, opisy i katalog danych wymagają oddzielnego przeglądu pochodzenia przed wydaniem.

## Repozytoria referencyjne

- `asundr/gwent-classic`, HEAD `08592d847f2573440a34deb9f32bc855b2544cdd` — upstream oznacza kod jako MIT z Commons Clause v1.0 (licensor: Arun Sundaram). Commons Clause wyłącza prawo sprzedaży zdefiniowane w klauzuli i wymaga zachowania informacji licencyjnych. Źródło: https://github.com/asundr/gwent-classic/blob/08592d847f2573440a34deb9f32bc855b2544cdd/LICENSE
- `ia2904/gwent-classic-v3.5`, HEAD `2efc94ed626c0baeb28fc5e51c0e279ed5ee51dd` — rozwinięty fork; jego kod i zawartość nie są kopiowane do tego repozytorium. Weryfikacja źródeł i plikowych licencji przed ewentualnym użyciem jest wymagana.
- `steward-fu/sdl2`, audytowany HEAD `0631abc8e8916db6f9bc7e2afd0c22913d092a29` — SDL2, licencja LGPL-2.1. Biblioteki nie są na razie dołączone.
- `OnionUI/Ports-Collection`, audytowany HEAD `685c7e6e9065232d6350765022baa4723a641f13` — wzorzec integracji systemowej. Launcher i materiały muszą zachować licencje upstream przy użyciu.
- `nothings/stb`, commit `9600522de53973336fa033a289858a4354cbb04c` — `stb_image.h` v2.32, deklarowana licencja public domain; używany do dekodowania JPEG-ów kart i planszy.
- `mackron/dr_libs`, commit `f12e197fd462e4c22873fbd70d11b1d2f2c1dc3c` — `dr_mp3.h`, używany do dekodowania efektów MP3. Projekt wybiera dołączoną opcję Unlicense/public domain; szczegóły w `third_party/DR_MP3_NOTICE.md`.
- Oficjalny `steward-fu/mini_toolchain-v1.0`, SHA-256 `8addff71be4b015a4e1aef51e43635e50978d558a1675f5b1664124e8437d071` — źródło dynamicznych bibliotek `libstdc++.so.6` i `libgcc_s.so.1` w paczce ARM. Odpowiadające im teksty GPLv3 i GCC Runtime Library Exception 3.1 pobrano verbatim z `gcc-mirror/gcc` tag `releases/gcc-8.2.0`; znajdują się w `third_party/gcc-runtime/` oraz w paczce OnionOS. Źródło projektu GCC 8.2.x: https://github.com/gcc-mirror/gcc/tree/releases/gcc-8.2.0.

## Zaimportowane dane

`data/cards.json` zawiera 216 rekordów z `card_dict`, `data/premade-decks.json` 10 przepisów talii z `decks.js`, a `data/ability-text.json` nazwy i opisy zdolności z `abilities.js` w `asundr/gwent-classic` commit `08592d847f2573440a34deb9f32bc855b2544cdd`. Zapisano upstreamowy plik `LICENSE` i informację o pochodzeniu w `third_party/gwent-classic/`. Dane podlegają MIT + Commons Clause v1.0; warunek Commons Clause ogranicza sprzedaż. Przed wydaniem należy zachować ten notice i sprawdzić ograniczenia dystrybucyjne.

Repozytorium zawiera przypięte zasoby `img/`, `sfx/` i `svg/` z `asundr/gwent-classic` (531 grafik, 35 efektów dźwiękowych, 6 ikon SVG) oraz dwa utwory MP3 dostarczone przez właściciela projektu jako muzyka tła menu i meczu. Pochodzenie zapisano w `assets/gwent-classic/ASSET_ORIGIN.md`. README projektu źródłowego przypisuje wiele edytowanych efektów dźwiękowych RandomPianistowi — ta atrybucja pozostaje zachowana. Repo wzorcowe nie zawiera plików muzycznych; jego muzyka była strumieniowana z YouTube. W repozytorium źródłowym nie znaleziono licencji mediów określającej warunki redystrybucji poszczególnych plików.

## Materiały referencyjne

- https://github.com/asundr/gwent-classic
- https://github.com/ia2904/gwent-classic-v3.5
- https://github.com/steward-fu/sdl2
- https://github.com/OnionUI/Ports-Collection
- https://onionui.github.io/docs/ports
