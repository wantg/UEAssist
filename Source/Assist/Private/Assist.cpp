#include "Assist.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetInternationalizationLibrary.h"
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
            FExecuteAction::CreateLambda([]() {
                FText DialogText              = FText::Format(LOCTEXT("PluginButtonDialogText", "Reload {0} Project ?"), FText::FromString(FApp::GetProjectName()));
                EAppReturnType::Type Decision = FMessageDialog::Open(EAppMsgType::OkCancel, DialogText);
                if (Decision == EAppReturnType::Ok) {
                    FUnrealEdMisc::Get().RestartEditor(false);
                    // FText OutFailReason;
                    // GameProjectUtils::OpenProject(FPaths::GetProjectFilePath(), OutFailReason);
                }
            })));

        // Section Language
        FToolMenuSection& SectionLanguage = AssistMenu->AddSection("Language", LOCTEXT("Label", "Language"));
        SectionLanguage.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetLanguageToEn",
            INVTEXT("en"),
            INVTEXT("Set Language to en"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetLanguageToEn"),
            FExecuteAction::CreateLambda([]() {
                UKismetInternationalizationLibrary::SetCurrentLanguage("en");
            })));

        SectionLanguage.AddEntry(FToolMenuEntry::InitMenuEntry(
            "SetLanguageToZhHans",
            INVTEXT("zh-Hans"),
            INVTEXT("Set Language to zh-Hans"),
            FSlateIcon(FAssistStyle::GetStyleSetName(), "Assist.SetLanguageToZhHans"),
            FExecuteAction::CreateLambda([]() {
                UKismetInternationalizationLibrary::SetCurrentLanguage("zh-hans");
            })));

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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssistModule, Assist)
