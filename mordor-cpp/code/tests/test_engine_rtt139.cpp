#include "test_support.h"

using namespace mordor;

// SKIP B_0065#1: needs the combat round API (party, monster list, spells, seeded rng, per-round electrocution counter); no declaration in our headers.
// SKIP B_0064#62: needs specialHitsRaw(rng, SpecialAttackKind, attackerGuildLevel, targetLevel); no declaration in our headers.
// SKIP B_0064#63: needs specialHitsRaw(rng, SpecialAttackKind, attackerGuildLevel, targetLevel); no declaration in our headers.
// SKIP B_0065#103: needs spellEffectBindCompanion(Companion&, slot, Spell, casterSpellLevel) and a BindFirst spell fixture; no declaration in our headers.
// SKIP B_0065#104: needs spellEffectBindCompanion(Companion&, slot, Spell, casterSpellLevel) and a BindFirst spell fixture; no declaration in our headers.
// SKIP B_0065#105: needs spellEffectBindCompanion(Companion&, slot, Spell, casterSpellLevel) and a BindForLife spell fixture; no declaration in our headers.
// SKIP B_0064#76: needs the combat round API and per-round breath/mishap/monster-breath counters; no declaration in our headers.
// SKIP B_0064#101: needs CompanionCarry::run and a too-big party fixture; no declaration in our headers.
