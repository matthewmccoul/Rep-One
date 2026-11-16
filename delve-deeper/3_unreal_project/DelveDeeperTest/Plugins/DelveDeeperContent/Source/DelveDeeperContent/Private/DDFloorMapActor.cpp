// Hand-written. PROPOSAL, NOT RUN - Unreal missing.
#include "DDFloorMapActor.h"

#include "DDText.h"

#include "dd/map.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogDDFloor, Log, All);

namespace
{
// Colour from the text of a type or door name, so no name is listed in code.
FLinearColor ColorFromText(const FString& Text)
{
    const uint32 Hash = GetTypeHash(Text);
    return FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash & 0xFF), 170, 230);
}

AStaticMeshActor* SpawnPiece(UWorld* World, AActor* Owner, UStaticMesh* Mesh, const FVector& Location,
                             const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color,
                             const FString& Label, const TArray<FName>& Tags)
{
    FActorSpawnParameters Params;
    Params.Owner = Owner;
    AStaticMeshActor* Piece = World->SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
    if (!Piece)
    {
        return nullptr;
    }
    Piece->SetMobility(EComponentMobility::Movable);
    UStaticMeshComponent* Component = Piece->GetStaticMeshComponent();
    Component->SetStaticMesh(Mesh);
    Piece->SetActorScale3D(Scale);
    UMaterialInterface* Base =
        LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Base)
    {
        UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(0, Base);
        if (Material)
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
    Piece->Tags = Tags;
    Piece->AttachToActor(Owner, FAttachmentTransformRules::KeepWorldTransform);
#if WITH_EDITOR
    Piece->SetActorLabel(Label);
#endif
    return Piece;
}
}  // namespace

