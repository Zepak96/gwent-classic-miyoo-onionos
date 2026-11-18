# Talar / Ves early-pass review — 2026-11-18

Reference: pinned `upstream/asundr-gwent-classic/gwent.js`, source revision recorded in `data/cards-manifest.json` as `08592d847f2573440a34deb9f32bc855b2544cdd`. No main/menu/settings changed. A separately authorized minimal realms-1 recipe correction is documented below.

## Proven native AI defect and correction

`ControllerAI.weightRowChangeTrue` (source lines 313–321) computes existing-row power before/after `row.updateState(card,true)`. `Row.updateState` (1085–1098) changes effect counters only: it does **not** insert the proposed card into `row.cards`. The AI then explicitly includes the proposed card's resulting power using `dif[0] -= row.calcCardScore(card)`. Its weight is therefore existing-card gain **plus candidate power** (with the source's hero filtering).

The native helper `src/ai.cpp:113` instead simulates a row already containing the candidate. `after-before` already includes that candidate power, but the previous implementation subtracted it again. This incorrectly made standalone Bond/Morale/Horn units weight zero, and undervalued later copies too. The hero-specific extra addition patched only hero cards, hiding the general error.

Correction: return `max(0, after-before)` for this simulation. This preserves source weights and source pass policy; it does not introduce a difficulty boost or minimum card-play rule. Existing heroes ignore row effects, so their inclusion in native before/after totals cancels. Commander Horn special cards retain their separate helper.

Executed actual pinned JS methods, not a handwritten interpretation, through Node (`node tests/source_ai_row_weights.cjs`):

| Candidate | Existing ally | Source weight | Previous native weight |
| --- | --- | ---: | ---: |
| Blue Stripes Commando | empty | 4 | 0 |
| Blue Stripes Commando | another identical copy | 12 | 4 |
| Kaedweni Siege Expert | empty | 1 | 0 |
| Kaedweni Siege Expert | ordinary strength 5 | 2 | 1 |
| Dandelion | empty | 2 | 0 |
| Dandelion | ordinary strength 5 | 7 | 5 |
| Isengrim | ordinary strength 5 | 11 | 11 (existing special patch) |

The JS fixture preserves the real `ControllerAI` and `Row` methods, constructing only DOM-free card/row stubs. All seven assertions passed. Native engine regressions cover 896 weighted tickets across both life counts, exact RNG progression, and a four-card synergy-only last-life hand which previously fell into zero-total pass. Native compilation/execution was not available locally: CI must execute these tests before release.

## What passing with four cards actually means

Source `weightPass` (272–282) and native `chooseAiAction` pass block match:

- With two lives, pass weight is the absolute point difference, becomes 100 when behind by more than 30, or when ahead by more than 30 while the opponent has over two extra cards.
- With one life, normal pass weight is zero.
- If the opponent already passed and AI is winning (or Nilfgaard ties), source immediately passes.
- When all action weights are zero, source fallback may still pass even with cards and one life.

There is no source rule that four remaining cards prohibit passing. For example four ordinary five-point cards against pass weight 100 still give source AI roughly 83% chance to pass. The observed screen alone cannot distinguish source strategy from the discovered defect; the actual turn state/save is needed for an exact replay. The defect strongly increases pass probability with the two campaign recipes' synergy cards. A four-card hand of Blue Stripes, Poor Infantry, Siege Expert and Dandelion on empty rows previously had all zero card weights; source weights total 8. Against a ten-point difference the intended normal pass probability is 10/18, not certainty, and on the last life these cards must produce a play rather than a zero-weight fallback.

## Campaign recipe findings

Both `realms-1` (Talar) and `realms-2` (Ves) pass structural checks against current `data/cards.json`: correct faction/neutral/special availability, legal leader, legal per-definition copy counts, at least 22 units and no more than 10 specials. A read-only Python check produced:

| Recipe | Units | Specials | Printed strength sum | Bond copies | Other relevant abilities |
| --- | ---: | ---: | ---: | ---: | --- |
| realms-1 | 24 | 2 | 51 | 7 | 3 Morale, Dandelion, 2 spies, 4 Muster, Cow |
| realms-2 | 23 | 3 | 76 | 7 | 3 spies, 3 Muster, Decoy, Frost, Horn |

