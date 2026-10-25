#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProgressionBarrier.generated.h"

class ABantayStatue;
class ABaseEnemy;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EBarrierUnlockType : uint8
{
    BantayActivation,
    AllEnemiesDefeated,
    BossDefeated,
    BantayAndEnemiesDefeated
};

UCLASS()
class ECHOESOFTHESUNKENVOW_API AProgressionBarrier : public AActor
{
    GENERATED_BODY()

public:
    AProgressionBarrier();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
    UStaticMeshComponent* WallMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Barrier")
    EBarrierUnlockType UnlockType;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Barrier")
    ABantayStatue* RequiredBantay;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Barrier")
    TArray<ABaseEnemy*> RequiredEnemies;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Barrier")
    ABaseEnemy* RequiredBoss;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
    bool bIsUnlocked;

protected:
    virtual void BeginPlay() override;

private:
    // Snapshot valid assignments before actors can be destroyed or references cleared.
    // Empty means combat requirements were not configured successfully.
    TArray<TWeakObjectPtr<ABaseEnemy>> TrackedEnemies;

    bool AreTrackedEnemiesDefeated() const;
    void UnlockBarrier();
};
