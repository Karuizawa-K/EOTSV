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

    UFUNCTION()
        void UpdateBantayPrompt();

    virtual void Tick(float DeltaTime) override;


protected:
    virtual void SetupPlayerInputComponent(
        UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void AttackPressed();
    virtual void BeginPlay() override;

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

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UCombatComponent* CombatComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UStatsComponent* StatsComponent;

    UFUNCTION(BlueprintCallable, Category = "Combat")
        void TakeCombatDamage(float DamageAmount);
};
