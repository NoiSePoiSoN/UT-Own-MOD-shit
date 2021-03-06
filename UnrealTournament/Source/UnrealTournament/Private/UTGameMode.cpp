// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/GameMode.h"
#include "UTDeathMessage.h"
#include "UTGameMessage.h"
#include "UTVictoryMessage.h"
#include "UTTimedPowerup.h"
#include "UTCountDownMessage.h"
#include "UTFirstBloodMessage.h"
#include "UTSpectatorPickupMessage.h"
#include "UTMutator.h"
#include "UTScoreboard.h"
#include "SlateBasics.h"
#include "UTAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Analytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "UTBot.h"
#include "UTSquadAI.h"
#include "Slate/Panels/SULobbyMatchSetupPanel.h"
#include "Slate/SUWPlayerInfoDialog.h"
#include "Slate/SlateGameResources.h"
#include "Slate/Widgets/SUTTabWidget.h"
#include "SNumericEntryBox.h"
#include "UTCharacterContent.h"
#include "UTGameEngine.h"
#include "UTWorldSettings.h"
#include "UTLevelSummary.h"
#include "UTHUD_CastingGuide.h"
#include "UTBotCharacter.h"
#include "UTReplicatedMapInfo.h"
#include "StatNames.h"
#include "UTProfileItemMessage.h"
#include "UTWeap_ImpactHammer.h"
#include "UTWeap_Translocator.h"
#include "UTWeap_Enforcer.h"
#include "Engine/DemoNetDriver.h"
#include "EngineBuildSettings.h"

UUTResetInterface::UUTResetInterface(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{}

namespace MatchState
{
	const FName CountdownToBegin = FName(TEXT("CountdownToBegin"));
	const FName MatchEnteringOvertime = FName(TEXT("MatchEnteringOvertime"));
	const FName MatchIsInOvertime = FName(TEXT("MatchIsInOvertime"));
	const FName MapVoteHappening = FName(TEXT("MapVoteHappening"));
	const FName MatchIntermission = FName(TEXT("MatchIntermission"));
}

AUTGameMode::AUTGameMode(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FObjectFinder<UClass> PlayerPawnObject(TEXT("Class'/Game/RestrictedAssets/Blueprints/DefaultCharacter.DefaultCharacter_C'"));
	
	if (PlayerPawnObject.Object != NULL)
	{
		DefaultPawnClass = (UClass*)PlayerPawnObject.Object;
	}

	// use our custom HUD class
	HUDClass = AUTHUD::StaticClass();
	CastingGuideHUDClass = AUTHUD_CastingGuide::StaticClass();

	GameStateClass = AUTGameState::StaticClass();
	PlayerStateClass = AUTPlayerState::StaticClass();

	PlayerControllerClass = AUTPlayerController::StaticClass();
	BotClass = AUTBot::StaticClass();

	MinRespawnDelay = 1.5f;
	bUseSeamlessTravel = false;
	CountDown = 4;
	bPauseable = false;
	RespawnWaitTime = 1.5f;
	ForceRespawnTime = 3.5f;
	MaxReadyWaitTime = 60;
	bHasRespawnChoices = false;
	MinPlayersToStart = 2;
	MaxWaitForPlayers = 90.f;
	bOnlyTheStrongSurvive = false;
	EndScoreboardDelay = 3.0f;
	GameDifficulty = 3.0f;
	BotFillCount = 0;
	bWeaponStayActive = true;
	VictoryMessageClass = UUTVictoryMessage::StaticClass();
	DeathMessageClass = UUTDeathMessage::StaticClass();
	GameMessageClass = UUTGameMessage::StaticClass();
	SquadType = AUTSquadAI::StaticClass();
	MaxSquadSize = 3;
	bClearPlayerInventory = false;
	bDelayedStart = true;
	bDamageHurtsHealth = true;
	bAmmoIsLimited = true;
	bAllowOvertime = true;
	bForceRespawn = false;

	DefaultPlayerName = FText::FromString(TEXT("Player"));
	MapPrefix = TEXT("DM");
	LobbyInstanceID = 0;
	DemoFilename = TEXT("%m-%td");
	bDedicatedInstance = false;

	MapVoteTime = 30;

	bSpeedHackDetection = false;
	MaxTimeMargin = 2.0f;
	MinTimeMargin = -2.0f;
	TimeMarginSlack = 0.001f;

	bCasterControl = false;
}

void AUTGameMode::BeginPlayMutatorHack(FFrame& Stack, RESULT_DECL)
{
	P_FINISH;

	// WARNING: 'this' is actually an AActor! Only do AActor things!
	if (!IsA(ALevelScriptActor::StaticClass()) && !IsA(AUTMutator::StaticClass()) &&
		(RootComponent == NULL || RootComponent->Mobility != EComponentMobility::Static || (!IsA(AStaticMeshActor::StaticClass()) && !IsA(ALight::StaticClass()))) )
	{
		AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
		// a few type checks being AFTER the CheckRelevance() call is intentional; want mutators to be able to modify, but not outright destroy
		if (Game != NULL && Game != this && !Game->CheckRelevance((AActor*)this) && !IsA(APlayerController::StaticClass()))
		{
			Destroy();
		}
	}
}

void AUTGameMode::Demigod()
{
	bDamageHurtsHealth = !bDamageHurtsHealth;
}

// Parse options for this game...
void AUTGameMode::InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage )
{
	// HACK: workaround to inject CheckRelevance() into the BeginPlay sequence
	UFunction* Func = AActor::GetClass()->FindFunctionByName(FName(TEXT("ReceiveBeginPlay")));
	Func->FunctionFlags |= FUNC_Native;
	Func->SetNativeFunc((Native)&AUTGameMode::BeginPlayMutatorHack);

	UE_LOG(UT,Log,TEXT("==============="));
	UE_LOG(UT,Log,TEXT("  Init Game Option: %s"), *Options);

	if (IOnlineSubsystem::Get() != NULL)
	{
		IOnlineEntitlementsPtr EntitlementInterface = IOnlineSubsystem::Get()->GetEntitlementsInterface();
		if (EntitlementInterface.IsValid())
		{
			FOnQueryEntitlementsCompleteDelegate Delegate;
			Delegate.BindUObject(this, &AUTGameMode::EntitlementQueryComplete);
			EntitlementInterface->AddOnQueryEntitlementsCompleteDelegate_Handle(Delegate);
		}
	}

	Super::InitGame(MapName, Options, ErrorMessage);

	GameDifficulty = FMath::Max(0, GetIntOption(Options, TEXT("Difficulty"), GameDifficulty));
	
	HostLobbyListenPort = GetIntOption(Options, TEXT("HostPort"), 14000);
	FString InOpt = ParseOption(Options, TEXT("ForceRespawn"));
	bForceRespawn = EvalBoolOptions(InOpt, bForceRespawn);

	InOpt = ParseOption(Options, TEXT("OnlyStrong"));
	bOnlyTheStrongSurvive = EvalBoolOptions(InOpt, bOnlyTheStrongSurvive);

	MaxWaitForPlayers = GetIntOption(Options, TEXT("MaxPlayerWait"), MaxWaitForPlayers);
	MaxReadyWaitTime = GetIntOption(Options, TEXT("MaxReadyWait"), MaxReadyWaitTime);

	TimeLimit = FMath::Max(0,GetIntOption( Options, TEXT("TimeLimit"), TimeLimit ));
	TimeLimit *= 60;

	// Set goal score to end match.
	GoalScore = FMath::Max(0,GetIntOption( Options, TEXT("GoalScore"), GoalScore ));

	MinPlayersToStart = FMath::Max(1, GetIntOption( Options, TEXT("MinPlayers"), MinPlayersToStart));

	RespawnWaitTime = FMath::Max(0,GetIntOption( Options, TEXT("RespawnWait"), RespawnWaitTime ));

	InOpt = ParseOption(Options, TEXT("Hub"));
	if (!InOpt.IsEmpty()) HubAddress = InOpt;

	InOpt = ParseOption(Options, TEXT("HubKey"));
	if (!InOpt.IsEmpty()) HubKey = InOpt;

	// alias for testing convenience
	if (HasOption(Options, TEXT("Bots")))
	{
		BotFillCount = GetIntOption(Options, TEXT("Bots"), BotFillCount) + 1;
	}
	else
	{
		BotFillCount = GetIntOption(Options, TEXT("BotFill"), BotFillCount);
	}

	InOpt = ParseOption(Options, TEXT("CasterControl"));
	bCasterControl = EvalBoolOptions(InOpt, bCasterControl);

	for (int32 i = 0; i < BuiltInMutators.Num(); i++)
	{
		AddMutatorClass(BuiltInMutators[i]);
	}
	
	for (int32 i = 0; i < ConfigMutators.Num(); i++)
	{
		AddMutator(ConfigMutators[i]);
	}

	InOpt = ParseOption(Options, TEXT("Mutator"));
	if (InOpt.Len() > 0)
	{
		UE_LOG(UT, Log, TEXT("Mutators: %s"), *InOpt);
		while (InOpt.Len() > 0)
		{
			FString LeftOpt;
			int32 Pos = InOpt.Find(TEXT(","));
			if (Pos > 0)
			{
				LeftOpt = InOpt.Left(Pos);
				InOpt = InOpt.Right(InOpt.Len() - Pos - 1);
			}
			else
			{
				LeftOpt = InOpt;
				InOpt.Empty();
			}
			AddMutator(LeftOpt);
		}
	}

	InOpt = ParseOption(Options, TEXT("Demorec"));
	if (InOpt.Len() > 0)
	{
		bRecordDemo = InOpt != TEXT("off") && InOpt != TEXT("false") && InOpt != TEXT("0") && InOpt != TEXT("no") && InOpt != GFalse.ToString() && InOpt != GNo.ToString();
		if (bRecordDemo)
		{
			DemoFilename = InOpt;
		}
	}

	PostInitGame(Options);

	UE_LOG(UT, Log, TEXT("LobbyInstanceID: %i"), LobbyInstanceID);
	UE_LOG(UT, Log, TEXT("=================="));

	// If we are a lobby instance, establish a communication beacon with the lobby.  For right now, this beacon is created on the local host
	// but in time, the lobby's address will have to be passed
	RecreateLobbyBeacon();
}

void AUTGameMode::AddMutator(const FString& MutatorPath)
{
	int32 PeriodIndex = INDEX_NONE;
	if (MutatorPath.Right(2) != FString(TEXT("_C")) && MutatorPath.FindChar(TCHAR('.'), PeriodIndex))
	{
		FName MutatorModuleName = FName(*MutatorPath.Left(PeriodIndex));
		if (!FModuleManager::Get().IsModuleLoaded(MutatorModuleName))
		{
			if (!FModuleManager::Get().LoadModule(MutatorModuleName).IsValid())
			{
				UE_LOG(UT, Warning, TEXT("Failed to load module for mutator %s"), *MutatorModuleName.ToString());
			}
		}
	}
	TSubclassOf<AUTMutator> MutClass = LoadClass<AUTMutator>(NULL, *MutatorPath, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (MutClass == NULL && !MutatorPath.Contains(TEXT(".")))
	{
		// use asset registry to try to find shorthand name
		static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
		if (MutatorAssets.Num() == 0)
		{
			GetAllBlueprintAssetData(AUTMutator::StaticClass(), MutatorAssets);

			// create fake asset entries for native classes
			for (TObjectIterator<UClass> It; It; ++It)
			{
				if (It->IsChildOf(AUTMutator::StaticClass()) && It->HasAnyClassFlags(CLASS_Native) && !It->HasAnyClassFlags(CLASS_Abstract))
				{
					FAssetData NewData;
					NewData.AssetName = It->GetFName();
					NewData.TagsAndValues.Add(NAME_GeneratedClass, It->GetPathName());
					MutatorAssets.Add(NewData);
				}
			}
		}
			
		for (const FAssetData& Asset : MutatorAssets)
		{
			if (Asset.AssetName == FName(*MutatorPath) || Asset.AssetName == FName(*FString(TEXT("Mutator_") + MutatorPath)) || Asset.AssetName == FName(*FString(TEXT("UTMutator_") + MutatorPath)))
			{
				const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
				if (ClassPath != NULL)
				{
					MutClass = LoadObject<UClass>(NULL, **ClassPath);
					if (MutClass != NULL)
					{
						break;
					}
				}
			}
		}
	}
	if (MutClass == NULL)
	{
		UE_LOG(UT, Warning, TEXT("Failed to find or load mutator '%s'"), *MutatorPath);
	}
	else
	{
		AddMutatorClass(MutClass);
	}
}

void AUTGameMode::AddMutatorClass(TSubclassOf<AUTMutator> MutClass)
{
	if (MutClass != NULL && AllowMutator(MutClass))
	{
		AUTMutator* NewMut = GetWorld()->SpawnActor<AUTMutator>(MutClass);
		if (NewMut != NULL)
		{
			NewMut->Init(OptionsString);
			if (BaseMutator == NULL)
			{
				BaseMutator = NewMut;
			}
			else
			{
				BaseMutator->AddMutator(NewMut);
			}
		}
	}
}

bool AUTGameMode::AllowMutator(TSubclassOf<AUTMutator> MutClass)
{
	const AUTMutator* DefaultMut = MutClass.GetDefaultObject();

	for (AUTMutator* Mut = BaseMutator; Mut != NULL; Mut = Mut->NextMutator)
	{
		if (Mut->GetClass() == MutClass)
		{
			// already have this exact mutator
			UE_LOG(UT, Log, TEXT("Rejected mutator %s - already have one"), *MutClass->GetPathName());
			return false;
		}
		for (int32 i = 0; i < Mut->GroupNames.Num(); i++)
		{
			for (int32 j = 0; j < DefaultMut->GroupNames.Num(); j++)
			{
				if (Mut->GroupNames[i] == DefaultMut->GroupNames[j])
				{
					// group conflict
					UE_LOG(UT, Log, TEXT("Rejected mutator %s - already have mutator %s with group %s"), *MutClass->GetPathName(), *Mut->GetPathName(), *Mut->GroupNames[i].ToString());
					return false;
				}
			}
		}
	}
	return true;
}

void AUTGameMode::InitGameState()
{
	Super::InitGameState();

	UTGameState = Cast<AUTGameState>(GameState);
	if (UTGameState != NULL)
	{
		UTGameState->SetGoalScore(GoalScore);
		UTGameState->SetTimeLimit(0);
		UTGameState->RespawnWaitTime = RespawnWaitTime;
		UTGameState->ForceRespawnTime = ForceRespawnTime;
		UTGameState->bTeamGame = bTeamGame;
		UTGameState->bWeaponStay = bWeaponStayActive;

		UTGameState->bIsInstanceServer = IsGameInstanceServer();

		// Setup the loadout replication
		for (int32 i=0; i < AvailableLoadout.Num(); i++)
		{
			if (AvailableLoadout[i].ItemClass)
			{
				UTGameState->AddLoadoutItem(AvailableLoadout[i]);
			}
		}
	}
	else
	{
		UE_LOG(UT,Error, TEXT("UTGameState is NULL %s"), *GameStateClass->GetFullName());
	}

	if (GameSession != NULL && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession)
		{
			UTGameSession->RegisterServer();
			FTimerHandle TempHandle;
			GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::UpdateOnlineServer, 60.0f);	

			if (UTGameSession->bSessionValid)
			{
				NotifyLobbyGameIsReady();
			}
		}
	}
}

