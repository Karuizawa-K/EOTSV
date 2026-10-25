#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryNPC.generated.h"

class AMC;
class USceneComponent;
class UStoryDialogueWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FStoryNPCDialogueEvent,
    AStoryNPC*,
    NPC
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FStoryNPCDialogueLineEvent,
    AStoryNPC*,
    NPC,
    FText,
    SpeakerName,
    FText,
    DialogueLine,
    int32,
    LineIndex,
    int32,
    TotalLines
);

UCLASS()
class ECHOESOFTHESUNKENVOW_API AStoryNPC : public AActor
{
    GENERATED_BODY()

public:
    AStoryNPC();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
    USceneComponent* SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
    FText NPCDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
    TArray<FText> DialogueLines;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
    TArray<FText> ReminderDialogueLines;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
    FName DialogueID;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    TSubclassOf<UStoryDialogueWidget> DialogueWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
    float InteractionDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float RequiredFacingDot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
    bool bIsDialogueActive;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
    int32 CurrentDialogueLineIndex;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FStoryNPCDialogueEvent OnDialogueStarted;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FStoryNPCDialogueLineEvent OnDialogueLineChanged;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
    FStoryNPCDialogueEvent OnDialogueEnded;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void Interact(AMC* Player);

    UFUNCTION(BlueprintPure, Category = "Interaction")
    bool CanInteractWithPlayer(const AMC* Player) const;

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void AdvanceDialogue();

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void EndDialogue(bool bNotifyObjective = true);

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsDialogueActive() const { return bIsDialogueActive; }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    FText GetCurrentDialogueLine() const;

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    int32 GetDialogueLineCount() const { return GetActiveDialogueLines().Num(); }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    int32 GetCurrentLineIndex() const { return CurrentDialogueLineIndex; }

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    const TArray<FText>& GetActiveDialogueLines() const;
    void BroadcastCurrentDialogueLine();
    void NotifyObjectiveManager();
    bool CreateDialogueWidget();

    UPROPERTY()
    AMC* DialoguePlayer;

    UPROPERTY()
    UStoryDialogueWidget* DialogueWidgetInstance;

    bool bHasNotifiedObjectiveCompletion;
    bool bUsingReminderDialogue;
};
