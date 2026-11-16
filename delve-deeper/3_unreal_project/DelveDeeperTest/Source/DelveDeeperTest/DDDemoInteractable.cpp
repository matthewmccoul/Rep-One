#include "DDDemoInteractable.h"

#include "DDDemoDirector.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ADDDemoInteractable::ADDDemoInteractable()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Mesh);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(28.f);
    Label->SetTextRenderColor(FColor::White);
    Label->SetAbsolute(false, false, true);
}

void ADDDemoInteractable::Setup(ADDDemoDirector* InDirector, EDDDemoKind InKind, const FString& InId,
                                const FString& InName, UStaticMesh* MeshAsset, const FVector& Scale,
                                const FLinearColor& Color)
{
    Director = InDirector;
    Kind = InKind;
    Id = InId;
    DisplayName = InName;
    Mesh->SetStaticMesh(MeshAsset);
    Mesh->SetWorldScale3D(Scale);
    if (UMaterialInterface* Base =
            LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        if (UMaterialInstanceDynamic* Mat = Mesh->CreateDynamicMaterialInstance(0, Base))
        {
            Mat->SetVectorParameterValue(TEXT("Color"), Color);
        }
    }
    Label->SetText(FText::FromString(InName));
    // Label floats above the mesh's top, independent of the mesh scale.
    const float Top = Mesh->Bounds.BoxExtent.Z;
    Label->SetWorldLocation(GetActorLocation() + FVector(0.f, 0.f, Top + 30.f));
}

FString ADDDemoInteractable::GetPrompt() const
{
    return Director.IsValid() ? Director->PromptFor(this) : FString();
}

void ADDDemoInteractable::Interact(bool bAlt)
{
    if (Director.IsValid())
    {
        Director->HandleInteract(this, bAlt);
    }
}

void ADDDemoInteractable::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Keep the label facing the player camera.
    if (APlayerCameraManager* Cam = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        const FVector ToCam = Cam->GetCameraLocation() - Label->GetComponentLocation();
        Label->SetWorldRotation(FRotator(0.f, ToCam.Rotation().Yaw, 0.f));
    }
}