void AUTGameMode::UpdateOnlineServer()
{
	if (GameSession &&  GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		AUTGameSession* GS = Cast<AUTGameSession>(GameSession);
		if (GS)
		{
			GS->UpdateGameState();
		}
	}
}

void AUTGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// Because of the behavior changes to PostBeginPlay() this really has to go here as PreInitializeCompoennts is sort of the UE4 PBP even
	// though PBP still exists.  It can't go in InitGame() or InitGameState() because team info needed for team locked GameObjectives are not
	// setup at that point.

	for (TActorIterator<AUTGameObjective> ObjIt(GetWorld()); ObjIt; ++ObjIt)
	{
		ObjIt->InitializeObjective();	
		GameObjectiveInitialized(*ObjIt);
	}

	// init startup bots
	for (int32 i = 0; i < SelectedBots.Num() && NumPlayers + NumBots < BotFillCount; i++)
	{
		AddAssetBot(SelectedBots[i].BotAsset, SelectedBots[i].Team);
	}
}

void AUTGameMode::GameObjectiveInitialized(AUTGameObjective* Obj)
{
	// Allow subclasses to track game objectives as they are initialized
}

APlayerController* AUTGameMode::Login(UPlayer* NewPlayer, ENetRole RemoteRole, const FString& Portal, const FString& Options, const TSharedPtr<FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	bool bCastingView = EvalBoolOptions(ParseOption(Options, TEXT("CastingView")), false);
	if (bCastingView)
	{
		// we allow the casting split views to ignore certain restrictions, so make sure they aren't lying
		UChildConnection* ChildConn = Cast<UChildConnection>(NewPlayer);
		if ( ChildConn == NULL || ChildConn->Parent == NULL || Cast<AUTPlayerController>(ChildConn->Parent->PlayerController) == NULL ||
			!((AUTPlayerController*)ChildConn->Parent->PlayerController)->bCastingGuide )
		{
			ErrorMessage = TEXT("Illegal URL options");
			return NULL;
		}
	}

	FString ModdedPortal = Portal;
	FString ModdedOptions = Options;
	if (BaseMutator != NULL)
	{
		BaseMutator->ModifyLogin(ModdedPortal, ModdedOptions);
	}
	APlayerController* Result = Super::Login(NewPlayer, RemoteRole, Portal, Options, UniqueId, ErrorMessage);
	if (Result != NULL)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(Result);
		if (UTPC != NULL)
		{
			UTPC->bCastingGuide = EvalBoolOptions(ParseOption(Options, TEXT("CastingGuide")), false);
			// TODO: check if allowed?
			if (UTPC->bCastingGuide)
			{
				UTPC->PlayerState->bOnlySpectator = true;
				UTPC->CastingGuideViewIndex = 0;
			}

			if (bCastingView)
			{
				UChildConnection* ChildConn = Cast<UChildConnection>(NewPlayer);
				UTPC->CastingGuideViewIndex = (ChildConn != NULL) ? (ChildConn->Parent->Children.Find(ChildConn) + 1) : 0;
			}
		}
		AUTPlayerState* PS = Cast<AUTPlayerState>(Result->PlayerState);
		if (PS != NULL)
		{
			FString InOpt = ParseOption(Options, TEXT("Character"));
			if (InOpt.Len() > 0)
			{
				PS->SetCharacter(InOpt);
			}
			InOpt = ParseOption(Options, TEXT("Hat"));
			if (InOpt.Len() > 0)
			{
				PS->ServerReceiveHatClass(InOpt);
			}
			InOpt = ParseOption(Options, TEXT("Eyewear"));
			if (InOpt.Len() > 0)
			{
				PS->ServerReceiveEyewearClass(InOpt);
			}
			InOpt = ParseOption(Options, TEXT("Taunt"));
			if (InOpt.Len() > 0)
			{
				PS->ServerReceiveTauntClass(InOpt);
			}
			InOpt = ParseOption(Options, TEXT("Taunt2"));
			if (InOpt.Len() > 0)
			{
				PS->ServerReceiveTaunt2Class(InOpt);
			}
			int32 HatVar = GetIntOption(Options, TEXT("HatVar"), 0);
			PS->ServerReceiveHatVariant(HatVar);
			int32 EyewearVar = GetIntOption(Options, TEXT("EyewearVar"), 0);
			PS->ServerReceiveEyewearVariant(EyewearVar);

			// warning: blindly calling this here relies on ValidateEntitlements() defaulting to "allow" if we have not yet obtained this user's entitlement information
			PS->ValidateEntitlements();

			// Setup the default loadout
			if (UTGameState->AvailableLoadout.Num() > 0)
			{
				for (int32 i=0; i < UTGameState->AvailableLoadout.Num(); i++)			
				{
					if (UTGameState->AvailableLoadout[i]->bDefaultInclude)
					{
						PS->Loadout.Add(UTGameState->AvailableLoadout[i]);
					}
				}
			}

			bool bCaster = EvalBoolOptions(ParseOption(Options, TEXT("Caster")), false);
			if (bCaster && bCasterControl)
			{
				PS->bCaster = true;
				PS->bOnlySpectator = true;
			}
		}
	}
	return Result;
}

void AUTGameMode::EntitlementQueryComplete(bool bWasSuccessful, const FUniqueNetId& UniqueId, const FString& Namespace, const FString& ErrorMessage)
{
	// validate player's custom options
	// note that it is possible that they have not entered the game yet, since this is started via PreLogin() - in that case we'll validate from Login()
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (PS->UniqueId.IsValid() && *PS->UniqueId.GetUniqueNetId().Get() == UniqueId)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
			if (UTPS != NULL)
			{
				UTPS->ValidateEntitlements();
			}
		}
	}
}

AUTBot* AUTGameMode::AddBot(uint8 TeamNum)
{
	AUTBot* NewBot = GetWorld()->SpawnActor<AUTBot>(BotClass);
	if (NewBot != NULL)
	{
		if (BotAssets.Num() == 0)
		{
			GetAllAssetData(UUTBotCharacter::StaticClass(), BotAssets);
		}
		
		TArray<const FAssetData*> EligibleBots;
		for (const FAssetData& Asset : BotAssets)
		{
			const FString* SkillText = Asset.TagsAndValues.Find(TEXT("Skill"));
			if (SkillText != NULL)
			{
				float BaseSkill = FCString::Atof(**SkillText);
				if (BaseSkill >= GameDifficulty - 0.5f && BaseSkill < GameDifficulty + 1.0f)
				{
					EligibleBots.Add(&Asset);
				}
			}
		}
		bool bLoadedBotData = false;
		while (EligibleBots.Num() > 0 && !bLoadedBotData)
		{
			int32 Index = FMath::RandHelper(EligibleBots.Num());
			const UUTBotCharacter* BotData = Cast<UUTBotCharacter>(EligibleBots[Index]->GetAsset());
			if (BotData != NULL)
			{
				NewBot->CharacterData = BotData;
				NewBot->Personality = BotData->Personality;
				SetUniqueBotName(NewBot, BotData);
				NewBot->InitializeSkill(BotData->Skill);
				AUTPlayerState* PS = Cast<AUTPlayerState>(NewBot->PlayerState);
				if (PS != NULL)
				{
					PS->SetCharacter(BotData->Character.ToString());
					PS->ServerReceiveHatClass(BotData->HatType.ToString());
					PS->ServerReceiveHatVariant(BotData->HatVariantId);
					PS->ServerReceiveEyewearClass(BotData->EyewearType.ToString());
					PS->ServerReceiveEyewearVariant(BotData->EyewearVariantId);
				}
				bLoadedBotData = true;
			}
			else
			{
				EligibleBots.RemoveAt(Index);
			}
		}
		// pick bot character
		if (!bLoadedBotData)
		{
			UE_LOG(UT, Warning, TEXT("AddBot(): No BotCharacters defined that are appropriate for game difficulty %f"), GameDifficulty);
			static int32 NameCount = 0;
			NewBot->PlayerState->SetPlayerName(FString(TEXT("TestBot")) + ((NameCount > 0) ? FString::Printf(TEXT("_%i"), NameCount) : TEXT("")));
			NewBot->InitializeSkill(GameDifficulty);
			NameCount++;
		}

		AUTPlayerState* PS = Cast<AUTPlayerState>(NewBot->PlayerState);
		if (PS != NULL)
		{
			PS->bReadyToPlay = true;
		}

		NumBots++;
		ChangeTeam(NewBot, TeamNum);
		GenericPlayerInitialization(NewBot);
	}
	return NewBot;
}
AUTBot* AUTGameMode::AddNamedBot(const FString& BotName, uint8 TeamNum)
{
	if (BotAssets.Num() == 0)
	{
		GetAllAssetData(UUTBotCharacter::StaticClass(), BotAssets);
	}

	const UUTBotCharacter* BotData = NULL;
	for (const FAssetData& Asset : BotAssets)
	{
		if (Asset.AssetName.ToString() == BotName)
		{
			BotData = Cast<UUTBotCharacter>(Asset.GetAsset());
			if (BotData != NULL)
			{
				break;
			}
		}
	}


	if (BotData == NULL)
	{
		UE_LOG(UT, Error, TEXT("Character data for bot '%s' not found"), *BotName);
		return NULL;
	}
	else
	{
		AUTBot* NewBot = GetWorld()->SpawnActor<AUTBot>(BotClass);
		if (NewBot != NULL)
		{
			NewBot->CharacterData = BotData;
			NewBot->Personality = BotData->Personality;
			SetUniqueBotName(NewBot, BotData);

			AUTPlayerState* PS = Cast<AUTPlayerState>(NewBot->PlayerState);
			if (PS != NULL)
			{
				PS->bReadyToPlay = true;
				PS->SetCharacter(BotData->Character.ToString());
				PS->ServerReceiveHatClass(BotData->HatType.ToString());
				PS->ServerReceiveHatVariant(BotData->HatVariantId);
				PS->ServerReceiveEyewearClass(BotData->EyewearType.ToString());
				PS->ServerReceiveEyewearVariant(BotData->EyewearVariantId);
			}

			NewBot->InitializeSkill(BotData->Skill);
			NumBots++;
			ChangeTeam(NewBot, TeamNum);
			GenericPlayerInitialization(NewBot);
		}

		return NewBot;
	}
}
AUTBot* AUTGameMode::AddAssetBot(const FStringAssetReference& BotAssetPath, uint8 TeamNum)
{
	const UUTBotCharacter* BotData = Cast<UUTBotCharacter>(BotAssetPath.TryLoad());
	if (BotData != NULL)
	{
		AUTBot* NewBot = GetWorld()->SpawnActor<AUTBot>(BotClass);
		if (NewBot != NULL)
		{
			NewBot->CharacterData = BotData;
			NewBot->Personality = BotData->Personality;
			SetUniqueBotName(NewBot, BotData);

			AUTPlayerState* PS = Cast<AUTPlayerState>(NewBot->PlayerState);
			if (PS != NULL)
			{
				PS->bReadyToPlay = true;
				PS->SetCharacter(BotData->Character.ToString());
				PS->ServerReceiveHatClass(BotData->HatType.ToString());
				PS->ServerReceiveHatVariant(BotData->HatVariantId);
				PS->ServerReceiveEyewearClass(BotData->EyewearType.ToString());
				PS->ServerReceiveEyewearVariant(BotData->EyewearVariantId);
			}

			NewBot->InitializeSkill(BotData->Skill);
			NumBots++;
			ChangeTeam(NewBot, TeamNum);
			GenericPlayerInitialization(NewBot);
		}
		return NewBot;
	}
	else
	{
		return NULL;
	}
}

void AUTGameMode::SetUniqueBotName(AUTBot* B, const UUTBotCharacter* BotData)
{
	TArray<FString> PossibleNames;
	PossibleNames.Add(BotData->GetName());
	PossibleNames += BotData->AltNames;

	for (int32 i = 1; true; i++)
	{
		for (const FString& TestName : PossibleNames)
		{
			FString FinalName = (i == 1) ? TestName : FString::Printf(TEXT("%s-%i"), *TestName, i);
			bool bTaken = false;
			for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
			{
				if (It->IsValid() && It->Get()->PlayerState != NULL && It->Get()->PlayerState->PlayerName == FinalName)
				{
					bTaken = true;
					break;
				}
			}
			if (!bTaken)
			{
				B->PlayerState->SetPlayerName(FinalName);
				return;
			}
		}
	}
}

AUTBot* AUTGameMode::ForceAddBot(uint8 TeamNum)
{
	BotFillCount = FMath::Max<int32>(BotFillCount, NumPlayers + NumBots + 1);
	return AddBot(TeamNum);
}
AUTBot* AUTGameMode::ForceAddNamedBot(const FString& BotName, uint8 TeamNum)
{
	return AddNamedBot(BotName, TeamNum);
}

void AUTGameMode::SetBotCount(uint8 NewCount)
{
	BotFillCount = NumPlayers + NewCount;
}

void AUTGameMode::AddBots(uint8 Num)
{
	BotFillCount = FMath::Max(NumPlayers, BotFillCount) + Num;
}

void AUTGameMode::KillBots()
{
	BotFillCount = 0;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AUTBot* B = Cast<AUTBot>(It->Get());
		if (B != NULL)
		{
			B->Destroy();
			It--;
		}
	}
}

