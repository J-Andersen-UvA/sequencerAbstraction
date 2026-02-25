#include "SequencerAbstractionBPLibrary.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "ISequencer.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "LevelSequence.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "Logging/LogMacros.h"

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"
#include "Tracks/MovieSceneSkeletalAnimationTrack.h"
#include "Sections/MovieSceneSkeletalAnimationSection.h"

#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"

static FString MakeMasterTrackKey(UMovieSceneTrack* Track, int32 Index)
{
    return FString::Printf(TEXT("MASTER::%s::%d"), *Track->GetClass()->GetName(), Index);
}

static FString MakeBindingTrackKey(const FGuid& Guid, UMovieSceneTrack* Track, int32 Index)
{
    return FString::Printf(TEXT("BIND::%s::%s::%d"), *Guid.ToString(), *Track->GetClass()->GetName(), Index);
}

ULevelSequence* USequencerAbstractionBPLibrary::CreateLevelSequenceAsset(const FString& PackagePath, const FString& AssetName, FSequenceOpenResult& Result)
{
    Result = {};
    if (PackagePath.IsEmpty() || AssetName.IsEmpty())
    {
        Result.Error = TEXT("PackagePath or AssetName is empty.");
        return nullptr;
    }

    FAssetToolsModule& AssetToolsModule =
        FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    UObject* NewAsset = AssetTools.CreateAsset(
        AssetName,
        PackagePath,
        ULevelSequence::StaticClass(),
        nullptr
    );

    ULevelSequence* Seq = Cast<ULevelSequence>(NewAsset);
    if (!Seq)
    {
        Result.Error = TEXT("Failed to create LevelSequence asset.");
        return nullptr;
    }

    Result.bSuccess = true;
    return Seq;
}

bool USequencerAbstractionBPLibrary::SaveAsset(UObject* Asset)
{
    if (!Asset) return false;
    return UEditorAssetLibrary::SaveLoadedAsset(Asset, false);
}

ULevelSequence* USequencerAbstractionBPLibrary::LoadLevelSequenceAsset(const FString& AssetPath)
{
    return Cast<ULevelSequence>(UEditorAssetLibrary::LoadAsset(AssetPath));
}

ULevelSequence* USequencerAbstractionBPLibrary::GetCurrentOpenedLevelSequence()
{
    return ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence();
}

ULevelSequence* USequencerAbstractionBPLibrary::CreateEmptyLevelSequenceAsset(
    const FString& PackagePath,
    const FString& AssetName,
    FSequenceOpenResult& Result)
{
    Result = {};

    if (PackagePath.IsEmpty() || AssetName.IsEmpty())
    {
        Result.Error = TEXT("PackagePath or AssetName is empty.");
        return nullptr;
    }

    FAssetToolsModule& AssetToolsModule =
        FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

    IAssetTools& AssetTools = AssetToolsModule.Get();

    UObject* NewAsset = AssetTools.CreateAsset(
        AssetName,
        PackagePath,
        ULevelSequence::StaticClass(),
        nullptr
    );

    ULevelSequence* Seq = Cast<ULevelSequence>(NewAsset);
    if (!Seq)
    {
        Result.Error = TEXT("Failed to create LevelSequence asset.");
        return nullptr;
    }

    Result.bSuccess = true;
    return Seq;
}

bool USequencerAbstractionBPLibrary::OpenLevelSequenceInSequencer(ULevelSequence* Sequence)
{
    if (!Sequence)
    {
        return false;
    }

    return ULevelSequenceEditorBlueprintLibrary::OpenLevelSequence(Sequence);
}

TArray<FSequenceTrackInfo> USequencerAbstractionBPLibrary::GetAllTracksInCurrentSequence()
{
    TArray<FSequenceTrackInfo> Out;

    ULevelSequence* Seq = GetCurrentOpenedLevelSequence();
    if (!Seq)
    {
        return Out;
    }

    const UMovieScene* MovieScene = Seq->GetMovieScene();
    if (!MovieScene)
    {
        return Out;
    }

    // Master / Root Tracks
    const TArray<UMovieSceneTrack*>& MasterTracks = MovieScene->GetTracks();

    for (int32 i = 0; i < MasterTracks.Num(); ++i)
    {
        UMovieSceneTrack* Track = MasterTracks[i];
        if (!Track)
        {
            continue;
        }

        FSequenceTrackInfo Info;
        Info.bIsMasterTrack = true;
        Info.DisplayName = Track->GetDisplayName().ToString();
        Info.TrackType = Track->GetClass()->GetName();
        Info.BindingGuid = FGuid();
        Info.ObjectBindingPath = TEXT("");
        Info.TrackPath = FString::Printf(
            TEXT("MASTER::%s::%d"),
            *Info.TrackType,
            i
        );

        Out.Add(Info);
    }

    // Object Bindings + Tracks
    const TArray<FMovieSceneBinding>& Bindings = MovieScene->GetBindings();

    for (const FMovieSceneBinding& Binding : Bindings)
    {
        const FGuid Guid = Binding.GetObjectGuid();
        const TArray<UMovieSceneTrack*>& Tracks = Binding.GetTracks();

        for (int32 i = 0; i < Tracks.Num(); ++i)
        {
            UMovieSceneTrack* Track = Tracks[i];
            if (!Track)
            {
                continue;
            }

            FSequenceTrackInfo Info;
            Info.bIsMasterTrack = false;
            Info.DisplayName = Track->GetDisplayName().ToString();
            Info.TrackType = Track->GetClass()->GetName();
            Info.BindingGuid = Guid;
            Info.ObjectBindingPath = TEXT(""); // Resolve later if needed
            Info.TrackPath = FString::Printf(
                TEXT("BIND::%s::%s::%d"),
                *Guid.ToString(),
                *Info.TrackType,
                i
            );

            Out.Add(Info);
        }
    }

    return Out;
}

