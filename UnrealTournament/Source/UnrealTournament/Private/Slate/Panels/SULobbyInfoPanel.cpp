
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Slate/SlateGameResources.h"
#include "../SUWPanel.h"
#include "../SUWindowsStyle.h"
#include "SUMidGameInfoPanel.h"
#include "SULobbyMatchSetupPanel.h"
#include "SULobbyInfoPanel.h"
#include "UTLobbyPC.h"
#include "Engine/UserInterfaceSettings.h"
#include "../Public/UTLobbyHUD.h"



#if !UE_SERVER

void SULobbyInfoPanel::ConstructPanel(FVector2D ViewportSize)
{
	SUInGameHomePanel::ConstructPanel(ViewportSize);

	Tag = FName(TEXT("LobbyInfoPanel"));

	ChatDestination = ChatDestinations::Global;
	LastMatchState = ELobbyMatchState::Setup;

	AUTLobbyPlayerState* PlayerState = Cast<AUTLobbyPlayerState>(GetOwnerPlayerState());
	PlayerState->CurrentMatchChangedDelegate.BindSP(this, &SULobbyInfoPanel::OwnerCurrentMatchChanged);
	bShowingMatchDock = PlayerState->CurrentMatch == NULL;

	if (MenuArea.IsValid())
	{
		MenuArea->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.HeightOverride(64)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(8)
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.Bar"))
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(56)
					[
						// Buttom Menu Bar
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Fill)
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.TileBar"))
							]
						]
						+SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.Padding(10.0f,0.0f,0.0f,0.0f)
							[
								SNew(STextBlock)
								.Text(this, &SULobbyInfoPanel::GetServerNameText)
								.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
							]
						]
						+SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Right)
							[
								BuildMatchMenu()
							]
						]
					]
				]
			]
		];
	}



	if (ChatArea.IsValid())
	{
		ChatArea->AddSlot()
		.AutoHeight()
		[
			SAssignNew(MatchArea, SVerticalBox)
		];

		BuildMatchPanel();

		ChatArea->AddSlot()
		.FillHeight(1.0)
		[
			BuildChatArea()
		];
	}


}

AUTPlayerState* SULobbyInfoPanel::GetOwnerPlayerState()
{
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerOwner->PlayerController);
	if (PC) 
	{
		return Cast<AUTPlayerState>(PC->PlayerState);
	}
	return NULL;
}

void SULobbyInfoPanel::BuildMatchPanel()
{
	// Clear out any existing panels
	MatchArea->ClearChildren();

	AUTLobbyPlayerState* PlayerState = Cast<AUTLobbyPlayerState>(GetOwnerPlayerState());
	if (MatchArea.IsValid() && PlayerState)
	{
		if (bShowingMatchDock)
		{
			MatchArea->AddSlot()
			.AutoHeight()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()		
						.HAlign(HAlign_Fill)
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UWindows.Standard.DarkBackground"))
							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()		
								.AutoHeight()
								[
									SNew(SScrollBox)
									.Orientation(Orient_Horizontal)
									+SScrollBox::Slot()
									[
										SAssignNew(MatchPanelDock, SHorizontalBox)
										+SHorizontalBox::Slot()
										.Padding(5.0f,0.0f,5.0f,0.0f)
										[
											BuildNewMatchButton()
										]
									]
								]
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(MatchPlayerList, STextBlock)
									.Text(this, &SULobbyInfoPanel::GetMatchPlayerListText)
									.TextStyle(SUWindowsStyle::Get(), "UT.HUB.PlayerListText")
								]

							]
						]			
				];
		}

		else if (PlayerState->CurrentMatch)

		// We need to show the match setup panel.  Hosts will have an interactive panel, non-hosts will just get an info dump

		{
			MatchArea->AddSlot()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()		
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Fill)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.Bar"))
						]
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()		
							.AutoHeight()
							[
								SNew(SULobbyMatchSetupPanel)
								.PlayerOwner(PlayerOwner)
								.MatchInfo(PlayerState->CurrentMatch)
								.bIsHost(PlayerState->CurrentMatch->OwnerId == PlayerState->UniqueId)
							]
						]
					]			
				];
		}
	}
}

