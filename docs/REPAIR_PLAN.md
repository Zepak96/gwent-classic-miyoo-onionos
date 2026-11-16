# Plan naprawy grywalności portu na Miyoo Mini Plus

Ten dokument zapisuje zgłoszone problemy, ustalenia z przeglądu kodu oraz
kolejność naprawy. Podstawą zgodności pozostaje przypięte repozytorium
`upstream/asundr-gwent-classic/`; zmiany mają odtwarzać jego zasady i przepływ
gry, a nie wprowadzać nowe reguły. Zgłoszenia z urządzenia są oznaczone jako
objawy, dopóki nie da się ich odtworzyć w kodzie lub logu.

## Zgłoszone objawy

- Jedna zagrana karta może wyglądać tak, jakby po stronie przeciwnika pojawiło
  się kilka kart; po zagraniu przesuwa się też kilka kart w ręce.
- Suma rzędu/siła wygląda czasem na niezgodną z widoczną siłą karty, np. 16
  przy karcie o sile 8.
- Talia lub frakcja przeciwnika wydaje się pomieszana; Geralt miał przywołać
  Ciri.
- Nie wiadomo, jak spasować; oczekiwane jest przytrzymanie X, a podpowiedzi
  przycisków nie są widoczne.
- Karta natychmiast trafia na planszę po zatwierdzeniu, a podświetlenie rzędu
  pozwala zmieniać rząd niezależnie od wybranej karty.
- Zagrana pogoda, w szczególności Trzaskający Mróz, bywa słabo widoczna;
  podczas ruchu gra wyglądała na zawieszoną.
- Nawigacja po kolekcji i talii jest nieintuicyjna, a zmiana lidera nie działa
  lub nie jest zrozumiała.
- Wartości siły na kartach czasem są przesunięte i mają niespójny krój/skale.
- Siła kart nadal bywa nierówno wycentrowana; cyfry zmieniają rozmiar lub krój,
  szczególnie gdy wartość jest dwucyfrowa.
- Użytkownik zgłasza, że fizyczne przyciski B/A nie zachowują się jak opisane.

## Potwierdzone ustalenia z kodu i źródła

1. **Muster — błąd potwierdzony i poprawiany.** Źródło w `abilities.js` wybiera
   karty, których nazwy zaczynają się od klucza Muster. Stary kod C++ porównywał
   również pole `group`; Geralt i Ciri mają oba `group="Roach"`, więc mogły
   błędnie przywołać siebie nawzajem. Poprawka używa źródłowego porównania po
   prefiksie nazwy. Regresja musi wykazać, że Geralt przywołuje Roacha, a Ciri
   pozostaje w ręce.
2. **Tajemniczy Elf nie dowodzi pomieszania frakcji.** Katalog źródłowy oznacza
   go jako neutralnego szpiega, więc może wystąpić w talii Nilfgaardu.
3. **Przeciwnik jest losowany.** Tak działa również oryginalne repozytorium.
   Edytor pokazuje obecnie przewidywaną, sąsiednią talię, która nie musi być
   tą wylosowaną po rozpoczęciu meczu. Interfejs ma jasno oznaczyć losowanie,
   a mecz ma pokazywać faktyczną frakcję i lidera.
4. **Nawigacja edytora ma błąd potwierdzony w kodzie.** Lewo/prawo zawsze
   przełącza panel zamiast poruszać się po kartach. Góra odejmuje pełny wiersz,
   ale dół zwiększa indeks tylko o jedną kartę. Wybór lidera ma tryb
   przełączany klawiszem L/Tab, ale Miyoo A nie zatwierdza ani nie zamyka tego
   trybu; brak czytelnych wskazówek na ekranie.
