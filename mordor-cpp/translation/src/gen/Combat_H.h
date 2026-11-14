#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CombatEngine_ParalysisView {
    int32_t Index(cs::Ref<Character> i);
    void SetIndex(cs::Ref<Character> i, int32_t v);
    bool Remove(cs::Ref<Character> c);
};

struct CombatEngine_CombatLine {
    cs::String Text{};
    cs::Ref<Character> Actor = nullptr;
    bool Paced = false;
    cs::String SwingSound{};
    cs::List<cs::String> HitSounds;
    LineChannel Channel = LineChannel::Log;
    bool OpensChest = false;
    cs::List<cs::String> CastSounds;
    cs::List<cs::String> PickSounds;
    cs::Nullable<int32_t> AimAt{};
    MessageKind Kind = MessageKind::None;

    CombatEngine_CombatLine(cs::String Text = {}, cs::Ref<Character> Actor = {}, bool Paced = false,
                            cs::String SwingSound = {}, cs::List<cs::String> HitSounds = {},
                            LineChannel Channel = LineChannel::Log, bool OpensChest = false,
                            cs::List<cs::String> CastSounds = {}, cs::List<cs::String> PickSounds = {},
                            cs::Nullable<int32_t> AimAt = {}, MessageKind Kind = MessageKind::None)
        : Text(Text), Actor(Actor), Paced(Paced), SwingSound(SwingSound), HitSounds(HitSounds),
          Channel(Channel), OpensChest(OpensChest), CastSounds(CastSounds), PickSounds(PickSounds),
          AimAt(AimAt), Kind(Kind) {}

    bool operator==(const CombatEngine_CombatLine& o) const {
        return Text == o.Text && Actor == o.Actor && Paced == o.Paced && SwingSound == o.SwingSound
            && HitSounds == o.HitSounds && Channel == o.Channel && OpensChest == o.OpensChest
            && CastSounds == o.CastSounds && PickSounds == o.PickSounds && AimAt == o.AimAt && Kind == o.Kind;
    }
    bool operator!=(const CombatEngine_CombatLine& o) const { return !(*this == o); }
};

struct CombatEngine_ItemOrder {
    cs::Ref<InventorySlot> Slot = nullptr;
    cs::Ref<mc::Item> Item = nullptr;
    bool ItemCast = false;
};

struct CombatEngine_CompanionRound {
    bool Breathed = false;
    int32_t Kills = 0;
    cs::String Suffix = "";
    bool Cast = false;
    bool Over = false;
};

struct CombatEngine {
    using ParalysisView = CombatEngine_ParalysisView;
    using Blow = CombatEngine_Blow;
    using CombatLine = CombatEngine_CombatLine;
    using ItemOrder = CombatEngine_ItemOrder;
    using CompanionRound = CombatEngine_CompanionRound;

    cs::Ref<cs::Random> _rng = nullptr;
    cs::Func<cs::Ref<mc::Item>(int16_t)> _item;
    cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> _monsterDef;
    cs::HashSet<cs::Ref<Character>> _deadAtStart;
    cs::List<cs::Ref<Spell>> _spells;
    cs::List<CombatEngine_CombatLine> _said = cs::List<CombatEngine_CombatLine>::New();
    static constexpr int32_t CompanionSlotsWalked = 4;
    inline static cs::List<cs::String> SpellVerbs = cs::List<cs::String>::Of({
        "Melted", "Fried", "Froze", "Scared", "Drained", "Electrocuted", "Dispelled", "Banished",
        "Dazzled", "Immolated", "Shocked", "Slept", "Choked", "Killed", "Rotted", "Stoned", "Teleported",
    });
    inline static cs::List<cs::String> CountWords = cs::List<cs::String>::Of({
        "Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", "A Lot",
    });
    cs::Ref<mc::Party> Party = nullptr;
    cs::List<cs::Ref<CombatMonster>> Monsters = cs::List<cs::Ref<CombatMonster>>::New();
    cs::List<cs::String> Log = cs::List<cs::String>::New();
    bool Fled = false;
    cs::Dictionary<cs::Ref<Character>, int64_t> Experience = cs::Dictionary<cs::Ref<Character>, int64_t>::New();
    int64_t GoldEarned = 0;
    int32_t ChestClass = 0;
    int32_t ChestTrap = 0;
    int32_t ChestGuard = 0;
    int32_t ChestLocked = 0;
    cs::Ref<CombatEngine_ParalysisView> Paralyzed = cs::New<CombatEngine_ParalysisView>();
    cs::Dictionary<cs::Ref<Character>, int64_t> GoldRecovered = cs::Dictionary<cs::Ref<Character>, int64_t>::New();
    cs::List<int16_t> ItemsStolen = cs::List<int16_t>::New();
    cs::List<cs::String> ItemsDestroyed = cs::List<cs::String>::New();
    bool LastCastTookTurn = true;
    cs::Ref<mc::Encounter> Source = nullptr;
    cs::List<Tup_killer_def_3ffae> RecordableKills = cs::List<Tup_killer_def_3ffae>::New();
    cs::Func<void(CombatDamage)> DamageDealt;
    cs::Func<void(CombatExperience)> ExperienceGained;
    cs::String GoldLine{};
    cs::Dictionary<cs::Ref<Character>, int32_t> ChestRounds = cs::Dictionary<cs::Ref<Character>, int32_t>::New();
    cs::Dictionary<cs::Ref<Character>, cs::Ref<CombatEngine_ItemOrder>> ItemOrders = cs::Dictionary<cs::Ref<Character>, cs::Ref<CombatEngine_ItemOrder>>::New();
    bool AntiMagicHere = false;
    bool SoundFxOff = false;
    bool NoPartySound = false;
    cs::List<cs::String> LastCaptions = cs::List<cs::String>::New();
    cs::List<int32_t> LastCaptionAims = cs::List<int32_t>::New();
    int32_t LastCastAim = -1;
    cs::List<cs::String> LastCaptionSounds;
    cs::List<cs::String> LastSaidSounds;
    cs::Func<void(cs::Ref<Character>)> AntiMagicRefused;

