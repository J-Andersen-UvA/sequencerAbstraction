#include "SequencerAbstractionBPLibrary.h"

#include "Editor.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "LevelSequence.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "MovieSceneTimeUnit.h"
#include "MovieSceneTrack.h"
#include "Channels/MovieSceneChannel.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "ControlRig.h"
#include "ExtensionLibraries/MovieSceneSectionExtensions.h"
#include "MovieSceneScriptingChannel.h"
#include "MVVM/SectionModelStorageExtension.h"
#include "MVVM/Selection/Selection.h"
#include "MVVM/ViewModels/ChannelModel.h"
#include "MVVM/ViewModels/SectionModel.h"
#include "MVVM/ViewModels/SequencerEditorViewModel.h"
#include "Rigs/FKControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "Sequencer/MovieSceneControlRigParameterSection.h"
#include "Sequencer/MovieSceneControlRigParameterTrack.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/UnrealType.h"

TArray<UMovieSceneScriptingChannel*> USequencerAbstractionBPLibrary::GetAllChannelsFromRigBindingSequenceTrack(UMovieSceneTrack* Track)
{
    TArray<UMovieSceneScriptingChannel*> AllChannels;
    if (!Track)
    {
        return AllChannels;
    }

    const TArray<UMovieSceneSection*>& Sections = Track->GetAllSections();
    for (UMovieSceneSection* Section : Sections)
    {
        if (!Section)
        {
            continue;
        }

        AllChannels.Append(UMovieSceneSectionExtensions::GetAllChannels(Section));
    }

    return AllChannels;
}

static FString GetScriptingKeyValueString(UMovieSceneScriptingKey* Key)
{
    if (!Key)
    {
        return FString();
    }

    UFunction* GetValueFunction = Key->FindFunction(TEXT("GetValue"));
    if (!GetValueFunction)
    {
        return FString();
    }

    FProperty* ReturnProperty = nullptr;
    for (TFieldIterator<FProperty> It(GetValueFunction); It; ++It)
    {
        if (It->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            ReturnProperty = *It;
            break;
        }
    }

    if (!ReturnProperty)
    {
        return FString();
    }

    uint8* Params = static_cast<uint8*>(FMemory_Alloca(GetValueFunction->ParmsSize));
    FMemory::Memzero(Params, GetValueFunction->ParmsSize);

    for (TFieldIterator<FProperty> It(GetValueFunction); It; ++It)
    {
        It->InitializeValue_InContainer(Params);
    }

    Key->ProcessEvent(GetValueFunction, Params);

    FString ValueString;
    ReturnProperty->ExportTextItem_Direct(
        ValueString,
        ReturnProperty->ContainerPtrToValuePtr<void>(Params),
        nullptr,
        Key,
        PPF_None);

    for (TFieldIterator<FProperty> It(GetValueFunction); It; ++It)
    {
        It->DestroyValue_InContainer(Params);
    }

    return ValueString;
}

static bool DoesChannelMatchAnyRequestedName(
    const FName ChannelName,
    const TArray<FName>& Names,
    const bool bMatchContains,
    FName& OutRequestedName)
{
    OutRequestedName = NAME_None;

    if (Names.Num() == 0)
    {
        return true;
    }

    const FString ChannelString = ChannelName.ToString();
    for (const FName& Name : Names)
    {
        if (Name.IsNone())
        {
            continue;
        }

        const FString RequestedString = Name.ToString();
        const bool bMatches = bMatchContains
            ? ChannelString.Contains(RequestedString, ESearchCase::IgnoreCase)
            : ChannelString.Equals(RequestedString, ESearchCase::IgnoreCase);

        if (bMatches)
        {
            OutRequestedName = Name;
            return true;
        }
    }

    return false;
}

static TSharedPtr<ISequencer> GetOpenSequencerForSelection(ULevelSequence* Sequence)
{
#if WITH_EDITOR
    if (!Sequence || !GEditor)
    {
        return nullptr;
    }

    UAssetEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
    if (!EditorSubsystem)
    {
        return nullptr;
    }

    if (IAssetEditorInstance* Inst = EditorSubsystem->FindEditorForAsset(Sequence, false))
    {
        if (ILevelSequenceEditorToolkit* Toolkit = static_cast<ILevelSequenceEditorToolkit*>(Inst))
        {
            return Toolkit->GetSequencer();
        }
    }
#endif

    return nullptr;
}

