#include "BagabagBoss.h"

#include "BagabagBossAIController.h"
#include "Kismet/GameplayStatics.h"
#include "StatsComponent.h"
#include "StoryObjectiveManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"

ABagabagBoss::ABagabagBoss()
{
    BossDisplayName = FText::FromString(TEXT("Ang Bagabag"));
    BossID = FName(TEXT("MiniBossDefeated"));

    DetectionRange = 1200.0f;
    bRequireLineOfSight = true;
    OpeningChargeMontage = nullptr;
    OpeningChargeDistance = 600.0f;

    // Legacy Phase B values retained for Blueprint compatibility. Movement is
    // now driven only by OpeningChargeMontage root motion.
    OpeningChargeSpeed = 900.0f;
    OpeningChargeUpwardVelocity = 650.0f;
    OpeningChargeRecoveryTimeout = 3.0f;
    bUseLaunchCharacterMovement = false;

    ChargeCooldown = 12.0f;
    MinChargeDistance = 350.0f;
    MaxChargeDistance = 900.0f;

    NormalAttackRange = 220.0f;
    NormalAttackDamage = 10.0f;
    NormalAttackCooldown = 2.0f;

    BossState = EBagabagBossState::Waiting;
    bOpeningChargeStarted = false;
    OpeningChargeTargetLocation = FVector::ZeroVector;
    ChargeCooldownRemaining = 0.0f;
    bBossDeathNotified = false;
    OpeningChargeElapsedTime = 0.0f;
    bOpeningChargeMontageStarted = false;
    bOpeningChargeLeftGround = false;

    PrimaryActorTick.bCanEverTick = true;

    AIControllerClass = ABagabagBossAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABagabagBoss::BeginPlay()
{
    Super::BeginPlay();

    BossState = EBagabagBossState::Waiting;
    bOpeningChargeStarted = false;
    OpeningChargeTargetLocation = FVector::ZeroVector;
    ChargeCooldownRemaining = 0.0f;
    bBossDeathNotified = false;
    OpeningChargeElapsedTime = 0.0f;
    bOpeningChargeMontageStarted = false;
    bOpeningChargeLeftGround = false;

    // Ang Bagabag uses a separate top-center boss UI in a later phase.
    if (EnemyHealthWidgetComponent)
    {
        EnemyHealthWidgetComponent->SetVisibility(false, true);
    }

    if (bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] Boss BeginPlay: Pawn=%s Controller=%s AIClass=%s AutoPossessAI=%d Tick=%d Montage=%s AnimInstance=%s"),
            *GetName(), *GetNameSafe(GetController()), *GetNameSafe(AIControllerClass.Get()),
            static_cast<int32>(AutoPossessAI), IsActorTickEnabled(),
            *GetNameSafe(OpeningChargeMontage),
            *GetNameSafe(GetMesh() ? GetMesh()->GetAnimInstance() : nullptr));
    }
}

EBagabagBossState ABagabagBoss::GetBossState() const
{
    return BossState;
}

bool ABagabagBoss::HasOpeningChargeStarted() const
{
    return bOpeningChargeStarted;
}

bool ABagabagBoss::IsDefeated() const
{
    return BossState == EBagabagBossState::Defeated
        || (StatsComponent && StatsComponent->bIsDead);
}

bool ABagabagBoss::BeginOpeningChargeState()
{
    if (BossState != EBagabagBossState::Waiting
        || bOpeningChargeStarted
        || !StatsComponent
        || StatsComponent->bIsDead)
    {
        return false;
    }

    bOpeningChargeStarted = true;
    BossState = EBagabagBossState::OpeningCharge;
    OpeningChargeElapsedTime = 0.0f;
    bOpeningChargeMontageStarted = false;
    bOpeningChargeLeftGround = false;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] State transition: Waiting -> OpeningCharge.")
    );

    return true;
}

