# Płynność, sterowanie i wyniki — 2026-11-17

Wersja kodu: `2a5c7f7`. Desktop CI `44090905300` oraz ARM/OnionOS CI
`44090908557` zakończyły się powodzeniem. Zakres: wydajność prezentacji,
kontrakt przycisków i geometria wyników; zasady i wagi AI nie zostały zmienione.

## Potwierdzone problemy i zmiany

- Plansza liczyła siłę kart dwa razy w każdej klatce, skanując ponownie rzędy.
  Renderer pamięta teraz wyniki rzędu i porównuje dokładny stan kart oraz
  pogodę/róg/efekty lidera. Zmiana karty w miejscu również unieważnia cache.
- AI blokowało główną pętlę na czas symulacji. Wątek SDL liczy decyzję na
  własnej kopii. Główny wątek stosuje ją i stan RNG tylko po sprawdzeniu
  generacji meczu, fazy, gracza, medyka i początkowego RNG. Zmiana lub
  wczytanie meczu nie czeka na stary wątek ani nie przyjmuje jego decyzji.
- Muzyka była dekodowana w całości na głównym wątku i przechowywana w RAM.
  Teraz ma osobny dekoder i stały ring PCM 65 536 bajtów. Callback nie
  dekoduje plików, nie czyta dysku i nie alokuje pamięci. Zmiana utworu
  odrzuca porcje starej generacji. Małe SFX są przygotowywane podczas
  istniejącego ekranu ładowania, zamiast podczas pierwszej animacji.
- Usunięto bezwarunkowe dodatkowe 8 ms po prezentacji/VSync. Pętla czeka
  tylko przez niewykorzystaną część budżetu klatki 60 Hz.
- Co pięć sekund log Miyoo zapisuje liczbę klatek, średni/maksymalny czas
  pracy, klatki ponad 33 ms, dekodowania tekstur, przebudowy rzędów,
  aktywność AI i statystyki bufora muzyki. Pozwala to zmierzyć rzeczywiste
  przycięcia na urządzeniu zamiast zgadywać z wyników komputera.

## Przyciski

Aktualna tabela znajduje się w `MIYOO_INPUT.md`. B zatwierdza, A cofa/anuluje.
Pauza i edytor nie zamieniają już tych ról. Wymiana kart używa B do
zaznaczania, A do cofnięcia i Y do zakończenia/pominięcia. X otwiera podgląd;
przytrzymanie 600 ms pasuje tylko w swobodnej własnej turze. Otwarcie pauzy
lub modala kasuje rozpoczęte przytrzymanie, więc X nie pasuje przez okno.
Gamepad SDL ma ten sam krótki/długi gest X. Podpowiedzi nie używają już
skompresowanych do trzech pikseli liter; stopka mulliganu mieści się w viewport.

## Ekrany wyników

Poprzednio notyfikacja była rysowana po wyniku: jej obszar y154–205 zasłaniał
punktację y174. Loty kart i efekty również mogły zakryć wynik. Wynik ma teraz
osobną warstwę całego ekranu, wyświetlaną po zakończeniu właściwej animacji.
Nie renderuje nad sobą powiadomień, podglądu kart ani lotów.

Ornament zachowuje proporcje 656×416 → 224×142. Jego angielski napis jest
zastąpiony tekstem. Tytuł, dwa pola punktów, osiągnięcie i podpowiedź B/A mają
osobne, wycentrowane obszary. Zrzuty utrwalają pełne krycie po zakończeniu fade,
obejmują wygraną, przegraną, remis, remis punktowy Nilfgaardu oraz kampanię.

## Weryfikacja i jej granice

- Wszystkie wymagane testy silnika i frontendu CI przeszły.
- Test porównuje decyzję i końcowy RNG wątku AI z wersją synchroniczną oraz
  odrzucenie decyzji nieaktualnej generacji bez zmiany RNG żywego meczu.
- Test muzyki sprawdza faktycznie zdekodowane dane, przejście menu/walka,
  wyciszenie, ponowne włączenie, zapętlenie krótkiego MP3 i limit bufora.
- Test renderuje prawdziwą planszę: 108 jednostek i 10 kart ręki przez
  60 klatek. Po rozgrzaniu: zero kolejnych dekodowań i zero ponownych
  obliczeń niezmienionych rzędów. Pogoda przebudowuje dwa właściwe rzędy,
  zmiana siły karty w miejscu jeden. Test działa również z kompozycją programową.
- Wyniki z aktywną notyfikacją, lotem i inspektorem są pikselowo identyczne
  z czystym wynikiem. Sprawdzenie przechodzi w obu trybach kompozycji.
- Testy przycisków obejmują modalne potwierdzanie/cofanie, edytor, pauzę,
  mulligan oraz blokadę długiego X. Zrzuty wygranej i przegranej obejrzano.

Pomiar CI nie jest pomiarem FPS Miyoo. Płynność przy różnych efektach,
odsłuch zapętlenia i rzeczywiste przyciski wymagają ponownej próby na konsoli.
Do rozstrzygnięcia pozostałych przycięć służy nowa telemetria w `last-run.log`.

## Instalacja SD i weryfikacja po ponownym uruchomieniu komputera

Pakiet wgrano na kartę `MIYOO MINI`, FAT32. Po zgłoszonym restarcie komputera
wykonano niezależną kontrolę wszystkich 626 plików bez ponownego kopiowania:
każdy SHA-256 odpowiada plikowi z archiwum. Brak plików niekompletnych lub
niezgodnych. Raport lokalny:
`artifacts/qa/package-2a5c7f7-connector/post-restart-verification.json`.

Archiwum SHA-256:
`bc60f61c1ec136239609b394a3a819ffd229ab190b447451dcbca4ae9b85406d`.

Plik gry na SD SHA-256:
`ea2fe4c7e25571a0b1311bdb09aaa93f84c43e13a4eee33d2998bbe16db951f4`.

Kopia poprzedniej instalacji:
`artifacts/qa/package-2a5c7f7-connector/sd-backup-20261117-143603`.
Kopia danych użytkownika: `artifacts/qa/package-2a5c7f7-connector/user-save-backup`.
Pobranie awaryjne przez aplikację GitHub zachowało zgodność SHA-256 również
zewnętrznego archiwum artefaktu; nie wyłączano walidacji TLS.
