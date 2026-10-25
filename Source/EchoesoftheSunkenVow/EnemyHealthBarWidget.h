#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

UCLASS()
class ECHOESOFTHESUNKENVOW_API UEnemyHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Health")
    void UpdateHealthBar(float CurrentHealth, float MaxHealth);
};