static ULevelSequence* GetLevelSequenceFromTrack(UMovieSceneTrack* Track)
{
    if (!Track)
    {
        return nullptr;
    }

    if (UMovieScene* MovieScene = Track->GetTypedOuter<UMovieScene>())
    {
        return MovieScene->GetTypedOuter<ULevelSequence>();
    }

    return nullptr;
}

static int32 DeselectKeysByChannelProxy(
    const FSequencerChannelProxy& ChannelProxy,
    const TArray<int32>& Indices,
    TSharedPtr<ISequencer> Sequencer)
{
    using namespace UE::Sequencer;

    if (!Sequencer.IsValid())
    {
        return 0;
    }

    UMovieSceneSection* Section = ChannelProxy.Section;
    if (!Section)
    {
        return 0;
    }

    FSectionModelStorageExtension* SectionModelStorage =
        Sequencer->GetViewModel()->GetRootModel()->CastDynamic<FSectionModelStorageExtension>();
    if (!SectionModelStorage)
    {
        return 0;
    }

    TSharedPtr<FSectionModel> SectionHandle = SectionModelStorage->FindModelForSection(Section);
    if (!SectionHandle)
    {
        return 0;
    }

    TParentFirstChildIterator<FChannelGroupModel> KeyAreaNodes =
        SectionHandle->GetParentTrackModel().AsModel()->GetDescendantsOfType<FChannelGroupModel>();

    for (const TViewModelPtr<FChannelGroupModel>& KeyAreaNode : KeyAreaNodes)
    {
        if (KeyAreaNode->GetChannelName() != ChannelProxy.ChannelName)
        {
            continue;
        }

        TSharedPtr<FChannelModel> ChannelModel = KeyAreaNode->GetChannel(Section);
        if (!ChannelModel)
        {
            return 0;
        }

        FMovieSceneChannel* MovieSceneChannel = ChannelModel->GetChannel();
        if (!MovieSceneChannel)
        {
            return 0;
        }

        FKeySelection& KeySelection = Sequencer->GetViewModel()->GetSelection()->KeySelection;
        TUniqueFragmentSelectionSet<FKeyHandle, FChannelModel>& BaseKeySelection = KeySelection;
        int32 NumDeselected = 0;
        for (int32 Index : Indices)
        {
            if (Index >= 0 && Index < MovieSceneChannel->GetNumKeys())
            {
                const FKeyHandle KeyHandle = MovieSceneChannel->GetHandle(Index);
                BaseKeySelection.Deselect(KeyHandle);
                ++NumDeselected;
            }
        }
        return NumDeselected;
    }

    return 0;
}

static bool IsBoneChannelNameDelimiter(TCHAR Character)
{
    return Character == TEXT('.') ||
        Character == TEXT(':') ||
        Character == TEXT('/') ||
        Character == TEXT('\\') ||
        Character == TEXT(' ') ||
        Character == TEXT('(') ||
        Character == TEXT(')') ||
        Character == TEXT('[') ||
        Character == TEXT(']') ||
        Character == TEXT(',');
}

static bool TextContainsBoneToken(const FString& Text, const TSet<FString>& BoneNameSet)
{
    if (Text.IsEmpty() || BoneNameSet.Num() == 0)
    {
        return false;
    }

    const FString LowerText = Text.ToLower();
    if (BoneNameSet.Contains(LowerText))
    {
        return true;
    }

    int32 TokenStart = 0;
    for (int32 Index = 0; Index <= LowerText.Len(); ++Index)
    {
        const bool bEndOfText = Index == LowerText.Len();
        if (bEndOfText || IsBoneChannelNameDelimiter(LowerText[Index]))
        {
            if (Index > TokenStart)
            {
                const FString Token = LowerText.Mid(TokenStart, Index - TokenStart);
                if (BoneNameSet.Contains(Token))
                {
                    return true;
                }
            }

            TokenStart = Index + 1;
        }
    }

    return false;
}

