// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "Net/UnrealNetwork.h"

AUTCTFGameState::AUTCTFGameState(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	bOldSchool = false;
	bSecondHalf = false;
	bHalftime = false;
}

void AUTCTFGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTCTFGameState, bOldSchool);
	DOREPLIFETIME(AUTCTFGameState, bSecondHalf);
	DOREPLIFETIME(AUTCTFGameState, bHalftime);
	DOREPLIFETIME(AUTCTFGameState, FlagBases);
	DOREPLIFETIME(AUTCTFGameState, bPlayingAdvantage);
}

void AUTCTFGameState::SetMaxNumberOfTeams(int TeamCount)
{
	MaxNumberOfTeams = TeamCount;
	for (int TeamIdx=0; TeamIdx < MaxNumberOfTeams; TeamIdx++)
	{
		FlagBases.Add(NULL);
	}
}

void AUTCTFGameState::CacheFlagBase(AUTCTFFlagBase* BaseToCache)
{
	if (BaseToCache->MyFlag != NULL)
	{
		uint8 TeamNum = BaseToCache->MyFlag->GetTeamNum();
		if (TeamNum < FlagBases.Num())
		{
			FlagBases[TeamNum] = BaseToCache;
		}
		else
		{
			UE_LOG(UT,Warning,TEXT("Found a Flag Base with TeamNum of %i that was unexpected (%i)"), TeamNum, FlagBases.Num());
		}
	}
	else
	{
		UE_LOG(UT,Warning,TEXT("Found a Flag Base (%s) without a flag"), *GetNameSafe(BaseToCache));
	}

}

AUTTeamInfo* AUTCTFGameState::FindLeadingTeam()
{
	AUTTeamInfo* WinningTeam = NULL;
	bool bTied;

	if (Teams.Num() > 0)
	{
		WinningTeam = Teams[0];
		bTied = false;
		for (int i=1;i<Teams.Num();i++)
		{
			if (Teams[i]->Score == WinningTeam->Score)
			{
				bTied = true;
			}
			else if (Teams[i]->Score > WinningTeam->Score)
			{
				WinningTeam = Teams[i];
				bTied = false;
			}
		}

		if (bTied) WinningTeam = NULL;
	}

	return WinningTeam;	

}

FName AUTCTFGameState::GetFlagState(uint8 TeamNum)
{
	if (TeamNum < FlagBases.Num() && FlagBases[TeamNum] != NULL)
	{
		return FlagBases[TeamNum]->GetFlagState();
	}

	return NAME_None;
}

AUTPlayerState* AUTCTFGameState::GetFlagHolder(uint8 TeamNum)
{
	if (TeamNum < FlagBases.Num() && FlagBases[TeamNum] != NULL)
	{
		return FlagBases[TeamNum]->GetCarriedObjectHolder();
	}

	return NULL;
}

void AUTCTFGameState::ResetFlags()
{
	for (int i=0; i < FlagBases.Num(); i++)
	{
		if (FlagBases[i] != NULL)
		{
			FlagBases[i]->RecallFlag();
		}
	}

}

bool AUTCTFGameState::IsMatchInProgress() const
{
	FName MatchState = GetMatchState();
	if (MatchState == MatchState::InProgress || MatchState == MatchState::MatchIsInOvertime || MatchState == MatchState::MatchIsAtHalftime ||
			MatchState == MatchState::MatchEnteringHalftime || MatchState == MatchState::MatchExitingHalftime ||
			MatchState == MatchState::MatchEnteringSuddenDeath || MatchState == MatchState::MatchIsInSuddenDeath)
	{
		return true;
	}

	return false;
}

bool AUTCTFGameState::IsMatchInOvertime() const
{
	FName MatchState = GetMatchState();
	if (MatchState == MatchState::MatchIsInOvertime ||	MatchState == MatchState::MatchEnteringSuddenDeath || MatchState == MatchState::MatchIsInSuddenDeath)
	{
		return true;
	}

	return false;
}


bool AUTCTFGameState::IsMatchAtHalftime() const
{
	FName MatchState = GetMatchState();
	if (MatchState == MatchState::MatchIsAtHalftime || MatchState == MatchState::MatchEnteringHalftime || MatchState == MatchState::MatchExitingHalftime)
	{
		return true;
	}

	return false;
}

bool AUTCTFGameState::IsMatchInSuddenDeath() const
{
	FName MatchState = GetMatchState();
	if (MatchState == MatchState::MatchEnteringSuddenDeath || MatchState == MatchState::MatchIsInSuddenDeath)
	{
		return true;
	}

	return false;
}

FName AUTCTFGameState::OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle)
{
	if (IsMatchAtHalftime() || HasMatchEnded())
	{
		return FName(TEXT("FreeCam"));
	}

	return CurrentCameraStyle;
}

void AUTCTFGameState::OnHalftimeChanged()
{
}