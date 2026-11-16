// Demo-only: something the player can walk up to and press E (or Q) on. Logic lives in ADDDemoDirector.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DDDemoInteractable.generated.h"

class ADDDemoDirector;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM()
enum class EDDDemoKind : uint8
{
    Carving,
    Control,
    Figure,
    StoreroomDoor
};

UCLASS()
class ADDDemoInteractable : public AActor
{
    GENERATED_BODY()

public:
    ADDDemoInteractable();

    void Setup(ADDDemoDirector* InDirector, EDDDemoKind InKind, const FString& InId, const FString& InName,
               UStaticMesh* MeshAsset, const FVector& Scale, const FLinearColor& Color);

    FString GetPrompt() const;
    void Interact(bool bAlt);

    virtual void Tick(float DeltaSeconds) override;

    EDDDemoKind Kind = EDDDemoKind::Control;
    FString Id;
    FString DisplayName;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;

    TWeakObjectPtr<ADDDemoDirector> Director;
};
