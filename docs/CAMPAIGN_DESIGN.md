# Kampania — projekt talii i progresji

Status: projekt do realizacji po domknięciu mechaniki i nowym menu.

## Ustalenia użytkownika

Cztery drabinki: Królestwa Północy, Nilfgaard, Potwory, Skellige. Układ
wieży jak w Mortal Kombat, portret aktualnego przeciwnika, krótka wypowiedź
przed partią i osiągnięcie za każdego pokonanego przeciwnika. Portrety będą
generowane narzędziem graficznym; wypowiedzi będą autorskie, w charakterze postaci.
Proponowana liczba: pięć przeciwników na frakcję; nazwiska i grafiki wymagają
osobnego projektu. Poziom oznacza siłę talii, nie siłę postaci w fabule.

## Obowiązkowa legalność

Źródło: upstream/asundr-gwent-classic/gwent.js, DeckMaker.startNewGame:
co najmniej 22 jednostki i najwyżej 10 kart specjalnych. Lider jest osobny.
Jednostki bohaterskie wliczają się do jednostek. Nie ma założenia, że 22 oznacza
łączną liczbę wszystkich kart albo maksymalny rozmiar talii.

Każdy przepis kampanii musi przejść validateUserDeck/materializeUserDeck:
frakcja jednostek zgodna z talią albo neutralna, lider zgodny z frakcją,
liczba kopii zgodna z katalogiem, bez kart niedostępnych w puli. Brak sztucznie
podniesionej siły, dodatkowych kart na początku ani ukrytej wiedzy AI.

## Progresja talii — propozycja

| Etap | Jednostki | Specjalne | Charakter talii |
| --- | ---: | ---: | --- |
| 1 — Nowicjusz | 24 | 2 | Słabsze legalne jednostki, niewiele synergii |
| 2 — Gracz | 23 | 3 | Pierwszy spójny zestaw synergii |
| 3 — Weteran | 22 | 4 | Lepsze jednostki, odpowiedzi na pogodę i mocne rzędy |
| 4 — Mistrz | 22 | 4 | Spójne kombinacje, właściwy lider, kontrolowana liczba specjalnych |
| 5 — Boss | 22 | 3 | Najmocniejsza przetestowana kompozycja i wysoka powtarzalność dobrań |

To docelowe budżety projektowe, nie gotowe przepisy. Więcej specjalnych nie
jest automatycznie mocniejsze. Ostateczny skład i lider zależą od frakcji.

- Północ: stopniowo pełniejsze Więzi, medycy, dobór kart i wsparcie rzędów.
- Nilfgaard: rosnąca jakość szpiegów, medyków i manekinów; mocne jednostki
  oraz wykorzystanie przewagi w remisach.
- Potwory: coraz spójniejsze grupy Zbratania, kontrola rozwodnienia ręki,
  wsparcie rogu i lider pasujący do kompozycji.
- Skellige: stopniowo pełniejsze pakiety Berserker/Mardroeme i Więzi,
  odpowiedni lider oraz kompozycja korzystająca z trzeciej rundy.

## Dowód rosnącej trudności

Najpierw tworzymy konkretne listy kart z indeksami katalogu. Walidator odrzuca
nielegalne przepisy. Potem każdy poziom gra przeciw temu samemu zestawowi talii
kontrolnych na wspólnych ziarnach, po obu stronach i przy obu pierwszych graczach.
Raport zapisuje liczbę partii, wygrane/remisy/przegrane, błędy akcji i wersję AI.
Nie zatwierdzamy kolejności na podstawie samej sumy siły kart. Kolejny etap ma
wykazać wzrost skuteczności na zestawie kontrolnym; wyjątki wynikające z kontr
frakcyjnych opisujemy i korygujemy skład przed wydaniem.

## Przebieg i zapis

Wybór frakcji → wieża → portret i krótki dialog → podgląd własnej talii → partia
na zwykłych zasadach → wynik → osiągnięcie i odblokowanie kolejnego szczebla.
Porażka pozwala powtórzyć przeciwnika. Osiągnięcie jest nadawane raz, wyłącznie
po wygranej; powtórka nie dubluje nagrody. Zapis przechowuje stabilne ID ścieżki,
przeciwnika, wersji przepisu i osiągnięcia. Kampania działa offline.

## Kolejność realizacji

1. Domknąć medyków i regresje źródłowej mechaniki.
2. Nowe menu według przesłanej inspiracji, czytelne w 640×480.
3. Konkretne legalne listy kart dla wszystkich etapów i raport balansu.
4. Wieże, portrety, dialogi, osiągnięcia i trwały zapis progresu.
5. Testy pełnych ścieżek oraz kontrola wizualna na urządzeniu.

## Pierwszy pomiar — CI 43908517238, silnik 7f3b659

Przeprowadzono 4800 partii: 20 talii po 240 spotkań z 10 źródłowymi
przepisami kontrolnymi, 12 wspólnych ziaren i obie strony stołu. Wynik
oznacza (wygrane + 0,5 × remisy) / liczba partii. To zbiór treningowy,
a nie dowód uniwersalnej trudności ani niezależna walidacja.

- Północ: 0,037 / 0,052 / 0,156 / 0,175 / 0,271.
- Nilfgaard: 0,067 / 0,104 / 0,163 / 0,365 / 0,792.
- Potwory: 0,237 / 0,250 / 0,562 / 0,681 / 0,817.
- Skellige: 0,258 / 0,221 / 0,342 / 0,179 / 0,410 — dwie inwersje.

Skellige zostało przełożone według zmierzonego wyniku: stare składy
4, 2, 1, 3, 5. Pole composition_origin_stage zachowuje pochodzenie.
Wcześniejsze budżety tabeli były propozycją, nie regułą wymagającą sztucznego
zmieniania legalnej talii. Nowa kolejność musi ponownie przejść pomiar.

