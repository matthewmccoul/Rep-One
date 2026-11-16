#include "DDDemoCharacter.h"

#include "DDDemoInteractable.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

ADDDemoCharacter::ADDDemoCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    Camera->bUsePawnControlRotation = true;
    bUseControllerRotationYaw = true;
    GetCharacterMovement()->MaxWalkSpeed = 450.f;
    GetCharacterMovement()->JumpZVelocity = 520.f;
    GetCharacterMovement()->AirControl = 0.5f;
}

void ADDDemoCharacter::EnsureInput()
{
    if (Context)
    {
        return;
    }
    auto MakeAction = [this](EInputActionValueType Type) {
        UInputAction* A = NewObject<UInputAction>(this);
        A->ValueType = Type;
        return A;
    };
    MoveAction = MakeAction(EInputActionValueType::Axis2D);
    LookAction = MakeAction(EInputActionValueType::Axis2D);
    JumpAction = MakeAction(EInputActionValueType::Boolean);
    InteractAction = MakeAction(EInputActionValueType::Boolean);
    AltAction = MakeAction(EInputActionValueType::Boolean);

    Context = NewObject<UInputMappingContext>(this);
    auto MapMove = [this](FKey Key, bool bSwizzle, bool bNegate) {
        FEnhancedActionKeyMapping& M = Context->MapKey(MoveAction, Key);
        if (bSwizzle)
        {
            UInputModifierSwizzleAxis* S = NewObject<UInputModifierSwizzleAxis>(this);
            S->Order = EInputAxisSwizzle::YXZ;
            M.Modifiers.Add(S);
        }
        if (bNegate)
        {
            M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
        }
    };
    MapMove(EKeys::W, true, false);
    MapMove(EKeys::S, true, true);
    MapMove(EKeys::D, false, false);
    MapMove(EKeys::A, false, true);

    FEnhancedActionKeyMapping& LookMap = Context->MapKey(LookAction, EKeys::Mouse2D);
    UInputModifierNegate* NegY = NewObject<UInputModifierNegate>(this);
    NegY->bX = false;
    NegY->bY = true;
    NegY->bZ = false;
    LookMap.Modifiers.Add(NegY);

    Context->MapKey(JumpAction, EKeys::SpaceBar);
    Context->MapKey(InteractAction, EKeys::E);
    Context->MapKey(AltAction, EKeys::Q);
}

void ADDDemoCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    EnsureInput();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Sub->ClearAllMappings();
            Sub->AddMappingContext(Context, 0);
        }
    }
}

void ADDDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    EnsureInput();
    if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADDDemoCharacter::Move);
        Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADDDemoCharacter::Look);
        Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ADDDemoCharacter::Interact);
        Input->BindAction(AltAction, ETriggerEvent::Started, this, &ADDDemoCharacter::AltInteract);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Demo] Input component is not an EnhancedInputComponent; check Config/DefaultInput.ini"));
    }
}

void ADDDemoCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D V = Value.Get<FVector2D>();
    AddMovementInput(GetActorForwardVector(), V.Y);
    AddMovementInput(GetActorRightVector(), V.X);
}

void ADDDemoCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D V = Value.Get<FVector2D>();
    AddControllerYawInput(V.X);
    AddControllerPitchInput(V.Y);
}

void ADDDemoCharacter::Interact()
{
    if (ADDDemoInteractable* Target = Focus.Get())
    {
        Target->Interact(false);
    }
}

void ADDDemoCharacter::AltInteract()
{
    if (ADDDemoInteractable* Target = Focus.Get())
    {
        Target->Interact(true);
    }
}

void ADDDemoCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Focus = nearest interactable in range that has a prompt.
    ADDDemoInteractable* Best = nullptr;
    float BestDist = InteractRange;
    const FVector Me = GetActorLocation();
    for (TActorIterator<ADDDemoInteractable> It(GetWorld()); It; ++It)
    {
        const float D = FVector::Dist2D(Me, It->GetActorLocation());
        if (D < BestDist && FMath::Abs(Me.Z - It->GetActorLocation().Z) < 250.f && !It->GetPrompt().IsEmpty())
        {
            Best = *It;
            BestDist = D;
        }
    }
    Focus = Best;
}
