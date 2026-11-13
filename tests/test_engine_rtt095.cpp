// Batch t095: translated from reference assertions on Spellbook / SpellBuffers.
// All cases are skipped until our declarations for the spellbook, buffer and
// castables API are provided (see NEED line).
#include "test_support.h"

using namespace mordor;

// SKIP B_0066#23: our SpellBuffers API (buffer slot state after Describe) is not declared
// SKIP B_0066#25: our SpellBuffers.Describe / costText and Spellbook cost API are not declared
// SKIP B_0063#38: our Spellbook.Castables API and item-carrier (Elixr of Fate) support are not declared
// SKIP B_0062#62: our Spellbook.Castables API and PutInPack for carrier items are not declared
// SKIP B_0061#57: our Spellbook.Cost API and multi-guild Character setup are not declared
// SKIP B_0061#61: our Spellbook.Cost API (Human Sorcerer, Points column) is not declared
// SKIP B_0061#62: our Spellbook.Cost API (Human Sorcerer, Firebolt) is not declared
// SKIP B_0061#63: our Spellbook.Cost API (Nomad/Sorcerer multi-guild, Charm of Opening) is not declared