5. **Zmiana rzędu jest myląca.** Strzałki góra/dół zmieniają `rowIndex` bez
   względu na zaznaczoną kartę. Silnik poprawnie ignoruje ten wybór dla zwykłej
   jednostki i używa jej źródłowego rzędu, lecz podświetlenie sugeruje co
   innego. Wolno wybierać rząd tylko dla kart Agile albo kart specjalnych,
   które rzeczywiście przyjmują cel rzędu.
6. **Wartości siły używały różnych ścieżek rysowania.** Bohaterowie mieli
   wyśrodkowany tekst skalowany 1×/2×, a zwykłe jednostki osobne, obrysowane
   rysowanie bitmapowe. Wspólny renderer już zastąpił te ścieżki, ale ostatni
   przegląd wykazał, że nadal powiększał pojedyncze cyfry 2×, a dwucyfrowe
   rysował 1×. To powodowało zmianę rozmiaru przy przejściu np. z 9 na 10.
   Skala została ujednolicona do 1×; centrowanie uwzględnia szerokość całej
   wartości i cień pozostaje identyczny dla bohaterów oraz zwykłych jednostek.
   Nadal wymaga to zrzutu z aktualnego builda 640×480, bo nie ma lokalnego
   kompilatora ani świeżego obrazu z urządzenia.
7. **Fizyczne mapowanie nadal nie jest potwierdzone na konsoli.** Obecny test
   wejścia wywołuje założone kody SDL, nie czyta rzeczywistych zdarzeń Miyoo.
   Zgłoszenie, że B zagrywa kartę, jest sprzeczne z mapowaniem wpisanym w
   projekcie i wymaga logu rzeczywistego `SDL_KEYDOWN/UP`.
8. **Pogoda jest modelowana w silniku.** Mróz ustawia flagę rzędu zwarcia,
   zmienia wynik i powinien rysować nakładkę. Pętla rysowania nie wykazuje
   martwej blokady, ale blokada sterowania może trwać ok. 2 s dla pogody i
   ok. 3 s dla jednostek przez lot, dodatkowe 600 ms oraz oczekiwanie na audio.
   To może wyglądać jak zwis; trzeba skrócić blokadę do rzeczywistego końca
   animacji i pokazywać czytelny stan animacji.
9. **Ruch kilku kart może być skutkiem efektu albo animacji.** Muster ma
   rozgrywać karty z tej samej grupy z ręki i talii; zwykłe usunięcie karty z
   wachlarza zmienia pozycje pozostałych. Trzeba porównać stan silnika przed i
   po ruchu oraz animacje, by odróżnić poprawny Muster od przypadkowego ruchu.
10. **Wynik 16 przy sile 8 — błąd wyświetlania potwierdzony i naprawiony.**
    Silnik prawidłowo liczył premie, ale znaczek siły na karcie planszy zawsze
    pokazywał siłę bazową. Przy Rogu suma rzędu wynosiła więc 16, a karta nadal
    pokazywała 8. Karty na planszy i w locie wyświetlają teraz aktualną siłę;
    ręka i podgląd nadal pokazują bazową wartość. Regresja sprawdza jednostkę
    o sile 8 pod Rogiem (16 na karcie i w wyniku rzędu). Weryfikacja na Miyoo
    wymaga jeszcze uruchomienia nowej paczki.
11. **Animacja Muster ma oddzielny filtr od silnika.** Należy go ujednolicić z
    regułą źródłową, aby nie pokazywał fałszywego przywołania bohatera.
12. **Spójność cyfr po zmianie siły.** Renderer wspólny musi zachować krój,
    skalę i centrowanie również po premii Rogu, Więzi lub osłabieniu; należy
    obejrzeć wartości jedno-, dwu- i trzycyfrowe w docelowym buildzie.
