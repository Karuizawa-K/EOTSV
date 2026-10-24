
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BantayStatue.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class ECHOESOFTHESUNKENVOW_API ABantayStatue : public AActor
{
    GENERATED_BODY()

public:
    ABantayStatue();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bantay")
        USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bantay")
        UStaticMeshComponent* StatueMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bantay")
        USphereComponent* ActivationRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bantay")
        float InteractionDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bantay")
        float RequiredFacingDot;

    UFUNCTION()
        void ActivateCheckpoint();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
};
