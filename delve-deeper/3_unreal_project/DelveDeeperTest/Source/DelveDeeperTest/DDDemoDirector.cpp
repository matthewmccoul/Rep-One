#include "DDDemoDirector.h"

#include "DDDemoInteractable.h"
#include "DDFloorMapActor.h"
#include "DDMapLibrary.h"
#include "DDPuzzleComponent.h"
#include "DDQuestComponent.h"

#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

static const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
static const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

ADDDemoDirector::ADDDemoDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Puzzle = CreateDefaultSubobject<UDDPuzzleComponent>(TEXT("Puzzle"));
    Quest = CreateDefaultSubobject<UDDQuestComponent>(TEXT("Quest"));
    Puzzle->bLoadOnBeginPlay = false;
    Quest->bLoadOnBeginPlay = false;
}

void ADDDemoDirector::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();

    // 1. The floor, built from floor1_map.json by the plugin's floor actor.
    Floor = World->SpawnActor<ADDFloorMapActor>(FVector::ZeroVector, FRotator::ZeroRotator);
    CellSize = Floor->CellSize;
    Floor->BuildFloor();

    TArray<FDDMapNode> Nodes;
    TArray<FDDMapEdge> Edges;
    FString Error;
    if (!UDDMapLibrary::LoadFloor(Floor->MapFile, Nodes, Edges, Error))
    {
        AddMessage(TEXT("Error"), TEXT("Map load failed: ") + Error);
        return;
    }
    for (const FDDMapNode& N : Nodes)
    {
        NodeGrid.Add(N.Id, FVector2D(N.X, N.Y));
        NodeNames.Add(N.Id, N.Name.IsEmpty() ? N.Id : N.Name);
    }

    // 2. The puzzle and quest, loaded by the plugin's components.
    TArray<FString> Problems;
    if (!Puzzle->LoadPuzzle(Error, Problems))
    {
        AddMessage(TEXT("Error"), TEXT("Puzzle load failed: ") + Error + TEXT(" ") + FString::Join(Problems, TEXT("; ")));
    }
    if (!Quest->LoadQuest(Error, Problems))
    {
        AddMessage(TEXT("Error"), TEXT("Quest load failed: ") + Error + TEXT(" ") + FString::Join(Problems, TEXT("; ")));
    }
    Puzzle->OnDoorOpened.AddDynamic(this, &ADDDemoDirector::HandleDoorOpened);

    FString PuzzleText;
    if (UDDMapLibrary::ReadDataFile(Puzzle->PuzzleFile, PuzzleText, Error))
    {
        TSharedPtr<FJsonObject> Root;
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PuzzleText), Root) && Root.IsValid())
        {
            const TSharedPtr<FJsonObject>* Strings = nullptr;
            if (Root->TryGetObjectField(TEXT("strings"), Strings))
            {
                for (const auto& Pair : (*Strings)->Values)
                {
                    PuzzleStrings.Add(FString(Pair.Key), FString(Pair.Value->AsString()));
                }
            }
        }
    }

    // 3. Interactables placed on the floor's places (ids from the map data).
    const FLinearColor Stone(0.55f, 0.5f, 0.42f);
    SpawnInteractable(EDDDemoKind::Carving, TEXT("obj_carving"), TEXT("Carving"),
                      NodeLocation(TEXT("puzzle_clue_site")) + FVector(0.f, -170.f, 70.f), CubePath,
                      FVector(1.2f, 0.15f, 1.2f), Stone);

    const FVector Controls = NodeLocation(TEXT("puzzle_controls_site"));
    const TCHAR* ControlIds[] = {TEXT("ctl_pick"), TEXT("ctl_cart"), TEXT("ctl_furnace")};
    const FLinearColor ControlColors[] = {FLinearColor(0.6f, 0.6f, 0.65f), FLinearColor(0.45f, 0.3f, 0.15f),
                                          FLinearColor(0.9f, 0.35f, 0.05f)};
    for (int32 i = 0; i < 3; ++i)
    {
        const FString ControlId = ControlIds[i];
        FString Name = PuzzleString(TEXT("str.label.") + ControlId.RightChop(4));
        if (Name.IsEmpty())
        {
            Name = ControlId;
        }
        SpawnInteractable(EDDDemoKind::Control, ControlId, Name,
                          Controls + FVector(-150.f + 150.f * i, 160.f, 40.f), CubePath, FVector(0.6f), ControlColors[i]);
    }

    SpawnInteractable(EDDDemoKind::Figure, TEXT("mysterious_figure"), TEXT("Mysterious Figure"),
                      NodeLocation(TEXT("sewer_grate")) + FVector(0.f, -120.f, 100.f), CylinderPath,
                      FVector(0.6f, 0.6f, 1.8f), FLinearColor(0.1f, 0.45f, 0.15f));

    // Storeroom: sealed until the puzzle (or a bypass) opens door e08.
    const FVector Store = NodeLocation(TEXT("storeroom"));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, CubePath))
    {
        AStaticMeshActor* Seal = World->SpawnActor<AStaticMeshActor>(Store + FVector(0.f, 0.f, 160.f), FRotator::ZeroRotator);
        Seal->SetMobility(EComponentMobility::Movable);
        Seal->GetStaticMeshComponent()->SetStaticMesh(Cube);
        Seal->SetActorScale3D(FVector(5.f, 5.f, 3.f));
        if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
                nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
        {
            if (UMaterialInstanceDynamic* Mat = Seal->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, Base))
            {
                Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.35f, 0.05f, 0.05f));
            }
        }
        StoreroomSeal = Seal;
    }
    const FVector Smelter = NodeLocation(TEXT("smelter_hall"));
    const FVector DoorPos = Store + (Smelter - Store) * 0.48f + FVector(0.f, 0.f, 110.f);
    StoreroomDoor = SpawnInteractable(EDDDemoKind::StoreroomDoor, TEXT("e08"), TEXT("Storeroom door"), DoorPos,
                                      CubePath, FVector(0.15f, 1.2f, 2.0f), FLinearColor(0.3f, 0.18f, 0.08f));

    // Ore lift dressing (visual only): four corner pillars and the drum the chain hangs from.
    auto SpawnDecor = [this](const TCHAR* MeshPath, const FVector& Location, const FVector& Scale,
                             const FLinearColor& Color) {
        UStaticMesh* DecorMesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
        AStaticMeshActor* Piece = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
        if (!DecorMesh || !Piece)
        {
            return;
        }
        Piece->SetMobility(EComponentMobility::Movable);
        Piece->GetStaticMeshComponent()->SetStaticMesh(DecorMesh);
        Piece->SetActorScale3D(Scale);
        if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
                nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
        {
            if (UMaterialInstanceDynamic* Mat = Piece->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0, Base))
            {
                Mat->SetVectorParameterValue(TEXT("Color"), Color);
            }
        }
    };
    const FVector Lift = NodeLocation(TEXT("ore_lift"));
    const float Corner = CellSize * 0.8f * 0.5f - 25.f;
    for (const FVector2D& C : {FVector2D(1, 1), FVector2D(1, -1), FVector2D(-1, 1), FVector2D(-1, -1)})
    {
        SpawnDecor(CylinderPath, Lift + FVector(C.X * Corner, C.Y * Corner, 160.f), FVector(0.25f, 0.25f, 3.f),
                   FLinearColor(0.45f, 0.45f, 0.47f));
    }
    SpawnDecor(CylinderPath, Lift + FVector(0.f, 0.f, 45.f), FVector(1.4f, 1.4f, 0.7f), FLinearColor(0.45f, 0.2f, 0.07f));

    AddMessage(TEXT("Demo"), TEXT("Floor 1, the storeroom puzzle and the Mysterious Figure quest, running through the DelveDeeperContent plugin."));
}

