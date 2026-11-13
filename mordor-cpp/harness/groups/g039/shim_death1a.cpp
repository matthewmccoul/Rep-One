namespace mordor {
// The harness renamed this file's pruneTheDead to pruneTheDead__death1a.
void shim_death1a_prune(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead) {
    pruneTheDead__death1a(ctx, party, countedDead);
}
}  // namespace mordor
