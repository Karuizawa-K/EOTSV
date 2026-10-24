#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseEnemy.generated.h"

class UStatsComponent;

UCLASS()
class ECHOESOFTHESUNKENVOW_API ABaseEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ABaseEnemy();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UStatsComponent* StatsComponent;

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void TakeCombatDamage(float DamageAmount);
};
