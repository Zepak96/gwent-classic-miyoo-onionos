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
