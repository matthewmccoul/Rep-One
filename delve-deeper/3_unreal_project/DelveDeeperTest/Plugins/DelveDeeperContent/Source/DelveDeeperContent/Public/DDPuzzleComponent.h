// Hand-written thin wrapper around dd::puzzle::Puzzle. No game rules, no content ids.
// PROPOSAL, NOT RUN - Unreal missing.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "DDTypes.h"

#include "DDPuzzleComponent.generated.h"

struct FDDPuzzleImpl;
// LOCAL TEST FIX: out-of-line deleter so UHT-generated code never deletes the incomplete type.
struct FDDPuzzleImplDeleter { void operator()(FDDPuzzleImpl* Ptr) const; };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDDPuzzleUsedSignature, const FDDUseOutcome&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDDPuzzleDoorOpenedSignature, const FString&, OpenedVia);

UCLASS(ClassGroup = (DelveDeeper), meta = (BlueprintSpawnableComponent))
class DELVEDEEPERCONTENT_API UDDPuzzleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDDPuzzleComponent();
    virtual ~UDDPuzzleComponent() override;

    // Data file names under the plugin's Content/Data/.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    FString PuzzleFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    FString MapFile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DelveDeeper")
    bool bLoadOnBeginPlay = true;

    // What the player should see: every Use, and the moment the door opens.
    UPROPERTY(BlueprintAssignable, Category = "DelveDeeper")
    FDDPuzzleUsedSignature OnUsed;

    UPROPERTY(BlueprintAssignable, Category = "DelveDeeper")
    FDDPuzzleDoorOpenedSignature OnDoorOpened;

    // Loads puzzle and map data, checks the puzzle against the map. Problems lists every issue found.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    bool LoadPuzzle(FString& Error, TArray<FString>& Problems);

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Puzzle")
    bool IsLoaded() const;

    // Use on a control (control id comes from the map data, supplied by the caller).
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    FDDUseOutcome UseControl(const FString& Control);

    // Opens the door by a bypass route (route name from the puzzle data).
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    EDDBypassResult OpenOther(const FString& Route);

    // Returns the XP class key once after a puzzle solve, "" otherwise.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    FString ClaimXp();

    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    void ResetProgress();

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Puzzle")
    FString GetStateName() const;

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Puzzle")
    FString GetSummary() const;

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Puzzle")
    TArray<FString> GetProgress() const;

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Puzzle")
    bool IsDoorOpen() const;

    UFUNCTION(BlueprintPure, Category = "DelveDeeper|Puzzle")
    FString GetOpenedVia() const;

    // Save and load the puzzle state as JSON text (for the game's save system).
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    FString SaveStateText() const;

    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Puzzle")
    bool LoadStateText(const FString& Text, FString& Error);

protected:
    virtual void BeginPlay() override;

private:
    TUniquePtr<FDDPuzzleImpl, FDDPuzzleImplDeleter> Impl;
};
