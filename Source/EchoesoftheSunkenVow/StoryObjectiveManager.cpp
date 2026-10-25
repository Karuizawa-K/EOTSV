#include "StoryObjectiveManager.h"

#include "BantayStatue.h"
#include "ProgressionBarrier.h"

AStoryObjectiveManager::AStoryObjectiveManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    CurrentObjectiveIndex = 0;
}

void AStoryObjectiveManager::BeginPlay()
{
    Super::BeginPlay();

    CurrentObjectiveIndex = 0;
    bCurrentBantayWasActivated = false;
    bCurrentBarrierWasUnlocked = false;
    bIsCompletingObjective = false;
    bAllObjectivesCompleted = false;

    ValidateObjectives();
    CaptureCurrentObjectiveState();

    if (const FStoryObjectiveData* CurrentObjective = GetCurrentObjectiveData())
    {
        OnObjectiveChanged.Broadcast(*CurrentObjective);
    }
}

void AStoryObjectiveManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bAllObjectivesCompleted || bIsCompletingObjective)
    {
        return;
    }

    const FStoryObjectiveData* CurrentObjective = GetCurrentObjectiveData();
    if (!CurrentObjective)
    {
        return;
    }

    switch (CurrentObjective->CompletionType)
    {
    case EStoryObjectiveCompletionType::BantayActivated:
        if (IsValid(CurrentObjective->RequiredBantay)
            && CurrentObjective->RequiredBantay->bIsActivated
            && !bCurrentBantayWasActivated)
        {
            CompleteCurrentObjective();
        }
        break;

    case EStoryObjectiveCompletionType::BarrierUnlocked:
        if (IsValid(CurrentObjective->RequiredBarrier)
            && CurrentObjective->RequiredBarrier->bIsUnlocked
            && !bCurrentBarrierWasUnlocked)
        {
            CompleteCurrentObjective();
        }
        break;

    case EStoryObjectiveCompletionType::DialogueCompleted:
    case EStoryObjectiveCompletionType::LocationReached:
    case EStoryObjectiveCompletionType::BossDefeated:
    default:
        // These objective types complete only through their matching notification.
        break;
    }
}

FStoryObjectiveData AStoryObjectiveManager::GetCurrentObjective() const
{
    if (const FStoryObjectiveData* CurrentObjective = GetCurrentObjectiveData())
    {
        return *CurrentObjective;
    }

    return FStoryObjectiveData();
}

void AStoryObjectiveManager::CompleteCurrentObjective()
{
    if (bIsCompletingObjective || bAllObjectivesCompleted)
    {
        return;
    }

    const FStoryObjectiveData* CurrentObjective = GetCurrentObjectiveData();
    if (!CurrentObjective)
    {
        return;
    }

    bIsCompletingObjective = true;

    const FStoryObjectiveData CompletedObjective = *CurrentObjective;
    OnObjectiveCompleted.Broadcast(CompletedObjective);

    ++CurrentObjectiveIndex;

    if (CurrentObjectiveIndex >= Objectives.Num())
    {
        bAllObjectivesCompleted = true;
        SetActorTickEnabled(false);
        OnAllObjectivesCompleted.Broadcast();
    }
    else
    {
        CaptureCurrentObjectiveState();

        if (const FStoryObjectiveData* NewObjective = GetCurrentObjectiveData())
        {
            OnObjectiveChanged.Broadcast(*NewObjective);
        }
    }

    bIsCompletingObjective = false;
}

void AStoryObjectiveManager::NotifyDialogueCompleted(FName DialogueID)
{
    if (MatchesCurrentEvent(
        EStoryObjectiveCompletionType::DialogueCompleted,
        DialogueID))
    {
        CompleteCurrentObjective();
    }
}

void AStoryObjectiveManager::NotifyLocationReached(FName LocationID)
{
    if (MatchesCurrentEvent(
        EStoryObjectiveCompletionType::LocationReached,
        LocationID))
    {
        CompleteCurrentObjective();
    }
}

