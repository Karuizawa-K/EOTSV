
#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "MC.h"

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
