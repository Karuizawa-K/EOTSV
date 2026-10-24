
#include "EnemyAIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AEnemyAIController::AEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    TargetPlayer = nullptr;
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Controller started."));
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

    // Reissue the chase if the player is not already being pursued.
    if (GetMoveStatus() != EPathFollowingStatus::Moving)
    {
        FindAndChasePlayer();
    }
}

void AEnemyAIController::FindAndChasePlayer()
{
    TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    if (!TargetPlayer || !GetPawn())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[EnemyAI] Player or enemy pawn not found."));
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[EnemyAI] Chasing player: %s"),
        *GetNameSafe(TargetPlayer));

    MoveToActor(TargetPlayer, 100.0f);
}