bool ABagabagBoss::StartOpeningChargeAtLocation(
    const FVector& TargetLocation,
    bool bIsGuaranteedOpeningCharge
)
{
    if (bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] StartOpeningCharge invoked: Pawn=%s Opening=%d State=%s StatsValid=%d Dead=%d MontageBusy=%d Target=%s"),
            *GetName(), bIsGuaranteedOpeningCharge, *UEnum::GetValueAsString(BossState),
            IsValid(StatsComponent), StatsComponent && StatsComponent->bIsDead,
            IsAnyMontagePlaying(), *TargetLocation.ToString());
    }

    if (!StatsComponent
        || StatsComponent->bIsDead
        || IsAnyMontagePlaying())
    {
        return false;
    }

    if (bIsGuaranteedOpeningCharge)
    {
        if (!BeginOpeningChargeState())
        {
            return false;
        }
    }
    else
    {
        if (BossState != EBagabagBossState::NormalCombat
            || !bOpeningChargeStarted
            || ChargeCooldownRemaining > KINDA_SMALL_NUMBER)
        {
            return false;
        }

        BossState = EBagabagBossState::OpeningCharge;
        OpeningChargeElapsedTime = 0.0f;
        bOpeningChargeMontageStarted = false;
        bOpeningChargeLeftGround = false;

        UE_LOG(
            LogTemp,
            Display,
            TEXT("[BagabagBoss] State transition: NormalCombat -> OpeningCharge (repeat charge).")
        );
    }

    const FVector CurrentLocation = GetActorLocation();
    FVector ToTarget = TargetLocation - CurrentLocation;
    ToTarget.Z = 0.0f;

    const float DistanceToTarget = ToTarget.Size();
    const float MaximumChargeDistance = FMath::Max(
        0.0f,
        bIsGuaranteedOpeningCharge ? OpeningChargeDistance : MaxChargeDistance
    );

    if (DistanceToTarget > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = ToTarget / DistanceToTarget;
        const float ChargeDistance = FMath::Min(
            DistanceToTarget,
            MaximumChargeDistance
        );

        OpeningChargeTargetLocation = CurrentLocation + Direction * ChargeDistance;
        OpeningChargeTargetLocation.Z = TargetLocation.Z;

        const FRotator FacingRotation(0.0f, Direction.Rotation().Yaw, 0.0f);
        SetActorRotation(FacingRotation);
    }
    else
    {
        OpeningChargeTargetLocation = CurrentLocation;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Charge target captured at %s. Maximum target distance: %.1f."),
        *OpeningChargeTargetLocation.ToString(),
        MaximumChargeDistance
    );

    OnOpeningChargeStarted();

    if (bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] After OnOpeningChargeStarted: State=%s ActiveMontage=%s AssignedMontage=%s"),
            *UEnum::GetValueAsString(BossState), *GetNameSafe(GetCurrentMontage()),
            *GetNameSafe(OpeningChargeMontage));
    }

    if (BossState != EBagabagBossState::OpeningCharge
        || (StatsComponent && StatsComponent->bIsDead))
    {
        return true;
    }

    if (!PlayOpeningChargeMontage())
    {
        RecoverFromOpeningCharge(TEXT("Opening charge montage could not start."));
    }

    return true;
}

bool ABagabagBoss::EnterNormalCombatState()
{
    if (BossState != EBagabagBossState::OpeningCharge
        || !StatsComponent
        || StatsComponent->bIsDead)
    {
        return false;
    }

    BossState = EBagabagBossState::NormalCombat;
    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] State transition: OpeningCharge -> NormalCombat.")
    );
    return true;
}

void ABagabagBoss::MarkDefeated()
{
    if (BossState == EBagabagBossState::Defeated)
    {
        return;
    }

    if (BossState == EBagabagBossState::OpeningCharge
        && GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
    }

    BossState = EBagabagBossState::Defeated;
    bOpeningChargeMontageStarted = false;
    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] State transition: boss -> Defeated.")
    );
}

void ABagabagBoss::NotifyOpeningChargeLandedFromAnimation()
{
    if (BossState != EBagabagBossState::OpeningCharge
        || !bOpeningChargeMontageStarted)
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Opening charge landing received from animation/Blueprint.")
    );
    HandleOpeningChargeLanding(true);
}

bool ABagabagBoss::IsAnyMontagePlaying() const
{
    const UAnimInstance* AnimInstance = GetMesh()
        ? GetMesh()->GetAnimInstance()
        : nullptr;
    return AnimInstance && AnimInstance->IsAnyMontagePlaying();
}

float ABagabagBoss::GetChargeCooldownRemaining() const
{
    return ChargeCooldownRemaining;
}

