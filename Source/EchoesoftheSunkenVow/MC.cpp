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
    Super::BeginPlay();

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



void AMC::TakeCombatDamage(float DamageAmount)
{
    if (!StatsComponent || StatsComponent->bIsDead)
    {
        return;
    }

    StatsComponent->ApplyDamage(DamageAmount);

    if (StatsComponent->bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player defeated!"));

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

    SetActorLocation(
        CurrentCheckpoint->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f)
    );

    SetActorRotation(CurrentCheckpoint->GetActorRotation());

    if (StatsComponent)
    {
        StatsComponent->bIsDead = false;
        StatsComponent->CurrentHealth = StatsComponent->MaxHealth;
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

    UpdateBantayPrompt();
}
