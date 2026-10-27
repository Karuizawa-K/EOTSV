#include "BagabagBossAIController.h"

#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "MC.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "StatsComponent.h"

ABagabagBossAIController::ABagabagBossAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentBossAIState = EBagabagBossState::Waiting;
}

void ABagabagBossAIController::BeginPlay()
{
    // Do not call AEnemyAIController::BeginPlay(). Its implementation
    // immediately acquires and chases the player. Calling the direct parent
    // here preserves normal controller initialization while keeping this
    // boss in its waiting state.
    AAIController::BeginPlay();

    UE_LOG(LogTemp, Display, TEXT("[BagabagDebug] Controller BeginPlay: Controller=%s Pawn=%s Tick=%d"),
        *GetName(), *GetNameSafe(GetPawn()), IsActorTickEnabled());

    CurrentBossAIState = EBagabagBossState::Waiting;

    if (ABagabagBoss* Boss = GetControlledBoss())
    {
        if (!Boss->IsDefeated())
        {
            CurrentBossAIState = Boss->GetBossState();
        }

        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
    }
}

void ABagabagBossAIController::Tick(float DeltaSeconds)
{
    // Skip AEnemyAIController::Tick(), which contains the regular melee and
    // ranged attack behavior. Phase A must not produce unfinished movement
    // or attacks.
    AAIController::Tick(DeltaSeconds);

    ABagabagBoss* Boss = GetControlledBoss();
    ChargeDiagnosticTimeRemaining -= FMath::Max(0.0f, DeltaSeconds);
    if (ChargeDiagnosticTimeRemaining <= 0.0f)
    {
        ChargeDiagnosticTimeRemaining = 2.0f;
        if (Boss && Boss->bEnableChargeDiagnostics)
        {
            LogChargeDiagnostics(Boss);
        }
        else if (!Boss)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[BagabagDebug] Controller Tick: Controller=%s Pawn=%s FirstFail=NoBagabagPawn"),
                *GetName(), *GetNameSafe(GetPawn()));
        }
    }
    if (!Boss)
    {
        CurrentBossAIState = EBagabagBossState::Waiting;
        return;
    }

    if (Boss->StatsComponent && Boss->StatsComponent->bIsDead)
    {
        Boss->MarkDefeated();
        CurrentBossAIState = EBagabagBossState::Defeated;
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        Boss->NotifyBossDefeatedOnce();
        return;
    }

    UpdateBossStateFromPawn(Boss);

    if (CurrentBossAIState == EBagabagBossState::Waiting)
    {
        // Waiting is intentionally stationary, but it is now allowed to
        // perform the boss-specific first-detection check.
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        CheckForOpeningChargeTarget(Boss);
    }
    else if (CurrentBossAIState == EBagabagBossState::OpeningCharge)
    {
        // Do not call StopMovement here. The charge montage's root motion
        // owns movement during this phase.
        ClearFocus(EAIFocusPriority::Gameplay);
    }
    else if (CurrentBossAIState == EBagabagBossState::NormalCombat)
    {
        CheckForRepeatChargeTarget(Boss);

        // A repeat charge may have transitioned the boss out of NormalCombat.
        // Do not begin a melee attack on the same frame as that transition.
        if (Boss->GetBossState() == EBagabagBossState::NormalCombat)
        {
            UpdateNormalMeleeCombat(Boss);
        }
    }
}

void ABagabagBossAIController::OnPossess(APawn* InPawn)
{
    AAIController::OnPossess(InPawn);

    ControlledBoss = Cast<ABagabagBoss>(InPawn);
    CurrentBossAIState = EBagabagBossState::Waiting;

    if (!ControlledBoss.IsValid() || ControlledBoss->bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[BagabagDebug] Controller OnPossess: Controller=%s RequestedPawn=%s ActualPawn=%s IsBagabag=%d"),
            *GetName(), *GetNameSafe(InPawn), *GetNameSafe(GetPawn()), ControlledBoss.IsValid());
    }

    if (ABagabagBoss* Boss = ControlledBoss.Get())
    {
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        if (!Boss->IsDefeated())
        {
            CurrentBossAIState = Boss->GetBossState();
        }
    }
}

void ABagabagBossAIController::OnUnPossess()
{
    if (ControlledBoss.IsValid() && ControlledBoss->bEnableChargeDiagnostics)
    {
        UE_LOG(LogTemp, Display, TEXT("[BagabagDebug] Controller OnUnPossess: Controller=%s Pawn=%s"),
            *GetName(), *GetNameSafe(GetPawn()));
    }
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);
    ControlledBoss = nullptr;
    CurrentBossAIState = EBagabagBossState::Waiting;

    // Bypass AEnemyAIController::OnUnPossess() because it is tied to the
    // regular ranged-enemy warning state.
    AAIController::OnUnPossess();
}

ABagabagBoss* ABagabagBossAIController::GetControlledBoss() const
{
    if (ABagabagBoss* CachedBoss = ControlledBoss.Get())
    {
        return CachedBoss;
    }

    return Cast<ABagabagBoss>(GetPawn());
}

