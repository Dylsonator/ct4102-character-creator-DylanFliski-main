#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharacterOptionCatalogBuilder.generated.h"

class UCharacterOptionCatalog;

/**
 * Editor-only workflow helper, placed in a level temporarily.
 * Select the actor and press Generate / Refresh Catalog in the Details panel.
 * No asset scanning occurs at runtime or in a packaged game.
 */
UCLASS(NotBlueprintable)
class CHARACTERCREATOR_API ACharacterOptionCatalogBuilder : public AActor
{
    GENERATED_BODY()

public:
    ACharacterOptionCatalogBuilder();

    /** Create a Data Asset (CharacterOptionCatalog) and assign it here. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Catalog")
    TObjectPtr<UCharacterOptionCatalog> TargetCatalog = nullptr;

    /** Unreal content path, such as /Game/Separate_Skeletal_Meshes_and_Anim. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Scan")
    FString MeshFolder = TEXT("/Game/Separate_Skeletal_Meshes_and_Animations/Separate_Skeletal_Meshes_and_Animations");

    /** Optional thumbnail textures (for example /Game/Pictures/Characters). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Scan")
    FString ThumbnailFolder = TEXT("/Game/Pictures/Characters");

    /** Existing colour swatch thumbnails; stored for the Phase 3 picker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Scan")
    FString ColourThumbnailFolder = TEXT("/Game/Pictures/Colours");

    /** Existing skin preview thumbnails; stored for the Phase 3 picker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Scan")
    FString SkinThumbnailFolder = TEXT("/Game/Pictures/Skins");

    /** Preserve previously edited names and selected thumbnails for matching mesh paths. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Creator|Scan")
    bool bKeepExistingOverrides = true;

    /** Safe on repeated runs: regenerates rather than appending duplicate options. */
    UFUNCTION(CallInEditor, Category = "Character Creator|Actions", meta = (DisplayName = "Generate / Refresh Catalog"))
    void GenerateOrRefreshCatalog();
};
