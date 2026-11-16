// Hand-written. Reads the floor map through the core and spawns simple placeholder actors
// (engine basic shapes) so the floor can be seen and walked in the editor. No game rules, no content ids.
// PROPOSAL, NOT RUN - Unreal missing.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DDFloorMapActor.generated.h"

UCLASS(BlueprintType)
class DELVEDEEPERCONTENT_API ADDFloorMapActor : public AActor
{
    GENERATED_BODY()

public:
    ADDFloorMapActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    FString MapFile;

    // Unreal units per minimap grid cell.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper", meta = (ClampMin = "100"))
    float CellSize = 600.f;

    // Spawns placeholder actors for nodes, doors (edges), and encounter / trap / loot slots.
    // Each spawned actor carries tags "DDId:<id>", "DDType:<type>" (or "DDDoor:<door>") for Blueprints.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "DelveDeeper")
    void BuildFloor();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "DelveDeeper")
    void ClearFloor();

    // Fired for every problem found (load, parse or validation). Building continues on validation problems.
    UFUNCTION(BlueprintImplementableEvent, Category = "DelveDeeper")
    void OnFloorProblem(const FString& Code, const FString& Detail);

    UFUNCTION(BlueprintImplementableEvent, Category = "DelveDeeper")
    void OnFloorBuilt(int32 NodeCount, int32 EdgeCount);
};