static TSet<FName> BuildRequestedBoneNameSet(const TArray<FName>& BoneNames)
{
    TSet<FName> BoneNameSet;
    BoneNameSet.Reserve(BoneNames.Num());

    for (const FName& BoneName : BoneNames)
    {
        if (!BoneName.IsNone())
        {
            BoneNameSet.Add(BoneName);
        }
    }

    return BoneNameSet;
}

static bool IsAnimatableNonCurveControl(UControlRig* ControlRig, const FRigControlElement* ControlElement)
{
    if (!ControlRig || !ControlElement)
    {
        return false;
    }

    const FRigControlSettings& Settings = ControlElement->Settings;
    if (Settings.bIsCurve || Settings.bIsTransientControl || ControlRig->IsCurveControl(ControlElement))
    {
        return false;
    }

    return Settings.AnimationType == ERigControlAnimationType::AnimationControl;
}

static UControlRig* ResolveControlRigFromSectionOrTrack(
    UMovieSceneControlRigParameterSection* Section,
    UMovieSceneTrack* Track)
{
    if (Section)
    {
        if (UControlRig* ControlRig = Section->GetControlRig())
        {
            return ControlRig;
        }
    }

    if (const UMovieSceneControlRigParameterTrack* ControlRigTrack = Cast<UMovieSceneControlRigParameterTrack>(Track))
    {
        return ControlRigTrack->GetControlRig();
    }

    return nullptr;
}

static TSet<FName> BuildFkBoneControlNameSet(
    UControlRig* ControlRig,
    const TSet<FName>& RequestedBoneNames)
{
    TSet<FName> BoneControlNames;
    if (!ControlRig)
    {
        return BoneControlNames;
    }

    URigHierarchy* Hierarchy = ControlRig->GetHierarchy();
    if (!Hierarchy)
    {
        return BoneControlNames;
    }

    BoneControlNames.Reserve(Hierarchy->GetBonesFast().Num());

    if (UFKControlRig* FKControlRig = Cast<UFKControlRig>(ControlRig))
    {
        for (FRigBoneElement* BoneElement : Hierarchy->GetBones(true))
        {
            if (!BoneElement)
            {
                continue;
            }

            const FName BoneName = BoneElement->GetFName();
            const FName ControlName = UFKControlRig::GetControlName(BoneName, ERigElementType::Bone);
            if (ControlName.IsNone())
            {
                continue;
            }

            if (RequestedBoneNames.Num() > 0 &&
                !RequestedBoneNames.Contains(BoneName) &&
                !RequestedBoneNames.Contains(ControlName))
            {
                continue;
            }

            FRigControlElement* ControlElement = FKControlRig->FindControl(ControlName);
            if (IsAnimatableNonCurveControl(FKControlRig, ControlElement))
            {
                BoneControlNames.Add(ControlName);
            }
        }

        return BoneControlNames;
    }

    TSet<FName> HierarchyBoneNames;
    HierarchyBoneNames.Reserve(Hierarchy->GetBonesFast().Num());
    for (FRigBoneElement* BoneElement : Hierarchy->GetBones(true))
    {
        if (BoneElement)
        {
            HierarchyBoneNames.Add(BoneElement->GetFName());
        }
    }

    for (FRigControlElement* ControlElement : Hierarchy->GetControls(true))
    {
        if (!ControlElement || !IsAnimatableNonCurveControl(ControlRig, ControlElement))
        {
            continue;
        }

        const FName ControlName = ControlElement->GetFName();
        if (!HierarchyBoneNames.Contains(ControlName))
        {
            continue;
        }

        if (RequestedBoneNames.Num() > 0 && !RequestedBoneNames.Contains(ControlName))
        {
            continue;
        }

        BoneControlNames.Add(ControlName);
    }

    return BoneControlNames;
}

static bool DisplayFrameMatchesSelectionRange(
    int32 DisplayFrame,
    int32 StartDisplayFrame,
    int32 EndDisplayFrame,
    TOptional<int32> ExactDisplayFrame)
{
    if (ExactDisplayFrame.IsSet())
    {
        return DisplayFrame == ExactDisplayFrame.GetValue();
    }

    return DisplayFrame >= StartDisplayFrame && DisplayFrame <= EndDisplayFrame;
}

