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
| D-pad | strzałki | Nawigacja kart/rzędów/menu |
| A | Left Ctrl | Zatwierdź / zagraj |
| B | Left Alt | Wróć / anuluj |
| X | Left Shift | Podgląd karty; w menu talii otwiera edytor |
| Y | Space | Zdolność lidera w meczu; język w menu talii |
| L | Tab | Zmiana fokusu w meczu |
| R | Backspace | Zmiana fokusu w meczu; nawigacja w edytorze |
| L2 / R2 | PageDown / PageUp | Przewijanie stron/ręki |
| Start | Enter | Pauza/ustawienia podczas meczu; zatwierdzenie w menu |
| Select | Escape | Pasowanie po dodatkowym potwierdzeniu; zapis talii w edytorze |
| MENU | brak mapowania `KEY_HOME` w przypiętym `SDL_event_mini.c` | Nieobsługiwane przez obecną konfigurację SDL; wymaga poprawki backendu i testu urządzenia |

Klawisz Select nie pasuje automatycznie: najpierw otwiera pytanie, A zatwierdza,
a B anuluje. Przytrzymanie Select przez 500 ms nadal otwiera inspektor karty w
meczu i edytorze. Wbudowany smoke `--input-test` sprawdza translację klawiszy
Mini SDL, pauzę, bezpieczne potwierdzanie pasowania oraz ochronę wyboru celu.

## Status weryfikacji

Mapowanie A/B/X/Y i klawiszy systemowych wynika z publicznego pomiaru; fizyczne
naciśnięcia na Miyoo trzeba nadal potwierdzić na urządzeniu. Aplikacja zapisuje
mecz i kończy działanie po zdarzeniu `SDLK_HOME`, ale przypięty sterownik Mini
nie mapuje `KEY_HOME`, więc nie ma dowodu, że przycisk MENU może obecnie wysłać
to zdarzenie. To pozostaje konkretnym zadaniem portu.