ADDFloorMapActor::ADDFloorMapActor()
{
    PrimaryActorTick.bCanEverTick = false;
    MapFile = TEXT("floor1_map.json");
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void ADDFloorMapActor::ClearFloor()
{
    TArray<AActor*> Attached;
    GetAttachedActors(Attached);
    for (AActor* Piece : Attached)
    {
        if (Piece)
        {
            Piece->Destroy();
        }
    }
}

void ADDFloorMapActor::BuildFloor()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FString Text;
    FString Error;
    if (!DDText::LoadDataFile(MapFile, Text, Error))
    {
        ReportProblem(TEXT("LOAD"), Error);
        return;
    }
    dd::map::FloorMap Map;
    dd::map::Issue Issue;
    if (!dd::map::fromText(DDText::ToStd(Text), Map, Issue))
    {
        ReportProblem(DDText::FromStd(Issue.code), DDText::FromStd(Issue.detail));
        return;
    }
    for (const dd::map::Issue& Found : dd::map::validate(Map))
    {
        ReportProblem(DDText::FromStd(Found.code), DDText::FromStd(Found.detail));
    }

    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!Cube || !Sphere)
    {
        ReportProblem(TEXT("MESH"), TEXT("engine basic shape meshes not found"));
        return;
    }

    ClearFloor();
    const FVector Origin = GetActorLocation();
    const float SlabScale = (CellSize * 0.8f) / 100.f;
    int32 NodeCount = 0;
    int32 EdgeCount = 0;

    auto NodeLocation = [&](const dd::map::Node& N) {
        return Origin + FVector(N.x * CellSize, N.y * CellSize, 0.f);
    };

    for (const dd::map::Node& N : Map.nodes)
    {
        const FString Id = DDText::FromStd(N.id);
        const FString Type = DDText::FromStd(N.type);
        const FLinearColor Color = ColorFromText(Type);
        TArray<FName> PieceTags;
        PieceTags.Add(FName(*FString::Printf(TEXT("DDId:%s"), *Id)));
        PieceTags.Add(FName(*FString::Printf(TEXT("DDType:%s"), *Type)));
        const FString Label = FString::Printf(TEXT("%s (%s)"), *Id, *Type);

        if (N.insideRoom.empty())
        {
            // A place of its own: a flat walkable slab.
            SpawnPiece(World, this, Cube, NodeLocation(N), FRotator::ZeroRotator, FVector(SlabScale, SlabScale, 0.2f),
                       Color, Label, PieceTags);
        }
        else
        {
            // A site inside a room: a marker sphere above the room slab.
            SpawnPiece(World, this, Sphere, NodeLocation(N) + FVector(0.f, 0.f, 120.f), FRotator::ZeroRotator,
                       FVector(0.8f), Color, Label, PieceTags);
        }
        ++NodeCount;

        // Slots (encounters, traps, loot) as small markers in a row above the node.
        float Offset = -100.f;
        auto AddSlot = [&](const std::string& Slot, const TCHAR* Kind) {
            TArray<FName> SlotTags;
            const FString SlotId = DDText::FromStd(Slot);
            SlotTags.Add(FName(*FString::Printf(TEXT("DDId:%s"), *SlotId)));
            SlotTags.Add(FName(*FString::Printf(TEXT("DDSlot:%s"), Kind)));
            SpawnPiece(World, this, Sphere, NodeLocation(N) + FVector(Offset, 0.f, 260.f), FRotator::ZeroRotator,
                       FVector(0.4f), ColorFromText(FString(Kind)), SlotId, SlotTags);
            Offset += 90.f;
        };
        for (const dd::map::Encounter& E : N.encounters)
        {
            AddSlot(E.slot, TEXT("encounter"));
        }
        for (const dd::map::Trap& T : N.traps)
        {
            AddSlot(T.slot, TEXT("trap"));
        }
        for (const std::string& L : N.lootSlots)
        {
            AddSlot(L, TEXT("loot"));
        }
    }

    for (const dd::map::Edge& E : Map.edges)
    {
        const dd::map::Node* From = Map.node(E.from);
        const dd::map::Node* To = Map.node(E.to);
        if (!From || !To)
        {
            ReportProblem(TEXT("EDGE_NODE"), DDText::FromStd(E.id));
            continue;
        }
        const FVector A = NodeLocation(*From);
        const FVector B = NodeLocation(*To);
        const FVector Delta = B - A;
        const float Length = Delta.Size();
        if (Length < 1.f)
        {
            continue;
        }
        const FString Id = DDText::FromStd(E.id);
        const FString Door = DDText::FromStd(E.door);
        TArray<FName> PieceTags;
        PieceTags.Add(FName(*FString::Printf(TEXT("DDId:%s"), *Id)));
        PieceTags.Add(FName(*FString::Printf(TEXT("DDDoor:%s"), *Door)));
        SpawnPiece(World, this, Cube, (A + B) * 0.5f + FVector(0.f, 0.f, 15.f), Delta.Rotation(),
                   FVector(Length / 100.f, 1.2f, 0.2f), ColorFromText(Door),
                   FString::Printf(TEXT("%s [%s]"), *Id, *Door), PieceTags);
        ++EdgeCount;
    }

    TArray<AActor*> Spawned;
    GetAttachedActors(Spawned);
    ReportMessage(FString::Printf(TEXT("Build Floor: %d places, %d doors, %d pieces spawned around %s"), NodeCount,
                                  EdgeCount, Spawned.Num(), *GetActorLocation().ToCompactString()),
                  Spawned.Num() == 0);
#if WITH_EDITOR
    // Bring the new floor into view in the editor viewports.
    if (GEditor && !World->IsGameWorld() && Spawned.Num() > 0)
    {
        FBox Bounds(ForceInit);
        for (AActor* Piece : Spawned)
        {
            Bounds += Piece->GetComponentsBoundingBox();
        }
        GEditor->MoveViewportCamerasToBox(Bounds, true);
    }
#endif
    OnFloorBuilt(NodeCount, EdgeCount);
}

void ADDFloorMapActor::ReportProblem(const FString& Code, const FString& Detail)
{
    ReportMessage(FString::Printf(TEXT("Build Floor problem %s: %s"), *Code, *Detail), true);
    OnFloorProblem(Code, Detail);
}

void ADDFloorMapActor::ReportMessage(const FString& Message, bool bProblem)
{
    if (bProblem)
    {
        UE_LOG(LogDDFloor, Warning, TEXT("%s"), *Message);
    }
    else
    {
        UE_LOG(LogDDFloor, Display, TEXT("%s"), *Message);
    }
#if WITH_EDITOR
    if (GEditor && FSlateApplication::IsInitialized())
    {
        FNotificationInfo Info(FText::FromString(Message));
        Info.ExpireDuration = bProblem ? 10.f : 5.f;
        if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
        {
            Item->SetCompletionState(bProblem ? SNotificationItem::CS_Fail : SNotificationItem::CS_Success);
        }
    }
#endif
}

