#include "BagabagBoss.h"

#include "BagabagBossAIController.h"
#include "MC.h"
#include "Kismet/GameplayStatics.h"
#include "StatsComponent.h"
#include "StoryObjectiveManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "MatineeCameraShake.h"

ABagabagBoss::ABagabagBoss()
{
    BossDisplayName = FText::FromString(TEXT("Ang Bagabag"));
    BossID = FName(TEXT("MiniBossDefeated"));

    DetectionRange = 1200.0f;
    bRequireLineOfSight = true;
    OpeningChargeMontage = nullptr;
    PunchMontage = nullptr;
    SwipeMontage = nullptr;
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
    ActiveMeleeAttack = EBagabagMeleeAttackType::None;
    ActiveMeleeMontage = nullptr;
    MeleeAttackCooldownRemaining = 0.0f;
    MeleeAttackElapsedTime = 0.0f;
    bMeleeAttackActive = false;
    bMeleeAttackImpactHandled = true;
    bNextMeleeAttackIsPunch = true;

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
    bChargeImpactHandled = true;
    ActiveMeleeAttack = EBagabagMeleeAttackType::None;
    ActiveMeleeMontage = nullptr;
    MeleeAttackCooldownRemaining = 0.0f;
    MeleeAttackElapsedTime = 0.0f;
    bMeleeAttackActive = false;
    bMeleeAttackImpactHandled = true;
    bNextMeleeAttackIsPunch = true;

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
        || IsAnyMontagePlaying()
        || bMeleeAttackActive)
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

    if (bMeleeAttackActive)
    {
        RecoverFromMeleeAttack(TEXT("Boss defeated."));
    }

    BossState = EBagabagBossState::Defeated;
    bOpeningChargeMontageStarted = false;
    bChargeImpactHandled = true;
    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] State transition: boss -> Defeated.")
    );
}

void ABagabagBoss::NotifyOpeningChargeLandedFromAnimation()
{
    // Preserve existing landing-notify Blueprint wiring. Both entry points
    // represent the same impact and share the same once-per-charge guard.
    HandleChargeImpact();
}

void ABagabagBoss::HandleChargeImpact()
{
    if (BossState != EBagabagBossState::OpeningCharge
        || !bOpeningChargeMontageStarted
        || bChargeImpactHandled
        || !IsValid(StatsComponent)
        || StatsComponent->bIsDead)
    {
        return;
    }

    const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    // A stopped/interrupted montage can still dispatch queued notifies while
    // blending out, before OnMontageEnded has recovered the charge state.
    if (!IsValid(OpeningChargeMontage)
        || !AnimInstance
        || !AnimInstance->Montage_IsPlaying(OpeningChargeMontage)
        || AnimInstance->Montage_GetIsStopped(OpeningChargeMontage))
    {
        return;
    }

    // Consume the impact even on a miss, blocked hit, or successful dodge.
    // Set this before calling the player or any Blueprint landing event.
    bChargeImpactHandled = true;

    const UCapsuleComponent* BossCapsule = GetCapsuleComponent();
    const FVector ImpactLocation = BossCapsule
        ? BossCapsule->GetComponentLocation()
            - BossCapsule->GetUpVector() * BossCapsule->GetScaledCapsuleHalfHeight()
        : GetActorLocation();

    if (bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagBoss] Charge impact notify: Origin=%s Radius=%.1f Damage=%.1f"),
            *ImpactLocation.ToString(), ChargeImpactRadius, ChargeImpactDamage);
    }

    ApplyChargeImpactDamage(ImpactLocation);
    // The shared impact guard has already been consumed. Shake independently
    // of whether the damage helper hit, missed, was blocked, or was dodged.
    PlayGroundImpactShake(ImpactLocation);
    HandleOpeningChargeLanding(true);
}

void ABagabagBoss::PlayMeleeImpactShake(float ShakeIntensity)
{
    if (IsDefeated())
    {
        return;
    }

    // Melee impact handlers can call this independently of their damage
    // result. The boss location is the physical source of the impact.
    PlayGroundImpactShake(GetActorLocation(), ShakeIntensity);
}

