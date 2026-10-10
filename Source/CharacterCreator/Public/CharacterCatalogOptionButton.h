#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOptionCatalog.h"
#include "CharacterCatalogOptionButton.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UCharacterCustomizationComponent;
class UCharacterOptionCatalog;

/**
 * A self-contained, C++-built clickable option tile. No UMG button Blueprint is required.
 * Used by UCharacterCatalogUIBuilder when populating the existing scroll boxes.
 */
UCLASS()
class CHARACTERCREATOR_API UCharacterCatalogOptionButton : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Called after CreateWidget; the widget remembers its own specific mesh option. */
    void Configure(const FCreatorMeshOption& InOption,
                   UCharacterOptionCatalog* InCatalog,
                   UCharacterCustomizationComponent* InTarget);

protected:
    virtual void NativeOnInitialized() override;

    UFUNCTION()
    void HandleClicked();

private:
    UPROPERTY(Transient)
    FCreatorMeshOption SelectedOption;

    UPROPERTY(Transient)
    TObjectPtr<UCharacterOptionCatalog> Catalog;

    UPROPERTY(Transient)
    TObjectPtr<UCharacterCustomizationComponent> Target;

    UPROPERTY(Transient)
    TObjectPtr<UButton> TileButton;

    UPROPERTY(Transient)
    TObjectPtr<UImage> PreviewImage;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> OptionLabel;
};