ADDDemoInteractable* ADDDemoDirector::SpawnInteractable(EDDDemoKind Kind, const FString& Id, const FString& Name,
                                                         const FVector& Location, const TCHAR* MeshPath,
                                                         const FVector& Scale, const FLinearColor& Color)
{
    ADDDemoInteractable* Item = GetWorld()->SpawnActor<ADDDemoInteractable>(Location, FRotator::ZeroRotator);
    if (Item)
    {
        Item->Setup(this, Kind, Id, Name, LoadObject<UStaticMesh>(nullptr, MeshPath), Scale, Color);
    }
    return Item;
}

FVector ADDDemoDirector::NodeLocation(const FString& NodeId) const
{
    const FVector Origin = Floor ? Floor->GetActorLocation() : FVector::ZeroVector;
    if (const FVector2D* Cell = NodeGrid.Find(NodeId))
    {
        return Origin + FVector(Cell->X * CellSize, Cell->Y * CellSize, 0.f);
    }
    return Origin;
}

FString ADDDemoDirector::PuzzleString(const FString& Key) const
{
    const FString* Found = PuzzleStrings.Find(Key);
    return Found ? *Found : FString();
}

void ADDDemoDirector::AddMessage(const FString& Speaker, const FString& Text)
{
    if (Text.IsEmpty())
    {
        return;
    }
    Messages.Add({Speaker, Text});
    while (Messages.Num() > 7)
    {
        Messages.RemoveAt(0);
    }
    UE_LOG(LogTemp, Display, TEXT("[Demo] %s: %s"), *Speaker, *Text);
}

