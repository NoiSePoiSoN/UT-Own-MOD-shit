// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUWindowsDesktop.h"
#include "SUWindowsStyle.h"
#include "SUTMenuBase.h"
#include "SUWDialog.h"
#include "SUWSystemSettingsDialog.h"
#include "SUWPlayerSettingsDialog.h"
#include "SUWSocialSettingsDialog.h"
#include "SUWWeaponConfigDialog.h"
#include "SUWControlSettingsDialog.h"
#include "SUWInputBox.h"
#include "SUWMessageBox.h"
#include "SUWScaleBox.h"
#include "SUWFriendsPopup.h"
#include "UTGameEngine.h"
#include "Panels/SUWServerBrowser.h"
#include "Panels/SUWStatsViewer.h"
#include "Panels/SUWCreditsPanel.h"
#include "UnrealNetwork.h"
#include "SUWProfileItemsDialog.h"

#if !UE_SERVER

void SUTMenuBase::CreateDesktop()
{
	bNeedsPlayerOptions = false;
	bNeedsWeaponOptions = false;
	bShowingFriends = false;
	TickCountDown = 0;
	
	LeftMenuBar = NULL;
	RightMenuBar = NULL;
	ChildSlot
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				BuildBackground()
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					SNew(SBox)
					.HeightOverride(64)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.HAlign(HAlign_Fill)
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.TileBar"))
							]
						]
						+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SBox)
									.HeightOverride(56)
									[
										// Left Menu
										SNew(SOverlay)
										+ SOverlay::Slot()
										.HAlign(HAlign_Left)
										.VAlign(VAlign_Center)
										[
											BuildDefaultLeftMenuBar()
										]
										+ SOverlay::Slot()
											.HAlign(HAlign_Right)
											.VAlign(VAlign_Center)
											[
												BuildDefaultRightMenuBar()
											]
									]
								]
								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SBox)
										.HeightOverride(8)
										[
											SNew(SCanvas)
										]
									]

							]
					]
				]

				+ SVerticalBox::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SAssignNew(Desktop, SOverlay)
						]
					]
			]
		];

	SetInitialPanel();
}

void SUTMenuBase::SetInitialPanel()
{
}


/****************************** [ Build Sub Menus ] *****************************************/

void SUTMenuBase::BuildLeftMenuBar() {}
void SUTMenuBase::BuildRightMenuBar() {}

TSharedRef<SWidget> SUTMenuBase::BuildDefaultLeftMenuBar()
{
	SAssignNew(LeftMenuBar, SHorizontalBox);
	if (LeftMenuBar.IsValid())
	{
		LeftMenuBar->AddSlot()
		.Padding(5.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
			.OnClicked(this, &SUTMenuBase::OnShowHomePanel)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(48)
					.HeightOverride(48)
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Home"))
					]
				]
			]
		];

		if (ShouldShowBrowserIcon())
		{
			LeftMenuBar->AddSlot()
			.Padding(5.0f, 0.0f, 0.0f, 0.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.OnClicked(this, &SUTMenuBase::OnShowServerBrowserPanel)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(48)
						.HeightOverride(48)
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Browser"))
						]
					]
				]
			];
		}


		BuildLeftMenuBar();
	}

	return LeftMenuBar.ToSharedRef();
}

FText SUTMenuBase::GetBrowserButtonText() const
{
	return PlayerOwner->GetWorld()->GetNetMode() == ENetMode::NM_Standalone ? NSLOCTEXT("SUWindowsDesktop","MenuBar_HUBS","Play Online") : NSLOCTEXT("SUWindowsDesktop","MenuBar_Browser","Server Browser");
}

