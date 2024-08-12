// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Chatting.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API UChatting : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void ActivateChatText();

	UPROPERTY(meta = (BindWidget))
	class UEditableText* ChatText;

	UPROPERTY(meta = (BindWidget))
	class UButton* SendButton;

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ChatScrollBox;

protected:
	virtual bool Initialize() override;

	UFUNCTION()
	void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnSendButtonClicked();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UChatMessage> ChatMessageClass;

	UPROPERTY()
	class APlayerController* OwningPlayer;

	bool bIsInputActive = false;
};
