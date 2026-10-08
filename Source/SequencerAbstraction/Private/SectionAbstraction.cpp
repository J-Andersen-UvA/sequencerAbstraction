#include "SequencerAbstractionBPLibrary.h"

#include "LevelSequence.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "MovieScene.h"
#include "LevelSequenceEditorBlueprintLibrary.h"

#include "Sections/MovieSceneSkeletalAnimationSection.h"


void USectionAbstraction::MatchSectionByBone(UMovieSceneSkeletalAnimationSection* CurrentSection, USkeletalMeshComponent* SkelMeshComp, FFrameTime CurrentFrame, FFrameRate FrameRate,
    FName BoneName)
{   
   if (!CurrentSection || !SkelMeshComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SectionAbstraction] Invalid CurrentSection or SkelMeshComp passed to MatchSectionByBone."));
        return;
    }

    CurrentSection->Modify();

    // 1. Assign target bone
    CurrentSection->MatchedBoneName = BoneName;

    // 2. Translation & Z matching
    CurrentSection->bMatchTranslation = true;
    CurrentSection->bMatchIncludeZHeight = true;

    // 3. Rotation matching (Yaw, Pitch, and Roll)
    CurrentSection->bMatchRotationYaw = true;
    CurrentSection->bMatchRotationPitch = true;
    CurrentSection->bMatchRotationRoll = true;

    // 4. Ensure CurrentFrame and FrameRate are in TickResolution
    UMovieScene* MovieScene = CurrentSection->GetTypedOuter<UMovieScene>();
    if (MovieScene)
    {
        const FFrameRate TickResolution = MovieScene->GetTickResolution();
        if (FrameRate != TickResolution)
        {
            CurrentFrame = FFrameRate::TransformTime(CurrentFrame, FrameRate, TickResolution);
            FrameRate = TickResolution;
        }
    }

    // 5. Calculate & apply the full delta transform
    CurrentSection->MatchSectionByBoneTransform(SkelMeshComp, CurrentFrame, FrameRate, BoneName);

    // 6. Invalidate root motion cache
    if (CurrentSection->GetRootMotionParams())
    {
        CurrentSection->GetRootMotionParams()->bRootMotionsDirty = true;
    }

    CurrentSection->MarkAsChanged();
}


void USectionAbstraction::SplitAnimationSection(
    UMovieSceneSection* Section,
    int32 SplitFrame,
    FFrameRate FrameRate,
    UMovieSceneSkeletalAnimationSection*& OutRightSection,
    UMovieSceneSkeletalAnimationSection*& OutLeftSection,
    bool bDeleteKeys)
{   
    OutLeftSection = nullptr;
    OutRightSection = nullptr;

    if (!Section)
    {
        return;
    }

    UMovieScene* MovieScene = Section->GetTypedOuter<UMovieScene>();
    if (!MovieScene)
    {
        return;
    }

    FFrameRate TickResolution = MovieScene->GetTickResolution();

    // Convert display frame (e.g., Frame 100 @ 30fps) to internal TickResolution (24000fps)
    FFrameTime TickTime = FFrameRate::TransformTime(
        FFrameTime(FFrameNumber(SplitFrame)),
        FrameRate,          // Source Display Rate (e.g., 30 fps)
        TickResolution      // Target Tick Resolution (e.g., 24000 fps)
    );

    FQualifiedFrameTime SplitTime(TickTime, TickResolution);

    // SplitSection modifies 'Section' in-place (Left) and returns the new section (Right)
    UMovieSceneSkeletalAnimationSection* RightSection = Cast<UMovieSceneSkeletalAnimationSection>(Section->SplitSection(SplitTime, bDeleteKeys));
    UMovieSceneSkeletalAnimationSection* LeftSection = Cast<UMovieSceneSkeletalAnimationSection>(Section);

    if (RightSection)
    {
        if (LeftSection)
        {
            LeftSection->Modify();
        }
        RightSection->Modify();

        if (UMovieSceneTrack* Track = Section->GetTypedOuter<UMovieSceneTrack>())
        {
            Track->Modify();
			Track->MarkAsChanged();
			Track->UpdateEasing();
        }

        if (GIsEditor && !IsRunningCommandlet())
        {
            ULevelSequenceEditorBlueprintLibrary::RefreshCurrentLevelSequence();
        }
    }

    OutLeftSection = LeftSection;
    OutRightSection = RightSection;

}

void USectionAbstraction::SetAnimationAsset(
	UMovieSceneSkeletalAnimationSection* Section,
	UAnimSequenceBase* Animation)
{
	if (!Section || !Animation)
	{
		return;
	}

	Section->Params.Animation = Animation;
	Section->Modify(true);

	return;
}

FSectionLabelEntry USectionAbstraction::CreateSectionLabelEntry(
	UMovieSceneSection* Section,
	const FString& Label,
    int32 GlossIndex)
{
	FSectionLabelEntry Entry;

	Entry.Section = Section;
	Entry.Label = Label;
    Entry.GlossIndex = GlossIndex;

	return Entry;
}

bool USectionAbstraction::RemoveAnimationSection(
    ULevelSequence* Sequence,
    UMovieSceneSkeletalAnimationSection* Section,
    FSequenceOpenResult& Result)
{
    Result = {};

    if (!Sequence || !Section)
    {
        Result.Error = TEXT("Sequence or Section is null.");
        return false;
    }

    UMovieScene* MovieScene = Sequence->GetMovieScene();
    if (!MovieScene)
    {
        Result.Error = TEXT("Sequence has no MovieScene.");
        return false;
    }

    UMovieSceneSkeletalAnimationTrack* Track =
        Cast<UMovieSceneSkeletalAnimationTrack>(Section->GetTypedOuter<UMovieSceneTrack>());

    if (!Track)
    {
        Result.Error = TEXT("Could not resolve owning track for section.");
        return false;
    }

    MovieScene->Modify();
    Track->Modify();
    Section->Modify();

    const int32 Before = Track->GetAllSections().Num();
    Track->RemoveSection(*Section);
    const int32 After = Track->GetAllSections().Num();
    const bool bRemoved = After < Before;
    if (!bRemoved)
    {
        Result.Error = TEXT("Track->RemoveSection failed (section not found).");
        return false;
    }

    Track->MarkAsChanged();
    Sequence->MarkPackageDirty();
    Result.bSuccess = true;
    return true;
}