#include "core.h"

namespace mc {

Tup_said_refused_e7850 GuildHall::Greeting(cs::Ref<GuildDef> def, cs::Ref<Character> c, bool alreadyAMember) {
    if (alreadyAMember) return Tup_said_refused_e7850{cs::String("\n") + (c->Sex == Sex::Female ? "Welcome, Sister." : "Welcome, Brother."), false};

    cs::String said = "";
    bool refused = false;

    // Plural is race name + "s", as the original builds it (FRMCTYG.BAS:790).
    if (!def->AllowsRace(c->Race)) { said = "We don't accept " + cs::ToStr(c->Race) + "s!\n"; refused = true; }
    if (!def->AllowsAlignment(c->Alignment)) { said += "You're " + ToStr(c->Alignment) + "!  Go away!\n"; refused = true; }
    for (int32_t i = 0; i < 6; i++)
        if (c->Stats[i] < def->ReqStats[i]) { said += "Your natural stats are too low to join!"; refused = true; break; }

    return refused ? Tup_said_refused_e7850{said, true} : Tup_said_refused_e7850{"Welcome, Visitor.", false};
}

cs::String GuildHall::JoinCaption(bool alreadyAMember) { return alreadyAMember ? "Re-Acquaint" : "Join"; }

cs::String GuildHall::ListRow(cs::String guild, int32_t levelOrZero) {
    return levelOrZero > 0 ? cs::ToStr(guild) + " (" + cs::ToStr(levelOrZero) + ")" : guild;
}

// cmdInfo_Click (FRMCTYG.BAS:45-81)
cs::String GuildHall::Info(int64_t experienceNeeded, int64_t experienceHave, int64_t goldNeeded, VbCurrency goldHave,
                           int32_t questType, cs::String questTarget, bool questDone) {
    cs::String t = "We require you to have " + cs::Format(experienceNeeded, "N0") + " experience to make the next level.\n"
                 + "You currently have " + cs::Format(experienceHave, "N0") + " experience.\n"
                 + "We also require " + cs::Format(goldNeeded, "N0") + " gold.\n"
                 + "You currently have " + cs::ToStr(CharStatFile::FormatNumber(goldHave)) + " gold.\n";
    if (questType > 0) {
        t += "\nYou have also been assigned a Quest to complete.\n";
        t += questType == 1
            ? "You must kill one " + cs::ToStr(questTarget) + ".\n"
            : "You must return with one " + cs::ToStr(questTarget) + " and present it when making level.\n";
        if (questDone) t += "Your quest has been completed.";
    }
    return t;
}

cs::String GuildHall::MadeLevel(int32_t level) { return "Congratulations!\nYou are now Level " + cs::ToStr(level) + "!"; }

// cmdJoin_Click (FRMCTYG.BAS:84-131)
cs::String GuildHall::JoinQuestion(cs::String guild, bool priced, int64_t cost) {
    return priced ? "Cost to join the " + cs::ToStr(guild) + "'s guild\nis " + cs::Format(cost, "N0") + ".\nDo you wish to join?"
                  : "Join the " + cs::ToStr(guild) + "'s guild?";
}

}  // namespace mc
