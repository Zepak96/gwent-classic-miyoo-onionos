# Campaign artwork — work in progress

Assets are generated with the built-in `image_gen.imagegen` tool. Original
outputs remain in the Codex generated-images directory; selected project assets
are copied into `assets/gwent-classic/img/campaign/`.

All twenty clean opponent portraits are now present. Talar (`realms-1.png`)
is the canonical visual reference for the series. The twelve additions and
corrections are recorded with exact prompts and source output basenames in
`campaign-art-style-lock.json`. Dijkstra, Morvran and Griffin replace earlier
versions; original generated outputs are preserved outside the asset directory.
The earlier Dijkstra and Morvran sections below describe historical versions,
not the selected revised portraits.

The UI uses a 3:4 portrait region to retain heads and antlers without stretching
or the previous square crop. All twenty screens are emitted by the campaign
smoke test in normal and software-composition modes for visual review.

## Dijkstra

Reference: `assets/gwent-classic/img/lg/realms_dijkstra.jpg`.
Output: `assets/gwent-classic/img/campaign/realms-4.png`.
Original output basename: `exec-5f2f1a41-e0ec-4a81-8d80-6252fd601fa8.png`.

Prompt:

> Create a clean character portrait based on the man shown in the reference
> card. Preserve his recognizable bald head, stern face, broad build and ornate
> green-and-gold medieval doublet. Show his head and upper torso with generous
> margins. Remove all card-game graphics: number, circular icons, blue ribbon,
> border, name and quote. The output should be ONLY a painted character
> illustration against a quiet dark brown atmospheric background, warm amber
> side lighting, consistent painterly game-art finish, vertical 3:4 composition.
> Fully clothed, calm formal portrait. No text or UI.

Viewed at full size: no card numbers, frame or text remain. In-game verification
of this particular portrait at 640×480 remains outstanding. The latest inspected
campaign smoke frame uses Talar and shows separate ladder, portrait and dialogue
regions without overlap; that does not prove all names and dialogues fit.

## Morvran Voorhis

Reference: `assets/gwent-classic/img/lg/nilfgaard_moorvran.jpg`.
Output: `assets/gwent-classic/img/campaign/nilfgaard-1.png`.
Original output basename: `exec-86eba4b5-15a4-4cfd-9f49-a3bd5bf7629b.png`.

Prompt:

> Create a clean character portrait based on the man shown in this reference
> card. Preserve his face, slicked-back brown hair, formal black and gold
> medieval court clothing and composed expression. Head and upper torso, with
> generous margins. Remove all card-game graphics: number, icons, ribbon,
> border, name and quote. ONLY a painted character illustration against a quiet
> dark brown atmospheric background, warm amber side lighting, consistent
> painterly game-art finish, vertical 3:4 composition. Fully clothed, calm formal
> portrait. No text or UI.

Viewed at full size: clean portrait without card text or symbols. Device-size
verification remains outstanding.