TSharedRef<SWidget> SUTMenuBase::BuildDefaultRightMenuBar()
{
	// Build the Right Menu Bar
	if (!RightMenuBar.IsValid())
	{
		SAssignNew(RightMenuBar, SHorizontalBox);
	}
	else
	{
		RightMenuBar->ClearChildren();
	}

	if (RightMenuBar.IsValid())
	{

		BuildRightMenuBar();

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,5.0f,0.0f)
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			BuildOnlinePresence()
		];


		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,5.0f,0.0f)
		.AutoWidth()
		[
			BuildOptionsSubMenu()
		];

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,35.0f,0.0f)
		.AutoWidth()
		[
			BuildAboutSubMenu()
		];

		TSharedPtr<SComboButton> ExitButton = NULL;

		RightMenuBar->AddSlot()
		.Padding(0.0f,0.0f,5.0f,0.0f)
		.AutoWidth()
		[

			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(48)
					.HeightOverride(48)
					[
						SAssignNew(ExitButton, SComboButton)
						.HasDownArrow(false)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
						.ButtonContent()
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Exit"))
						]
					]
				]
			]
		];

		if (ExitButton.IsValid())
		{
			TSharedPtr<SVerticalBox> MenuSpace;
			SAssignNew(MenuSpace, SVerticalBox);
			if (MenuSpace.IsValid())
			{
				// Allow children to place menu options here....
				BuildExitMenu(ExitButton, MenuSpace);

				if (MenuSpace->NumSlots() > 0)
				{
					MenuSpace->AddSlot()
					.AutoHeight()
					[
						SNew(SBox)
						.HeightOverride(16)
						[
							SNew(SButton)
							.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button.Empty")
						]
					];

					MenuSpace->AddSlot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.Padding(FMargin(0.0f, 2.0f))
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.ContextMenu.Spacer"))
					];
				}

				MenuSpace->AddSlot()
				.AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
					.ContentPadding(FMargin(10.0f, 5.0f))
					.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_Exit_QuitGame", "Quit the Game"))
					.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
					.OnClicked(this, &SUTMenuBase::OnMenuConsoleCommand, FString(TEXT("quit")), ExitButton)
				];
			}

			ExitButton->SetMenuContent( MenuSpace.ToSharedRef());
		}



	}


	return RightMenuBar.ToSharedRef();
}

EVisibility SUTMenuBase::GetSocialBangVisibility() const
{
	if (PlayerOwner.IsValid())
	{
		if (PlayerOwner->IsPlayerShowingSocialNotification())
		{
			return EVisibility::Visible;
		}
	}
	return EVisibility::Collapsed;
}

TSharedRef<SWidget> SUTMenuBase::BuildOptionsSubMenu()
{
	
	TSharedPtr<SComboButton> DropDownButton = NULL;
	
	SNew(SHorizontalBox)
	+SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.WidthOverride(48)
			.HeightOverride(48)
			[
				SAssignNew(DropDownButton, SComboButton)
				.HasDownArrow(false)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.ContentPadding(FMargin(0.0f, 0.0f))
				.ButtonContent()
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Settings"))
				]
			]
		]
	];

	DropDownButton->SetMenuContent
	(
		SNew(SBorder)
		.BorderImage(SUWindowsStyle::Get().GetBrush("UT.ContextMenu.Background"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_PlayerSettings", "Player Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenPlayerSettings, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_SocialSettings", "Social Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenSocialSettings, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_WeaponSettings", "Weapon Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenWeaponSettings, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_HUDSettings", "HUD Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenHUDSettings, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_SystemSettings", "System Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenSystemSettings, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_ControlSettings", "Control Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenControlSettings, DropDownButton)
			]
	
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SNew(SImage)
				.Image(SUWindowsStyle::Get().GetBrush("UT.ContextMenu.Spacer"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_Options_ClearCloud", "Clear Game Settings"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::ClearCloud, DropDownButton)
			]
		]
	);

	MenuButtons.Add(DropDownButton);
	return DropDownButton.ToSharedRef();

}

