namespace mordor {
i32 mordor_export_monsterSpellDamage(Ctx& ctx, i32 monsterLevel, i32 damage2, i32 specialEffect,
                                     bool hasResist, i32 resistPercent) {
    return monsterSpellDamage(ctx, monsterLevel, damage2, specialEffect, hasResist, resistPercent);
}
} // namespace mordor
