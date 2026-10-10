#include "CharacterOptionCatalogBuilder.h"

#include "CharacterOptionCatalog.h"

#if WITH_EDITOR
#include "Engine/ObjectLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#endif

ACharacterOptionCatalogBuilder::ACharacterOptionCatalogBuilder()
{
    PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
    bIsEditorOnlyActor = true;
#endif
}

#if WITH_EDITOR
namespace
{
    /**
     * Explicitly discover every child directory on disk before loading Unreal assets.
     * This avoids relying on whether UObjectLibrary searches child paths itself.
     * Only /Game paths are supported; editor-only and no Build.cs changes needed.
     */
    static void CollectAssetsRecursively(const FString& RootPath, UClass* AssetClass,
                                         TArray<UObject*>& Out)
    {
        if (!RootPath.StartsWith(TEXT("/Game/")))
        {
            return;
        }

        const FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
        const FString RootRelative = RootPath.Mid(6); // Remove "/Game/".
        const FString DiskRoot = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(ContentDir, RootRelative));

        if (!IFileManager::Get().DirectoryExists(*DiskRoot))
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: Disk folder not found for %s (%s)"),
                *RootPath, *DiskRoot);
            return;
        }

        TArray<FString> AssetFiles;
        IFileManager::Get().FindFilesRecursive(AssetFiles, *DiskRoot, TEXT("*.uasset"), true, false);

        TSet<FString> PackageFolders;
        PackageFolders.Add(RootPath);
        for (const FString& AssetFile : AssetFiles)
        {
            FString RelativeFolder = FPaths::GetPath(AssetFile);
            if (!FPaths::MakePathRelativeTo(RelativeFolder, *ContentDir))
            {
                continue;
            }
            RelativeFolder.ReplaceInline(TEXT("\\"), TEXT("/"));
            if (RelativeFolder.StartsWith(TEXT("..")))
            {
                continue;
            }
            FString PackageFolder = TEXT("/Game/") + RelativeFolder;
            PackageFolder.RemoveFromEnd(TEXT("/"));
            PackageFolders.Add(PackageFolder);
        }

        TSet<FString> LoadedPaths;
        for (const FString& PackageFolder : PackageFolders)
        {
            UObjectLibrary* Library = UObjectLibrary::CreateLibrary(AssetClass, false, false);
            if (!IsValid(Library))
            {
                continue;
            }

            Library->AddToRoot();
            Library->LoadAssetsFromPath(PackageFolder);
            TArray<UObject*> FolderAssets;
            Library->GetObjects(FolderAssets);
            for (UObject* Asset : FolderAssets)
            {
                if (IsValid(Asset) && !LoadedPaths.Contains(Asset->GetPathName()))
                {
                    LoadedPaths.Add(Asset->GetPathName());
                    Out.Add(Asset);
                }
            }
            Library->RemoveFromRoot();
        }

        UE_LOG(LogTemp, Display,
            TEXT("CharacterCreator: Scanned %d nested asset folders under %s, found %d matching assets."),
            PackageFolders.Num(), *RootPath, Out.Num());
    }

    static bool ClassifyToken(const FString& Token, ECreatorBodyPart& Part)
    {
        if (Token.Equals(TEXT("Head"), ESearchCase::IgnoreCase))
        {
            Part = ECreatorBodyPart::Head;
            return true;
        }
        if (Token.Equals(TEXT("Torso"), ESearchCase::IgnoreCase) ||
            Token.Equals(TEXT("Body"), ESearchCase::IgnoreCase) ||
            Token.Equals(TEXT("Chest"), ESearchCase::IgnoreCase))
        {
            Part = ECreatorBodyPart::Body;
            return true;
        }
        if (Token.Equals(TEXT("Legs"), ESearchCase::IgnoreCase) ||
            Token.Equals(TEXT("Leg"), ESearchCase::IgnoreCase))
        {
            Part = ECreatorBodyPart::Legs;
            return true;
        }
        if (Token.Equals(TEXT("Feet"), ESearchCase::IgnoreCase) ||
            Token.Equals(TEXT("Foot"), ESearchCase::IgnoreCase) ||
            Token.Equals(TEXT("Boots"), ESearchCase::IgnoreCase))
        {
            Part = ECreatorBodyPart::Feet;
            return true;
        }
        return false;
    }

    /** Supports both Adventurer_Head and Head_01 within the Adventurer folder. */
    static bool GetPartFromAssetName(const FString& AssetName, const FString& ParentFolder,
                                     ECreatorBodyPart& OutPart, FString& OutLabel)
    {
        FString Normalized = AssetName;
        Normalized.RemoveFromStart(TEXT("SKM_"), ESearchCase::IgnoreCase);
        Normalized.RemoveFromStart(TEXT("SK_"), ESearchCase::IgnoreCase);
        Normalized.RemoveFromStart(TEXT("Skeletal_"), ESearchCase::IgnoreCase);
        Normalized.ReplaceInline(TEXT("-"), TEXT("_"));
        Normalized.ReplaceInline(TEXT(" "), TEXT("_"));

        TArray<FString> Tokens;
        Normalized.ParseIntoArray(Tokens, TEXT("_"), true);
        int32 CategoryIndex = INDEX_NONE;
        for (int32 i = 0; i < Tokens.Num(); ++i)
        {
            if (ClassifyToken(Tokens[i], OutPart))
            {
                CategoryIndex = i;
                break;
            }
        }
        if (CategoryIndex == INDEX_NONE)
        {
            return false; // Don't classify backpacks, skeletons, animation helpers, etc.
        }

        Tokens.RemoveAt(CategoryIndex);
        FString Label = FString::Join(Tokens, TEXT(" ")).TrimStartAndEnd();
        if (Label.IsEmpty() || Label.IsNumeric())
        {
            Label = ParentFolder.Replace(TEXT("_"), TEXT(" "));
            if (Label.IsEmpty())
            {
                Label = Normalized.Replace(TEXT("_"), TEXT(" "));
            }
        }
        OutLabel = Label;
        return true;
    }

    /** Names are kept meaningful, but spelling variants and prefixes are normalised. */
    static FString ThumbnailKey(FString Name)
    {
        Name.ReplaceInline(TEXT("-"), TEXT("_"));
        Name.ReplaceInline(TEXT(" "), TEXT("_"));
        Name.ToLowerInline();
        TArray<FString> Tokens;
        Name.ParseIntoArray(Tokens, TEXT("_"), true);
        TArray<FString> Clean;
        for (FString Token : Tokens)
        {
            if (Token == TEXT("sk") || Token == TEXT("skm") ||
                Token == TEXT("skeletal") || Token == TEXT("t") ||
                Token == TEXT("tex") || Token == TEXT("texture"))
            {
                continue;
            }
            if (Token == TEXT("adventurer")) Token = TEXT("adventure");
            if (Token == TEXT("body") || Token == TEXT("chest")) Token = TEXT("torso");
            if (Token == TEXT("leg")) Token = TEXT("legs");
            if (Token == TEXT("foot") || Token == TEXT("boots")) Token = TEXT("feet");
            Clean.Add(Token);
        }
        return FString::Join(Clean, TEXT(""));
    }

    static FString PartToken(ECreatorBodyPart Part)
    {
        switch (Part)
        {
            case ECreatorBodyPart::Head: return TEXT("Head");
            case ECreatorBodyPart::Body: return TEXT("Torso");
            case ECreatorBodyPart::Legs: return TEXT("Legs");
            case ECreatorBodyPart::Feet: return TEXT("Feet");
            default: return TEXT("");
        }
    }

    /** Prefer same outfit + body-part before the raw mesh name (which may just say 'Head'). */
    static UTexture2D* FindPartThumbnail(
        const USkeletalMesh* Mesh, ECreatorBodyPart Part,
        const TMap<FString, UTexture2D*>& Images)
    {
        if (!IsValid(Mesh)) return nullptr;

        const FString AssetPath = Mesh->GetPathName();
        const FString FolderPath = FPaths::GetPath(AssetPath);
        TArray<FString> Folders;
        FolderPath.ParseIntoArray(Folders, TEXT("/"), true);
        TArray<FString> Candidates;
        const FString Suffix = PartToken(Part);

        // A model may live in Adventurer/Meshes or Adventurer directly.
        for (int32 i = Folders.Num() - 1; i >= 0 && i >= Folders.Num() - 3; --i)
        {
            Candidates.AddUnique(ThumbnailKey(Folders[i] + TEXT("_") + Suffix));
        }
        Candidates.AddUnique(ThumbnailKey(Mesh->GetName()));

        for (const FString& Key : Candidates)
        {
            if (UTexture2D* const* Texture = Images.Find(Key)) return *Texture;
        }
        return nullptr; // Never silently use an unrelated outfit thumbnail.
    }

    static void ScanThumbnailGroup(
        const FString& Folder, ECreatorThumbnailGroup Group,
        TArray<FCreatorThumbnailEntry>& Library,
        TMap<FString, UTexture2D*>* OptionalCharacterMap)
    {
        if (!Folder.StartsWith(TEXT("/Game/")))
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: Skipping invalid thumbnail folder: %s"), *Folder);
            return;
        }

        TArray<UObject*> Assets;
        CollectAssetsRecursively(Folder, UTexture2D::StaticClass(), Assets);
        int32 Count = 0;
        for (UObject* Asset : Assets)
        {
            UTexture2D* Texture = Cast<UTexture2D>(Asset);
            if (!IsValid(Texture)) continue;

            FCreatorThumbnailEntry Entry;
            Entry.OptionId = FName(*Texture->GetPathName());
            Entry.Group = Group;
            Entry.DisplayName = FText::FromString(Texture->GetName().Replace(TEXT("_"), TEXT(" ")));
            Entry.Texture = Texture;
            Library.Add(MoveTemp(Entry));
            ++Count;

            if (OptionalCharacterMap)
            {
                const FString Key = ThumbnailKey(Texture->GetName());
                if (!OptionalCharacterMap->Contains(Key))
                {
                    OptionalCharacterMap->Add(Key, Texture);
                }
                else
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("CharacterCreator: Ambiguous thumbnail key '%s' in %s; using first match."),
                        *Key, *Folder);
                }
            }
        }
        UE_LOG(LogTemp, Display, TEXT("CharacterCreator: %s: found %d thumbnails."), *Folder, Count);
    }

}
#endif

