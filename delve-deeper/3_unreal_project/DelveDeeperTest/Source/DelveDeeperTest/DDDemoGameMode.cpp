#include "DDDemoGameMode.h"

#include "DDDemoCharacter.h"
#include "DDDemoDirector.h"
#include "DDDemoHUD.h"

#include "EngineUtils.h"

ADDDemoGameMode::ADDDemoGameMode()
{
    DefaultPawnClass = ADDDemoCharacter::StaticClass();
    HUDClass = ADDDemoHUD::StaticClass();
}

void ADDDemoGameMode::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<ADDDemoDirector> It(GetWorld()); It; ++It)
    {
        return;  // already placed in the level
    }
    GetWorld()->SpawnActor<ADDDemoDirector>(FVector::ZeroVector, FRotator::ZeroRotator);
}
