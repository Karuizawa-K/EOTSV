
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class ABaseEnemy;

UCLASS()
class ECHOESOFTHESUNKENVOW_API AEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    AEnemyAIController();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void CancelRangedAttack();

protected:
    virtual void OnUnPossess() override;

private:
    void FindAndChasePlayer();
    void AttackPlayer();
    void UpdateRangedEnemy(ABaseEnemy* Enemy, float DeltaSeconds);
    void FireProjectile(ABaseEnemy* Enemy);

    bool bIsWindingUp = false;
    float WarningTimeRemaining = 0.0f;
    float RangedCooldownRemaining = 0.0f;

    UPROPERTY()
        APawn* TargetPlayer;

    float AttackCooldown;
    float AttackTimer;

    float AttackRange;
};
