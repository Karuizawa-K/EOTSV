#include "BaseEnemy.h"
#include "StatsComponent.h"
#include "AIController.h"
#include "EnemyAIController.h"
#include "EnemyProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyHealthBarWidget.h"

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

    EnemyHealthWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(
        TEXT("EnemyHealthWidgetComponent")
    );
    EnemyHealthWidgetComponent->SetupAttachment(RootComponent);
    EnemyHealthWidgetComponent->SetRelativeLocation(FVector(
        0.0f,
        0.0f,
        GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + 70.0f
    ));
    EnemyHealthWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    EnemyHealthWidgetComponent->SetDrawSize(FVector2D(160.0f, 25.0f));
    EnemyHealthWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EnemyHealthWidgetComponent->SetGenerateOverlapEvents(false);
}

void ABaseEnemy::BeginPlay()
{
    Super::BeginPlay();

    // Live Coding can clear the cached reference while the component object remains.
    if (!StatsComponent)
    {
        StatsComponent = FindComponentByClass<UStatsComponent>();
    }

    if (EnemyHealthWidgetComponent && EnemyHealthBarWidgetClass)
    {
        EnemyHealthWidgetComponent->SetWidgetClass(EnemyHealthBarWidgetClass);
        EnemyHealthWidgetComponent->InitWidget();
        UpdateEnemyHealthBar();
    }
}

void ABaseEnemy::UpdateEnemyHealthBar()
{
    if (!StatsComponent || !EnemyHealthWidgetComponent)
    {
        return;
    }

    if (UEnemyHealthBarWidget* HealthBarWidget = Cast<UEnemyHealthBarWidget>(
        EnemyHealthWidgetComponent->GetUserWidgetObject()))
    {
        HealthBarWidget->UpdateHealthBar(
            StatsComponent->CurrentHealth,
            StatsComponent->MaxHealth
        );
    }
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
    UpdateEnemyHealthBar();

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

        if (EnemyHealthWidgetComponent)
        {
            EnemyHealthWidgetComponent->SetVisibility(false, true);
        }

        // Stop character movement.
        GetCharacterMovement()->DisableMovement();

        // Remove the enemy after two seconds.
        SetLifeSpan(2.0f);
    }
}

