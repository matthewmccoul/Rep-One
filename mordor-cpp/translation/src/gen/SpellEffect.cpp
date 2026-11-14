#include "core.h"

namespace mc {

int32_t SpellEffect::Of(cs::Ref<Spell> s) { return s->Effect >= PartyOffset ? s->Effect - PartyOffset : s->Effect; }

bool SpellEffect::IsParty(cs::Ref<Spell> s) { return s->Effect >= PartyOffset; }

bool SpellEffect::IsState(int32_t effect) {
    return effect == Levitate || effect == Invisible || effect == SeeInvisible || effect == Protect;
}

int32_t SpellEffect::ResistKind(int32_t effect) {
    return effect >= ResistFirst && effect < ResistFirst + Effects::ResistanceKinds ? effect - ResistFirst : -1;
}

cs::String SpellEffect::ApplyToCharacter(int32_t effect, cs::Ref<Character> c, cs::Ref<Spell> s, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item) {
    auto kind = ResistKind(effect);
    if (kind >= 0) {
        // raised to 95 where lower and nothing said where not (MAINMOD2.BAS:1106-1111)
        if (c->SpellResist[kind] < ResistGranted) {
            c->SpellResist[kind] = ResistGranted;
            Effects::Recompute(c, item);
        }
        return "";
    }

    switch (effect) {
        case Levitate:
        case Invisible:
        case SeeInvisible:
        case Protect:
            // state bit position equals the effect id (MAINMOD2.BAS mc0206 = mc0204)
            c->SpellEffects |= 1 << effect;
            Effects::Recompute(c, item);
            if (effect == Protect) {
                // MAINMOD2.BAS:1046 uses the caster's spell level halved; deliberately reproduced
                c->Protection = Spellbook::SpellLevelFor(c, s) / 2 + 1;
            }
            return "";

        case Fate:
            rng->Next(3);   // drawn and never used (MAINMOD2.BAS:1913)
            c->CurHits = cs::CastDouble<int32_t>(rng->NextDouble() * c->MaxHits) + 1;
            c->CurSpellPoints = cs::CastDouble<int32_t>(rng->NextDouble() * c->MaxSpellPoints) + 1;
            return "";

        case SetSanctuary:
            // stored 1-based, so that a zero means the spell has never been cast
            c->SanctuaryX = c->X + 1; c->SanctuaryY = c->Y + 1; c->SanctuaryLevel = c->DungeonLevel;
            return "";

        default:
            return {};  // C# null
    }
}

Tup_X_Y_Level_SpellEffect SpellEffect::TeleportTo(int32_t x, int32_t y, Facing facing, int32_t level, int32_t right, int32_t forwards, int32_t down, int32_t width, int32_t height) {
    // 1-based while we do this, because the wrap is written around a 1..30 grid
    int32_t nx = x + 1, ny = y + 1;
    switch (facing) {
        case Facing::North: nx += right; ny += forwards; break;
        case Facing::East: nx += forwards; ny -= right; break;
        case Facing::South: nx -= right; ny -= forwards; break;
        default: nx -= forwards; ny += right; break;
    }

    nx = Wrap(nx, width);
    ny = Wrap(ny, height);

    if (down != 0) level += cs::Math::Clamp<int32_t>(down, -3, 3);
    if (level < 0) level = 0;
    return Tup_X_Y_Level_SpellEffect{nx - 1, ny - 1, level};
}

Tup_Right_Forward_Down_SpellEffect SpellEffect::RelativeDirection(int32_t myX, int32_t myY, int32_t myLevel, Facing facing, int32_t theirX, int32_t theirY, int32_t theirLevel) {
    int32_t dx = theirX - myX, dy = theirY - myY;
    int32_t right, forward;
    if (facing == Facing::North) { right = dx; forward = dy; }
    else if (facing == Facing::East) { right = -dy; forward = dx; }
    else if (facing == Facing::South) { right = -dx; forward = -dy; }
    else { right = dy; forward = -dx; }
    return Tup_Right_Forward_Down_SpellEffect{right, forward, theirLevel - myLevel};
}

int32_t SpellEffect::Wrap(int32_t v, int32_t size) {
    if (v >= 1 && v <= size) return v;
    int32_t step = cs::Math::Abs(cs::CastDouble<int32_t>(cs::Math::Floor((v - 1) / 30.0)) * 30);
    return v < 1 ? v + step : v - step;
}

int32_t SpellEffect::BindLevelFrom(int32_t spellLevel, int32_t casterSpellLevel) {
    return cs::CastDouble<int32_t>(cs::Math::Round(spellLevel + cs::Math::Log((10.0 + casterSpellLevel) / 5), cs::MidpointRounding::ToEven));
}

cs::String SpellEffect::BindCompanion(cs::Ref<Companion> comp, int32_t slot, cs::Ref<Spell> s, int32_t casterSpellLevel) {
    if (Of(s) == BindForLife) {
        comp->BindingLevel = Binding::ForLife;
        cs::String called = cs::Str::Trim(comp->Name) == "" ? "Companion #" + cs::ToStr(slot + 1) : "\"" + cs::Str::Trim(comp->Name) + "\"";
        return called + " is now bound to you for life!";
    }
    int32_t level = BindLevelFrom(s->Level, casterSpellLevel);
    if (comp->BindingLevel >= level) return "\"" + s->Name + "\" had no effect!";
    comp->BindingLevel = level;
    return "\"" + s->Name + "\" raised the Bind Level of companion #" + cs::ToStr(slot + 1) + " to " + cs::ToStr(level) + ".";
}

int32_t SpellEffect::OpensLockUpTo(int32_t effect) {
    return effect >= OpenFirst && effect <= OpenLast ? effect - (OpenFirst - 1) : 0;
}

bool SpellEffect::IsRaise(int32_t effect) { return effect >= RaiseFirst && effect <= RaiseLast; }

int32_t SpellEffect::CompanionRaiseChance(int32_t monsterCon) { return cs::Math::Min(98, 85 + monsterCon); }

bool SpellEffect::RaisesToFull(int32_t effect) { return effect == RaiseFull; }

}  // namespace mc
