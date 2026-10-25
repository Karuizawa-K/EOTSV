#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

class UStatsComponent;
class UTextRenderComponent;
class AEnemyProjectile;

UCLASS()
class ECHOESOFTHESUNKENVOW_API ABaseEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ABaseEnemy();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UStatsComponent* StatsComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged")
    bool bIsRangedEnemy = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged", meta = (ClampMin = "0.0", EditCondition = "bIsRangedEnemy"))
    float RangedDetectionRange = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged", meta = (ClampMin = "1.0", EditCondition = "bIsRangedEnemy"))
    float RangedAttackRange = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged", meta = (ClampMin = "0.0", EditCondition = "bIsRangedEnemy"))
    float ProjectileDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged", meta = (ClampMin = "0.0", EditCondition = "bIsRangedEnemy"))
    float RangedAttackCooldown = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged", meta = (ClampMin = "0.0", EditCondition = "bIsRangedEnemy"))
    float RangedWarningDuration = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Ranged", meta = (EditCondition = "bIsRangedEnemy"))
    TSubclassOf<AEnemyProjectile> ProjectileClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Ranged")
    UTextRenderComponent* AttackWarning;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TakeCombatDamage(float DamageAmount);
};
