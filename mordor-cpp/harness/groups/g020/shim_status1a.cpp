namespace mordor {

// Export wrapper for the file-local status1a copy (renamed to paralyse__status1a by the harness).
void paralyse_via_status1a(Ctx& ctx, Character& c, i32 level) {
    paralyse__status1a(ctx, c, level);
}

}  // namespace mordor