bool ABagabagBoss::StartMeleeAttack()
{
    if (BossState != EBagabagBossState::NormalCombat
        || !IsValid(StatsComponent)
        || StatsComponent->bIsDead
        || bMeleeAttackActive
        || MeleeAttackCooldownRemaining > KINDA_SMALL_NUMBER
        || IsAnyMontagePlaying())
    {
        return false;
    }

    UAnimInstance* AnimInstance = GetMesh()
        ? GetMesh()->GetAnimInstance()
        : nullptr;
    if (!AnimInstance)
    {
        return false;
    }

    const bool bCanPunch = IsValid(PunchMontage);
    const bool bCanSwipe = IsValid(SwipeMontage);
    if (!bCanPunch && !bCanSwipe)
    {
        return false;
    }

    EBagabagMeleeAttackType SelectedAttack = bNextMeleeAttackIsPunch
        ? EBagabagMeleeAttackType::Punch
        : EBagabagMeleeAttackType::Swipe;

    if (SelectedAttack == EBagabagMeleeAttackType::Punch && !bCanPunch)
    {
        SelectedAttack = EBagabagMeleeAttackType::Swipe;
    }
    else if (SelectedAttack == EBagabagMeleeAttackType::Swipe && !bCanSwipe)
    {
        SelectedAttack = EBagabagMeleeAttackType::Punch;
    }

    UAnimMontage* SelectedMontage = SelectedAttack == EBagabagMeleeAttackType::Punch
        ? PunchMontage
        : SwipeMontage;
    if (!IsValid(SelectedMontage))
    {
        return false;
    }

    const float MontageLength = PlayAnimMontage(SelectedMontage);
    if (MontageLength <= 0.0f)
    {
        return false;
    }

    ActiveMeleeAttack = SelectedAttack;
    ActiveMeleeMontage = SelectedMontage;
    MeleeAttackElapsedTime = 0.0f;
    bMeleeAttackActive = true;
    bMeleeAttackImpactHandled = false;
    MeleeAttackCooldownRemaining = FMath::Max(0.0f, MeleeAttackCooldown);
    bNextMeleeAttackIsPunch = SelectedAttack != EBagabagMeleeAttackType::Punch;

    AnimInstance->OnMontageEnded.AddUniqueDynamic(
        this,
        &ABagabagBoss::OnMeleeAttackMontageEnded
    );

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Melee attack started: %s."),
        *UEnum::GetValueAsString(ActiveMeleeAttack)
    );
    return true;
}

void ABagabagBoss::CancelMeleeAttack()
{
    RecoverFromMeleeAttack(TEXT("Melee attack cancelled."));
}

void ABagabagBoss::HandleMeleeAttackImpact()
{
    if (BossState != EBagabagBossState::NormalCombat
        || !bMeleeAttackActive
        || bMeleeAttackImpactHandled
        || !IsValid(StatsComponent)
        || StatsComponent->bIsDead
        || !IsValid(ActiveMeleeMontage))
    {
        return;
    }

    const UAnimInstance* AnimInstance = GetMesh()
        ? GetMesh()->GetAnimInstance()
        : nullptr;
    // A queued notify during montage blending-out is not a valid impact.
    if (!AnimInstance
        || !AnimInstance->Montage_IsPlaying(ActiveMeleeMontage)
        || AnimInstance->Montage_GetIsStopped(ActiveMeleeMontage))
    {
        return;
    }

    const float DamageAmount = ActiveMeleeAttack == EBagabagMeleeAttackType::Punch
        ? PunchDamage
        : ActiveMeleeAttack == EBagabagMeleeAttackType::Swipe
            ? SwipeDamage
            : 0.0f;
    const float ShakeIntensity = ActiveMeleeAttack == EBagabagMeleeAttackType::Punch
        ? PunchShakeIntensity
        : ActiveMeleeAttack == EBagabagMeleeAttackType::Swipe
            ? SwipeShakeIntensity
            : 0.0f;

    if (ActiveMeleeAttack != EBagabagMeleeAttackType::Punch
        && ActiveMeleeAttack != EBagabagMeleeAttackType::Swipe)
    {
        return;
    }

    // Consume the notify before any range, facing, wall, dodge, or health
    // validation. The physical impact must shake even when damage is denied.
    bMeleeAttackImpactHandled = true;

    const FVector ImpactLocation = GetActorLocation();
    PlayMeleeImpactShake(ShakeIntensity);
    if (DamageAmount > 0.0f)
    {
        ApplyMeleeAttackDamage(ImpactLocation, DamageAmount);
    }
}

bool ABagabagBoss::IsMeleeAttackActive() const
{
    return bMeleeAttackActive;
}

float ABagabagBoss::GetMeleeAttackCooldownRemaining() const
{
    return MeleeAttackCooldownRemaining;
}

