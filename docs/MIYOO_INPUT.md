# Wejście Miyoo Mini Plus

## Backend

Przypięty SDL2 dla Miyoo używa sterownika wideo `Mini`. Jego `SDL_event_mini.c`
odczytuje `/dev/input/event0` i wysyła zdarzenia klawiatury SDL. Nie należy więc
zakładać, że przyciski urządzenia pojawią się jako `SDL_CONTROLLERBUTTON*`.
Źródło SDL jest pobierane do `.work/sdl2` przez workflow i przypięte do rewizji
opisanej w `BUILD.md`.

Praktyczny pomiar klawiszy na Miyoo Mini Plus jest opublikowany w
[mmp-sdl.csv](https://gist.github.com/bostrt/8cbfcea6c6dbd6d3c50669e84b9863ca).
Wskazuje on m.in. A jako lewy Ctrl, B jako lewy Alt, X jako lewy Shift, Y jako
Spację, Start jako Enter, Select jako Escape, L jako Tab, L2/R2 jako PageDown/
PageUp, R jako Backspace, a przycisk MENU jako Home. To źródło społecznościowe;
nie zastępuje odczytu `event0` na konkretnej wersji firmware.

## Mapowanie portu

| Miyoo | Zdarzenie SDL Mini | Akcja w grze |
|---|---|---|
| D-pad | strzałki | W edytorze góra/dół wybiera kartę, lewo/prawo przełącza bank i talię; w meczu wybiera kartę/rząd |
| A | Left Ctrl | Zatwierdź / zagraj; podczas mulliganu zaznacza kartę |
| B | Left Alt | W edytorze: przejście z banku do talii albo usunięcie kopii w panelu talii; zamyka podgląd karty. W meczu anuluje bieżący wybór lub zapisuje mecz i wraca do edytora |
| X | Left Shift | Otwiera/zamyka czytelny podgląd karty |
| Y | Space | Zmienia frakcję w edytorze; używa zdolności lidera w meczu; zatwierdza/pomija wymianę w mulliganie |
| L | Tab | Poprzedni rząd w meczu; wybór lidera w edytorze |
| R | Backspace | Następny rząd w meczu; nawigacja w edytorze |
| L2 / R2 | PageDown / PageUp | L2: następny, R2: poprzedni slot talii; w meczu L2 przewija rękę o pięć kart naprzód, R2 wstecz |
| Start | Enter | Rozpoczyna mecz z edytora; otwiera pauzę w meczu; zatwierdza/pomija mulligan |
| Select | Escape | Zapis talii w edytorze; zamyka podgląd, jeśli jest otwarty. W meczu otwiera potwierdzenie pasowania lub zatwierdza wybór fazy |
| MENU | `KEY_HOME` → `SDLK_HOME` przez lokalną poprawkę SDL | Zapisuje mecz (jeśli trwa) i zamyka grę, aby OnionOS odzyskał kontrolę |

Jest jeden ekran przygotowania talii. Usunięto dodatkowy ekran `DeckSelect` z
wcześniejszego prototypu. W edytorze `P` wczytuje następny preset, `R` wznawia
zapisany mecz, `Z` zmienia język, a `F` zmienia frakcję. Te klawisze są dostępne
na klawiaturze; fizyczne Y zmienia frakcję, a język jest domyślnie polski.

Klawisz Select nie pasuje automatycznie: najpierw otwiera pytanie, A zatwierdza,
a B anuluje. Na ekranie mulliganu lewo/prawo wybiera kartę, A ją zaznacza, a
Start, Y lub Select zatwierdza wybór albo pomija wymianę, gdy nic nie
zaznaczono. X otwiera tekstowy inspektor karty. Wbudowany smoke `--input-test`
sprawdza mapowanie klawiszy Mini SDL i alternatywnych zdarzeń gamepada dla obu
ekranów, zachowanie B zależnie od panelu, pauzę, bezpieczne potwierdzanie
pasowania oraz ochronę wyboru celu.

## Status weryfikacji

Przypięty sterownik nie mapował `KEY_HOME`; projekt dodaje patch
[`patches/miyoo-sdl-keyhome.patch`](../patches/miyoo-sdl-keyhome.patch), buduje z
nim bibliotekę SDL i dołącza patch do paczki. ELF SDL jest ARM32 hard-float, a
aplikacja zapisuje mecz i kończy działanie po `SDLK_HOME`. Fizycznego naciśnięcia
MENU oraz zachowania OnionOS nadal trzeba potwierdzić na konsoli.
