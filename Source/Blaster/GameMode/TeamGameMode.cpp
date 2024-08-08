// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"

void ATeamGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	if (BGameState)
	{
		for (auto PlayerState : BGameState->PlayerArray)
		{
			ABlasterPlayerState* BPState = Cast<ABlasterPlayerState>(PlayerState.Get());
			if (BPState && BPState->GetTeam() == ETeam::ETeam_NoTeam)
			{
				AssignTeam(BPState);
			}
		}
	}
}

void ATeamGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABlasterPlayerState* BPState = NewPlayer->GetPlayerState<ABlasterPlayerState>();
	if (BPState && BPState->GetTeam() == ETeam::ETeam_NoTeam)
	{
		AssignTeam(BPState);
	}
}

void ATeamGameMode::Logout(AController* ExitingPlayer)
{
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* BPState = ExitingPlayer->GetPlayerState<ABlasterPlayerState>();

	if (BGameState && BPState)
	{
		switch (BPState->GetTeam())
		{
		case ETeam::ETeam_BlueTeam:
			if (BGameState->BlueTeam.Contains(BPState))
			{
				BGameState->BlueTeam.Remove(BPState);
			}
			break;
		case ETeam::ETeam_RedTeam:
			if (BGameState->RedTeam.Contains(BPState))
			{
				BGameState->RedTeam.Remove(BPState);
			}
			break;
		}
	}
}

float ATeamGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	ABlasterPlayerState* AttackerPState = Attacker->GetPlayerState<ABlasterPlayerState>();
	ABlasterPlayerState* VictimPState = Victim->GetPlayerState<ABlasterPlayerState>();

	if (AttackerPState == nullptr || VictimPState == nullptr)
	{
		return BaseDamage;
	}

	if (VictimPState->GetTeam() == AttackerPState->GetTeam())
	{
		return 0.f;
	}

	return BaseDamage;
}

void ATeamGameMode::AssignTeam(ABlasterPlayerState* PlayerState)
{
	ABlasterGameState* BGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

	if (BGameState->BlueTeam.Num() > BGameState->RedTeam.Num())
	{
		BGameState->RedTeam.AddUnique(PlayerState);
		PlayerState->SetTeam(ETeam::ETeam_RedTeam);
	}
	else
	{
		BGameState->BlueTeam.AddUnique(PlayerState);
		PlayerState->SetTeam(ETeam::ETeam_BlueTeam);
	}
}