TSharedRef<SWidget> SULobbyInfoPanel::BuildMatchMenu()
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding(0.0f,0.0f,5.0f,0.0f)
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
			.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
			.OnClicked(this, &SULobbyInfoPanel::ReadyButtonClicked)
			.IsEnabled(this, &SULobbyInfoPanel::GetReadyButtonEnabled)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SULobbyInfoPanel::GetReadyButtonText)
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
				]
			]
		]

		+SHorizontalBox::Slot()
		.Padding(0.0f,0.0f,5.0f,0.0f)
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
			.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
			.OnClicked(this, &SULobbyInfoPanel::MatchButtonClicked)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SULobbyInfoPanel::GetMatchButtonText)
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
				]
			]
		];
}

FReply SULobbyInfoPanel::MatchButtonClicked()
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController)
	{
		AUTLobbyPlayerState* PlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
		if (PlayerState)
		{
			PlayerState->MatchButtonPressed();
		}
	}

	return FReply::Handled();
}

bool SULobbyInfoPanel::GetReadyButtonEnabled() const
{
	AUTLobbyPlayerState* LPS = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
	if (LPS && LPS->CurrentMatch && LPS->CurrentMatch->CurrentRuleset.IsValid() && !PlayerOwner->IsDownloadInProgress())
	{
		return true;
	}
	return false;
}

FReply SULobbyInfoPanel::ReadyButtonClicked()
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
	{
		AUTLobbyPlayerState* LPS = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
		AUTLobbyPC* PC = Cast<AUTLobbyPC>(PlayerOwner->PlayerController);

		if (LPS && LPS->CurrentMatch)
		{
			if (LPS->CurrentMatch->OwnerId == LPS->UniqueId)	// Host
			{
					AUTLobbyMatchInfo* MatchInfo = LPS->CurrentMatch;

					if (MatchInfo->CurrentState == ELobbyMatchState::WaitingForPlayers)
					{
						MatchInfo->ServerStartMatch();	
					}
					else
					{
						MatchInfo->ServerAbortMatch();
					}
			}
			else
			{
				if (LPS->CurrentMatch->CurrentState == ELobbyMatchState::WaitingForPlayers)
				{
					PC->ServerSetReady(!LPS->bReadyToPlay);
				}
			
			}
		}
	}
	return FReply::Handled();
}


FText SULobbyInfoPanel::GetReadyButtonText() const
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
	{
		AUTLobbyPlayerState* LPS = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
		if (LPS)
		{
			if (LPS->CurrentMatch)
			{
				if (LPS->CurrentMatch->OwnerId == LPS->UniqueId)
				{
					return (LPS->CurrentMatch->CurrentState == ELobbyMatchState::Launching ? NSLOCTEXT("Gerneric","AbortMatch","ABORT MATCH") : NSLOCTEXT("Gerneric","StartMatch","START MATCH"));
				}

				return (LPS->bReadyToPlay) ? NSLOCTEXT("Gerneric","NotReady","NOT READY") : NSLOCTEXT("Gerneric","Ready","READY");
			}
		}
	}
	return FText::GetEmpty();
}

FText SULobbyInfoPanel::GetMatchButtonText() const
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
	{
		AUTLobbyPlayerState* LPS = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
		if (LPS)
		{
			if (LPS->CurrentMatch)
			{
				return NSLOCTEXT("Gerneric","LeaveMatch","LEAVE MATCH");
			}
			return NSLOCTEXT("Gerneric","NewMatch","NEW MATCH");
	
		}
	}
	return FText::GetEmpty();
}