static int32 SelectBoneKeysInternal(
    UMovieSceneTrack* Track,
    const TArray<FName>& BoneNames,
    int32 StartDisplayFrame,
    int32 EndDisplayFrame,
    TOptional<int32> ExactDisplayFrame,
    bool bClearExistingSelection,
    bool bThrobSelection,
    FString& ErrorMessage)
{
#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return 0;
#else
    if (!Track)
    {
        ErrorMessage = TEXT("Track is null.");
        return 0;
    }

    const TSet<FName> RequestedBoneNames = BuildRequestedBoneNameSet(BoneNames);

    UMovieScene* MovieScene = Track->GetTypedOuter<UMovieScene>();
    if (!MovieScene)
    {
        ErrorMessage = TEXT("Could not resolve MovieScene from track.");
        return 0;
    }

    if (ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track))
    {
        if (USequencerAbstractionBPLibrary::GetCurrentOpenedLevelSequence() != Sequence)
        {
            ULevelSequenceEditorBlueprintLibrary::OpenLevelSequence(Sequence);
        }
    }

    if (bClearExistingSelection)
    {
        ULevelSequenceEditorBlueprintLibrary::EmptySelection();
    }

    const FFrameRate TickResolution = MovieScene->GetTickResolution();
    const FFrameRate DisplayRate = MovieScene->GetDisplayRate();

    int32 NumSelected = 0;
    for (UMovieSceneSection* Section : Track->GetAllSections())
    {
        if (!Section)
        {
            continue;
        }

        UMovieSceneControlRigParameterSection* ControlRigSection = Cast<UMovieSceneControlRigParameterSection>(Section);
        if (!ControlRigSection)
        {
            continue;
        }

        UControlRig* ControlRig = ResolveControlRigFromSectionOrTrack(ControlRigSection, Track);
        const TSet<FName> BoneControlNames = BuildFkBoneControlNameSet(ControlRig, RequestedBoneNames);
        if (BoneControlNames.Num() == 0)
        {
            continue;
        }

        const FMovieSceneChannelProxy& ChannelProxy = Section->GetChannelProxy();
        for (const FMovieSceneChannelEntry& Entry : ChannelProxy.GetAllEntries())
        {
            const TArrayView<FMovieSceneChannel* const> Channels = Entry.GetChannels();
            const TArrayView<const FMovieSceneChannelMetaData> MetaData = Entry.GetMetaData();
            const int32 NumChannels = FMath::Min(Channels.Num(), MetaData.Num());

            for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
            {
                FMovieSceneChannel* Channel = Channels[ChannelIndex];
                if (!Channel)
                {
                    continue;
                }

                const UE::MovieScene::FControlRigChannelMetaData ControlRigMetaData =
                    ControlRigSection->GetChannelMetaData(Channel);
                if (!ControlRigMetaData || !BoneControlNames.Contains(ControlRigMetaData.GetControlName()))
                {
                    continue;
                }

                TArray<FFrameNumber> KeyTimes;
                TArray<FKeyHandle> KeyHandles;
                Channel->GetKeys(TRange<FFrameNumber>::All(), &KeyTimes, &KeyHandles);

                if (KeyTimes.Num() == 0 || KeyTimes.Num() != KeyHandles.Num())
                {
                    continue;
                }

                const FFrameNumber KeyOffset = MetaData[ChannelIndex].GetOffsetTime(Section);
                TArray<int32> KeyIndicesToSelect;
                KeyIndicesToSelect.Reserve(KeyTimes.Num());

                for (int32 KeyIndex = 0; KeyIndex < KeyTimes.Num(); ++KeyIndex)
                {
                    const FFrameNumber OffsetKeyTime = KeyTimes[KeyIndex] + KeyOffset;
                    const FFrameTime DisplayTime = FFrameRate::TransformTime(
                        FFrameTime(OffsetKeyTime),
                        TickResolution,
                        DisplayRate);
                    const int32 DisplayFrame = DisplayTime.FrameNumber.Value;

                    if (!DisplayFrameMatchesSelectionRange(DisplayFrame, StartDisplayFrame, EndDisplayFrame, ExactDisplayFrame))
                    {
                        continue;
                    }

                    const int32 RawKeyIndex = Channel->GetIndex(KeyHandles[KeyIndex]);
                    if (RawKeyIndex != INDEX_NONE)
                    {
                        KeyIndicesToSelect.Add(RawKeyIndex);
                    }
                }

                if (KeyIndicesToSelect.Num() > 0)
                {
                    const FSequencerChannelProxy SequencerChannelProxy(MetaData[ChannelIndex].Name, Section);
                    ULevelSequenceEditorBlueprintLibrary::SelectKeys(SequencerChannelProxy, KeyIndicesToSelect);
                    NumSelected += KeyIndicesToSelect.Num();
                }
            }
        }
    }

    if (NumSelected > 0 && bThrobSelection)
    {
        if (ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track))
        {
            if (TSharedPtr<ISequencer> Sequencer = GetOpenSequencerForSelection(Sequence))
            {
                Sequencer->ThrobKeySelection();
            }
        }
    }

    return NumSelected;
