// One-time deterministic importer for the pinned upstream `decks.js` file.
const fs = require("node:fs");
const vm = require("node:vm");
const [sourcePath, outputPath] = process.argv.slice(2);
if (!sourcePath || !outputPath) throw new Error("usage: node import_reference_decks.js <decks.js> <output.json>");
const context = {};
const source = fs.readFileSync(sourcePath, "utf8") + "\nglobalThis.importedDecks = premade_deck;";
vm.runInNewContext(source, context, { timeout: 5000 });
const decks = context.importedDecks.map((deck) => JSON.parse(deck));
fs.writeFileSync(outputPath, JSON.stringify(decks, null, 2) + "\n", "utf8");
console.log(`Imported ${decks.length} pinned reference decks.`);
