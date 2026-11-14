#include "core.h"

namespace mc {

cs::Ref<SaveImport_ImportPlan> SaveImport::Plan(cs::Ref<OriginalSaves> src, cs::String savesDir) {
    auto existing = ExistingNames(savesDir);
    auto chars = cs::List<cs::Ref<CharacterChoice>>::New();
    // OrdinalIgnoreCase set: keys are stored lower-cased
    auto files = cs::HashSet<cs::String>::New();
    for (auto& ic : src->Characters) {
        if (ic->Character == nullptr) continue;
        cs::String file = Character::SafeFileName(ic->Name) + ".json";
        if (!files.Add(cs::Str::ToLower(file))) continue;   // a second record with the same file name: the first wins (noted when read)
        bool exists = cs::File::Exists(cs::Path::Combine(savesDir, file)) || existing.Contains(cs::Str::ToLower(cs::Str::TrimEnd(ic->Name)));
        chars.Add(cs::New<CharacterChoice>(ic, exists, file));
    }
    auto world = cs::List<cs::Ref<WorldChoice>>::New();
    auto W = [&](bool have, cs::String file, cs::String holds) {
        if (have) world.Add(cs::New<WorldChoice>(file, holds, cs::File::Exists(cs::Path::Combine(savesDir, file))));
    };
    W(src->Automap != nullptr, "automap.json", "the automap (MDATA8)");
    W(src->Dungeon != nullptr || src->DeepestReached.has_value(), "dungeon.json", "the rooms, respawn tallies and deepest level (MDATA10, MDATA8)");
    W(src->Store != nullptr, "store.json", "the General Store's shelves (MDATA6)");
    W(src->Library != nullptr, "library.json", "the Library (MDATA13)");
    W(src->Hall != nullptr, "records.json", "the Hall of Records (MDATA14)");
    W(src->Guildmasters != nullptr, "guildmasters.json", "the guildmasters (MDATA7)");
    W(src->Pens != nullptr, "confinement.json", "the Confinement's pens (MDATA15)");
    W(src->GuildLog != nullptr, GuildLog::FileName, "the guild log (MDATA9)");
    return cs::New<ImportPlan>(chars, world);
}

cs::HashSet<cs::String> SaveImport::ExistingNames(cs::String savesDir) {
    // OrdinalIgnoreCase set: names are stored lower-cased
    auto names = cs::HashSet<cs::String>::New();
    try {
        if (!cs::Directory::Exists(savesDir)) return names;
        for (auto& f : cs::Directory::GetFiles(savesDir, "*.json")) {
            if (!Backups::IsCharacterFile(f)) continue;
            try { names.Add(cs::Str::ToLower(cs::Str::TrimEnd(Character::ReadRaw(f)->Name))); } catch (...) { }
        }
    } catch (...) { }
    return names;
}

cs::String SaveImport::Backup(cs::String savesDir, cs::DateTime now) {
    if (!cs::Directory::Exists(savesDir)) return cs::String();
    auto files = cs::Directory::GetFiles(savesDir);
    if (files.Length() == 0) return cs::String();
    cs::String first = cs::Path::Combine(savesDir, "BACKUP", "before-import-" + now.ToString("yyyyMMdd-HHmmss"));
    cs::String dir = first;
    for (int32_t n = 2; cs::Directory::Exists(dir); n++) dir = first + "-" + cs::ToStr(n);
    cs::Directory::CreateDirectory(dir);
    for (auto& f : files) cs::File::Copy(f, cs::Path::Combine(dir, cs::Path::GetFileName(f)), false);
    return dir;
}

cs::Ref<SaveImport_ImportResult> SaveImport::Apply(cs::Ref<OriginalSaves> src, cs::String savesDir, cs::List<cs::String> characters, bool world, cs::DateTime now) {
    auto chosen = cs::HashSet<cs::String>::From(characters);
    auto plan = Plan(src, savesDir);
    cs::String backup = Backup(savesDir, now);
    cs::Directory::CreateDirectory(savesDir);
    auto written = cs::List<cs::String>::New();
    auto skipped = cs::List<cs::String>::New();

    auto existing = Roster::Saved(savesDir).Select([&](auto& p) { return p.Ch->Name; }).ToList();
    auto writtenNames = cs::HashSet<cs::String>::New();
    for (auto& choice : plan->Characters) {
        cs::String name = choice->Imported->Name;
        if (!chosen.Contains(name)) { skipped.Add(name); continue; }
        choice->Imported->Character->Save(cs::Path::Combine(savesDir, choice->File));
        Roster::Added(savesDir, name, existing);
        if (!existing.Contains(name)) existing.Add(name);
        writtenNames.Add(name);
        written.Add(choice->File);
    }
    for (auto& ic : src->Characters.Where([&](auto& c) { return c->Character == nullptr; })) skipped.Add(ic->Name);

    // a remembered party comes with its leader: party-<leader>.json, as Party.SaveParty writes it
    for (auto& p : src->Parties) {
        if (!writtenNames.Contains(p->Leader)) continue;
        cs::String file = "party-" + Character::SafeFileName(p->Leader) + ".json";
        (void)file;
        throw cs::NotSupportedException("not translated: JSON serialization of the party roster");
    }

    if (world) {
        if (src->Automap != nullptr) { src->Automap->Save(savesDir); written.Add("automap.json"); }
        if (src->Dungeon != nullptr || src->DeepestReached.has_value()) {
            auto d = (src->Dungeon ? src->Dungeon : DungeonState::Load(savesDir));
            if (src->DeepestReached.has_value()) { int32_t deepest = src->DeepestReached.value(); d->DeepestReached = deepest; }
            d->Save(savesDir);
            written.Add("dungeon.json");
        }
        if (src->Store != nullptr) { src->Store->Save(savesDir); written.Add("store.json"); }
        if (src->Library != nullptr) { src->Library->Save(savesDir); written.Add("library.json"); }
        if (src->Hall != nullptr) { src->Hall->Save(savesDir); written.Add("records.json"); }
        if (src->Guildmasters != nullptr) { src->Guildmasters->Save(savesDir); written.Add("guildmasters.json"); }
        if (src->Pens != nullptr) { src->Pens->Save(savesDir); written.Add("confinement.json"); }
        if (src->GuildLog != nullptr) { GuildLog::Rewrite(savesDir, src->GuildLog); written.Add(GuildLog::FileName); }
    }
    return cs::New<ImportResult>(backup, written, skipped);
}

cs::List<cs::String> SaveImport::Summary(cs::Ref<OriginalSaves> src) {
    auto lines = cs::List<cs::String>::Of({"Characters (" + cs::ToStr(src->Characters.Count([&](auto& c) { return c->Character != nullptr; })) + " of " + cs::ToStr(src->CharacterCount) + " in MDATA4):"});
    for (auto& ic : src->Characters) {
        auto c = ic->Character;
        if (c == nullptr) { lines.Add("  " + cs::ToStr(ic->Name) + ": not importable - " + cs::ToStr(ic->Refusal)); continue; }
        cs::String state = c->IsDead ? " - " + DeathStates::Name(c->CauseOfDeath) : "";
        cs::String where = c->DungeonLevel > 0 ? "level " + cs::ToStr(c->DungeonLevel) + " at " + cs::ToStr(c->X + 1) + "," + cs::ToStr(c->Y + 1) : "in town";
        lines.Add("  " + cs::ToStr(c->Name) + ": " + cs::ToStr(c->Race) + " " + cs::ToStr(c->Guild()->Guild) + " " + cs::ToStr(c->Guild()->Level) + ", " + cs::ToStr(c->CurHits) + "/" + cs::ToStr(c->MaxHits) + " hits, "
                  + CharStatFile::FormatNumber(c->GoldOnHand) + " gold, " + where + state);
    }
    if (src->Parties.Count() > 0)
        lines.Add("Parties: " + cs::Str::Join("; ", src->Parties.Select([&](auto& p) { return cs::Str::Join(", ", p->Members); })));
    lines.Add("Files:");
    for (auto& f : src->Files) lines.Add("  " + cs::ToStr(f->Name) + " - " + cs::ToStr(f->Holds) + ": " + cs::ToStr(f->Status));
    return lines;
}

}  // namespace mc
