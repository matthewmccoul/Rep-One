#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GuildHall {
    static Tup_said_refused_e7850 Greeting(cs::Ref<GuildDef> def, cs::Ref<Character> c, bool alreadyAMember);
    static cs::String JoinCaption(bool alreadyAMember);
    static cs::String ListRow(cs::String guild, int32_t levelOrZero);
    static cs::String Info(int64_t experienceNeeded, int64_t experienceHave, int64_t goldNeeded, VbCurrency goldHave,
                           int32_t questType, cs::String questTarget, bool questDone);
    inline static const cs::String InfoTitle = "Current Experience Information";
    inline static const cs::String NothingLeftToTeach = "You cannot learn any more from this Guild.";
    inline static const cs::String NotEnoughExperience = "You do not currently have enough experience to make a level.";
    inline static const cs::String NotEnoughGold = "You currently do not have enough gold to make a level.";
    inline static const cs::String MakeLevelTitle = "Make Level";
    static cs::String MadeLevel(int32_t level);
    inline static const cs::String Warning =
        cs::String("A WARNING!\n")
      + "Your combined experience is below that which is recommended.\n"
      + "Our requirements for advancement may be too difficult for you at this point.\n\n";
    static cs::String JoinQuestion(cs::String guild, bool priced, int64_t cost);
    inline static const cs::String JoinTitle = "JOIN GUILD";
};

}  // namespace mc