Both contain three Blue Stripes and four strength-one Poor Infantry, so this bug affects a substantial fraction of their playable units. Realms-1 additionally contains three Siege Experts and Dandelion. Their effects depend on drawing complementary cards and establishing rows. The correction restores source play likelihood; it does not establish balanced difficulty.

Realms-1 originally used Foltest King of Temeria with **no Impenetrable Fog**, so this leader's source ability could not find its required weather card in the deck. This was a concrete recipe compatibility problem, not an AI pass-rule defect. The authorized minimal correction replaces its one Clear Weather (index 4) with one Impenetrable Fog (index 9); units, total cards, special count and leader remain unchanged. Fog can be drawn into hand, in which case the leader has no remaining target, which is normal source behavior. Source AI still evaluates Fog against both sides, so this is compatibility repair rather than unconditional leader use. Realms-2's clear-weather leader is situational, especially with its own Frost. Realms-1/2 are custom campaign drafts rather than original upstream difficulty levels; their stored `balance_status` remains `unverified-draft`.

The existing historical report `artifacts/qa/ci-44064830723/campaign-balance/campaign-balance.csv` shows realms-1 9 wins/2 draws/229 losses and realms-2 11/1/228 out of 240 each. These numbers predate this correction and are **not current performance measurements**. They document weak past results, not an established difficulty ramp. Rerun paired-seed control/holdout matches after source parity tests pass before changing compositions. Do not sort new difficulties using those old numbers.

## Remaining limits / next steps

1. Run full native engine regression suite, particularly existing Agile, fractional Horn ticket, hero Morale and deferred-target/RNG tests, plus the new weighted-choice fixtures.
2. Run an updated deterministic campaign benchmark and record pass turn, remaining hand, health, scores and available actions. Include source-like and strong human deck controls; AI-vs-AI scores alone do not measure human challenge.
3. Replay a device save from the reported Talar/Ves match if available; no such turn-state evidence was present during this review.
4. Recheck the minimally corrected realms-1 recipe in the new benchmark. `tools/draft_campaign_decks.py` now generates the same Fog replacement; an isolated temporary-directory generator replay confirmed identical realms-1 cards and leader, preventing regeneration from restoring the unusable leader. All twenty generated recipes passed its structural assertions; the current realms-1/2 files were independently checked too. No other recipe was modified.

No rule was added to prevent a source-valid pass with cards in hand. `git diff --check` passed for the native edits. No commit was created by this review.

## Post-correction training benchmark and recipe ordering

The new report `artifacts/qa/ci-5b5b499/14421683832/campaign-balance.md` ran 240 matches per recipe after the source-weight correction, using the existing paired-side/control seeds 1–12 protocol. Reported point estimates were:

- Northern Realms: .054, .062, .150, .540, .767.
- Nilfgaard: .077, .056, .104, .444, .733.
- Monsters: .071, .090, .285, .490, .625.
- Skellige: .425, .098, .140, .212, .477.

At the parent's explicit instruction, only complete existing recipes were reassigned to stages: Nilfgaard stages now use previous stage compositions **2,1,3,4,5**; Skellige stages now use previous stage compositions **2,3,4,1,5**. Each complete recipe retains its own leader, exact card counts, unit/special counts and balance metadata. Stable stage IDs, factions and stage numbers are retained; characters, portraits, dialogues, achievements and progress files are not changed. Northern Realms and Monsters recipes are unchanged. No stats, extra cards, health, draw or hidden bonuses were added.

This makes the **same training results reordered**, not newly measured outcomes: Nilfgaard .056,.077,.104,.444,.733; Skellige .098,.140,.212,.425,.477. The small Nilfgaard difference is not evidence of statistically established difficulty, and all numbers describe this AI/control setup, not human challenge. Independent seeds 13–24 must verify the ordering before making a difficulty claim; no holdout result was available during this change.

`composition_origin_stage` tracks the original generated recipe rather than recursively numbering each permutation. The generator's original-index ordering is consequently **2,1,3,4,5 for both affected factions**, because the previous Skellige stage order already permuted original indices as 4,2,1,3,5. `tools/draft_campaign_decks.py` now emits `enabled-in-game` status. An isolated temporary-directory replay asserted **exact equality of the entire generated JSON with the active JSON**, not just selected fields. Independent structural checks passed all20 recipes, including leaders, factions, available copies, 22-unit minimum and ten-special maximum. The realms-1 Fog repair is preserved. `git diff --check` passed. Native engine benchmark/holdout integration remains the parent's CI task.