TSharedRef<SWidget> SUTMenuBase::BuildAboutSubMenu()
{
	TSharedPtr<SComboButton> DropDownButton = NULL;

	SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.WidthOverride(48)
			.HeightOverride(48)
			[
				SAssignNew(DropDownButton, SComboButton)
				.HasDownArrow(false)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.ButtonContent()
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.About"))
				]
			]
		]
	];


	// NOTE: If you inline the setting of the menu content during the construction of the ComboButton
	// it doesn't assign the sharedptr until after the whole menu is constructed.  So if for example,
	// like these buttons here you need the value of the sharedptr it won't be available :(

	DropDownButton->SetMenuContent
	(
		SNew(SBorder)
		.BorderImage(SUWindowsStyle::Get().GetBrush("UT.ContextMenu.Background"))
		.Padding(0)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_About_TPSReport", "Third Party Software"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenTPSReport, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_About_Credits", "Credits"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OpenCredits, DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_About_UTSite", "UnrealTournament.com"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OnMenuHTTPButton, FString(TEXT("http://www.unrealtournament.com/")),DropDownButton)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_About_UTForums", "Forums"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::OnMenuHTTPButton, FString(TEXT("http://forums.unrealtournament.com/")), DropDownButton)
			]

	#if UE_BUILD_DEBUG
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
				.ContentPadding(FMargin(10.0f, 5.0f))
				.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_About_WR", "Widget Reflector"))
				.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
				.OnClicked(this, &SUTMenuBase::ShowWidgetReflector, DropDownButton)
			]
	#endif

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::Format(NSLOCTEXT("SUWindowsDesktop", "MenuBar_NetVersion", "Network Version: {Ver}"), FText::FromString(FString::Printf(TEXT("%i"), FNetworkVersion::GetLocalNetworkVersion()))))
				.TextStyle(SUWindowsStyle::Get(), "UT.Version.TextStyle")
			]
		]
	);

	MenuButtons.Add(DropDownButton);
	return DropDownButton.ToSharedRef();
}

void SUTMenuBase::BuildExitMenu(TSharedPtr<SComboButton> ExitButton, TSharedPtr<SVerticalBox> MenuSpace)
{
}


FReply SUTMenuBase::OpenPlayerSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);

	if (TickCountDown <= 0)
	{
		PlayerOwner->ShowContentLoadingMessage();
		bNeedsPlayerOptions = true;
		TickCountDown = 3;
	}

	return FReply::Handled();
}

FReply SUTMenuBase::OpenSocialSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	PlayerOwner->OpenDialog(SNew(SUWSocialSettingsDialog).PlayerOwner(PlayerOwner));
	return FReply::Handled();
}


FReply SUTMenuBase::OpenWeaponSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);

	if (TickCountDown <= 0)
	{
		PlayerOwner->ShowContentLoadingMessage();
		bNeedsWeaponOptions = true;
		TickCountDown = 3;
	}

	return FReply::Handled();
}

FReply SUTMenuBase::OpenSystemSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	PlayerOwner->OpenDialog(SNew(SUWSystemSettingsDialog).PlayerOwner(PlayerOwner).DialogTitle(NSLOCTEXT("SUWindowsDesktop","System","System Settings")));
	return FReply::Handled();
}

FReply SUTMenuBase::OpenControlSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	PlayerOwner->OpenDialog(SNew(SUWControlSettingsDialog).PlayerOwner(PlayerOwner).DialogTitle(NSLOCTEXT("SUWindowsDesktop","Controls","Control Settings")));
	return FReply::Handled();
}

FReply SUTMenuBase::OpenProfileItems()
{
	PlayerOwner->OpenDialog(SNew(SUWProfileItemsDialog).PlayerOwner(PlayerOwner));
	return FReply::Handled();
}

FReply SUTMenuBase::ClearCloud(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	if (PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->ClearProfileSettings();				
	}
	else
	{
		PlayerOwner->ShowMessage(NSLOCTEXT("SUWindowsMainMenu","NotLoggedInTitle","Not Logged In"), NSLOCTEXT("SuWindowsMainMenu","NoLoggedInMsg","You need to be logged in to clear your cloud settings!"), UTDIALOG_BUTTON_OK);
	}
	return FReply::Handled();
}