Boss Północy uzyskał zbyt niski wynik względem pozostałych frakcji. Heurystyka
preferowała wysoką drukowaną siłę kosztem słabszych szpiegów i pełnych grup
Więzi. Reguła jawnie zachowuje szpiegów, medyków i komplety Więzi o sile co najmniej
4, a pozostałe miejsca uzupełnia najlepiej ocenionymi jednostkami. Porównanie
list kart wykazało jednak, że ta reguła daje identyczny skład jak wcześniejszy
boss: nie stanowi faktycznego wzmocnienia i nie zmieniła wyniku 0,271.
Nie należy opisywać samego dodania reguły jako poprawy skuteczności.

Benchmark przyjmuje argumenty seedStart i seedCount; po dostrojeniu należy
użyć niezależnego zakresu, np. 101 48. Zmiany silnika medyków/Skellige wymagają
powtórzenia obu zestawów i zapisania rewizji, zamiast porównywania wyników z
różnych wersji mechaniki jak jednego eksperymentu.


## Powtórny trening — engine job w CI 43909090788

Po przestawieniu Skellige wszystkie cztery drabinki mają rosnące oszacowania
punktowe na tym samym zbiorze treningowym. Nowe wyniki Skellige:
0,179 / 0,221 / 0,258 / 0,342 / 0,410. Pozostałe wyniki są identyczne z
pierwszym raportem; skład bossa Północy pozostał niezmieniony. Sam frontend
tego uruchomienia zgłosił oddzielny błąd input-test, dlatego nie jest to dowód
pełnego przejścia CI portu. Raport kampanii powstał w zielonym jobie engine.

Wyniki są historyczne: późniejsza naprawa replay wskrzeszeń Skellige i medyków
zmienia rozgrywkę. Kolejność wymaga ponownej oceny na tej rewizji oraz na
niezależnym zakresie ziaren. Małe odstępy (np. Potwory 1→2) nie dają podstawy
do twierdzenia, że każdy kolejny przeciwnik będzie zawsze zauważalnie trudniejszy.

## Obsada turnieju — dane wdrożeniowe

`data/campaign-opponents.json` zawiera 20 przeciwników, po pięciu na ścieżkę.
Każde ID jest związane z dokładnie jednym istniejącym przepisem z
`campaign-decks-draft.json`. Etap oznacza projektowaną trudność talii, nie
kanoniczną siłę bohatera ani kolejność wydarzeń fabularnych.

Kampania jest umownym turniejem postaci i stworzeń reprezentowanych przez karty
źródłowego Gwent Classic. Nie twierdzi, że wszystkie te pojedynki wydarzyły się
w fabule Wiedźmina 3 ani że gracz osobiście spotyka w niej każdą z tych postaci.
W szczególności udział Vattiera opiera się na obecności jego karty w katalogu,
a nie na założeniu osobistego spotkania w grze. Frakcja ścieżki określa legalną
talię przeciwnika. Możliwość wybrania postaci jako twarzy etapu nie daje jej
kartom wyjątku od limitów ani przynależności frakcyjnej.

| Ścieżka | 1 | 2 | 3 | 4 | 5 |
| --- | --- | --- | --- | --- | --- |
| Królestwa Północy | Talar | Ves | Vernon Roche | Sigismund Dijkstra | Filippa Eilhart |
| Nilfgaard | Morvran Voorhis | Letho z Gulety | Fringilla Vigo | Vattier de Rideaux | Emhyr var Emreis |
| Potwory | Nekker | Gryf | Leszy | Imlerith | Eredin |
| Skellige | Błękitny Lugos | Myszowór | Hjalmar | Crach an Craite | Cerys |

Uzasadnienie dramaturgiczne: Północ przechodzi od rozmówcy i żołnierzy do
intrygantów; Nilfgaard od dworu i wykonawców do wywiadu i cesarza; Potwory od
stworzeń do dowódców Dzikiego Gonu; Skellige od wojownika przez druida do
rodziny an Craite. Ostateczna trudność jest przypisana przepisom, a nie tej
hierarchii fabularnej. Poszczególne nazwy nie mogą być podstawą twierdzenia,
że etap jest mocniejszy bez wyników rozgrywek kontrolnych.

Każdy przeciwnik ma autorski, krótki tekst polski i angielski oraz tytuł
osiągnięcia. Teksty nie są cytatami z gry. Nekker, Gryf i Leszy mają
`dialogue_mode: narration`: panel opisuje zachowanie stworzenia i otoczenie,
nie przypisuje mu ludzkiej wypowiedzi. Pozostali mają `speech`.

Pole `portrait` wskazuje docelową, generowaną ilustrację
`img/campaign/<id>.png`. `portrait_reference_name` jest sprawdzoną dokładną
nazwą karty katalogowej i służy wyłącznie do technicznego podglądu podczas
wdrażania. Obecność tego pola nie oznacza, że wygenerowany portret jest gotowy.
Przed wydaniem każdy plik docelowy musi faktycznie istnieć, zostać obejrzany i
sprawdzony w rozdzielczości urządzenia. Tło i styl portretów powinny współgrać
ze złotem, drewnem i bursztynową poświatą menu.

Walidacja treści wykonana przy przygotowaniu danych: 20 unikalnych ID,
zgodność zbioru ID i par frakcja/etap z przepisami, obecność wszystkich nazw
referencyjnych w źródłowym katalogu oraz niepuste nazwy, wstępy i osiągnięcia
w obu językach. To walidacja zawartości danych, nie dowód gotowości kampanii,
portretów ani jej pełnego balansu.
