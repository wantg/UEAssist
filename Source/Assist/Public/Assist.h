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
};