void ADDDemoDirector::AddQuestStep(const FDDQuestStep& Step)
{
    for (const FDDQuestLine& Line : Step.Lines)
    {
        FString Speaker = Line.Kind;
        if (Line.Kind == TEXT("figure"))
        {
            Speaker = TEXT("Figure");
        }
        else if (Line.Kind == TEXT("log"))
        {
            Speaker = TEXT("Log");
        }
        else if (Line.Kind == TEXT("narration"))
        {
            Speaker = TEXT("");
        }
        AddMessage(Speaker, Line.Text);
    }
    if (Step.Lines.Num() == 0 && (!Step.bOk || Step.bRefused))
    {
        AddMessage(TEXT("Quest"), FString::Printf(TEXT("(%s %s)"), *Step.Code, *Step.Detail));
    }
    ++Day;
}

FString ADDDemoDirector::PromptFor(const ADDDemoInteractable* Target) const
{
    switch (Target->Kind)
    {
    case EDDDemoKind::Carving:
        return TEXT("E  Read the carving");
    case EDDDemoKind::Control:
        return TEXT("E  Use ") + Target->DisplayName;
    case EDDDemoKind::StoreroomDoor:
        return Puzzle->IsDoorOpen() ? FString() : TEXT("Storeroom door (locked)      Q  Pick the lock (bypass)");
    case EDDDemoKind::Figure:
    {
        const FString State = Quest->GetRecord(Hero).State;
        if ((State == TEXT("not_offered") && bOfferHeard) || State == TEXT("declined"))
        {
            return TEXT("E  Accept the task      Q  Decline");
        }
        if (State.StartsWith(TEXT("observed")))
        {
            return TEXT("E  Report what you saw");
        }
        return TEXT("E  Talk to the figure");
    }
    }
    return FString();
}

void ADDDemoDirector::HandleInteract(ADDDemoInteractable* Target, bool bAlt)
{
    switch (Target->Kind)
    {
    case EDDDemoKind::Carving:
        if (!bAlt)
        {
            AddMessage(TEXT("Carving"), PuzzleString(TEXT("str.carving.text")));
        }
        break;

    case EDDDemoKind::Control:
        if (!bAlt)
        {
            const FDDUseOutcome Outcome = Puzzle->UseControl(Target->Id);
            AddMessage(Target->DisplayName, Outcome.Text.IsEmpty() ? Outcome.ResultKey : Outcome.Text);
            if (Outcome.Result == EDDUseResult::Solved)
            {
                const FString Xp = Puzzle->ClaimXp();
                if (!Xp.IsEmpty())
                {
                    AddMessage(TEXT("XP"), FString::Printf(TEXT("%s awarded once (amount TBD in the GDD)"), *Xp));
                }
            }
        }
        break;

    case EDDDemoKind::StoreroomDoor:
        if (bAlt && !Puzzle->IsDoorOpen())
        {
            Puzzle->OpenOther(TEXT("pick_lock"));
        }
        break;

    case EDDDemoKind::Figure:
    {
        const FString State = Quest->GetRecord(Hero).State;
        if (State == TEXT("not_offered") && !bOfferHeard)
        {
            AddQuestStep(Quest->Offer(Hero, 0, Day));
            bOfferHeard = true;
        }
        else if (State == TEXT("not_offered") || State == TEXT("declined"))
        {
            AddQuestStep(bAlt ? Quest->Decline(Hero, 0, Day) : Quest->Accept(Hero, 0, Day));
        }
        else if (!bAlt)
        {
            AddQuestStep(Quest->Report(Hero, TEXT("sewer_grate"), 0, Day, FMath::FRand() * 0.999f));
        }
        break;
    }
    }
}