13. **Czyszczenie planszy może zostawić kartę z poprzedniej rundy.** Test pełnego
    meczu odtworzył kolizję: usuwanie Kambi wywołuje Avengera, który wstawia
    Hemdalla do aktualnie czyszczonego rzędu. Wstawienie przesuwa indeksy
    pętli, przez co Olaf pozostał na planszy, mimo że jego identyfikator został
    równocześnie dodany do kolejki odrodzenia Skellige z cmentarza. Czyszczenie
    rundy ma najpierw odłączyć stare karty od rzędów, a potem rozpatrzyć ich
    usunięcia i efekty Avengera; regresja sprawdza brak starego Olafa na planszy
    i zgodność identyfikatorów kolejki z cmentarzem.
14. **Skrót OnionOS był pakowany pod błędnym rozszerzeniem.** Paczka tworzyła
    `GwentClassic.notfound`, choć OnionOS uruchamia port przez
    `GwentClassic.port`. To mogło sprawić, że aktualizacja nie pojawiała się
    jako poprawny port w menu; nazwa źródła i walidacja archiwum zostały
    poprawione.
15. **Karty w rzędach były dopisywane zamiast układane jak w źródle.** Oryginalny
    `Row.addCard()` wstawia kartę przez `CardContainer.addCardSorted()`:
    najpierw typ karty, potem siła bazowa, a następnie nazwa. Port dopisywał
    większość zagrań na końcu wektora. Jedna karta wywołująca Muster lub kilka
    równoczesnych lotów mogły więc wizualnie przestawiać układ. Dodano wspólne
    źródłowe wstawianie dla zwykłych zagrań, Muster, Szpiega, Medyka, Manekina,
    Avengera i odrodzeń Skellige; regresja sprawdza posortowane trzy karty
    Muster z ręki i talii.
16. **Dźwięk stałych kart z rzędów był wybierany z pustego celu.** Interfejs
    przekazuje brak celu rzędu dla kart, które nie pozwalają go wybierać, więc
    Ranged i Siege dostawały dźwięk zwarcia. Wspólny wybór audio i blokady
    animacji odczytuje teraz wydrukowany rząd karty; Agile nadal używa rzędu
    wybranego przez gracza. Test SDL sprawdza dźwięk `common_ranged`.
17. **Czwarta różna karta pogody była ukrywana.** Pętla zatrzymywała rysowanie
    przed slotem x=134, chociaż oryginał może mieć jednocześnie Frost, Fog,
    Rain i Skellige Storm. Usunięto limit; obliczanie slotu lotu i rysowanie
    używają teraz tej samej funkcji, a test sprawdza pozycję czwartej karty.
18. **Karty specjalne trafiały do cmentarza za wcześnie.** Źródło trzyma Róg
    Dowódcy i Mardroeme w slocie rzędu do końca rundy; port wcześniej trzymał
    wyłącznie flagę efektu. Crach mógł odzyskać aktywną kartę, a Mardroeme
    znikało z animacji. Dodano osobne karty slotów, zapis v5 z odczytem starszych
    zapisów, lot i rysowanie do pola specjalnego oraz przeniesienie karty do
    cmentarza przy końcu rundy. Testy sprawdzają tożsamość, Crach i cleanup.
19. **Losowe wskrzeszanie nie może udawać wyboru Medyka — naprawione w kodzie.**
    Przy Emhyrze Najeźdźcy źródło samo losuje jednostkę z cmentarza; port
    wcześniej wyświetlał picker i czekał na wybór gracza, choć silnik go
    ignorował. Teraz podgląd powtarza następne losowanie bez zmiany RNG.
    Zwykła jednostka wraca automatycznie, a dla wylosowanej karty Agile gracz
    wybiera już tylko rząd. Test wejścia obejmuje oba przypadki. Nadal trzeba
    potwierdzić zachowanie na Miyoo.
20. **Zatwierdzenie celu Medyka omijało blokadę animacji — naprawione w kodzie.**
    Zwykłe zagranie i ruch AI blokowały wejście do zakończenia sekwencji lotu,
    ale ścieżka zatwierdzenia celu Medyka tego nie robiła. Dodano tę samą
    blokadę po zagrywce i regresję wejścia SDL; fizyczny test pozostaje otwarty.
