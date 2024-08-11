// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "Grenade.generated.h"

/**
 *
 */
UCLASS()
class BLASTER_API AGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AGrenade();
	virtual void Destroyed() override;
protected:
	virtual void BeginPlay() override;
};
