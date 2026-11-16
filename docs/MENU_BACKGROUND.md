# Menu background v1

File: assets/gwent-classic/img/menu/woodland-shrine-v1.png
Created 2026-11-16 using the built-in image_gen tool. This is newly generated
menu artwork; it is not an imported upstream game asset. UI text and panels
are rendered separately in code. The image is integrated into the main menu. Source card portraits are cropped
to illustration-only regions; selected tiles use a warm amber glow and aged-gold
frames. No numeric power or ability symbols are drawn in menu portraits.

Main menu navigation is left/right plus B. In the deck editor, B returns from
the deck panel to the collection, then a further B returns to the main menu.
The menu smoke test exercises these normalized Miyoo keys and verifies that
resuming without a save leaves the menu visible with a localized message.

## Generation prompt

Use case: stylized-concept. Production asset for the main menu of a Witcher 3 Gwent handheld port. Create ONLY a background illustration, landscape 4:3 composition, intended to be downscaled to 640x480. Dark medieval Slavic fantasy atmosphere, richly painted realistic game environment: an abandoned woodland shrine at dusk, weathered stone paving, gnarled roots, moss, candles and a warm lantern near the outer corners, faint cool mist behind. Inspired by the atmosphere of a premium dark fantasy card game menu. Keep the central 75 percent relatively calm, dark and low contrast so three large selectable illustrated card panels can be composited there later. Strong readable large shapes and restrained fine detail to survive handheld resolution. Top and bottom also dark enough for crisp UI lettering. No cards, no panels, no buttons, no portraits, no lettering, no logos, no HUD, no border. Background must fill whole image.
