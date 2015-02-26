// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SUWSystemSettingsDialog.h"
#include "SUWindowsStyle.h"
#include "SUTUtils.h"
#include "UTPlayerInput.h"
#include "Scalability.h"
#include "UTWorldSettings.h"
#include "UTGameEngine.h"

#if !UE_SERVER

SVerticalBox::FSlot& SUWSystemSettingsDialog::AddSectionHeader(const FText& SectionDesc)
{
	return SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		.Padding(FMargin(0.0f, 15.0f, 0.0f, 15.0f))
		[
			SNew(STextBlock)
			.Text(SectionDesc)
			.TextStyle(SUWindowsStyle::Get(),"UT.Common.BoldText")
		];

}

SVerticalBox::FSlot& SUWSystemSettingsDialog::AddGeneralScalabilityWidget(const FString& Desc, TSharedPtr< SComboBox< TSharedPtr<FString> > >& ComboBox, TSharedPtr<STextBlock>& SelectedItemWidget, void (SUWSystemSettingsDialog::*SelectionFunc)(TSharedPtr<FString>, ESelectInfo::Type), int32 SettingValue, const TAttribute<FText>& TooltipText)
{
	return SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.AutoHeight()
		.Padding(FMargin(10.0f, 15.0f, 10.0f, 5.0f))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(650)
				[
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					.Text(Desc)
					.ToolTip(SUTUtils::CreateTooltip(TooltipText))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(ComboBox, SComboBox< TSharedPtr<FString> >)
				.InitiallySelectedItem(GeneralScalabilityList[SettingValue])
				.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
				.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
				.OptionsSource(&GeneralScalabilityList)
				.OnGenerateWidget(this, &SUWDialog::GenerateStringListWidget)
				.OnSelectionChanged(this, SelectionFunc)
				.Content()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(10.0f, 0.0f, 10.0f, 0.0f)
					[
						SAssignNew(SelectedItemWidget, STextBlock)
						.Text(*GeneralScalabilityList[SettingValue].Get())
						.TextStyle(SUWindowsStyle::Get(),"UT.Common.NormalText.Black")
					]
				]
			]
		];
}

SVerticalBox::FSlot& SUWSystemSettingsDialog::AddAAModeWidget(const FString& Desc, TSharedPtr< SComboBox< TSharedPtr<FString> > >& ComboBox, TSharedPtr<STextBlock>& SelectedItemWidget, void (SUWSystemSettingsDialog::*SelectionFunc)(TSharedPtr<FString>, ESelectInfo::Type), int32 SettingValue, const TAttribute<FText>& TooltipText)
{
	return SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.AutoHeight()
		.Padding(FMargin(10.0f, 15.0f, 10.0f, 5.0f))
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(650)
				[
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					.Text(Desc)
					.ToolTip(SUTUtils::CreateTooltip(TooltipText))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(ComboBox, SComboBox< TSharedPtr<FString> >)
				.InitiallySelectedItem(AAModeList[SettingValue])
				.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
				.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
				.OptionsSource(&AAModeList)
				.OnGenerateWidget(this, &SUWDialog::GenerateStringListWidget)
				.OnSelectionChanged(this, SelectionFunc)
				.Content()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(10.0f, 0.0f, 10.0f, 0.0f)
					[
						SAssignNew(SelectedItemWidget, STextBlock)
						.Text(*AAModeList[SettingValue].Get())
						.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText.Black")
					]
				]
			]
		];
}

SVerticalBox::FSlot& SUWSystemSettingsDialog::AddGeneralSliderWidget(const FString& Desc, TSharedPtr<SSlider>& SliderWidget, float SettingValue, const TAttribute<FText>& TooltipText)
{
	return SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(650)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(Desc)
				.ToolTip(SUTUtils::CreateTooltip(TooltipText))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300.0f)
			.Padding(FMargin(0.0f, 2.0f))
			.Content()
			[
				SAssignNew(SliderWidget, SSlider)
				.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
				.Orientation(Orient_Horizontal)
				.Value(SettingValue)
			]
		]
	];
}

