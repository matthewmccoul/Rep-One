// Demo-only first-person character: WASD, mouse look, Space jump, E interact, Q alternate interact.
// Input actions and mapping are created in code (no input assets needed).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "DDDemoCharacter.generated.h"

class ADDDemoInteractable;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class ADDDemoCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADDDemoCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PawnClientRestart() override;
    virtual void Tick(float DeltaSeconds) override;

    ADDDemoInteractable* GetFocus() const { return Focus.Get(); }

private:
    void EnsureInput();
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Interact();
    void AltInteract();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> Context;

    UPROPERTY()
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY()
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY()
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY()
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY()
    TObjectPtr<UInputAction> AltAction;

    TWeakObjectPtr<ADDDemoInteractable> Focus;
    float InteractRange = 280.f;
};
