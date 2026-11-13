namespace mordor {
i32 shim_export_respawnDelaySeconds(i32 level, i32 areaCount, i32 tally) {
    return respawnDelaySeconds(level, areaCount, tally);
}
}  // namespace mordor
