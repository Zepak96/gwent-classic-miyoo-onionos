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
| D-pad | strzałki | W edytorze porusza się po siatce kart; przejście przez krawędź łączy kolekcję z talią. W meczu wybiera kartę; w podglądzie zagrania góra/dół zmienia rząd, jeśli karta pozwala go wybrać |
| A | Left Alt | Anuluje podgląd zagrania, wyboru celu lub zdolności przywódcy; podczas mulliganu zaznacza kartę |
| B | Left Ctrl | W meczu otwiera wybór karty, a następne B zagrywa ją do podświetlonego rzędu. Potwierdza też wybrany cel, kartę przywódcy i pasowanie; w edytorze wraca z talii do kolekcji |
| X | Left Shift | Krótkie naciśnięcie otwiera podgląd karty; przytrzymanie przez 600 ms podczas swojej tury od razu pasuje |
| Y | Space | Zmienia frakcję w edytorze; w meczu otwiera podgląd zdolności przywódcy, który B potwierdza, a A anuluje; zatwierdza/pomija wymianę w mulliganie |
| L | Tab | W meczu zmienia wybrany rząd dla karty z wyborem rzędu; w edytorze otwiera wybór lidera |
| R | Backspace | W meczu zmienia wybrany rząd dla karty z wyborem rzędu; w edytorze przesuwa kursor w dół |
| L2 / R2 | PageDown / PageUp | L2: następny, R2: poprzedni slot talii; w meczu L2 przewija rękę o pięć kart naprzód, R2 wstecz |
| Start | Enter | Rozpoczyna mecz z edytora; otwiera pauzę w meczu; zatwierdza/pomija mulligan |
| Select | Escape | Zapis talii w edytorze; zamyka podgląd, jeśli jest otwarty. W meczu otwiera potwierdzenie pasowania lub zatwierdza wybór fazy |
| MENU | `KEY_HOME` → `SDLK_HOME` przez lokalną poprawkę SDL | Zapisuje mecz (jeśli trwa) i zamyka grę, aby OnionOS odzyskał kontrolę |

Jest jeden ekran przygotowania talii. Usunięto dodatkowy ekran `DeckSelect` z
wcześniejszego prototypu. W edytorze `P` wczytuje następny preset, `R` wznawia
zapisany mecz, `Z` zmienia język, a `F` zmienia frakcję. Te klawisze są dostępne
na klawiaturze; fizyczne Y zmienia frakcję, a język jest domyślnie polski.

Klawisz Select nie pasuje automatycznie: najpierw otwiera pytanie, B zatwierdza,
a A anuluje. Przytrzymanie X przez 600 ms pasuje bez dodatkowego potwierdzenia. Na ekranie
mulliganu lewo/prawo wybiera kartę, A ją zaznacza, a Start, Y lub Select
zatwierdza wybór albo pomija wymianę, gdy nic nie zaznaczono. Krótkie naciśnięcie
X otwiera tekstowy inspektor karty. Wbudowany smoke `--input-test`
sprawdza mapowanie klawiszy Mini SDL i alternatywnych zdarzeń gamepada dla obu
ekranów, zachowanie B zależnie od panelu, pauzę, bezpieczne potwierdzanie
pasowania oraz ochronę wyboru celu. Test sprawdza założone kody SDL, nie odczyt
fizycznych przycisków konkretnej konsoli; wymaga to osobnego testu na Miyoo.

## Status weryfikacji

Przypięty sterownik nie mapował `KEY_HOME`; projekt dodaje patch
[`patches/miyoo-sdl-keyhome.patch`](../patches/miyoo-sdl-keyhome.patch), buduje z
nim bibliotekę SDL i dołącza patch do paczki. ELF SDL jest ARM32 hard-float, a
aplikacja zapisuje mecz i kończy działanie po `SDLK_HOME`. Fizycznego naciśnięcia
MENU oraz zachowania OnionOS nadal trzeba potwierdzić na konsoli.

## Odczyt na konsoli użytkownika — 2026-11-17

Użytkownik zgłosił naciśnięcie B przy wyjściu z menu. Log urządzenia zawiera
`raw=Left Ctrl code=1073742048 normalized=X`. Wersja zainstalowana na SD
interpretowała ten klawisz jako anulowanie/wyjście. W mapowaniu portu B jest
teraz Left Ctrl, A Left Alt, zgodnie z tym zgłoszeniem i odczytem. Jest to
korekta dla testowanego urządzenia, a nie potwierdzenie wszystkich firmware.
Menu dodatkowo akceptuje oba przyciski jako zatwierdzenie, żeby omyłkowy
przycisk nie kończył procesu. Regresje używają odpowiednio zmienionych kodów
SDL; rzeczywiste A/B w meczu nadal wymagają sprawdzenia na konsoli.

## Wgranie poprawki menu/mapowania — 2026-11-17

Wersja 150843d: desktop CI 44055543319 i ARM 44055543930 zakończone sukcesem.
Na karcie E: MIYOO MINI zweryfikowano SHA-256 wszystkich 607 plików paczki.
SHA-256 archiwum: 0e86021342e628c6020378f02e543d6c15a702d10accc9717165992363221ffb.
Poprzednią wersję zachowano lokalnie w
artifacts/qa/package-150843d/sd-backup-20261117-103028.
Odczyt logu i zgłoszenie B wskazały odwrotną interpretację A/B; sama instalacja
nie potwierdza jeszcze faktycznych przycisków w rozgrywce po aktualizacji.
