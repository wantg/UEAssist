#include "Assist.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "FileHelpers.h"
#include "UnrealEdGlobals.h"
#include "HAL/PlatformApplicationMisc.h"
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
    for (auto& MenuModule : AssistConfig.SupportedEditors) {
        FString AssistMenuName     = "AssistMenu";
        FString MainMenuName       = FString(MenuModule).Append(".MainMenu");
        FString AssistMenuFullName = FString(MainMenuName).Append(".").Append(AssistMenuName);

        UToolMenus* ToolMenus = UToolMenus::Get();
        UToolMenu* AssistMenu = ToolMenus->RegisterMenu(FName(AssistMenuFullName));

        // Section Project
        FToolMenuSection& SectionProject = AssistMenu->AddSection("Project", LOCTEXT("Label", "Project"));
        SectionProject.AddEntry(FToolMenuEntry::InitMenuEntry(
            "ReloadProject",
            INVTEXT("Reload Project"),
            INVTEXT("No tooltip for Reload Project"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.ReloadProject"),
            FExecuteAction::CreateRaw(this, &FAssistModule::ReloadProject)));

        // Section Layout
        FToolMenuSection& SectionLayout = AssistMenu->AddSection("Layout", LOCTEXT("Label", "Layout"));
        SectionLayout.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetHorizontalLayout",
            INVTEXT("Set Horizontal Layout"),
            INVTEXT("No tooltip for Set Horizontal Layout"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetHorizontalLayout"),
            FExecuteAction::CreateRaw(this, &FAssistModule::SetHorizontalLayout)));

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
            FExecuteAction::CreateRaw(this, &FAssistModule::ReloadAsset)));

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

void FAssistModule::SetHorizontalLayout() {
    float LeftPanelWidth  = 740.0f;
    float LeftPanelOffset = 4.0f;
    float TaskBarHeight   = 60.0f;
    // FDisplayMetrics DisplayMetrics;
    // FDisplayMetrics::RebuildDisplayMetrics(DisplayMetrics);
    // const float DisplayWidth  = DisplayMetrics.PrimaryDisplayWidth;
    // const float DisplayHeight = DisplayMetrics.PrimaryDisplayHeight;

    FDisplayMetrics DisplayMetrics;
    FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);
    const float DisplayWidth  = DisplayMetrics.PrimaryDisplayWidth;
    const float DisplayHeight = DisplayMetrics.PrimaryDisplayHeight;
    const float DPIScale      = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayMetrics.PrimaryDisplayWorkAreaRect.Left, DisplayMetrics.PrimaryDisplayWorkAreaRect.Top);

    FString WindowsLocalAppData        = FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
    FString EditorLayoutConfigFilepath = FPaths::Combine(WindowsLocalAppData, "UnrealEngine/Editor/EditorLayout.json");
    if (TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin("Assist")) {
        const FString& ResourceFilePath = ThisPlugin->GetBaseDir() / TEXT("Resources/DefaultEditorLayout.json");
        FString EditorLayoutContent;
        FFileHelper::LoadFileToString(EditorLayoutContent, *ResourceFilePath);
        EditorLayoutContent = EditorLayoutContent.Replace(TEXT("\"WindowSize_X\": 600"), *FString::Printf(TEXT("\"WindowSize_X\": %f"), (LeftPanelWidth / DPIScale) - LeftPanelOffset));
        EditorLayoutContent = EditorLayoutContent.Replace(TEXT("\"WindowSize_Y\": 1680"), *FString::Printf(TEXT("\"WindowSize_Y\": %f"), ((DisplayHeight - TaskBarHeight) / DPIScale) - LeftPanelOffset));
        FFileHelper::SaveStringToFile(EditorLayoutContent, *EditorLayoutConfigFilepath);
    }

    EditorReinit();

    TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
    RootWindow->ReshapeWindow(FVector2D(LeftPanelWidth, 0.0f), FVector2D(DisplayWidth - LeftPanelWidth, DisplayHeight - TaskBarHeight));
}

void FAssistModule::ReloadAsset() {
    UAssetEditorSubsystem* AssetEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
    if (AssetEditorSubsystem) {
        UPackage* AssetPackage  = nullptr;
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

        if (!AssetPackage) {
            if (UWorld* World = GEditor->GetEditorWorldContext().World()) {
                AssetPackage = World->GetPackage();
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
}

void FAssistModule::SetCurrentLanguage(const FString Language) {
    UKismetInternationalizationLibrary::SetCurrentLanguage(Language);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssistModule, Assist)