bool AUTGameMode::AllowRemovingBot(AUTBot* B)
{
	AUTPlayerState* PS = Cast<AUTPlayerState>(B->PlayerState);
	// flag carriers should stay in the game until they lose it
	if (PS != NULL && PS->CarriedObject != NULL)
	{
		return false;
	}
	else
	{
		// score leader should stay in the game unless it's the last bot
		if (NumBots > 1 && PS != NULL)
		{
			bool bHighScore = true;
			for (APlayerState* OtherPS : GameState->PlayerArray)
			{
				if (OtherPS != PS && OtherPS->Score >= PS->Score)
				{
					bHighScore = false;
					break;
				}
			}
			if (bHighScore)
			{
				return false;
			}
		}

		// remove as soon as dead or out of combat
		// TODO: if this isn't getting them out fast enough we can restrict to only human players
		return B->GetPawn() == NULL || B->GetEnemy() == NULL || B->LostContact(5.0f);
	}
}

void AUTGameMode::CheckBotCount()
{
	if (NumPlayers + NumBots > BotFillCount)
	{
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AUTBot* B = Cast<AUTBot>(It->Get());
			if (B != NULL && AllowRemovingBot(B))
			{
				B->Destroy();
				break;
			}
		}
	}
	else while (NumPlayers + NumBots < BotFillCount)
	{
		AddBot();
	}
}

void AUTGameMode::RecreateLobbyBeacon()
{
	UE_LOG(UT,Log,TEXT("RecreateLobbyBeacon: %s %i"), HubAddress.IsEmpty() ? TEXT("none") : *HubAddress, LobbyInstanceID);

	// If we have an instance id, or if we have a defined HUB address, then attempt to connect.
	if (LobbyInstanceID > 0 || !HubAddress.IsEmpty())
	{
		if (LobbyBeacon)
		{
			// Destroy the existing beacon first
			LobbyBeacon->DestroyBeacon();
			LobbyBeacon = nullptr;
		}

		LobbyBeacon = GetWorld()->SpawnActor<AUTServerBeaconLobbyClient>(AUTServerBeaconLobbyClient::StaticClass());
		if (LobbyBeacon)
		{
			FString IP = HubAddress.IsEmpty() ? TEXT("127.0.0.1") : HubAddress;

			FURL LobbyURL(nullptr, *IP, TRAVEL_Absolute);
			LobbyURL.Port = HostLobbyListenPort;

			LobbyBeacon->InitLobbyBeacon(LobbyURL, LobbyInstanceID, ServerInstanceGUID, HubKey);
			UE_LOG(UT, Verbose, TEXT("..... Connecting back to lobby on port %i!"), HostLobbyListenPort);
		}
	}
}

/**
 *	DefaultTimer is called once per second and is useful for consistent timed events that don't require to be 
 *  done every frame.
 **/
void AUTGameMode::DefaultTimer()
{
	// preview world is for blueprint editing, don't try to play
	if (GetWorld()->WorldType == EWorldType::Preview)
	{
		return;
	}

	if (MatchState == MatchState::MapVoteHappening)
	{
		UTGameState->VoteTimer--;
		if (UTGameState->VoteTimer<0)
		{
			UTGameState->VoteTimer = 0;
		}
	}

 	if (LobbyBeacon && LobbyBeacon->GetNetConnection()->State == EConnectionState::USOCK_Closed)
	{
		// if the server is empty and would be asking the hub to kill it, just kill ourselves rather than waiting for reconnection
		// this relies on there being good monitoring and cleanup code in the hub, but it's better than some kind of network port failure leaving an instance spamming connection attempts forever
		// also handles the hub itself failing
		if (!bDedicatedInstance && NumPlayers <= 0 && MatchState != MatchState::WaitingToStart)
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

		// Lost connection with the beacon. Recreate it.
		UE_LOG(UT, Verbose, TEXT("Beacon %s lost connection. Attempting to recreate."), *GetNameSafe(this));
		RecreateLobbyBeacon();
	}

	// Let the game see if it's time to end the match
	CheckGameTime();

	if (bForceRespawn)
	{
		for (auto It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AController* Controller = *It;
			if (Controller->IsInState(NAME_Inactive))
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(Controller->PlayerState);
				if (PS != NULL && PS->ForceRespawnTime <= 0.0f)
				{
					RestartPlayer(Controller);
				}
			}
		}
	}

	CheckBotCount();

	int32 NumPlayers = GetNumPlayers();

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		if (GetWorld()->GetTimeSeconds() - LastLobbyUpdateTime >= 10.0f) // MAKE ME CONIFG!!!!
		{
			UpdateLobbyMatchStats(TEXT(""));
		}

		if (!bDedicatedInstance)
		{
			if (!HasMatchStarted())
			{
				if (GetWorld()->GetRealTimeSeconds() > LobbyInitialTimeoutTime && NumPlayers <= 0)
				{
					// Catch all...
					SendEveryoneBackToLobby();
					LobbyBeacon->Empty();			
				}
			}
			else 
			{
				if (NumPlayers <= 0)
				{
					// Catch all...
					SendEveryoneBackToLobby();
					LobbyBeacon->Empty();
				}
			}
		}
	}
	else
	{
		// Look to see if we should restart the game due to server inactivity
		if (GetNumPlayers() <= 0 && NumSpectators <= 0 && HasMatchStarted())
		{
			EmptyServerTime++;
			if (EmptyServerTime >= AutoRestartTime)
			{
				TravelToNextMap();
			}
		}
		else
		{
			EmptyServerTime = 0;
		}
	}

	if (MatchState == MatchState::MapVoteHappening)
	{
		// Scan the maps and see if we have 


		TArray<AUTReplicatedMapInfo*> Best;
		for (int32 i=0; i< UTGameState->MapVoteList.Num(); i++)
		{
			if (UTGameState->MapVoteList[i]->VoteCount > 0)
			{
				if (Best.Num() == 0 || Best[0]->VoteCount < UTGameState->MapVoteList[i]->VoteCount)
				{
					Best.Empty();
					Best.Add(UTGameState->MapVoteList[i]);
				}
			}
		}
		if ( Best.Num() > 0 )
		{
			int32 Target = int32( float(GetNumPlayers()) * 0.5);
			if ( Best[0]->VoteCount > Target)
			{
				TallyMapVotes();
			}
		}
	}

}

void AUTGameMode::ForceLobbyUpdate()
{
	LastLobbyUpdateTime = -10.0;
}


void AUTGameMode::Reset()
{
	Super::Reset();

	bGameEnded = false;

	//now respawn all the players
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		if (Controller->PlayerState != NULL && !Controller->PlayerState->bOnlySpectator)
		{
			RestartPlayer(Controller);
		}
	}

	UTGameState->SetTimeLimit(0);
}

void AUTGameMode::RestartGame()
{
	if (HasMatchStarted())
	{
		Super::RestartGame();
	}
}

bool AUTGameMode::IsEnemy(AController * First, AController* Second)
{
	return First && Second && !UTGameState->OnSameTeam(First, Second);
}

void AUTGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	// Ignore all killing when entering overtime as we kill off players and don't want it affecting their score.
	if ((GetMatchState() != MatchState::MatchEnteringOvertime) && (GetMatchState() != MatchState::WaitingPostMatch) && (GetMatchState() != MatchState::MapVoteHappening))
	{
		AUTPlayerState* const KillerPlayerState = Killer ? Cast<AUTPlayerState>(Killer->PlayerState) : NULL;
		AUTPlayerState* const KilledPlayerState = KilledPlayer ? Cast<AUTPlayerState>(KilledPlayer->PlayerState) : NULL;

		//UE_LOG(UT, Log, TEXT("Player Killed: %s killed %s"), (KillerPlayerState != NULL ? *KillerPlayerState->PlayerName : TEXT("NULL")), (KilledPlayerState != NULL ? *KilledPlayerState->PlayerName : TEXT("NULL")));

		bool const bEnemyKill = IsEnemy(Killer, KilledPlayer);
		if (!bEnemyKill)
		{
			Killer = NULL;
		}
		if (KilledPlayerState != NULL)
		{
			KilledPlayerState->LastKillerPlayerState = KillerPlayerState;
			KilledPlayerState->IncrementDeaths(DamageType, KillerPlayerState);
			TSubclassOf<UUTDamageType> UTDamage(*DamageType);
			if (UTDamage)
			{
				UTDamage.GetDefaultObject()->ScoreKill(KillerPlayerState, KilledPlayerState, KilledPawn);
			}

			BroadcastDeathMessage(Killer, KilledPlayer, DamageType);
			ScoreKill(Killer, KilledPlayer, KilledPawn, DamageType);
			
			if (bHasRespawnChoices)
			{
				KilledPlayerState->RespawnChoiceA = nullptr;
				KilledPlayerState->RespawnChoiceB = nullptr;
				KilledPlayerState->RespawnChoiceA = Cast<APlayerStart>(ChoosePlayerStart(KilledPlayer));
				KilledPlayerState->RespawnChoiceB = Cast<APlayerStart>(ChoosePlayerStart(KilledPlayer));
				KilledPlayerState->bChosePrimaryRespawnChoice = true;
			}
		}

		DiscardInventory(KilledPawn, Killer);

		if (UTGameState->IsMatchInOvertime() && UTGameState->bOnlyTheStrongSurvive)
		{
			KilledPlayer->ChangeState(NAME_Spectating);
		}
	}
	NotifyKilled(Killer, KilledPlayer, KilledPawn, DamageType);
}

void AUTGameMode::NotifyKilled(AController* Killer, AController* Killed, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	// update AI data
	if (Killer != NULL && Killer != Killed)
	{
		AUTRecastNavMesh* NavData = GetUTNavData(GetWorld());
		if (NavData != NULL)
		{
			{
				UUTPathNode* Node = NavData->FindNearestNode(KilledPawn->GetNavAgentLocation(), KilledPawn->GetSimpleCollisionCylinderExtent());
				if (Node != NULL)
				{
					Node->NearbyDeaths++;
				}
			}
			if (Killer->GetPawn() != NULL)
			{
				// it'd be better to get the node from which the shot was fired, but it's probably not worth it
				UUTPathNode* Node = NavData->FindNearestNode(Killer->GetPawn()->GetNavAgentLocation(), Killer->GetPawn()->GetSimpleCollisionCylinderExtent());
				if (Node != NULL)
				{
					Node->NearbyKills++;
				}
			}
		}
	}

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			AUTBot* B = Cast<AUTBot>(It->Get());
			if (B != NULL)
			{
				B->UTNotifyKilled(Killer, Killed, KilledPawn, DamageType.GetDefaultObject());
			}
		}
	}
}

void AUTGameMode::ScorePickup_Implementation(AUTPickup* Pickup, AUTPlayerState* PickedUpBy, AUTPlayerState* LastPickedUpBy)
{
}

void AUTGameMode::ScoreDamage_Implementation(int32 DamageAmount, AController* Victim, AController* Attacker)
{
	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreDamage(DamageAmount, Victim, Attacker);
	}
}

void AUTGameMode::ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	if( (Killer == Other) || (Killer == NULL) )
	{
		// If it's a suicide, subtract a kill from the player...

		if (Other != NULL && Other->PlayerState != NULL && Cast<AUTPlayerState>(Other->PlayerState) != NULL)
		{
			Cast<AUTPlayerState>(Other->PlayerState)->AdjustScore(-1);
			Cast<AUTPlayerState>(Other->PlayerState)->IncrementKills(DamageType, false);
		}
	}
	else 
	{
		AUTPlayerState * KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		if ( KillerPlayerState != NULL )
		{
			KillerPlayerState->AdjustScore(+1);
			KillerPlayerState->IncrementKills(DamageType, true);
			FindAndMarkHighScorer();
			CheckScore(KillerPlayerState);
		}

		if (!bFirstBloodOccurred)
		{
			BroadcastLocalized(this, UUTFirstBloodMessage::StaticClass(), 0, KillerPlayerState, NULL, NULL);
			bFirstBloodOccurred = true;
		}
	}

	AddKillEventToReplay(Killer, Other, DamageType);

	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreKill(Killer, Other, DamageType);
	}
}

void AUTGameMode::AddKillEventToReplay(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType)
{
	// Could be a suicide
	if (Killer == nullptr)
	{
		return;
	}

	// Shouldn't happen, but safety first
	if (Other == nullptr)
	{
		return;
	}

	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		AUTPlayerState* KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		AUTPlayerState* OtherPlayerState = Cast<AUTPlayerState>(Other->PlayerState);
		TArray<uint8> Data;
		FString KillInfo = FString::Printf(TEXT("%s %s %s"),
											KillerPlayerState ? *KillerPlayerState->PlayerName : TEXT("None"),
											OtherPlayerState ? *OtherPlayerState->PlayerName : TEXT("None"),
											*DamageType->GetName());

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*KillInfo), KillInfo.Len() + 1);

		FString MetaTag = KillerPlayerState->StatsID;
		if (MetaTag.IsEmpty())
		{
			MetaTag = KillerPlayerState->PlayerName;
		}
		DemoNetDriver->AddEvent(TEXT("Kills"), MetaTag, Data);
	}
}

void AUTGameMode::AddMultiKillEventToReplay(AController* Killer)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (Killer && DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		AUTPlayerState* KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		TArray<uint8> Data;
		FString KillInfo = FString::Printf(TEXT("%s"), KillerPlayerState ? *KillerPlayerState->PlayerName : TEXT("None"));

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*KillInfo), KillInfo.Len() + 1);

		FString MetaTag = KillerPlayerState->StatsID;
		if (MetaTag.IsEmpty())
		{
			MetaTag = KillerPlayerState->PlayerName;
		}
		DemoNetDriver->AddEvent(TEXT("MultiKills"), MetaTag, Data);
	}
}

void AUTGameMode::AddSpreeKillEventToReplay(AController* Killer, int32 SpreeLevel)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (Killer && DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		AUTPlayerState* KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		TArray<uint8> Data;
		FString KillInfo = FString::Printf(TEXT("%s %d"), KillerPlayerState ? *KillerPlayerState->PlayerName : TEXT("None"), SpreeLevel);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*KillInfo), KillInfo.Len() + 1);

		FString MetaTag = KillerPlayerState->StatsID;
		if (MetaTag.IsEmpty())
		{
			MetaTag = KillerPlayerState->PlayerName;
		}
		DemoNetDriver->AddEvent(TEXT("SpreeKills"), MetaTag, Data);
	}
}

bool AUTGameMode::OverridePickupQuery_Implementation(APawn* Other, TSubclassOf<AUTInventory> ItemClass, AActor* Pickup, bool& bAllowPickup)
{
	return (BaseMutator != NULL && BaseMutator->OverridePickupQuery(Other, ItemClass, Pickup, bAllowPickup));
}

