#include "ProgressionBarrier.h"
#include "BantayStatue.h"
#include "BaseEnemy.h"
#include "StatsComponent.h"
#include "Components/StaticMeshComponent.h"

AProgressionBarrier::AProgressionBarrier()
{
    PrimaryActorTick.bCanEverTick = true;

    WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
    RootComponent = WallMesh;
    WallMesh->SetCollisionProfileName(TEXT("BlockAll"));
    WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WallMesh->SetVisibility(true);
    WallMesh->SetHiddenInGame(false);

    UnlockType = EBarrierUnlockType::BantayActivation;
    RequiredBantay = nullptr;
    RequiredBoss = nullptr;
    bIsUnlocked = false;
}

void AProgressionBarrier::BeginPlay()
{
    Super::BeginPlay();

    bIsUnlocked = false;
    WallMesh->SetVisibility(true);
    WallMesh->SetHiddenInGame(false);
    WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    TrackedEnemies.Empty();

    // Invalid initial setup must stay locked, even if a Bantay is assigned later.
    if (UnlockType == EBarrierUnlockType::BantayAndEnemiesDefeated
        && !IsValid(RequiredBantay))
    {
        return;
    }

    if (UnlockType == EBarrierUnlockType::AllEnemiesDefeated
        || UnlockType == EBarrierUnlockType::BantayAndEnemiesDefeated)
    {
        for (ABaseEnemy* Enemy : RequiredEnemies)
        {
            // A missing assignment is not a defeated enemy. Fail closed.
            if (!IsValid(Enemy))
            {
                TrackedEnemies.Empty();
                return;
            }

            TrackedEnemies.Add(TWeakObjectPtr<ABaseEnemy>(Enemy));
        }
    }
    else if (UnlockType == EBarrierUnlockType::BossDefeated && IsValid(RequiredBoss))
    {
        TrackedEnemies.Add(TWeakObjectPtr<ABaseEnemy>(RequiredBoss));
    }
}

void AProgressionBarrier::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsUnlocked)
    {
        return;
    }

    bool bShouldUnlock = false;

    switch (UnlockType)
    {
    case EBarrierUnlockType::BantayActivation:
        bShouldUnlock = IsValid(RequiredBantay) && RequiredBantay->bIsActivated;
        break;

    case EBarrierUnlockType::AllEnemiesDefeated:
    case EBarrierUnlockType::BossDefeated:
        bShouldUnlock = AreTrackedEnemiesDefeated();
        break;

    case EBarrierUnlockType::BantayAndEnemiesDefeated:
        bShouldUnlock = IsValid(RequiredBantay)
            && RequiredBantay->bIsActivated
            && AreTrackedEnemiesDefeated();
        break;

    default:
        break;
    }

    if (bShouldUnlock)
    {
        UnlockBarrier();
    }
}

bool AProgressionBarrier::AreTrackedEnemiesDefeated() const
{
    if (TrackedEnemies.Num() == 0)
    {
        return false;
    }

    for (const TWeakObjectPtr<ABaseEnemy>& EnemyReference : TrackedEnemies)
    {
        ABaseEnemy* Enemy = EnemyReference.Get();

        // Every tracked reference was valid at BeginPlay. Invalid now means removed.
        if (!IsValid(Enemy))
        {
            continue;
        }

        if (!IsValid(Enemy->StatsComponent) || !Enemy->StatsComponent->bIsDead)
        {
            return false;
        }
    }

    return true;
}

void AProgressionBarrier::UnlockBarrier()
{
    bIsUnlocked = true;
    WallMesh->SetVisibility(false);
    WallMesh->SetHiddenInGame(true);
    WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorTickEnabled(false);
}