bool SULobbyInfoPanel::AlreadyTrackingMatch(AUTLobbyMatchInfo* TestMatch)
{
	for (int32 i=0;i<MatchData.Num();i++)
	{
		if (MatchData[i].Get()->Info == TestMatch)
		{
			return true;
		}
	}

	return false;

}
void SULobbyInfoPanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	AUTLobbyPlayerState* PlayerState = Cast<AUTLobbyPlayerState>(GetOwnerPlayerState());
	if (!PlayerState) return;	// Quick out if we don't have an owner player.

	if (bShowingMatchDock)
	{
		// Look to see if we have been waiting for a valid GameModeData to replicate.  If we have, open the right panel.
		if (PlayerState && PlayerState->CurrentMatch && PlayerState->CurrentMatch->MatchIsReadyToJoin(PlayerState))
		{
			ShowMatchSetupPanel();
			return;
		}

		// We are showing the dock, look to see if new matches to join have been added to the queue and if they
		// have, add them

		AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
		if (LobbyGameState)
		{
			bool bDockRequiresUpdate = false;

			// Expire out any old matches that are no longer useful.

			for (int32 i=0;i<MatchData.Num();i++)
			{
				if (MatchData[i]->Info.IsValid())
				{
					if (!LobbyGameState->IsMatchStillValid(MatchData[i]->Info.Get()) || !MatchData[i]->Info->ShouldShowInDock())
					{
						bDockRequiresUpdate = true;
					}
				}
			}

			// Look for any matches not in the dock
			for (int32 i=0;i<LobbyGameState->AvailableMatches.Num();i++)
			{
				AUTLobbyMatchInfo* Match = LobbyGameState->AvailableMatches[i];
				if (Match && Match->CurrentState != ELobbyMatchState::Dead && Match->CurrentState != ELobbyMatchState::Recycling)
				{
					if (Match->ShouldShowInDock() && !AlreadyTrackingMatch(LobbyGameState->AvailableMatches[i]))
					{
						bDockRequiresUpdate = true;
					}
				}
			}

			if (bDockRequiresUpdate)
			{
				MatchData.Empty();
				MatchPanelDock->ClearChildren();


				MatchPanelDock->AddSlot()
				[
					BuildNewMatchButton()
				];


				if ( LobbyGameState->AvailableMatches.Num() >0 )
				{
					for (int32 i=0;i<LobbyGameState->AvailableMatches.Num();i++)
					{
						if (LobbyGameState->AvailableMatches[i] && LobbyGameState->AvailableMatches[i]->ShouldShowInDock())
						{
							TSharedPtr<SUMatchPanel> MP;
							TWeakObjectPtr<AUTLobbyMatchInfo> MatchInfo = LobbyGameState->AvailableMatches[i];
							MatchPanelDock->AddSlot()
							[
								SAssignNew(MP,SUMatchPanel)
								.MatchInfo(MatchInfo)
								.PlayerOwner(PlayerOwner)
							];

							MatchData.Add( FMatchData::Make(LobbyGameState->AvailableMatches[i], MP));
						}
					}
				}
			}
		}
	}

	WatchedMatch.Reset();
	for (int32 i=0;i<MatchData.Num();i++)
	{
		if (MatchData[i]->Panel->IsHovered())
		{
			WatchedMatch = MatchData[i]->Info;
			break;	
		}
	}

	UpdateUserList();
	UpdateChatText();
}

TSharedRef<SWidget> SULobbyInfoPanel::BuildDefaultMatchMessage()
{
	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
	if (!LobbyGameState || LobbyGameState->AvailableMatches.Num() <= 0)
	{
		return SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text( NSLOCTEXT("LobbyMatches","NoMatches","There are currently no matches, start one!"))
			.TextStyle(SUWindowsStyle::Get(), "UWindows.Lobby.MatchButton.Action.TextStyle")
		];
	}
	
	return SNew(SCanvas);

}
void SULobbyInfoPanel::OwnerCurrentMatchChanged(AUTLobbyPlayerState* LobbyPlayerState)
{
	if (LobbyPlayerState->CurrentMatch) 
	{
		if (ChatDestination != ChatDestinations::Match) ChatDestination = ChatDestinations::Match;
	}
	else
	{
		if (ChatDestination == ChatDestinations::Match) ChatDestination = ChatDestinations::Global;
	}

	BuildChatDestinationMenu();

	if (LobbyPlayerState->CurrentMatch && LobbyPlayerState->CurrentMatch->MatchIsReadyToJoin(LobbyPlayerState))
	{
		if (bShowingMatchDock)
		{
			ShowMatchSetupPanel();
		}
	}
	else
	{
		if (!bShowingMatchDock)
		{
			ShowMatchDock();		
		}
	}
}