void AUTGameMode::DiscardInventory(APawn* Other, AController* Killer)
{
	AUTCharacter* UTC = Cast<AUTCharacter>(Other);
	if (UTC != NULL)
	{
		// toss weapon
		if (UTC->GetWeapon() != NULL)
		{
			UTC->TossInventory(UTC->GetWeapon());
		}
		// toss all powerups
		for (TInventoryIterator<> It(UTC); It; ++It)
		{
			if (It->bAlwaysDropOnDeath)
			{
				UTC->TossInventory(*It, FVector(FMath::FRandRange(0.0f, 200.0f), FMath::FRandRange(-400.0f, 400.0f), FMath::FRandRange(0.0f, 200.0f)));
			}
		}
		// delete the rest
		UTC->DiscardAllInventory();
	}
}

void AUTGameMode::FindAndMarkHighScorer()
{
	int32 BestScore = 0;
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState *PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS != nullptr)
		{
			if (BestScore == 0 || PS->Score > BestScore)
			{
				BestScore = PS->Score;
			}
		}
	}

	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState *PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		AController *C = Cast<AController>(PS->GetOwner());
		if (PS != nullptr && PS->Score == BestScore)
		{
			PS->bHasHighScore = true;
			if (C != nullptr)
			{
				AUTCharacter *UTChar = Cast<AUTCharacter>(C->GetPawn());
				if (UTChar && !UTChar->bHasHighScore)
				{
					UTChar->bHasHighScore = true;
					UTChar->HasHighScoreChanged();
				}
			}
		}
		else
		{
			// Clear previous high scores
			PS->bHasHighScore = false;
			if (C != nullptr)
			{
				AUTCharacter *UTChar = Cast<AUTCharacter>(C->GetPawn());
				if (UTChar && UTChar->bHasHighScore)
				{
					UTChar->bHasHighScore = false;
					UTChar->HasHighScoreChanged();
				}
			}
		}
	}
}

bool AUTGameMode::CheckScore_Implementation(AUTPlayerState* Scorer)
{
	if ( Scorer != NULL )
	{
		if ( (GoalScore > 0) && (Scorer->Score >= GoalScore) )
		{
			EndGame(Scorer,FName(TEXT("fraglimit")));
		}
	}
	return true;
}


void AUTGameMode::StartMatch()
{
	if (HasMatchStarted())
	{
		// Already started
		return;
	}
	if (GetWorld()->IsPlayInEditor() || !bDelayedStart)
	{
		SetMatchState(MatchState::InProgress);
	}
	else
	{
		SetMatchState(MatchState::CountdownToBegin);	
	}

	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld()->GetNetMode() != NM_Standalone)
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("MapName"), GetWorld()->GetMapName()));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("GameName"), GetNameSafe(this)));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("GoalScore"), GoalScore));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("TimeLimit"), TimeLimit));
			UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
			if (UTEngine)
			{
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("CustomContent"), UTEngine->LocalContentChecksums.Num()));
			}
			else
			{
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("CustomContent"), 0));
			}
			FUTAnalytics::GetProvider().RecordEvent( TEXT("NewMatch"), ParamArray );
		}
		else
		{
			UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
			if (UTEngine && UTEngine->LocalContentChecksums.Num() > 0)
			{
				TArray<FAnalyticsEventAttribute> ParamArray;
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("CustomContent"), UTEngine->LocalContentChecksums.Num()));
				FUTAnalytics::GetProvider().RecordEvent(TEXT("MatchWithCustomContent"), ParamArray);
			}
		}
	}
}

void AUTGameMode::HandleMatchHasStarted()
{
	// reset things, relevant for any kind of warmup mode and to make sure placed Actors like pickups are in their initial state no matter how much time has passed in pregame
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(UUTResetInterface::StaticClass()))
		{
			IUTResetInterface::Execute_Reset(*It);
		}
	}

	if (UTGameState != NULL)
	{
		UTGameState->CompactSpectatingIDs();
	}

	Super::HandleMatchHasStarted();

	UTGameState->SetTimeLimit(TimeLimit);
	bFirstBloodOccurred = false;
	AnnounceMatchStart();
}

void AUTGameMode::AnnounceMatchStart()
{
	BroadcastLocalized(this, UUTGameMessage::StaticClass(), 0, NULL, NULL, NULL);
}

void AUTGameMode::BeginGame()
{
	UE_LOG(UT,Log,TEXT("BEGIN GAME GameType: %s"), *GetNameSafe(this));
	UE_LOG(UT,Log,TEXT("Difficulty: %f GoalScore: %i TimeLimit (sec): %i"), GameDifficulty, GoalScore, TimeLimit);

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* TestActor = *It;
		if (TestActor &&
			!TestActor->IsPendingKill() &&
			TestActor->IsA<APlayerState>())
		{
			Cast<APlayerState>(TestActor)->StartTime = 0;
		}
	}
	GameState->ElapsedTime = 0;

	//Let the game session override the StartMatch function, in case it wants to wait for arbitration
	if (GameSession->HandleStartMatchRequest())
	{
		return;
	}
	SetMatchState(MatchState::InProgress);
}

void AUTGameMode::EndMatch()
{
	Super::EndMatch();
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::PlayEndOfMatchMessage, 1.0f);

	for (FConstPawnIterator Iterator = GetWorld()->GetPawnIterator(); Iterator; ++Iterator )
	{
		// If a pawn is marked pending kill, *Iterator will be NULL
		APawn* Pawn = *Iterator;
		if (Pawn && !Cast<ASpectatorPawn>(Pawn))
		{
			Pawn->TurnOff();
		}
	}
}

bool AUTGameMode::AllowPausing(APlayerController* PC)
{
	// allow pausing even in listen server mode if no remote players are connected
	return (Super::AllowPausing(PC) || GetWorld()->GetNetDriver() == NULL || GetWorld()->GetNetDriver()->ClientConnections.Num() == 0);
}

void AUTGameMode::UpdateSkillRating()
{

}

void AUTGameMode::SendEndOfGameStats(FName Reason)
{
	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld()->GetNetMode() != NM_Standalone)
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("WinnerName"), UTGameState->WinnerPlayerState ? UTGameState->WinnerPlayerState->PlayerName : TEXT("None")));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("Reason"), *Reason.ToString()));
			FUTAnalytics::GetProvider().RecordEvent(TEXT("EndFFAMatch"), ParamArray);
		}
	}

	if (!bDisableCloudStats)
	{
		UpdateSkillRating();

		const double CloudStatsStartTime = FPlatformTime::Seconds();
		for (int32 i = 0; i < GetWorld()->GameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(GetWorld()->GameState->PlayerArray[i]);

			PS->SetStatsValue(NAME_MatchesPlayed, 1);
			PS->SetStatsValue(NAME_TimePlayed, UTGameState->ElapsedTime);
			PS->SetStatsValue(NAME_PlayerXP, PS->Score);
			
			PS->AddMatchToStats(GetClass()->GetPathName(), nullptr, &GetWorld()->GameState->PlayerArray, &InactivePlayerArray);
			if (PS != nullptr)
			{
				PS->WriteStatsToCloud();
			}
		}

		for (int32 i = 0; i < InactivePlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(InactivePlayerArray[i]);
			if (PS && !PS->HasWrittenStatsToCloud())
			{
				PS->SetStatsValue(NAME_MatchesQuit, 1);

				PS->SetStatsValue(NAME_MatchesPlayed, 1);
				PS->SetStatsValue(NAME_TimePlayed, UTGameState->ElapsedTime);
				PS->SetStatsValue(NAME_PlayerXP, PS->Score);

				PS->AddMatchToStats(GetClass()->GetPathName(), nullptr, &GetWorld()->GameState->PlayerArray, &InactivePlayerArray);
				if (PS != nullptr)
				{
					PS->WriteStatsToCloud();
				}
			}
		}

		const double CloudStatsTime = FPlatformTime::Seconds() - CloudStatsStartTime;
		UE_LOG(UT, Verbose, TEXT("Cloud stats write time %.3f"), CloudStatsTime);
	}

	AwardProfileItems();
}

void AUTGameMode::AwardProfileItems()
{
#if !UE_BUILD_SHIPPING
	if (!FEngineBuildSettings::IsInternalBuild())
	{
		return;
	}
	// TODO: temporarily profile item giveaway for testing
	// give item to highest scoring player
	APlayerState* Best = NULL;
	float BestScore = 0.0f;
	for (APlayerState* PS : GetWorld()->GameState->PlayerArray)
	{
		if (PS != NULL && PS->Score > BestScore)
		{
			Best = PS;
			BestScore = PS->Score;
		}
	}
	if (Best != NULL && Best->UniqueId.GetUniqueNetId().IsValid())
	{
		TArray<FAssetData> AllItems;
		GetAllAssetData(UUTProfileItem::StaticClass(), AllItems, false);
		if (AllItems.Num() > 0)
		{
			TArray<FProfileItemEntry> Rewards;
			new(Rewards)FProfileItemEntry(Cast<UUTProfileItem>(AllItems[FMath::RandHelper(AllItems.Num())].GetAsset()), 1);

			if (Rewards[0].Item != NULL)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(Best->GetOwner());
				if (PC != NULL)
				{
					PC->ClientReceiveLocalizedMessage(UUTProfileItemMessage::StaticClass(), 0, Best, NULL, Rewards[0].Item);
				}
				GiveProfileItems(Best->UniqueId.GetUniqueNetId(), Rewards);
			}
		}
	}
#endif
}

void AUTGameMode::EndGame(AUTPlayerState* Winner, FName Reason )
{
	// Dont ever end the game in PIE
	if (GetWorld()->WorldType == EWorldType::PIE) return;

	// If we don't have a winner, then go and find one
	if (Winner == NULL)
	{
		for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
		{
			AController* Controller = *Iterator;
			AUTPlayerState* CPS = Cast<AUTPlayerState> (Controller->PlayerState);
			if ( CPS && ((Winner == NULL) || (CPS->Score >= Winner->Score)) )
			{
				Winner = CPS;
			}
		}
	}

	UTGameState->SetWinner(Winner);
	EndTime = GetWorld()->TimeSeconds;

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		FString MatchStats = FString::Printf(TEXT("%i"), GetWorld()->GetGameState()->ElapsedTime);
		LobbyBeacon->EndGame(MatchStats);
	}

	SetEndGameFocus(Winner);

	// Allow replication to happen before reporting scores, stats, etc.
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::HandleMatchHasEnded, 1.5f);
	bGameEnded = true;

	// Setup a timer to pop up the final scoreboard on everyone
	FTimerHandle TempHandle2;
	GetWorldTimerManager().SetTimer(TempHandle2, this, &AUTGameMode::ShowFinalScoreboard, EndScoreboardDelay);

	// Setup a timer to continue to the next map.

	EndTime = GetWorld()->TimeSeconds;
	FTimerHandle TempHandle3;
	GetWorldTimerManager().SetTimer(TempHandle3, this, &AUTGameMode::TravelToNextMap, EndTimeDelay);

	FTimerHandle TempHandle4;
	float EndReplayDelay = EndTimeDelay - 10.f;
	GetWorldTimerManager().SetTimer(TempHandle4, this, &AUTGameMode::StopReplayRecording, EndReplayDelay);

	SendEndOfGameStats(Reason);

	EndMatch();
}

void AUTGameMode::StopReplayRecording()
{
	if (IsHandlingReplays() && GetGameInstance() != nullptr)
	{
		GetGameInstance()->StopRecordingReplay();
	}
}

void AUTGameMode::InstanceNextMap(const FString& NextMap)
{
	if (NextMap != TEXT(""))
	{
		FString TravelMapName = NextMap;
		if ( FPackageName::IsShortPackageName(NextMap) )
		{
			FPackageName::SearchForPackageOnDisk(NextMap, &TravelMapName); 
		}
		
		GetWorld()->ServerTravel(TravelMapName, false);
	}
	else
	{
		SendEveryoneBackToLobby();
	}
}

/**
 *	NOTE: This is a really simple map list.  It doesn't support multiple maps in the list, etc and is really dumb.  But it
 *  will work for now.
 **/
void AUTGameMode::TravelToNextMap_Implementation()
{
	FString CurrentMapName = GetWorld()->GetMapName();
	UE_LOG(UT,Log,TEXT("TravelToNextMap: %i %i"),bDedicatedInstance,IsGameInstanceServer());

	if (!bDedicatedInstance && IsGameInstanceServer())
	{
		if (UTGameState->MapVoteList.Num() > 0)
		{
			SetMatchState(MatchState::MapVoteHappening);
		}
		else
		{
			SendEveryoneBackToLobby();
		}
	}
	else
	{
		if (!RconNextMapName.IsEmpty())
		{

			FString TravelMapName = RconNextMapName;
			if ( FPackageName::IsShortPackageName(RconNextMapName) )
			{
				FPackageName::SearchForPackageOnDisk(RconNextMapName, &TravelMapName); 
			}

			GetWorld()->ServerTravel(TravelMapName, false);
			return;
		}

		int32 MapIndex = -1;
		for (int32 i=0;i<MapRotation.Num();i++)
		{
			if (MapRotation[i].EndsWith(CurrentMapName))
			{
				MapIndex = i;
				break;
			}
		}

		if (MapRotation.Num() > 0)
		{
			MapIndex = (MapIndex + 1) % MapRotation.Num();
			if (MapIndex >=0 && MapIndex < MapRotation.Num())
			{

				FString TravelMapName = MapRotation[MapIndex];
				if ( FPackageName::IsShortPackageName(MapRotation[MapIndex]) )
				{
					FPackageName::SearchForPackageOnDisk(MapRotation[MapIndex], &TravelMapName); 
				}
		
				GetWorld()->ServerTravel(TravelMapName, false);
				return;
			}
		}

		RestartGame();	
	}
}

void AUTGameMode::ShowFinalScoreboard()
{
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL)
		{
			PC->ClientToggleScoreboard(true);
		}
	}
}

void AUTGameMode::SetEndGameFocus(AUTPlayerState* Winner)
{
	if (Winner == NULL) return; // It's possible to call this with Winner == NULL if timelimit is hit with noone on the server

	EndGameFocus = Cast<AController>(Winner->GetOwner())->GetPawn();
	if ( (EndGameFocus == NULL) && (Cast<AController>(Winner->GetOwner()) != NULL) )
	{
		// If the controller of the winner does not have a pawn, give him one.
		RestartPlayer(Cast<AController>(Winner->GetOwner()));
		EndGameFocus = Cast<AController>(Winner->GetOwner())->GetPawn();
	}

	if ( EndGameFocus != NULL )
	{
		EndGameFocus->bAlwaysRelevant = true;
	}

	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		Controller->GameHasEnded(EndGameFocus, (Controller->PlayerState != NULL) && (Controller->PlayerState == Winner) );
	}
}