void ABagabagBossAIController::UpdateBossStateFromPawn(ABagabagBoss* Boss)
{
    if (!Boss)
    {
        CurrentBossAIState = EBagabagBossState::Waiting;
        return;
    }

    CurrentBossAIState = Boss->GetBossState();
}

void ABagabagBossAIController::CheckForOpeningChargeTarget(ABagabagBoss* Boss)
{
    if (!Boss
        || Boss->GetBossState() != EBagabagBossState::Waiting
        || Boss->HasOpeningChargeStarted())
    {
        return;
    }

    AMC* Player = Cast<AMC>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!IsValid(Player)
        || !Player->StatsComponent
        || Player->StatsComponent->bIsDead)
    {
        return;
    }

    const float DetectionRange = FMath::Max(0.0f, Boss->DetectionRange);
    const float DistanceSquared = FVector::DistSquared(
        Boss->GetActorLocation(),
        Player->GetActorLocation()
    );

    if (DistanceSquared > FMath::Square(DetectionRange))
    {
        return;
    }

    if (Boss->bRequireLineOfSight && !LineOfSightTo(Player))
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBossAI] Player detected within %.1f units. Starting opening charge."),
        FMath::Sqrt(DistanceSquared)
    );

    if (!Boss->StartOpeningChargeAtLocation(Player->GetActorLocation(), true))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[BagabagBossAI] Opening charge request was rejected by the boss state guard.")
        );
    }
}

void ABagabagBossAIController::CheckForRepeatChargeTarget(ABagabagBoss* Boss)
{
    if (!Boss
        || Boss->GetBossState() != EBagabagBossState::NormalCombat
        || !Boss->HasOpeningChargeStarted()
        || Boss->GetChargeCooldownRemaining() > KINDA_SMALL_NUMBER
        || Boss->IsAnyMontagePlaying())
    {
        return;
    }

    AMC* Player = Cast<AMC>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!IsValid(Player)
        || !Player->StatsComponent
        || Player->StatsComponent->bIsDead)
    {
        return;
    }

    const float Distance = FVector::Dist(
        Boss->GetActorLocation(),
        Player->GetActorLocation()
    );
    const float MinimumDistance = FMath::Max(0.0f, Boss->MinChargeDistance);
    const float MaximumDistance = FMath::Max(
        MinimumDistance,
        Boss->MaxChargeDistance
    );

    if (Distance < MinimumDistance || Distance > MaximumDistance)
    {
        return;
    }

    // Repeat charges always require clear visibility, regardless of the
    // optional opening-charge LOS setting.
    if (!LineOfSightTo(Player))
    {
        return;
    }

    if (!IsPlayerReachable(Boss, Player))
    {
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[BagabagBossAI] Repeat charge conditions met at %.1f units."),
        Distance
    );

    if (!Boss->StartOpeningChargeAtLocation(Player->GetActorLocation(), false))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[BagabagBossAI] Repeat charge request was rejected by the boss state guard.")
        );
    }
}

void ABagabagBossAIController::UpdateNormalMeleeCombat(ABagabagBoss* Boss)
{
    if (!Boss)
    {
        return;
    }

    AMC* Player = Cast<AMC>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!IsValid(Player)
        || !Player->StatsComponent
        || Player->StatsComponent->bIsDead)
    {
        Boss->CancelMeleeAttack();
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    const float DetectionRange = FMath::Max(0.0f, Boss->MeleeDetectionRange);
    if (FVector::DistSquared2D(
            Boss->GetActorLocation(),
            Player->GetActorLocation()) > FMath::Square(DetectionRange))
    {
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    SetFocus(Player);

    // Keep the boss stationary for the full attack montage. This also keeps
    // navigation from competing with montage playback or recovery.
    if (Boss->IsMeleeAttackActive() || Boss->IsAnyMontagePlaying())
    {
        StopMovement();
        return;
    }

    const float MeleeAttackDistance =
        Boss->GetMeleeAttackDistanceToTarget(Player);
    const float DistanceSquared = FVector::DistSquared2D(
        Boss->GetActorLocation(),
        Player->GetActorLocation());

    if (DistanceSquared > FMath::Square(MeleeAttackDistance))
    {
        // bStopOnOverlap=false is deliberate: MeleeAttackDistance already
        // includes both capsule radii, so navigation must not add them again.
        if (GetMoveStatus() != EPathFollowingStatus::Moving)
        {
            MoveToActor(Player, MeleeAttackDistance, false);
        }
        return;
    }

    StopMovement();

    FVector ToPlayer = Player->GetActorLocation() - Boss->GetActorLocation();
    ToPlayer.Z = 0.0f;
    if (ToPlayer.Normalize())
    {
        const FRotator FacingRotation(0.0f, ToPlayer.Rotation().Yaw, 0.0f);
        SetControlRotation(FacingRotation);
        Boss->SetActorRotation(FacingRotation);
    }

    if (Boss->GetMeleeAttackCooldownRemaining() <= KINDA_SMALL_NUMBER)
    {
        Boss->StartMeleeAttack();
    }
}

bool ABagabagBossAIController::IsPlayerReachable(
    ABagabagBoss* Boss,
    APawn* Player
) const
{
    if (!Boss || !Player || !GetWorld())
    {
        return false;
    }

    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        GetWorld(),
        Boss->GetActorLocation(),
        Player->GetActorLocation(),
        Boss
    );

    return Path && Path->IsValid() && !Path->IsPartial();
}

