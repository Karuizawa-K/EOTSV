
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

UCLASS()
class ECHOESOFTHESUNKENVOW_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyAIController();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void FindAndChasePlayer();
    void AttackPlayer();

    UPROPERTY()
        APawn* TargetPlayer;

    float AttackCooldown;
    float AttackTimer;

    float AttackRange;
};
