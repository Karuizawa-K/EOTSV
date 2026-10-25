#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StoryDialogueWidget.generated.h"

class AStoryNPC;

UCLASS(Abstract, Blueprintable)
class ECHOESOFTHESUNKENVOW_API UStoryDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Dialogue")
    void ShowDialogueLine(
        AStoryNPC* Speaker,
        const FText& SpeakerName,
        const FText& DialogueLine,
        int32 LineIndex,
        int32 TotalLines
    );

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Dialogue")
    void CloseDialogue();
};
