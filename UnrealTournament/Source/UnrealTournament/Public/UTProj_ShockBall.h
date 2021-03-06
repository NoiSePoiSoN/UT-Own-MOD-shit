// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTProjectile.h"
#include "UTPhysicsVortex.h"
#include "UTProj_ShockBall.generated.h"

UCLASS(Abstract)
class UNREALTOURNAMENT_API AUTProj_ShockBall : public AUTProjectile
{
	GENERATED_UCLASS_BODY()

	virtual void InitFakeProjectile(AUTPlayerController* OwningPlayer) override;
	virtual void NotifyClientSideHit(AUTPlayerController* InstigatedBy, FVector HitLocation, AActor* DamageCauser) override;

	/** combo parameters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShockCombo)
	FRadialDamageParams ComboDamageParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShockCombo)
	float ComboMomentum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShockCombo)
	TSubclassOf<UUTDamageType> ComboDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShockCombo)
	TSubclassOf<UUTDamageType> ComboTriggerType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ShockCombo)
	int32 ComboAmmoCost;

	/** Effects */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TSubclassOf<AUTImpactEffect> ComboExplosionEffects;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TSubclassOf<AUTPhysicsVortex> ComboVortexType;

protected:
	/** when set and InstigatorController is a bot, ask it when we should combo */
	bool bMonitorBotCombo;
public:
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_ComboExplosion)
	bool bComboExplosion;
	UFUNCTION()
	void OnRep_ComboExplosion();

	/** hook to do some additional shock combo stuff; called on both client and server */
	UFUNCTION(BlueprintNativeEvent, Category = Damage)
	void OnComboExplode();

	/** Overridden to do the combo */
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void PerformCombo(class AController* InstigatedBy, class AActor* DamageCauser);

	virtual void Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp = NULL);

	virtual float GetMaxDamageRadius_Implementation() const override
	{
		return FMath::Max<float>(DamageParams.OuterRadius, ComboDamageParams.OuterRadius);
	}

	virtual void StartBotComboMonitoring();
	virtual void ClearBotCombo();

	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Reward announcement for impressive combo kill. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Announcement)
		TSubclassOf<class UUTRewardMessage> ComboRewardMessageClass;

	virtual void RateShockCombo(AUTPlayerController *PC, AUTPlayerState* PS, int32 OldComboKillCount);
};
