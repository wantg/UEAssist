#include "Assist.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "FileHelpers.h"
#include "UnrealEdGlobals.h"
#include "HAL/PlatformApplicationMisc.h"
#include "SAssetView.h"
#include "Settings/ContentBrowserSettings.h"
#include "AssistStyle.h"

#define LOCTEXT_NAMESPACE "FAssistModule"

void FAssistModule::StartupModule() {
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

    init();
    FAssistStyle::Initialize();
    FAssistStyle::ReloadTextures();

    // https://minifloppy.it/posts/2024/adding-custom-buttons-unreal-editor-toolbars-menus
    // Register a function to be called when menu system is initialized
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssistModule::RegisterMenus));
}

void FAssistModule::ShutdownModule() {
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.

    // Unregister the startup function
    UToolMenus::UnRegisterStartupCallback(this);

    // Unregister all our menu extensions
    UToolMenus::UnregisterOwner(this);

    FAssistStyle::Shutdown();
}

void FAssistModule::init() {
    if (TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin("Assist")) {
        const FString& ResourceFilePath = ThisPlugin->GetBaseDir() / TEXT("Resources/config.json");
        FString FileContentString;
        FFileHelper::LoadFileToString(FileContentString, *ResourceFilePath);
        if (!FJsonObjectConverter::JsonObjectStringToUStruct(FileContentString, &AssistConfig)) {
            UE_LOG(LogJson, Error, TEXT("JsonObjectStringToUStruct failed (%s)"), *ResourceFilePath);
        }
    }
}

void FAssistModule::RegisterMenus() {
    UToolMenus* ToolMenus = UToolMenus::Get();
    for (auto& MenuModule : AssistConfig.SupportedEditors) {
        FString MainMenuName       = MenuModule + ".MainMenu";
        FString AssistMenuName     = "AssistMenu";
        FString AssistMenuFullName = MainMenuName + "." + AssistMenuName;
        UToolMenu* AssistMenu      = ToolMenus->RegisterMenu(FName(AssistMenuFullName));

        // AddSubMenu
        TArray<FString> SeparatedStrings;
        MenuModule.ParseIntoArray(SeparatedStrings, TEXT("."), false);
        FString AssistMenuLabel = SeparatedStrings[SeparatedStrings.Num() - 1];

        UToolMenu* MenuBar = ToolMenus->ExtendMenu(FName(MainMenuName));
        MenuBar->AddSubMenu(
            "",                         // InOwner
            "",                         // SectionName
            FName(AssistMenuName),      // InName
            LOCTEXT("Label", "Assist"), // InLabel
            LOCTEXT("ToolTip", "Some Useful tools"));

        // Section Project
        FToolMenuSection& SectionProject = AssistMenu->AddSection("Project", LOCTEXT("Label", "Project"));
        SectionProject.AddEntry(FToolMenuEntry::InitMenuEntry(
            "ReloadProject",
            INVTEXT("Reload Project"),
            INVTEXT("No tooltip for Reload Project"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.ReloadProject"),
            FExecuteAction::CreateRaw(this, &FAssistModule::ReloadProject)));

        // Section Asset
        FToolMenuSection& SectionAsset = AssistMenu->AddSection("Asset", LOCTEXT("Label", "Asset"));
        TArray<FString> MenuModuleArray;
        MenuModule.ParseIntoArray(MenuModuleArray, TEXT("."));
        FString AssetType = MenuModuleArray.Last();
        if (AssetType.EndsWith("Editor")) {
            AssetType = AssetType.LeftChop(6);
        }
        SectionAsset.AddEntry(FToolMenuEntry::InitMenuEntry(
            "ReloadAsset",
            INVTEXT("Reload " + AssetType),
            INVTEXT("No tooltip for Reload " + AssetType),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.ReloadProject"),
            FExecuteAction::CreateRaw(this, &FAssistModule::ReloadAsset, AssetType)));

        // Section Layout
        FToolMenuSection& SectionLayout = AssistMenu->AddSection("Layout", LOCTEXT("Label", "Layout"));
        SectionLayout.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetHorizontalLayout",
            INVTEXT("Set Horizontal Layout"),
            INVTEXT("No tooltip for Set Horizontal Layout"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetHorizontalLayout"),
            FExecuteAction::CreateRaw(this, &FAssistModule::SetLayout, true)));

        SectionLayout.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetVerticalLayout",
            INVTEXT("Set Vertical Layout"),
            INVTEXT("No tooltip for Set Vertical Layout"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetVerticalLayout"),
            FExecuteAction::CreateRaw(this, &FAssistModule::SetLayout, false)));

        // Section Language
        FToolMenuSection& SectionLanguage = AssistMenu->AddSection("Language", LOCTEXT("Label", "Language"));
        SectionLanguage.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetLanguageToEn",
            INVTEXT("en"),
            INVTEXT("Set Language to en"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetLanguageToEn"),
            FExecuteAction::CreateRaw(this, &FAssistModule::SetCurrentLanguage, FString("en"))));

        SectionLanguage.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetLanguageToZhHans",
            INVTEXT("zh-Hans"),
            INVTEXT("Set Language to zh-Hans"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetLanguageToZhHans"),
            FExecuteAction::CreateRaw(this, &FAssistModule::SetCurrentLanguage, FString("zh-hans"))));
    }
}