#endif
}

TArray<FSequencerKeyInSelectionRangeInfo> USequencerAbstractionBPLibrary::GetKeysInSelectionRangeForNames(
    UMovieSceneTrack* Track,
    const TArray<FName>& Names,
    bool bMatchContains,
    FString& ErrorMessage)
{
    TArray<FSequencerKeyInSelectionRangeInfo> OutKeys;
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return OutKeys;
#else
    if (!Track)
    {
        ErrorMessage = TEXT("Track is null.");
        return OutKeys;
    }

    const int32 RawStartFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeStart();
    const int32 RawEndFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeEnd();
    const int32 StartFrame = FMath::Min(RawStartFrame, RawEndFrame);
    const int32 EndFrame = FMath::Max(RawStartFrame, RawEndFrame);

    const TArray<UMovieSceneScriptingChannel*> Channels = GetAllChannelsFromRigBindingSequenceTrack(Track);
    for (UMovieSceneScriptingChannel* Channel : Channels)
    {
        if (!Channel)
        {
            continue;
        }

        FName RequestedName = NAME_None;
        if (!DoesChannelMatchAnyRequestedName(Channel->ChannelName, Names, bMatchContains, RequestedName))
        {
            continue;
        }

        const TArray<UMovieSceneScriptingKey*> Keys = Channel->GetKeys();
        for (UMovieSceneScriptingKey* Key : Keys)
        {
            if (!Key)
            {
                continue;
            }

            const FFrameTime KeyTime = Key->GetTime(EMovieSceneTimeUnit::DisplayRate);
            const int32 DisplayFrame = KeyTime.FrameNumber.Value;
            if (DisplayFrame < StartFrame || DisplayFrame > EndFrame)
            {
                continue;
            }

            FSequencerKeyInSelectionRangeInfo Info;
            Info.Key = Key;
            Info.RequestedName = RequestedName;
            Info.ChannelName = Channel->ChannelName;
            Info.DisplayFrame = DisplayFrame;
            Info.SubFrame = KeyTime.GetSubFrame();
            Info.ValueString = GetScriptingKeyValueString(Key);
            OutKeys.Add(Info);
        }
    }

    if (OutKeys.Num() == 0)
    {
        ErrorMessage = FString::Printf(
            TEXT("No keys found between selection frames %d and %d for the requested names."),
            StartFrame,
            EndFrame);
    }

    return OutKeys;
#endif
}