SVerticalBox::FSlot& SUWSystemSettingsDialog::AddGeneralSliderWithLabelWidget(TSharedPtr<SSlider>& SliderWidget, TSharedPtr<STextBlock>& LabelWidget, void(SUWSystemSettingsDialog::*SelectionFunc)(float), const FString& InitialLabel, float SettingValue, const TAttribute<FText>& TooltipText)
{
	return SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(650)
			[
				SAssignNew(LabelWidget, STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(InitialLabel)
				.ToolTip(SUTUtils::CreateTooltip(TooltipText))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300.0f)
			.Padding(FMargin(0.0f, 2.0f))
			.Content()
			[
				SAssignNew(SliderWidget, SSlider)
				.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
				.OnValueChanged(this, SelectionFunc)
				.Orientation(Orient_Horizontal)
				.Value(SettingValue)
			]
		]
	];
}

void SUWSystemSettingsDialog::Construct(const FArguments& InArgs)
{
	DecalLifetimeRange = FVector2D(5.0f, 105.0f);
	ScreenPercentageRange = FVector2D(25.0f, 100.0f);

	SUWDialog::Construct(SUWDialog::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(InArgs._DialogTitle)
							.DialogSize(InArgs._DialogSize)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(InArgs._ButtonMask)
							.OnDialogResult(InArgs._OnDialogResult)
						);

	if (DialogContent.IsValid())
	{
		DialogContent->AddSlot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(46)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(25)
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.LightFill"))
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(GeneralSettingsTabButton, SUTTabButton)
						.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
						.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.SimpleTabButton")
						.ClickMethod(EButtonClickMethod::MouseDown)
						.Text(NSLOCTEXT("SUWSystemSettingsDialog", "ControlTabGeneral", "General").ToString())
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
						.OnClicked(this, &SUWSystemSettingsDialog::OnTabClickGeneral)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(GraphicsSettingsTabButton, SUTTabButton)
						.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
						.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.SimpleTabButton")
						.ClickMethod(EButtonClickMethod::MouseDown)
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
						.Text(NSLOCTEXT("SUWSystemSettingsDialog", "ControlTabGraphics", "Graphics").ToString())
						.OnClicked(this, &SUWSystemSettingsDialog::OnTabClickGraphics)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(AudioSettingsTabButton, SUTTabButton)
						.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f))
						.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.SimpleTabButton")
						.ClickMethod(EButtonClickMethod::MouseDown)
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
						.Text(NSLOCTEXT("SUWSystemSettingsDialog", "ControlTabAudio", "Audio").ToString())
						.OnClicked(this, &SUWSystemSettingsDialog::OnTabClickAudio)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0)
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.LightFill"))
					]
				]
			]

			// Content

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(5.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(0.0f, 5.0f, 0.0f, 5.0f)
				.AutoHeight()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					// Settings Tabs
					SAssignNew(TabWidget, SWidgetSwitcher)

					// General Settings
					+ SWidgetSwitcher::Slot()
					[
						BuildGeneralTab()
					]

					// Graphics Settings
					+ SWidgetSwitcher::Slot()
					[
						BuildGraphicsTab()
					]

					// Audio Settings
					+ SWidgetSwitcher::Slot()
					[
						BuildAudioTab()
					]
				]
			]
		];
	}

	OnTabClickGeneral();
}


