// Fill out your copyright notice in the Description page of Project Settings.


#include "ChattingManager.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

// Sets default values
AChattingManager::AChattingManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AChattingManager::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AChattingManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}