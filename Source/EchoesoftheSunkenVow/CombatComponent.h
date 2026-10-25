#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AActor;
class UWorld;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHOESOFTHESUNKENVOW_API UCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatComponent();

    UFUNCTION(BlueprintCallable, Category = "Combat")
        void LightAttack();

    // Retain the existing property name for Blueprint references; it now controls hit one.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (DisplayName = "First Hit Damage", ClampMin = "0.0"))
        float LightAttackDamage = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
        float SecondHitDamage = 14.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0.0"))
        float ThirdHitDamage = 22.0f;

    // Measured from the previous executed attack, including a miss.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo", meta = (ClampMin = "0.0", Units = "s"))
        float ComboResetTime = 1.0f;

    // Call at death before an immediate respawn can restore the owner's health.
    void CancelAttacks();

    // Discard a queued swing without changing the active cooldown or combo timing.
    void ClearBufferedAttack();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
        float AttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
        float AttackRadius = 45.0f;

    // Show the attack sweep for troubleshooting; hit confirmation is independent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Debug")
        bool bShowDebugTraces = false;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
        bool bIsAttacking = false;

    FTimerHandle AttackTimerHandle;

    void ResetAttack();

private:
    bool bHasBufferedAttack = false;
    int32 NextComboIndex = 0;
    float LastAttackTime = -1.0f;
};
