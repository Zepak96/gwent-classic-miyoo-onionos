# Kampania — cztery wieże, wydanie testowe 2026-11-17

Kod pakietu: `0fd4cba`. Desktop CI `44082434774` i ARM CI `44082436337`
zakończyły się powodzeniem. Pakiet wgrano na kartę `MIYOO MINI`, FAT32,
narzędziem `tools/install_verified_sd.ps1`: 626 plików zweryfikowanych SHA-256.

## Zgłoszenie z urządzenia i poprawiony przebieg

Użytkownik zgłosił brak odblokowania Ves po zwycięstwie i komunikat
„Talia nieprawidłowa” przy próbie uruchomienia Talara. Odczytany sidecar
ostatniego zapisanego meczu zawierał ID `-`, czyli zwykłą partię. Nie było
pliku postępu kampanii. Log i sidecar zachowano lokalnie w
`artifacts/qa/campaign-entry-device-20261117/`.

Wybór rywala prowadzi teraz do wyboru talii. Niekompletna własna talia nie
blokuje wyboru gotowej, legalnej talii. Y otwiera edytor bieżącej talii bez
kasowania niezapisanych zmian. Nad planszą wyświetlane są przeciwnik i etap
kampanii; zwykła partia ma osobne, jednoznaczne oznaczenie.

B obsługuje wynik rundy i meczu przed ogólną obsługą zagrywania kart.
Powrót po zakończeniu meczu ustawia właściwą wieżę i etap oraz czyści stan
wyboru talii. Zwykłe mecze nie przyznają osiągnięć kampanii.

## Zakres kampanii i grafika

Cztery niezależne wieże po pięć etapów: Królestwa Północy, Nilfgaard,
Potwory i Skellige. Każda korzysta z legalnych przepisów własnej frakcji;
obowiązują źródłowe wymagania talii. Każde zwycięstwo ma osobne osiągnięcie.

W pakiecie są wszystkie 20 portretów. Talar jest wzorcem stylu; poprawiono
Dijkstrę i Morvrana, uzupełniono brakujące ilustracje, a gryf nie ma już
korony ani szaty. Region portretu ma proporcje 3:4, bez dawnego kwadratowego
kadrowania. Prompty i pliki wyjściowe: `campaign-art-style-lock.json`.

## Przeprowadzone sprawdzenia

- Legalność wszystkich talii, niezależne przejście czterech wież, zapis
  i wczytanie po każdym z 20 zaliczeń, odrzucanie duplikatów i błędnych zapisów.
- UI: niekompletna własna talia → wybór gotowej → rzeczywisty mecz Talara.
  Dwie rundy rozstrzygnięte przez silnik: pierwsza nie daje nagrody,
  druga daje zwycięstwo, odblokowuje Ves i zapisuje postęp.
- Deterministyczna awaria I/O: bez udanego zapisu nie można utracić wyniku
  przez wyjście lub zastąpienie go inną partią; poprawne ponowienie zapisuje wygraną.
- Nieudany zapis metadanych meczu jest zgłaszany jako błąd. Metadane są
  podmieniane atomowo; istniejąca błędna asocjacja nie zamienia się po cichu
  w zwykły mecz. Istniejący uszkodzony postęp kampanii jest chroniony przed
  nadpisaniem i kopiowany do pliku diagnostycznego.
- Reset stanu pickera po wznowieniu i powrocie. Ustawienie właściwej wieży
  przy powrocie z innej frakcji sprawdzono osobną fixture; nie jest to dowód
  pełnego sprzętowego wznowienia wszystkich etapów.
- 20 ekranów rywali, wybór talii i nagłówek meczu wyrenderowano w 640×480
  w obu trybach kompozycji. Przejrzano zestaw 20 ekranów i pełne zrzuty
  wyboru talii oraz meczu. Naprawiono renderowanie długich myślników.
- Testy wejścia Miyoo, zatłoczonej planszy, dekodowania SFX i muzyki oraz
  pozostałe wymagane sprawdzenia CI przeszły.
- Archiwum zawiera 32-bitowy ARM ELF, zgodne ze źródłem 20 portretów,
  muzykę menu/walki i skrypt uruchamiania bez CRLF.

## Instalacja i granice weryfikacji

Archiwum SHA-256:
`1c8fe732e63abcc9c010048fc0142b41c3f105fb5c50d46a5cd280529c05331e`.

Plik gry na SD SHA-256:
`e0130114cc502a217aafdda67f4e254ba7d663c033867df489f8b5f987dedd00`.

Kopia starej instalacji: `artifacts/qa/package-0fd4cba/sd-backup-20261117-132603`.
Kopia danych użytkownika: `artifacts/qa/package-0fd4cba/user-save-backup`.
Instalacja nie zastępuje zapisów ani ustawień użytkownika.

To wydanie do testów na urządzeniu. CI i zrzuty nie potwierdzają pełnego
przejścia 20 pojedynków na Miyoo, wydajności sprzętowej w każdej sytuacji
ani rozwiązania wszystkich historycznych problemów całego portu.