TSharedRef<SWidget> SUWSystemSettingsDialog::BuildGeneralTab()
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());

	// Get Viewport size
	FVector2D ViewportSize;
	GetPlayerOwner()->ViewportClient->GetViewportSize(ViewportSize);

	// Get pointer to the UTGameEngine
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine == NULL) // PIE
	{
		UTEngine = UUTGameEngine::StaticClass()->GetDefaultObject<UUTGameEngine>();
	}

	// find current and available screen resolutions
	int32 CurrentResIndex = INDEX_NONE;
	FScreenResolutionArray ResArray;
	if (RHIGetAvailableResolutions(ResArray, false))
	{
		TArray<FIntPoint> AddedRes; // used to more efficiently avoid duplicates
		for (int32 ModeIndex = 0; ModeIndex < ResArray.Num(); ModeIndex++)
		{
			if (ResArray[ModeIndex].Width >= 800 && ResArray[ModeIndex].Height >= 600)
			{
				FIntPoint NewRes(int32(ResArray[ModeIndex].Width), int32(ResArray[ModeIndex].Height));
				if (!AddedRes.Contains(NewRes))
				{
					ResList.Add(MakeShareable(new FString(FString::Printf(TEXT("%ix%i"), NewRes.X, NewRes.Y))));
					if (NewRes.X == int32(ViewportSize.X) && NewRes.Y == int32(ViewportSize.Y))
					{
						CurrentResIndex = ResList.Num() - 1;
					}
					AddedRes.Add(NewRes);
				}
			}
		}
	}
	if (CurrentResIndex == INDEX_NONE)
	{
		CurrentResIndex = ResList.Add(MakeShareable(new FString(FString::Printf(TEXT("%ix%i"), int32(ViewportSize.X), int32(ViewportSize.Y)))));
	}

	// Calculate our current Screen Percentage
	auto ScreenPercentageCVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));
	int32 ScreenPercentage = ScreenPercentageCVar->GetValueOnGameThread();
	ScreenPercentage = int32(FMath::Clamp(float(ScreenPercentage), ScreenPercentageRange.X, ScreenPercentageRange.Y));

	float ScreenPercentageSliderSetting = (float(ScreenPercentage) - ScreenPercentageRange.X) / (ScreenPercentageRange.Y - ScreenPercentageRange.X);

	return SNew(SVerticalBox)

	+ AddSectionHeader(NSLOCTEXT("SUWSystemSettingsDialog", "Options", "- General Options -"))

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(10.0f, 5.0f, 10.0f, 5.0f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(650)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(NSLOCTEXT("SUWSystemSettingsDialog", "Resolution", "Resolution").ToString())
				.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUWSystemSettingsDialog", "Resolution_Tooltip", "Set the resolution of the game window")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SComboBox< TSharedPtr<FString> >)
			.InitiallySelectedItem(ResList[CurrentResIndex])
			.ComboBoxStyle(SUWindowsStyle::Get(), "UT.ComboBox")
			.ButtonStyle(SUWindowsStyle::Get(), "UT.Button.White")
			.OptionsSource(&ResList)
			.OnGenerateWidget(this, &SUWDialog::GenerateStringListWidget)
			.OnSelectionChanged(this, &SUWSystemSettingsDialog::OnResolutionSelected)
			.Content()
			[
				SAssignNew(SelectedRes, STextBlock)
				.Text(*ResList[CurrentResIndex].Get())
				.TextStyle(SUWindowsStyle::Get(),"UT.Common.NormalText.Black")
			]
		]
	]

	+ AddGeneralSliderWithLabelWidget(ScreenPercentageSlider, ScreenPercentageLabel, &SUWSystemSettingsDialog::OnScreenPercentageChange, 
		GetScreenPercentageLabelText(ScreenPercentageSliderSetting), ScreenPercentageSliderSetting, 
		NSLOCTEXT("SUWSystemSettingsDialog", "ScreenPercentage_Tooltip", "Sets the scale as a percentage of your resolution that the engine renders too, this is later upsampled to your desired resolution.\nThis can be a useful preformance tweak to ensure smooth preformance without changing your resolution away from "))

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(10.0f, 5.0f, 10.0f, 5.0f))
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(650)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(NSLOCTEXT("SUWSystemSettingsDialog", "Fullscreen", "Fullscreen").ToString())
				.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUWSystemSettingsDialog", "Fullscreen_Tooltip", "Toggle if the application runs in Fullscreen mode or is in a window.")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(Fullscreen, SCheckBox)
			.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
			.IsChecked(GetPlayerOwner()->ViewportClient->IsFullScreenViewport() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
		]
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(10.0f, 5.0f, 10.0f, 5.0f))
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(650)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(NSLOCTEXT("SUWSystemSettingsDialog", "VSync", "VSync").ToString())
				.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUWSystemSettingsDialog", "VSync_Tooltip", "Toggle VSync, when on the game will syncronize with your display to avoid frame tearing, this could mean slower framerate.")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(VSync, SCheckBox)
			.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
			.IsChecked(UserSettings->IsVSyncEnabled() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
		]
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(10.0f, 5.0f, 10.0f, 5.0f))
	[

		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(650)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(NSLOCTEXT("SUWSystemSettingsDialog", "Frame Rate Cap", "Frame Rate Cap").ToString())
				.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUWSystemSettingsDialog", "FrameRateCap_Tooltip", "The maximum framerate you want the game to run at.")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(FrameRateCap, SEditableTextBox)
			.Style(SUWindowsStyle::Get(),"UT.Common.Editbox.White")
			.ForegroundColor(FLinearColor::Black)
			.MinDesiredWidth(100.0f)
			.Text(FText::AsNumber(UTEngine->FrameRateCap))
		]
	]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10.0f, 5.0f, 10.0f, 5.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(650)
				[
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					.Text(NSLOCTEXT("SUWSystemSettingsDialog", "Smooth Framerate", "Smooth Framerate").ToString())
					.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUWSystemSettingsDialog", "SmoothFramerate_Tooltip", "If your experiancing spiking framerate this will attempt to smooth that by reducing your framerate.")))
				]
			]
			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(SmoothFrameRate, SCheckBox)
					.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
					.IsChecked(GEngine->bSmoothFrameRate ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
				]
		];
}

