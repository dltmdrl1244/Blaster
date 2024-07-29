// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Blaster/Blaster.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Weapon/Weapon.h"
#include "DrawDebugHelpers.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}



void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage Package;
	SaveFramePackage(Package);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& Iter : Character->HitCollisionBoxes)
		{
			UBoxComponent* Box = Iter.Value;
			FBoxInformation BoxInformation;
			BoxInformation.Location = Box->GetComponentLocation();
			BoxInformation.Rotation = Box->GetComponentRotation();
			BoxInformation.BoxExtent = Box->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(Iter.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority())
	{
		return;
	}

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& LeftFrame, const FFramePackage& RightFrame, float HitTime)
{
	const float Distance = RightFrame.Time - LeftFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - LeftFrame.Time) / Distance, 0.f, 1.f);
	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& RightIter : RightFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = RightIter.Key;
		const FBoxInformation& LeftBox = LeftFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& RightBox = RightFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(LeftBox.Location, RightBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(LeftBox.Rotation, RightBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = LeftBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (Character == nullptr)
	{
		return FServerSideRewindResult();
	}

	FFramePackage CurrentFrame;
	CacheBoxPosition(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	SetCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable Collision for head first
	SetHeadBoxesCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

	GetWorld()->LineTraceSingleByChannel(
		ConfirmHitResult,
		TraceStart,
		TraceEnd,
		ECC_HitBox
	);
	// Headshot
	if (ConfirmHitResult.bBlockingHit)
	{
		if (ConfirmHitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
			}
		}
		MoveBoxes(HitCharacter, CurrentFrame);
		SetBoxesCollision(HitCharacter, ECollisionEnabled::NoCollision);
		SetCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}

	SetBoxesCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	GetWorld()->LineTraceSingleByChannel(
		ConfirmHitResult,
		TraceStart,
		TraceEnd,
		ECC_HitBox
	);

	MoveBoxes(HitCharacter, CurrentFrame);
	SetBoxesCollision(HitCharacter, ECollisionEnabled::NoCollision);
	SetCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	if (ConfirmHitResult.bBlockingHit)
	{
		if (ConfirmHitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Green, false, 8.f);
			}
		}
		return FServerSideRewindResult{ true, false };
	}
	else
	{
		return FServerSideRewindResult{ false, false };
	}
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (Character == nullptr)
	{
		return FServerSideRewindResult();
	}

	FFramePackage CurrentFrame;
	CacheBoxPosition(HitCharacter, CurrentFrame);
	MoveBoxes(HitCharacter, Package);
	SetCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// Enable Collision for head first
	SetHeadBoxesCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());
	PathParams.DrawDebugTime = 5.f;
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// Headshot
	if (PathResult.HitResult.bBlockingHit)
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f);
			}
		}
		MoveBoxes(HitCharacter, CurrentFrame);
		SetBoxesCollision(HitCharacter, ECollisionEnabled::NoCollision);
		SetCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{ true, true };
	}

	SetBoxesCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// Reset
	MoveBoxes(HitCharacter, CurrentFrame);
	SetBoxesCollision(HitCharacter, ECollisionEnabled::NoCollision);
	SetCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);

	// Bodyshot
	if (PathResult.HitResult.bBlockingHit)
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Green, false, 8.f);
			}
		}
		return FServerSideRewindResult{ true, false };
	}
	else
	{
		return FServerSideRewindResult{ false, false };
	}
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& Packages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	// Enable Collision for head first
	for (auto& Frame : Packages)
	{
		if (Frame.Character == nullptr)
		{
			return FShotgunServerSideRewindResult();
		}
	}
	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : Packages)
	{
		FFramePackage CurrentFrame;
		CacheBoxPosition(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		SetCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrame.Character = Frame.Character;
		CurrentFrames.Add(CurrentFrame);
	}

	// Enable Collision for head
	for (auto& Frame : Packages)
	{
		SetHeadBoxesCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}
	// Check for Headshot
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
		// Headshot
		if (BlasterCharacter)
		{
			if (ShotgunResult.Headshots.Contains(BlasterCharacter))
			{
				ShotgunResult.Headshots[BlasterCharacter]++;
			}
			else
			{
				ShotgunResult.Headshots.Emplace(BlasterCharacter, 1);
			}
		}
	}
	// Disable Collision for head / Enable Collision for rest body
	for (auto& Frame : Packages)
	{
		SetBoxesCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
		SetHeadBoxesCollision(Frame.Character, ECollisionEnabled::NoCollision);
	}
	// Check for Bodyshot
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		GetWorld()->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
		// Bodyshot
		if (BlasterCharacter)
		{
			if (ShotgunResult.Bodyshots.Contains(BlasterCharacter))
			{
				ShotgunResult.Bodyshots[BlasterCharacter]++;
			}
			else
			{
				ShotgunResult.Bodyshots.Emplace(BlasterCharacter, 1);
			}
		}
	}

	for (auto& CurrentFrame : CurrentFrames)
	{
		MoveBoxes(CurrentFrame.Character, CurrentFrame);
		SetBoxesCollision(CurrentFrame.Character, ECollisionEnabled::NoCollision);
		SetCharacterMeshCollision(CurrentFrame.Character, ECollisionEnabled::QueryAndPhysics);
	}

	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPosition(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	for (auto& BoxComponent : HitCharacter->HitCollisionBoxes)
	{
		if (BoxComponent.Value != nullptr)
		{
			UBoxComponent* Box = BoxComponent.Value;

			FBoxInformation BoxInfo;
			BoxInfo.Location = Box->GetComponentLocation();
			BoxInfo.Rotation = Box->GetComponentRotation();
			BoxInfo.BoxExtent = Box->GetScaledBoxExtent();

			OutFramePackage.HitBoxInfo.Add(BoxComponent.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	for (auto& BoxComponent : HitCharacter->HitCollisionBoxes)
	{
		if (BoxComponent.Value != nullptr)
		{
			const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(BoxComponent.Key);
			if (BoxValue)
			{
				BoxComponent.Value->SetWorldLocation(BoxValue->Location);
				BoxComponent.Value->SetWorldRotation(BoxValue->Rotation);
				BoxComponent.Value->SetBoxExtent(BoxValue->BoxExtent);
			}
		}
	}
}

void ULagCompensationComponent::SetBoxesCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionType)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	for (auto& BoxComponent : HitCharacter->HitCollisionBoxes)
	{
		if (BoxComponent.Value != nullptr)
		{
			BoxComponent.Value->SetCollisionEnabled(CollisionType);

			if (CollisionType == ECollisionEnabled::QueryAndPhysics)
			{
				BoxComponent.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}
	}
}

void ULagCompensationComponent::SetHeadBoxesCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionType)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(CollisionType);
	if (CollisionType == ECollisionEnabled::QueryAndPhysics)
	{
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}
}