void SULobbyInfoPanel::BuildChatDestinationMenu()
{
	if (ChatMenu.IsValid())
	{
		ChatMenu->ClearChildren();
		ChatMenu->AddSlot()
		.AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
			.ContentPadding(FMargin(10.0f, 5.0f))
			.Text(NSLOCTEXT("Chat", "ChatDestination_HUB", "Everyone"))
			.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
			.OnClicked(this, &SUInGameHomePanel::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Global)
		];
	
		AUTLobbyPlayerState* PS = Cast<AUTLobbyPlayerState>(GetOwnerPlayerState());
		if (PS && PS->CurrentMatch)
		{
			ChatMenu->AddSlot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("Chat", "ChatDestination_MATCH", "Players in Match"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUInGameHomePanel::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Match)
			];
		}

		if (PlayerOwner->IsLoggedIn())
		{
			ChatMenu->AddSlot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("Chat", "ChatDestination_Friends", "Friends"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUInGameHomePanel::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Friends)
			];
		}
	}
}


void SULobbyInfoPanel::ShowMatchSetupPanel()
{
	bShowingMatchDock = false;
	BuildMatchPanel();
}

void SULobbyInfoPanel::ShowMatchDock()
{
	bShowingMatchDock = true;
	MatchData.Empty();
	BuildMatchPanel();
}

TSharedRef<SWidget> SULobbyInfoPanel::BuildChatArea()
{
	SAssignNew( Splitter, SSplitter )
	.Orientation(Orient_Horizontal)

	// Left side of the spliter is the chat, right side is the user list.

	+SSplitter::Slot()
	.Value(0.85f)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()		
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		.Padding(20,36,8,16)
		[
			SAssignNew(ChatScroller, SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SDPIScaler)
					.DPIScale(this, &SULobbyInfoPanel::GetReverseScale)
					[
						SAssignNew(ChatDisplay, SRichTextBlock)
						.TextStyle(SUWindowsStyle::Get(),"UWindows.Chat.Text.Global")
						.Justification(ETextJustify::Left)
						.DecoratorStyleSet( &SUWindowsStyle::Get() )
						.AutoWrapText( true )
					]
				]
			]
		]
	]

	// Right side of the splitter is the player list

	+SSplitter::Slot()
	.Value(0.15f)
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SImage)
			.Image(SUWindowsStyle::Get().GetBrush("UT.Vertical.Playerlist.Background"))
		]
		+SOverlay::Slot()
		.Padding(10.0f)
		[
			SNew(SBox)
			.WidthOverride(200)
			[
				SAssignNew( UserListView, SListView< TSharedPtr<FPlayerData> > )
				// List view items are this tall
				.ItemHeight(24)
				// Tell the list view where to get its source data
				.ListItemsSource( &UserList)
				// When the list view needs to generate a widget for some data item, use this method
				.OnGenerateRow( this, &SULobbyInfoPanel::OnGenerateWidgetForList)
				.SelectionMode(ESelectionMode::Single)
			]
		]
	];

	return Splitter.ToSharedRef();
}

TSharedRef<ITableRow> SULobbyInfoPanel::OnGenerateWidgetForList( TSharedPtr<FPlayerData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	FString PlayerName = InItem.IsValid() && InItem->PlayerState.IsValid() ? InItem->PlayerState->PlayerName : TEXT("???");
	return SNew(STableRow<TSharedPtr<FSimpleListData>>, OwnerTable)
		.Style(SUWindowsStyle::Get(),"UT.HUB.PlayerList")
		.Padding(5)
		[
			SNew(STextBlock)
			.Text(FText::FromString(PlayerName))
			.TextStyle(SUWindowsStyle::Get(),"UT.HUB.PlayerListText")
		];
}

