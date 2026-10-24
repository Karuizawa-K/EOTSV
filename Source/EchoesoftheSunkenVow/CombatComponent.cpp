
#include "CombatComponent.h"
#include "BaseEnemy.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::LightAttack()
{
    UE_LOG(LogTemp, Warning, TEXT("[Combat] LightAttack called."));

    if (bIsAttacking)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Combat] Cooldown active."));
        return;
    }

    AActor* OwnerActor = GetOwner();

    if (!OwnerActor || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[Combat] Owner or World is invalid."));
        return;
    }

    bIsAttacking = true;

    const FVector Start =
        OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

    const FVector End =
        Start + OwnerActor->GetActorForwardVector() * AttackRange;

    UE_LOG(LogTemp, Warning,
        TEXT("[Combat] Owner location: %s | Forward: %s"),
        *OwnerActor->GetActorLocation().ToString(),
        *OwnerActor->GetActorForwardVector().ToString());

    UE_LOG(LogTemp, Warning,
        TEXT("[Combat] Trace Start: %s | End: %s | Radius: %.1f"),
        *Start.ToString(),
        *End.ToString(),
        AttackRadius);

    FCollisionQueryParams QueryParams(
        FName(TEXT("LightAttackTrace")),
        false,
        OwnerActor
    );

    FHitResult HitResult;

    const bool bHit = GetWorld()->SweepSingleByChannel(
        HitResult,
        Start,
        End,
        FQuat::Identity,
        // Use the visibility channel so the trace matches the default
        // character capsule collision setup used by the enemy blueprints.
        ECC_Visibility,
        FCollisionShape::MakeSphere(AttackRadius),
        QueryParams
    );

    DrawDebugLine(
        GetWorld(), Start, End,
        bHit ? FColor::Green : FColor::Red,
        false, 1.5f, 0, 2.0f
    );

    DrawDebugSphere(
        GetWorld(), End, AttackRadius, 20,
        bHit ? FColor::Green : FColor::Red,
        false, 1.5f
    );

    if (!bHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Combat] MISS: nothing blocked the trace."));
    }
    else
    {
        AActor* HitActor = HitResult.GetActor();

        UE_LOG(LogTemp, Warning, TEXT("[Combat] Hit actor: %s"),
            *GetNameSafe(HitActor));

        ABaseEnemy* Enemy = Cast<ABaseEnemy>(HitActor);

        if (Enemy)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Combat] Enemy confirmed. Applying %.1f damage."),
                LightAttackDamage);

            Enemy->TakeCombatDamage(LightAttackDamage);

            UE_LOG(LogTemp, Warning,
                TEXT("[Combat] TakeCombatDamage called on %s."),
                *GetNameSafe(Enemy));
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[Combat] Hit actor is not derived from ABaseEnemy."));
        }
    }

    GetWorld()->GetTimerManager().SetTimer(
        AttackTimerHandle,
        this,
        &UCombatComponent::ResetAttack,
        0.4f,
        false
    );
}

void UCombatComponent::ResetAttack()
{
    bIsAttacking = false;

    UE_LOG(LogTemp, Warning, TEXT("[Combat] Ready to attack again."));
}
