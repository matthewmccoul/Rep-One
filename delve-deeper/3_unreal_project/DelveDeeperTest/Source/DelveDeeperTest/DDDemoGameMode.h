// Demo-only game mode: first-person character, demo HUD, and the director that sets up the floor.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "DDDemoGameMode.generated.h"

UCLASS()
class ADDDemoGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADDDemoGameMode();

protected:
    virtual void BeginPlay() override;
};
