#pragma once

#include "AssistConfig.generated.h"

USTRUCT()
struct FAssistConfig {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FString> SupportedEditors;
};
