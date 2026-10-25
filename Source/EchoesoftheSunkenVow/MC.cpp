#include "MC.h"
#include "CombatComponent.h"
#include "StatsComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "BantayStatue.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

AMC::AMC()
{
    PrimaryActorTick.bCanEverTick = true;

    CombatComponent = CreateDefaultSubobject<UCombatComponent>(
        TEXT("CombatComponent")
        );

    StatsComponent = CreateDefaultSubobject<UStatsComponent>(TEXT("StatsComponent"));

    // Allow the controller to rotate the character.
    bUseControllerRotationYaw = false;

    // Rotate the character toward its movement direction.
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

    // Configure the camera to follow controller rotation.
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    CurrentCheckpoint = nullptr;
    BantayPromptWidget = nullptr;

}

void AMC::BeginPlay()
{
    // Hot Reload can clear the cached property while leaving the owned component intact.
    if (!StatsComponent)
    {
        StatsComponent = FindComponentByClass<UStatsComponent>();
    }

    Super::BeginPlay();

    if (!StatsComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("[Stats] Player has no StatsComponent; health and stamina cannot update."));
    }

    if (BantayPromptClass)
    {
        BantayPromptWidget = CreateWidget<UUserWidget>(
            GetWorld(),
            BantayPromptClass
            );

        if (BantayPromptWidget)
        {
            BantayPromptWidget->AddToViewport();
            BantayPromptWidget->SetVisibility(
                ESlateVisibility::Hidden
            );
        }
    }
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

    PlayerInputComponent->BindAction(
        "Dodge", IE_Pressed, this, &AMC::DodgePressed
    );

    // Combat
    PlayerInputComponent->BindAction(
        "Attack", IE_Pressed, this, &AMC::AttackPressed
    );

    PlayerInputComponent->BindAction(
        "Interact",
        IE_Pressed,
        this,
        &AMC::InteractWithBantay
    );
}

void AMC::MoveForward(float Value)
{
    if (!bIsDodging && Controller && Value != 0.0f)
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
    if (!bIsDodging && Controller && Value != 0.0f)
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

void AMC::DodgePressed()
{
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (bIsDodging || !Controller || !InputComponent || !GetWorld() || !StatsComponent
        || !Movement || !Movement->IsMovingOnGround() || Controller->IsMoveInputIgnored()
        || GetWorld()->GetTimeSeconds() < NextDodgeTime)
    {
        return;
    }

    const FRotator CameraYaw(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    const FVector Forward = CameraYaw.Vector();
    const FVector Right = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);
    FVector Direction = Forward * InputComponent->GetAxisValue(TEXT("MoveForward"))
        + Right * InputComponent->GetAxisValue(TEXT("MoveRight"));
    if (!Direction.Normalize())
    {
        Direction = -Forward;
    }

    if (!StatsComponent->TrySpendStamina(20.0f))
    {
        return;
    }

    bIsDodging = true;
    DodgeDirection = Direction;
    DodgeStartTime = GetWorld()->GetTimeSeconds();
    NextDodgeTime = DodgeStartTime + 0.9f;
    SavedMaxWalkSpeed = Movement->MaxWalkSpeed;
    Movement->MaxWalkSpeed = FMath::Max(1.0f, DodgeSpeed);

    // Replace old horizontal momentum; CharacterMovement still handles collision and gravity.
    Movement->Velocity.X = DodgeDirection.X * Movement->MaxWalkSpeed;
    Movement->Velocity.Y = DodgeDirection.Y * Movement->MaxWalkSpeed;
    ConsumeMovementInputVector();

    if (CombatComponent)
    {
        CombatComponent->ClearBufferedAttack();
    }

    UE_LOG(LogTemp, Log, TEXT("[Dodge] Started | Direction: %s"), *DodgeDirection.ToString());
}

void AMC::EndDodge(bool bResetCooldown)
{
    if (bResetCooldown)
    {
        NextDodgeTime = 0.0f;
    }

    if (!bIsDodging)
    {
        return;
    }

    bIsDodging = false;
    DodgeStartTime = -1.0f;
    DodgeDirection = FVector::ZeroVector;
    ConsumeMovementInputVector();

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = SavedMaxWalkSpeed;
        // Keep vertical velocity so jumping or falling still works normally.
        Movement->Velocity.X = 0.0f;
        Movement->Velocity.Y = 0.0f;
    }

    UE_LOG(LogTemp, Log, TEXT("[Dodge] Ended."));
}

void AMC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndDodge(true);
    Super::EndPlay(EndPlayReason);
}



void AMC::TakeCombatDamage(float DamageAmount)
{
    if (!StatsComponent || StatsComponent->bIsDead)
    {
        return;
    }

    if (bIsDodging && GetWorld() && GetWorld()->GetTimeSeconds() - DodgeStartTime < 0.3f)
    {
        UE_LOG(LogTemp, Log, TEXT("[Dodge] Damage avoided."));
        return;
    }

    StatsComponent->ApplyDamage(DamageAmount);

    if (StatsComponent->bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player defeated!"));

        EndDodge(true);

        // Discard queued swings before a checkpoint can immediately revive the player.
        if (CombatComponent)
        {
            CombatComponent->CancelAttacks();
        }

        // Stop player movement.
        GetCharacterMovement()->DisableMovement();

        // Stop player input.
        if (APlayerController* PlayerController =
            Cast<APlayerController>(GetController()))
        {
            PlayerController->StopMovement();
            DisableInput(PlayerController);
        }

        // Prevent further collisions.
        SetActorEnableCollision(false);

        // Respawn at the last activated Bantay.
        if (CurrentCheckpoint)
        {
            RespawnAtCheckpoint();
        }
        else
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("No checkpoint activated. Player remains defeated.")
            );
        }
    }
}



