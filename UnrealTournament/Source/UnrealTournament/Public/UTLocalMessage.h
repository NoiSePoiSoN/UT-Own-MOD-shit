// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#pragma once


#include "UTLocalMessage.generated.h"

UCLASS(Blueprintable, Abstract, NotPlaceable)
class UUTLocalMessage : public ULocalMessage
{
	GENERATED_UCLASS_BODY()

	/** Message area on HUD (index into UTHUD.MessageOffset[]) */
	UPROPERTY(EditDefaultsOnly, Category = Message)
	FName MessageArea;

	// How much weight should be given to this message.  The MessageWidget will
	// use this number to determine how important the message is. Range is 0-1.
	UPROPERTY(EditDefaultsOnly, Category = Message)
	float Importance;			

	// If true, don't add to normal queue.  
	UPROPERTY(EditDefaultsOnly, Category = Message)
	uint32 bIsSpecial:1;    
	// If true and special, only one can be in the HUD queue at a time.
	UPROPERTY(EditDefaultsOnly, Category = Message)
	uint32 bIsUnique:1;    

	// If true and special, only one can be in the HUD queue with the same switch value
	UPROPERTY(EditDefaultsOnly, Category = Message)
	uint32 bIsPartiallyUnique:1;    

	// If true, put a GetString on the console.
	UPROPERTY(EditDefaultsOnly, Category = Message)
	uint32 bIsConsoleMessage:1;    

	// if true, if sent to HUD multiple times, count up instances (only if bIsUnique)
	UPROPERTY(EditDefaultsOnly, Category = Message)
	uint32 bCountInstances:1;    

	// # of seconds to stay in HUD message queue.
	UPROPERTY(EditDefaultsOnly, Category = Message)
	float Lifetime;    

	virtual void ClientReceive(const FClientReceiveData& ClientData) const OVERRIDE;
	UFUNCTION(BlueprintImplementableEvent)
	void OnClientReceive(APlayerController* LocalPC, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const;
	UFUNCTION(BlueprintNativeEvent)
	FText ResolveMessage(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const;

	/** return the name of announcement to play for this message (if any); UTAnnouncer will map to an actual sound */
	UFUNCTION(BlueprintNativeEvent)
	FName GetAnnouncementName(int32 Switch, const UObject* OptionalObject) const;
	/** return whether this announcement should interrupt/cancel the passed in announcement */
	UFUNCTION(BlueprintNativeEvent)
	bool InterruptAnnouncement(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const;
	/** called when the UTAnnouncer plays the announcement sound - can be used to e.g. display HUD text at the same time */
	UFUNCTION(BlueprintNativeEvent)
	void OnAnnouncementPlayed(int32 Switch, const UObject* OptionalObject) const;

	virtual void GetArgs(FFormatNamedArguments& Args, int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const;
	virtual FText GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const;
	virtual float GetLifeTime(int32 Switch) const;
	virtual bool IsConsoleMessage(int32 Switch) const;
	bool PartiallyDuplicates(int32 Switch1, int32 Switch2, class UObject* OptionalObject1, class UObject* OptionalObject2 );
};



