# Build

## Status

Projekt używa C++17 i CMake. Konfiguracja zakłada opcjonalny frontend SDL2 i niezależną bibliotekę silnika. Na środowisku, w którym utworzono ten plik, nie ma wykrytego CMake ani kompilatora C++, więc kompilacja nie została tu wykonana.

## Desktop

Wymagania: CMake 3.16+, kompilator C++17, Python 3, SDL2. Python generuje natywny katalog kart na etapie kompilacji. Z katalogu projektu:

```sh
cmake -S . -B build -DGWENT_WITH_SDL=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Bez SDL2 można budować same testy i silnik: ustaw `-DGWENT_WITH_SDL=OFF`.

## Miyoo Mini Plus / OnionOS

Wymagany jest 32-bitowy ARM hard-float (arm-linux-gnueabihf) oraz wersje bibliotek zgodne z docelowym OnionOS. Referencyjny SDL z `steward-fu/sdl2` deklaruje wsparcie dla Miyoo Mini/Plus i OnionOS v4.3.1-1; trzeba potwierdzić docelową wersję systemu i biblioteki na urządzeniu. Ta konfiguracja nie produkuje jeszcze binarki ARM.

Wydanie powinno korzystać z oficjalnego standalone launcher template OnionOS, `Roms/PORTS/Games/<nazwa>/`, `Shortcuts/<kategoria>/<nazwa>.notfound` i oficjalnego kroku `~Import ports`. Nie należy kopiować przykładowego launchera bez sprawdzenia jego argumentów i wymaganych bibliotek.

## Odtwarzalność

Audyt referencyjnych commitów i ograniczeń licencji znajduje się w `PROGRESS.md` i `LICENSES.md`. Wydanie musi zapisać commit toolchaina, wersje SDL oraz SHA-256 paczki; nie są jeszcze dostępne.
