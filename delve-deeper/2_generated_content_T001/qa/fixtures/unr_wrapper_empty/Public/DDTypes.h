// Hand-written. Blueprint-visible value types that mirror core results. No game rules.
// PROPOSAL, NOT RUN - Unreal missing.
#pragma once

#include "CoreMinimal.h"

#include "DDTypes.generated.h"

UENUM(BlueprintType)
enum class EDDUseResult : uint8
{
    StepOk,
    WrongReset,
    Solved,
    Inert,
    UnknownControl
};

UENUM(BlueprintType)
enum class EDDBypassResult : uint8
{
    Applied,
    NotApplied,
    UnknownRoute
};

USTRUCT(BlueprintType)
struct FDDUseOutcome
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    EDDUseResult Result = EDDUseResult::UnknownControl;

    // Result and string keys exactly as the core returns them ("" when unknown).
    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString ResultKey;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString StringKey;

    // The puzzle's text for StringKey, looked up in the loaded puzzle data ("" when absent).
    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Text;
};

USTRUCT(BlueprintType)
struct FDDQuestLine
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Kind;   // as the core labels it: who or what shows the line

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Key;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Text;   // blanks already filled by the core
};

USTRUCT(BlueprintType)
struct FDDQuestStep
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bOk = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bFatal = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bRefused = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Code;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Detail;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString StateBefore;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString StateAfter;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    TArray<FDDQuestLine> Lines;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bRewardRolled = false;
};

USTRUCT(BlueprintType)
struct FDDQuestRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString State;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bOffered = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bRefused = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Chain;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bRewardRolled = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString RewardItem;   // "" = no item
};

USTRUCT(BlueprintType)
struct FDDMapNode
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Type;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Area;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString InsideRoom;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    int32 X = 0;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    int32 Y = 0;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    bool bHidden = false;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    TArray<FString> EncounterSlots;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    TArray<FString> TrapSlots;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    TArray<FString> LootSlots;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    TArray<FString> Controls;
};

USTRUCT(BlueprintType)
struct FDDMapEdge
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Id;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString From;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString To;

    UPROPERTY(BlueprintReadOnly, Category = "DelveDeeper")
    FString Door;
};
