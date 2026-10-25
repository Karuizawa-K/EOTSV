#include "StoryNPC.h"

#include "BantayStatue.h"
#include "Blueprint/UserWidget.h"
#include "Components/SceneComponent.h"
#include "MC.h"
#include "Kismet/GameplayStatics.h"
#include "StoryDialogueWidget.h"
#include "StoryObjectiveManager.h"

AStoryNPC::AStoryNPC()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    NPCDisplayName = FText::FromString(TEXT("NPC"));
    DialogueID = NAME_None;
    InteractionDistance = 250.0f;
    RequiredFacingDot = 0.5f;
    bIsDialogueActive = false;
    CurrentDialogueLineIndex = INDEX_NONE;
    DialoguePlayer = nullptr;
    DialogueWidgetInstance = nullptr;
    bHasNotifiedObjectiveCompletion = false;
    bUsingReminderDialogue = false;
}

void AStoryNPC::Interact(AMC* Player)
{
    if (bIsDialogueActive || !Player || !CanInteractWithPlayer(Player))
    {
        return;
    }

    bUsingReminderDialogue = bHasNotifiedObjectiveCompletion
        && ReminderDialogueLines.Num() > 0;

    if (GetActiveDialogueLines().Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Dialogue] NPC %s has no dialogue lines."), *GetName());
        return;
    }

    if (!Player->BeginStoryDialogue(this))
    {
        bUsingReminderDialogue = false;
        return;
    }

    DialoguePlayer = Player;
    bIsDialogueActive = true;
    CurrentDialogueLineIndex = 0;

    CreateDialogueWidget();
    OnDialogueStarted.Broadcast(this);
    BroadcastCurrentDialogueLine();
}

bool AStoryNPC::CanInteractWithPlayer(const AMC* Player) const
{
    if (!Player || bIsDialogueActive)
    {
        return false;
    }

    FVector ToNPC = GetActorLocation() - Player->GetActorLocation();
    const float Distance = ToNPC.Size();

    if (Distance <= 0.0f || Distance > InteractionDistance)
    {
        return false;
    }

    ToNPC.Normalize();

    return FVector::DotProduct(Player->GetActorForwardVector(), ToNPC)
        >= RequiredFacingDot;
}

void AStoryNPC::AdvanceDialogue()
{
    if (!bIsDialogueActive)
    {
        return;
    }

    const TArray<FText>& ActiveDialogueLines = GetActiveDialogueLines();
    const int32 NextLineIndex = CurrentDialogueLineIndex + 1;
    if (ActiveDialogueLines.IsValidIndex(NextLineIndex))
    {
        CurrentDialogueLineIndex = NextLineIndex;
        BroadcastCurrentDialogueLine();
        return;
    }

    EndDialogue(true);
}

void AStoryNPC::EndDialogue(bool bNotifyObjective)
{
    if (!bIsDialogueActive)
    {
        return;
    }

    bIsDialogueActive = false;

    OnDialogueEnded.Broadcast(this);

    if (DialogueWidgetInstance)
    {
        DialogueWidgetInstance->CloseDialogue();
        DialogueWidgetInstance->RemoveFromParent();
        DialogueWidgetInstance = nullptr;
    }

    AMC* Player = DialoguePlayer;
    DialoguePlayer = nullptr;
    bUsingReminderDialogue = false;

    if (Player)
    {
        Player->EndStoryDialogue(this);
    }

    if (bNotifyObjective && !bHasNotifiedObjectiveCompletion)
    {
        bHasNotifiedObjectiveCompletion = true;
        NotifyObjectiveManager();
    }
}

FText AStoryNPC::GetCurrentDialogueLine() const
{
    const TArray<FText>& ActiveDialogueLines = GetActiveDialogueLines();
    if (ActiveDialogueLines.IsValidIndex(CurrentDialogueLineIndex))
    {
        return ActiveDialogueLines[CurrentDialogueLineIndex];
    }

    return FText::GetEmpty();
}

void AStoryNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndDialogue(false);
    Super::EndPlay(EndPlayReason);
}

void AStoryNPC::BroadcastCurrentDialogueLine()
{
    const FText CurrentLine = GetCurrentDialogueLine();
    const TArray<FText>& ActiveDialogueLines = GetActiveDialogueLines();

    OnDialogueLineChanged.Broadcast(
        this,
        NPCDisplayName,
        CurrentLine,
        CurrentDialogueLineIndex,
        ActiveDialogueLines.Num()
    );

    if (DialogueWidgetInstance)
    {
        DialogueWidgetInstance->ShowDialogueLine(
            this,
            NPCDisplayName,
            CurrentLine,
            CurrentDialogueLineIndex,
            ActiveDialogueLines.Num()
        );
    }
}

const TArray<FText>& AStoryNPC::GetActiveDialogueLines() const
{
    return bUsingReminderDialogue && ReminderDialogueLines.Num() > 0
        ? ReminderDialogueLines
        : DialogueLines;
}

void AStoryNPC::NotifyObjectiveManager()
{
    if (DialogueID.IsNone() || !GetWorld())
    {
        return;
    }

    TArray<AActor*> FoundManagers;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AStoryObjectiveManager::StaticClass(),
        FoundManagers
    );

    for (AActor* Actor : FoundManagers)
    {
        if (AStoryObjectiveManager* Manager = Cast<AStoryObjectiveManager>(Actor))
        {
            Manager->NotifyDialogueCompleted(DialogueID);
            return;
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Dialogue] No StoryObjectiveManager found for DialogueID %s."),
        *DialogueID.ToString()
    );
}

bool AStoryNPC::CreateDialogueWidget()
{
    if (DialogueWidgetInstance || !DialogueWidgetClass || !GetWorld())
    {
        return DialogueWidgetInstance != nullptr;
    }

    DialogueWidgetInstance = CreateWidget<UStoryDialogueWidget>(
        GetWorld(),
        DialogueWidgetClass
    );

    if (!DialogueWidgetInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Dialogue] Failed to create widget for NPC %s."), *GetName());
        return false;
    }

    DialogueWidgetInstance->AddToViewport();
    DialogueWidgetInstance->SetVisibility(ESlateVisibility::Visible);
    return true;
}
