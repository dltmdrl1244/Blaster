// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "Kismet//GameplayStatics.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectionImpulse = 10.f;
	bShellSoundPlayed = false;
}

void ACasing::BeginPlay()
{
	Super::BeginPlay();

	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);
}

void ACasing::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bShellSoundPlayed)
		return;

	if (ShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
		bShellSoundPlayed = true;
	}
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ACasing::DestroyShell, 2.f, false);
}

void ACasing::DestroyShell()
{
	Destroy();
}
