#pragma once

#include "CoreMinimal.h"
#include "BaseEnemy.h"
#include "BagabagBoss.generated.h"

class UAnimMontage;
class UMatineeCameraShake;

UENUM(BlueprintType)
enum class EBagabagBossState : uint8
{
    Waiting,
    OpeningCharge,
    NormalCombat,
    Defeated
};

UENUM(BlueprintType)
enum class EBagabagMeleeAttackType : uint8
{
    None,
    Punch,
    Swipe
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

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Charge|Impact", meta = (ClampMin = "0.0"))
    float ChargeImpactDamage = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Charge|Impact", meta = (ClampMin = "0.0", Units = "cm"))
    float ChargeImpactRadius = 300.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Charge|Impact")
    bool bChargeImpactRequiresLineOfSight = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Charge|Camera Shake")
    TSubclassOf<UMatineeCameraShake> GroundImpactShakeClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Charge|Camera Shake", meta = (ClampMin = "0.0", Units = "cm"))
    float GroundShakeRadius = 1000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Charge|Camera Shake", meta = (ClampMin = "0.0"))
    float GroundShakeMaxIntensity = 1.0f;

    // Used by future notify-driven Punch and Swipe impact handlers. These
    // values are intentionally independent of damage and hit validation.
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Combat|Camera Shake", meta = (ClampMin = "0.0"))
    float PunchShakeIntensity = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Combat|Camera Shake", meta = (ClampMin = "0.0"))
    float SwipeShakeIntensity = 0.7f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat|Melee")
    UAnimMontage* PunchMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Combat|Melee")
    UAnimMontage* SwipeMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Combat|Melee", meta = (ClampMin = "0.0"))
    float PunchDamage = 20.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Boss|Combat|Melee", meta = (ClampMin = "0.0"))
    float SwipeDamage = 25.0f;

    // This is the extra melee reach between the boss and player capsules.
    // Capsule radii are added once by GetMeleeAttackDistanceToTarget().
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat|Melee", meta = (ClampMin = "0.0", Units = "cm"))
    float MeleeAttackRange = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat|Melee", meta = (ClampMin = "0.0"))
    float MeleeAttackCooldown = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat|Melee", meta = (ClampMin = "0.1"))
    float MeleeAttackRecoveryTimeout = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat|Melee", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float MeleeAttackFacingDot = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Combat|Melee")
    bool bMeleeAttackRequiresLineOfSight = true;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Melee")
    EBagabagMeleeAttackType ActiveMeleeAttack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Combat|Melee")
    float MeleeAttackCooldownRemaining;

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

    // Call only from the charge montage's actual ground-impact notify.
    UFUNCTION(BlueprintCallable, Category = "Boss|Charge|Impact")
    void HandleChargeImpact();

    // Reusable camera shake entry point for future melee impact notifies.
    // Call only when the animation reaches its actual impact frame.
    UFUNCTION(BlueprintCallable, Category = "Boss|Combat|Camera Shake")
    void PlayMeleeImpactShake(float ShakeIntensity);

    bool StartMeleeAttack();
    void CancelMeleeAttack();

    UFUNCTION(BlueprintCallable, Category = "Boss|Combat|Melee|Impact")
    void HandleMeleeAttackImpact();

    UFUNCTION(BlueprintPure, Category = "Boss|Combat|Melee")
    bool IsMeleeAttackActive() const;

    UFUNCTION(BlueprintPure, Category = "Boss|Combat|Melee")
    float GetMeleeAttackCooldownRemaining() const;

    float GetMeleeAttackDistanceToTarget(const AActor* Target) const;

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
    void ApplyChargeImpactDamage(const FVector& ImpactLocation);
    void ApplyMeleeAttackDamage(const FVector& ImpactLocation, float DamageAmount);
    void PlayGroundImpactShake(const FVector& ImpactLocation, float ShakeIntensity = -1.0f);
    void RecoverFromOpeningCharge(const TCHAR* Reason);
    void RecoverFromMeleeAttack(const TCHAR* Reason);
    bool PlayOpeningChargeMontage();

    UFUNCTION()
    void OnOpeningChargeMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void OnMeleeAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    float OpeningChargeElapsedTime;
    bool bOpeningChargeMontageStarted;
    bool bOpeningChargeLeftGround;
    bool bChargeImpactHandled = true;
    float ChargeDiagnosticTimeRemaining = 0.0f;

    UPROPERTY()
    UAnimMontage* ActiveMeleeMontage = nullptr;

    float MeleeAttackElapsedTime = 0.0f;
    bool bMeleeAttackActive = false;
    bool bMeleeAttackImpactHandled = true;
    bool bNextMeleeAttackIsPunch = true;
};