void AMC::SetCheckpoint(ABantayStatue* NewCheckpoint)
{
    if (!NewCheckpoint)
    {
        return;
    }

    CurrentCheckpoint = NewCheckpoint;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Checkpoint set to: %s"),
        *GetNameSafe(CurrentCheckpoint)
    );
}

void AMC::RespawnAtCheckpoint()
{
    if (!CurrentCheckpoint)
    {
        UE_LOG(LogTemp, Warning, TEXT("No Bantay checkpoint activated."));
        return;
    }

    EndDodge(true);

    SetActorLocation(
        CurrentCheckpoint->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f)
    );

    SetActorRotation(CurrentCheckpoint->GetActorRotation());

    if (StatsComponent)
    {
        StatsComponent->bIsDead = false;
        StatsComponent->CurrentHealth = StatsComponent->MaxHealth;
        StatsComponent->RestoreStamina();
    }

    SetActorEnableCollision(true);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
    }

    UE_LOG(LogTemp, Warning, TEXT("Player respawned at Bantay."));
}


void AMC::InteractWithBantay()
{
    if (StatsComponent && StatsComponent->bIsDead)
    {
        return;
    }

    TArray<AActor*> FoundStatues;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        ABantayStatue::StaticClass(),
        FoundStatues
    );

    ABantayStatue* BestStatue = nullptr;
    float BestDistance = TNumericLimits<float>::Max();

    for (AActor* Actor : FoundStatues)
    {
        ABantayStatue* Statue = Cast<ABantayStatue>(Actor);

        if (!Statue)
        {
            continue;
        }

        FVector ToStatue =
            Statue->GetActorLocation() - GetActorLocation();

        float Distance = ToStatue.Size();

        if (Distance > Statue->InteractionDistance || Distance > BestDistance)
        {
            continue;
        }

        if (!ToStatue.Normalize())
        {
            continue;
        }

        float FacingDot = FVector::DotProduct(
            GetActorForwardVector(),
            ToStatue
        );

        if (FacingDot >= Statue->RequiredFacingDot)
        {
            BestStatue = Statue;
            BestDistance = Distance;
        }
    }

    if (BestStatue)
    {
        SetCheckpoint(BestStatue);
        BestStatue->ActivateCheckpoint();

        if (StatsComponent)
        {
            StatsComponent->CurrentHealth = StatsComponent->MaxHealth;
            StatsComponent->RestoreStamina();

            UE_LOG(LogTemp, Log, TEXT("[Bantay] Restored HP: %.1f / %.1f | Stamina: %.1f / %.1f"),
                StatsComponent->CurrentHealth, StatsComponent->MaxHealth,
                StatsComponent->CurrentStamina, StatsComponent->MaxStamina);
        }

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                3.0f,
                FColor::Yellow,
                TEXT("Bantay activated!")
            );
        }

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Bantay activated!")
            );
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Move closer to the Bantay and face it.")
        );
    }
}

void AMC::UpdateBantayPrompt()
{
    if (!BantayPromptWidget || !GetWorld())
    {
        return;
    }

    bool bCanShowPrompt = false;

    if (!StatsComponent || !StatsComponent->bIsDead)
    {
        TArray<AActor*> FoundStatues;

        UGameplayStatics::GetAllActorsOfClass(
            GetWorld(),
            ABantayStatue::StaticClass(),
            FoundStatues
        );

        for (AActor* Actor : FoundStatues)
        {
            ABantayStatue* Statue = Cast<ABantayStatue>(Actor);

            if (!Statue)
            {
                continue;
            }

            FVector ToStatue =
                Statue->GetActorLocation() - GetActorLocation();

            const float Distance = ToStatue.Size();

            if (Distance > Statue->InteractionDistance || Distance <= 0.0f)
            {
                continue;
            }

            ToStatue.Normalize();

            const float FacingDot = FVector::DotProduct(
                GetActorForwardVector(),
                ToStatue
            );

            if (FacingDot >= Statue->RequiredFacingDot)
            {
                bCanShowPrompt = true;
                break;
            }
        }
    }

    BantayPromptWidget->SetVisibility(
        bCanShowPrompt
        ? ESlateVisibility::Visible
        : ESlateVisibility::Hidden
    );
}

void AMC::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDodging)
    {
        UCharacterMovementComponent* Movement = GetCharacterMovement();
        if (!StatsComponent || StatsComponent->bIsDead || StatsComponent->CurrentHealth <= 0.0f)
        {
            EndDodge(true);
        }
        else if (GetWorld()->GetTimeSeconds() >= NextDodgeTime
            || !Movement || !Movement->IsMovingOnGround())
        {
            // Leaving the ground ends movement/invulnerability, but retains the cooldown.
            EndDodge();
        }
        else
        {
            ConsumeMovementInputVector();
            AddMovementInput(DodgeDirection, 1.0f);
        }
    }

    if (StatsComponent)
    {
        StatsComponent->RegenerateStamina(DeltaTime);
    }

    UpdateBantayPrompt();
}