float ABagabagBoss::GetMeleeAttackDistanceToTarget(const AActor* Target) const
{
    if (!Target)
    {
        return 0.0f;
    }

    float AttackDistance = FMath::Max(0.0f, MeleeAttackRange);

    if (const UCapsuleComponent* BossCapsule = GetCapsuleComponent())
    {
        AttackDistance += BossCapsule->GetScaledCapsuleRadius();
    }

    if (const UCapsuleComponent* TargetCapsule =
        Target->FindComponentByClass<UCapsuleComponent>())
    {
        AttackDistance += TargetCapsule->GetScaledCapsuleRadius();
    }

    return AttackDistance;
}

void ABagabagBoss::ApplyMeleeAttackDamage(
    const FVector& ImpactLocation,
    float DamageAmount
)
{
    AMC* Player = Cast<AMC>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!IsValid(Player)
        || !IsValid(Player->StatsComponent)
        || Player->StatsComponent->bIsDead
        || Player->StatsComponent->CurrentHealth <= 0.0f
        || DamageAmount <= 0.0f)
    {
        return;
    }

    const float AttackDistance = GetMeleeAttackDistanceToTarget(Player);
    if (FVector::DistSquared2D(ImpactLocation, Player->GetActorLocation())
        > FMath::Square(AttackDistance))
    {
        return;
    }

    FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
    ToPlayer.Z = 0.0f;
    if (bMeleeAttackRequiresLineOfSight
        || MeleeAttackFacingDot > -1.0f)
    {
        if (!ToPlayer.Normalize())
        {
            return;
        }
    }

    if (MeleeAttackFacingDot > -1.0f
        && FVector::DotProduct(GetActorForwardVector(), ToPlayer)
            < MeleeAttackFacingDot)
    {
        return;
    }

    if (bMeleeAttackRequiresLineOfSight && GetWorld())
    {
        FCollisionQueryParams QueryParams(
            SCENE_QUERY_STAT(BagabagMeleeAttack),
            false,
            this
        );
        QueryParams.AddIgnoredActor(Player);

        FHitResult BlockingHit;
        const FVector TraceStart = GetActorLocation();
        const FVector TraceEnd = Player->GetActorLocation();
        if (GetWorld()->LineTraceSingleByChannel(
            BlockingHit,
            TraceStart,
            TraceEnd,
            ECC_Visibility,
            QueryParams))
        {
            return;
        }
    }

    // Keep player-side dodge invulnerability, health, and Game Over logic
    // authoritative.
    Player->TakeCombatDamage(DamageAmount);
}

void ABagabagBoss::PlayGroundImpactShake(const FVector& ImpactLocation, float ShakeIntensity)
{
    const float EffectiveShakeIntensity = ShakeIntensity >= 0.0f
        ? ShakeIntensity
        : GroundShakeMaxIntensity;

    if (!GroundImpactShakeClass || !GetWorld()
        || !FMath::IsFinite(GroundShakeRadius) || GroundShakeRadius <= 0.0f
        || !FMath::IsFinite(EffectiveShakeIntensity) || EffectiveShakeIntensity <= 0.0f)
    {
        return;
    }

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
    {
        return;
    }

    APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
    if (!IsValid(CameraManager))
    {
        return;
    }

    const float Distance = FVector::Dist(ImpactLocation, CameraManager->GetCameraLocation());
    if (!FMath::IsFinite(Distance) || Distance >= GroundShakeRadius)
    {
        return;
    }

    const float Intensity = EffectiveShakeIntensity
        * FMath::Clamp(1.0f - Distance / GroundShakeRadius, 0.0f, 1.0f);
    if (Intensity > 0.0f)
    {
        CameraManager->StartCameraShake(GroundImpactShakeClass, Intensity);
        if (bEnableChargeDiagnostics)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[BagabagBoss] Ground impact shake: CameraDistance=%.1f Radius=%.1f Intensity=%.3f"),
                Distance, GroundShakeRadius, Intensity);
        }
    }
}