void AUTGameMode::BroadcastDeathMessage(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType)
{
	if (DeathMessageClass != NULL)
	{
		if ( (Killer == Other) || (Killer == NULL) )
		{
			BroadcastLocalized(this, DeathMessageClass, 1, NULL, Other->PlayerState, DamageType);
		}
		else
		{
			BroadcastLocalized(this, DeathMessageClass, 0, Killer->PlayerState, Other->PlayerState, DamageType);
		}
	}
}

void AUTGameMode::PlayEndOfMatchMessage()
{
	if (!UTGameState)
	{
		return;
	}
	bool bIsFlawlessVictory = (UTGameState->WinnerPlayerState->Deaths == 0);
/*	if (bIsFlawlessVictory)
	{
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			AController* Controller = *Iterator;
			if (Controller->PlayerState != NULL && !Controller->PlayerState->bOnlySpectator && (Controller->PlayerState->Score > 0.f) & (Controller->PlayerState != UTGameState->WinnerPlayerState))
			{
				bIsFlawlessVictory = false;
				break;
			}
		}
	}*/
	uint32 FlawlessOffset = bIsFlawlessVictory ? 2 : 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC && (PC->PlayerState != NULL) && !PC->PlayerState->bOnlySpectator)
		{
			PC->ClientReceiveLocalizedMessage(VictoryMessageClass, FlawlessOffset + ((UTGameState->WinnerPlayerState == PC->PlayerState) ? 1 : 0), UTGameState->WinnerPlayerState, PC->PlayerState, NULL);
		}
	}
}

void AUTGameMode::RestartPlayer(AController* aPlayer)
{
	if ((aPlayer == NULL) || (aPlayer->PlayerState == NULL) || aPlayer->PlayerState->PlayerName.IsEmpty())
	{
		UE_LOG(UT, Warning, TEXT("RestartPlayer with a bad player, bad playerstate, or empty player name"));
		return;
	}

	if (!IsMatchInProgress() || aPlayer->PlayerState->bOnlySpectator)
	{
		return;
	}

	{
		TGuardValue<bool> FlagGuard(bSetPlayerDefaultsNewSpawn, true);
		Super::RestartPlayer(aPlayer);

		// apply any health changes
		AUTCharacter* UTC = Cast<AUTCharacter>(aPlayer->GetPawn());
		if (UTC != NULL && UTC->GetClass()->GetDefaultObject<AUTCharacter>()->Health == 0)
		{
			UTC->Health = UTC->HealthMax;
		}
	}

	if (Cast<AUTBot>(aPlayer) != NULL)
	{
		((AUTBot*)aPlayer)->LastRespawnTime = GetWorld()->TimeSeconds;
	}

	if (!aPlayer->IsLocalController() && Cast<AUTPlayerController>(aPlayer) != NULL)
	{
		((AUTPlayerController*)aPlayer)->ClientSwitchToBestWeapon();
	}

	// clear spawn choices
	Cast<AUTPlayerState>(aPlayer->PlayerState)->RespawnChoiceA = nullptr;
	Cast<AUTPlayerState>(aPlayer->PlayerState)->RespawnChoiceB = nullptr;

	// clear multikill in progress
	Cast<AUTPlayerState>(aPlayer->PlayerState)->LastKillTime = -100.f;
}

void AUTGameMode::GiveDefaultInventory(APawn* PlayerPawn)
{
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(PlayerPawn);
	if (UTCharacter != NULL)
	{
		if (bClearPlayerInventory)
		{
			UTCharacter->DefaultCharacterInventory.Empty();
		}
		UTCharacter->AddDefaultInventory(DefaultInventory);
	}
}

/* 
  Make sure pawn properties are back to default
  Also add default inventory
*/
void AUTGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	if (BaseMutator != NULL)
	{
		BaseMutator->ModifyPlayer(PlayerPawn, bSetPlayerDefaultsNewSpawn);
	}

	if (bSetPlayerDefaultsNewSpawn)
	{
		GiveDefaultInventory(PlayerPawn);
	}
}

void AUTGameMode::ChangeName(AController* Other, const FString& S, bool bNameChange)
{
	// Cap player name's at 15 characters...
	FString SMod = S;
	if (SMod.Len()>15)
	{
		SMod = SMod.Left(15);
	}

    if ( !Other->PlayerState|| FCString::Stricmp(*Other->PlayerState->PlayerName, *SMod) == 0 )
    {
		return;
	}

	// Look to see if someone else is using the the new name
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		if (Controller->PlayerState && FCString::Stricmp(*Controller->PlayerState->PlayerName, *SMod) == 0)
		{
			if ( Cast<APlayerController>(Other) != NULL )
			{
					Cast<APlayerController>(Other)->ClientReceiveLocalizedMessage( GameMessageClass, 5 );
					if ( FCString::Stricmp(*Other->PlayerState->PlayerName, *(DefaultPlayerName.ToString())) == 0 )
					{
						Other->PlayerState->SetPlayerName(FString::Printf(TEXT("%s%i"), *DefaultPlayerName.ToString(), Other->PlayerState->PlayerId));
					}
				return;
			}
		}
	}

    Other->PlayerState->SetPlayerName(SMod);
}

bool AUTGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	if ( Player && Cast<APlayerStartPIE>(Player->StartSpot.Get()) )
	{
		return true;
	}

	return ( GetWorld()->GetNetMode() == NM_Standalone && Player != NULL && Player->StartSpot.IsValid() &&
		(GetMatchState() == MatchState::WaitingToStart || (Player->PlayerState != NULL && Cast<AUTPlayerState>(Player->PlayerState)->bWaitingPlayer))
		 && (RatePlayerStart(Cast<APlayerStart>(Player->StartSpot.Get()), Player) >= 0.f) );
}


AActor* AUTGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	AActor* const Best = Super::FindPlayerStart_Implementation(Player, IncomingName);
	if (Best)
	{
		LastStartSpot = Best;
	}

	return Best;
}

FString AUTGameMode::InitNewPlayer(APlayerController* NewPlayerController, const TSharedPtr<FUniqueNetId>& UniqueId, const FString& Options, const FString& Portal)
{
	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	AUTPlayerState* NewPlayerState = Cast<AUTPlayerState>(NewPlayerController->PlayerState);
	if (bHasRespawnChoices && NewPlayerState && !NewPlayerState->bIsSpectator)
	{
		NewPlayerState->RespawnChoiceA = nullptr;
		NewPlayerState->RespawnChoiceB = nullptr;
		NewPlayerState->RespawnChoiceA = Cast<APlayerStart>(ChoosePlayerStart(NewPlayerController));
		NewPlayerState->RespawnChoiceB = Cast<APlayerStart>(ChoosePlayerStart(NewPlayerController));
		NewPlayerState->bChosePrimaryRespawnChoice = true;
	}

	return ErrorMessage;
}

AActor* AUTGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	AUTPlayerState* UTPS = Cast<AUTPlayerState>(Player->PlayerState);
	if (bHasRespawnChoices && UTPS->RespawnChoiceA != nullptr && UTPS->RespawnChoiceB != nullptr)
	{
		if (UTPS->bChosePrimaryRespawnChoice)
		{
			return UTPS->RespawnChoiceA;
		}
		else
		{
			return UTPS->RespawnChoiceB;
		}
	}

	TArray<APlayerStart*> PlayerStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		PlayerStarts.Add(*It);
	}

	if (PlayerStarts.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		for (int32 i = 0; i < PlayerStarts.Num(); i++)
		{
			APlayerStart* P = PlayerStarts[i];

			if (P->IsA(APlayerStartPIE::StaticClass()))
			{
				// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
				return P;
			}
		}
	}
	
	// Always randomize the list order a bit to prevent groups of bad starts from permanently making the next decent start overused
	for (int32 i = 0; i < 2; i++)
	{
		int32 RandIndexOne = FMath::RandHelper(PlayerStarts.Num());
		int32 RandIndexTwo = FMath::RandHelper(PlayerStarts.Num());
		APlayerStart* SavedStart = PlayerStarts[RandIndexOne]; 
		PlayerStarts[RandIndexOne] = PlayerStarts[RandIndexTwo];
		PlayerStarts[RandIndexTwo] = SavedStart;
	}

	// Start by choosing a random start
	int32 RandStart = FMath::RandHelper(PlayerStarts.Num());

	float BestRating = 0.f;
	APlayerStart* BestStart = NULL;
	for ( int32 i=RandStart; i<PlayerStarts.Num(); i++ )
	{
		APlayerStart* P = PlayerStarts[i];

		float NewRating = RatePlayerStart(P,Player);

		if (NewRating >= 30.0f)
		{
			// this PlayerStart is good enough
			return P;
		}
		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
		}
	}
	for ( int32 i=0; i<RandStart; i++ )
	{
		APlayerStart* P = PlayerStarts[i];

		float NewRating = RatePlayerStart(P,Player);

		if (NewRating >= 30.0f)
		{
			// this PlayerStart is good enough
			return P;
		}
		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
		}
	}
	return (BestStart != NULL) ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

float AUTGameMode::RatePlayerStart(APlayerStart* P, AController* Player)
{
	float Score = 30.0f;

	AActor* LastSpot = (Player != NULL && Player->StartSpot.IsValid()) ? Player->StartSpot.Get() : NULL;
	AUTPlayerState *UTPS = Player ? Cast<AUTPlayerState>(Player->PlayerState) : NULL;
	if (P == LastStartSpot || (LastSpot != NULL && P == LastSpot))
	{
		// avoid re-using starts
		Score -= 15.0f;
	}
	FVector StartLoc = P->GetActorLocation() + AUTCharacter::StaticClass()->GetDefaultObject<AUTCharacter>()->BaseEyeHeight;
	if (UTPS && UTPS->RespawnChoiceA)
	{
		if (P == UTPS->RespawnChoiceA)
		{
			// make sure to have two choices
			return -5.f;
		}
		// try to get far apart choices
		float Dist = (UTPS->RespawnChoiceA->GetActorLocation() - StartLoc).Size();
		if (Dist < 5000.0f)
		{
			Score -= 5.f;
		}
	}

	if (Player != NULL)
	{
		bool bTwoPlayerGame = (NumPlayers + NumBots == 2);
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			AController* OtherController = *Iterator;
			ACharacter* OtherCharacter = Cast<ACharacter>( OtherController->GetPawn());

			if ( OtherCharacter && OtherCharacter->PlayerState )
			{
				if (FMath::Abs(StartLoc.Z - OtherCharacter->GetActorLocation().Z) < P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
					&& (StartLoc - OtherCharacter->GetActorLocation()).Size2D() < P->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius())
				{
					// overlapping - would telefrag
					return -10.f;
				}

				float NextDist = (OtherCharacter->GetActorLocation() - StartLoc).Size();
				static FName NAME_RatePlayerStart = FName(TEXT("RatePlayerStart"));
				bool bIsLastKiller = (OtherCharacter->PlayerState == Cast<AUTPlayerState>(Player->PlayerState)->LastKillerPlayerState);

				if (((NextDist < 8000.0f) || bTwoPlayerGame) && !UTGameState->OnSameTeam(Player, OtherController))
				{
					if (!GetWorld()->LineTraceTestByChannel(StartLoc, OtherCharacter->GetActorLocation() + FVector(0.f, 0.f, OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), ECC_Visibility, FCollisionQueryParams(NAME_RatePlayerStart, false)))
					{
						// Avoid the last person that killed me
						if (bIsLastKiller)
						{
							Score -= 7.f;
						}

						Score -= (5.f - 0.0003f * NextDist);
					}
					else if (NextDist < 4000.0f)
					{
						// Avoid the last person that killed me
						Score -= bIsLastKiller ? 5.f : 0.0005f * (5000.f - NextDist);

						if (!GetWorld()->LineTraceTestByChannel(StartLoc, OtherCharacter->GetActorLocation(), ECC_Visibility, FCollisionQueryParams(NAME_RatePlayerStart, false, this)))
						{
							Score -= 2.f;
						}
					}
				}
			}
			else if (bHasRespawnChoices && OtherController->PlayerState && !OtherController->GetPawn() && !OtherController->PlayerState->bOnlySpectator)
			{
				// make sure no one else has this start as a pending choice
				AUTPlayerState* OtherUTPS = Cast<AUTPlayerState>(OtherController->PlayerState);
				if (OtherUTPS)
				{
					if (P == OtherUTPS->RespawnChoiceA || P == OtherUTPS->RespawnChoiceB)
					{
						return -5.f;
					}
					if (bTwoPlayerGame)
					{
						// avoid choosing starts near a pending start
						if (OtherUTPS->RespawnChoiceA)
						{
							float Dist = (OtherUTPS->RespawnChoiceA->GetActorLocation() - StartLoc).Size();
							Score -= 7.f * FMath::Max(0.f, (5000.f - Dist) / 5000.f);
						}
						if (OtherUTPS->RespawnChoiceB)
						{
							float Dist = (OtherUTPS->RespawnChoiceB->GetActorLocation() - StartLoc).Size();
							Score -= 7.f * FMath::Max(0.f, (5000.f - Dist) / 5000.f);
						}
					}
				}
			}

		}
	}
	return FMath::Max(Score, 0.2f);
}

/**
 *	We are going to duplicate GameMode's StartNewPlayer because we need to replicate the scoreboard class along with the hud class.  
 *  We are doing this here like this because we are trying to not change engine.  Ultimately the code to create the hud should be
 *  moved to it's own easy to override function instead of being hard-coded in StartNewPlayer
 **/
void AUTGameMode::StartNewPlayer(APlayerController* NewPlayer)
{
	AUTPlayerController* UTNewPlayer = Cast<AUTPlayerController>(NewPlayer);
	if (UTNewPlayer != NULL)
	{
		// tell client what hud class to use
		UTNewPlayer->HUDClass = HUDClass;
		UTNewPlayer->OnRep_HUDClass();

		// start match, or let player enter, immediately
		if (UTGameState->HasMatchStarted())
		{
			RestartPlayer(NewPlayer);
		}

		if (NewPlayer->GetPawn() != NULL)
		{
			NewPlayer->GetPawn()->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation());
		}
	}
	else
	{
		Super::StartNewPlayer(NewPlayer);
	}
}

void AUTGameMode::StartPlay()
{
	Super::StartPlay();
	StartPlayTime = GetWorld()->GetTimeSeconds();
}