void SULobbyInfoPanel::UpdateUserList()
{
	// Grab a list of users
	AUTGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GS)
	{
		for (int32 i=0; i<GS->PlayerArray.Num();i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
			if (PS != NULL)
			{
				bool bFound = false;
				// Look to see if this player is in the list.
				for (int32 j=0; j < UserList.Num(); j++)
				{
					if (UserList[j]->PlayerState.IsValid() && UserList[j]->PlayerState.Get() == PS)
					{
						bFound = true;
					}
				}

				if (!bFound)
				{
					UserList.Add(FPlayerData::Make(PS));
				}
			}
		}

		for (auto It = UserList.CreateIterator(); It; It++)
		{
			TSharedPtr<FPlayerData> Data = *It;
			if (GS->PlayerArray.Find(Data->PlayerState.Get()) == INDEX_NONE)
			{
				UserList.Remove(Data);
			}
		}

		UserListView->RequestListRefresh();
	}
}

void SULobbyInfoPanel::UpdateChatText()
{
	if (PlayerOwner->ChatArchive.Num() != LastChatCount)
	{
		LastChatCount = PlayerOwner->ChatArchive.Num();

		FString RichText = TEXT("");
		for (int32 i=0;i<PlayerOwner->ChatArchive.Num(); i++)
		{
			TSharedPtr<FStoredChatMessage> Msg = PlayerOwner->ChatArchive[i];
			if (i>0) RichText += TEXT("\n");
			if (Msg->Type != ChatDestinations::MOTD)
			{
				FString Style;

				if (Msg->Type == ChatDestinations::Friends)
				{
					Style = TEXT("UWindows.Chat.Text.Friends");
				}
				else if (Msg->Type == ChatDestinations::System || Msg->Type == ChatDestinations::MOTD)
				{
					Style = TEXT("UWindows.Chat.Text.Admin");
				}
				else if (Msg->Type == ChatDestinations::Lobby)
				{
					Style = TEXT("UWindows.Chat.Text.Lobby");
				}
				else if (Msg->Type == ChatDestinations::Match)
				{
					Style = TEXT("UWindows.Chat.Text.Match");
				}
				else if (Msg->Type == ChatDestinations::Local)
				{
					Style = TEXT("UWindows.Chat.Text.Local");
				}
				else if (Msg->Type == ChatDestinations::Team)
				{
					if (Msg->Color.R > Msg->Color.B)
					{
						Style = TEXT("UWindows.Chat.Text.Team.Red");;
					}
					else
					{
						Style = TEXT("UWindows.Chat.Text.Team.Blue");;
					}
				}
				else
				{
					Style = TEXT("UWindows.Chat.Text.Global");
				}

				RichText += FString::Printf(TEXT("<%s>[%s]%s</>"), *Style, *(GetChatDestinationTag(Msg->Type).ToString()), *Msg->Message);
			}
			else
			{
				RichText += Msg->Message;
			}
		}

		ChatDisplay->SetText(FText::FromString(RichText));
		ChatScroller->SetScrollOffset(290999);
	}
}

float SULobbyInfoPanel::GetReverseScale() const
{
	if (PlayerOwner.IsValid() && PlayerOwner->ViewportClient)
	{
		FVector2D ViewportSize;
		PlayerOwner->ViewportClient->GetViewportSize(ViewportSize);

		float AntiScale = 1.0f / GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));

		return AntiScale;
	}
	return 1.0f;
}