void ABagabagBoss::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ChargeDiagnosticTimeRemaining -= FMath::Max(0.0f, DeltaSeconds);
    if (bEnableChargeDiagnostics && ChargeDiagnosticTimeRemaining <= 0.0f)
    {
        ChargeDiagnosticTimeRemaining = 2.0f;
        const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] Boss Tick: Pawn=%s Controller=%s State=%s Elapsed=%.2f Timeout=%.2f AnimInstance=%s ActiveMontage=%s Position=%.2f MovementMode=%d MeshTick=%d Paused=%d"),
            *GetName(), *GetNameSafe(GetController()), *UEnum::GetValueAsString(BossState),
            OpeningChargeElapsedTime, OpeningChargeRecoveryTimeout, *GetNameSafe(AnimInstance),
            *GetNameSafe(GetCurrentMontage()),
            AnimInstance && OpeningChargeMontage ? AnimInstance->Montage_GetPosition(OpeningChargeMontage) : -1.0f,
            GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1,
            GetMesh() && GetMesh()->IsComponentTickEnabled(), GetMesh() && GetMesh()->bPauseAnims);
        if (AnimInstance && OpeningChargeMontage)
        {
            for (const FSlotAnimationTrack& Track : OpeningChargeMontage->SlotAnimTracks)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[BagabagDebug] Montage slot: %s GlobalWeight=%.3f (verify Slot is connected to the AnimGraph output if weight stays zero during playback)"),
                    *Track.SlotName.ToString(), AnimInstance->GetSlotMontageGlobalWeight(Track.SlotName));
            }
        }
    }

    if (BossState != EBagabagBossState::OpeningCharge
        && ChargeCooldownRemaining > 0.0f)
    {
        ChargeCooldownRemaining = FMath::Max(
            0.0f,
            ChargeCooldownRemaining - FMath::Max(0.0f, DeltaSeconds)
        );
    }

    if (BossState != EBagabagBossState::OpeningCharge)
    {
        return;
    }

    OpeningChargeElapsedTime += FMath::Max(0.0f, DeltaSeconds);

    if (GetCharacterMovement()
        && !GetCharacterMovement()->IsMovingOnGround())
    {
        bOpeningChargeLeftGround = true;
    }

    if (OpeningChargeElapsedTime >= FMath::Max(0.1f, OpeningChargeRecoveryTimeout))
    {
        RecoverFromOpeningCharge(TEXT("Opening charge recovery timeout."));
    }
}

void ABagabagBoss::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    if (BossState != EBagabagBossState::OpeningCharge
        || !bOpeningChargeMontageStarted)
    {
        return;
    }

    if (!bOpeningChargeLeftGround)
    {
        RecoverFromOpeningCharge(TEXT("Landed without a confirmed airborne phase."));
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Opening charge landed successfully at %s."),
        *GetActorLocation().ToString()
    );
    HandleOpeningChargeLanding(true);
}

void ABagabagBoss::HandleOpeningChargeLanding(bool bWasValidLanding)
{
    if (BossState != EBagabagBossState::OpeningCharge
        || !bWasValidLanding
        || (StatsComponent && StatsComponent->bIsDead))
    {
        return;
    }

    OpeningChargeElapsedTime = 0.0f;
    bOpeningChargeMontageStarted = false;
    bOpeningChargeLeftGround = false;
    ChargeCooldownRemaining = FMath::Max(0.0f, ChargeCooldown);
    EnterNormalCombatState();
    OnOpeningChargeLanded();
}

void ABagabagBoss::RecoverFromOpeningCharge(const TCHAR* Reason)
{
    if (BossState != EBagabagBossState::OpeningCharge)
    {
        return;
    }

    if (StatsComponent && StatsComponent->bIsDead)
    {
        MarkDefeated();
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[BagabagBoss] Opening charge recovered: %s Elapsed=%.2f Timeout=%.2f ActiveMontage=%s"),
        Reason, OpeningChargeElapsedTime, OpeningChargeRecoveryTimeout, *GetNameSafe(GetCurrentMontage())
    );

    OpeningChargeElapsedTime = 0.0f;
    bOpeningChargeMontageStarted = false;
    bOpeningChargeLeftGround = false;
    ChargeCooldownRemaining = FMath::Max(0.0f, ChargeCooldown);

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->StopMovementImmediately();
    }

    BossState = EBagabagBossState::NormalCombat;
    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] State transition: OpeningCharge -> NormalCombat after recovery.")
    );
    OnOpeningChargeRecovered();
}