21. **Kopie tych samych neutralnych kart miały kolidujące identyfikatory między
    taliami — naprawione w kodzie.** Identyfikator generowany z katalogu
    wskazywał definicję/kopię karty, więc ta sama karta obecna w obu taliach
    miała ten sam ID. Interfejs dopasowuje loty i efekty kart po ID; kolizja
    mogła przypisać ruch do kopii drugiego gracza. Przy inicjalizacji meczu
    każda karta dostaje teraz ID z prefiksem właściciela i indeksem instancji.
    Test używa tej samej neutralnej karty po obu stronach i wymaga różnych ID.
    Symulacje pełnych meczów dla 100 seedów kontrolują unikalność ID i obecność
    każdej oryginalnej karty w dokładnie jednej strefie po każdej akcji oraz po
    końcu meczu. Stare zapisy z aktywnym meczem wymagają osobnego sprawdzenia
    migracji.

## Kolejność naprawy i kryteria odbioru

### 1. Reguły kart i rozbieżności talii

- Utrwalić poprawkę Muster i regresję Geralt/Ciri/Roach.
- Sprawdzić wszystkie talie premade pod kątem kart spoza frakcji; karty
  neutralne są dozwolone.
- Dopisać test liczenia wyniku: pojedyncza jednostka 8, jednostka z Rogiem,
  dwie karty Więzi oraz wartości każdej karty i rzędu muszą zgadzać się z
  obliczeniem silnika.
- Podgląd w edytorze ma być nazwany jako losowy przykład albo odpowiadać
  faktycznie wylosowanej talii; nie może deklarować frakcji, która potem nie
  wystąpi.

**Odbiór:** regresje silnika przechodzą; każde wyciągnięcie dodatkowej karty
ma w źródle konkretny efekt (Muster, Szpieg, Medyk, przywódca itp.) i jest
widoczne w stanie gry oraz animacji.

### 2. Sterowanie i edytor talii

- Zdefiniować widoczne i jednakowe na ekranie przypisania Miyoo: kierunki
  poruszają zaznaczeniem, A dodaje/zatwierdza, B cofa/anuluje, Y zmienia
  frakcję, L/R lub przejście krawędzią zmienia panel, X otwiera podgląd;
  przytrzymanie X w meczu otwiera potwierdzenie spasowania.
- W kolekcji i talii wdrożyć ruch siatką: lewo/prawo zmienia kartę w wierszu,
  góra/dół zmienia wiersz, a przejście przez krawędź prowadzi do sąsiedniego
  panelu z zachowaniem możliwie tego samego wiersza.
- Tryb lidera ma pokazywać aktywny nagłówek i instrukcje. Strzałki zmieniają
  lidera, A zatwierdza, B wraca bez utraty bieżącej talii.
- B w meczu ma anulować bieżący wybór; nie może od razu zapisywać i porzucać
  partii. Zagrać kartę może wyłącznie jawny przycisk zatwierdzenia.
- Dodać diagnostykę zapisu rzeczywistych zdarzeń SDL z fizycznych przycisków,
  aby wyjaśnić rozbieżność między B zgłaszanym przez użytkownika a kodem SDL.

**Odbiór:** testy wykonują pełne sekwencje przez `handleKey`/zdarzenia SDL,
nie tylko sprawdzają funkcję mapowania. Na Miyoo przechodzi karta po siatce,
przełączenie panelu, dodanie/usunięcie kopii, zmiana lidera, anulowanie oraz
pasowanie.

### 3. Przebieg meczu, rzędy i podpowiedzi

- Pokazywać zaznaczony rząd tylko wtedy, gdy wybrana karta faktycznie pozwala
  go wybrać: Agile, Róg, Mardroeme albo pogoda bez określonego typu.