int32 USequencerAbstractionBPLibrary::SelectKeysInSelectionRangeForNames(
    UMovieSceneTrack* Track,
    const TArray<FName>& Names,
    bool bMatchContains,
    bool bClearExistingSelection,
    bool bThrobSelection,
    FString& ErrorMessage)
{
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return 0;
#else
    if (!Track)
    {
        ErrorMessage = TEXT("Track is null.");
        return 0;
    }

    if (ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track))
    {
        if (GetCurrentOpenedLevelSequence() != Sequence)
        {
            ULevelSequenceEditorBlueprintLibrary::OpenLevelSequence(Sequence);
        }
    }

    const int32 RawStartFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeStart();
    const int32 RawEndFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeEnd();
    const int32 StartFrame = FMath::Min(RawStartFrame, RawEndFrame);
    const int32 EndFrame = FMath::Max(RawStartFrame, RawEndFrame);

    if (bClearExistingSelection)
    {
        ULevelSequenceEditorBlueprintLibrary::EmptySelection();
    }

    int32 NumSelected = 0;
    const TArray<UMovieSceneScriptingChannel*> Channels = GetAllChannelsFromRigBindingSequenceTrack(Track);
    for (UMovieSceneScriptingChannel* Channel : Channels)
    {
        if (!Channel)
        {
            continue;
        }

        FName RequestedName = NAME_None;
        if (!DoesChannelMatchAnyRequestedName(Channel->ChannelName, Names, bMatchContains, RequestedName))
        {
            continue;
        }

        UMovieSceneSection* Section = nullptr;
        TArray<int32> KeyIndicesToSelect;

        const TArray<UMovieSceneScriptingKey*> Keys = Channel->GetKeys();
        for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
        {
            UMovieSceneScriptingKey* Key = Keys[KeyIndex];
            if (!Key)
            {
                continue;
            }

            const FFrameTime KeyTime = Key->GetTime(EMovieSceneTimeUnit::DisplayRate);
            const int32 DisplayFrame = KeyTime.FrameNumber.Value;
            if (DisplayFrame < StartFrame || DisplayFrame > EndFrame)
            {
                continue;
            }

            if (!Section)
            {
                Section = Key->OwningSection.Get();
            }

            KeyIndicesToSelect.Add(KeyIndex);
        }

        if (Section && KeyIndicesToSelect.Num() > 0)
        {
            const FSequencerChannelProxy ChannelProxy(Channel->ChannelName, Section);
            ULevelSequenceEditorBlueprintLibrary::SelectKeys(ChannelProxy, KeyIndicesToSelect);
            NumSelected += KeyIndicesToSelect.Num();
        }
    }

    if (NumSelected == 0)
    {
        ErrorMessage = FString::Printf(
            TEXT("No keys selected between selection frames %d and %d for the requested names."),
            StartFrame,
            EndFrame);
        return 0;
    }

    if (bThrobSelection)
    {
        if (ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track))
        {
            if (TSharedPtr<ISequencer> Sequencer = GetOpenSequencerForSelection(Sequence))
            {
                Sequencer->ThrobKeySelection();
            }
        }
    }

    return NumSelected;
#endif
}

int32 USequencerAbstractionBPLibrary::SelectKeysAtFrameForNames(
    UMovieSceneTrack* Track,
    int32 DisplayFrame,
    const TArray<FName>& Names,
    bool bMatchContains,
    bool bClearExistingSelection,
    bool bThrobSelection,
    FString& ErrorMessage)
{
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return 0;
#else
    if (!Track)
    {
        ErrorMessage = TEXT("Track is null.");
        return 0;
    }

    if (ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track))
    {
        if (GetCurrentOpenedLevelSequence() != Sequence)
        {
            ULevelSequenceEditorBlueprintLibrary::OpenLevelSequence(Sequence);
        }
    }

    if (bClearExistingSelection)
    {
        ULevelSequenceEditorBlueprintLibrary::EmptySelection();
    }

    int32 NumSelected = 0;
    const TArray<UMovieSceneScriptingChannel*> Channels = GetAllChannelsFromRigBindingSequenceTrack(Track);
    for (UMovieSceneScriptingChannel* Channel : Channels)
    {
        if (!Channel)
        {
            continue;
        }

        FName RequestedName = NAME_None;
        if (!DoesChannelMatchAnyRequestedName(Channel->ChannelName, Names, bMatchContains, RequestedName))
        {
            continue;
        }

        UMovieSceneSection* Section = nullptr;
        TArray<int32> KeyIndicesToSelect;

        const TArray<UMovieSceneScriptingKey*> Keys = Channel->GetKeys();
        for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
        {
            UMovieSceneScriptingKey* Key = Keys[KeyIndex];
            if (!Key)
            {
                continue;
            }

            const FFrameTime KeyTime = Key->GetTime(EMovieSceneTimeUnit::DisplayRate);
            if (KeyTime.FrameNumber.Value != DisplayFrame)
            {
                continue;
            }

            if (!Section)
            {
                Section = Key->OwningSection.Get();
            }

            KeyIndicesToSelect.Add(KeyIndex);
        }

        if (Section && KeyIndicesToSelect.Num() > 0)
        {
            const FSequencerChannelProxy ChannelProxy(Channel->ChannelName, Section);
            ULevelSequenceEditorBlueprintLibrary::SelectKeys(ChannelProxy, KeyIndicesToSelect);
            NumSelected += KeyIndicesToSelect.Num();
        }
    }

    if (NumSelected == 0)
    {
        ErrorMessage = FString::Printf(
            TEXT("No keys selected at display frame %d for the requested names."),
            DisplayFrame);
        return 0;
    }

    if (bThrobSelection)
    {
        if (ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track))
        {
            if (TSharedPtr<ISequencer> Sequencer = GetOpenSequencerForSelection(Sequence))
            {
                Sequencer->ThrobKeySelection();
            }
        }
    }

    return NumSelected;