TSharedRef<SWidget> SUWSystemSettingsDialog::BuildGraphicsTab()
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());

	// find current and available engine scalability options	
	UserSettings->OnSettingsAutodetected().AddSP(this, &SUWSystemSettingsDialog::OnSettingsAutodetected);
	Scalability::FQualityLevels QualitySettings = UserSettings->ScalabilityQuality;
	GeneralScalabilityList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "SettingsLow", "Low").ToString())));
	GeneralScalabilityList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "SettingsMedium", "Medium").ToString())));
	GeneralScalabilityList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "SettingsHigh", "High").ToString())));
	GeneralScalabilityList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "SettingsEpic", "Epic").ToString())));
	QualitySettings.TextureQuality = FMath::Clamp<int32>(QualitySettings.TextureQuality, 0, GeneralScalabilityList.Num() - 1);
	QualitySettings.ShadowQuality = FMath::Clamp<int32>(QualitySettings.ShadowQuality, 0, GeneralScalabilityList.Num() - 1);
	QualitySettings.PostProcessQuality = FMath::Clamp<int32>(QualitySettings.PostProcessQuality, 0, GeneralScalabilityList.Num() - 1);
	QualitySettings.EffectsQuality = FMath::Clamp<int32>(QualitySettings.EffectsQuality, 0, GeneralScalabilityList.Num() - 1);

	AAModeList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "AAModeNone", "None").ToString())));
	AAModeList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "AAModeFXAA", "FXAA").ToString())));
	AAModeList.Add(MakeShareable(new FString(NSLOCTEXT("SUWSystemSettingsDialog", "AAModeTemporal", "Temporal").ToString())));
	auto AAModeCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessAAQuality"));
	int32 AAModeSelection = ConvertAAModeToComboSelection(AAModeCVar->GetValueOnGameThread());

	float DecalSliderSetting = (GetDefault<AUTWorldSettings>()->MaxImpactEffectVisibleLifetime <= 0.0f) ? 1.0f : ((GetDefault<AUTWorldSettings>()->MaxImpactEffectVisibleLifetime - DecalLifetimeRange.X) / (DecalLifetimeRange.Y - DecalLifetimeRange.X));
	float FOVSliderSetting = (GetDefault<AUTPlayerController>()->ConfigDefaultFOV - FOV_CONFIG_MIN) / (FOV_CONFIG_MAX - FOV_CONFIG_MIN);


	return SNew(SVerticalBox)

	+AddSectionHeader(NSLOCTEXT("SUWSystemSettingsDialog", "DetailSettings", "- Graphics Detail Settings -"))

	+ AddGeneralScalabilityWidget(NSLOCTEXT("SUWSystemSettingsDialog", "TextureDetail", "Texture Detail").ToString(), TextureRes, SelectedTextureRes, 
		&SUWSystemSettingsDialog::OnTextureResolutionSelected, QualitySettings.TextureQuality, 
		NSLOCTEXT("SUWSystemSettingsDialog", "TextureDetail_Tooltip", "Controls the quality of textures, lower setting can improve preformance when GPU preformance is an issue."))

	+ AddGeneralScalabilityWidget(NSLOCTEXT("SUWSystemSettingsDialog", "ShadowQuality", "Shadow Quality").ToString(), ShadowQuality, SelectedShadowQuality, 
		&SUWSystemSettingsDialog::OnShadowQualitySelected, QualitySettings.ShadowQuality,
		NSLOCTEXT("SUWSystemSettingsDialog", "ShadowQuality_Tooltip", "Controls the quality of shadows, lower setting can improve preformance on both CPU and GPU."))
	
	+ AddGeneralScalabilityWidget(NSLOCTEXT("SUWSystemSettingsDialog", "EffectsQuality", "Effects Quality").ToString(), EffectQuality, SelectedEffectQuality, 
		&SUWSystemSettingsDialog::OnEffectQualitySelected, QualitySettings.EffectsQuality,
		NSLOCTEXT("SUWSystemSettingsDialog", "EffectQuality_Tooltip", "Controls the quality of effects, lower setting can improve preformance on both CPU and GPU."))

	+ AddGeneralScalabilityWidget(NSLOCTEXT("SUWSystemSettingsDialog", "PP Quality", "Post Process Quality").ToString(), PPQuality, SelectedPPQuality, 
		&SUWSystemSettingsDialog::OnPPQualitySelected, QualitySettings.PostProcessQuality,
		NSLOCTEXT("SUWSystemSettingsDialog", "PPQuality_Tooltip", "Controls the quality of post processing effect, lower setting can improve preformance when GPU preformance is an issue."))

	+ AddAAModeWidget(NSLOCTEXT("SUWSystemSettingsDialog", "AAMode", "Anti Aliasing Mode").ToString(), AAMode, SelectedAAMode, 
		&SUWSystemSettingsDialog::OnAAModeSelected, AAModeSelection,
		NSLOCTEXT("SUWSystemSettingsDialog", "AAMode_Tooltip", "Controls the type of antialiasing, turning it of can improve preformance."))

	+ AddGeneralSliderWithLabelWidget(DecalLifetime, DecalLifetimeLabel, &SUWSystemSettingsDialog::OnDecalLifetimeChange, GetDecalLifetimeLabelText(DecalSliderSetting), DecalSliderSetting,
		NSLOCTEXT("SUWSystemSettingsDialog", "DecalLifetime_Tooltip", "How long decals with live. One example of a decal is the bullet impact marks left on walls."))
	
	+ AddGeneralSliderWithLabelWidget(FOV, FOVLabel, &SUWSystemSettingsDialog::OnFOVChange, GetFOVLabelText(FOVSliderSetting), FOVSliderSetting,
		NSLOCTEXT("SUWSystemSettingsDialog", "FOV_Tooltip", "Controls the Field of View of the gameplay camera."))

	// Autodetect settings button
	+SVerticalBox::Slot()
	.HAlign(HAlign_Center)
	[
		SNew(SButton)
		.HAlign(HAlign_Center)
		.ButtonStyle(SUWindowsStyle::Get(), "UWindows.Standard.Button")
		.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.BoldText")
		.ForegroundColor(FLinearColor::Black)
		.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
		.Text(NSLOCTEXT("SUWSystemSettingsDialog", "AutoSettingsButtonText", "Autodetect Settings"))
		.OnClicked(this, &SUWSystemSettingsDialog::OnAutodetectClick)
	];
}