bool AUTGameMode::ReadyToStartMatch_Implementation()
{
	if (GetWorld()->IsPlayInEditor() || !bDelayedStart)
	{
		// starting on first frame has side effects in PIE because of differences in ordering; components haven't been initialized/registered yet...
		if (GetWorld()->TimeSeconds == 0.0f)
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AUTGameMode::StartMatch);
			return false;
		}
		else
		{
			// PIE is always ready to start.
			return true;
		}
	}

	// By default start when we have > 0 players
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		UTGameState->PlayersNeeded = (GetNetMode() == NM_Standalone) ? 0 : FMath::Max(0, MinPlayersToStart - NumPlayers - NumBots);
		if ((UTGameState->PlayersNeeded == 0) && (NumPlayers + NumSpectators > 0))
		{
			bool bCasterReady = false;
			if ((bCasterControl) || (MaxReadyWaitTime <= 0) || (UTGameState->RemainingTime > 0) || (GetNetMode() == NM_Standalone))
			{
				for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
				{
					AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
					if (PS != NULL && !PS->bOnlySpectator && !PS->bReadyToPlay)
					{
						return false;
					}

					//Only need one caster to be ready
					if (bCasterControl && PS->bCaster && PS->bReadyToPlay)
					{
						bCasterReady = true;
					}
				}
			}
			return (!bCasterControl || bCasterReady);
		}
		else
		{
			if ((MaxWaitForPlayers > 0) && (GetWorld()->GetTimeSeconds() - StartPlayTime > MaxWaitForPlayers))
			{
				BotFillCount = FMath::Max(BotFillCount, MinPlayersToStart);
			}
			if (MaxReadyWaitTime > 0)
			{
				// reset max wait for players to ready up
				UTGameState->SetTimeLimit(MaxReadyWaitTime);
			}
		}
	}
	return false;
}

/**
 *	Overwriting all of these functions to work the way I think it should.  Really, the match state should
 *  only be in 1 place otherwise it's prone to mismatch errors.  I'm chosen the GameState because it's
 *  replicated and will be available on clients.
 **/
bool AUTGameMode::HasMatchStarted() const
{
	return UTGameState->HasMatchStarted();
}

bool AUTGameMode::IsMatchInProgress() const
{
	return UTGameState->IsMatchInProgress();
}

bool AUTGameMode::HasMatchEnded() const
{
	return UTGameState->HasMatchEnded();
}

/**	I needed to rework the ordering of SetMatchState until it can be corrected in the engine. **/
void AUTGameMode::SetMatchState(FName NewState)
{
	if (MatchState == NewState)
	{
		return;
	}

	MatchState = NewState;
	if (GameState)
	{
		GameState->SetMatchState(NewState);
	}

	CallMatchStateChangeNotify();

	if (BaseMutator != NULL)
	{
		BaseMutator->NotifyMatchStateChange(MatchState);
	}
}

void AUTGameMode::CallMatchStateChangeNotify()
{
	// Call change callbacks

	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchIsWaitingToStart();
	}
	else if (MatchState == MatchState::CountdownToBegin)
	{
		HandleCountdownToBegin();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		HandleMatchHasEnded();
	}
	else if (MatchState == MatchState::LeavingMap)
	{
		HandleLeavingMap();
	}
	else if (MatchState == MatchState::Aborted)
	{
		HandleMatchAborted();
	}
	else if (MatchState == MatchState::MatchEnteringOvertime)
	{
		HandleEnteringOvertime();
	}
	else if (MatchState == MatchState::MatchIsInOvertime)
	{
		HandleMatchInOvertime();
	}
	else if (MatchState == MatchState::MapVoteHappening)
	{
		HandleMapVote();
	}
}

void AUTGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// save AI data only after completed matches
	AUTRecastNavMesh* NavData = GetUTNavData(GetWorld());
	if (NavData != NULL)
	{
		NavData->SaveMapLearningData();
	}
}

void AUTGameMode::HandleEnteringOvertime()
{
	if (bOnlyTheStrongSurvive)
	{
		// We are entering overtime, kill off anyone not at the top of the leader board....

		AUTPlayerState* BestPlayer = NULL;
		AUTPlayerState* KillPlayer = NULL;
		float BestScore = 0.0;

		for (int32 PlayerIdx = 0; PlayerIdx < UTGameState->PlayerArray.Num(); PlayerIdx++)
		{
			if (UTGameState->PlayerArray[PlayerIdx] != NULL)
			{
				if (BestPlayer == NULL || UTGameState->PlayerArray[PlayerIdx]->Score > BestScore)
				{
					if (BestPlayer != NULL)
					{
						KillPlayer = BestPlayer;
					}
					BestPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
					BestScore = BestPlayer->Score;
				}
				else if (UTGameState->PlayerArray[PlayerIdx]->Score < BestScore)
				{
					KillPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
				}
			}

			if (KillPlayer != NULL)
			{
				// No longer the best.. kill him.. KILL HIM NOW!!!!!
				AController* COwner = Cast<AController>(KillPlayer->GetOwner());
				if (COwner != NULL )
				{
					if (COwner->GetPawn() != NULL)
					{
						AUTCharacter* UTChar = Cast<AUTCharacter>(COwner->GetPawn());
						if (UTChar != NULL)
						{
							//UE_LOG(UT, Log, TEXT("    -- Calling Died"));
							// Kill off the pawn...
							UTChar->Died(NULL, FDamageEvent(UUTDamageType::StaticClass()));
							// Send this character a message/taunt about not making the cut....
						}
					}

					// Tell the player they didn't make the cut
					AUTPlayerController* PC = Cast<AUTPlayerController>(COwner);
					if (PC)
					{
						PC->ClientReceiveLocalizedMessage(UUTGameMessage::StaticClass(), 8);					
						PC->ChangeState(NAME_Spectating);
					}
				}
				KillPlayer = NULL;
			}
		}
		
		// force respawn any players that are applicable for overtime but are currently dead
		if (BestPlayer != NULL)
		{
			for (APlayerState* TestPlayer : UTGameState->PlayerArray)
			{
				if (TestPlayer->Score == BestPlayer->Score && !TestPlayer->bOnlySpectator)
				{
					AController* C = Cast<AController>(TestPlayer->GetOwner());
					if (C != NULL && C->GetPawn() == NULL)
					{
						RestartPlayer(C);
					}
				}
			}
		}

		UTGameState->bOnlyTheStrongSurvive = true;
	}

	SetMatchState(MatchState::MatchIsInOvertime);
}

void AUTGameMode::HandleMatchInOvertime()
{
	// Send the overtime message....
	BroadcastLocalized( this, UUTGameMessage::StaticClass(), 1, NULL, NULL, NULL);
}

void AUTGameMode::HandleCountdownToBegin()
{
	// Currently broken by replay streaming
	/*
	if (bRecordDemo)
	{
		FString MapName = GetOutermost()->GetName();
		GetWorld()->Exec(GetWorld(), *FString::Printf(TEXT("Demorec %s"), *DemoFilename.Replace(TEXT("%m"), *MapName.RightChop(MapName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd) + 1))));
	}*/
	CountDown = 5;
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::CheckCountDown, 1.0, false);
}

void AUTGameMode::CheckCountDown()
{
	if (CountDown >0)
	{
		// Broadcast the localized message saying when the game begins.
		BroadcastLocalized( this, UUTCountDownMessage::StaticClass(), CountDown, NULL, NULL, NULL);
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::CheckCountDown, 1.0, false);
		CountDown--;
	}
	else
	{
		BeginGame();
	}
}

void AUTGameMode::CheckGameTime()
{
	if (IsMatchInProgress() && !HasMatchEnded() && TimeLimit > 0 && UTGameState->RemainingTime <= 0)
	{
		// Game should be over.. look to see if we need to go in to overtime....	

		uint32 bTied = 0;
		AUTPlayerState* Winner = IsThereAWinner(bTied);

		if (!bAllowOvertime || !bTied)
		{
			EndGame(Winner, FName(TEXT("TimeLimit")));			
		}
		else if (bAllowOvertime && !UTGameState->IsMatchInOvertime())
		{
			// Stop the clock in Overtime. 
			UTGameState->bStopGameClock = true;
			SetMatchState(MatchState::MatchEnteringOvertime);
		}
	}
}

void AUTGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (MaxReadyWaitTime > 0)
	{
		UTGameState->SetTimeLimit(MaxReadyWaitTime);
	}
}

/**
 *	Look though the player states and see if we have a winner.  If there is a tie, we return
 *  NULL so that we can enter overtime.
 **/
AUTPlayerState* AUTGameMode::IsThereAWinner(uint32& bTied)
{
	AUTPlayerState* BestPlayer = NULL;
	float BestScore = 0.0;

	for (int32 PlayerIdx=0; PlayerIdx < UTGameState->PlayerArray.Num();PlayerIdx++)
	{
		if (UTGameState->PlayerArray[PlayerIdx] != NULL)
		{
			if (BestPlayer == NULL || UTGameState->PlayerArray[PlayerIdx]->Score > BestScore)
			{
				BestPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
				BestScore = BestPlayer->Score;
				bTied = 0;
			}
			else if (UTGameState->PlayerArray[PlayerIdx]->Score == BestScore)
			{
				bTied = 1;
			}
		}
	}

	return BestPlayer;
}

void AUTGameMode::OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState)
{
	Super::OverridePlayerState(PC, OldPlayerState);

	// if we're in this function GameMode swapped PlayerState objects so we need to update the precasted copy
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
	if (UTPC != NULL)
	{
		UTPC->UTPlayerState = Cast<AUTPlayerState>(UTPC->PlayerState);
	}
}

void AUTGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	if (BaseMutator != NULL)
	{
		BaseMutator->PostPlayerInit(C);
	}

	UpdatePlayersPresence();

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		if (C && Cast<AUTPlayerController>(C) && C->PlayerState)
		{
			AUTPlayerState* PlayerState = Cast<AUTPlayerState>(C->PlayerState);
			if (PlayerState)
			{
				LobbyBeacon->UpdatePlayer(PlayerState->UniqueId, PlayerState->PlayerName, int32(PlayerState->Score), PlayerState->bOnlySpectator, false, PlayerState->AverageRank);
			}
		}
	}



}

void AUTGameMode::PostLogin( APlayerController* NewPlayer )
{
	TSubclassOf<AHUD> SavedHUDClass = HUDClass;

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(NewPlayer);
	bool bIsCastingGuidePC = UTPC != NULL && UTPC->CastingGuideViewIndex >= 0 && GameState->PlayerArray.IsValidIndex(UTPC->CastingGuideViewIndex);
	if (bIsCastingGuidePC)
	{
		HUDClass = CastingGuideHUDClass;
	}

	Super::PostLogin(NewPlayer);

	NewPlayer->ClientSetLocation(NewPlayer->GetFocalLocation(), NewPlayer->GetControlRotation());
	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UpdateGameState();
		}
	}

	
	if (bIsCastingGuidePC)
	{
		// TODO: better choice of casting views
		UTPC->ServerViewPlayerState(Cast<AUTPlayerState>(GameState->PlayerArray[UTPC->CastingGuideViewIndex]));
	}

	CheckBotCount();

	HUDClass = SavedHUDClass;
}

void AUTGameMode::SwitchToCastingGuide(AUTPlayerController* NewCaster)
{
	// TODO: check if allowed
	if (NewCaster != NULL && !NewCaster->bCastingGuide && NewCaster->PlayerState->bOnlySpectator)
	{
		NewCaster->bCastingGuide = true;
		NewCaster->CastingGuideViewIndex = 0;
		NewCaster->ClientSetHUD(CastingGuideHUDClass);
	}
}

void AUTGameMode::Logout(AController* Exiting)
{
	if (BaseMutator != NULL)
	{
		BaseMutator->NotifyLogout(Exiting);
	}

	// Let's Analytics know how long this player has been online....

	AUTPlayerState* PS = Cast<AUTPlayerState>(Exiting->PlayerState);
		
	if (PS != NULL && FUTAnalytics::IsAvailable())
	{
		float TotalTimeOnline = GameState->ElapsedTime - PS->StartTime;
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ID"), PS->StatsID));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("PlayerName"), PS->PlayerName));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("TimeOnline"), TotalTimeOnline));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Kills"), PS->Kills));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Deaths"), PS->Deaths));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Score"), PS->Score));
		FUTAnalytics::GetProvider().RecordEvent( TEXT("PlayerLogoutStat"), ParamArray );
		PS->RespawnChoiceA = NULL;
		PS->RespawnChoiceB = NULL;
	}

	if (Cast<AUTBot>(Exiting) != NULL)
	{
		NumBots--;
	}

	Super::Logout(Exiting);

	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UpdateGameState();
		}
	}

	UpdatePlayersPresence();

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		if ( PS->GetOwner() && Cast<AUTPlayerController>(PS->GetOwner()) )
		{
			LobbyBeacon->UpdatePlayer(PS->UniqueId, PS->PlayerName, int32(PS->Score), PS->bOnlySpectator, true, PS->AverageRank);
		}
	}

}

bool AUTGameMode::PlayerCanRestart_Implementation( APlayerController* Player )
{
	// Can't restart in overtime
	if (bOnlyTheStrongSurvive && UTGameState->IsMatchInOvertime())
	{
		return false;
	}
	else
	{
		return Super::PlayerCanRestart_Implementation(Player);
	}
}

bool AUTGameMode::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
	AUTCharacter* InjuredChar = Cast<AUTCharacter>(Injured);
	if (InjuredChar != NULL && InjuredChar->bSpawnProtectionEligible && InstigatedBy != NULL && InstigatedBy != Injured->Controller && GetWorld()->TimeSeconds - Injured->CreationTime < UTGameState->SpawnProtectionTime)
	{
		Damage = 0;
	}

	if (BaseMutator != NULL)
	{
		BaseMutator->ModifyDamage(Damage, Momentum, Injured, InstigatedBy, HitInfo, DamageCauser, DamageType);
	}

	return true;
}

bool AUTGameMode::CheckRelevance_Implementation(AActor* Other)
{
	if (BaseMutator == NULL)
	{
		return true;
	}
	else
	{
		bool bPreventModify = false;
		bool bForceKeep = BaseMutator->AlwaysKeep(Other, bPreventModify);
		if (bForceKeep && bPreventModify)
		{
			return true;
		}
		else
		{
			return (BaseMutator->CheckRelevance(Other) || bForceKeep);
		}
	}
}

void AUTGameMode::SetWorldGravity(float NewGravity)
{
	AWorldSettings* Settings = GetWorld()->GetWorldSettings();
	Settings->bWorldGravitySet = true;
	Settings->WorldGravityZ = NewGravity;
}

bool AUTGameMode::ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast)
{
	// By default, we don't do anything.
	return true;
}

