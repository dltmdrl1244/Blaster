#pragma once

UENUM(BlueprintType)
enum class ETeam : uint8
{
	ETeam_RedTeam		UMETA(DisplayName = "RedTeam"),
	ETeam_BlueTeam		UMETA(DisplayName = "BlueTeam"),
	ETeam_NoTeam		UMETA(DisplayName = "No Team"),

	ETeam_MAX			UMETA(DisplayName = "DefaultMAX"),
};