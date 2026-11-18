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

## Aktualny kontrakt przycisków — 2026-11-17

B zatwierdza wybór lub akcję, A cofa/anuluje. X i Y uruchamiają akcje
pomocnicze opisane na ekranie. Kod SDL `SDLK_x` jest wewnętrznym aliasem A,
nie nazwą fizycznego X; `SDLK_AC_BACK` oznacza fizyczne B.

| Przycisk | Kod SDL Mini | Działanie |
| --- | --- | --- |
| B | Left Ctrl | Menu/kampania: zatwierdź. Edytor: dodaj/usuń kartę lub zatwierdź lidera. Mecz: podgląd zagrania, następne B zagrywa do podświetlonego rzędu; wybór celu/lidera/pasu. Pauza: zatwierdź opcję. Wynik: kontynuuj. Mulligan: zaznacz/odznacz kartę. |
| A | Space | Anuluj podgląd/wybór. Edytor: z talii do kolekcji, potem do poprzedniego ekranu; podgląd lidera cofa niezapisany wybór. Pauza: wróć. Mulligan: cofnij zaznaczenie, a bez zaznaczeń otwórz pauzę. Na ręce lub wyniku rundy otwiera pauzę. |
| X | Left Shift | Krótko: podgląd karty. Przytrzymanie 600 ms w swobodnej własnej turze: pas. Pauza, inspektor i inne modale blokują ten gest; zmiana kontekstu kasuje rozpoczęte przytrzymanie. |
| Y | Left Alt | Edytor: frakcja. Kampania: edytuj własną talię. Mecz: podgląd zdolności lidera, B potwierdza, A anuluje. Mulligan: zakończ/pomiń wymianę. |
| D-pad | Strzałki | Nawigacja po kartach i polach, wybór rzędu tam, gdzie pozwala karta. |
| L / R | Tab / Backspace | Edytor: L otwiera/zamyka wybór lidera; zamknięcie L anuluje niezapisany wybór. R przesuwa kursor w dół. W meczu odpowiadają wyborowi rzędu góra/dół. |
| L2 / R2 | PageDown / PageUp | Następny/poprzedni slot talii; w meczu przewijanie ręki po pięć kart. |
| Start | Return | Edytor: rozpocznij partię. Mecz: pauza; podczas mulliganu zakończ wymianę. Nie jest przyciskiem opisanym jako kontynuacja wyniku. |
| Select | Escape | Edytor: zapisz talię. Mecz: potwierdzenie pasowania/akcja bieżącej fazy; dłuższe przytrzymanie otwiera inspektor. Menu główne: wyjście. |
| MENU | Home | Zapis meczu i powrót do OnionOS. |

Przy obowiązkowym wyborze wynikającym z już zagranej zdolności cofnięcie nie
odwraca jej skutków; obowiązują zasady źródłowego Gwent Classic. Gamepad SDL
stosuje ten sam kontrakt A/B i krótki/długi X. Klawiatura zachowuje pomocnicze
Enter/Escape/F5/F9.

Test `--input-test` sprawdza rzeczywiste przejścia modalne i kody Mini SDL:
B/B na karcie, A bez zagrania, B w pauzie, A zamykające pauzę, B w edytorze,
A cofające lidera, mulligan i ochronę długiego X w modalach. Test menu
sprawdza, że A nie zatwierdza opcji. Są to testy programowe; weryfikację
fizyczną trzeba wykonać na docelowym Miyoo.

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
Historyczna poprawka menu akceptowała oba przyciski jako zatwierdzenie.
Obecna wersja zastępuje to kontraktem B: zatwierdź, A: cofnij. Regresje używają odpowiednio zmienionych kodów
SDL; rzeczywiste A/B w meczu nadal wymagają sprawdzenia na konsoli.

## Wgranie poprawki menu/mapowania — 2026-11-17

Wersja 150843d: desktop CI 44055543319 i ARM 44055543930 zakończone sukcesem.
Na karcie E: MIYOO MINI zweryfikowano SHA-256 wszystkich 607 plików paczki.
SHA-256 archiwum: 0e86021342e628c6020378f02e543d6c15a702d10accc9717165992363221ffb.
Poprzednią wersję zachowano lokalnie w
artifacts/qa/package-150843d/sd-backup-20261117-103028.
Odczyt logu i zgłoszenie B wskazały odwrotną interpretację A/B; sama instalacja
nie potwierdza jeszcze faktycznych przycisków w rozgrywce po aktualizacji.

## Korekta A/Y — 2026-11-18

Poprzednie przypisanie A do Left Alt było błędne: powodowało otwarcie
edytora zamiast powrotu z wieży. Aktualny domyślny układ A/B/X/Y to
Space / Left Ctrl / Left Shift / Left Alt. Potwierdza go oficjalny
[OnionUI keymap_hw.h](https://github.com/OnionUI/Onion/blob/main/src/common/system/keymap_hw.h#L9-L12)
i przypięty SDL_event_mini.c. Ta korekta zastępuje wcześniejsze założenia
o A i Y opisane powyżej. B nadal zatwierdza, A cofa.

Ustawienia zawierają kalibrację czterech fizycznych przycisków. Format V4
zapisuje unikatowe kody; starsze ustawienia zachowują aktualny default.
Test settings-flow sprawdza również odmienną kalibrację, trwałość zapisu,
powrót z kampanii oraz anulowanie i atomowe wykonanie resetu progresji.