void ULagCompensationComponent::SetCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionType)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionType);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<ABlasterCharacter*> HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensationComp() == nullptr ||
		HitCharacter->GetLagCompensationComp()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensationComp()->FrameHistory.GetTail() == nullptr;

	if (bReturn)
	{
		return FFramePackage();
	}
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	// Frame History of the HitCharacter
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComp()->FrameHistory;
	if (History.GetTail()->GetValue().Time > HitTime)
	{
		// too far back - too laggy to do SSR
		return FFramePackage();
	}
	if (History.GetTail()->GetValue().Time == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (History.GetHead()->GetValue().Time <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Right = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Left = History.GetHead();
	while (Left->GetValue().Time > HitTime)
	{
		if (Left->GetNextNode() == nullptr)
		{
			break;
		}

		Left = Left->GetNextNode();
		if (Left->GetValue().Time > HitTime)
		{
			Right = Left;
		}
	}
	if (Left->GetValue().Time == HitTime)
	{
		FrameToCheck = Left->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		// Interpolate between Left and Right
		FrameToCheck = InterpBetweenFrames(Left->GetValue(), Right->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
	FServerSideRewindResult ConfirmedResult = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);
	if (Character && HitCharacter && DamageCauser && ConfirmedResult.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageCauser->GetDamage(),
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ServerProjectileScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult ConfirmedResult = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);
	if (Character && Character->GetEquippedWeapon() && HitCharacter && ConfirmedResult.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Character->GetEquippedWeapon()->GetDamage(),
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ServerShotgunScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult ConfirmedResult = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || HitCharacter->GetEquippedWeapon() == nullptr)
		{
			continue;
		}
		float TotalDamage = 0.f;
		if (ConfirmedResult.Bodyshots.Contains(HitCharacter))
		{
			TotalDamage += ConfirmedResult.Bodyshots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetDamage();
		}
		if (ConfirmedResult.Headshots.Contains(HitCharacter))
		{
			TotalDamage += ConfirmedResult.Headshots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetDamage();
		}

		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			HitCharacter->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SaveFramePackage();
}
