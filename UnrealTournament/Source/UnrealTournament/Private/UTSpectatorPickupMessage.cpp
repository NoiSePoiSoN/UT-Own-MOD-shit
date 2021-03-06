// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTSpectatorPickupMessage.h"

UUTSpectatorPickupMessage::UUTSpectatorPickupMessage(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MessageArea = FName(TEXT("GameMessages"));
	StyleTag = FName(TEXT("Default"));

	bIsStatusAnnouncement = true;
	bIsPartiallyUnique = true;

	PickupFormat = NSLOCTEXT("UUTSpectatorPickupMessage", "PickupFormat", "{PlayerName} picked up the {DisplayName}   ( {Num}  / {TotalNum} )");
}


FText UUTSpectatorPickupMessage::ResolveMessage_Implementation(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	const AUTInventory* Inventory = Cast<UClass>(OptionalObject) ? Cast<AUTInventory>(Cast<UClass>(OptionalObject)->GetDefaultObject()) : NULL;
	if (RelatedPlayerState_1 != nullptr && Inventory != nullptr)
	{
		int32 PlayerPickups = Switch & 0xFFFF;
		int32 TotalPickups = Switch >> 16;
		
		FFormatNamedArguments Args;
		Args.Add(TEXT("PlayerName"), FText::FromString(RelatedPlayerState_1->PlayerName));
		Args.Add(TEXT("DisplayName"), Inventory->DisplayName);
		Args.Add(TEXT("Num"), PlayerPickups);
		Args.Add(TEXT("TotalNum"), TotalPickups);

		return FText::Format(PickupFormat, Args);
	}
	return NSLOCTEXT("PickupMessage", "NoPickupMessage", "No Pickup Message");
}

bool UUTSpectatorPickupMessage::UseLargeFont(int32 MessageIndex) const
{
	return false;
}