#endif
}

int32 USequencerAbstractionBPLibrary::SelectBoneKeysInSelectionRange(
    UMovieSceneTrack* Track,
    const TArray<FName>& BoneNames,
    bool bClearExistingSelection,
    bool bThrobSelection,
    FString& ErrorMessage)
{
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return 0;
#else
    const int32 RawStartFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeStart();
    const int32 RawEndFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeEnd();
    const int32 StartFrame = FMath::Min(RawStartFrame, RawEndFrame);
    const int32 EndFrame = FMath::Max(RawStartFrame, RawEndFrame);

    const int32 NumSelected = SelectBoneKeysInternal(
        Track,
        BoneNames,
        StartFrame,
        EndFrame,
        TOptional<int32>(),
        bClearExistingSelection,
        bThrobSelection,
        ErrorMessage);

    if (NumSelected == 0 && ErrorMessage.IsEmpty())
    {
        ErrorMessage = FString::Printf(
            TEXT("No bone keys selected between selection frames %d and %d."),
            StartFrame,
            EndFrame);
    }

    return NumSelected;
#endif
}

int32 USequencerAbstractionBPLibrary::SelectBoneKeysAtFrame(
    UMovieSceneTrack* Track,
    int32 DisplayFrame,
    const TArray<FName>& BoneNames,
    bool bClearExistingSelection,
    bool bThrobSelection,
    FString& ErrorMessage)
{
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return 0;
#else
    const int32 NumSelected = SelectBoneKeysInternal(
        Track,
        BoneNames,
        DisplayFrame,
        DisplayFrame,
        DisplayFrame,
        bClearExistingSelection,
        bThrobSelection,
        ErrorMessage);

    if (NumSelected == 0 && ErrorMessage.IsEmpty())
    {
        ErrorMessage = FString::Printf(
            TEXT("No bone keys selected at display frame %d."),
            DisplayFrame);
    }

    return NumSelected;
#endif
}

