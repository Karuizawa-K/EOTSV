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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
        float LightAttackDamage = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
        float AttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
        float AttackRadius = 45.0f;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
        bool bIsAttacking = false;

    FTimerHandle AttackTimerHandle;

    void ResetAttack();
};