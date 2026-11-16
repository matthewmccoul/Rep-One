// Hand-written thin wrapper around dd::map (queries and validation). No game rules, no content ids.
// PROPOSAL, NOT RUN - Unreal missing.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "DDTypes.h"

#include "DDMapLibrary.generated.h"

UCLASS()
class DELVEDEEPERCONTENT_API UDDMapLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Reads a data file (plugin Content/Data/) as text.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Map")
    static bool ReadDataFile(const FString& FileName, FString& Text, FString& Error);

    // Parses the floor map and returns its nodes and edges as Blueprint types.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Map")
    static bool LoadFloor(const FString& MapFile, TArray<FDDMapNode>& Nodes, TArray<FDDMapEdge>& Edges, FString& Error);

    // Runs the core map validator. True when it reports no problems; Problems lists every issue.
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Map")
    static bool ValidateFloor(const FString& MapFile, TArray<FString>& Problems);

    // Nodes reachable from Start with optional blocked edges/nodes (ids from the data).
    UFUNCTION(BlueprintCallable, Category = "DelveDeeper|Map")
    static bool FindReachable(const FString& MapFile, const FString& Start, const TArray<FString>& BlockedEdges,
                              const TArray<FString>& BlockedNodes, bool bPlainDoorsOnly, TArray<FString>& Reachable,
                              FString& Error);
};
