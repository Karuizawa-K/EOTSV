
#include "CombatComponent.h"
#include "BaseEnemy.h"
#include "MC.h"
#include "StatsComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
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

    AActor* OwnerActor = GetOwner();

    if (!OwnerActor || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[Combat] Owner or World is invalid."));
        return;
    }

    UStatsComponent* OwnerStats = OwnerActor->FindComponentByClass<UStatsComponent>();
    if (!OwnerStats || OwnerStats->bIsDead || OwnerStats->CurrentHealth <= 0.0f)
    {
        CancelAttacks();
        return;
    }

    if (const AMC* PlayerCharacter = Cast<AMC>(OwnerActor))
    {
        if (PlayerCharacter->IsDodging())
        {
            ClearBufferedAttack();
            return;
        }
    }

    const bool bUsesPlayerStamina = OwnerActor->IsA<AMC>();
    const float ComboStaminaCosts[] = { 15.0f, 15.0f, 10.0f };
    const float AttackTime = GetWorld()->GetTimeSeconds();
    const bool bComboExpired = LastAttackTime >= 0.0f &&
        AttackTime - LastAttackTime > FMath::Max(0.0f, ComboResetTime);
    const int32 AttackComboIndex = bComboExpired ? 0 : NextComboIndex;
    const float StaminaCost = ComboStaminaCosts[AttackComboIndex];

    if (bIsAttacking)
    {
        if (bUsesPlayerStamina && OwnerStats->CurrentStamina < StaminaCost)
        {
            // Keep the active attack's cooldown; discard only the pending swing.
            bHasBufferedAttack = false;
            UE_LOG(LogTemp, Log, TEXT("[Stamina] Buffer rejected | Need: %.1f | Available: %.1f"),
                StaminaCost, OwnerStats->CurrentStamina);
            return;
        }

        // Repeated presses cannot add more than one pending attack.
        bHasBufferedAttack = true;
        return;
    }

    // Buffered attacks return here too. Spend only when a swing will execute.
    if (bUsesPlayerStamina && !OwnerStats->TrySpendStamina(StaminaCost))
    {
        bHasBufferedAttack = false;
        UE_LOG(LogTemp, Log, TEXT("[Stamina] Attack rejected | Need: %.1f | Available: %.1f"),
            StaminaCost, OwnerStats->CurrentStamina);
        return;
    }

    const float ComboDamage[] = { LightAttackDamage, SecondHitDamage, ThirdHitDamage };
    const int32 ComboHit = AttackComboIndex + 1;
    const float AttackDamage = FMath::Max(0.0f, ComboDamage[AttackComboIndex]);
    NextComboIndex = (AttackComboIndex + 1) % 3;
    LastAttackTime = AttackTime;
    bHasBufferedAttack = false;
    bIsAttacking = true;

    UE_LOG(LogTemp, Warning, TEXT("[Combat] Combo hit %d | Damage: %.1f"), ComboHit, AttackDamage);

    FVector AttackDirection = OwnerActor->GetActorForwardVector();
    if (AMC* PlayerCharacter = Cast<AMC>(OwnerActor))
    {
        if (AController* PlayerController = PlayerCharacter->GetController())
        {
            // Sample camera yaw when the swing executes, including buffered attacks.
            const FRotator AttackRotation(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f);
            AttackDirection = AttackRotation.Vector();
            PlayerCharacter->SetActorRotation(AttackRotation);
        }
    }

    const FVector Start =
        OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

    const FVector End =
        Start + AttackDirection * AttackRange;

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

    if (bShowDebugTraces)
    {
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
    }

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
                AttackDamage);

            const float HealthBeforeHit = Enemy->StatsComponent
                ? Enemy->StatsComponent->CurrentHealth : 0.0f;

            Enemy->TakeCombatDamage(AttackDamage);

            // Confirm actual damage, including lethal hits, without changing the damage path.
            // Debug drawing is available in Editor/Development builds, not Shipping builds.
            if (Enemy->StatsComponent && Enemy->StatsComponent->CurrentHealth < HealthBeforeHit)
            {
                DrawDebugSphere(
                    GetWorld(), HitResult.ImpactPoint, 12.0f, 12,
                    FColor::Yellow, false, 0.2f, 0, 2.0f
                );
            }

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

    if (bHasBufferedAttack)
    {
        bHasBufferedAttack = false;
        // Reuse the same health check, combo progression, sweep, and cooldown.
        LightAttack();
    }
}

void UCombatComponent::ClearBufferedAttack()
{
    bHasBufferedAttack = false;
}

void UCombatComponent::CancelAttacks()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
    }

    bIsAttacking = false;
    bHasBufferedAttack = false;
    NextComboIndex = 0;
    LastAttackTime = -1.0f;
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelAttacks();
    Super::EndPlay(EndPlayReason);
}