- Dla jednostek nieagile używać rzędu przypisanego karcie; dla Agile ograniczyć
  wybór do zwarcia/dystansu. Wybór celu Medyka i Manekina pozostaje osobnym
  etapem.
- Stale pokazywać podstawowe sterowanie, stan aktywnego gracza, pasowanie,
  siłę karty/każdego rzędu i wskazanie efektów, które zagrały dodatkowe karty.
- Z logu ruchu odróżnić pojedynczą animację przesunięcia wachlarza od
  niezamierzonego zagrania kilku kart; stan ręki po każdym ruchu ma zgadzać się
  z animacją.

**Odbiór:** jedna akcja gracza zużywa dokładnie jedną turę; każda dodatkowa
karta ma zarejestrowany efekt źródłowy; B nigdy nie wykonuje zagrania; wynik
rzędu jest sumą widocznych sił po efektach.

### 4. Pogoda, animacje i pozorne zawieszenie

- Dla Trzaskającego Mrozu sprawdzić w jednym scenariuszu: zagranie karty,
  aktywną flagę pogody na obu planszach, spadek siły jednostek zwarcia,
  niezmienione inne rzędy, widoczną nakładkę i prawidłowe czyszczenie pogody.
- Zmierzyć najdłuższą blokadę sterowania po pogodzie, Muster, Więzi, Medyku,
  Szpiegu i Scorch. Blokada musi mieć widoczny postęp/komunikat i skończyć się
  po końcu ostatniej animacji; żaden oczekujący dźwięk nie może trzymać gry
  bez końca.
- Sprawdzić klatki i przepływ ręka → plansza, stos pogody, cmentarz i talia;
  każda karta ma tylko jedną aktywną animację w danym momencie.
- Zmierzyć na urządzeniu SFX i muzykę walki/menu; brak dźwięku nie może
  blokować tury.
- Dla karty pogodowej pominąć dodatkowe oczekiwanie przeznaczone na dźwięk
  układania jednostki w rzędzie; sterowanie ma wrócić wraz z końcem jej lotu.

**Odbiór:** testy stanu plus porównanie przechwyconych klatek; Miyoo reaguje
po zakończeniu zaplanowanej animacji, a pogoda ma widoczny i zgodny ze źródłem
wpływ.

### 5. Czytelność siły i pełny audyt urządzenia

- Zastąpić rozdzielone rysowanie cyfr jedną funkcją z jednolitym krojem,
  grubością obrysu, centrowaniem i automatyczną szerokością dla 1–3 cyfr.
- Sprawdzić rozmiary 0–99, w tym bohaterów, jednostki, sumy rzędów i sumy
  całkowite przy 640×480; nie nachodzić na ikonę siły ani obramowanie.
- Uruchomić grę od zimnego startu, rozegrać pełną rundę i mecz, zapisać/
  wczytać, sprawdzić wszystkie typy celów, frakcje, przywódców, animacje,
  efekty audio, menu i brak zwiech.
- Po odbiorze desktop/CI zbudować binarium ARM, porównać sumy kontrolne,
  skopiować na kartę SD i zebrać log oraz zrzut z Miyoo. Do czasu ponownego
  zamontowania karty nie oznaczać wdrożenia ani testu fizycznego jako
  zakończonego.

**Odbiór:** brak nakładania lub zmiany kroju/liczby pikseli cyfr między
rodzajami kart; pełny test na urządzeniu opisany wersją binarium i wynikiem.

## Status teraz

- [x] Potwierdzono źródłową regułę Geralt → Roach, nie Ciri.
- [x] Potwierdzono neutralną frakcję Tajemniczego Elfa i losowy wybór talii
  przeciwnika w źródle.
- [x] Potwierdzono błąd nawigacji siatką oraz brak wygodnego zatwierdzenia
  lidera w edytorze.
- [x] Potwierdzono niespójne ścieżki rysowania cyfr siły.
- [x] Ujednolicono skalę cyfr siły; bohaterowie i jednostki korzystają ze
  wspólnego bitmapowego renderera z centrowaniem według szerokości liczby.
