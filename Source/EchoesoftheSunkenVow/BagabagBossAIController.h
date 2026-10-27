#pragma once

#include "CoreMinimal.h"
#include "EnemyAIController.h"
#include "BagabagBoss.h"
#include "BagabagBossAIController.generated.h"

class APawn;

UCLASS()
class ECHOESOFTHESUNKENVOW_API ABagabagBossAIController : public AEnemyAIController
{
    GENERATED_BODY()

public:
    ABagabagBossAIController();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss AI")
    EBagabagBossState CurrentBossAIState;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

private:
    ABagabagBoss* GetControlledBoss() const;
    void UpdateBossStateFromPawn(ABagabagBoss* Boss);
    void CheckForOpeningChargeTarget(ABagabagBoss* Boss);
    void CheckForRepeatChargeTarget(ABagabagBoss* Boss);
    void UpdateNormalMeleeCombat(ABagabagBoss* Boss);
    bool IsPlayerReachable(ABagabagBoss* Boss, APawn* Player) const;
    void LogChargeDiagnostics(ABagabagBoss* Boss) const;

    float ChargeDiagnosticTimeRemaining = 0.0f;

    UPROPERTY()
    TWeakObjectPtr<ABagabagBoss> ControlledBoss;
};
