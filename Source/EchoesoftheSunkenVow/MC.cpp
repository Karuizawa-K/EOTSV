
#include "MC.h"
#include "CombatComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"

AMC::AMC()
{
    PrimaryActorTick.bCanEverTick = false;

    CombatComponent = CreateDefaultSubobject<UCombatComponent>(
        TEXT("CombatComponent")
        );

    // Allow the controller to rotate the character.
    bUseControllerRotationYaw = false;

    // Rotate the character toward its movement direction.
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

    // Configure the camera to follow controller rotation.
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
}

void AMC::SetupPlayerInputComponent(
    UInputComponent* PlayerInputComponent
)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Movement
    PlayerInputComponent->BindAxis(
        "MoveForward", this, &AMC::MoveForward
    );

    PlayerInputComponent->BindAxis(
        "MoveRight", this, &AMC::MoveRight
    );

    // Mouse camera control
    PlayerInputComponent->BindAxis(
        "Turn", this, &AMC::Turn
    );

    PlayerInputComponent->BindAxis(
        "LookUp", this, &AMC::LookUp
    );

    // Jump
    PlayerInputComponent->BindAction(
        "Jump", IE_Pressed, this, &ACharacter::Jump
    );

    PlayerInputComponent->BindAction(
        "Jump", IE_Released, this, &ACharacter::StopJumping
    );

    // Combat
    PlayerInputComponent->BindAction(
        "Attack", IE_Pressed, this, &AMC::AttackPressed
    );
}

void AMC::MoveForward(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

        const FVector Direction =
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        AddMovementInput(Direction, Value);
    }
}

void AMC::MoveRight(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

        const FVector Direction =
            FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(Direction, Value);
    }
}

void AMC::Turn(float Value)
{
    AddControllerYawInput(Value * 0.3f);
}

void AMC::LookUp(float Value)
{
    AddControllerPitchInput(Value * 0.3f);
}

void AMC::AttackPressed()
{
    if (CombatComponent)
    {
        CombatComponent->LightAttack();
    }
}