TSharedRef<SWidget> SUWSystemSettingsDialog::BuildAudioTab()
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());

	return SNew(SVerticalBox)

	+ AddSectionHeader(NSLOCTEXT("SUWSystemSettingsDialog", "SoundSettings", "- Sound/Voice Settings -"))
	+ AddGeneralSliderWidget(NSLOCTEXT("SUWSystemSettingsDialog", "MasterSoundVolume", "Master Sound Volume").ToString(), SoundVolumes[EUTSoundClass::Master], UserSettings->GetSoundClassVolume(EUTSoundClass::Master),
		NSLOCTEXT("SUWSystemSettingsDialog", "MasterSoundVolume_Tooltip", "Controls the volume of all audio, this setting in conjuction the vlolumes below will determine the volume of a particular piece of audio."))
	+ AddGeneralSliderWidget(NSLOCTEXT("SUWSystemSettingsDialog", "MusicVolume", "Music Volume").ToString(), SoundVolumes[EUTSoundClass::Music], UserSettings->GetSoundClassVolume(EUTSoundClass::Music))
	+ AddGeneralSliderWidget(NSLOCTEXT("SUWSystemSettingsDialog", "SFXVolume", "Effects Volume").ToString(), SoundVolumes[EUTSoundClass::SFX], UserSettings->GetSoundClassVolume(EUTSoundClass::SFX))
	+ AddGeneralSliderWidget(NSLOCTEXT("SUWSystemSettingsDialog", "VoiceVolume", "Voice Volume").ToString(), SoundVolumes[EUTSoundClass::Voice], UserSettings->GetSoundClassVolume(EUTSoundClass::Voice));
}


