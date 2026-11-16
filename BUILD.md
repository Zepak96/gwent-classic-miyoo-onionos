# Build

## Status

Projekt używa C++17 i CMake. Konfiguracja zakłada opcjonalny frontend SDL2 i niezależną bibliotekę silnika. Desktop oraz ARM hard-float dla Miyoo Mini Plus mają osobne konfiguracje.

## Desktop

Wymagania: CMake 3.16+, kompilator C++17, Python 3 i SDL2. Python generuje natywny katalog kart na etapie kompilacji. Frontend dekoduje dołączone efekty MP3 przez `dr_mp3` i odtwarza je przez SDL Audio, bez dodatkowej biblioteki miksującej. Z katalogu projektu:

```sh
cmake -S . -B build -DGWENT_WITH_SDL=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

When SDL2 is available, the `gwent` executable opens the 640×480 match UI. The desktop CI also runs a renderer smoke check without a display server:

```sh
SDL_VIDEODRIVER=dummy ./build/gwent --smoke-test match.bmp deck-builder.bmp card-details.bmp card-flight.bmp
```

Wszystkie referencyjne efekty MP3 można zdekodować na urządzeniu SDL dummy:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/gwent --audio-test
```

Bez SDL2 można budować same testy i silnik: ustaw `-DGWENT_WITH_SDL=OFF`.

## Save files

The SDL app writes a versioned, checksummed match save in `SDL_GetPrefPath("GwentClassic", "MiyooMiniPlus")`. The engine save API can also be called with an explicit path. Writes go to a temporary file and replace the previous save only after the new data is complete. `S`/short SELECT saves, `O`/START loads, and `R` on deck selection resumes the save. Hold SELECT for 500 ms or press `I` to inspect the selected card at a larger scale; `B`/ESC closes the inspector. The save format stores the complete engine match state; UI language and menu selection are not part of the match save.

## User deck data API

`gwent/deck.hpp` exposes validated user deck recipes and checksummed `.gwd` persistence. Recipes store a display name, faction, leader reference, and card reference/count pairs from the pinned Witcher 3 Gwent catalog; `materializeUserDeck` resolves them back to the catalog entries before starting a match. The SDL frontend currently has three deck slots and an in-progress card-bank editor; see `PROGRESS.md` for remaining gaps.

## Miyoo Mini Plus / OnionOS

Wymagany jest 32-bitowy ARM hard-float (arm-linux-gnueabihf) oraz wersje bibliotek zgodne z docelowym OnionOS. Referencyjny SDL z `steward-fu/sdl2` deklaruje wsparcie dla Miyoo Mini/Plus i OnionOS v4.3.1-1. Repozytorium SDL jest przypięte do `0631abc8e8916db6f9bc7e2afd0c22913d092a29`, a oficjalny toolchain `mini_toolchain-v1.0` do SHA-256 `8addff71be4b015a4e1aef51e43635e50978d558a1675f5b1664124e8437d071`. Toolchain wymaga ścieżek `/opt/mini` i `/opt/prebuilt`.

Po pobraniu i sprawdzeniu toolchaina oraz checkoutcie powyższej rewizji SDL, w środowisku Linux z CMake, Python 3, zip, binutils i git uruchom:

```sh
bash tools/build_onion.sh /opt /path/to/steward-fu-sdl2 out/miyoo /path/to/mini_toolchain-v1.0.tar.gz
```

Skrypt kompiluje frontend z biblioteką SDL Miyoo, wymusza zgodność rewizji i sumy SDK, weryfikuje ELF32 ARM hard-float, normalizuje ścieżki/końce linii/metadane ZIP i tworzy SHA-256 paczki. Workflow `Miyoo OnionOS package` wykonuje te kroki i publikuje paczkę jako artefakt Actions. Referencyjna biblioteka SDL ma zależności SigmaStar `libmi_*`, które dostarcza system Miyoo/OnionOS; nie kopiuje się ich z niezweryfikowanego obrazu urządzenia.

Wydanie korzysta z standalone launcher template OnionOS, `Roms/PORTS/Games/GwentClassic/`, `Roms/PORTS/Shortcuts/Cards/GwentClassic.notfound` i oficjalnego kroku `~Import ports`. Pakiet obejmuje SDL2, EGL/GLES, json-c oraz biblioteki GCC runtime wraz z licencjami. Sama poprawna kompilacja i instalacja na karcie nie potwierdzają jeszcze startu gry ani wejścia/audio na fizycznym Miyoo.

Skrót portu ustawia `KillAudioserver=1`: przypięty SDL Mini otwiera bezpośrednio urządzenie MI_AO, a log z urządzenia pokazał błąd `0xa0052009` (`MI_AO_ERR_NOT_PERM`), gdy serwer OnionOS nadal zajmował wyjście audio. Standardowy launcher OnionOS zatrzymuje serwer przed startem portu. Trzeba jeszcze potwierdzić odtwarzanie na konsoli i zachowanie audio po wyjściu z gry.

Launcher zapisuje stdout i stderr uruchomienia do `/mnt/SDCARD/Saves/Ports/GwentClassic/last-run.log`, a gdy ten folder nie jest zapisywalny, do `Roms/PORTS/Games/GwentClassic/last-run.log`. Niepowodzenie zapisu logu nie zatrzymuje uruchamiania gry. Plik zawiera punkty kontrolne inicjalizacji SDL, okna i renderera oraz komunikaty o błędach, które zgłosi proces lub dynamiczny loader.

Skrót OnionOS zapisuje osobny log `shortcut.log` w tym samym katalogu Saves, z kopią awaryjną w katalogu gry. Rejestruje wejście do skrótu, obecność launchera i binarki oraz kod wyjścia launchera; ten log pozwala odróżnić problem importu/uruchomienia skrótu od błędu samej aplikacji.

## Odtwarzalność

Audyt referencyjnych commitów i ograniczeń licencji znajduje się w `PROGRESS.md` i `LICENSES.md`. Pakiet zapisuje rewizję SDL, sumę SDK oraz SHA-256 artefaktu; końcowa QA urządzenia oraz wydanie GitHub Release pozostają do wykonania.
