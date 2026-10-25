
#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "MC.h"
#include "BaseEnemy.h"
#include "EnemyProjectile.h"
#include "StatsComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyAIController::AEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    TargetPlayer = nullptr;

    AttackCooldown = 1.5f;
    AttackTimer = 0.0f;
    AttackRange = 200.0f;
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    FindAndChasePlayer();
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn());
    if (Enemy && Enemy->bIsRangedEnemy)
    {
        UpdateRangedEnemy(Enemy, DeltaSeconds);
        return;
    }

    if (!TargetPlayer || !GetPawn())
    {
        FindAndChasePlayer();
        return;
    }

    const float Distance = FVector::Dist(
        GetPawn()->GetActorLocation(),
        TargetPlayer->GetActorLocation()
    );

    if (Distance <= AttackRange)
    {
        StopMovement();

        AttackTimer -= DeltaSeconds;

        if (AttackTimer <= 0.0f)
        {
            AttackPlayer();
            AttackTimer = AttackCooldown;
        }
    }
    else
    {
        if (GetMoveStatus() != EPathFollowingStatus::Moving)
        {
            FindAndChasePlayer();
        }
    }
}

void AEnemyAIController::FindAndChasePlayer()
{
    // Ranged enemies acquire their target only after range and visibility checks.
    ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn());
    if (Enemy && Enemy->bIsRangedEnemy)
    {
        return;
    }

    TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (TargetPlayer && GetPawn())
    {
        MoveToActor(TargetPlayer, AttackRange * 0.75f);
    }
}

void AEnemyAIController::AttackPlayer()
{
    AMC* PlayerCharacter = Cast<AMC>(TargetPlayer);

    if (PlayerCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy attacks player!"));

        PlayerCharacter->TakeCombatDamage(10.0f);
    }
}

void AEnemyAIController::CancelRangedAttack()
{
    bIsWindingUp = false;
    WarningTimeRemaining = 0.0f;

    if (ABaseEnemy* Enemy = Cast<ABaseEnemy>(GetPawn()))
    {
        Enemy->AttackWarning->SetHiddenInGame(true);
    }
}

void AEnemyAIController::OnUnPossess()
{
    CancelRangedAttack();
    TargetPlayer = nullptr;
    RangedCooldownRemaining = 0.0f;
    Super::OnUnPossess();
}

void AEnemyAIController::UpdateRangedEnemy(ABaseEnemy* Enemy, float DeltaSeconds)
{
    RangedCooldownRemaining = FMath::Max(0.0f, RangedCooldownRemaining - DeltaSeconds);

    // Recover an existing component if Hot Reload cleared the cached reference.
    if (!Enemy->StatsComponent)
    {
        Enemy->StatsComponent = Enemy->FindComponentByClass<UStatsComponent>();
    }

    if (!Enemy->StatsComponent || Enemy->StatsComponent->bIsDead)
    {
        CancelRangedAttack();
        TargetPlayer = nullptr;
        ClearFocus(EAIFocusPriority::Gameplay);
        StopMovement();
        return;
    }

    AMC* Player = Cast<AMC>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    const float DetectionRange = FMath::Max(0.0f, Enemy->RangedDetectionRange);
    if (!IsValid(Player) || !Player->StatsComponent || Player->StatsComponent->bIsDead
        || FVector::DistSquared(Enemy->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(DetectionRange)
        || !LineOfSightTo(Player))
    {
        CancelRangedAttack();
        TargetPlayer = nullptr;
        ClearFocus(EAIFocusPriority::Gameplay);
        StopMovement();
        return;
    }

    if (TargetPlayer != Player)
    {
        CancelRangedAttack();
        TargetPlayer = Player;
    }

    const float FiringRange = FMath::Max(1.0f, Enemy->RangedAttackRange);
    SetFocus(Player);
    if (FVector::DistSquared(Enemy->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(FiringRange))
    {
        CancelRangedAttack();
        if (GetMoveStatus() != EPathFollowingStatus::Moving)
        {
            // Disable the extra capsule-radius allowance to reach the actual firing range.
            MoveToActor(Player, FiringRange * 0.75f, false);
        }
        return;
    }

    StopMovement();
    Enemy->GetCharacterMovement()->StopMovementImmediately();
    const FVector ToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
    const FRotator FacingRotation(0.0f, ToPlayer.Rotation().Yaw, 0.0f);
    SetControlRotation(FacingRotation);
    Enemy->SetActorRotation(FacingRotation);

    // Billboard the warning toward the camera, including an elevated third-person camera.
    if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
    {
        Enemy->AttackWarning->SetWorldRotation(
            (Camera->GetCameraLocation() - Enemy->AttackWarning->GetComponentLocation()).Rotation());
    }

    if (!Enemy->ProjectileClass)
    {
        CancelRangedAttack();
        return;
    }

    if (bIsWindingUp)
    {
        WarningTimeRemaining -= DeltaSeconds;
        if (WarningTimeRemaining <= 0.0f)
        {
            CancelRangedAttack();
            FireProjectile(Enemy);
            RangedCooldownRemaining = FMath::Max(0.0f, Enemy->RangedAttackCooldown);
        }
    }
    else if (RangedCooldownRemaining <= 0.0f)
    {
        bIsWindingUp = true;
        WarningTimeRemaining = FMath::Max(0.0f, Enemy->RangedWarningDuration);
        Enemy->AttackWarning->SetHiddenInGame(false);
    }
}

void AEnemyAIController::FireProjectile(ABaseEnemy* Enemy)
{
    if (!IsValid(TargetPlayer) || !GetWorld())
    {
        return;
    }

    // Start inside the shooter's capsule and ignore the shooter, rather than
    // spawning ahead of the capsule where a nearby wall could be skipped.
    const FVector SpawnLocation = Enemy->GetActorLocation();
    const FRotator AimRotation = (TargetPlayer->GetActorLocation() - SpawnLocation).Rotation();
    const FTransform SpawnTransform(AimRotation, SpawnLocation);
    AEnemyProjectile* Projectile = GetWorld()->SpawnActorDeferred<AEnemyProjectile>(
        Enemy->ProjectileClass, SpawnTransform, Enemy, Enemy,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (Projectile)
    {
        // Initialize before BeginPlay and any initial overlap callbacks.
        Projectile->Damage = FMath::Max(0.0f, Enemy->ProjectileDamage);
        UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
    }
}