FReply SUWSystemSettingsDialog::OnTabClickGeneral()
{
	TabWidget->SetActiveWidgetIndex(0);
	GeneralSettingsTabButton->BePressed();
	GraphicsSettingsTabButton->UnPressed();
	AudioSettingsTabButton->UnPressed();
	return FReply::Handled();
}

FReply SUWSystemSettingsDialog::OnTabClickGraphics()
{
	TabWidget->SetActiveWidgetIndex(1);
	GeneralSettingsTabButton->UnPressed();
	GraphicsSettingsTabButton->BePressed();
	AudioSettingsTabButton->UnPressed();
	return FReply::Handled();
}

FReply SUWSystemSettingsDialog::OnTabClickAudio()
{
	TabWidget->SetActiveWidgetIndex(2);
	GeneralSettingsTabButton->UnPressed();
	GraphicsSettingsTabButton->UnPressed();
	AudioSettingsTabButton->BePressed();
	return FReply::Handled();
}

FString SUWSystemSettingsDialog::GetFOVLabelText(float SliderValue)
{
	int32 FOVAngle = FMath::TruncToInt(SliderValue * (FOV_CONFIG_MAX - FOV_CONFIG_MIN) + FOV_CONFIG_MIN);
	return FText::Format(NSLOCTEXT("SUWPlayerSettingsDialog", "FOV", "Field of View ({Value})"), FText::FromString(FString::Printf(TEXT("%i"), FOVAngle))).ToString();
}

void SUWSystemSettingsDialog::OnFOVChange(float NewValue)
{
	FOVLabel->SetText(GetFOVLabelText(NewValue));
}

FString SUWSystemSettingsDialog::GetScreenPercentageLabelText(float SliderValue)
{
	// Increments of 5, so divide by 5 and multiply by 5
	int32 ScreenPercentage = FMath::TruncToInt(SliderValue * (ScreenPercentageRange.Y - ScreenPercentageRange.X) + ScreenPercentageRange.X) / 5 * 5;
	return FText::Format(NSLOCTEXT("SUWPlayerSettingsDialog", "ScreenPercentage", "Screen Percentage ({Value}%)"), FText::FromString(FString::Printf(TEXT("%i"), ScreenPercentage))).ToString();
}

void SUWSystemSettingsDialog::OnScreenPercentageChange(float NewValue)
{
	ScreenPercentageLabel->SetText(GetScreenPercentageLabelText(NewValue));
}

FString SUWSystemSettingsDialog::GetDecalLifetimeLabelText(float SliderValue)
{
	if (SliderValue == 1.0f)
	{
		return NSLOCTEXT("SUWPlayerSettingsDialog", "DecalLifetimeInf", "Decal Lifetime (INF)").ToString();
	}
	
	int32 DecalLifetime = FMath::TruncToInt(SliderValue * (DecalLifetimeRange.Y - DecalLifetimeRange.X) + DecalLifetimeRange.X);
	return FText::Format(NSLOCTEXT("SUWPlayerSettingsDialog", "DecalLifetime", "Decal Lifetime ({Value} seconds)"), FText::FromString(FString::Printf(TEXT("%i"), DecalLifetime))).ToString();
}

void SUWSystemSettingsDialog::OnDecalLifetimeChange(float NewValue)
{
	DecalLifetimeLabel->SetText(GetDecalLifetimeLabelText(NewValue));
}

