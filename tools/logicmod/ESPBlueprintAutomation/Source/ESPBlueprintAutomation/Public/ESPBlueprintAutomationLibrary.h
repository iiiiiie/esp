#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ESPBlueprintAutomationLibrary.generated.h"

UCLASS()
class ESPBLUEPRINTAUTOMATION_API UESPBlueprintAutomationLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="ESP Automation")
    static bool BuildPalworldResourceESPAssets();

    UFUNCTION(BlueprintCallable, Category="ESP Automation")
    static bool BuildPalworldResourceESPRichTextAssets();
};