TSubclassOf<AGameSession> AUTGameMode::GetGameSessionClass() const
{
	return AUTGameSession::StaticClass();
}

void AUTGameMode::ScoreObject_Implementation(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason)
{
	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreObject(GameObject, HolderPawn, Holder, Reason);
	}
}

void AUTGameMode::GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToEntry, ActorList);

	for (AUTMutator* Mut = BaseMutator; Mut != NULL; Mut = Mut->NextMutator)
	{
		Mut->GetSeamlessTravelActorList(bToEntry, ActorList);
	}
}

void AUTGameMode::GetGameURLOptions(TArray<FString>& OptionsList, int32& DesiredPlayerCount)
{
	OptionsList.Add(FString::Printf(TEXT("TimeLimit=%i"), TimeLimit));
	OptionsList.Add(FString::Printf(TEXT("GoalScore=%i"), GoalScore));
	OptionsList.Add(FString::Printf(TEXT("bForceRespawn=%i"), bForceRespawn));

	DesiredPlayerCount = BotFillCount;
}

#if !UE_SERVER
void AUTGameMode::CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps)
{
	TSharedPtr< TAttributeProperty<int32> > TimeLimitAttr = MakeShareable(new TAttributeProperty<int32>(this, &TimeLimit, TEXT("TimeLimit")));
	ConfigProps.Add(TimeLimitAttr);
	TSharedPtr< TAttributeProperty<int32> > GoalScoreAttr = MakeShareable(new TAttributeProperty<int32>(this, &GoalScore, TEXT("GoalScore")));
	ConfigProps.Add(GoalScoreAttr);
	TSharedPtr< TAttributePropertyBool > ForceRespawnAttr = MakeShareable(new TAttributePropertyBool(this, &bForceRespawn, TEXT("ForceRespawn")));
	ConfigProps.Add(ForceRespawnAttr);
	TSharedPtr< TAttributeProperty<int32> > CombatantsAttr = MakeShareable(new TAttributeProperty<int32>(this, &BotFillCount, TEXT("BotFill")));
	ConfigProps.Add(CombatantsAttr);

	// FIXME: temp 'ReadOnly' handling by creating new widgets; ideally there would just be a 'disabled' or 'read only' state in Slate...
	MenuSpace->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(0.0f,0.0f,0.0f,5.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 5.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(350)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.Text(NSLOCTEXT("UTGameMode", "NumCombatants", "Number of Combatants"))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(20.0f,0.0f,0.0f,0.0f)
		[
			SNew(SBox)
			.WidthOverride(300)
			[
				bCreateReadOnly ?
				StaticCastSharedRef<SWidget>(
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
					.Text(CombatantsAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
				) :
				StaticCastSharedRef<SWidget>(
					SNew(SNumericEntryBox<int32>)
					.Value(CombatantsAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
					.OnValueChanged(CombatantsAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
					.AllowSpin(true)
					.Delta(1)
					.MinValue(1)
					.MaxValue(32)
					.MinSliderValue(1)
					.MaxSliderValue(32)
					.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")

				)
			]
		]
	];
	MenuSpace->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(0.0f,0.0f,0.0f,5.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(350)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(),"UT.Common.NormalText")
				.Text(NSLOCTEXT("UTGameMode", "GoalScore", "Goal Score"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(20.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300)
			[
				bCreateReadOnly ?
				StaticCastSharedRef<SWidget>(
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(),"UT.Common.ButtonText.White")
					.Text(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
				) :
				StaticCastSharedRef<SWidget>(
					SNew(SNumericEntryBox<int32>)
					.Value(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
					.OnValueChanged(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
					.AllowSpin(true)
					.Delta(1)
					.MinValue(0)
					.MaxValue(999)
					.MinSliderValue(0)
					.MaxSliderValue(99)
					.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
				)
			]
		]
	];
	MenuSpace->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(0.0f,0.0f,0.0f,5.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(350)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(),"UT.Common.NormalText")
				.Text(NSLOCTEXT("UTGameMode", "TimeLimit", "Time Limit"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(20.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300)
			[
				bCreateReadOnly ?
				StaticCastSharedRef<SWidget>(
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
				.Text(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
				) :
				StaticCastSharedRef<SWidget>(
				SNew(SNumericEntryBox<int32>)
				.Value(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
				.OnValueChanged(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
				.AllowSpin(true)
				.Delta(1)
				.MinValue(0)
				.MaxValue(999)
				.MinSliderValue(0)
				.MaxSliderValue(60)
				.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
				)
			]
		]
	];
	MenuSpace->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Top)
	.Padding(0.0f,0.0f,0.0f,5.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(350)
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(),"UT.Common.NormalText")
				.Text(NSLOCTEXT("UTGameMode", "ForceRespawn", "Force Respawn"))
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(20.0f, 0.0f, 0.0f, 10.0f)
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(300)
			[
				bCreateReadOnly ?
				StaticCastSharedRef<SWidget>(
					SNew(SCheckBox)
					.IsChecked(ForceRespawnAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
					.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
					.ForegroundColor(FLinearColor::White)
					.Type(ESlateCheckBoxType::CheckBox)
				) :
				StaticCastSharedRef<SWidget>(
					SNew(SCheckBox)
					.IsChecked(ForceRespawnAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
					.OnCheckStateChanged(ForceRespawnAttr.ToSharedRef(), &TAttributePropertyBool::SetFromCheckBox)
					.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
					.ForegroundColor(FLinearColor::White)
					.Type(ESlateCheckBoxType::CheckBox)
				)
			]
		]
	];

}


#endif



void AUTGameMode::ProcessServerTravel(const FString& URL, bool bAbsolute)
{
	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UnRegisterServer(false);
		}
	}

	Super::ProcessServerTravel(URL, bAbsolute);
}

FText AUTGameMode::BuildServerRules(AUTGameState* GameState)
{
	return FText::Format(NSLOCTEXT("UTGameMode", "GameRules", "{0} - GoalScore: {1}  Time Limit: {2}"), DisplayName, FText::AsNumber(GameState->GoalScore), (GameState->TimeLimit > 0) ? FText::Format(NSLOCTEXT("UTGameMode", "TimeMinutes", "{0} min"), FText::AsNumber(uint32(GameState->TimeLimit / 60))) : NSLOCTEXT("General", "None", "None"));
}

void AUTGameMode::BuildServerResponseRules(FString& OutRules)
{
	// TODO: need to rework this so it can be displayed in the clien't local language
	OutRules += FString::Printf(TEXT("Goal Score\t%i\t"), GoalScore);
	OutRules += FString::Printf(TEXT("Time Limit\t%i\t"), int32(TimeLimit/60.0));
	OutRules += FString::Printf(TEXT("Allow Overtime\t%s\t"), bAllowOvertime ? TEXT("True") : TEXT("False"));
	OutRules += FString::Printf(TEXT("Forced Respawn\t%s\t"), bForceRespawn ?  TEXT("True") : TEXT("False"));
	OutRules += FString::Printf(TEXT("Only The Strong\t%s\t"), bOnlyTheStrongSurvive ? TEXT("True") : TEXT("False"));

	AUTMutator* Mut = BaseMutator;
	while (Mut)
	{
		OutRules += FString::Printf(TEXT("Mutator\t%s\t"), *Mut->DisplayName.ToString());
		Mut = Mut->NextMutator;
	}
}

void AUTGameMode::BlueprintBroadcastLocalized( AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	BroadcastLocalized(Sender, Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

void AUTGameMode::BlueprintSendLocalized( AActor* Sender, AUTPlayerController* Receiver, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	Receiver->ClientReceiveLocalizedMessage(Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

void AUTGameMode::BroadcastSpectator(AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = (*Iterator);
		if (PC->PlayerState != nullptr && PC->PlayerState->bOnlySpectator)
		{
			PC->ClientReceiveLocalizedMessage(Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
		}
	}
}

void AUTGameMode::BroadcastSpectatorPickup(AUTPlayerState* PS, const AUTInventory* Inventory)
{
	if (PS != nullptr && Inventory != nullptr && Inventory->StatsNameCount != NAME_None)
	{
		int32 PlayerNumPickups = (int32)PS->GetStatsValue(Inventory->StatsNameCount);
		int32 TotalPickups = (int32)UTGameState->GetStatsValue(Inventory->StatsNameCount);

		//Stats may not have been replicated to the client so pack them in the switch
		int32 Switch = TotalPickups << 16 | PlayerNumPickups;

		BroadcastSpectator(nullptr, UUTSpectatorPickupMessage::StaticClass(), Switch, PS, nullptr, Inventory->GetClass());
	}
}

void AUTGameMode::PrecacheAnnouncements(UUTAnnouncer* Announcer) const
{
	// slow but fairly reliable base implementation that looks up all local messages
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UUTLocalMessage::StaticClass()))
		{
			It->GetDefaultObject<UUTLocalMessage>()->PrecacheAnnouncements(Announcer);
		}
		if (It->IsChildOf(UUTDamageType::StaticClass()))
		{
			It->GetDefaultObject<UUTDamageType>()->PrecacheAnnouncements(Announcer);
		}
	}
}

void AUTGameMode::AssignDefaultSquadFor(AController* C)
{
	if (C != NULL)
	{
		if (SquadType == NULL)
		{
			UE_LOG(UT, Warning, TEXT("Game mode %s missing SquadType"), *GetName());
			SquadType = AUTSquadAI::StaticClass();
		}
		AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
		if (PS != NULL && PS->Team != NULL)
		{
			PS->Team->AssignDefaultSquadFor(C);
		}
		else
		{
			// default is to just spawn a squad for each individual
			AUTBot* B = Cast<AUTBot>(C);
			if (B != NULL)
			{
				B->SetSquad(GetWorld()->SpawnActor<AUTSquadAI>(SquadType));
			}
		}
	}
}

void AUTGameMode::NotifyLobbyGameIsReady()
{
	if (IsGameInstanceServer() && LobbyBeacon)
	{
		LobbyBeacon->Lobby_NotifyInstanceIsReady(LobbyInstanceID, ServerInstanceGUID);
	}
}

void AUTGameMode::UpdateLobbyMatchStats(FString Update)
{
	// Update the players

	UpdateLobbyPlayerList();
	UpdateLobbyBadge(TEXT(""));

	if (ensure(LobbyBeacon) && UTGameState)
	{
		// Add the time remaining command
		if (Update != TEXT("")) Update += TEXT("?");
		Update += FString::Printf(TEXT("GameTime=%i"), TimeLimit > 0 ? UTGameState->RemainingTime : UTGameState->ElapsedTime);
		LobbyBeacon->UpdateMatch(Update);
	}

	LastLobbyUpdateTime = GetWorld()->GetTimeSeconds();
}

void AUTGameMode::UpdateLobbyPlayerList()
{
	if (ensure(LobbyBeacon))
	{
		for (int32 i=0;i<UTGameState->PlayerArray.Num();i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
			if ( PS->GetOwner() && Cast<AUTPlayerController>(PS->GetOwner()) )
			{
				LobbyBeacon->UpdatePlayer(PS->UniqueId, PS->PlayerName, int32(PS->Score), PS->bOnlySpectator, false, PS->AverageRank);
			}
		}
	}
}

void AUTGameMode::UpdateLobbyBadge(FString BadgeText)
{
	if (BadgeText != "") BadgeText += TEXT("\n");

	AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
	FString MapName = GetWorld()->GetMapName();
	if (WS)
	{
		const UUTLevelSummary* Summary = WS->GetLevelSummary();
		if ( Summary && Summary->Title != TEXT("") )
		{
			MapName = Summary->Title;
		}
	}

	BadgeText += FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Small>%s</>\n<UWindows.Standard.MatchBadge.Small>%i Players</>"), *MapName, NumPlayers);

	if (BadgeText != TEXT("") && ensure(LobbyBeacon))
	{
		LobbyBeacon->Lobby_UpdateBadge(LobbyInstanceID, BadgeText);
	}

}


void AUTGameMode::SendEveryoneBackToLobby()
{
	// Game Instance Servers just tell everyone to just return to the lobby.
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller)
		{
			Controller->ClientReturnToLobby();
		}
	}
}

#if !UE_SERVER
FString AUTGameMode::GetHUBPregameFormatString()
{
	return FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Header>%s</>\n\n<UWindows.Standard.MatchBadge.Small>Host</>\n<UWindows.Standard.MatchBadge>{Player0Name}</>\n\n<UWindows.Standard.MatchBadge.Small>({NumPlayers} Players)</>"), *DisplayName.ToString());
}
#endif

void AUTGameMode::UpdatePlayersPresence()
{
	bool bAllowJoin = (NumPlayers < GameSession->MaxPlayers);
	UE_LOG(UT,Verbose,TEXT("AllowJoin: %i %i %i"), bAllowJoin, NumPlayers, GameSession->MaxPlayers);
	FString PresenceString = FText::Format(NSLOCTEXT("UTGameMode","PlayingPresenceFormatStr","Playing {0} on {1}"), DisplayName, FText::FromString(*GetWorld()->GetMapName())).ToString();
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller)
		{
			Controller->ClientSetPresence(PresenceString, bAllowJoin, bAllowJoin, bAllowJoin, false);
		}
	}
}

#if !UE_SERVER
void AUTGameMode::NewPlayerInfoLine(TSharedPtr<SVerticalBox> VBox, FText DisplayName, TSharedPtr<TAttributeStat> Stat, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	//Add stat in here for layout convenience
	StatList.Add(Stat);

	VBox->AddSlot()
	.AutoHeight()
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(DisplayName)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor::Gray)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(Stat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
			]
		]
	];
}

void AUTGameMode::NewWeaponInfoLine(TSharedPtr<SVerticalBox> VBox, FText DisplayName, TSharedPtr<TAttributeStat> KillStat, TSharedPtr<TAttributeStat> DeathStat, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	//Add stat in here for layout convenience
	StatList.Add(KillStat);
	StatList.Add(DeathStat);

	VBox->AddSlot()
	.AutoHeight()
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(DisplayName)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor::Gray)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			.HAlign(HAlign_Fill)

			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(KillStat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor(0.6f, 1.0f, 0.6f))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.2f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(DeathStat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor(1.0f,0.6f,0.6f))
			]
		]
	];
}

void AUTGameMode::BuildPaneHelper(TSharedPtr<SHorizontalBox>& HBox, TSharedPtr<SVerticalBox>& LeftPane, TSharedPtr<SVerticalBox>& RightPane)
{
	SAssignNew(HBox, SHorizontalBox)
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Fill)
	.Padding(20.0f, 20.0f, 20.0f, 10.0f)
	[
		SAssignNew(LeftPane, SVerticalBox)
	]
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Fill)
	.Padding(20.0f, 20.0f, 20.0f, 10.0f)
	[
		SAssignNew(RightPane, SVerticalBox)
	];
}

