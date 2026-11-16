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
    // True once the player has gone down the open descent (GDD Win, lines 44-45); the HUD shows the win banner.
    bool IsDemoOver() const { return bDemoOver; }

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
    // Crew encounter data (DD-TOLL-01): strings from bandit_strings.csv, toll from faction_stages.json.
    void LoadBanditData();
    FString BossLine(const FString& StringId) const;
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

    // Demo gold (GDD "Currency: Gold", line 319). Balance starts at 0.
    UPROPERTY()
    TObjectPtr<ADDDemoInteractable> GoldPurse;
    int32 Gold = 0;
    bool bPurseTaken = false;
    // Demo value: 100 is the crew's EXPERIMENTAL toll value (faction_stages.json), not approved design.
    static constexpr int32 PurseGold = 100;

    // Bandit boss (Boss Encounter, GDD lines 385-387). Starts at the threatening stage.
    UPROPERTY()
    TObjectPtr<ADDDemoInteractable> Boss;
    TMap<FString, FString> BanditStrings;
    // Toll for the threatening stage, read from faction_stages.json. Demo value 100 (crew EXPERIMENTAL, not approved).
    int32 TollGold = 0;
    // True once the toll is paid for this run (crew rule TOLL-03). Read by the descent in DD-DESC-01.
    bool bTollPaid = false;

    // Descent at node "descent" (GDD Win lines 44-45, Winning line 390). Using it with the toll paid ends the demo.
    UPROPERTY()
    TObjectPtr<ADDDemoInteractable> Descent;
    bool bDemoOver = false;

    static constexpr const TCHAR* Hero = TEXT("hero");
};
