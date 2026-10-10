#include "CharacterCatalogOptionButton.h"

#include "Blueprint/WidgetTree.h"
#include "CharacterCustomizationComponent.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"

void UCharacterCatalogOptionButton::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // This is a native-only UUserWidget: create the small, reusable tile in C++.
    // It does not depend on any names in the old Button Widget Blueprint.
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    TileButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SelectOption"));
    WidgetTree->RootWidget = TileButton;

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("TileContents"));
    TileButton->AddChild(Column);

    USizeBox* PreviewArea = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("PreviewArea"));
    PreviewArea->SetWidthOverride(94.0f);
    PreviewArea->SetHeightOverride(76.0f);
    Column->AddChildToVerticalBox(PreviewArea);

    PreviewImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("OptionThumbnail"));
    PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
    PreviewArea->AddChild(PreviewImage);

    OptionLabel = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("OptionName"));
    OptionLabel->SetText(FText::FromString(TEXT("Option")));
    Column->AddChildToVerticalBox(OptionLabel);

    TileButton->OnClicked.AddUniqueDynamic(this, &UCharacterCatalogOptionButton::HandleClicked);
}

void UCharacterCatalogOptionButton::Configure(
    const FCreatorMeshOption& InOption,
    UCharacterOptionCatalog* InCatalog,
    UCharacterCustomizationComponent* InTarget)
{
    SelectedOption = InOption;
    Catalog = InCatalog;
    Target = InTarget;

    const FText Label = InOption.DisplayName.IsEmpty()
        ? FText::FromName(InOption.OptionId)
        : InOption.DisplayName;

    if (OptionLabel)
    {
        OptionLabel->SetText(Label);
    }
    if (TileButton)
    {
        TileButton->SetToolTipText(Label);
    }
    if (PreviewImage)
    {
        UTexture2D* Preview = InOption.Thumbnail.LoadSynchronous();
        if (IsValid(Preview))
        {
            PreviewImage->SetBrushFromTexture(Preview, true);
            PreviewImage->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            // Still show the named button even if one thumbnail is missing.
            PreviewImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UCharacterCatalogOptionButton::HandleClicked()
{
    if (!IsValid(Catalog) || !IsValid(Target))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("CharacterCreator Phase3: Button clicked but catalogue or target character is unavailable."));
        return;
    }

    if (!Catalog->ApplyOption(Target, SelectedOption))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterCreator Phase3: Failed to apply %s."),
            *SelectedOption.OptionId.ToString());
    }
}
