#include "BaseEnemy.h"
#include "StatsComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

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

        // Stop the enemy AI from moving.
        AAIController* AIController = Cast<AAIController>(GetController());

        if (AIController)
        {
            AIController->StopMovement();
        }

        // Prevent further collisions after death.
        SetActorEnableCollision(false);

        // Stop character movement.
        GetCharacterMovement()->DisableMovement();

        // Remove the enemy after two seconds.
        SetLifeSpan(2.0f);
    }
}

