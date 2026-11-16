// Hand-written thin wrapper. All behaviour comes from dd::quest (the core).
// PROPOSAL, NOT RUN - Unreal missing.
#include "DDQuestComponent.h"

#include "DDText.h"

#include "dd/map.h"
#include "dd/quest.h"

#include <memory>

namespace
{
// Hands the caller's roll to the core. The core decides what a roll means.
class FSuppliedRoll : public dd::quest::RewardSource
{
public:
    explicit FSuppliedRoll(double InValue) : Value(InValue) {}
    double roll() override { return Value; }

private:
    double Value;
};

FDDQuestStep ToStep(const dd::quest::Step& Core)
{
    FDDQuestStep Out;
    Out.bOk = Core.ok;
    Out.bFatal = Core.fatal;
    Out.bRefused = Core.refused;
    Out.Code = DDText::FromStd(Core.code);
    Out.Detail = DDText::FromStd(Core.detail);
    Out.StateBefore = DDText::FromStd(dd::quest::stateName(Core.before));
    Out.StateAfter = DDText::FromStd(dd::quest::stateName(Core.after));
    Out.bRewardRolled = Core.rewardRolled;
    for (const dd::quest::Line& L : Core.lines)
    {
        FDDQuestLine Line;
        Line.Kind = DDText::FromStd(L.kind);
        Line.Key = DDText::FromStd(L.key);
        Line.Text = DDText::FromStd(L.text);
        Out.Lines.Add(Line);
    }
    return Out;
}
}  // namespace

struct FDDQuestImpl
{
    dd::map::FloorMap Map;
    dd::quest::QuestData Data;
    dd::quest::RewardConfig Reward;
    std::unique_ptr<dd::quest::Engine> Engine;
};

UDDQuestComponent::UDDQuestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    QuestFile = TEXT("figure_ore_lift_quest.json");
    StringsFile = TEXT("figure_ore_lift_strings.csv");
    MapFile = TEXT("floor1_map.json");
}

UDDQuestComponent::~UDDQuestComponent() = default;

void UDDQuestComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bLoadOnBeginPlay)
    {
        FString Error;
        TArray<FString> Problems;
        LoadQuest(Error, Problems);
    }
}

bool UDDQuestComponent::LoadQuest(FString& Error, TArray<FString>& Problems)
{
    Problems.Reset();
    FString QuestText;
    FString CsvText;
    FString MapText;
    if (!DDText::LoadDataFile(QuestFile, QuestText, Error) || !DDText::LoadDataFile(StringsFile, CsvText, Error) ||
        !DDText::LoadDataFile(MapFile, MapText, Error))
    {
        return false;
    }

    TUniquePtr<FDDQuestImpl> NewImpl = MakeUnique<FDDQuestImpl>();
    dd::map::Issue MapIssue;
    if (!dd::map::fromText(DDText::ToStd(MapText), NewImpl->Map, MapIssue))
    {
        Error = DDText::IssueText(MapIssue);
        return false;
    }
    dd::quest::Issue QuestIssue;
    if (!dd::quest::load(DDText::ToStd(QuestText), DDText::ToStd(CsvText), NewImpl->Data, QuestIssue))
    {
        Error = DDText::IssueText(QuestIssue);
        return false;
    }
    for (const dd::quest::Issue& Found : dd::quest::validate(NewImpl->Data, NewImpl->Map))
    {
        Problems.Add(DDText::IssueText(Found));
    }
    if (Problems.Num() > 0)
    {
        Error = Problems[0];
        return false;
    }

    // Map and Data live on the heap inside NewImpl, so the engine's references stay valid after the move.
    NewImpl->Engine = std::make_unique<dd::quest::Engine>(NewImpl->Data, NewImpl->Map);
    Impl = MoveTemp(NewImpl);
    return true;
}

bool UDDQuestComponent::IsLoaded() const
{
    return Impl.IsValid();
}

void UDDQuestComponent::SetRewardConfig(float ItemChance, const TArray<FString>& Items)
{
    if (!Impl)
    {
        return;
    }
    Impl->Reward.itemChance = ItemChance;
    Impl->Reward.items.clear();
    for (const FString& Item : Items)
    {
        Impl->Reward.items.push_back(DDText::ToStd(Item));
    }
    Impl->Engine->setRewardConfig(Impl->Reward);
}