FReply SUTMenuBase::OpenTPSReport(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	PlayerOwner->OpenDialog(
							SNew(SUWMessageBox)
							.PlayerOwner(PlayerOwner)
							.DialogSize(FVector2D(0.7, 0.8))
							.bDialogSizeIsRelative(true)
							.DialogTitle(NSLOCTEXT("SUWindowsDesktop", "TPSReportTitle", "Third Party Software"))
							.MessageText(NSLOCTEXT("SUWindowsDesktop", "TPSReportText", "TPSText"))
							//.MessageTextStyleName("UWindows.Standard.Dialog.TextStyle.Legal")
							.MessageTextStyleName("UT.Common.NormalText")
							.ButtonMask(UTDIALOG_BUTTON_OK)
							);
	return FReply::Handled();
}

FReply SUTMenuBase::OpenCredits(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);

	TSharedPtr<class SUWCreditsPanel> CreditsPanel = PlayerOwner->GetCreditsPanel();
	if (CreditsPanel.IsValid())
	{
		ActivatePanel(CreditsPanel);
	}
	return FReply::Handled();
}

FReply SUTMenuBase::OnMenuHTTPButton(FString URL,TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	FString Error;
	FPlatformProcess::LaunchURL(*URL, NULL, &Error);
	if (Error.Len() > 0)
	{
		PlayerOwner->OpenDialog(
								SNew(SUWMessageBox)
								.PlayerOwner(PlayerOwner)
								.DialogTitle(NSLOCTEXT("SUWindowsDesktop", "HTTPBrowserError", "Error Launching Browser"))
								.MessageText(FText::FromString(Error))
								.ButtonMask(UTDIALOG_BUTTON_OK)
								);
	}
	return FReply::Handled();
}


FReply SUTMenuBase::OnShowStatsViewer()
{
	TSharedPtr<class SUWStatsViewer> StatsViewer = PlayerOwner->GetStatsViewer();
	if (StatsViewer.IsValid())
	{
		StatsViewer->SetQueryWindow(TEXT("alltime"));
		//StatsViewer->SetQueryWindow(TEXT("monthly"));
		//StatsViewer->SetQueryWindow(TEXT("weekly"));
		//StatsViewer->SetQueryWindow(TEXT("daily"));
		ActivatePanel(StatsViewer);
	}
	return FReply::Handled();
}

FReply SUTMenuBase::OnCloseClicked()
{
	return FReply::Handled();
}

TSharedRef<SWidget> SUTMenuBase::BuildOnlinePresence()
{
	if ( PlayerOwner->IsLoggedIn() )
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
				.OnClicked(this, &SUTMenuBase::OnOnlineClick)		
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.0f,0.0f,25.0f,0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(PlayerOwner->GetOnlinePlayerNickname()))
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.OnClicked(this, &SUTMenuBase::OnShowStatsViewer)
				.ToolTipText(NSLOCTEXT("ToolTips","TPMyStats","Show stats for this player."))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(48)
						.HeightOverride(48)
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Stats"))
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.OnClicked(this, &SUTMenuBase::OpenProfileItems)
				.ToolTipText(NSLOCTEXT("ToolTips", "TPMyItems", "Show collectable items you own."))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(48)
						.HeightOverride(48)
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.UpArrow"))
						]
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SMenuAnchor)
				.Method(EPopupMethod::UseCurrentWindow)
				[
					SNew(SButton)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
					.OnClicked(this, &SUTMenuBase::ToggleFriendsAndChat)
#if PLATFORM_LINUX
					.ToolTipText(NSLOCTEXT("ToolTips", "TPFriendsNotSupported", "Friends list not supported yet on this platform."))
#else
					.ToolTipText(NSLOCTEXT("ToolTips","TPFriends","Show / Hide your friends list."))
#endif
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SBox)
								.WidthOverride(48)
								.HeightOverride(48)
								[
									SNew(SImage)
									.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Online"))
								]
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Top)
							[
								SNew(SImage)
								.Visibility(this, &SUTMenuBase::GetSocialBangVisibility)
								.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.SocialBang"))
							]
						]
					]

				]
			];
	}
	else
	{
		return 	SNew(SButton)
		.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
		.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
		.OnClicked(this, &SUTMenuBase::OnOnlineClick)		
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(48)
				.HeightOverride(48)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.SignIn"))
				]

			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUWindowsDesktop","MenuBar_SignIn","Sign In"))
				.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
			]
		];
	}
}