void ADDDemoDirector::HandleDoorOpened(const FString& OpenedVia)
{
    if (StoreroomSeal)
    {
        StoreroomSeal->Destroy();
        StoreroomSeal = nullptr;
    }
    if (StoreroomDoor)
    {
        StoreroomDoor->Destroy();
        StoreroomDoor = nullptr;
    }
    if (OpenedVia != TEXT("puzzle"))
    {
        AddMessage(TEXT("Log"), PuzzleString(TEXT("str.log.door_other")));
    }
    AddMessage(TEXT("Demo"), FString::Printf(TEXT("Storeroom door e08 opened (via %s)."), *OpenedVia));
}

TArray<FString> ADDDemoDirector::GetStatusLines() const
{
    TArray<FString> Lines;
    FString Door = Puzzle->IsDoorOpen() ? FString::Printf(TEXT("open (via %s)"), *Puzzle->GetOpenedVia()) : TEXT("sealed");
    Lines.Add(FString::Printf(TEXT("Storeroom puzzle: %s   door: %s   progress: %s"), *Puzzle->GetStateName(), *Door,
                              *FString::Join(Puzzle->GetProgress(), TEXT(" > "))));
    const FDDQuestRecord Rec = Quest->GetRecord(Hero);
    FString QuestLine = TEXT("Quest (Mysterious Figure): ") + Rec.State;
    if (!Rec.Chain.IsEmpty())
    {
        QuestLine += TEXT("   chain: ") + Rec.Chain;
    }
    Lines.Add(QuestLine);
    // Prefer the room's name over a site inside it.
    FString Where = TEXT("-");
    for (const FString& Id : OccupiedNodes)
    {
        const FString* Name = NodeNames.Find(Id);
        Where = Name ? *Name : Id;
        if (!Id.Contains(TEXT("site")))
        {
            break;
        }
    }
    Lines.Add(TEXT("Location: ") + Where);
    return Lines;
}

void ADDDemoDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn || NodeGrid.Num() == 0)
    {
        return;
    }

    if (!bPlayerPlaced)
    {
        const FVector Start = NodeLocation(TEXT("town_entrance")) + FVector(0.f, 0.f, 110.f);
        Pawn->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
        PC->SetControlRotation(FRotator(-10.f, 90.f, 0.f));
        SafeLocation = Start;
        bPlayerPlaced = true;
        return;
    }

    const FVector P = Pawn->GetActorLocation();
    if (P.Z < -800.f)
    {
        Pawn->SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
        return;
    }

    // Arrival at map places: the quest core decides what (if anything) an arrival means.
    TSet<FString> Now;
    const float Radius = CellSize * 0.42f;
    for (const auto& Pair : NodeGrid)
    {
        const FVector L = NodeLocation(Pair.Key);
        if (FVector::Dist2D(L, P) < Radius && FMath::Abs(P.Z - L.Z) < 300.f)
        {
            Now.Add(Pair.Key);
        }
    }
    if (Now.Num() > 0)
    {
        SafeLocation = P + FVector(0.f, 0.f, 50.f);
    }
    for (const FString& Id : Now)
    {
        if (!OccupiedNodes.Contains(Id))
        {
            const FDDQuestStep Step = Quest->Observe(Hero, Id, TEXT(""), Day);
            if (Step.Lines.Num() > 0)
            {
                AddQuestStep(Step);
            }
        }
    }
    OccupiedNodes = MoveTemp(Now);
}
