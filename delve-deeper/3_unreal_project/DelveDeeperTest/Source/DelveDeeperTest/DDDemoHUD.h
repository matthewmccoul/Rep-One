// Demo-only HUD: status (top left), interaction prompt (centre), recent lines (bottom left).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "DDDemoHUD.generated.h"

UCLASS()
class ADDDemoHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