    int64_t ExpEarned();
    int64_t GoldStolen();
    cs::Ref<Monster> ChestLead();
    cs::List<cs::Ref<Character>> NewlyDead();
    cs::Ref<Character> Actor();
    bool IsOver();
    bool Won();
    bool PartyWiped();
    int32_t Killed();
    void UnlockChest();
    CombatEngine(cs::Ref<mc::Party> party, cs::List<cs::Ref<CombatMonster>> monsters, cs::Func<cs::Ref<mc::Item>(int16_t)> itemLookup, cs::Ref<cs::Random> rng = {}, bool isLair = false);
    static int64_t GoldAfterClass(cs::Decimal fightGold, int32_t treasureClass);
    CombatEngine(cs::Ref<Character> lone, cs::List<cs::Ref<CombatMonster>> monsters, cs::Func<cs::Ref<mc::Item>(int16_t)> itemLookup, cs::Ref<cs::Random> rng = {}, bool isLair = false);
    void NoteWhoIsAlive();
    void UseMonsterTable(cs::List<cs::Ref<Monster>> table);
    void UseCompanionDefs(cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> lookup);
    void UseSpellTable(cs::List<cs::Ref<Spell>> spells);
    int32_t ParalyzedRounds(cs::Ref<Character> c);
    int32_t StatsADOf(cs::Ref<Character> c);
    cs::List<int32_t> StatsWithGear(cs::Ref<Character> c);
    int32_t AttackOf(cs::Ref<Character> c);
    int32_t DefenseOf(cs::Ref<Character> c);
    int32_t EquippedSum(cs::Ref<Character> c, cs::Func<int32_t(cs::Ref<mc::Item>)> f);
    static int32_t WornSum(cs::Ref<Character> c, cs::Func<cs::Ref<mc::Item>(int16_t)> item, cs::Func<int32_t(cs::Ref<mc::Item>)> f);
    cs::Ref<mc::Item> WeaponOf(cs::Ref<Character> c);
    static int32_t SizeOf(cs::Ref<Character> c);
    static int32_t Clamp(int32_t v, int32_t lo, int32_t hi);
    static Tup_def_level_ba376 BestAbilityGuild(cs::Ref<Character> c, cs::Func<float(cs::Ref<GuildDef>)> pick);
    static Tup_mod_level_cd3a8 WinningGuild(cs::Ref<Character> c, int32_t ability);
    void Credit(cs::Ref<Character> who, cs::Ref<CombatMonster> target, int32_t dmg, bool byCompanion, DamageKind kind, bool removes = true);
    void GiveExperience(cs::Ref<Character> who, int64_t xp, ExperienceSource source = ExperienceSource::Damage);
    void AwardSpoils();
    cs::List<CombatEngine_CombatLine> TakeSaid();
    void KilledBy(cs::Ref<Character> killer, cs::Ref<Monster> def, DamageKind how);
    void CompanionKill(cs::Ref<Character> owner, cs::Ref<CombatMonster> victim);
    void Dealt(cs::Ref<Character> who, int32_t amount, DamageKind kind, int32_t kills);
    void QuestKill(cs::Ref<Monster> def);
    void PayGold();
    void TrapSprung();
    void ChestEmptied();
    int32_t ChestRoundsOf(cs::Ref<Character> c);
    static cs::Nullable<int32_t> OpeningRounds(double disarm);
    CombatEngine_CombatLine ChestCaption();
    cs::List<CombatEngine_CombatLine> ChestTick(cs::Ref<Character> c);
    cs::List<CombatEngine_CombatLine> PlayerSwings(cs::Ref<Character> actor, cs::Nullable<int32_t> targetIndex = {});
    cs::Ref<CombatMonster> TargetFor(cs::Nullable<int32_t> targetIndex);
    cs::String PlayerAttacks(cs::Ref<Character> actor, cs::Nullable<int32_t> targetIndex = {});
    cs::List<cs::Ref<mc::Item>> WeaponsOf(cs::Ref<Character> c);
    Tup_blow_said_sounds_bb3b1 SwingOnce(cs::Ref<Character> actor, cs::Ref<CombatMonster> target, cs::Ref<mc::Item> weapon);
    cs::String Caption(cs::Ref<Character> actor, cs::String text);
    static cs::String SwingSoundFor(cs::Ref<mc::Item> weapon);
    static cs::String DeathSound(cs::Ref<Monster> def);
    cs::String PlayerAttacks(cs::Nullable<int32_t> targetIndex = {});
    cs::List<cs::String> Round(cs::List<cs::Ref<Spell>> spells);
    cs::List<CombatEngine_CombatLine> RoundPaced(cs::List<cs::Ref<Spell>> spells);
    cs::List<CombatEngine_CombatLine> WithSaid(cs::List<CombatEngine_CombatLine> lines);
    cs::List<CombatEngine_CombatLine> CharacterActs(cs::Ref<Character> c, cs::List<cs::Ref<Spell>> spells);
    void CastSpellDraw();
    cs::Nullable<CombatEngine_CombatLine> ItemCharge(cs::Ref<Character> c, cs::Ref<CombatOrder> order, cs::Ref<CombatEngine_ItemOrder> io);
    cs::List<CombatEngine_CombatLine> ItemOrderActs(cs::Ref<Character> c, cs::Ref<CombatOrder> order, cs::Ref<Spell> s, cs::Ref<CombatEngine_ItemOrder> io);
    cs::List<CombatEngine_CombatLine> CompanionTurn(cs::Ref<Character> owner);
    void CompanionsCleared(cs::Ref<Character> owner);
    cs::Ref<CombatMonster> FoeOf(cs::Ref<Character> owner);
    void Sub0D08(cs::Ref<CombatMonster> aimed, cs::Ref<CombatMonster> fallen);
    void KeepRecord(cs::Ref<CombatMonster> fallen);
    cs::Ref<CombatMonster> AimedRecord(cs::Ref<Character> who);
    void Retarget(cs::Ref<Character> who);
    cs::Ref<CombatMonster> NextInGroup(cs::Ref<CombatMonster> m);
    static cs::String HisHer(cs::Ref<Character> c);
    static cs::String Accidently(cs::Ref<Character> owner, cs::Ref<Companion> c);
    static CombatEngine_CombatLine CompanionCry(cs::Ref<Character> guard, cs::String sound);
    cs::List<CombatEngine_CombatLine> CompanionBreathes(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Ref<CombatEngine_CompanionRound> round);
    cs::List<CombatEngine_CombatLine> CompanionSpits(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, int32_t victim, int32_t slot, cs::Ref<CombatEngine_CompanionRound> round);
    cs::List<CombatEngine_CombatLine> CompanionStrikes(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, int32_t victim, int32_t slot, cs::Ref<CombatEngine_CompanionRound> round);
    static int32_t StatOf(cs::Ref<Monster> m, int32_t i);
    cs::List<CombatEngine_CombatLine> CompanionCasts(cs::Ref<Character> owner, cs::Ref<Companion> c, cs::Ref<Monster> def, cs::Ref<Spell> spell, cs::Ref<CombatEngine_CompanionRound> round);
    int32_t CompanionSpellOnParty(cs::Ref<Spell> s, cs::Ref<Monster> caster, cs::Ref<Monster> facing, cs::Ref<Character> hurt, cs::Ref<Companion> victimComp);
    cs::String CastSpell(cs::Ref<Character> actor, cs::Ref<Spell> s, cs::Nullable<int32_t> targetIndex = {}, bool free = false, int32_t itemLevel = 0);
    cs::List<CombatEngine_CombatLine> CaptionLines(cs::Ref<Character> caster);
    cs::String CastDamage(cs::Ref<Character> actor, cs::Ref<Spell> s, cs::Nullable<int32_t> targetIndex, int32_t level, int32_t compSlot = -1, cs::Ref<Monster> compDef = {});
    int32_t SpellHitsOn(cs::Ref<Spell> s, int32_t level, cs::Ref<Monster> target, cs::Ref<Monster> compDef);
    cs::String CastSpell(cs::Ref<Spell> s, cs::Nullable<int32_t> targetIndex = {}, bool free = false, int32_t itemLevel = 0);
    cs::String Who(cs::Ref<Character> c);
    cs::String Named(cs::Ref<CombatMonster> m, int32_t count = 1);
    cs::String NonCombatInFight(cs::Ref<Character> c, cs::Ref<Spell> s);
    cs::Nullable<CombatEngine_CombatLine> RoundCheckCast(cs::Ref<Character> c, cs::Ref<CombatOrder> order, cs::Ref<Spell> s, int32_t itemLevel);
    cs::Ref<Companion> Charmed(cs::Ref<Character> caster, cs::Ref<CombatMonster> m, int32_t hitsBefore, cs::Ref<Spell> s);
    static int32_t CharmIdLevel(cs::Ref<Character> c, cs::Ref<Monster> def, cs::Func<cs::Ref<mc::Item>(int16_t)> item);
    static int32_t IdLevelRoll(cs::Ref<Character> c, cs::Ref<Monster> def, cs::Func<cs::Ref<mc::Item>(int16_t)> item, bool p036E);
    void WalkAway();
    void Say(cs::String s);