void ABagabagBoss::ApplyChargeImpactDamage(const FVector& ImpactLocation)
{
    AMC* Player = Cast<AMC>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!IsValid(Player)
        || !IsValid(Player->StatsComponent)
        || Player->StatsComponent->bIsDead
        || Player->StatsComponent->CurrentHealth <= 0.0f
        || ChargeImpactDamage <= 0.0f
        || ChargeImpactRadius <= 0.0f)
    {
        return;
    }

    // Test the impact sphere against the capsule, not just the player's
    // actor center. This also avoids damaging players far above/below impact.
    const UCapsuleComponent* PlayerCapsule = Player->GetCapsuleComponent();
    FVector ClosestCapsuleAxisPoint = Player->GetActorLocation();
    float CapsuleRadius = 0.0f;
    if (PlayerCapsule)
    {
        CapsuleRadius = PlayerCapsule->GetScaledCapsuleRadius();
        const float HalfAxisLength = FMath::Max(
            0.0f, PlayerCapsule->GetScaledCapsuleHalfHeight() - CapsuleRadius);
        const FVector CapsuleCenter = PlayerCapsule->GetComponentLocation();
        const FVector AxisOffset = PlayerCapsule->GetUpVector() * HalfAxisLength;
        ClosestCapsuleAxisPoint = FMath::ClosestPointOnSegment(
            ImpactLocation, CapsuleCenter - AxisOffset, CapsuleCenter + AxisOffset);
    }

    if (FVector::DistSquared(ImpactLocation, ClosestCapsuleAxisPoint)
        > FMath::Square(ChargeImpactRadius + CapsuleRadius))
    {
        if (bEnableChargeDiagnostics)
        {
            UE_LOG(LogTemp, Display, TEXT("[BagabagBoss] Charge impact missed: player capsule outside radius."));
        }
        return;
    }

    if (bChargeImpactRequiresLineOfSight && GetWorld())
    {
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BagabagChargeImpact), false, this);
        QueryParams.AddIgnoredActor(Player);
        FHitResult BlockingHit;
        // Start slightly above the capsule foot so a floor-contact tolerance
        // does not make the supporting floor obstruct an otherwise clear hit.
        const FVector TraceStart = ImpactLocation + FVector(0.0f, 0.0f, 10.0f);
        if (GetWorld()->LineTraceSingleByChannel(
            BlockingHit, TraceStart, Player->GetActorLocation(), ECC_Visibility, QueryParams))
        {
            if (bEnableChargeDiagnostics)
            {
                UE_LOG(LogTemp, Display, TEXT("[BagabagBoss] Charge impact blocked by %s."),
                    *GetNameSafe(BlockingHit.GetActor()));
            }
            return;
        }
    }

    // Keep all player-side dodge invulnerability, health, and Game Over logic
    // authoritative. Do not apply damage directly to StatsComponent.
    Player->TakeCombatDamage(ChargeImpactDamage);
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

    const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);

    if (MeleeAttackCooldownRemaining > 0.0f)
    {
        MeleeAttackCooldownRemaining = FMath::Max(
            0.0f,
            MeleeAttackCooldownRemaining - SafeDeltaSeconds
        );
    }

    if (bMeleeAttackActive)
    {
        MeleeAttackElapsedTime += SafeDeltaSeconds;
        if (MeleeAttackElapsedTime >= FMath::Max(0.1f, MeleeAttackRecoveryTimeout))
        {
            RecoverFromMeleeAttack(TEXT("Melee attack recovery timeout."));
        }
    }

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

    OpeningChargeElapsedTime += SafeDeltaSeconds;

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

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Physics landing at %s; waiting for the montage impact notify (no damage from Landed)."),
        *GetActorLocation().ToString()
    );
    // Root-motion playback may touch the floor before the visual slam, or
    // never leave Walking at all. Only the deliberate notify confirms impact.
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
    bChargeImpactHandled = true;
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
    bChargeImpactHandled = true;
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

    bChargeImpactHandled = false;
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

void ABagabagBoss::RecoverFromMeleeAttack(const TCHAR* Reason)
{
    if (!bMeleeAttackActive)
    {
        return;
    }

    UAnimInstance* AnimInstance = GetMesh()
        ? GetMesh()->GetAnimInstance()
        : nullptr;
    UAnimMontage* MontageToStop = ActiveMeleeMontage;

    bMeleeAttackActive = false;
    bMeleeAttackImpactHandled = true;
    ActiveMeleeAttack = EBagabagMeleeAttackType::None;
    ActiveMeleeMontage = nullptr;
    MeleeAttackElapsedTime = 0.0f;

    if (AnimInstance
        && IsValid(MontageToStop)
        && AnimInstance->Montage_IsPlaying(MontageToStop))
    {
        AnimInstance->Montage_Stop(0.1f, MontageToStop);
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBoss] Melee attack recovered: %s."),
        Reason
    );
}

void ABagabagBoss::OnMeleeAttackMontageEnded(
    UAnimMontage* Montage,
    bool bInterrupted
)
{
    if (!bMeleeAttackActive || Montage != ActiveMeleeMontage)
    {
        return;
    }

    RecoverFromMeleeAttack(
        bInterrupted
            ? TEXT("Melee attack montage interrupted.")
            : TEXT("Melee attack montage completed.")
    );
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
