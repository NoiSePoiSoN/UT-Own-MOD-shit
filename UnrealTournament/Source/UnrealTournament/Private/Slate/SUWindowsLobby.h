// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Panels/SUMidGameInfoPanel.h"
#include "Panels/SULobbyInfoPanel.h"
#include "Slate/SlateGameResources.h"


#if !UE_SERVER
class UNREALTOURNAMENT_API SUWindowsLobby : public SUTInGameMenu
{

protected:
	
	TSharedPtr<SButton> MatchButton;

	virtual void SetInitialPanel();

	virtual FText GetDisconnectButtonText() const;


	FText GetMatchButtonText() const;
	FString GetMatchCount() const;

	FReply MatchButtonClicked();
	
	virtual void BuildExitMenu(TSharedPtr <SComboButton> ExitButton, TSharedPtr<SVerticalBox> MenuSpace);
	virtual TSharedRef<SWidget> BuildBackground();
	virtual TSharedRef<SWidget> BuildOptionsSubMenu();

};

#endif