    // MonsterAttack.cs
    cs::List<cs::String> PendingPickSounds;
    bool NoSpellsHere = false;
    cs::Func<void(cs::Ref<Character>, cs::Ref<CombatMonster>)> CharacterKilled;
    cs::Func<void(cs::Ref<Character>)> PartyUpdate;
    cs::Func<void()> ListUpdate;
    static bool Bit(int32_t flags, int32_t bit);
    static int32_t Floor(double v);
    static int32_t VbInt(double v);
    int32_t Res(cs::Ref<Character> c, int32_t kind);
    static int32_t Res(cs::Ref<Monster> m, int32_t kind);
    static int32_t MaxGuildLevel(cs::Ref<Character> c);
    cs::Ref<Monster> DefOf(cs::Ref<Companion> comp);
    cs::List<cs::List<cs::Ref<CombatMonster>>> AliveGroups();
    static int32_t AttackChance(AttackKind kind, int32_t monDex, int32_t charDex);
    cs::List<cs::String> MonstersAttack();
    cs::List<CombatEngine_CombatLine> MonsterAttacks(AttackKind kind = AttackKind::Round);
    static int32_t TheftSlot(cs::Ref<cs::Random> rng);
    static cs::Nullable<int64_t> GoldToSteal(VbCurrency onHand, int64_t rolled);
    static int32_t AcidTarget(cs::Ref<cs::Random> rng, cs::Ref<CompanionSlots> slots);
    cs::List<CombatEngine_CombatLine> GroupAttacks(cs::Ref<Character> c, cs::List<cs::Ref<CombatMonster>> group);
    cs::Ref<Spell> ChooseSpell(cs::Ref<Monster> def);
    CombatEngine_CombatLine Picked(CombatEngine_CombatLine line);
    int32_t MonsterSpellHits(cs::Ref<Character> c, cs::Ref<Spell> spell, cs::Ref<Monster> caster, cs::Ref<Companion> comp);
    static int32_t ProtectionBonus(cs::Ref<Character> c);
    static cs::String AttackSound(cs::Ref<Monster> def);
    static cs::List<cs::String> HurtSounds(cs::Ref<Monster> def);
    static cs::String CompanionName(cs::Ref<Companion> comp, int32_t index);
    cs::String NameFor(cs::Ref<Monster> def, cs::Ref<Character> viewer, cs::Ref<CombatMonster> m, int32_t count);
    CombatEngine_CombatLine Caption(cs::Ref<Character> c, cs::String text, cs::List<cs::String> sounds);
    CombatEngine_CombatLine Message(cs::String text, cs::List<cs::String> sounds = {}, cs::Ref<Character> soundOf = {});
    CombatEngine_CombatLine FoeMessage(cs::String text, cs::List<cs::String> sounds = {}, cs::Ref<Character> soundOf = {});
    cs::List<CombatEngine_CombatLine> TakeDamage(cs::Ref<Character> c, int32_t dmg, bool silent = false);
    cs::List<CombatEngine_CombatLine> OuchLine(cs::Ref<Character> c, int32_t dmg);
    static void Dies(cs::Ref<Character> c, int32_t cause);
    static void SetDead(cs::Ref<Character> c, int32_t cause);
    cs::List<CombatEngine_CombatLine> CompanionFalls(cs::Ref<Character> c, bool repaint = true);
    void ShowCompanions(cs::Ref<Character> c);
    static void RemoveItem(cs::Ref<Character> c, cs::Ref<InventorySlot> slot);
};

}  // namespace mc