void SUWSystemSettingsDialog::OnSettingsAutodetected(const Scalability::FQualityLevels& DetectedQuality)
{
	TextureRes->SetSelectedItem(GeneralScalabilityList[DetectedQuality.TextureQuality]);
	ShadowQuality->SetSelectedItem(GeneralScalabilityList[DetectedQuality.ShadowQuality]);
	EffectQuality->SetSelectedItem(GeneralScalabilityList[DetectedQuality.EffectsQuality]);
	PPQuality->SetSelectedItem(GeneralScalabilityList[DetectedQuality.PostProcessQuality]);

	int32 AAModeInt = UUTGameUserSettings::ConvertAAScalabilityQualityToAAMode(DetectedQuality.AntiAliasingQuality);
	int32 AAModeSelection = ConvertAAModeToComboSelection(AAModeInt);
	AAMode->SetSelectedItem(AAModeList[AAModeSelection]);

	int32 ScreenPercentage = DetectedQuality.ResolutionQuality;
	ScreenPercentage = int32(FMath::Clamp(float(ScreenPercentage), ScreenPercentageRange.X, ScreenPercentageRange.Y));
	float ScreenPercentageSliderSetting = (float(ScreenPercentage) - ScreenPercentageRange.X) / (ScreenPercentageRange.Y - ScreenPercentageRange.X);
	ScreenPercentageSlider->SetValue(ScreenPercentageSliderSetting);
	ScreenPercentageLabel->SetText(GetScreenPercentageLabelText(ScreenPercentageSliderSetting));
}

FReply SUWSystemSettingsDialog::OnAutodetectClick()
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	UUTLocalPlayer* LocalPlayer = GetPlayerOwner().Get();
	if (ensure(LocalPlayer))
	{
		UserSettings->BenchmarkDetailSettings(LocalPlayer, false);
	}

	return FReply::Handled();
}

FReply SUWSystemSettingsDialog::OKClick()
{
	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	// sound settings
	for (int32 i = 0; i < ARRAY_COUNT(SoundVolumes); i++)
	{
		UserSettings->SetSoundClassVolume(EUTSoundClass::Type(i), SoundVolumes[i]->GetValue());
	}
	// engine scalability
	UserSettings->ScalabilityQuality.TextureQuality = GeneralScalabilityList.Find(TextureRes->GetSelectedItem());
	UserSettings->ScalabilityQuality.ShadowQuality = GeneralScalabilityList.Find(ShadowQuality->GetSelectedItem());
	UserSettings->ScalabilityQuality.PostProcessQuality = GeneralScalabilityList.Find(PPQuality->GetSelectedItem());
	UserSettings->ScalabilityQuality.EffectsQuality = GeneralScalabilityList.Find(EffectQuality->GetSelectedItem());
	Scalability::SetQualityLevels(UserSettings->ScalabilityQuality);
	Scalability::SaveState(GGameUserSettingsIni);
	// resolution
	GetPlayerOwner()->ViewportClient->ConsoleCommand(*FString::Printf(TEXT("setres %s%s"), *SelectedRes->GetText().ToString(), Fullscreen->IsChecked() ? TEXT("f") : TEXT("w")));

	UserSettings->SetAAMode(ConvertComboSelectionToAAMode(*AAMode->GetSelectedItem().Get()));

	// Increments of 5, so divide by 5 and multiply by 5
	int32 NewScreenPercentage = FMath::TruncToInt(ScreenPercentageSlider->GetValue() * (ScreenPercentageRange.Y - ScreenPercentageRange.X) + ScreenPercentageRange.X) / 5 * 5;
	UserSettings->SetScreenPercentage(NewScreenPercentage);

	const TCHAR* Cmd = *SelectedRes->GetText().ToString();
	int32 X=FCString::Atoi(Cmd);
	const TCHAR* CmdTemp = FCString::Strchr(Cmd,'x') ? FCString::Strchr(Cmd,'x')+1 : FCString::Strchr(Cmd,'X') ? FCString::Strchr(Cmd,'X')+1 : TEXT("");
	int32 Y=FCString::Atoi(CmdTemp);
	UserSettings->SetScreenResolution(FIntPoint(X, Y));
	UserSettings->SetFullscreenMode(Fullscreen->IsChecked() ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	UserSettings->SetVSyncEnabled(VSync->IsChecked());
	UserSettings->SaveConfig();

	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine == NULL) // PIE
	{
		UTEngine = UUTGameEngine::StaticClass()->GetDefaultObject<UUTGameEngine>();
	}
	if (FrameRateCap->GetText().ToString().IsNumeric())
	{
		UTEngine->FrameRateCap = FCString::Atoi(*FrameRateCap->GetText().ToString());
	}
	GEngine->bSmoothFrameRate = SmoothFrameRate->IsChecked();
	GEngine->SaveConfig();
	UTEngine->SaveConfig();

	// FOV
	float NewFOV = FMath::TruncToFloat(FOV->GetValue() * (FOV_CONFIG_MAX - FOV_CONFIG_MIN) + FOV_CONFIG_MIN);
	AUTPlayerController* PC = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController);
	if (PC != NULL)
	{
		PC->FOV(NewFOV);
	}
	else
	{
		AUTPlayerController::StaticClass()->GetDefaultObject<AUTPlayerController>()->ConfigDefaultFOV = NewFOV;
		AUTPlayerController::StaticClass()->GetDefaultObject<AUTPlayerController>()->SaveConfig();
	}

	// impact effect lifetime - note that 1.0 on the slider is infinite lifetime
	float NewDecalLifetime = (DecalLifetime->GetValue() * (DecalLifetimeRange.Y - DecalLifetimeRange.X) + DecalLifetimeRange.X);
	AUTWorldSettings* DefaultWS = AUTWorldSettings::StaticClass()->GetDefaultObject<AUTWorldSettings>();
	DefaultWS->MaxImpactEffectVisibleLifetime = NewDecalLifetime;
	DefaultWS->MaxImpactEffectInvisibleLifetime = NewDecalLifetime * 0.5f;
	DefaultWS->SaveConfig();
	if (GetPlayerOwner()->PlayerController != NULL)
	{
		AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetPlayerOwner()->PlayerController->GetWorld()->GetWorldSettings());
		if (WS != NULL)
		{
			WS->MaxImpactEffectVisibleLifetime = DefaultWS->MaxImpactEffectVisibleLifetime;
			WS->MaxImpactEffectInvisibleLifetime = DefaultWS->MaxImpactEffectInvisibleLifetime;
		}
	}
	
	GetPlayerOwner()->CloseDialog(SharedThis(this));
	return FReply::Handled();
}

