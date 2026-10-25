
#include "StatsComponent.h"

UStatsComponent::UStatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UStatsComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
    bIsDead = false;
    RestoreStamina();
}

bool UStatsComponent::TrySpendStamina(float Amount)
{
    if (bIsDead || CurrentHealth <= 0.0f || Amount <= 0.0f || CurrentStamina < Amount)
    {
        return false;
    }

    CurrentStamina = FMath::Clamp(CurrentStamina - Amount, 0.0f, FMath::Max(0.0f, MaxStamina));
    StaminaRegenDelayRemaining = FMath::Max(0.0f, StaminaRegenDelay);

    UE_LOG(LogTemp, Log, TEXT("[Stamina] Spent %.1f | Remaining: %.1f / %.1f"),
        Amount, CurrentStamina, MaxStamina);
    return true;
}

void UStatsComponent::RegenerateStamina(float DeltaTime)
{
    if (bIsDead || CurrentHealth <= 0.0f || DeltaTime <= 0.0f)
    {
        return;
    }

    // Only regenerate for the portion of this frame after the delay expires.
    const float DelayTime = FMath::Min(DeltaTime, StaminaRegenDelayRemaining);
    StaminaRegenDelayRemaining -= DelayTime;
    const float PreviousStamina = CurrentStamina;
    const float StaminaLimit = FMath::Max(0.0f, MaxStamina);
    CurrentStamina = FMath::Clamp(
        CurrentStamina + FMath::Max(0.0f, StaminaRegenRate) * (DeltaTime - DelayTime),
        0.0f, StaminaLimit);

    // Report ten-point milestones and full stamina without logging every frame.
    if (CurrentStamina > PreviousStamina &&
        (FMath::FloorToInt(CurrentStamina / 10.0f) > FMath::FloorToInt(PreviousStamina / 10.0f) ||
            CurrentStamina >= StaminaLimit))
    {
        UE_LOG(LogTemp, Log, TEXT("[Stamina] Regenerated: %.1f / %.1f"),
            CurrentStamina, MaxStamina);
    }
}

void UStatsComponent::RestoreStamina()
{
    CurrentStamina = FMath::Max(0.0f, MaxStamina);
    StaminaRegenDelayRemaining = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("[Stamina] Restored: %.1f / %.1f"),
        CurrentStamina, MaxStamina);
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
