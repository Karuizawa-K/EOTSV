
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        float CurrentHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
        bool bIsDead = false;

    UFUNCTION(BlueprintCallable, Category = "Stats")
        void ApplyDamage(float DamageAmount);
};