void ACharacterOptionCatalogBuilder::GenerateOrRefreshCatalog()
{
#if WITH_EDITOR
    if (!IsValid(TargetCatalog))
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterCreator: Assign DA_CharacterOptions to Target Catalog first."));
        return;
    }
    if (!MeshFolder.StartsWith(TEXT("/Game/")))
    {
        UE_LOG(LogTemp, Error, TEXT("CharacterCreator: Mesh Folder must be a /Game/... path. Got: %s"), *MeshFolder);
        return;
    }

    TArray<UObject*> MeshAssets;
    CollectAssetsRecursively(MeshFolder, USkeletalMesh::StaticClass(), MeshAssets);

    // Scan all three existing Pictures folders independently, without moving assets.
    TMap<FString, UTexture2D*> CharacterThumbnails;
    TArray<FCreatorThumbnailEntry> ThumbnailLibrary;
    ScanThumbnailGroup(ThumbnailFolder, ECreatorThumbnailGroup::Character,
        ThumbnailLibrary, &CharacterThumbnails);
    ScanThumbnailGroup(ColourThumbnailFolder, ECreatorThumbnailGroup::Colour,
        ThumbnailLibrary, nullptr);
    ScanThumbnailGroup(SkinThumbnailFolder, ECreatorThumbnailGroup::Skin,
        ThumbnailLibrary, nullptr);

    ThumbnailLibrary.Sort([](const FCreatorThumbnailEntry& A, const FCreatorThumbnailEntry& B)
    {
        if (A.Group != B.Group) return static_cast<uint8>(A.Group) < static_cast<uint8>(B.Group);
        return A.DisplayName.ToString() < B.DisplayName.ToString();
    });

    TMap<FName, FCreatorMeshOption> OldOptions;
    if (bKeepExistingOverrides)
    {
        for (const FCreatorMeshOption& Old : TargetCatalog->Options)
        {
            OldOptions.Add(Old.OptionId, Old);
        }
    }

    TArray<FCreatorMeshOption> Found;
    TSet<FName> SeenIds;
    int32 SkippedUnrecognised = 0;

    for (UObject* Asset : MeshAssets)
    {
        USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset);
        if (!IsValid(Mesh))
        {
            continue;
        }

        ECreatorBodyPart Part = ECreatorBodyPart::Head;
        FString Label;
        const FString FolderName = FPaths::GetCleanFilename(FPaths::GetPath(Mesh->GetPathName()));
        if (!GetPartFromAssetName(Mesh->GetName(), FolderName, Part, Label))
        {
            ++SkippedUnrecognised;
            continue;
        }

        const FName Id(*Mesh->GetPathName());
        if (SeenIds.Contains(Id))
        {
            continue;
        }
        SeenIds.Add(Id);

        FCreatorMeshOption Option;
        Option.OptionId = Id;
        Option.DisplayName = FText::FromString(Label);
        Option.BodyPart = Part;
        Option.Mesh = Mesh;

        if (UTexture2D* Thumbnail = FindPartThumbnail(Mesh, Part, CharacterThumbnails))
        {
            Option.Thumbnail = Thumbnail;
        }

        if (const FCreatorMeshOption* Previous = OldOptions.Find(Id))
        {
            if (!Previous->DisplayName.IsEmpty())
            {
                Option.DisplayName = Previous->DisplayName;
            }
            if (!Previous->Thumbnail.IsNull())
            {
                Option.Thumbnail = Previous->Thumbnail;
            }
        }
        Found.Add(MoveTemp(Option));
    }

    // Don't wipe an existing catalog when the path is wrong or nothing matches.
    if (Found.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("CharacterCreator: No recognised body-part meshes under %s (%d assets checked, %d unrecognised). " )
            TEXT("Open an outfit folder and check mesh names contain Head, Body, Torso, Chest, Legs, Feet or Foot."),
            *MeshFolder, MeshAssets.Num(), SkippedUnrecognised);
        return;
    }

    Found.Sort([](const FCreatorMeshOption& A, const FCreatorMeshOption& B)
    {
        if (A.BodyPart != B.BodyPart)
        {
            return static_cast<uint8>(A.BodyPart) < static_cast<uint8>(B.BodyPart);
        }
        return A.DisplayName.ToString() < B.DisplayName.ToString();
    });

    // Preserve any human-edited thumbnail labels or overrides on refresh.
    if (bKeepExistingOverrides)
    {
        TMap<FName, FCreatorThumbnailEntry> PreviousEntries;
        for (const FCreatorThumbnailEntry& Entry : TargetCatalog->ThumbnailLibrary)
        {
            PreviousEntries.Add(Entry.OptionId, Entry);
        }
        for (FCreatorThumbnailEntry& Entry : ThumbnailLibrary)
        {
            if (const FCreatorThumbnailEntry* Previous = PreviousEntries.Find(Entry.OptionId))
            {
                if (!Previous->DisplayName.IsEmpty()) Entry.DisplayName = Previous->DisplayName;
                if (!Previous->Texture.IsNull()) Entry.Texture = Previous->Texture;
            }
        }
    }

    int32 Matched = 0;
    for (const FCreatorMeshOption& Option : Found)
    {
        if (!Option.Thumbnail.IsNull()) ++Matched;
    }

    TargetCatalog->Modify();
    TargetCatalog->Options = MoveTemp(Found);
    TargetCatalog->ThumbnailLibrary = MoveTemp(ThumbnailLibrary);
    TargetCatalog->MarkPackageDirty();

    UE_LOG(LogTemp, Display,
        TEXT("CharacterCreator: Generated %d mesh options, %d with preview thumbnails, %d image-library entries " )
        TEXT("(%d unrecognised skeletal meshes skipped) in %s. Save the catalog."),
        TargetCatalog->Options.Num(), Matched, TargetCatalog->ThumbnailLibrary.Num(),
        SkippedUnrecognised, *TargetCatalog->GetName());

#else
    UE_LOG(LogTemp, Warning, TEXT("CharacterCreator: Catalog generation is editor-only."));
#endif
}
