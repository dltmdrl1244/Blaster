// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
    if (DisplayText)
    {
        DisplayText->SetText(FText::FromString(TextToDisplay));
    }

}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
    ENetRole LocalRole = InPawn->GetLocalRole();
    ENetRole RemoteRole = InPawn->GetRemoteRole();
    FString Role;

    // switch (LocalRole)
    switch (RemoteRole)
    {
        case ENetRole::ROLE_Authority:
            Role = FString("Authority");
            break;

        case ENetRole::ROLE_AutonomousProxy:
            Role = FString("Autonomous Proxy");
            break;

        case ENetRole::ROLE_SimulatedProxy:
            Role = FString("Simulated Proxy");
            break;
        
        default:
            Role = FString("None");
            break;
    }

    FString LocalRoleString = FString::Printf(TEXT("Local Role : %s"), *Role);
    FString RemoteRoleString = FString::Printf(TEXT("Remote Role : %s"), *Role);
    // SetDisplayText(LocalRoleString);
    // SetDisplayText(RemoteRoleString);
}