void AStoryObjectiveManager::NotifyBossDefeated(FName BossID)
{
    if (MatchesCurrentEvent(
        EStoryObjectiveCompletionType::BossDefeated,
        BossID))
    {
        CompleteCurrentObjective();
    }
}

const FStoryObjectiveData* AStoryObjectiveManager::GetCurrentObjectiveData() const
{
    if (!Objectives.IsValidIndex(CurrentObjectiveIndex))
    {
        return nullptr;
    }

    return &Objectives[CurrentObjectiveIndex];
}

void AStoryObjectiveManager::CaptureCurrentObjectiveState()
{
    bCurrentBantayWasActivated = false;
    bCurrentBarrierWasUnlocked = false;

    const FStoryObjectiveData* CurrentObjective = GetCurrentObjectiveData();
    if (!CurrentObjective)
    {
        return;
    }

    if (CurrentObjective->CompletionType
        == EStoryObjectiveCompletionType::BantayActivated)
    {
        bCurrentBantayWasActivated = IsValid(CurrentObjective->RequiredBantay)
            && CurrentObjective->RequiredBantay->bIsActivated;
    }
    else if (CurrentObjective->CompletionType
        == EStoryObjectiveCompletionType::BarrierUnlocked)
    {
        bCurrentBarrierWasUnlocked = IsValid(CurrentObjective->RequiredBarrier)
            && CurrentObjective->RequiredBarrier->bIsUnlocked;
    }
}

void AStoryObjectiveManager::ValidateObjectives() const
{
    TSet<FName> ObjectiveIDs;

    for (int32 Index = 0; Index < Objectives.Num(); ++Index)
    {
        const FStoryObjectiveData& Objective = Objectives[Index];

        if (Objective.ObjectiveID.IsNone())
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[StoryObjectiveManager] Objective %d has no ObjectiveID."),
                Index
            );
        }
        else if (ObjectiveIDs.Contains(Objective.ObjectiveID))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[StoryObjectiveManager] Duplicate ObjectiveID: %s"),
                *Objective.ObjectiveID.ToString()
            );
        }
        else
        {
            ObjectiveIDs.Add(Objective.ObjectiveID);
        }

        switch (Objective.CompletionType)
        {
        case EStoryObjectiveCompletionType::DialogueCompleted:
        case EStoryObjectiveCompletionType::LocationReached:
        case EStoryObjectiveCompletionType::BossDefeated:
            if (Objective.ExpectedEventID.IsNone())
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("[StoryObjectiveManager] Objective %s has no ExpectedEventID."),
                    *Objective.ObjectiveID.ToString()
                );
            }
            break;

        case EStoryObjectiveCompletionType::BantayActivated:
            if (!IsValid(Objective.RequiredBantay))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("[StoryObjectiveManager] Bantay objective %s has no RequiredBantay."),
                    *Objective.ObjectiveID.ToString()
                );
            }
            break;

        case EStoryObjectiveCompletionType::BarrierUnlocked:
            if (!IsValid(Objective.RequiredBarrier))
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("[StoryObjectiveManager] Barrier objective %s has no RequiredBarrier."),
                    *Objective.ObjectiveID.ToString()
                );
            }
            break;

        default:
            break;
        }
    }
}

bool AStoryObjectiveManager::MatchesCurrentEvent(
    EStoryObjectiveCompletionType ExpectedType,
    FName EventID
) const
{
    if (bIsCompletingObjective || EventID.IsNone())
    {
        return false;
    }

    const FStoryObjectiveData* CurrentObjective = GetCurrentObjectiveData();
    if (!CurrentObjective
        || CurrentObjective->CompletionType != ExpectedType
        || CurrentObjective->ExpectedEventID.IsNone())
    {
        return false;
    }

    return CurrentObjective->ExpectedEventID == EventID;
}
