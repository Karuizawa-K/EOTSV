
#include "StatsComponent.h"

UStatsComponent::UStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStatsComponent::ApplyDamage(float DamageAmount)
{
    if (bIsDead || DamageAmount <= 0.0f)
    {
        return;
    }

    CurrentHealth = FMath::Clamp(
        CurrentHealth - DamageAmount,
        0.0f,
        MaxHealth
    );

    UE_LOG(LogTemp, Warning,
        TEXT("[Stats] Damage: %.1f | Health: %.1f / %.1f"),
        DamageAmount, CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.0f)
    {
        bIsDead = true;

        UE_LOG(LogTemp, Warning, TEXT("[Stats] Character defeated."));
    }
}
