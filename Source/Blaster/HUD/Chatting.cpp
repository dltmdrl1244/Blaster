// Fill out your copyright notice in the Description page of Project Settings.


#include "Chatting.h"
#include "Components/EditableText.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerState.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void UChatting::NativeConstruct()
{
	Super::NativeConstruct();

	if (SendButton)
	{
		SendButton->OnClicked.AddDynamic(this, &UChatting::OnSendButtonClicked);
	}

	if (ChatText)
	{
		ChatText->OnTextCommitted.AddDynamic(this, &UChatting::OnTextCommitted);
	}

	ChatText->SetIsEnabled(false);
	bIsInputActive = false;
}

bool UChatting::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}
	return true;
}

void UChatting::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		OnSendButtonClicked();
	}
}

void UChatting::OnSendButtonClicked()
{
	if (ChatText)
	{
		FText InputText = ChatText->GetText();
		FString TrimmedText = InputText.ToString().TrimStartAndEnd();

		if (!TrimmedText.IsEmpty())
		{
			ABlasterPlayerController* PlayerController = Cast<ABlasterPlayerController>(GetWorld()->GetFirstPlayerController());
			if (PlayerController)
			{
				APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();
				FString Message = FString::Printf(TEXT("%s : %s"), *PlayerState->GetPlayerName(), *TrimmedText);
				PlayerController->ServerSendChatMessage(Message);
			}
		}

		if (APlayerController* PlayerController = GetOwningPlayer())
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}

		ChatText->SetText(FText::GetEmpty());
		ChatText->SetIsEnabled(false);
		bIsInputActive = false;
	}
}

void UChatting::ActivateChatText()
{
	if (ChatText)
	{
		ChatText->SetIsEnabled(true);
		ChatText->SetFocus();
		bIsInputActive = true;
	}
}