void FAssistModule::ReloadProject() {
    FText DialogText              = FText::Format(LOCTEXT("PluginButtonDialogText", "Reload {0} Project ?"), FText::FromString(FApp::GetProjectName()));
    EAppReturnType::Type Decision = FMessageDialog::Open(EAppMsgType::OkCancel, DialogText);
    if (Decision == EAppReturnType::Ok) {
        FUnrealEdMisc::Get().RestartEditor(false);
        // FText OutFailReason;
        // GameProjectUtils::OpenProject(FPaths::GetProjectFilePath(), OutFailReason);
    }
}

void FAssistModule::ReloadAsset(const FString AssetType) {
    UPackage* AssetPackage = nullptr;
    if (AssetType == "Level") {
        if (UWorld* World = GEditor->GetEditorWorldContext().World()) {
            AssetPackage = World->GetPackage();
        }
    } else {
        UAssetEditorSubsystem* AssetEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
        if (AssetEditorSubsystem) {
            TArray<UObject*> Assets = AssetEditorSubsystem->GetAllEditedAssets();
            for (auto& Asset : Assets) {
                IAssetEditorInstance* AssetEditor = AssetEditorSubsystem->FindEditorForAsset(Asset, false);
                // FAssetEditorToolkit* Editor       = static_cast<FAssetEditorToolkit*>(AssetEditor);
                TSharedPtr<SDockTab> Tab = AssetEditor->GetAssociatedTabManager()->GetOwnerTab();
                if (Tab->GetParentWindow()->IsActive() && Tab->IsForeground()) {
                    AssetPackage = Asset->GetPackage();
                    break;
                }
            }
        }
    }

    if (AssetPackage) {
        UE_LOG(LogTemp, Warning, TEXT("Reload %s"), *AssetPackage->GetPathName());
        TArray<UPackage*> PackagesToReload = {AssetPackage};
        bool AnyPackagesReloaded           = false;
        FText ErrorMessage;
        UEditorLoadingAndSavingUtils::ReloadPackages(PackagesToReload, AnyPackagesReloaded, ErrorMessage, EReloadPackagesInteractionMode::Interactive);
        if (ErrorMessage.ToString().Len() > 0) {
            FMessageDialog::Open(EAppMsgType::Ok, ErrorMessage);
        }
    }
}