- [x] Odtworzono i naprawiono błąd czyszczenia rundy po efekcie Avengera,
  który zostawiał kartę na planszy oraz rozjeżdżał kolejkę odrodzenia Skellige.
- [x] Naprawiono zatwierdzanie lidera: Miyoo A (`SDLK_x`) kończy wybór, a
  strzałki służą wyłącznie do przechodzenia między liderami.
- [x] Rzędy walki wstawiają karty według kolejności `Card.compare` ze źródła;
  test sprawdza porządek po efekcie Muster.
- [x] Róg Dowódcy i Mardroeme zostają w specjalnym polu do końca rundy; nowy
  format zapisu przechowuje te karty, a regresja potwierdza, że Crach nie może
  pobrać aktywnego Rogu.
- [x] Dźwięk stałych kart z rzędów wybiera się z ich faktycznego rzędu; czwarta
  karta pogody nie jest już ukrywana przez limit rysowania.
- [x] Medyk przy Emhyrze Najeźdźcy nie pokazuje fałszywego wyboru karty: zwykła
  karta wraca z losowania automatycznie, a wylosowana Agile pokazuje wyłącznie
  wybór rzędu. Test SDL pokrywa oba przebiegi; test urządzenia pozostaje otwarty.
- [x] Pakiet ARM z tą zmianą zainstalowano na wykrytej karcie SD; 603 pliki
  gry mają identyczne sumy SHA-256 jak zweryfikowany artefakt. Poprzedni folder
  gry i launcher zachowano jako kopie `.previous-9e74bb8`. Test na samym Miyoo
  pozostaje do wykonania.
- [x] Aktualizacja z `0828412` z poprawioną kolejnością rzędów, dźwiękiem
  stałych rzędów, czwartym slotem pogody i kartami w polach specjalnych została
  zainstalowana. Wszystkie 603 pliki na SD zgadzają się SHA-256 z paczką;
  poprzednią instalację zachowano jako `.previous-476af90`. Odczytowy `chkdsk`
  nie wykrył błędów FAT32, ale Windows nadal zgłasza `Full Repair Needed`.
  Uruchomienie i rozgrywka na fizycznym Miyoo nie są jeszcze potwierdzone.
- [x] Poprawiono nazwę skrótu w paczce OnionOS z `.notfound` na `.port` i
  dodano kontrolę tej ścieżki do walidacji archiwum.
- [x] Desktop CI dla silnika i frontendu przeszło: testy reguł, testy wejścia,
  smoke testy ekranów, audio i kompozycji programowej.
- [x] Build ARM hard-float i walidacja archiwum OnionOS przeszły. Pakiet jest
  wgrany na kartę SD; wszystkie 603 pliki katalogu gry mają zgodne rozmiary i
  sumy SHA-256 z artefaktem.
- [x] Build `d035158` z poprawionym przepływem losowego Medyka przeszedł CI
  desktopowe oraz build ARM. Paczka `GwentClassic-OnionOS.zip`
  (`44cf693e6c38e06d021e6805cb096b383f2a557e9069c0983f34d5471f6f7bb7`)
  została zainstalowana na SD; wszystkie 603 pliki i launcher `.port` zgodne
  są z artefaktem SHA-256. Poprzedni build zachowano jako
  `GwentClassic.previous-d035158`. Fizyczny test na Miyoo pozostaje otwarty.
- [x] Build `13075d6` dodający blokadę akcji po wyborze celu przeszedł CI
  silnika i frontendu oraz build ARM. Wszystkie 603 pliki i launcher na karcie
  SD mają zgodne SHA-256 z paczką (`ff347c261ff6c5f55cdaa1cbd298163bc198f3e3c0b61c2af1db37de3f84d37a`);
  poprzedni katalog zachowano jako `GwentClassic.previous-13075d6`.
  Rzeczywisty test na Miyoo pozostaje otwarty.
