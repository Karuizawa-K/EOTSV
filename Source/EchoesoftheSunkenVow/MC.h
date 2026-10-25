#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MC.generated.h"

class UInputComponent;
class UCombatComponent;
class UStatsComponent;
class ABantayStatue;
class UUserWidget;

UCLASS()
class ECHOESOFTHESUNKENVOW_API AMC : public ACharacter
{
    GENERATED_BODY()

public:
    AMC();

    UFUNCTION(BlueprintCallable, Category = "Game Over")
        void RetryAfterDeath();

    UFUNCTION(BlueprintCallable, Category = "Game Over")
        void ReturnToMainMenu();

    UFUNCTION()
        void UpdateBantayPrompt();

    virtual void Tick(float DeltaTime) override;

    bool IsDodging() const { return bIsDodging; }


protected:
    virtual void SetupPlayerInputComponent(
        UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void AttackPressed();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void DodgePressed();
    void EndDodge(bool bResetCooldown = false);

    UPROPERTY(EditDefaultsOnly, Category = "Movement|Dodge", meta = (ClampMin = "1.0"))
        float DodgeSpeed = 800.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
        ABantayStatue* CurrentCheckpoint;

    UFUNCTION(BlueprintCallable, Category = "Checkpoint")
        void SetCheckpoint(ABantayStatue* NewCheckpoint);

    UFUNCTION(BlueprintCallable, Category = "Checkpoint")
        void RespawnAtCheckpoint();

    UFUNCTION()
        void InteractWithBantay();

    UPROPERTY(EditDefaultsOnly, Category = "Bantay UI")
        TSubclassOf<UUserWidget> BantayPromptClass;

    UPROPERTY()
        UUserWidget* BantayPromptWidget;

    UPROPERTY(EditDefaultsOnly, Category = "Game Over")
        TSubclassOf<UUserWidget> GameOverWidgetClass;

    UPROPERTY()
        UUserWidget* GameOverWidgetInstance;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UCombatComponent* CombatComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UStatsComponent* StatsComponent;

    UFUNCTION(BlueprintCallable, Category = "Combat")
        void TakeCombatDamage(float DamageAmount);

private:
    bool bIsDodging = false;
    FVector DodgeDirection = FVector::ZeroVector;
    float DodgeStartTime = -1.0f;
    float NextDodgeTime = 0.0f;
    float SavedMaxWalkSpeed = 0.0f;
};