void ABagabagBossAIController::LogChargeDiagnostics(ABagabagBoss* Boss) const
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    AMC* Player = Cast<AMC>(PlayerPawn);
    const bool bPlayerAlive = IsValid(Player) && IsValid(Player->StatsComponent)
        && !Player->StatsComponent->bIsDead;
    const float Distance = IsValid(PlayerPawn)
        ? FVector::Dist(Boss->GetActorLocation(), PlayerPawn->GetActorLocation()) : -1.0f;
    const bool bLOS = IsValid(PlayerPawn) && LineOfSightTo(PlayerPawn);
    const bool bBusy = Boss->IsAnyMontagePlaying();
    const bool bWaiting = Boss->GetBossState() == EBagabagBossState::Waiting;
    const bool bNormalCombat = Boss->GetBossState() == EBagabagBossState::NormalCombat;
    const TCHAR* Navigation = bWaiting ? TEXT("NotRequiredForOpening") : TEXT("NotEvaluated");
    const TCHAR* FirstFail = TEXT("None_DetectionPassed");

    if (GetPawn() != Boss) FirstFail = TEXT("PawnControllerMismatch");
    else if (!IsValid(Boss->StatsComponent)) FirstFail = TEXT("BossStatsMissing");
    else if (Boss->IsDefeated()) FirstFail = TEXT("BossDefeated");
    else if (!IsValid(PlayerPawn)) FirstFail = TEXT("PlayerPawn0Missing");
    else if (!IsValid(Player)) FirstFail = TEXT("PlayerPawn0IsNotAMC");
    else if (!IsValid(Player->StatsComponent)) FirstFail = TEXT("PlayerStatsMissing");
    else if (!bPlayerAlive) FirstFail = TEXT("PlayerDead");
    else if (!bWaiting && !bNormalCombat) FirstFail = TEXT("ChargeAlreadyInProgress");
    else if (bWaiting && Boss->HasOpeningChargeStarted()) FirstFail = TEXT("OpeningAlreadyUsed");
    else if (bWaiting && Distance > FMath::Max(0.0f, Boss->DetectionRange)) FirstFail = TEXT("OutsideDetectionRange");
    else if (bWaiting && Boss->bRequireLineOfSight && !bLOS) FirstFail = TEXT("OpeningLineOfSightBlocked");
    else if (bNormalCombat && !Boss->HasOpeningChargeStarted()) FirstFail = TEXT("OpeningNotUsed");
    else if (bNormalCombat && Boss->GetChargeCooldownRemaining() > KINDA_SMALL_NUMBER) FirstFail = TEXT("RepeatCooldown");
    else if (bBusy) FirstFail = TEXT("MontageAlreadyPlaying");
    else if (bNormalCombat
        && (Distance < FMath::Max(0.0f, Boss->MinChargeDistance)
            || Distance > FMath::Max(FMath::Max(0.0f, Boss->MinChargeDistance), Boss->MaxChargeDistance)))
        FirstFail = TEXT("OutsideRepeatRange");
    else if (bNormalCombat && !bLOS) FirstFail = TEXT("RepeatLineOfSightBlocked");
    else if (bNormalCombat)
    {
        // Diagnostics query a path only where the normal repeat-charge code
        // would do so. Opening charges never require a navigation path.
        const bool bReachable = IsPlayerReachable(Boss, Player);
        Navigation = bReachable ? TEXT("ValidCompletePath") : TEXT("MissingInvalidOrPartialPath");
        if (!bReachable) FirstFail = TEXT("RepeatNavigationRejected");
    }

    UE_LOG(LogTemp, Display,
        TEXT("[BagabagDebug] AI Tick: Controller=%s Pawn=%s State=%s PlayerPawn0=%s PlayerClass=%s Alive=%d Distance=%.1f DetectionRange=%.1f LOS=%d RequireOpeningLOS=%d Navigation=%s MontageBusy=%d Cooldown=%.2f FirstFail=%s"),
        *GetName(), *Boss->GetName(), *UEnum::GetValueAsString(Boss->GetBossState()),
        *GetNameSafe(PlayerPawn), *GetNameSafe(PlayerPawn ? PlayerPawn->GetClass() : nullptr),
        bPlayerAlive, Distance, Boss->DetectionRange, bLOS, Boss->bRequireLineOfSight,
        Navigation, bBusy, Boss->GetChargeCooldownRemaining(), FirstFail);
}
