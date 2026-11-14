#include "core.h"

namespace mc {

const int32_t LimitedVersion::LastLevel = GameFiles::SharewareLevels;
const int32_t LimitedVersion::DeepestLevel = GameFiles::FullLevels;

void LimitedVersion::Apply(cs::Ref<GameFilesCheck> check) {
    On = (check ? check->Verdict == GameFilesVerdict::Limited : false);
    _program = On ? ReadProgram(cs::deref(check).Dir) : Tup_Date_Price_b4588{};
}

cs::String LimitedVersion::FrameCaption() { return On ? "Mordor 1.1 (Limited Version)" : "Mordor 1.1"; }

bool LimitedVersion::SaysGoodbye() { return !On; }

bool LimitedVersion::ShowsOrderingInfo() { return On; }

bool LimitedVersion::ShowsPublicBox() { return On; }

bool LimitedVersion::ShowsLimitationNotice() { return On; }

cs::String LimitedVersion::LimitationNoticeText() {
    return cs::String("NOTICE!\n")
        + "Because this is a PUBLIC BETA version of MORDOR, only " + cs::ToStr(LastLevel) + " dungeon levels are currently available.\n"
        + "\n"
        + "This message will not appear again.";
}

bool LimitedVersion::CloseBoxAsksToExit() { return On; }

bool LimitedVersion::AboutReadsOrdering() { return On; }

cs::String LimitedVersion::LevelDenied(int32_t level) {
    return "LIMITED VERSION: Request to go to Level " + cs::ToStr(level) + " is Denied.";
}

Tup_Level_Said_08d2b LimitedVersion::Clamp(int32_t level) {
    if (!On || level < 0 || level > DeepestLevel || level <= LastLevel) return Tup_Level_Said_08d2b{level, cs::String()};
    return Tup_Level_Said_08d2b{LastLevel, LevelDenied(level)};
}

bool LimitedVersion::ChuteIsDeadToTake(int32_t level) { return On && level == LastLevel; }

bool LimitedVersion::ChuteIsDeadUnderfoot(int32_t level) { return On && level == LastLevel; }

bool LimitedVersion::TeleporterIsDead(int32_t z2) { return On && z2 > LastLevel; }

cs::String LimitedVersion::LastEdited() {
    return On ? (!_program.Date.empty() ? _program.Date : SharewareLastEdited) : FullLastEdited;
}

cs::String LimitedVersion::Price() {
    return On ? (!_program.Price.empty() ? _program.Price : SharewarePrice) : FullPrice;
}

Tup_Date_Price_b4588 LimitedVersion::ReadProgram(cs::String gameDir) {
    try {
        cs::String exe = GameArt::Find(gameDir, "MORDOR.EXE");
        // MORDOR.EXE is 1.2 MB; a file far larger is not it
        if (exe.empty()) return Tup_Date_Price_b4588{};
        std::error_code ec;
        auto len = std::filesystem::file_size(exe, ec);
        if (ec) throw cs::IOException("Could not find file '" + exe + "'.");
        if ((int64_t)len > (16 << 20)) return Tup_Date_Price_b4588{};
        return FromProgram(cs::File::ReadAllBytes(exe));
    } catch (const cs::IOException&) {
        return Tup_Date_Price_b4588{};
    } catch (const cs::UnauthorizedAccessException&) {
        return Tup_Date_Price_b4588{};
    } catch (const cs::ArgumentException&) {
        return Tup_Date_Price_b4588{};
    } catch (const cs::NotSupportedException&) {
        return Tup_Date_Price_b4588{};
    }
}

Tup_Date_Price_b4588 LimitedVersion::FromProgram(cs::List<uint8_t> exe) {
    cs::String s = cs::Encoding::Latin1GetString(exe);
    cs::String date{}, price{};
    // Singleline: '.' written as [\s\S] so it also matches newlines
    std::regex dre(R"(Welcome to MORDOR 1\.1![\s\S][\s\S]\x06\x00Mordor[\s\S][\s\S]([\x01-\x10])\x00(\d{1,2}/\d{1,2}/\d{2})(?!\d))", std::regex::ECMAScript);
    std::smatch d;
    bool dOk = std::regex_search(s, d, dre);
    if (dOk && (int32_t)(unsigned char)d[1].str()[0] == (int32_t)d[2].length()) date = d[2].str();
    std::regex pre(R"(  You can now pre-order the full version of MORDOR at any time for only (\$\d{1,3}\.\d\d) \+ S&&H!)", std::regex::ECMAScript);
    std::smatch p;
    if (std::regex_search(s, p, pre)) price = p[1].str();
    return Tup_Date_Price_b4588{date, price};
}

cs::String LimitedVersion::OrderInfo() { return OrderInfoAt(Price()); }

cs::String LimitedVersion::OrderInfoAt(cs::String price) {
    // U+2019 has no Latin-1 form; Windows-1252 0x92 as the original Windows text
    return cs::String("Whether you\x92re ordering MORDOR for yourself or as a gift, this first of a kind Windows FRP game designed to be played for months and even longer is guaranteed to put a smile on most any avid FRPer\x92s face!!  ")
        + "The full commercial version of MORDOR has so many more Monsters, Items and Dungeon Levels than the PUBLIC version that we can\x92t even begin to get into detail.  Find out for yourself!\n\n"
        + "  You can now pre-order the full version of MORDOR at any time for only " + price + " + S&H!  Feel free to use your VISA, MASTERCARD, Check, or Money Order to have MORDOR: the Depths of Dejenol, sent directly to your doorstep!\n\n";
}

}  // namespace mc