- [x] Build `1b3a74a` naprawiający wartość siły na kartach pod wpływem premii
  przeszedł testy silnika, frontendu i mapowania wejścia oraz build ARM.
  Regresja sprawdza, że jednostka o bazowej sile 8 pod Rogiem pokazuje 16,
  zgodnie z wynikiem rzędu. Pakiet `GwentClassic-OnionOS.zip` ma SHA-256
  `fd1f161346624c3c35a5441b5a2ad50b6faf7601eaab37c32e20fe9cccfc7b1e`.
  Zainstalowano go na SD; 603 pliki gry, bez braków i nadmiarów, oraz launcher
  `.port` mają zgodne SHA-256. Poprzednią instalację i launcher zachowano jako
  kopie `GwentClassic.previous-1b3a74a` i `GwentClassic.port.previous-1b3a74a`.
  Fizyczny test gry nadal wymaga wykonania przez użytkownika.
- [x] Build `1de0f0b` rozdzielający tożsamości kopii kart z obu talii przeszedł
  testy silnika, frontendu, wejścia i build ARM. Regresja wkłada tę samą neutralną
  kartę do obu talii i sprawdza unikalne ID po rozpoczęciu partii. Pakiet
  `GwentClassic-OnionOS.zip` ma SHA-256
  `84dae4072f5367a5a3a8cef955fd57e557b91ab5f35bb4697bbce48b01763b8a`.
  Jest wgrany na SD; 603 pliki gry i launcher `.port` mają zgodne SHA-256,
  bez braków i nadmiarów. Poprzednia instalacja i launcher są zachowane jako
  `GwentClassic.previous-1de0f0b` oraz `GwentClassic.port.previous-1de0f0b`.
  Potwierdzenie na urządzeniu pozostaje otwarte.
- [ ] Dokończyć poprawki i testy opisane w etapach 1–5.
- [ ] Potwierdzić uruchomienie, sterowanie i pełny mecz na fizycznym Miyoo.

## Dodatkowa poprawka mechaniki

- [x] Francesca Hope wstawia przeniesione jednostki Agile zgodnie z porządkiem
  rzędów źródłowego `Row.addCard()`. Wcześniej symulacja optymalnego układu
  używała `push_back`, przez co karty po aktywacji mogły pozostać poza
  kolejnością. Regresja sprawdza posortowanie obu rzędów po zdolności.
- [x] Decyzje AI o spasowaniu porównano ze źródłowym `weightPass()` i
  `ControllerAI.startTurn()`. Testy blokują szybkie spasowanie po przewadze,
  remis Nilfgaardu, remis innej frakcji oraz zakaz wydawania ostatniego życia
  na pas; wymuszony pas nie zużywa RNG.
- [x] Waga AI dla Bohatera z Morale/Więzi zachowuje wynik źródłowego
  `weightRowChangeTrue()`: helper pomija Bohaterów w `calcRowPower()`, ale
  nadal odejmuje ich siłę z `row.calcCardScore()`. Port zaniżał przez to wagę
  Isengrimowi i Kayranowi; regresja z Isengrimem sprawdza przedział losowania,
  w którym zgodny helper wybiera właśnie tę kartę.
- [x] Manekin przyjmuje wyłącznie jawnie wskazaną własną jednostkę bojową
  niebędącą Bohaterem. Test sprawdza zwrot tej samej karty, miejsce Manekina,
  Avengera Krowy oraz odrzucenie celu z planszy przeciwnika.
- [x] Generator kart wyznacza domyślną grupę Muster przez podział przy
  pierwszym myślniku, jak źródłowe `abilities.js`. Poprzedni separator `" - "`
  pomijał „Arachas- Behemoth”; regresja potwierdza przywołanie Arachas z ręki
  i talii.