FReply SUWSystemSettingsDialog::CancelClick()
{
	GetPlayerOwner()->CloseDialog(SharedThis(this));
	return FReply::Handled();
}

void SUWSystemSettingsDialog::OnResolutionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedRes->SetText(*NewSelection.Get());
}
void SUWSystemSettingsDialog::OnTextureResolutionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedTextureRes->SetText(*NewSelection.Get());
}
void SUWSystemSettingsDialog::OnShadowQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedShadowQuality->SetText(*NewSelection.Get());
}
void SUWSystemSettingsDialog::OnPPQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedPPQuality->SetText(*NewSelection.Get());
}
void SUWSystemSettingsDialog::OnEffectQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedEffectQuality->SetText(*NewSelection.Get());
}

void SUWSystemSettingsDialog::OnAAModeSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	SelectedAAMode->SetText(*NewSelection.Get());
}

int32 SUWSystemSettingsDialog::ConvertAAModeToComboSelection(int32 AAMode)
{
	// 0:off, 1:very low (faster FXAA), 2:low (FXAA), 3:medium (faster TemporalAA), 4:high (default TemporalAA)
	if (AAMode == 0)
	{
		return 0;
	}
	else if (AAMode == 1 || AAMode == 2)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

int32 SUWSystemSettingsDialog::ConvertComboSelectionToAAMode(const FString& Selection)
{
	// 0:off, 1:very low (faster FXAA), 2:low (FXAA), 3:medium (faster TemporalAA), 4:high (default TemporalAA)
	if (Selection == *AAModeList[0].Get())
	{
		return 0;
	}
	else if (Selection == *AAModeList[1].Get())
	{
		return 2;
	}
	else
	{
		return 4;
	}
}

FReply SUWSystemSettingsDialog::OnButtonClick(uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK) OKClick();
	else if (ButtonID == UTDIALOG_BUTTON_CANCEL) CancelClick();
	return FReply::Handled();
}

#endif