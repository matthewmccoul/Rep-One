#include "core.h"

namespace mc {

bool Asmodeus::Is(cs::Ref<Monster> m) { return m->Id == MonsterId; }

Tup_low_top_4e748 Asmodeus::DropBand(cs::Ref<Monster> m) {
    return Tup_low_top_4e748{m->ItemDropLevel - 2, m->ItemDropLevel};
}

cs::String Asmodeus::Ending(bool inAParty) {
    cs::String s = cs::String("As the dust settles from the ferocious battle, ")
                 + (inAParty ? cs::String("the party suddenly realizes ") : cs::String("you suddenly realize "))
                 + "that the Prince of the Underworld was just defeated!\n\n";

    s += inAParty
        ? cs::String("As everybody regains their composure and gives thanks for their life, a member of the ")
          + "party notices that the body of Asmodeus has disappeared!\n\n"
          + "It is then that the party looks around at each other with wide eyes and the "
        : cs::String("As you regain your composure and give thanks for your life, you notice that the body ")
          + "of Asmodeus has disappeared!\n\n"
          + "It is then that you look around with wide eyes and the ";

    s += cs::String("realization of only one thing...\n\n")
       + "Something - or Someone - has magically taken the body!\n\n"
       + "Even though today has been victorious and will be recorded in the history books, "
       + (inAParty
            ? cs::String("the party continues on with uneasiness, wondering if there's some darker force at work...")
            : cs::String("you continue on with uneasiness, wondering if there's some darker force at work..."))
       + "\n\n...is this really the last of Asmodeus?";
    return s;
}

cs::String Asmodeus::LogLine(cs::Ref<Character> c, bool inAParty) {
    return inAParty ? GuildLog_Wrote::PartyBeatAsmodeus(c->Name) : GuildLog_Wrote::AloneBeatAsmodeus(c->Name);
}

}  // namespace mc