void SUTMenuBase::OnOwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID)
{
	// Rebuilds the right menu bar
	BuildDefaultRightMenuBar();
}

FReply SUTMenuBase::OnOnlineClick()
{
	PlayerOwner->LoginOnline(TEXT(""),TEXT(""),false);
	return FReply::Handled();
}

FReply SUTMenuBase::OnShowServerBrowser(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	return OnShowServerBrowserPanel();
}


FReply SUTMenuBase::OnShowServerBrowserPanel()
{

	if (!PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->LoginOnline(TEXT(""), TEXT(""));
		return FReply::Handled();
	}
	

	TSharedPtr<class SUWServerBrowser> Browser = PlayerOwner->GetServerBrowser();
	if (Browser.IsValid())
	{
		ActivatePanel(Browser);
	}

	return FReply::Handled();
}

FReply SUTMenuBase::ToggleFriendsAndChat()
{
#if PLATFORM_LINUX
	// Need launcher so this doesn't work on linux right now
	return FReply::Handled();
#endif

	if (bShowingFriends)
	{
		Desktop->RemoveSlot(200);
		bShowingFriends = false;
	}
	else
	{
		TSharedPtr<SUWFriendsPopup> Popup = PlayerOwner->GetFriendsPopup();
		Popup->SetOnCloseClicked(FOnClicked::CreateSP(this, &SUTMenuBase::ToggleFriendsAndChat));

		if (Popup.IsValid())
		{
			Desktop->AddSlot(200)
				[
					Popup.ToSharedRef()
				];
			bShowingFriends = true;
		}
	}


	return FReply::Handled();
}

void SUTMenuBase::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// NOTE: TickCountDown is in frames, not time.  We have to delay 3 frames before opening a blocking menu to insure the
	// meesage gets displayed.

	if (TickCountDown > 0)
	{
		TickCountDown--;

		if (TickCountDown <= 0)
		{
			OpenDelayedMenu();
		}
	}
}

void SUTMenuBase::OpenDelayedMenu()
{
	if (bNeedsPlayerOptions)
	{
		PlayerOwner->OpenDialog(SNew(SUWPlayerSettingsDialog).PlayerOwner(PlayerOwner));
		bNeedsPlayerOptions = false;
		PlayerOwner->HideContentLoadingMessage();
	}
	else if (bNeedsWeaponOptions)
	{
		PlayerOwner->OpenDialog(SNew(SUWWeaponConfigDialog).PlayerOwner(PlayerOwner));
		bNeedsWeaponOptions = false;
		PlayerOwner->HideContentLoadingMessage();
	}
}

FReply SUTMenuBase::OnShowHomePanel()
{
	if (HomePanel.IsValid())
	{
		ActivatePanel(HomePanel);
	}
	else if (ActivePanel.IsValid())
	{
		DeactivatePanel(ActivePanel);
	}

	return FReply::Handled();
}

FReply SUTMenuBase::ShowWidgetReflector(TSharedPtr<SComboButton> MenuButton)
{
	ConsoleCommand(TEXT("WidgetReflector"));
	return FReply::Handled();
}

TSharedRef<SWidget> SUTMenuBase::BuildBackground()
{
	return SNullWidget::NullWidget;
}

FReply SUTMenuBase::OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent )
{
	if (InKeyboardEvent.GetKey() == EKeys::Escape)
	{
		if (bShowingFriends)
		{
			ToggleFriendsAndChat();
		}
	}

	return SUWindowsDesktop::OnKeyUp(MyGeometry, InKeyboardEvent);
}

FReply SUTMenuBase::OpenHUDSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	PlayerOwner->ShowHUDSettings();
	return FReply::Handled();
}

#endif