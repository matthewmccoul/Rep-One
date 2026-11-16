// Hand-written thin wrapper. All behaviour comes from dd::puzzle (the core).
// PROPOSAL, NOT RUN - Unreal missing.
#include "DDPuzzleComponent.h"

#include "DDText.h"

#include "dd/map.h"
#include "dd/puzzle.h"

struct FDDPuzzleImpl
{
    dd::puzzle::Puzzle Puzzle;
};

UDDPuzzleComponent::UDDPuzzleComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PuzzleFile = TEXT("storeroom_puzzle.json");
    MapFile = TEXT("floor1_map.json");
}

void FDDPuzzleImplDeleter::operator()(FDDPuzzleImpl* Ptr) const { delete Ptr; }

UDDPuzzleComponent::~UDDPuzzleComponent() = default;

void UDDPuzzleComponent::BeginPlay()
{
    Super::BeginPlay();
    if (bLoadOnBeginPlay)
    {
        FString Error;
        TArray<FString> Problems;
        LoadPuzzle(Error, Problems);
    }
}

bool UDDPuzzleComponent::LoadPuzzle(FString& Error, TArray<FString>& Problems)
{
    Problems.Reset();
    FString PuzzleText;
    FString MapText;
    if (!DDText::LoadDataFile(PuzzleFile, PuzzleText, Error) || !DDText::LoadDataFile(MapFile, MapText, Error))
    {
        return false;
    }

    dd::map::FloorMap Map;
    dd::map::Issue MapIssue;
    if (!dd::map::fromText(DDText::ToStd(MapText), Map, MapIssue))
    {
        Error = DDText::IssueText(MapIssue);
        return false;
    }

    TUniquePtr<FDDPuzzleImpl> NewImpl = MakeUnique<FDDPuzzleImpl>();
    dd::puzzle::Issue PuzzleIssue;
    if (!dd::puzzle::Puzzle::fromText(DDText::ToStd(PuzzleText), NewImpl->Puzzle, PuzzleIssue))
    {
        Error = DDText::IssueText(PuzzleIssue);
        return false;
    }

    for (const dd::puzzle::Issue& Found : dd::puzzle::validateAgainstMap(NewImpl->Puzzle.data(), Map))
    {
        Problems.Add(DDText::IssueText(Found));
    }
    if (Problems.Num() > 0)
    {
        Error = Problems[0];
        return false;
    }

    Impl.Reset(NewImpl.Release());
    return true;
}

bool UDDPuzzleComponent::IsLoaded() const
{
    return Impl.IsValid();
}

FDDUseOutcome UDDPuzzleComponent::UseControl(const FString& Control)
{
    FDDUseOutcome Result;
    if (!Impl)
    {
        return Result;
    }
    const bool WasOpen = Impl->Puzzle.doorOpen();
    const dd::puzzle::UseOutcome Core = Impl->Puzzle.use(DDText::ToStd(Control));

    switch (Core.result)
    {
    case dd::puzzle::UseResult::StepOk: Result.Result = EDDUseResult::StepOk; break;
    case dd::puzzle::UseResult::WrongReset: Result.Result = EDDUseResult::WrongReset; break;
    case dd::puzzle::UseResult::Solved: Result.Result = EDDUseResult::Solved; break;
    case dd::puzzle::UseResult::Inert: Result.Result = EDDUseResult::Inert; break;
    case dd::puzzle::UseResult::UnknownControl: Result.Result = EDDUseResult::UnknownControl; break;
    }
    Result.ResultKey = DDText::FromStd(Core.resultKey);
    Result.StringKey = DDText::FromStd(Core.stringKey);
    if (const std::string* Text = Impl->Puzzle.data().text(Core.stringKey))
    {
        Result.Text = DDText::FromStd(*Text);
    }

    OnUsed.Broadcast(Result);
    if (!WasOpen && Impl->Puzzle.doorOpen())
    {
        OnDoorOpened.Broadcast(DDText::FromStd(Impl->Puzzle.openedVia()));
    }
    return Result;
}

EDDBypassResult UDDPuzzleComponent::OpenOther(const FString& Route)
{
    if (!Impl)
    {
        return EDDBypassResult::UnknownRoute;
    }
    const bool WasOpen = Impl->Puzzle.doorOpen();
    const dd::puzzle::BypassResult Core = Impl->Puzzle.openOther(DDText::ToStd(Route));
    if (!WasOpen && Impl->Puzzle.doorOpen())
    {
        OnDoorOpened.Broadcast(DDText::FromStd(Impl->Puzzle.openedVia()));
    }
    switch (Core)
    {
    case dd::puzzle::BypassResult::Applied: return EDDBypassResult::Applied;
    case dd::puzzle::BypassResult::NotApplied: return EDDBypassResult::NotApplied;
    case dd::puzzle::BypassResult::UnknownRoute: break;
    }
    return EDDBypassResult::UnknownRoute;
}

FString UDDPuzzleComponent::ClaimXp()
{
    return Impl ? DDText::FromStd(Impl->Puzzle.claimXp()) : FString();
}

void UDDPuzzleComponent::ResetProgress()
{
    if (Impl)
    {
        Impl->Puzzle.reset();
    }
}

FString UDDPuzzleComponent::GetStateName() const
{
    return Impl ? DDText::FromStd(dd::puzzle::Puzzle::stateName(Impl->Puzzle.state())) : FString();
}

FString UDDPuzzleComponent::GetSummary() const
{
    return Impl ? DDText::FromStd(Impl->Puzzle.summary()) : FString();
}

TArray<FString> UDDPuzzleComponent::GetProgress() const
{
    return Impl ? DDText::FromStdList(Impl->Puzzle.progress()) : TArray<FString>();
}

bool UDDPuzzleComponent::IsDoorOpen() const
{
    return Impl && Impl->Puzzle.doorOpen();
}

FString UDDPuzzleComponent::GetOpenedVia() const
{
    return Impl ? DDText::FromStd(Impl->Puzzle.openedVia()) : FString();
}

FString UDDPuzzleComponent::SaveStateText() const
{
    return Impl ? DDText::FromStd(Impl->Puzzle.toJson()) : FString();
}

bool UDDPuzzleComponent::LoadStateText(const FString& Text, FString& Error)
{
    if (!Impl)
    {
        Error = TEXT("NOT_LOADED: call LoadPuzzle first");
        return false;
    }
    dd::puzzle::Issue Issue;
    if (!Impl->Puzzle.loadState(DDText::ToStd(Text), Issue))
    {
        Error = DDText::IssueText(Issue);
        return false;
    }
    return true;
}
