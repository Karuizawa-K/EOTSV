#include "BaseEnemy.h"
#include "StatsComponent.h"
#include "AIController.h"
#include "EnemyAIController.h"
#include "EnemyProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseEnemy::ABaseEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    StatsComponent = CreateDefaultSubobject<UStatsComponent>(
        TEXT("StatsComponent")
        );

    ProjectileClass = AEnemyProjectile::StaticClass();

    AttackWarning = CreateDefaultSubobject<UTextRenderComponent>(TEXT("AttackWarning"));
    AttackWarning->SetupAttachment(RootComponent);
    AttackWarning->SetRelativeLocation(FVector(0.0f, 0.0f,
        GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 40.0f));
    AttackWarning->SetText(FText::FromString(TEXT("!")));
    AttackWarning->SetHorizontalAlignment(EHTA_Center);
    AttackWarning->SetVerticalAlignment(EVRTA_TextCenter);
    AttackWarning->SetWorldSize(60.0f);
    AttackWarning->SetTextRenderColor(FColor::Yellow);
    AttackWarning->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttackWarning->SetHiddenInGame(true);
}


void ABaseEnemy::TakeCombatDamage(float DamageAmount)
{
    // Live Coding can clear the cached reference while the component object remains.
    if (!StatsComponent)
    {
        StatsComponent = FindComponentByClass<UStatsComponent>();
    }

    if (!StatsComponent || StatsComponent->bIsDead)
    {
        return;
    }

    StatsComponent->ApplyDamage(DamageAmount);

    if (StatsComponent->bIsDead)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy defeated!"));

        if (bIsRangedEnemy)
        {
            AttackWarning->SetHiddenInGame(true);
            if (AEnemyAIController* EnemyController = Cast<AEnemyAIController>(GetController()))
            {
                EnemyController->CancelRangedAttack();
            }
        }

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

