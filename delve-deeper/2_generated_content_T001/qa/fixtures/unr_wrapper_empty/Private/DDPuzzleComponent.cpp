// Hand-written thin wrapper. All behaviour comes from dd::puzzle (the core).
// PROPOSAL, NOT RUN - Unreal missing.
#include "DDPuzzleComponent.h"
UDDPuzzleComponent::UDDPuzzleComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    PuzzleFile = TEXT("storeroom_puzzle.json");
    MapFile = TEXT("floor1_map.json");
}

UDDPuzzleComponent::~UDDPuzzleComponent() = default;
