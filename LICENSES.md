# Licencje i pochodzenie

## Ten projekt

Oryginalny kod tego projektu jest objęty dołączoną licencją MIT. Nie obejmuje ona cudzych zasobów, znaków towarowych ani materiałów gry. Nazwy kart, opisy i katalog danych wymagają oddzielnego przeglądu pochodzenia przed wydaniem.

## Repozytoria referencyjne

- `asundr/gwent-classic`, HEAD `08592d847f2573440a34deb9f32bc855b2544cdd` — upstream oznacza kod jako MIT z Commons Clause v1.0 (licensor: Arun Sundaram). Commons Clause wyłącza prawo sprzedaży zdefiniowane w klauzuli i wymaga zachowania informacji licencyjnych. Źródło: https://github.com/asundr/gwent-classic/blob/08592d847f2573440a34deb9f32bc855b2544cdd/LICENSE
- `ia2904/gwent-classic-v3.5`, HEAD `2efc94ed626c0baeb28fc5e51c0e279ed5ee51dd` — rozwinięty fork; jego kod i zawartość nie są kopiowane do tego repozytorium. Weryfikacja źródeł i plikowych licencji przed ewentualnym użyciem jest wymagana.
- `steward-fu/sdl2`, audytowany HEAD `0631abc8e8916db6f9bc7e2afd0c22913d092a29` — SDL2, licencja LGPL-2.1. Biblioteki nie są na razie dołączone.
- `OnionUI/Ports-Collection`, audytowany HEAD `685c7e6e9065232d6350765022baa4723a641f13` — wzorzec integracji systemowej. Launcher i materiały muszą zachować licencje upstream przy użyciu.
- `nothings/stb`, commit `9600522de53973336fa033a289858a4354cbb04c` — `stb_image.h` v2.32, deklarowana licencja public domain; używany do dekodowania JPEG-ów kart i planszy.

## Zaimportowane dane

`data/cards.json` zawiera 216 rekordów z `card_dict`, `data/premade-decks.json` 10 przepisów talii z `decks.js`, a `data/ability-text.json` nazwy i opisy zdolności z `abilities.js` w `asundr/gwent-classic` commit `08592d847f2573440a34deb9f32bc855b2544cdd`. Zapisano upstreamowy plik `LICENSE` i informację o pochodzeniu w `third_party/gwent-classic/`. Dane podlegają MIT + Commons Clause v1.0; warunek Commons Clause ogranicza sprzedaż. Przed wydaniem należy zachować ten notice i sprawdzić ograniczenia dystrybucyjne.

Repozytorium zawiera przypięte zasoby `img/`, `sfx/` i `svg/` z `asundr/gwent-classic` (531 grafik, 35 efektów dźwiękowych, 6 ikon SVG); ich pochodzenie jest zapisane w `assets/gwent-classic/ASSET_ORIGIN.md`. Właściciel projektu zgłasza, że przedstawiciel CD PROJEKT RED udzielił zgody na niekomercyjne użycie zasobów CDPR w tym modzie na Miyoo Mini Plus. Prywatna korespondencja nie jest publikowana. README projektu źródłowego przypisuje wiele edytowanych efektów dźwiękowych RandomPianistowi — ta atrybucja pozostaje zachowana. Repo wzorcowe nie zawiera plików muzycznych; jego muzyka była strumieniowana z YouTube.

## Materiały referencyjne

- https://github.com/asundr/gwent-classic
- https://github.com/ia2904/gwent-classic-v3.5
- https://github.com/steward-fu/sdl2
- https://github.com/OnionUI/Ports-Collection
- https://onionui.github.io/docs/ports