USkeletalMesh* USequencerAbstractionBPLibrary::LoadSkeletalMesh(const FString& AssetPath)
{
    return Cast<USkeletalMesh>(UEditorAssetLibrary::LoadAsset(AssetPath));
}

UAnimSequence* USequencerAbstractionBPLibrary::LoadAnimSequence(const FString& AssetPath)
{
    return Cast<UAnimSequence>(UEditorAssetLibrary::LoadAsset(AssetPath));
}

bool USequencerAbstractionBPLibrary::SetSequenceFrameRateFromAnimation(ULevelSequence* Sequence, UAnimSequence* Animation, bool bAlsoSetPlaybackRangeToAnim, FSequenceOpenResult& Result)
{
    Result = {};
    if (!Sequence || !Animation)
    {
        Result.Error = TEXT("Sequence or Animation is null.");
        return false;
    }

    UMovieScene* MovieScene = Sequence->GetMovieScene();
    if (!MovieScene)
    {
        Result.Error = TEXT("Sequence has no MovieScene.");
        return false;
    }

    // Anim sequences typically have a sampling frame rate; if not, you can derive from length / num frames.
    const FFrameRate AnimRate = Animation->GetSamplingFrameRate();
    if (AnimRate.Numerator <= 0 || AnimRate.Denominator <= 0)
    {
        Result.Error = TEXT("Animation sampling frame rate is invalid.");
        return false;
    }

    MovieScene->SetDisplayRate(AnimRate);

    if (bAlsoSetPlaybackRangeToAnim)
    {
        const double Seconds = Animation->GetPlayLength();
        const FFrameRate DisplayRate = MovieScene->GetDisplayRate();

        const int32 StartFrame = 0;
        const int32 DurationFrames = FMath::Max(1, (int32)FMath::RoundToInt(Seconds * DisplayRate.AsDecimal()));
        MovieScene->SetPlaybackRange(StartFrame, DurationFrames);
    }

    Result.bSuccess = true;
    return true;
}

static AActor* FindActorByLabelOrName(UWorld* World, const FName LabelOrName)
{
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* A = *It;
        if (!A) continue;

#if WITH_EDITOR
        if (A->GetActorLabel() == LabelOrName.ToString())
            return A;
#endif
        if (A->GetFName() == LabelOrName)
            return A;
    }
    return nullptr;
}

bool USequencerAbstractionBPLibrary::AddAnimationToSequenceOnSkeletalMeshComponent(
    ULevelSequence* Sequence,
    UObject* WorldContextObject,
    USkeletalMesh* SkeletalMesh,
    UAnimSequence* Animation,
    const FName ActorLabelOrName,
    FSequenceOpenResult& Result
){
    Result = {};
    if (!Sequence || !WorldContextObject || !SkeletalMesh || !Animation)
    {
        Result.Error = TEXT("One or more inputs are null.");
        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        Result.Error = TEXT("No world found from context.");
        return false;
    }

    AActor* TargetActor = FindActorByLabelOrName(World, ActorLabelOrName);
    if (!TargetActor)
    {
        Result.Error = TEXT("Target actor not found.");
        return false;
    }

    USkeletalMeshComponent* SkelComp = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
    if (!SkelComp)
    {
        Result.Error = TEXT("Target actor has no SkeletalMeshComponent.");
        return false;
    }

    SkelComp->SetSkeletalMesh(SkeletalMesh);

    UMovieScene* MovieScene = Sequence->GetMovieScene();
    if (!MovieScene)
    {
        Result.Error = TEXT("Sequence has no MovieScene.");
        return false;
    }

    // Possess the actor (basic possessable binding)
    const FGuid BindingGuid = MovieScene->AddPossessable(TargetActor->GetActorLabel(), TargetActor->GetClass());
    Sequence->BindPossessableObject(BindingGuid, *TargetActor, World);

    // Add skeletal animation track under that binding
    UMovieSceneSkeletalAnimationTrack* AnimTrack =
        MovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(BindingGuid);

    if (!AnimTrack)
    {
        Result.Error = TEXT("Failed to create skeletal animation track.");
        return false;
    }

    UMovieSceneSection* NewSection = AnimTrack->CreateNewSection();
    AnimTrack->AddSection(*NewSection);

    UMovieSceneSkeletalAnimationSection* AnimSection = Cast<UMovieSceneSkeletalAnimationSection>(NewSection);
    if (!AnimSection)
    {
        Result.Error = TEXT("Failed to cast animation section.");
        return false;
    }

    // Place at time 0, full length
    AnimSection->Params.Animation = Animation;

    const FFrameRate Rate = MovieScene->GetDisplayRate();
    const double Seconds = Animation->GetPlayLength();
    const int32 EndFrame = FMath::Max(1, (int32)FMath::RoundToInt(Seconds * Rate.AsDecimal()));

    AnimSection->SetRange(TRange<FFrameNumber>(FFrameNumber(0), FFrameNumber(EndFrame)));

    Result.bSuccess = true;
    return true;
}