void AUTGameMode::BuildScoreInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TAttributeStat::StatValueTextFunc TwoDecimal = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> FText
	{
		return FText::FromString(FString::Printf(TEXT("%8.2f"), Stat->GetValue()));
	};

	TSharedPtr<SVerticalBox> LeftPane;
	TSharedPtr<SVerticalBox> RightPane;
	TSharedPtr<SHorizontalBox> HBox;
	BuildPaneHelper(HBox, LeftPane, RightPane);

	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Score", "Score"), HBox);

	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Kills", "Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float { return PS->Kills;	})), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Deaths", "Deaths"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float {	return PS->Deaths; })), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Suicides", "Suicides"), MakeShareable(new TAttributeStat(PlayerState, NAME_Suicides)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "ScorePM", "Score Per Minute"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		return (PS->StartTime <  PS->GetWorld()->GameState->ElapsedTime) ? PS->Score * 60.f / (PS->GetWorld()->GameState->ElapsedTime - PS->StartTime) : 0.f;
	}, TwoDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "KDRatio", "K/D Ratio"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		return (PS->Deaths > 0) ? float(PS->Kills) / PS->Deaths : 0.f;
	}, TwoDecimal)), StatList);

	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "BeltPickups", "Shield Belt Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_ShieldBeltCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "VestPickups", "Armor Vest Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_ArmorVestCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "PadPickups", "Thigh Pad Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_ArmorPadsCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "HelmetPickups", "Helmet Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_HelmetCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "JumpBootJumps", "JumpBoot Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_BootJumps)), StatList);
	RightPane->AddSlot()[SNew(SSpacer).Size(FVector2D(0.0f, 20.0f))];

	TAttributeStat::StatValueTextFunc ToTime = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> FText
	{
		int32 Seconds = (int32)Stat->GetValue();
		int32 Mins = Seconds / 60;
		Seconds -= Mins * 60;
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Mins, Seconds));
	};

	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "UDamage", "UDamage Control"), MakeShareable(new TAttributeStat(PlayerState, NAME_UDamageTime, nullptr, ToTime)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "Berserk", "Berserk Control"), MakeShareable(new TAttributeStat(PlayerState, NAME_BerserkTime, nullptr, ToTime)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "Invisibility", "Invisibility Control"), MakeShareable(new TAttributeStat(PlayerState, NAME_InvisibilityTime, nullptr, ToTime)), StatList);
}

void AUTGameMode::BuildRewardInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TSharedPtr<SVerticalBox> LeftPane;
	TSharedPtr<SVerticalBox> RightPane;
	TSharedPtr<SHorizontalBox> HBox;
	BuildPaneHelper(HBox, LeftPane, RightPane);

	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Rewards", "Rewards"), HBox);

	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "DoubleKills", "Double Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel0)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "MultiKills", "Multi Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel1)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "UltraKills", "Ultra Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel2)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "MonsterKills", "Monster Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel3)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "KillingSprees", "Killing Spree"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel0)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "RampageSprees", "Rampage"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel1)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "DominatingSprees", "Dominating"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel2)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "UnstoppableSprees", "Unstoppable"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel3)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "GodlikeSprees", "Godlike"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel4)), StatList);
}

void AUTGameMode::BuildWeaponInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TSharedPtr<SVerticalBox> TopLeftPane;
	TSharedPtr<SVerticalBox> TopRightPane;
	TSharedPtr<SVerticalBox> BotLeftPane;
	TSharedPtr<SVerticalBox> BotRightPane;
	TSharedPtr<SHorizontalBox> TopBox;
	TSharedPtr<SHorizontalBox> BotBox;
	BuildPaneHelper(TopBox, TopLeftPane, TopRightPane);
	BuildPaneHelper(BotBox, BotLeftPane, BotRightPane);

	//4x4 panes
	TSharedPtr<SVerticalBox> MainBox = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		TopBox.ToSharedRef()
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		BotBox.ToSharedRef()
	];
	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Weapons", "Weapons"), MainBox);

	//Get the weapons used in this map
	TArray<AUTWeapon *> StatsWeapons;
	{
		// add default weapons - needs to be automated
		StatsWeapons.AddUnique(AUTWeap_ImpactHammer::StaticClass()->GetDefaultObject<AUTWeapon>());
		StatsWeapons.AddUnique(AUTWeap_Enforcer::StaticClass()->GetDefaultObject<AUTWeapon>());

		//Get the rest of the weapons from the pickups in the map
		for (FActorIterator It(PlayerState->GetWorld()); It; ++It)
		{
			AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
			if (Pickup && Pickup->GetInventoryType())
			{
				StatsWeapons.AddUnique(Pickup->GetInventoryType()->GetDefaultObject<AUTWeapon>());
			}
		}
		StatsWeapons.AddUnique(AUTWeap_Translocator::StaticClass()->GetDefaultObject<AUTWeapon>());
	}

	//Add weapons to the panes
	for (int32 i = 0; i < StatsWeapons.Num();i++)
	{
		TSharedPtr<SVerticalBox> Pane = (i % 2) ? TopRightPane : TopLeftPane;
		NewWeaponInfoLine(Pane, StatsWeapons[i]->DisplayName,
			MakeShareable(new TAttributeStatWeapon(PlayerState, StatsWeapons[i], true)),
			MakeShareable(new TAttributeStatWeapon(PlayerState, StatsWeapons[i], false)),
			StatList);
	}

	NewPlayerInfoLine(BotLeftPane, NSLOCTEXT("AUTGameMode", "ShockComboKills", "Shock Combo Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_ShockComboKills)), StatList);
	NewPlayerInfoLine(BotLeftPane, NSLOCTEXT("AUTGameMode", "AmazingCombos", "Amazing Combos"), MakeShareable(new TAttributeStat(PlayerState, NAME_AmazingCombos)), StatList);
	NewPlayerInfoLine(BotLeftPane, NSLOCTEXT("AUTGameMode", "HeadShots", "Sniper Headshots"), MakeShareable(new TAttributeStat(PlayerState, NAME_SniperHeadshotKills)), StatList);
	NewPlayerInfoLine(BotRightPane, NSLOCTEXT("AUTGameMode", "AirRox", "Air Rocket Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_AirRox)), StatList);
	NewPlayerInfoLine(BotRightPane, NSLOCTEXT("AUTGameMode", "FlakShreds", "Flak Shreds"), MakeShareable(new TAttributeStat(PlayerState, NAME_FlakShreds)), StatList);
	NewPlayerInfoLine(BotRightPane, NSLOCTEXT("AUTGameMode", "AirSnot", "Air Snot Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_AirSnot)), StatList);
}

void AUTGameMode::BuildMovementInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TAttributeStat::StatValueFunc ConvertToMeters = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		return 0.01f * PS->GetStatsValue(Stat->StatName);
	};

	TAttributeStat::StatValueTextFunc OneDecimal = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> FText
	{
		return FText::FromString(FString::Printf(TEXT("%8.1fm"), Stat->GetValue()));
	};

	TSharedPtr<SVerticalBox> LeftPane;
	TSharedPtr<SVerticalBox> RightPane;
	TSharedPtr<SHorizontalBox> HBox;
	BuildPaneHelper(HBox, LeftPane, RightPane);

	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Movement", "Movement"), HBox);

	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "RunDistance", "Run Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_RunDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "SprintDistance", "Sprint Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_SprintDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "SlideDistance", "Slide Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_SlideDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "WallRunDistance", "WallRun Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_WallRunDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "FallDistance", "Fall Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_InAirDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "SwimDistance", "Swim Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_SwimDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "TranslocDistance", "Teleport Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_TranslocDist, ConvertToMeters, OneDecimal)), StatList);
	LeftPane->AddSlot()[SNew(SSpacer).Size(FVector2D(0.0f, 20.0f))];
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Total", "Total"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		float Total = 0.0f;
		Total += PS->GetStatsValue(NAME_RunDist);
		Total += PS->GetStatsValue(NAME_SprintDist);
		Total += PS->GetStatsValue(NAME_SlideDist);
		Total += PS->GetStatsValue(NAME_WallRunDist);
		Total += PS->GetStatsValue(NAME_InAirDist);
		Total += PS->GetStatsValue(NAME_TranslocDist);
		return 0.01f * Total;
	}, OneDecimal)), StatList);

	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumJumps", "Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumJumps)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumDodges", "Dodges"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumDodges)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumWallDodges", "Wall Dodges"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumWallDodges)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumLiftJumps", "Lift Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumLiftJumps)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumFloorSlides", "Floor Slides"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumFloorSlides)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumWallRuns", "Wall Runs"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumWallRuns)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumImpactJumps", "Impact Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumImpactJumps)), StatList);
	//NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumRocketJumps", "Rocket Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumRocketJumps)), StatList);
}

void AUTGameMode::BuildPlayerInfo(AUTPlayerState* PlayerState, TSharedPtr<SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	//These need to be in the same order as they are in the scoreboard. Replication of stats are done per tab
	BuildScoreInfo(PlayerState, TabWidget, StatList);
	BuildWeaponInfo(PlayerState, TabWidget, StatList);
	BuildRewardInfo(PlayerState, TabWidget, StatList);
	BuildMovementInfo(PlayerState, TabWidget, StatList);
}
#endif

bool AUTGameMode::ValidateHat(AUTPlayerState* HatOwner, const FString& HatClass)
{
	// Load the hat and make sure it's not override only.
	UClass* HatClassObject = LoadClass<AUTHat>(NULL, *HatClass, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (HatClassObject)
	{
		AUTHat* DefaultHat = HatClassObject->GetDefaultObject<AUTHat>();
		if (DefaultHat && !DefaultHat->bOverrideOnly)
		{
			return true;
		}
	}
	
	return false;
}

bool AUTGameMode::PlayerCanAltRestart_Implementation( APlayerController* Player )
{
	return PlayerCanRestart(Player);
}

void AUTGameMode::BecomeDedicatedInstance(FGuid HubGuid)
{
	UTGameState->bIsInstanceServer = true;
	UTGameState->HubGuid = HubGuid;
	UE_LOG(UT,Log,TEXT("Becoming a Dedicated Instance"));
}

void AUTGameMode::HandleMapVote()
{

	// Force at least 20 seconds of map vote time.
	if (MapVoteTime < 20) MapVoteTime = 20;

	UTGameState->VoteTimer = MapVoteTime;
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::TallyMapVotes, MapVoteTime+1);	
	FTimerHandle TempHandle2;
	GetWorldTimerManager().SetTimer(TempHandle2, this, &AUTGameMode::CullMapVotes, MapVoteTime-10);	
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL && !PC->PlayerState->bOnlySpectator)
		{
			PC->ClientShowMapVote();
		}
	}
}

/**
 *	With 10 seconds to go in map voting, cull the list of voteable maps to no more than 6.  
 **/
void AUTGameMode::CullMapVotes()
{
	TArray<AUTReplicatedMapInfo*> Sorted;
	TArray<AUTReplicatedMapInfo*> DeleteList;
	for (int32 i=0; i< UTGameState->MapVoteList.Num(); i++)
	{
		int32 InsertIndex = 0;
		while (InsertIndex < Sorted.Num() && Sorted[InsertIndex]->VoteCount >= UTGameState->MapVoteList[i]->VoteCount)
		{
			InsertIndex++;
		}
		
		Sorted.Insert(UTGameState->MapVoteList[i], InsertIndex);
	}

	// If noone has voted, then randomly pick 6 maps

	int32 ForcedSize = 3;
	if (Sorted.Num() > 0)
	{
		if (Sorted[0]->VoteCount == 0)	// Top map has 0 votes so no one has voted
		{
			ForcedSize = 6;
			while (Sorted.Num() > 0)
			{
				DeleteList.Add(Sorted[Sorted.Num()-1]);
				Sorted.RemoveAt(Sorted.Num()-1,1);
			}
		}
		else 
		{
			// Remove any maps with 0 votes.

			int32 ZeroIndex = Sorted.Num()-1;
			while (ZeroIndex > 0)
			{
				if (Sorted[ZeroIndex]->VoteCount == 0)
				{
					DeleteList.Add(Sorted[ZeroIndex]);
					Sorted.RemoveAt(ZeroIndex,1);
				}
				ZeroIndex--;
			}

			// If we have more than 6 maps left to vote on, then find the # of votes of map 6 and cull anything with less votes.

			if (Sorted.Num() > 6)
			{
				int32 Idx = 5;
				while (Idx < Sorted.Num() && Sorted[Idx+1]->VoteCount == Sorted[Idx]->VoteCount)
				{
					Idx++;
				}

				while (Sorted.Num() > Idx)
				{
					DeleteList.Add(Sorted[Idx]);
					Sorted.RemoveAt(Idx,1);
				}
			}
		}
	}

	if (Sorted.Num() < ForcedSize)
	{
		// We want at least 3 maps to choose from.. so add a few back
		while (Sorted.Num() < ForcedSize && DeleteList.Num() > 0)
		{
			int32 RandIdx = FMath::RandRange(0, DeleteList.Num()-1);
			Sorted.Add(DeleteList[RandIdx]);
			DeleteList.RemoveAt(RandIdx,1);
		}
	}

	UE_LOG(UT,Log, TEXT("Culling Votes: %i %i"), Sorted.Num(), DeleteList.Num());

	UTGameState->MapVoteList.Empty();
	for (int32 i=0; i < Sorted.Num(); i++)
	{
		UTGameState->MapVoteList.Add(Sorted[i]);
	}

	for (int32 i=0; i<DeleteList.Num(); i++)
	{
		// Kill the actor
		DeleteList[i]->Destroy();
	}
}

void AUTGameMode::TallyMapVotes()
{

	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL)
		{
			PC->ClientHideMapVote();
		}
	}


	TArray<AUTReplicatedMapInfo*> Best;
	for (int32 i=0; i< UTGameState->MapVoteList.Num(); i++)
	{
		if (Best.Num() == 0 || Best[0]->VoteCount < UTGameState->MapVoteList[i]->VoteCount)
		{
			Best.Empty();
			Best.Add(UTGameState->MapVoteList[i]);
		}
	}

	if (Best.Num() > 0)
	{
		int32 Idx = FMath::RandRange(0, Best.Num() - 1);
		GetWorld()->ServerTravel(Best[Idx]->MapPackageName, false);
	}
	else
	{
		SendEveryoneBackToLobby();
	}
}
