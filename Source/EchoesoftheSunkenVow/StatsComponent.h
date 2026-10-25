
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatsComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHOESOFTHESUNKENVOW_API UStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStatsComponent();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        float CurrentHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        bool bIsDead = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Stamina", meta = (ClampMin = "0.0"))
        float MaxStamina = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Stamina")
        float CurrentStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Stamina", meta = (ClampMin = "0.0"))
        float StaminaRegenRate = 22.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats|Stamina", meta = (ClampMin = "0.0", Units = "s"))
        float StaminaRegenDelay = 0.7f;

    bool TrySpendStamina(float Amount);
    void RegenerateStamina(float DeltaTime);
    void RestoreStamina();

    UFUNCTION(BlueprintCallable, Category = "Stats")
        void ApplyDamage(float DamageAmount);

private:
    float StaminaRegenDelayRemaining = 0.0f;
};