int32 USequencerAbstractionBPLibrary::RemoveKeysFromSelectionInSelectionRangeForNames(
    UMovieSceneTrack* Track,
    const TArray<FName>& Names,
    bool bMatchContains,
    FString& ErrorMessage)
{
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return 0;
#else
    if (!Track)
    {
        ErrorMessage = TEXT("Track is null.");
        return 0;
    }

    ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track);
    if (Sequence && GetCurrentOpenedLevelSequence() != Sequence)
    {
        ULevelSequenceEditorBlueprintLibrary::OpenLevelSequence(Sequence);
    }

    TSharedPtr<ISequencer> Sequencer = GetOpenSequencerForSelection(Sequence);
    if (!Sequencer.IsValid())
    {
        ErrorMessage = TEXT("Could not resolve open Sequencer for this track.");
        return 0;
    }

    const int32 RawStartFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeStart();
    const int32 RawEndFrame = ULevelSequenceEditorBlueprintLibrary::GetSelectionRangeEnd();
    const int32 StartFrame = FMath::Min(RawStartFrame, RawEndFrame);
    const int32 EndFrame = FMath::Max(RawStartFrame, RawEndFrame);

    int32 NumRemoved = 0;
    const TArray<UMovieSceneScriptingChannel*> Channels = GetAllChannelsFromRigBindingSequenceTrack(Track);
    for (UMovieSceneScriptingChannel* Channel : Channels)
    {
        if (!Channel)
        {
            continue;
        }

        FName RequestedName = NAME_None;
        if (!DoesChannelMatchAnyRequestedName(Channel->ChannelName, Names, bMatchContains, RequestedName))
        {
            continue;
        }

        UMovieSceneSection* Section = nullptr;
        TArray<int32> KeyIndicesToDeselect;

        const TArray<UMovieSceneScriptingKey*> Keys = Channel->GetKeys();
        for (int32 KeyIndex = 0; KeyIndex < Keys.Num(); ++KeyIndex)
        {
            UMovieSceneScriptingKey* Key = Keys[KeyIndex];
            if (!Key)
            {
                continue;
            }

            const FFrameTime KeyTime = Key->GetTime(EMovieSceneTimeUnit::DisplayRate);
            const int32 DisplayFrame = KeyTime.FrameNumber.Value;
            if (DisplayFrame < StartFrame || DisplayFrame > EndFrame)
            {
                continue;
            }

            if (!Section)
            {
                Section = Key->OwningSection.Get();
            }

            KeyIndicesToDeselect.Add(KeyIndex);
        }

        if (Section && KeyIndicesToDeselect.Num() > 0)
        {
            const FSequencerChannelProxy ChannelProxy(Channel->ChannelName, Section);
            NumRemoved += DeselectKeysByChannelProxy(ChannelProxy, KeyIndicesToDeselect, Sequencer);
        }
    }

    if (NumRemoved == 0)
    {
        ErrorMessage = FString::Printf(
            TEXT("No matching keys were removed from selection between selection frames %d and %d."),
            StartFrame,
            EndFrame);
    }

    return NumRemoved;
#endif
}

TArray<FName> USequencerAbstractionBPLibrary::GetSelectedNamesInTrack(
    UMovieSceneTrack* Track,
    const TArray<FName>& Names,
    bool bMatchContains,
    FString& ErrorMessage)
{
    TArray<FName> OutNames;
    ErrorMessage.Empty();

#if !WITH_EDITOR
    ErrorMessage = TEXT("Editor only.");
    return OutNames;
#else
    if (!Track)
    {
        ErrorMessage = TEXT("Track is null.");
        return OutNames;
    }

    ULevelSequence* Sequence = GetLevelSequenceFromTrack(Track);
    if (Sequence && GetCurrentOpenedLevelSequence() != Sequence)
    {
        ULevelSequenceEditorBlueprintLibrary::OpenLevelSequence(Sequence);
    }

    TSet<FName> UniqueNames;
    const TArray<FSequencerChannelProxy> ChannelsWithSelectedKeys =
        ULevelSequenceEditorBlueprintLibrary::GetChannelsWithSelectedKeys();

    for (const FSequencerChannelProxy& ChannelProxy : ChannelsWithSelectedKeys)
    {
        UMovieSceneSection* Section = ChannelProxy.Section;
        if (!Section)
        {
            continue;
        }

        if (Section->GetTypedOuter<UMovieSceneTrack>() != Track)
        {
            continue;
        }

        FName RequestedName = NAME_None;
        if (DoesChannelMatchAnyRequestedName(ChannelProxy.ChannelName, Names, bMatchContains, RequestedName))
        {
            UniqueNames.Add(Names.Num() > 0 ? RequestedName : ChannelProxy.ChannelName);
        }
    }

    OutNames.Reserve(UniqueNames.Num());
    for (const FName& Name : UniqueNames)
    {
        OutNames.Add(Name);
    }
    OutNames.Sort([](const FName& A, const FName& B)
    {
        return A.LexicalLess(B);
    });

    if (OutNames.Num() == 0)
    {
        ErrorMessage = TEXT("No selected keys found on this track for the requested names.");
    }

    return OutNames;
#endif
}

