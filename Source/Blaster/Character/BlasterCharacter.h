// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	/*
	* Play Montages
	*/
	void PlayFireMontage(bool bAiming);
	void PlayElimMontage();
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();

	virtual void OnRep_ReplicatedMovement() override;
	void Elim();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();

	bool IsLocallyReloading();
	bool bFinishSwapping = false;

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

protected:
	virtual void BeginPlay() override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void EquipButtonPressed(const FInputActionValue& Value);
	void CrouchButtonPressed(const FInputActionValue& Value);
	void AimButtonPressed(const FInputActionValue& Value);
	void AimButtonReleased(const FInputActionValue& Value);
	virtual void Jump() override;
	void FireButtonPressed(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	void ReloadButtonPressed(const FInputActionValue& Value);
	void ThrowGrenadeButtonPressed(const FInputActionValue& Value);

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	// Poll for any relevant classes and initialize HUD
	void PollInit();
	void RotateInPlace(float DeltaTime);

	// HitBox used for server-side rewind
	UPROPERTY(EditAnywhere)
	UBoxComponent* head;
	UPROPERTY(EditAnywhere)
	UBoxComponent* pelvis;
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_02;
	UPROPERTY(EditAnywhere)
	UBoxComponent* spine_03;
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* upperarm_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* lowerarm_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* hand_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* backpack;
	UPROPERTY(EditAnywhere)
	UBoxComponent* blanket;
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* thigh_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* calf_r;
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_l;
	UPROPERTY(EditAnywhere)
	UBoxComponent* foot_r;

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* EquipAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ThrowGrenadeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	// Blaster Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBuffComponent* BuffComp;

	UPROPERTY(VisibleAnywhere)
	class ULagCompensationComponent* LagCompenstationComp;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;
	void PlayHitReactMontage();

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed() const;

	/*
	Player Health
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	/*
	Player Shield
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimed = false;

	FTimerHandle ElimTimer;
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	/*
	* Dissolve Effect
	*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// Dynamic instnace that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;
	// Material instance set on the Blueprint, used with the dynamic material instnace
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	/*
	* Elim Bot
	*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;
	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	class ABlasterPlayerState* BlasterPlayerState;

	/*
	Grenade
	*/
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	// Default Weapon
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

	void DropOrDestroyWeapon(AWeapon* Weapon);

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool isEliminated() const { return bElimed; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombatComp() const { return CombatComp; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuffComp() const { return BuffComp; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensationComp() const { return LagCompenstationComp; }
};