void SULobbyInfoPanel::ChatTextCommited(const FText& NewText, ETextCommit::Type CommitType)
{
	FString Cmd = NewText.ToString();
	if (Cmd.Left(1) == TEXT("@"))
	{
		if (Cmd.ToLower() == TEXT("@debugmatch"))
		{
			PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT(" " ), FLinearColor::White);
			AUTLobbyGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>();
			if (GS)
			{
				if (GS->AvailableMatches.Num() > 0)
				{
					for (int32 i=0; i < GS->AvailableMatches.Num(); i++)
					{
						if (GS->AvailableMatches[i])
						{
							FText Text = FText::Format(NSLOCTEXT("UTLOBBYHUD","LobbyDebugA","Lobby {0} - {1}"), FText::AsNumber(i), GS->AvailableMatches[i]->GetDebugInfo());
							PlayerOwner->SaveChat(FName(TEXT("Debug")), Text.ToString(), FLinearColor::White);
							if (GS->AvailableMatches[i]->Players.Num() > 0)
							{
								FString Players = TEXT("");
								for (int32 j=0; j < GS->AvailableMatches[i]->Players.Num(); j++)
								{
									AUTLobbyPlayerState* PS = GS->AvailableMatches[i]->Players[j].Get();
									Players += Players == TEXT("") ? PS->PlayerName : FString::Printf(TEXT(", %s"), *PS->PlayerName);
								}

								PlayerOwner->SaveChat(FName(TEXT("Debug")), Players, FLinearColor::White);

							}
							else
							{
								PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT("No Active Players"), FLinearColor::White);
							}

							if (GS->AvailableMatches[i]->PlayersInMatchInstance.Num() > 0)
							{
								FString Players = TEXT("");
								for (int32 j=0; j < GS->AvailableMatches[i]->PlayersInMatchInstance.Num(); j++)
								{
									FString PlayerName = GS->AvailableMatches[i]->PlayersInMatchInstance[j].PlayerName;
									Players += Players == TEXT("") ?  PlayerName : FString::Printf(TEXT(", %s"), *PlayerName);
								}
								PlayerOwner->SaveChat(FName(TEXT("Debug")), Players, FLinearColor::Gray);

							}
							else
							{
								PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT("No Active Players"), FLinearColor::Gray);
							}

						}
					}
				}
				else
				{
					PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT("No Matches to list!"), FLinearColor::White);
				}
			}
			PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT(" " ), FLinearColor::White);
			ChatText->SetText(FText::GetEmpty());
			return;
		}

		if (Cmd.ToLower() == TEXT("@debugrules"))
		{
			PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT(" " ), FLinearColor::White);
			AUTLobbyGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>();
			if (GS)
			{
				for (int32 i=0; i < GS->AvailableGameRulesets.Num(); i++)
				{
					FString X;
					TWeakObjectPtr<AUTReplicatedGameRuleset> R = GS->AvailableGameRulesets[i];
					for (int32 j=0;j < R->Categories.Num();j++)
					{
						X = X + R->Categories[j].ToString() + TEXT(", ");
					}
					FText TextRule = FText::Format(NSLOCTEXT("UTLOBBYHUD", "LobbyDebugRule", " Rule {0} = {1} {2} Cats= {3} "), FText::AsNumber(i), FText::FromString(R->UniqueTag), FText::FromString(R->Title), FText::FromString(X));
					PlayerOwner->SaveChat(FName(TEXT("Debug")), TextRule.ToString(), FLinearColor::White);
				}
			}
		}
		if (Cmd.ToLower() == TEXT("@debugplayers"))
		{
			PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT(" " ), FLinearColor::White);
			AUTLobbyGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>();
			if (GS)
			{
				for (int32 i=0;i < GS->PlayerArray.Num();i++)
				{
					AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
					if (PS)
					{
						FText Text3 = FText::Format(NSLOCTEXT("UTLOBBYHUD", "LobbyDebugG", " Player {0} = {1} - {2}"), FText::AsNumber(i), FText::FromString(PS->PlayerName), FText::AsNumber(PS->AverageRank));
						PlayerOwner->SaveChat(FName(TEXT("Debug")), Text3.ToString(), FLinearColor::White);
					}
				}

				for (int32 i=0; i < GS->AvailableMatches.Num(); i++)
				{
					if (GS->AvailableMatches[i])
					{
						if (GS->AvailableMatches[i]->PlayersInMatchInstance.Num() > 0)
						{
							for (int32 j=0; j < GS->AvailableMatches[i]->PlayersInMatchInstance.Num(); j++)
							{
								FText Text4 = FText::Format(NSLOCTEXT("UTLOBBYHUD", "LobbyDebugG", " iPlayer {0} = {1} - {2}"), 
									FText::FromString(GS->AvailableMatches[i]->PlayersInMatchInstance[j].PlayerName),
									FText::FromString(GS->AvailableMatches[i]->PlayersInMatchInstance[j].PlayerID.ToString()),
									FText::AsNumber(GS->AvailableMatches[i]->PlayersInMatchInstance[j].PlayerScore));
								PlayerOwner->SaveChat(FName(TEXT("Debug")), Text4.ToString(), FLinearColor::White);
							}
						}
					}
				}
			}
			PlayerOwner->SaveChat(FName(TEXT("Debug")), TEXT(" " ), FLinearColor::White);
			ChatText->SetText(FText::GetEmpty());
			return;
		}
	}

	SUInGameHomePanel::ChatTextCommited(NewText, CommitType);
}