bool ABagabagBoss::PlayOpeningChargeMontage()
{
    if (!OpeningChargeMontage)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[BagabagBoss] OpeningChargeMontage is not assigned.")
        );
        return false;
    }

    UAnimInstance* AnimInstance = GetMesh()
        ? GetMesh()->GetAnimInstance()
        : nullptr;
    if (!AnimInstance)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[BagabagBoss] Charge blocked: no AnimInstance on the inherited Mesh. Check Animation Mode and Anim Class on BP_AngBagabag's Mesh.")
        );
        return false;
    }

    // The existing Blueprint start event may already have played this exact
    // montage. Adopt it and bind recovery instead of rejecting or restarting it.
    const bool bAlreadyPlayingCharge = AnimInstance->Montage_IsPlaying(OpeningChargeMontage);
    if (!bAlreadyPlayingCharge && AnimInstance->IsAnyMontagePlaying())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BagabagBoss] Charge blocked after Blueprint start event: ActiveMontage=%s AssignedCharge=%s. A different montage must not interrupt the charge."),
            *GetNameSafe(AnimInstance->GetCurrentActiveMontage()), *GetNameSafe(OpeningChargeMontage));
        return false;
    }

    const float MontageLength = bAlreadyPlayingCharge
        ? OpeningChargeMontage->GetPlayLength()
        : PlayAnimMontage(OpeningChargeMontage);
    if (bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] Montage playback: Mode=%s Montage=%s AnimInstance=%s Duration=%.3f Position=%.3f IsPlaying=%d"),
            bAlreadyPlayingCharge ? TEXT("AdoptBlueprintPlayback_NoRestart") : TEXT("PlayAnimMontage"),
            *GetNameSafe(OpeningChargeMontage), *GetNameSafe(AnimInstance), MontageLength,
            AnimInstance->Montage_GetPosition(OpeningChargeMontage),
            AnimInstance->Montage_IsPlaying(OpeningChargeMontage));
    }
    if (MontageLength <= 0.0f)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[BagabagBoss] PlayAnimMontage returned no playable length for the opening charge.")
        );
        return false;
    }

    // Do not replace an end delegate installed by a Blueprint Play Montage
    // callback proxy when adopting its playback. Observe the multicast event
    // and filter by the assigned montage in the handler instead.
    AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &ABagabagBoss::OnOpeningChargeMontageEnded);

    bOpeningChargeMontageStarted = true;
    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Opening charge montage started: %s."),
        *OpeningChargeMontage->GetName()
    );
    return true;
}

void ABagabagBoss::OnOpeningChargeMontageEnded(
    UAnimMontage* Montage,
    bool bInterrupted
)
{
    if (bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] Montage ended: Montage=%s Interrupted=%d State=%s Elapsed=%.3f"),
            *GetNameSafe(Montage), bInterrupted, *UEnum::GetValueAsString(BossState), OpeningChargeElapsedTime);
    }

    if (Montage != OpeningChargeMontage
        || BossState != EBagabagBossState::OpeningCharge)
    {
        return;
    }

    if (bInterrupted)
    {
        RecoverFromOpeningCharge(TEXT("Opening charge montage was interrupted."));
    }
    else
    {
        RecoverFromOpeningCharge(
            TEXT("Opening charge montage completed without a confirmed landing.")
        );
    }
}

void ABagabagBoss::NotifyBossDefeatedOnce()
{
    if (bBossDeathNotified
        || !StatsComponent
        || !StatsComponent->bIsDead
        || BossState != EBagabagBossState::Defeated)
    {
        return;
    }

    bBossDeathNotified = true;

    AStoryObjectiveManager* ObjectiveManager = Cast<AStoryObjectiveManager>(
        UGameplayStatics::GetActorOfClass(
            GetWorld(),
            AStoryObjectiveManager::StaticClass()
        )
    );

    if (ObjectiveManager)
    {
        const FName NotificationID = BossID.IsNone()
            ? FName(TEXT("MiniBossDefeated"))
            : BossID;
        ObjectiveManager->NotifyBossDefeated(NotificationID);
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[BagabagBoss] No StoryObjectiveManager found for boss notification.")
        );
    }
}
