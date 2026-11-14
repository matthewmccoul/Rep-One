#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Confinement {
    static constexpr int32_t ShelfCap = 9;

    cs::Dictionary<cs::String, int32_t> Pens = cs::Dictionary<cs::String, int32_t>::New();

    cs::String KeyedBy{};

    inline static const cs::String ByRef = "Ref";

    static cs::String PathFor(cs::String savesDir);
    static cs::String Key(int32_t tableIndex, ItemAlignment align);

    static cs::Nullable<int32_t> RefOf(cs::Ref<Monster> def);

    bool _idKeyed = false;

    cs::List<Tup_Ref_Align_Count_8a8ac> Entries();

    static cs::Nullable<Tup_Ref_Align_38db7> Parse(cs::String key);

    static cs::Ref<Confinement> Load(cs::String savesDir, cs::List<cs::Ref<Monster>> monsters);

    int32_t FromIdKeys(cs::List<cs::Ref<Monster>> monsters);

    int32_t RepairUnaligned(cs::List<cs::Ref<Monster>> monsters);

    static cs::Ref<Confinement> Load(cs::String savesDir);

    void Save(cs::String savesDir);

    int32_t Count(cs::Ref<Monster> def, ItemAlignment align);

    int32_t Count(int32_t tableIndex, ItemAlignment align);

    bool AnyOf(cs::Ref<Monster> def);

    void Take(cs::Ref<Monster> def, ItemAlignment align);

    void Add(cs::Ref<Monster> def, ItemAlignment align);

    static double Cur(double v);

    static int64_t Lng(double v);

    static double BaseValue(cs::Ref<Monster> def);

    int64_t PriceToBuy(cs::Ref<Monster> def, ItemAlignment align);

    int64_t PriceToSell(cs::Ref<Monster> def, ItemAlignment align, int32_t idLevel);

    int64_t PriceToIdentify(cs::Ref<Monster> def, ItemAlignment align, int32_t idLevel);

    static int64_t PriceToRealign(cs::Ref<Monster> def, ItemAlignment from, ItemAlignment to);

    static int32_t BoughtBinding(cs::Ref<Monster> def);

    inline static const cs::String NoRoom = "No more room for companions!";
    inline static const cs::String NotEnoughGold = "Not enough gold!";
    inline static const cs::String NoDeadOnes = "We don't have any interest in dead companions!";
    inline static const cs::String FreeWillCannotBind = "You cannot bind a companion that has joined you free of will.";
    inline static const cs::String AlreadyBound = "Companion is already bound to or beyond our abilities!";

    static cs::String RealignOffer(int64_t cost);

    static cs::Ref<Confinement> Fresh(cs::List<cs::Ref<Monster>> monsters);

    static ItemAlignment AlignmentOf(cs::Ref<Monster> m);

    // default deepestReached = StoreStock::DeepestStocked (overload: StoreStock is incomplete here)
    void Replenish(cs::List<cs::Ref<Monster>> monsters, int32_t days, cs::Ref<cs::Random> rng);
    void Replenish(cs::List<cs::Ref<Monster>> monsters, int32_t days, cs::Ref<cs::Random> rng, int32_t deepestReached);

    static cs::Ref<Monster> DrawOne(cs::Dictionary<int32_t, cs::List<cs::Ref<Monster>>> byKind, int32_t kinds, int32_t floor, cs::Ref<cs::Random> rng);

    static ItemAlignment PenIndex(int32_t i);

    bool PenOne(cs::Ref<Monster> m, int32_t l0288);

    bool ReleaseOne(cs::Ref<Monster> m, cs::Ref<cs::Random> rng, int32_t& l0288);
};

}  // namespace mc
