// Hand-written thin wrapper around dd::quest::Engine. No game rules, no content ids.
// PROPOSAL, NOT RUN - Unreal missing.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "DDTypes.h"

#include "DDQuestComponent.generated.h"

struct FDDQuestImpl;
// LOCAL TEST FIX: out-of-line deleter so UHT-generated code never deletes the incomplete type.
struct FDDQuestImplDeleter { void operator()(FDDQuestImpl* Ptr) const; };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDDQuestLineSignature, const FDDQuestLine&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDDQuestStepSignature, const FDDQuestStep&, Step);

UCLASS(ClassGroup = (DelveDeeper), meta = (BlueprintSpawnableComponent))
class DELVEDEEPERCONTENT_API UDDQuestComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDDQuestComponent();
    virtual ~UDDQuestComponent() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    FString QuestFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    FString StringsFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    FString MapFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    bool bLoadOnBeginPlay = true;

    // What the player should see: each line the core returns, then the whole step.
    UPROPERTY(BlueprintAssignable, Category = "DelveDeeper")
    FDDQuestLineSignature OnLine;

    UPROPERTY(BlueprintAssignable, Category = "DelveDeeper")
    FDDQuestStepSignature OnStep;

    // Loads quest JSON, strings CSV and map, and validates them. Problems lists every issue found.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    bool LoadQuest(FString& Error, TArray<FString>& Problems);

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Quest")
    bool IsLoaded() const;

    // Reward data supplied by the caller (TBD in the content; the core default gives no item).
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    void SetRewardConfig(float ItemChance, const TArray<FString>& Items);

    // Stage = the character's standing (-2..2) with the Figure, Day = the calendar day.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    FDDQuestStep Offer(const FString& Character, int32 Stage, int32 Day);

    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    FDDQuestStep Accept(const FString& Character, int32 Stage, int32 Day);

    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    FDDQuestStep Decline(const FString& Character, int32 Stage, int32 Day);

    // Arrival at a map node. Chain "" means the authored default.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    FDDQuestStep Observe(const FString& Character, const FString& Node, const FString& Chain, int32 Day);

    // Report back. Roll is supplied by the caller (for example Blueprint "Random Float"); the core defines its range.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    FDDQuestStep Report(const FString& Character, const FString& Node, int32 Stage, int32 Day, float Roll);

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Quest")
    FDDQuestRecord GetRecord(const FString& Character) const;

    // Relationship score (0..100) to stage; false when out of range.
    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Quest")
    static bool StageFromScore(int32 Score, int32& Stage);

    // Save and restore as JSON text (for the game's save system).
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    FString SaveStateText() const;

    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Quest")
    bool RestoreStateText(const FString& Text, FString& Error);

protected:
    virtual void BeginPlay() override;

private:
    TUniquePtr<FDDQuestImpl, FDDQuestImplDeleter> Impl;
    FDDQuestStep Emit(const FDDQuestStep& Step);
};
