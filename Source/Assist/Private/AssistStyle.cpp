#include "AssistStyle.h"
#include "Assist.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FAssistStyle::StyleInstance = nullptr;

void FAssistStyle::Initialize() {
    if (!StyleInstance.IsValid()) {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FAssistStyle::Shutdown() {
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FAssistStyle::GetStyleSetName() {
    static FName StyleSetName(TEXT("AssistStyle"));
    return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef<FSlateStyleSet> FAssistStyle::Create() {
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("AssistStyle"));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("Assist")->GetBaseDir() / TEXT("Resources"));

    Style->Set("Assist.ReloadProject", new IMAGE_BRUSH_SVG(TEXT("ReloadProjectButtonIcon"), Icon20x20));
    Style->Set("Assist.SetHorizontalLayout", new IMAGE_BRUSH(TEXT("SetHorizontalLayout"), Icon20x20));
    Style->Set("Assist.SetVerticalLayout", new IMAGE_BRUSH(TEXT("SetVerticalLayout"), Icon20x20));
    Style->Set("Assist.SetLanguageToEn", new IMAGE_BRUSH(TEXT("SetLanguageButtonUsa"), Icon20x20));
    Style->Set("Assist.SetLanguageToZhHans", new IMAGE_BRUSH(TEXT("SetLanguageButtonChina"), Icon20x20));

    return Style;
}

void FAssistStyle::ReloadTextures() {
    if (FSlateApplication::IsInitialized()) {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

const ISlateStyle& FAssistStyle::Get() {
    return *StyleInstance;
}