void FAssistModule::SetLayout(const bool IsHorizontal) {
    // GConfig->SetFloat(TEXT("ContentBrowser"), TEXT("ContentBrowserTab1.VerticalSplitter.SlotSize0"), 0.2f, GEditorPerProjectIni);
    // GConfig->SetFloat(TEXT("ContentBrowser"), TEXT("ContentBrowserTab1.VerticalSplitter.SlotSize1"), 0.8f, GEditorPerProjectIni);
    // GConfig->SetInt(TEXT("ContentBrowser"), TEXT("ContentBrowserTab1.CurrentViewType"), 0, GEditorPerProjectIni);
    // GConfig->SetInt(TEXT("ContentBrowser"), TEXT("ContentBrowserTab1.ThumbnailSize"), 1, GEditorPerProjectIni);
    // GConfig->Flush(false, GEditorPerProjectIni);

    float LeftPanelWidth    = 740.0f;
    float BottomPanelHeight = 600.0f;
    float LeftPanelOffset   = 4.0f;
    float TaskBarHeight     = 60.0f;
    // FDisplayMetrics DisplayMetrics;
    // FDisplayMetrics::RebuildDisplayMetrics(DisplayMetrics);
    // const float DisplayWidth  = DisplayMetrics.PrimaryDisplayWidth;
    // const float DisplayHeight = DisplayMetrics.PrimaryDisplayHeight;

    FDisplayMetrics DisplayMetrics;
    FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);
    const float DisplayWidth     = DisplayMetrics.PrimaryDisplayWidth;
    const float DisplayHeight    = DisplayMetrics.PrimaryDisplayHeight;
    const float DPIScale         = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayMetrics.PrimaryDisplayWorkAreaRect.Left, DisplayMetrics.PrimaryDisplayWorkAreaRect.Top);
    float AvailableDisplayHeight = DisplayHeight - TaskBarHeight;

    const TSharedRef<FGlobalTabmanager> GlobalTabmanager = FGlobalTabmanager::Get();

    // Set Layout
    TSharedRef<FTabManager::FLayout> PersistLayout = GlobalTabmanager->PersistLayout();
    TSharedRef<FTabManager::FArea> NewArea         = GlobalTabmanager->NewArea(0.0f, 0.0f);
    TSharedRef<FTabManager::FStack> NewStack       = GlobalTabmanager->NewStack();
    NewArea->SetWindow(FVector2D::Zero(), false);
    NewArea->Split(NewStack);
    NewStack->SetForegroundTab(FTabId{"ContentBrowserTab1"});
    NewStack->AddTab("ContentBrowserTab1", ETabState::OpenedTab);
    PersistLayout->AddArea(NewArea);
    FLayoutSaveRestore::SaveToConfig("EditorLayout", PersistLayout);

    EditorReinit();

    // Reshape Root Window
    TSharedPtr<SWindow> RootWindow = GlobalTabmanager->GetRootWindow();
    FVector2D RootWindowPosition   = IsHorizontal ? FVector2D(LeftPanelWidth, 0.0f) : FVector2D::Zero();
    FVector2D RootWindowSize       = IsHorizontal ? FVector2D(DisplayWidth - LeftPanelWidth, AvailableDisplayHeight) : FVector2D(DisplayWidth, AvailableDisplayHeight - BottomPanelHeight);
    RootWindow->ReshapeWindow(RootWindowPosition, RootWindowSize);

    // Reshape Content Browser Window
    TSharedPtr<SWindow> ContentBrowserWindow = nullptr;
    TArray<TSharedRef<SWindow>> AllVisibleWindowsOrdered;
    FSlateApplication::Get().GetAllVisibleWindowsOrdered(AllVisibleWindowsOrdered);
    for (auto Window : AllVisibleWindowsOrdered) {
        if (Window->GetTitle().EqualTo(FText::FromString("Content Browser"))) {
            ContentBrowserWindow = Window;
            break;
        }
    }
    if (ContentBrowserWindow) {
        FVector2D Position = IsHorizontal ? FVector2D::Zero() : FVector2D(0, AvailableDisplayHeight - BottomPanelHeight);
        FVector2D Size     = IsHorizontal ? FVector2D(LeftPanelWidth, AvailableDisplayHeight) : FVector2D(DisplayWidth, BottomPanelHeight);
        ContentBrowserWindow->ReshapeWindow(Position, Size);
        TSharedRef<SWidget> Content = ContentBrowserWindow->GetContent();
        if (SWidget* Result = FindWidget(Content, "SAssetView")) {
            SAssetView& AssetView = static_cast<SAssetView&>(*Result);
            AssetView.SetCurrentViewType(IsHorizontal ? EAssetViewType::List : EAssetViewType::Tile);
            AssetView.SetCurrentThumbnailSize(IsHorizontal ? EThumbnailSize::Small : EThumbnailSize::Medium);
            GetMutableDefault<UContentBrowserSettings>()->bShowAllFolder = false;
            GetMutableDefault<UContentBrowserSettings>()->PostEditChange();
        }
    }
}

void FAssistModule::SetCurrentLanguage(const FString Language) {
    UKismetInternationalizationLibrary::SetCurrentLanguage(Language);
}

SWidget* FAssistModule::FindWidget(TSharedRef<SWidget> Parent, FString TypeAsString) {
    if (Parent->GetTypeAsString() == "SAssetView") {
        return &Parent.Get();
    }
    for (int32 i = 0; i < Parent->GetChildren()->Num(); i++) {
        SWidget* W = FindWidget(Parent->GetChildren()->GetChildAt(i), TypeAsString);
        if (W) return W;
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssistModule, Assist)
