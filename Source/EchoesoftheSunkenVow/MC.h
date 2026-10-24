
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MC.generated.h"

class UInputComponent;
class UCombatComponent;

UCLASS()
class ECHOESOFTHESUNKENVOW_API AMC : public ACharacter
{
    GENERATED_BODY()

public:
    AMC();

protected:
    virtual void SetupPlayerInputComponent(
        UInputComponent* PlayerInputComponent) override;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void AttackPressed();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
    UCombatComponent* CombatComponent;
};
