#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "SequencerTypes.generated.h"

USTRUCT(BlueprintType)
struct FSequenceTrackInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(BlueprintReadOnly) FString TrackType;          // Class name (or a friendlier label if you map it)
    UPROPERTY(BlueprintReadOnly) FString ObjectBindingPath;  // e.g. "MyActor/MyComponent" if resolvable
    UPROPERTY(BlueprintReadOnly) FGuid BindingGuid;          // For object-bound tracks
    UPROPERTY(BlueprintReadOnly) FString TrackPath;          // Your own stable string key (see below)
    UPROPERTY(BlueprintReadOnly) bool bIsMasterTrack = false;
};

USTRUCT(BlueprintType)
struct FSequenceOpenResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FString Error;
};