TSharedRef<SWidget> SULobbyInfoPanel::BuildNewMatchButton()
{
	return SNew(SOverlay)
	+SOverlay::Slot()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)						// First the overlaid box that controls everything....
			.HeightOverride(192)
			.WidthOverride(192)
			[
				SNew(SButton)
				.OnClicked(this, &SULobbyInfoPanel::MatchButtonClicked)
				.ButtonStyle(SUWindowsStyle::Get(),"UWindows.Lobby.MatchButton")
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.Padding(5.0,5.0,5.0,0.0)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(SBox)						// First the overlaid box that controls everything....
							.HeightOverride(145)
							.WidthOverride(145)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.Padding(5.0,0.0,5.0,0.0)
								.VAlign(VAlign_Fill)
								.HAlign(HAlign_Center)
								[
									SNew(SRichTextBlock)
									.TextStyle(SUWindowsStyle::Get(),"UT.Common.BoldText")
									.Justification(ETextJustify::Center)
									.DecoratorStyleSet( &SUWindowsStyle::Get() )
									.AutoWrapText( true )
									.Text(NSLOCTEXT("Generic","NewMatchBadge","START\nNEW\nMATCH"))
								]
							]
						]
					]
				]
			]
		]
			
	];
}

FText SULobbyInfoPanel::GetMatchPlayerListText() const
{
	if (WatchedMatch.IsValid())
	{
		FString PlayerList = TEXT("");
		if (WatchedMatch->CurrentState == ELobbyMatchState::InProgress)
		{
			for (int32 i=0;i<WatchedMatch->PlayersInMatchInstance.Num();i++)
			{
				if (WatchedMatch->PlayersInMatchInstance[i].PlayerName != TEXT(""))
				{
					FString PlayerEntry = FString::Printf( TEXT("%s (%i)"), *WatchedMatch->PlayersInMatchInstance[i].PlayerName, int32(WatchedMatch->PlayersInMatchInstance[i].PlayerScore));
					PlayerList += PlayerList == TEXT("") ? PlayerEntry : TEXT(", ") + PlayerEntry;
				}
			}
		}
		else if (WatchedMatch->CurrentState == ELobbyMatchState::WaitingForPlayers || WatchedMatch->CurrentState == ELobbyMatchState::Launching)
		{
			for (int32 i=0;i<WatchedMatch->Players.Num();i++)
			{
				if (WatchedMatch->Players[i].IsValid() && WatchedMatch->Players[i]->PlayerName != TEXT(""))
				{
					PlayerList += PlayerList == TEXT("") ? WatchedMatch->Players[i]->PlayerName : TEXT(", ") + WatchedMatch->Players[i]->PlayerName;
				}
			}

		}

		if (PlayerList != TEXT(""))
		{
			return FText::Format( NSLOCTEXT("SULobbytInfoPanel","MatchPlayerListFormat","Players In Match: {0}"), FText::FromString(PlayerList));
		}
	}

	return FText::GetEmpty();
}

FText SULobbyInfoPanel::GetServerNameText() const
{
	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		return FText::Format(NSLOCTEXT("SULobbyInfoPanel","ServerNameFormat","Connected To: {0}"), FText::FromString(LobbyGameState->ServerName));
	}

	return FText::GetEmpty();
}


#endif