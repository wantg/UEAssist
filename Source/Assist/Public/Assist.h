#pragma once

#include "Modules/ModuleManager.h"
#include "AssistConfig.h"

class FAssistModule : public IModuleInterface {
   public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

   private:
    void init();
    void RegisterMenus();
    FAssistConfig AssistConfig;

    void ReloadProject();
    void ReloadAsset(const FString AssetType);
    void SetLayout(const bool IsHorizontal);
    void SetCurrentLanguage(const FString Language);

    void FindWidget(TSharedRef<SWidget> Parent, FString TypeString, TSharedPtr<SWidget>& Result);
};
