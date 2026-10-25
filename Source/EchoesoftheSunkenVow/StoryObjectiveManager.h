#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryObjectiveManager.generated.h"

class ABantayStatue;
class AProgressionBarrier;

UENUM(BlueprintType)
enum class EStoryObjectiveCompletionType : uint8
{
    DialogueCompleted,
    BantayActivated,
    BarrierUnlocked,
    LocationReached,
    BossDefeated
};

USTRUCT(BlueprintType)
struct ECHOESOFTHESUNKENVOW_API FStoryObjectiveData
{
    GENERATED_BODY()

    FStoryObjectiveData()
        : ObjectiveID(NAME_None)
        , CompletionType(EStoryObjectiveCompletionType::DialogueCompleted)
        , RequiredBantay(nullptr)
        , RequiredBarrier(nullptr)
        , ExpectedEventID(NAME_None)
    {
    }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FName ObjectiveID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FText ObjectiveTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FText ObjectiveDescription;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    EStoryObjectiveCompletionType CompletionType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    ABantayStatue* RequiredBantay;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    AProgressionBarrier* RequiredBarrier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective")
    FName ExpectedEventID;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FStoryObjectiveEvent,
    FStoryObjectiveData,
    Objective
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStoryAllObjectivesCompletedEvent);

UCLASS()
class ECHOESOFTHESUNKENVOW_API AStoryObjectiveManager : public AActor
{
    GENERATED_BODY()

public:
    AStoryObjectiveManager();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Story Objectives")
    TArray<FStoryObjectiveData> Objectives;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Story Objectives")
    int32 CurrentObjectiveIndex;

    UPROPERTY(BlueprintAssignable, Category = "Story Objectives|Events")
    FStoryObjectiveEvent OnObjectiveChanged;

    UPROPERTY(BlueprintAssignable, Category = "Story Objectives|Events")
    FStoryObjectiveEvent OnObjectiveCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Story Objectives|Events")
    FStoryAllObjectivesCompletedEvent OnAllObjectivesCompleted;

    UFUNCTION(BlueprintCallable, Category = "Story Objectives")
    FStoryObjectiveData GetCurrentObjective() const;

    UFUNCTION(BlueprintCallable, Category = "Story Objectives")
    void CompleteCurrentObjective();

    UFUNCTION(BlueprintCallable, Category = "Story Objectives")
    void NotifyDialogueCompleted(FName DialogueID);

    UFUNCTION(BlueprintCallable, Category = "Story Objectives")
    void NotifyLocationReached(FName LocationID);

    UFUNCTION(BlueprintCallable, Category = "Story Objectives")
    void NotifyBossDefeated(FName BossID);

protected:
    virtual void BeginPlay() override;

private:
    const FStoryObjectiveData* GetCurrentObjectiveData() const;
    void CaptureCurrentObjectiveState();
    void ValidateObjectives() const;
    bool MatchesCurrentEvent(
        EStoryObjectiveCompletionType ExpectedType,
        FName EventID
    ) const;

    bool bCurrentBantayWasActivated = false;
    bool bCurrentBarrierWasUnlocked = false;
    bool bIsCompletingObjective = false;
    bool bAllObjectivesCompleted = false;
};