- [x] Commit `450237d` sortuje rząd po przemianie Berserkera przez Mardroeme,
  zgodnie ze źródłowym `removeCard()` + `addCard()`. Nowy przypadek regresyjny
  przechodzi w CTest; CI desktopowe i build ARMHF również przeszły. Pakiet
  `GwentClassic-OnionOS.zip` (`84f4b9eddfe31abee808df93401b7386fd0cbe53e40d8167df3ce149f6525137`)
  wgrano na SD i zweryfikowano SHA-256 wszystkich 603 plików. Zachowano kopię
  `GwentClassic.previous-450237d`. Fizyczny test partii z przemianą pozostaje
  otwarty.
- [x] Commit `c472e24` usuwa tymczasowy znacznik `vildkarrl` po przemianie,
  zgodnie z callbackiem źródłowym; regresja sprawdza obie odmiany Vildkaarla.
  CTest (2/2), desktop CI i ARMHF build przeszły. Paczka ma SHA-256
  `068432cd0187cadad9ca0c1bf24ca4a0f1123a3ca4c3f2c7549b65a78c2ad94e` i jest
  wgrana na SD; wszystkie 603 pliki są zgodne. Binarka `gwent` ma hash
  `1e09003cbaddb861992298b8efc4ff1afc096e02cf2a339f0a7aa89ce57eb4d4`.
  Poprzedni build zachowano jako `GwentClassic.previous-c472e24`. Fizyczne
  sprawdzenie audio i rozgrywki pozostaje otwarte.

- [x] Commit `21b05e5` corrects two AI tie-order differences against the pinned
  source: Horn retains the first equal-gain row in melee/ranged/siege order,
  and Decoy's fallback target order is siege/ranged/melee. Fixed-seed engine
  regressions cover both choices. Engine CTest passed 2/2, desktop CI passed,
  and the ARMHF workflow completed successfully. Package SHA-256:
  `cfe59836495e131c8b9a14a13f30898c6c433fa03fcf9f9bc04f25689daebf01`;
  binary SHA-256: `80209b060cdfc30fce6990e4168da8527de990ae85574c59be550f5d10c0fe1f`.
  Installed on SD and verified all 603 files against the package. Backup:
  `GwentClassic.previous-21b05e5`. Physical match testing remains open.
- [x] Weather parity is now regression-checked against `Weather.types`,
  `Weather.addCard()`, `changeWeather()` and `clearWeather()` in the pinned
  source: Frost/Fog/Rain affect the expected rows, Skellige Storm applies Fog
  and Rain together, duplicate Frost goes directly to its owner's graveyard,
  and Clear Weather clears all active effects while returning each active card
  to its original owner's graveyard. Ubuntu CTest passed 2/2; these checks found
  no engine discrepancy. Physical visual/audio timing remains open.
- [x] Result-screen audit found that `RoundResult` always displayed the loss
  stamp because the frontend read `matchWinner` before a match was over; match
  draws were also rendered as defeats. The renderer now derives round outcome
  from score and Nilfgaard's tie-break, and renders `end_draw` plus a localized
  draw title for drawn matches. Engine tests cover last-gem loss and drawn match;
  frontend tests render five outcome frames and input tests cover win/loss/draw
  mapping. Local SDL input/result smoke checks and five-frame visual review
  passed; desktop CI passed for commit `550435f`, including Release CTest and
  screen smoke test. ARMHF package SHA-256:
  `ac2af1a15df9ba36864bc7cf9c8dcd7d3cf65dbbcb2db69e8971edb3fcc734ea`;
  binary SHA-256:
  `a08da61a4cc517a590eb356b6406b14607a207d7c2ef49b33051ef907fac38f9`.
  Installed on SD; all 603 files were verified. Backup:
  `GwentClassic.previous-550435f`. Visual captures are under
  `artifacts/qa/result-screens/`. Physical-device result confirmation remains
  open.
