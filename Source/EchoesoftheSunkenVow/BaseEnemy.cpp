
#include "BaseEnemy.h"
#include "StatsComponent.h"

ABaseEnemy::ABaseEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    StatsComponent = CreateDefaultSubobject<UStatsComponent>(
        TEXT("StatsComponent")
        );
}

void ABaseEnemy::TakeCombatDamage(float DamageAmount)
{
    if (!StatsComponent || StatsComponent->bIsDead)
    {
        return;
    }

    StatsComponent->ApplyDamage(DamageAmount);

    if (StatsComponent->bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy defeated!"));

        SetActorEnableCollision(false);
    }
}
