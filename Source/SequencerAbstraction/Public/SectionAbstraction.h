#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

USTRUCT(BlueprintType)
struct FSectionLabelEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SequenceAbstraction|SecctionAbstraction")
	TObjectPtr<UMovieSceneSection> Section = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SequenceAbstraction|SectionAbstraction")
	FString Label;
};

UCLASS()
class SEQUENCERABSTRACTION_API USectionAbstraction : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    	/** Pairs sections with corresponding labels into a list of structs */
	UFUNCTION(BlueprintCallable, Category = "SequenceAbstraction|SectionAbstraction")
	static FSectionLabelEntry CreateSectionLabelEntry(
		UMovieSceneSection* Section,
		const FString& Label
	);

	UFUNCTION(BlueprintCallable, Category = "SequenceAbstraction|SectionAbstraction")
	static void SetAnimationAsset(
		UMovieSceneSkeletalAnimationSection* Section,
		UAnimSequenceBase* Animation
	);

private:

};