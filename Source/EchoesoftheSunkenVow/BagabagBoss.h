#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "BagabagBoss.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EBagabagBossState : uint8
{
    Waiting,
    OpeningCharge,
    NormalCombat,
    Defeated
};

UCLASS()
class ECHOESOFTHESUNKENVOW_API ABagabagBoss : public ABaseEnemy
{
    GENERATED_BODY()

public:
    ABagabagBoss();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    FText BossDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss")
    FName BossID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Detection", meta = (ClampMin = "0.0"))
    float DetectionRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Detection")
    bool bRequireLineOfSight;

    // Temporary, rate-limited Output Log diagnostics. Disable after testing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Debug")
    bool bEnableChargeDiagnostics = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Opening Charge")
    UAnimMontage* OpeningChargeMontage;

    // Opening-charge detection and travel constraints are separate from the
    // repeat-charge distance window used during NormalCombat.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Opening Charge", meta = (ClampMin = "0.0"))
    float OpeningChargeDistance;

    // Retained for compatibility with the Phase B Blueprint surface. The
    // montage/root-motion implementation does not use LaunchCharacter.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Opening Charge", meta = (ClampMin = "0.0"))
    float OpeningChargeSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Opening Charge", meta = (ClampMin = "0.0"))
    float OpeningChargeUpwardVelocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Opening Charge", meta = (ClampMin = "0.1"))
    float OpeningChargeRecoveryTimeout;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Opening Charge")
    bool bUseLaunchCharacterMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Charge", meta = (ClampMin = "0.0"))
    float ChargeCooldown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Charge", meta = (ClampMin = "0.0"))
    float MinChargeDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Charge", meta = (ClampMin = "0.0"))
    float MaxChargeDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat", meta = (ClampMin = "0.0"))
    float NormalAttackRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat", meta = (ClampMin = "0.0"))
    float NormalAttackDamage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat", meta = (ClampMin = "0.0"))
    float NormalAttackCooldown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
    EBagabagBossState BossState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
    bool bOpeningChargeStarted;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Opening Charge")
    FVector OpeningChargeTargetLocation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Charge")
    float ChargeCooldownRemaining;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Progression")
    bool bBossDeathNotified;

    UFUNCTION(BlueprintPure, Category = "Boss")
    EBagabagBossState GetBossState() const;

    UFUNCTION(BlueprintPure, Category = "Boss")
    bool HasOpeningChargeStarted() const;

    UFUNCTION(BlueprintPure, Category = "Boss")
    bool IsDefeated() const;

    UFUNCTION(BlueprintCallable, Category = "Boss|State")
    bool BeginOpeningChargeState();

    bool StartOpeningChargeAtLocation(
        const FVector& TargetLocation,
        bool bIsGuaranteedOpeningCharge
    );

    UFUNCTION(BlueprintCallable, Category = "Boss|State")
    bool EnterNormalCombatState();

    UFUNCTION(BlueprintCallable, Category = "Boss|Opening Charge")
    void NotifyOpeningChargeLandedFromAnimation();

    UFUNCTION(BlueprintPure, Category = "Boss|Opening Charge")
    bool IsAnyMontagePlaying() const;

    UFUNCTION(BlueprintPure, Category = "Boss|Charge")
    float GetChargeCooldownRemaining() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Opening Charge")
    void OnOpeningChargeStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Opening Charge")
    void OnOpeningChargeLanded();

    UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Opening Charge")
    void OnOpeningChargeRecovered();

    void MarkDefeated();

    // Called by the specialized AI controller after the inherited death path
    // has set StatsComponent->bIsDead.
    void NotifyBossDefeatedOnce();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Landed(const FHitResult& Hit) override;

private:
    void HandleOpeningChargeLanding(bool bWasValidLanding);
    void RecoverFromOpeningCharge(const TCHAR* Reason);
    bool PlayOpeningChargeMontage();

    UFUNCTION()
    void OnOpeningChargeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    float OpeningChargeElapsedTime;
    bool bOpeningChargeMontageStarted;
    bool bOpeningChargeLeftGround;
    float ChargeDiagnosticTimeRemaining = 0.0f;
};