FDDQuestStep UDDQuestComponent::Emit(const FDDQuestStep& Step)
{
    for (const FDDQuestLine& Line : Step.Lines)
    {
        OnLine.Broadcast(Line);
    }
    OnStep.Broadcast(Step);
    return Step;
}

FDDQuestStep UDDQuestComponent::Offer(const FString& Character, int32 Stage, int32 Day)
{
    if (!Impl)
    {
        FDDQuestStep NotLoaded;
        NotLoaded.bFatal = true;
        NotLoaded.Code = TEXT("NOT_LOADED");
        return NotLoaded;
    }
    dd::quest::Context Ctx;
    Ctx.day = Day;
    return Emit(ToStep(Impl->Engine->offer(DDText::ToStd(Character), Stage, Ctx)));
}

FDDQuestStep UDDQuestComponent::Accept(const FString& Character, int32 Stage, int32 Day)
{
    if (!Impl)
    {
        FDDQuestStep NotLoaded;
        NotLoaded.bFatal = true;
        NotLoaded.Code = TEXT("NOT_LOADED");
        return NotLoaded;
    }
    dd::quest::Context Ctx;
    Ctx.day = Day;
    return Emit(ToStep(Impl->Engine->accept(DDText::ToStd(Character), Stage, Ctx)));
}

FDDQuestStep UDDQuestComponent::Decline(const FString& Character, int32 Stage, int32 Day)
{
    if (!Impl)
    {
        FDDQuestStep NotLoaded;
        NotLoaded.bFatal = true;
        NotLoaded.Code = TEXT("NOT_LOADED");
        return NotLoaded;
    }
    dd::quest::Context Ctx;
    Ctx.day = Day;
    return Emit(ToStep(Impl->Engine->decline(DDText::ToStd(Character), Stage, Ctx)));
}

FDDQuestStep UDDQuestComponent::Observe(const FString& Character, const FString& Node, const FString& Chain, int32 Day)
{
    if (!Impl)
    {
        FDDQuestStep NotLoaded;
        NotLoaded.bFatal = true;
        NotLoaded.Code = TEXT("NOT_LOADED");
        return NotLoaded;
    }
    dd::quest::Context Ctx;
    Ctx.day = Day;
    return Emit(ToStep(Impl->Engine->arrive(DDText::ToStd(Character), DDText::ToStd(Node), DDText::ToStd(Chain), Ctx)));
}

FDDQuestStep UDDQuestComponent::Report(const FString& Character, const FString& Node, int32 Stage, int32 Day, float Roll)
{
    if (!Impl)
    {
        FDDQuestStep NotLoaded;
        NotLoaded.bFatal = true;
        NotLoaded.Code = TEXT("NOT_LOADED");
        return NotLoaded;
    }
    dd::quest::Context Ctx;
    Ctx.day = Day;
    FSuppliedRoll Source(static_cast<double>(Roll));
    return Emit(ToStep(Impl->Engine->report(DDText::ToStd(Character), DDText::ToStd(Node), Stage, Ctx, Source)));
}

FDDQuestRecord UDDQuestComponent::GetRecord(const FString& Character) const
{
    FDDQuestRecord Out;
    if (!Impl)
    {
        return Out;
    }
    const dd::quest::Record Core = Impl->Engine->record(DDText::ToStd(Character));
    Out.State = DDText::FromStd(dd::quest::stateName(Core.state));
    Out.bOffered = Core.offered;
    Out.bRefused = Core.refused;
    Out.Chain = DDText::FromStd(Core.chain);
    Out.bRewardRolled = Core.rewardRolled;
    Out.RewardItem = DDText::FromStd(Core.rewardItem);
    return Out;
}

bool UDDQuestComponent::StageFromScore(int32 Score, int32& Stage)
{
    int CoreStage = 0;
    const bool Ok = dd::quest::stageFromScore(Score, CoreStage);
    if (Ok)
    {
        Stage = CoreStage;
    }
    return Ok;
}

FString UDDQuestComponent::SaveStateText() const
{
    return Impl ? DDText::FromStd(Impl->Engine->save()) : FString();
}

bool UDDQuestComponent::RestoreStateText(const FString& Text, FString& Error)
{
    if (!Impl)
    {
        Error = TEXT("NOT_LOADED: call LoadQuest first");
        return false;
    }
    dd::quest::Issue Issue;
    if (!Impl->Engine->restore(DDText::ToStd(Text), Issue))
    {
        Error = DDText::IssueText(Issue);
        return false;
    }
    return true;
}
