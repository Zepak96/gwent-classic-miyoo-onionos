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
10. **Wynik 16 przy sile 8 nie jest sam w sobie dowodem błędu.** Róg albo
    Więź mogą go podwoić; należy sprawdzić pełny stan rzędu i osobne wyniki
    kart. Bez zrzutu lub zapisu meczu nie da się przypisać konkretnej przyczyny.
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
- [ ] Dokończyć poprawki i testy opisane w etapach 1–5.
- [ ] Ponowić test fizyczny na Miyoo po udostępnieniu karty SD/urządzenia.
