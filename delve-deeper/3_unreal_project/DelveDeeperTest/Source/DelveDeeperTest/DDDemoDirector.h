// Demo-only: builds the floor at Play, places the puzzle and quest pieces on it, and routes player
// interactions to the real plugin components (UDDPuzzleComponent, UDDQuestComponent). Holds no game rules.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DDDemoInteractable.h"
#include "DDTypes.h"

#include "DDDemoDirector.generated.h"

class ADDDemoInteractable;
class ADDFloorMapActor;
class UDDPuzzleComponent;
class UDDQuestComponent;

struct FDDDemoMessage
{
    FString Speaker;
    FString Text;
};

UCLASS()
class ADDDemoDirector : public AActor
{
    GENERATED_BODY()

public:
    ADDDemoDirector();

    virtual void Tick(float DeltaSeconds) override;

    FString PromptFor(const ADDDemoInteractable* Target) const;
    void HandleInteract(ADDDemoInteractable* Target, bool bAlt);

    const TArray<FDDDemoMessage>& GetMessages() const { return Messages; }
    TArray<FString> GetStatusLines() const;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UDDPuzzleComponent> Puzzle;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UDDQuestComponent> Quest;

    UPROPERTY()
    TObjectPtr<ADDFloorMapActor> Floor;

    UPROPERTY()
    TObjectPtr<AActor> StoreroomSeal;

    UPROPERTY()
    TObjectPtr<ADDDemoInteractable> StoreroomDoor;

    UFUNCTION()
    void HandleDoorOpened(const FString& OpenedVia);

    void AddMessage(const FString& Speaker, const FString& Text);
    void AddQuestStep(const FDDQuestStep& Step);
    FVector NodeLocation(const FString& NodeId) const;
    FString PuzzleString(const FString& Key) const;
    ADDDemoInteractable* SpawnInteractable(EDDDemoKind Kind, const FString& Id, const FString& Name,
                                           const FVector& Location, const TCHAR* MeshPath, const FVector& Scale,
                                           const FLinearColor& Color);

    TArray<FDDDemoMessage> Messages;
    TMap<FString, FVector2D> NodeGrid;
    TMap<FString, FString> NodeNames;
    TMap<FString, FString> PuzzleStrings;
    TSet<FString> OccupiedNodes;
    float CellSize = 600.f;
    int32 Day = 1;
    bool bPlayerPlaced = false;
    bool bOfferHeard = false;
    FVector SafeLocation = FVector::ZeroVector;

    static constexpr const TCHAR* Hero = TEXT("hero");
};
