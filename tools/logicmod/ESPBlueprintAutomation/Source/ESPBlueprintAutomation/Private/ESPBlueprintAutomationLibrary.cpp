#include "ESPBlueprintAutomationLibrary.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "BlueprintGraph/Classes/K2Node_CallFunction.h"
#include "BlueprintGraph/Classes/K2Node_CallArrayFunction.h"
#include "BlueprintGraph/Classes/K2Node_ComponentBoundEvent.h"
#include "BlueprintGraph/Classes/K2Node_CustomEvent.h"
#include "BlueprintGraph/Classes/K2Node_DynamicCast.h"
#include "BlueprintGraph/Classes/K2Node_Event.h"
#include "BlueprintGraph/Classes/K2Node_IfThenElse.h"
#include "BlueprintGraph/Classes/K2Node_MacroInstance.h"
#include "BlueprintGraph/Classes/K2Node_Self.h"
#include "BlueprintGraph/Classes/K2Node_SwitchEnum.h"
#include "BlueprintGraph/Classes/K2Node_VariableGet.h"
#include "BlueprintGraph/Classes/K2Node_VariableSet.h"
#include "BlueprintGraph/Classes/K2Node.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "FileHelpers.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompilerModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace {

const FName OverlayVariableName(TEXT("ESP_OverlayWidget"));
const FName BridgeGenderDiagnosticVariableName(TEXT("ESP_BridgeGenderDiagnostic"));
const FName BridgeGenderDiagnosticCodeVariableName(TEXT("ESP_BridgeGenderDiagnosticCode"));
// __DEPRECATED_20260716__ [reason: the visible guide now renders all accepted targets in one widget]
// const FName OverlayTargetVariableName(TEXT("ESP_Target"));
// const FName OverlaySetTargetEventName(TEXT("PalworldResourceESP_WidgetSetTarget"));
const FName OverlayTargetsVariableName(TEXT("ESP_Targets"));
const FName OverlayGenderLoggedVariableName(TEXT("ESP_GenderLogged"));
const FName OverlayGenderDiagnosticVariableName(TEXT("ESP_GenderDiagnostic"));
const FName OverlayGenderDiagnosticCodeVariableName(TEXT("ESP_GenderDiagnosticCode"));
const FName OverlayAddTargetEventName(TEXT("PalworldResourceESP_WidgetAddTarget"));
const FName OverlayClearTargetsEventName(TEXT("PalworldResourceESP_WidgetClearTargets"));
const FName PanelBridgeVariableName(TEXT("ESP_Bridge"));
const FName PanelVariableName(TEXT("ESP_PanelWidget"));
const FName ControlRevisionVariableName(TEXT("ESP_ControlRevision"));
const FName RuntimeEnabledVariableName(TEXT("ESP_RuntimeEnabled"));
const FName ProfileIdVariableName(TEXT("ESP_ProfileId"));
const FName PresetIdVariableName(TEXT("ESP_PresetId"));
const FName CaptureRequestedVariableName(TEXT("ESP_CaptureRequested"));
const FName LanguageIdVariableName(TEXT("ESP_LanguageId"));

UBlueprint* LoadBlueprint(const TCHAR* Path) {
    return LoadObject<UBlueprint>(nullptr, Path);
}

UEdGraph* EventGraph(UBlueprint* Blueprint) {
    return Blueprint ? FBlueprintEditorUtils::FindEventGraph(Blueprint) : nullptr;
}

void ClearGraph(UEdGraph* Graph) {
    if (!Graph) {
        return;
    }
    TArray<UEdGraphNode*> Nodes = Graph->Nodes;
    for (UEdGraphNode* Node : Nodes) {
        if (Node) {
            FBlueprintEditorUtils::RemoveNode(nullptr, Node, true);
        }
    }
}

FEdGraphPinType ObjectPin(UClass* Class) {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
    PinType.PinSubCategoryObject = Class;
    return PinType;
}

FEdGraphPinType ObjectArrayPin(UClass* Class) {
    FEdGraphPinType PinType = ObjectPin(Class);
    PinType.ContainerType = EPinContainerType::Array;
    return PinType;
}

FEdGraphPinType IntPin() {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    return PinType;
}

FEdGraphPinType BoolPin() {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    return PinType;
}

FEdGraphPinType StringPin() {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    return PinType;
}

UK2Node_CustomEvent* AddCustomEvent(
    UBlueprint* Blueprint,
    UEdGraph* Graph,
    const TCHAR* EventName,
    int32 X,
    int32 Y,
    const TArray<TPair<FName, FEdGraphPinType>>& Inputs) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddCustomEvent begin name=%s graph=%s inputs=%d"), EventName, *GetNameSafe(Graph), Inputs.Num());
    if (!Blueprint || !Graph) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddCustomEvent missing blueprint or graph name=%s"), EventName);
        return nullptr;
    }
    UK2Node_CustomEvent* Node = NewObject<UK2Node_CustomEvent>(Graph);
    if (!Node) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddCustomEvent NewObject failed name=%s"), EventName);
        return nullptr;
    }
    Node->CustomFunctionName = FName(EventName);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    for (const TPair<FName, FEdGraphPinType>& Input : Inputs) {
        UEdGraphPin* Pin = Node->CreateUserDefinedPin(Input.Key, Input.Value, EGPD_Output);
        UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddCustomEvent pin name=%s pin=%s"), *Input.Key.ToString(), Pin ? TEXT("ok") : TEXT("null"));
    }
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddCustomEvent done name=%s pins=%d"), EventName, Node->Pins.Num());
    return Node;
}

UK2Node_Event* AddOverrideEvent(
    UBlueprint* Blueprint,
    UEdGraph* Graph,
    UClass* OwnerClass,
    const TCHAR* FunctionName,
    int32 X,
    int32 Y) {
    UFunction* Function = OwnerClass ? OwnerClass->FindFunctionByName(FName(FunctionName)) : nullptr;
    if (!Blueprint || !Graph || !Function) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddOverrideEvent missing input function=%s"), FunctionName);
        return nullptr;
    }
    UK2Node_Event* Node = NewObject<UK2Node_Event>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->EventReference.SetFromField<UFunction>(Function, false);
    Node->bOverrideFunction = true;
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddOverrideEvent done function=%s pins=%d"), FunctionName, Node->Pins.Num());
    return Node;
}

UK2Node_CallFunction* AddSelfCall(
    UBlueprint* Blueprint,
    UEdGraph* Graph,
    const TCHAR* FunctionName,
    int32 X,
    int32 Y) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddSelfCall begin function=%s graph=%s"), FunctionName, *GetNameSafe(Graph));
    if (!Blueprint || !Graph) {
        return nullptr;
    }
    UFunction* Function = nullptr;
    if (Blueprint->SkeletonGeneratedClass) {
        Function = Blueprint->SkeletonGeneratedClass->FindFunctionByName(FName(FunctionName));
    }
    if (!Function && Blueprint->GeneratedClass) {
        Function = Blueprint->GeneratedClass->FindFunctionByName(FName(FunctionName));
    }
    if (!Function) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddSelfCall function not found function=%s skeleton=%s generated=%s"), FunctionName, *GetNameSafe(Blueprint->SkeletonGeneratedClass), *GetNameSafe(Blueprint->GeneratedClass));
        return nullptr;
    }
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    if (!Node) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddSelfCall NewObject failed function=%s"), FunctionName);
        return nullptr;
    }
    Node->SetFromFunction(Function);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddSelfCall done function=%s pins=%d"), FunctionName, Node->Pins.Num());
    return Node;
}

UK2Node_CallFunction* AddStaticCall(
    UEdGraph* Graph,
    UClass* OwnerClass,
    const TCHAR* FunctionName,
    int32 X,
    int32 Y) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddStaticCall begin owner=%s function=%s graph=%s"), *GetNameSafe(OwnerClass), FunctionName, *GetNameSafe(Graph));
    if (!OwnerClass) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddStaticCall owner class missing function=%s"), FunctionName);
        return nullptr;
    }
    UFunction* Function = OwnerClass->FindFunctionByName(FName(FunctionName));
    if (!Function) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddStaticCall function missing owner=%s function=%s"), *GetNameSafe(OwnerClass), FunctionName);
        return nullptr;
    }
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    if (!Node) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddStaticCall NewObject failed function=%s"), FunctionName);
        return nullptr;
    }
    Node->FunctionReference.SetExternalMember(FName(FunctionName), OwnerClass);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddStaticCall done function=%s pins=%d"), FunctionName, Node->Pins.Num());
    return Node;
}

UK2Node_CallArrayFunction* AddArrayCall(
    UEdGraph* Graph,
    const TCHAR* FunctionName,
    int32 X,
    int32 Y) {
    UFunction* Function = UKismetArrayLibrary::StaticClass()->FindFunctionByName(FName(FunctionName));
    if (!Graph || !Function) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddArrayCall missing graph or function=%s"), FunctionName);
        return nullptr;
    }
    UK2Node_CallArrayFunction* Node = NewObject<UK2Node_CallArrayFunction>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->SetFromFunction(Function);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    // __MISTAKE_20260716__ [reason: diagnostic pin logging was initially inserted in AddArrayCall]
    // for (UEdGraphPin* MacroPin : Node->Pins) {
    //     UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] ForEach pin name=%s direction=%d category=%s"),
    //         MacroPin ? *MacroPin->PinName.ToString() : TEXT("<null>"),
    //         MacroPin ? static_cast<int32>(MacroPin->Direction) : -1,
    //         MacroPin ? *MacroPin->PinType.PinCategory.ToString() : TEXT("<null>"));
    // }
    return Node;
}

UK2Node_MacroInstance* AddForEachLoop(UEdGraph* Graph, int32 X, int32 Y) {
    UEdGraph* MacroGraph = LoadObject<UEdGraph>(
        nullptr,
        TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoop"));
    if (!Graph || !MacroGraph) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddForEachLoop missing graph or macro"));
        return nullptr;
    }
    UK2Node_MacroInstance* Node = NewObject<UK2Node_MacroInstance>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->SetMacroGraph(MacroGraph);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    for (UEdGraphPin* MacroPin : Node->Pins) {
        UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] ForEach pin name=%s direction=%d category=%s"),
            MacroPin ? *MacroPin->PinName.ToString() : TEXT("<null>"),
            MacroPin ? static_cast<int32>(MacroPin->Direction) : -1,
            MacroPin ? *MacroPin->PinType.PinCategory.ToString() : TEXT("<null>"));
    }
    return Node;
}

UK2Node_Self* AddSelfNode(UEdGraph* Graph, int32 X, int32 Y) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddSelfNode begin graph=%s"), *GetNameSafe(Graph));
    if (!Graph) {
        return nullptr;
    }
    UK2Node_Self* Node = NewObject<UK2Node_Self>(Graph);
    if (!Node) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddSelfNode NewObject failed"));
        return nullptr;
    }
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] AddSelfNode done pins=%d"), Node->Pins.Num());
    return Node;
}

UK2Node_VariableGet* AddVariableGet(UEdGraph* Graph, const FName& VariableName, int32 X, int32 Y) {
    if (!Graph) {
        return nullptr;
    }
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->VariableReference.SetSelfMember(VariableName);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UK2Node_VariableGet* AddExternalVariableGet(
    UEdGraph* Graph,
    const FName& VariableName,
    UClass* OwnerClass,
    int32 X,
    int32 Y) {
    if (!Graph || !OwnerClass) {
        return nullptr;
    }
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->VariableReference.SetExternalMember(VariableName, OwnerClass);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UK2Node_VariableSet* AddVariableSet(UEdGraph* Graph, const FName& VariableName, int32 X, int32 Y) {
    if (!Graph) {
        return nullptr;
    }
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->VariableReference.SetSelfMember(VariableName);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UK2Node_VariableSet* AddExternalVariableSet(
    UEdGraph* Graph,
    const FName& VariableName,
    UClass* OwnerClass,
    int32 X,
    int32 Y) {
    if (!Graph || !OwnerClass) {
        return nullptr;
    }
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->VariableReference.SetExternalMember(VariableName, OwnerClass);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UK2Node_IfThenElse* AddBranch(UEdGraph* Graph, int32 X, int32 Y) {
    if (!Graph) {
        return nullptr;
    }
    UK2Node_IfThenElse* Node = NewObject<UK2Node_IfThenElse>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UK2Node_SwitchEnum* AddSwitchEnum(UEdGraph* Graph, UEnum* Enum, int32 X, int32 Y) {
    if (!Graph || !Enum) {
        return nullptr;
    }
    UK2Node_SwitchEnum* Node = NewObject<UK2Node_SwitchEnum>(Graph);
    if (!Node) {
        return nullptr;
    }
    // UK2Node_SwitchEnum::SetEnum is not exported from BlueprintGraph in UE 5.1.
    // Populate the node's public enum metadata using the same logic before pins are allocated.
    Node->Enum = Enum;
    Node->EnumEntries.Empty();
    Node->EnumFriendlyNames.Empty();
    Enum->ConditionalPostLoad();
    for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums() - 1; ++EnumIndex) {
        const bool bHidden = Enum->HasMetaData(TEXT("Hidden"), EnumIndex)
            || Enum->HasMetaData(TEXT("Spacer"), EnumIndex);
        if (!bHidden) {
            Node->EnumEntries.Add(FName(*Enum->GetNameStringByIndex(EnumIndex)));
            Node->EnumFriendlyNames.Add(Enum->GetDisplayNameTextByIndex(EnumIndex));
        }
    }
    Node->bHasDefaultPin = false;
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UK2Node_DynamicCast* AddDynamicCast(UEdGraph* Graph, UClass* TargetClass, int32 X, int32 Y) {
    if (!Graph || !TargetClass) {
        return nullptr;
    }
    UK2Node_DynamicCast* Node = NewObject<UK2Node_DynamicCast>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->TargetType = TargetClass;
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

UEdGraphPin* Pin(UEdGraphNode* Node, const FName& Name) {
    return Node ? Node->FindPin(Name) : nullptr;
}

bool Link(UEdGraphNode* Source, const FName& SourcePin, UEdGraphNode* Target, const FName& TargetPin) {
    UEdGraphPin* From = Pin(Source, SourcePin);
    UEdGraphPin* To = Pin(Target, TargetPin);
    const bool bLinked = From && To && GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] Link %s.%s -> %s.%s source_pin=%s target_pin=%s result=%s"),
        *GetNameSafe(Source), *SourcePin.ToString(), *GetNameSafe(Target), *TargetPin.ToString(),
        From ? TEXT("ok") : TEXT("missing"), To ? TEXT("ok") : TEXT("missing"), bLinked ? TEXT("ok") : TEXT("failed"));
    return bLinked;
}

bool Link(UEdGraphNode* Source, const TCHAR* SourcePin, UEdGraphNode* Target, const TCHAR* TargetPin) {
    return Link(Source, FName(SourcePin), Target, FName(TargetPin));
}

bool SetClassPin(UEdGraphNode* Node, const FName& PinName, UClass* Class) {
    UEdGraphPin* Target = Pin(Node, PinName);
    if (!Target || !Class) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] SetClassPin failed node=%s pin=%s class=%s"), *GetNameSafe(Node), *PinName.ToString(), *GetNameSafe(Class));
        return false;
    }
    Target->DefaultObject = Class;
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] SetClassPin ok node=%s pin=%s class=%s"), *GetNameSafe(Node), *PinName.ToString(), *GetNameSafe(Class));
    return true;
}

bool SetPinDefault(UEdGraphNode* Node, const FName& PinName, const FString& Value) {
    UEdGraphPin* Target = Pin(Node, PinName);
    if (!Target) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] SetPinDefault missing node=%s pin=%s"), *GetNameSafe(Node), *PinName.ToString());
        return false;
    }
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Target, Value);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] SetPinDefault node=%s pin=%s value=%s"), *GetNameSafe(Node), *PinName.ToString(), *Value);
    return true;
}

bool SetPinDefaultText(UEdGraphNode* Node, const FName& PinName, const FText& Value) {
    UEdGraphPin* Target = Pin(Node, PinName);
    if (!Target) {
        return false;
    }
    Target->DefaultTextValue = Value;
    return true;
}

UK2Node_ComponentBoundEvent* AddButtonEvent(
    UWidgetBlueprint* Blueprint,
    UButton* Button,
    int32 X,
    int32 Y) {
    if (!Blueprint || !Button || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        Button->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        UButton::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UButton, OnClicked)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddButtonEvent missing graph/property button=%s"), *GetNameSafe(Button));
        return nullptr;
    }

    UK2Node_ComponentBoundEvent* Node = NewObject<UK2Node_ComponentBoundEvent>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->InitializeComponentBoundEventParams(ComponentProperty, DelegateProperty);
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    return Node;
}

// __DEPRECATED_20260716__ [reason: replaced by target-projected OnPaint guide]
bool BuildStaticOverlay(UWidgetBlueprint* Blueprint) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildOverlay begin blueprint=%s"), *GetNameSafe(Blueprint));
    if (!Blueprint || !Blueprint->WidgetTree) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay missing blueprint or widget tree"));
        return false;
    }

    UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ESP_Canvas"));
    USizeBox* Line = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ESP_TopGuideLine"));
    UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ESP_TopGuideLineImage"));
    if (!Canvas || !Line || !Image) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay widget construction failed canvas=%s line=%s image=%s"), Canvas ? TEXT("ok") : TEXT("null"), Line ? TEXT("ok") : TEXT("null"), Image ? TEXT("ok") : TEXT("null"));
        return false;
    }

    Line->SetWidthOverride(2.0f);
    Line->SetHeightOverride(220.0f);
    FLinearColor Green(0.15f, 1.0f, 0.25f, 0.95f);
    Image->SetColorAndOpacity(Green);
    Line->AddChild(Image);
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Line);
    if (!Slot) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay AddChildToCanvas failed"));
        return false;
    }
    Slot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
    Slot->SetAlignment(FVector2D(0.5f, 0.0f));
    Slot->SetOffsets(FMargin(-1.0f, 24.0f, 1.0f, 244.0f));
    Blueprint->WidgetTree->RootWidget = Canvas;
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildOverlay done"));
    return true;
}

bool BuildOverlay(
    UWidgetBlueprint* Blueprint,
    UClass* PalMonsterClass,
    UClass* CharacterParameterComponentClass,
    UClass* IndividualParameterClass,
    UEnum* GenderEnum,
    UClass* PalUtilityClass) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildOverlay dynamic begin blueprint=%s"), *GetNameSafe(Blueprint));
    if (!Blueprint || !Blueprint->WidgetTree || !PalMonsterClass || !CharacterParameterComponentClass
        || !IndividualParameterClass || !GenderEnum || !PalUtilityClass) {
        return false;
    }

    UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ESP_Canvas"));
    if (!Canvas) {
        return false;
    }
    Blueprint->WidgetTree->RootWidget = Canvas;

    // __DEPRECATED_20260716__ [reason: replaced by the multi-target array graph below]
#if 0
    const int32 ExistingTargetIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetVariableName);
    if (ExistingTargetIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetVariableName, ObjectPin(PalMonsterClass))) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetIndex].VarType = ObjectPin(PalMonsterClass);
    }

    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);

    UK2Node_CustomEvent* SetTarget = AddCustomEvent(Blueprint, Graph, *OverlaySetTargetEventName.ToString(), -1200, -500, {
        TPair<FName, FEdGraphPinType>(FName("Target"), ObjectPin(PalMonsterClass))
    });
    UK2Node_VariableSet* StoreTarget = AddVariableSet(Graph, OverlayTargetVariableName, -880, -500);
    if (!SetTarget || !StoreTarget
        || !Link(SetTarget, UEdGraphSchema_K2::PN_Then, StoreTarget, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetTarget, TEXT("Target"), StoreTarget, OverlayTargetVariableName)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay target storage graph failed"));
        return false;
    }

    UK2Node_Event* OnPaint = AddOverrideEvent(Blueprint, Graph, UUserWidget::StaticClass(), TEXT("OnPaint"), -1200, 0);
    UK2Node_VariableGet* TargetGet = AddVariableGet(Graph, OverlayTargetVariableName, -1200, 260);
    UK2Node_CallFunction* TargetValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -960, 260);
    UK2Node_IfThenElse* TargetBranch = AddBranch(Graph, -720, 0);
    UK2Node_CallFunction* ActorLocation = AddStaticCall(Graph, AActor::StaticClass(), TEXT("K2_GetActorLocation"), -960, 420);
    UK2Node_Self* Self = AddSelfNode(Graph, -1200, 620);
    UK2Node_CallFunction* PlayerController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), -960, 620);
    UK2Node_CallFunction* Project = AddStaticCall(Graph, UWidgetLayoutLibrary::StaticClass(), TEXT("ProjectWorldLocationToWidgetPosition"), -620, 420);
    UK2Node_IfThenElse* ProjectBranch = AddBranch(Graph, -320, 0);
    UK2Node_CallFunction* ViewportSize = AddStaticCall(Graph, UWidgetLayoutLibrary::StaticClass(), TEXT("GetViewportSize"), -620, 760);
    UK2Node_CallFunction* ViewportScale = AddStaticCall(Graph, UWidgetLayoutLibrary::StaticClass(), TEXT("GetViewportScale"), -620, 900);
    UK2Node_CallFunction* BreakViewport = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BreakVector2D"), -320, 760);
    UK2Node_CallFunction* RemoveScale = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Divide_DoubleDouble"), -40, 760);
    UK2Node_CallFunction* HalfWidth = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Multiply_DoubleDouble"), 220, 760);
    UK2Node_CallFunction* MakeStart = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("MakeVector2D"), 480, 760);
    UK2Node_CallFunction* DrawLine = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("DrawLine"), 220, 0);
    if (!OnPaint || !TargetGet || !TargetValid || !TargetBranch || !ActorLocation || !Self || !PlayerController
        || !Project || !ProjectBranch || !ViewportSize || !ViewportScale || !BreakViewport || !RemoveScale
        || !HalfWidth || !MakeStart || !DrawLine
        || !Link(OnPaint, UEdGraphSchema_K2::PN_Then, TargetBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(TargetGet, OverlayTargetVariableName, TargetValid, TEXT("Object"))
        || !Link(TargetValid, UEdGraphSchema_K2::PN_ReturnValue, TargetBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetBranch, UEdGraphSchema_K2::PN_Then, ProjectBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(TargetGet, OverlayTargetVariableName, ActorLocation, UEdGraphSchema_K2::PN_Self)
        || !Link(Self, UEdGraphSchema_K2::PN_Self, PlayerController, TEXT("WorldContextObject"))
        || !Link(PlayerController, UEdGraphSchema_K2::PN_ReturnValue, Project, TEXT("PlayerController"))
        || !Link(ActorLocation, UEdGraphSchema_K2::PN_ReturnValue, Project, TEXT("WorldLocation"))
        || !Link(Project, UEdGraphSchema_K2::PN_ReturnValue, ProjectBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ProjectBranch, UEdGraphSchema_K2::PN_Then, DrawLine, UEdGraphSchema_K2::PN_Execute)
        || !Link(OnPaint, TEXT("Context"), DrawLine, TEXT("Context"))
        || !Link(Project, TEXT("ScreenPosition"), DrawLine, TEXT("PositionB"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, ViewportSize, TEXT("WorldContextObject"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, ViewportScale, TEXT("WorldContextObject"))
        || !Link(ViewportSize, UEdGraphSchema_K2::PN_ReturnValue, BreakViewport, TEXT("InVec"))
        || !Link(BreakViewport, TEXT("X"), RemoveScale, TEXT("A"))
        || !Link(ViewportScale, UEdGraphSchema_K2::PN_ReturnValue, RemoveScale, TEXT("B"))
        || !Link(RemoveScale, UEdGraphSchema_K2::PN_ReturnValue, HalfWidth, TEXT("A"))
        || !Link(HalfWidth, UEdGraphSchema_K2::PN_ReturnValue, MakeStart, TEXT("X"))
        || !Link(MakeStart, UEdGraphSchema_K2::PN_ReturnValue, DrawLine, TEXT("PositionA"))
        || !SetPinDefault(HalfWidth, TEXT("B"), TEXT("0.5"))
        || !SetPinDefault(MakeStart, TEXT("Y"), TEXT("24.0"))
        || !SetPinDefault(DrawLine, TEXT("Tint"), TEXT("(R=0.15,G=1.0,B=0.25,A=0.95)"))
        || !SetPinDefault(DrawLine, TEXT("Thickness"), TEXT("1.5"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay OnPaint graph failed"));
        return false;
    }
#endif

    const int32 ExistingTargetsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetsVariableName);
    if (ExistingTargetsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetsVariableName, ObjectArrayPin(PalMonsterClass))) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetsIndex].VarType = ObjectArrayPin(PalMonsterClass);
    }

    const int32 ExistingGenderLoggedIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayGenderLoggedVariableName);
    if (ExistingGenderLoggedIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayGenderLoggedVariableName, BoolPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingGenderLoggedIndex].VarType = BoolPin();
    }

    const int32 ExistingGenderDiagnosticIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayGenderDiagnosticVariableName);
    if (ExistingGenderDiagnosticIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayGenderDiagnosticVariableName, StringPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingGenderDiagnosticIndex].VarType = StringPin();
    }

    const int32 ExistingGenderDiagnosticCodeIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayGenderDiagnosticCodeVariableName);
    if (ExistingGenderDiagnosticCodeIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayGenderDiagnosticCodeVariableName, IntPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingGenderDiagnosticCodeIndex].VarType = IntPin();
    }

    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);

    UK2Node_CustomEvent* AddTarget = AddCustomEvent(Blueprint, Graph, *OverlayAddTargetEventName.ToString(), -1500, -600, {
        TPair<FName, FEdGraphPinType>(FName("Target"), ObjectPin(PalMonsterClass))
    });
    UK2Node_VariableGet* AddTargetsGet = AddVariableGet(Graph, OverlayTargetsVariableName, -1240, -440);
    UK2Node_CallArrayFunction* AddUnique = AddArrayCall(Graph, TEXT("Array_AddUnique"), -960, -600);
    UK2Node_CustomEvent* ClearTargets = AddCustomEvent(Blueprint, Graph, *OverlayClearTargetsEventName.ToString(), -1500, -300, {});
    UK2Node_VariableGet* ClearTargetsGet = AddVariableGet(Graph, OverlayTargetsVariableName, -1240, -140);
    UK2Node_CallArrayFunction* ClearArray = AddArrayCall(Graph, TEXT("Array_Clear"), -960, -300);
    if (!AddTarget || !AddTargetsGet || !AddUnique || !ClearTargets || !ClearTargetsGet || !ClearArray
        || !Link(AddTarget, UEdGraphSchema_K2::PN_Then, AddUnique, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddTargetsGet, OverlayTargetsVariableName, AddUnique, TEXT("TargetArray"))
        || !Link(AddTarget, TEXT("Target"), AddUnique, TEXT("NewItem"))
        || !Link(ClearTargets, UEdGraphSchema_K2::PN_Then, ClearArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearTargetsGet, OverlayTargetsVariableName, ClearArray, TEXT("TargetArray"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay target array graph failed"));
        return false;
    }

    UK2Node_VariableGet* GenderLoggedGet = AddVariableGet(Graph, OverlayGenderLoggedVariableName, -700, -780);
    UK2Node_CallFunction* GenderNot = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -460, -780);
    UK2Node_IfThenElse* GenderGate = AddBranch(Graph, -180, -600);
    UK2Node_CallFunction* GenderTargetValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 60, -440);
    UK2Node_IfThenElse* GenderTargetBranch = AddBranch(Graph, 300, -600);
    UK2Node_CallFunction* CharacterParameter = AddStaticCall(Graph, PalMonsterClass, TEXT("GetCharacterParameterComponent"), 60, -280);
    UK2Node_CallFunction* CharacterParameterValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 540, -280);
    UK2Node_IfThenElse* CharacterParameterBranch = AddBranch(Graph, 780, -600);
    UK2Node_CallFunction* IndividualParameter = AddStaticCall(Graph, CharacterParameterComponentClass, TEXT("GetIndividualParameter"), 540, -120);
    UK2Node_CallFunction* IndividualParameterValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 1020, -120);
    UK2Node_IfThenElse* IndividualParameterBranch = AddBranch(Graph, 1260, -600);
    UK2Node_CallFunction* GenderType = AddStaticCall(Graph, IndividualParameterClass, TEXT("GetGenderType"), 1020, 40);
    UK2Node_SwitchEnum* GenderSwitch = AddSwitchEnum(Graph, GenderEnum, 1500, -600);
    UK2Node_Self* GenderSelf = AddSelfNode(Graph, 1500, 160);
    if (!GenderLoggedGet || !GenderNot || !GenderGate || !GenderTargetValid || !GenderTargetBranch
        || !CharacterParameter || !CharacterParameterValid || !CharacterParameterBranch
        || !IndividualParameter || !IndividualParameterValid || !IndividualParameterBranch
        || !GenderType || !GenderSwitch || !GenderSelf
        || !Link(AddUnique, UEdGraphSchema_K2::PN_Then, GenderGate, UEdGraphSchema_K2::PN_Execute)
        || !Link(GenderLoggedGet, OverlayGenderLoggedVariableName, GenderNot, TEXT("A"))
        || !Link(GenderNot, UEdGraphSchema_K2::PN_ReturnValue, GenderGate, UEdGraphSchema_K2::PN_Condition)
        || !Link(GenderGate, UEdGraphSchema_K2::PN_Then, GenderTargetBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddTarget, TEXT("Target"), GenderTargetValid, TEXT("Object"))
        || !Link(GenderTargetValid, UEdGraphSchema_K2::PN_ReturnValue, GenderTargetBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(GenderTargetBranch, UEdGraphSchema_K2::PN_Then, CharacterParameterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddTarget, TEXT("Target"), CharacterParameter, UEdGraphSchema_K2::PN_Self)
        || !Link(CharacterParameter, UEdGraphSchema_K2::PN_ReturnValue, CharacterParameterValid, TEXT("Object"))
        || !Link(CharacterParameterValid, UEdGraphSchema_K2::PN_ReturnValue, CharacterParameterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(CharacterParameterBranch, UEdGraphSchema_K2::PN_Then, IndividualParameterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(CharacterParameter, UEdGraphSchema_K2::PN_ReturnValue, IndividualParameter, UEdGraphSchema_K2::PN_Self)
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, IndividualParameterValid, TEXT("Object"))
        || !Link(IndividualParameterValid, UEdGraphSchema_K2::PN_ReturnValue, IndividualParameterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(IndividualParameterBranch, UEdGraphSchema_K2::PN_Then, GenderSwitch, UEdGraphSchema_K2::PN_Execute)
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, GenderType, UEdGraphSchema_K2::PN_Self)
        || !Link(GenderType, UEdGraphSchema_K2::PN_ReturnValue, GenderSwitch, TEXT("Selection"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay gender adapter graph failed"));
        return false;
    }

    auto AddGenderLogPath = [&](UEdGraphNode* ExecNode, const FName& ExecPin, const TCHAR* Value, int32 Code, int32 X, int32 Y) -> bool {
        UK2Node_VariableSet* StoreDiagnostic = AddVariableSet(Graph, OverlayGenderDiagnosticVariableName, X, Y);
        UK2Node_VariableSet* StoreDiagnosticCode = AddVariableSet(Graph, OverlayGenderDiagnosticCodeVariableName, X + 300, Y);
        UK2Node_CallFunction* Print = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("PrintString"), X + 600, Y);
        UK2Node_VariableSet* MarkLogged = AddVariableSet(Graph, OverlayGenderLoggedVariableName, X + 900, Y);
        const FString Message = FString::Printf(TEXT("[PalworldResourceESP] BLUEPRINT_GENDER value=%s"), Value);
        return StoreDiagnostic && StoreDiagnosticCode && Print && MarkLogged
            && Link(ExecNode, ExecPin, StoreDiagnostic, UEdGraphSchema_K2::PN_Execute)
            && Link(StoreDiagnostic, UEdGraphSchema_K2::PN_Then, StoreDiagnosticCode, UEdGraphSchema_K2::PN_Execute)
            && Link(StoreDiagnosticCode, UEdGraphSchema_K2::PN_Then, Print, UEdGraphSchema_K2::PN_Execute)
            && Link(GenderSelf, UEdGraphSchema_K2::PN_Self, Print, TEXT("WorldContextObject"))
            && Link(Print, UEdGraphSchema_K2::PN_Then, MarkLogged, UEdGraphSchema_K2::PN_Execute)
            && SetPinDefault(StoreDiagnostic, OverlayGenderDiagnosticVariableName, FString(Value))
            && SetPinDefault(StoreDiagnosticCode, OverlayGenderDiagnosticCodeVariableName, FString::FromInt(Code))
            && SetPinDefault(Print, TEXT("InString"), Message)
            && SetPinDefault(Print, TEXT("bPrintToScreen"), TEXT("false"))
            && SetPinDefault(Print, TEXT("bPrintToLog"), TEXT("true"))
            && SetPinDefault(MarkLogged, OverlayGenderLoggedVariableName, TEXT("true"));
    };
    if (!AddGenderLogPath(GenderTargetBranch, UEdGraphSchema_K2::PN_Else, TEXT("unknown"), 0, 1740, -1040)
        || !AddGenderLogPath(CharacterParameterBranch, UEdGraphSchema_K2::PN_Else, TEXT("unknown"), 0, 1740, -880)
        || !AddGenderLogPath(IndividualParameterBranch, UEdGraphSchema_K2::PN_Else, TEXT("unknown"), 0, 1740, -720)
        || !AddGenderLogPath(GenderSwitch, TEXT("None"), TEXT("unknown"), 0, 1740, -560)
        || !AddGenderLogPath(GenderSwitch, TEXT("Male"), TEXT("male"), 1, 1740, -400)
        || !AddGenderLogPath(GenderSwitch, TEXT("Female"), TEXT("female"), 2, 1740, -240)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay gender diagnostic graph failed"));
        return false;
    }

    UK2Node_Event* OnPaint = AddOverrideEvent(Blueprint, Graph, UUserWidget::StaticClass(), TEXT("OnPaint"), -1500, 80);
    UK2Node_VariableGet* TargetsGet = AddVariableGet(Graph, OverlayTargetsVariableName, -1500, 300);
    UK2Node_MacroInstance* ForEachTarget = AddForEachLoop(Graph, -1220, 80);
    UK2Node_CallFunction* TargetValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -940, 300);
    UK2Node_IfThenElse* TargetBranch = AddBranch(Graph, -700, 80);
    UK2Node_CallFunction* TargetDead = AddStaticCall(Graph, PalUtilityClass, TEXT("IsDead"), -700, 440);
    UK2Node_CallFunction* TargetNotDead = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -440, 440);
    UK2Node_IfThenElse* TargetAliveBranch = AddBranch(Graph, -420, 80);
    UK2Node_CallFunction* ActorLocation = AddStaticCall(Graph, AActor::StaticClass(), TEXT("K2_GetActorLocation"), -940, 460);
    UK2Node_Self* Self = AddSelfNode(Graph, -1220, 660);
    UK2Node_CallFunction* PlayerController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), -940, 660);
    UK2Node_CallFunction* Project = AddStaticCall(Graph, UWidgetLayoutLibrary::StaticClass(), TEXT("ProjectWorldLocationToWidgetPosition"), -600, 460);
    UK2Node_IfThenElse* ProjectBranch = AddBranch(Graph, -300, 80);
    UK2Node_CallFunction* ViewportSize = AddStaticCall(Graph, UWidgetLayoutLibrary::StaticClass(), TEXT("GetViewportSize"), -600, 800);
    UK2Node_CallFunction* ViewportScale = AddStaticCall(Graph, UWidgetLayoutLibrary::StaticClass(), TEXT("GetViewportScale"), -600, 940);
    UK2Node_CallFunction* BreakViewport = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BreakVector2D"), -300, 800);
    UK2Node_CallFunction* RemoveScale = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Divide_DoubleDouble"), -20, 800);
    UK2Node_CallFunction* HalfWidth = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Multiply_DoubleDouble"), 240, 800);
    UK2Node_CallFunction* MakeStart = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("MakeVector2D"), 500, 800);
    UK2Node_CallFunction* DrawLine = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("DrawLine"), 240, 80);
    if (!OnPaint || !TargetsGet || !ForEachTarget || !TargetValid || !TargetBranch || !TargetDead || !TargetNotDead
        || !TargetAliveBranch || !ActorLocation || !Self || !PlayerController
        || !Project || !ProjectBranch || !ViewportSize || !ViewportScale || !BreakViewport || !RemoveScale
        || !HalfWidth || !MakeStart || !DrawLine
        || !Link(OnPaint, UEdGraphSchema_K2::PN_Then, ForEachTarget, TEXT("Exec"))
        || !Link(TargetsGet, OverlayTargetsVariableName, ForEachTarget, TEXT("Array"))
        // __DEPRECATED_20260716__ [reason: UE 5.1 StandardMacros exposes this pin as LoopBody]
        // || !Link(ForEachTarget, TEXT("Loop Body"), TargetBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("LoopBody"), TargetBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("Array Element"), TargetValid, TEXT("Object"))
        || !Link(TargetValid, UEdGraphSchema_K2::PN_ReturnValue, TargetBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetBranch, UEdGraphSchema_K2::PN_Then, TargetAliveBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("Array Element"), TargetDead, TEXT("Actor"))
        || !Link(TargetDead, UEdGraphSchema_K2::PN_ReturnValue, TargetNotDead, TEXT("A"))
        || !Link(TargetNotDead, UEdGraphSchema_K2::PN_ReturnValue, TargetAliveBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetAliveBranch, UEdGraphSchema_K2::PN_Then, ProjectBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("Array Element"), ActorLocation, UEdGraphSchema_K2::PN_Self)
        || !Link(Self, UEdGraphSchema_K2::PN_Self, PlayerController, TEXT("WorldContextObject"))
        || !Link(PlayerController, UEdGraphSchema_K2::PN_ReturnValue, Project, TEXT("PlayerController"))
        || !Link(ActorLocation, UEdGraphSchema_K2::PN_ReturnValue, Project, TEXT("WorldLocation"))
        || !Link(Project, UEdGraphSchema_K2::PN_ReturnValue, ProjectBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ProjectBranch, UEdGraphSchema_K2::PN_Then, DrawLine, UEdGraphSchema_K2::PN_Execute)
        || !Link(OnPaint, TEXT("Context"), DrawLine, TEXT("Context"))
        || !Link(Project, TEXT("ScreenPosition"), DrawLine, TEXT("PositionB"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, ViewportSize, TEXT("WorldContextObject"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, ViewportScale, TEXT("WorldContextObject"))
        || !Link(ViewportSize, UEdGraphSchema_K2::PN_ReturnValue, BreakViewport, TEXT("InVec"))
        || !Link(BreakViewport, TEXT("X"), RemoveScale, TEXT("A"))
        || !Link(ViewportScale, UEdGraphSchema_K2::PN_ReturnValue, RemoveScale, TEXT("B"))
        || !Link(RemoveScale, UEdGraphSchema_K2::PN_ReturnValue, HalfWidth, TEXT("A"))
        || !Link(HalfWidth, UEdGraphSchema_K2::PN_ReturnValue, MakeStart, TEXT("X"))
        || !Link(MakeStart, UEdGraphSchema_K2::PN_ReturnValue, DrawLine, TEXT("PositionA"))
        || !SetPinDefault(HalfWidth, TEXT("B"), TEXT("0.5"))
        || !SetPinDefault(MakeStart, TEXT("Y"), TEXT("24.0"))
        || !SetPinDefault(DrawLine, TEXT("Tint"), TEXT("(R=0.15,G=1.0,B=0.25,A=0.95)"))
        || !SetPinDefault(DrawLine, TEXT("Thickness"), TEXT("1.5"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay multi-target OnPaint graph failed"));
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildOverlay dynamic compile status=%d nodes=%d"), static_cast<int32>(Blueprint->Status), Graph->Nodes.Num());
    return Blueprint->Status != BS_Error;
}

bool EnsureMemberVariable(
    UBlueprint* Blueprint,
    const FName& Name,
    const FEdGraphPinType& Type,
    const FString& DefaultValue = FString()) {
    int32 Index = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, Name);
    if (Index == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type)) {
            return false;
        }
        Index = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, Name);
    } else {
        Blueprint->NewVariables[Index].VarType = Type;
    }
    if (Index == INDEX_NONE) {
        return false;
    }
    if (!DefaultValue.IsEmpty()) {
        Blueprint->NewVariables[Index].DefaultValue = DefaultValue;
    }
    return true;
}

bool PrepareModActorControls(UBlueprint* Blueprint) {
    if (!Blueprint
        || !EnsureMemberVariable(Blueprint, ControlRevisionVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, RuntimeEnabledVariableName, BoolPin(), TEXT("true"))
        || !EnsureMemberVariable(Blueprint, ProfileIdVariableName, IntPin(), TEXT("2"))
        || !EnsureMemberVariable(Blueprint, PresetIdVariableName, IntPin(), TEXT("1"))
        || !EnsureMemberVariable(Blueprint, CaptureRequestedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, LanguageIdVariableName, IntPin(), TEXT("0"))) {
        return false;
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Blueprint->Status != BS_Error;
}

UTextBlock* AddPanelText(
    UWidgetBlueprint* Blueprint,
    UPanelWidget* Parent,
    const FName& Name,
    const TCHAR* Text,
    int32 FontSize) {
    UTextBlock* Widget = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    if (!Widget) {
        return nullptr;
    }
    Widget->bIsVariable = true;
    Widget->SetText(FText::FromString(Text));
    Widget->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.95f, 0.96f, 1.0f)));
    FSlateFontInfo Font = Widget->Font;
    Font.Size = FontSize;
    Widget->SetFont(Font);
    Widget->SetAutoWrapText(true);
    Parent->AddChild(Widget);
    return Widget;
}

UButton* AddPanelButton(
    UWidgetBlueprint* Blueprint,
    UPanelWidget* Parent,
    const FName& ButtonName,
    const FName& TextName,
    const TCHAR* Text) {
    UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
    UTextBlock* Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
    if (!Button || !Label) {
        return nullptr;
    }
    Button->bIsVariable = true;
    Button->IsFocusable = true;
    Button->SetBackgroundColor(FLinearColor(0.15f, 0.18f, 0.21f, 0.96f));
    Label->bIsVariable = true;
    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.97f, 0.98f, 1.0f)));
    FSlateFontInfo Font = Label->Font;
    Font.Size = 15;
    Label->SetFont(Font);
    Label->SetJustification(ETextJustify::Center);
    Button->AddChild(Label);
    Parent->AddChild(Button);
    return Button;
}

bool AppendExternalAssignment(
    UEdGraph* Graph,
    UClass* OwnerClass,
    UK2Node_VariableGet* BridgeGet,
    UEdGraphNode*& ExecTail,
    const FName& VariableName,
    const FString& Value,
    int32 X,
    int32 Y) {
    UK2Node_VariableSet* Set = AddExternalVariableSet(Graph, VariableName, OwnerClass, X, Y);
    if (!Set
        || !SetPinDefault(Set, VariableName, Value)
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, Set, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, Set, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    ExecTail = Set;
    return true;
}

bool AppendTextAssignment(
    UEdGraph* Graph,
    UEdGraphNode*& ExecTail,
    const FName& TextVariableName,
    const TCHAR* Text,
    int32 X,
    int32 Y) {
    UK2Node_VariableGet* TextGet = AddVariableGet(Graph, TextVariableName, X, Y + 80);
    UK2Node_CallFunction* SetText = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 260, Y);
    if (!TextGet || !SetText
        || !SetPinDefaultText(SetText, TEXT("InText"), FText::FromString(Text))
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetText, UEdGraphSchema_K2::PN_Execute)
        || !Link(TextGet, TextVariableName, SetText, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    ExecTail = SetText;
    return true;
}

bool AppendRevisionIncrement(
    UEdGraph* Graph,
    UClass* OwnerClass,
    UK2Node_VariableGet* BridgeGet,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    UK2Node_VariableGet* RevisionGet = AddExternalVariableGet(Graph, ControlRevisionVariableName, OwnerClass, X, Y + 120);
    UK2Node_CallFunction* AddOne = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_IntInt"), X + 260, Y + 120);
    UK2Node_VariableSet* RevisionSet = AddExternalVariableSet(Graph, ControlRevisionVariableName, OwnerClass, X + 520, Y);
    if (!RevisionGet || !AddOne || !RevisionSet
        || !SetPinDefault(AddOne, TEXT("B"), TEXT("1"))
        || !Link(BridgeGet, PanelBridgeVariableName, RevisionGet, UEdGraphSchema_K2::PN_Self)
        || !Link(RevisionGet, ControlRevisionVariableName, AddOne, TEXT("A"))
        || !Link(AddOne, UEdGraphSchema_K2::PN_ReturnValue, RevisionSet, ControlRevisionVariableName)
        || !Link(BridgeGet, PanelBridgeVariableName, RevisionSet, UEdGraphSchema_K2::PN_Self)
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, RevisionSet, UEdGraphSchema_K2::PN_Execute)) {
        return false;
    }
    ExecTail = RevisionSet;
    return true;
}

struct FPanelControlAssignment {
    FName Name;
    FString Value;
};

bool BuildPanelControlEvent(
    UWidgetBlueprint* Blueprint,
    UButton* Button,
    UClass* ModActorClass,
    const TArray<FPanelControlAssignment>& Assignments,
    const FName& StatusTextName,
    const TCHAR* StatusText,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddButtonEvent(Blueprint, Button, -1300, Y);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1040, Y + 120);
    UEdGraphNode* ExecTail = Event;
    if (!Graph || !Event || !BridgeGet) {
        return false;
    }

    int32 X = -1000;
    for (const FPanelControlAssignment& Assignment : Assignments) {
        if (!AppendExternalAssignment(Graph, ModActorClass, BridgeGet, ExecTail, Assignment.Name, Assignment.Value, X, Y)) {
            return false;
        }
        X += 300;
    }
    if (StatusTextName != NAME_None
        && !AppendTextAssignment(Graph, ExecTail, StatusTextName, StatusText, X, Y)) {
        return false;
    }
    X += StatusTextName == NAME_None ? 0 : 560;
    return AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, X, Y);
}

bool BuildPanelVisibilityEvent(
    UWidgetBlueprint* Blueprint,
    UButton* Button,
    const FName& TargetName,
    const TCHAR* Visibility,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddButtonEvent(Blueprint, Button, -1000, Y);
    UK2Node_VariableGet* TargetGet = AddVariableGet(Graph, TargetName, -740, Y + 100);
    UK2Node_CallFunction* SetVisibility = AddStaticCall(Graph, UWidget::StaticClass(), TEXT("SetVisibility"), -460, Y);
    return Graph && Event && TargetGet && SetVisibility
        && SetPinDefault(SetVisibility, TEXT("InVisibility"), Visibility)
        && Link(Event, UEdGraphSchema_K2::PN_Then, SetVisibility, UEdGraphSchema_K2::PN_Execute)
        && Link(TargetGet, TargetName, SetVisibility, UEdGraphSchema_K2::PN_Self);
}

struct FPanelTranslation {
    FName TextName;
    const TCHAR* Chinese;
    const TCHAR* English;
};

bool BuildPanelLanguageEvent(
    UWidgetBlueprint* Blueprint,
    UButton* Button,
    UClass* ModActorClass,
    int32 LanguageId,
    const TArray<FPanelTranslation>& Translations,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddButtonEvent(Blueprint, Button, -1400, Y);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1160, Y + 120);
    UEdGraphNode* ExecTail = Event;
    if (!Graph || !Event || !BridgeGet
        || !AppendExternalAssignment(
            Graph,
            ModActorClass,
            BridgeGet,
            ExecTail,
            LanguageIdVariableName,
            FString::FromInt(LanguageId),
            -1120,
            Y)) {
        return false;
    }

    int32 X = -820;
    for (const FPanelTranslation& Translation : Translations) {
        const TCHAR* Text = LanguageId == 0 ? Translation.Chinese : Translation.English;
        if (!AppendTextAssignment(Graph, ExecTail, Translation.TextName, Text, X, Y)) {
            return false;
        }
        X += 560;
    }
    return AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, X, Y);
}

bool BuildPanel(UWidgetBlueprint* Blueprint, UClass* ModActorClass) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPanel begin blueprint=%s"), *GetNameSafe(Blueprint));
    if (!Blueprint || !Blueprint->WidgetTree || !ModActorClass) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);
    if (!EnsureMemberVariable(Blueprint, PanelBridgeVariableName, ObjectPin(ModActorClass))) {
        return false;
    }

    UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ESP_PanelCanvas"));
    USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ESP_PanelSize"));
    UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ESP_PanelBorder"));
    UVerticalBox* Content = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_PanelContent"));
    if (!Canvas || !Size || !Border || !Content) {
        return false;
    }
    Size->SetWidthOverride(540.0f);
    Size->SetHeightOverride(690.0f);
    Border->SetBrushColor(FLinearColor(0.045f, 0.055f, 0.065f, 0.97f));
    Border->SetPadding(FMargin(18.0f));
    Border->AddChild(Content);
    Size->AddChild(Border);
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Size);
    PanelSlot->SetPosition(FVector2D(28.0f, 72.0f));
    PanelSlot->SetSize(FVector2D(540.0f, 690.0f));
    Blueprint->WidgetTree->RootWidget = Canvas;

    UTextBlock* Title = AddPanelText(Blueprint, Content, TEXT("ESP_TitleText"), TEXT("帕鲁资源 ESP"), 24);
    UTextBlock* RuntimeStatus = AddPanelText(Blueprint, Content, TEXT("ESP_RuntimeStatusText"), TEXT("Mod 状态 / Mod: 开启 / On"), 16);
    UHorizontalBox* RuntimeRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_RuntimeRow"));
    Content->AddChild(RuntimeRow);
    UButton* RuntimeOn = AddPanelButton(Blueprint, RuntimeRow, TEXT("ESP_RuntimeOnButton"), TEXT("ESP_RuntimeOnText"), TEXT("开启 Mod"));
    UButton* RuntimeOff = AddPanelButton(Blueprint, RuntimeRow, TEXT("ESP_RuntimeOffButton"), TEXT("ESP_RuntimeOffText"), TEXT("关闭 Mod"));

    UTextBlock* PresetHeading = AddPanelText(Blueprint, Content, TEXT("ESP_PresetHeadingText"), TEXT("性能预设"), 18);
    UHorizontalBox* PresetRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_PresetRow"));
    Content->AddChild(PresetRow);
    UButton* PresetLow = AddPanelButton(Blueprint, PresetRow, TEXT("ESP_PresetLowButton"), TEXT("ESP_PresetLowText"), TEXT("低开销"));
    UButton* PresetBalanced = AddPanelButton(Blueprint, PresetRow, TEXT("ESP_PresetBalancedButton"), TEXT("ESP_PresetBalancedText"), TEXT("均衡"));
    UButton* PresetQuality = AddPanelButton(Blueprint, PresetRow, TEXT("ESP_PresetQualityButton"), TEXT("ESP_PresetQualityText"), TEXT("高质量"));

    UTextBlock* LanguageHeading = AddPanelText(Blueprint, Content, TEXT("ESP_LanguageHeadingText"), TEXT("Language"), 18);
    UHorizontalBox* LanguageRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_LanguageRow"));
    Content->AddChild(LanguageRow);
    UButton* Chinese = AddPanelButton(Blueprint, LanguageRow, TEXT("ESP_ChineseButton"), TEXT("ESP_ChineseText"), TEXT("中文"));
    UButton* English = AddPanelButton(Blueprint, LanguageRow, TEXT("ESP_EnglishButton"), TEXT("ESP_EnglishText"), TEXT("English"));

    UButton* AdvancedExpand = AddPanelButton(Blueprint, Content, TEXT("ESP_AdvancedExpandButton"), TEXT("ESP_AdvancedExpandText"), TEXT("高级诊断"));
    UVerticalBox* Advanced = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_AdvancedBox"));
    Advanced->bIsVariable = true;
    Advanced->SetVisibility(ESlateVisibility::Collapsed);
    Content->AddChild(Advanced);
    UTextBlock* ModeHeading = AddPanelText(Blueprint, Advanced, TEXT("ESP_ModeHeadingText"), TEXT("实验模式"), 18);
    UTextBlock* ModeStatus = AddPanelText(Blueprint, Advanced, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 当前分批 / Current chunked"), 15);
    UHorizontalBox* ModeRowA = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ModeRowA"));
    UHorizontalBox* ModeRowB = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ModeRowB"));
    Advanced->AddChild(ModeRowA);
    Advanced->AddChild(ModeRowB);
    UButton* ModeOff = AddPanelButton(Blueprint, ModeRowA, TEXT("ESP_ModeOffButton"), TEXT("ESP_ModeOffText"), TEXT("关闭"));
    UButton* ModeSnapshot = AddPanelButton(Blueprint, ModeRowA, TEXT("ESP_ModeSnapshotButton"), TEXT("ESP_ModeSnapshotText"), TEXT("单次快照"));
    UButton* ModeChunked = AddPanelButton(Blueprint, ModeRowB, TEXT("ESP_ModeChunkedButton"), TEXT("ESP_ModeChunkedText"), TEXT("当前分批"));
    UButton* ModeEvent = AddPanelButton(Blueprint, ModeRowB, TEXT("ESP_ModeEventButton"), TEXT("ESP_ModeEventText"), TEXT("事件优先"));
    UTextBlock* CaptureStatus = AddPanelText(Blueprint, Advanced, TEXT("ESP_CaptureStatusText"), TEXT("采集状态 / Capture: 已停止 / Stopped"), 15);
    UHorizontalBox* CaptureRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_CaptureRow"));
    Advanced->AddChild(CaptureRow);
    UButton* CaptureStart = AddPanelButton(Blueprint, CaptureRow, TEXT("ESP_CaptureStartButton"), TEXT("ESP_CaptureStartText"), TEXT("开始采集"));
    UButton* CaptureStop = AddPanelButton(Blueprint, CaptureRow, TEXT("ESP_CaptureStopButton"), TEXT("ESP_CaptureStopText"), TEXT("停止采集"));
    UButton* AdvancedCollapse = AddPanelButton(Blueprint, Advanced, TEXT("ESP_AdvancedCollapseButton"), TEXT("ESP_AdvancedCollapseText"), TEXT("收起高级诊断"));

    if (!Title || !RuntimeStatus || !RuntimeOn || !RuntimeOff || !PresetHeading || !PresetLow || !PresetBalanced
        || !PresetQuality || !LanguageHeading || !Chinese || !English || !AdvancedExpand || !ModeHeading
        || !ModeStatus || !ModeOff || !ModeSnapshot || !ModeChunked || !ModeEvent || !CaptureStatus
        || !CaptureStart || !CaptureStop || !AdvancedCollapse) {
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) {
        return false;
    }

    int32 Y = -1200;
    auto Control = [&](UButton* Button, const TArray<FPanelControlAssignment>& Assignments, const FName& StatusName, const TCHAR* Status) {
        const bool bOk = BuildPanelControlEvent(Blueprint, Button, ModActorClass, Assignments, StatusName, Status, Y);
        Y += 360;
        return bOk;
    };
    if (!Control(RuntimeOn, {{RuntimeEnabledVariableName, TEXT("true")}}, TEXT("ESP_RuntimeStatusText"), TEXT("Mod 状态 / Mod: 开启 / On"))
        || !Control(RuntimeOff, {{RuntimeEnabledVariableName, TEXT("false")}}, TEXT("ESP_RuntimeStatusText"), TEXT("Mod 状态 / Mod: 关闭 / Off"))
        || !Control(PresetLow, {{PresetIdVariableName, TEXT("0")}}, NAME_None, TEXT(""))
        || !Control(PresetBalanced, {{PresetIdVariableName, TEXT("1")}}, NAME_None, TEXT(""))
        || !Control(PresetQuality, {{PresetIdVariableName, TEXT("2")}}, NAME_None, TEXT(""))
        || !Control(ModeOff, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("0")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 关闭 / Off"))
        || !Control(ModeSnapshot, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("1")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 单次快照 / Snapshot once"))
        || !Control(ModeChunked, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("2")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 当前分批 / Current chunked"))
        || !Control(ModeEvent, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("3")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 事件优先 / Event first"))
        || !Control(CaptureStart, {{CaptureRequestedVariableName, TEXT("true")}}, TEXT("ESP_CaptureStatusText"), TEXT("采集状态 / Capture: 采集中 / Recording"))
        || !Control(CaptureStop, {{CaptureRequestedVariableName, TEXT("false")}}, TEXT("ESP_CaptureStatusText"), TEXT("采集状态 / Capture: 已停止 / Stopped"))) {
        return false;
    }

    if (!BuildPanelVisibilityEvent(Blueprint, AdvancedExpand, TEXT("ESP_AdvancedBox"), TEXT("Visible"), Y)) {
        return false;
    }
    Y += 360;
    if (!BuildPanelVisibilityEvent(Blueprint, AdvancedCollapse, TEXT("ESP_AdvancedBox"), TEXT("Collapsed"), Y)) {
        return false;
    }
    Y += 360;

    const TArray<FPanelTranslation> Translations = {
        {TEXT("ESP_TitleText"), TEXT("帕鲁资源 ESP"), TEXT("Pal & Resource ESP")},
        {TEXT("ESP_RuntimeOnText"), TEXT("开启 Mod"), TEXT("Enable Mod")},
        {TEXT("ESP_RuntimeOffText"), TEXT("关闭 Mod"), TEXT("Disable Mod")},
        {TEXT("ESP_PresetHeadingText"), TEXT("性能预设"), TEXT("Performance preset")},
        {TEXT("ESP_PresetLowText"), TEXT("低开销"), TEXT("Low overhead")},
        {TEXT("ESP_PresetBalancedText"), TEXT("均衡"), TEXT("Balanced")},
        {TEXT("ESP_PresetQualityText"), TEXT("高质量"), TEXT("High quality")},
        {TEXT("ESP_AdvancedExpandText"), TEXT("高级诊断"), TEXT("Advanced diagnostics")},
        {TEXT("ESP_ModeHeadingText"), TEXT("实验模式"), TEXT("Experiment mode")},
        {TEXT("ESP_ModeOffText"), TEXT("关闭"), TEXT("Off")},
        {TEXT("ESP_ModeSnapshotText"), TEXT("单次快照"), TEXT("Snapshot once")},
        {TEXT("ESP_ModeChunkedText"), TEXT("当前分批"), TEXT("Current chunked")},
        {TEXT("ESP_ModeEventText"), TEXT("事件优先"), TEXT("Event first")},
        {TEXT("ESP_CaptureStartText"), TEXT("开始采集"), TEXT("Start capture")},
        {TEXT("ESP_CaptureStopText"), TEXT("停止采集"), TEXT("Stop capture")},
        {TEXT("ESP_AdvancedCollapseText"), TEXT("收起高级诊断"), TEXT("Collapse diagnostics")},
    };
    if (!BuildPanelLanguageEvent(Blueprint, Chinese, ModActorClass, 0, Translations, Y)
        || !BuildPanelLanguageEvent(Blueprint, English, ModActorClass, 1, Translations, Y + 720)) {
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPanel compile status=%d nodes=%d"), static_cast<int32>(Blueprint->Status), Graph->Nodes.Num());
    return Blueprint->Status != BS_Error;
}

bool BuildModActor(UBlueprint* Blueprint, UClass* PalMonsterClass, UClass* OverlayClass, UClass* PanelClass) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor begin blueprint=%s pal_class=%s overlay_class=%s panel_class=%s"), *GetNameSafe(Blueprint), *GetNameSafe(PalMonsterClass), *GetNameSafe(OverlayClass), *GetNameSafe(PanelClass));
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph || !PanelClass || !PrepareModActorControls(Blueprint)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor event graph missing blueprint=%s"), *GetNameSafe(Blueprint));
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor event graph=%s nodes_before=%d"), *GetNameSafe(Graph), Graph->Nodes.Num());
    ClearGraph(Graph);

    const int32 ExistingOverlayIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayVariableName);
    if (ExistingOverlayIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayVariableName, ObjectPin(OverlayClass))) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor overlay variable creation failed"));
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingOverlayIndex].VarType = ObjectPin(OverlayClass);
    }

    if (!EnsureMemberVariable(Blueprint, PanelVariableName, ObjectPin(PanelClass))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor panel variable creation failed"));
        return false;
    }

    const int32 ExistingBridgeGenderIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, BridgeGenderDiagnosticVariableName);
    if (ExistingBridgeGenderIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, BridgeGenderDiagnosticVariableName, StringPin())) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor gender diagnostic variable creation failed"));
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingBridgeGenderIndex].VarType = StringPin();
    }

    const int32 ExistingBridgeGenderCodeIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, BridgeGenderDiagnosticCodeVariableName);
    if (ExistingBridgeGenderCodeIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, BridgeGenderDiagnosticCodeVariableName, IntPin())) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor gender diagnostic code variable creation failed"));
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingBridgeGenderCodeIndex].VarType = IntPin();
    }

    UClass* ActorClass = AActor::StaticClass();
    TArray<TPair<FName, FEdGraphPinType>> ActorInput;
    ActorInput.Add(TPair<FName, FEdGraphPinType>(FName("BridgeActor"), ObjectPin(ActorClass)));
    UK2Node_CustomEvent* PostBeginPlay = AddCustomEvent(Blueprint, Graph, TEXT("PostBeginPlay"), -900, -200, {});
    UK2Node_CustomEvent* BridgeReady = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_LuaBridgeReady"), -900, 80, ActorInput);
    UK2Node_CustomEvent* Reset = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_ResetSession"), -900, 360, {
        TPair<FName, FEdGraphPinType>(FName("SessionIndex"), IntPin())
    });
    UK2Node_CustomEvent* SetTarget = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_SetTarget"), -900, 640, {
        TPair<FName, FEdGraphPinType>(FName("Target"), ObjectPin(PalMonsterClass)),
        TPair<FName, FEdGraphPinType>(FName("SessionIndex"), IntPin())
    });
    UK2Node_CustomEvent* ClearTarget = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_ClearTarget"), -900, 920, {
        TPair<FName, FEdGraphPinType>(FName("SessionIndex"), IntPin())
    });
    UK2Node_CustomEvent* TogglePanel = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_TogglePanel"), -900, 1520, {});
    if (!PostBeginPlay || !BridgeReady || !Reset || !SetTarget || !ClearTarget || !TogglePanel) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor custom event creation failed post=%s ready=%s reset=%s set=%s clear=%s toggle=%s"), PostBeginPlay ? TEXT("ok") : TEXT("null"), BridgeReady ? TEXT("ok") : TEXT("null"), Reset ? TEXT("ok") : TEXT("null"), SetTarget ? TEXT("ok") : TEXT("null"), ClearTarget ? TEXT("ok") : TEXT("null"), TogglePanel ? TEXT("ok") : TEXT("null"));
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor compiling event signatures nodes=%d"), Graph->Nodes.Num());
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor event signature compile status=%d skeleton=%s generated=%s"), static_cast<int32>(Blueprint->Status), *GetNameSafe(Blueprint->SkeletonGeneratedClass), *GetNameSafe(Blueprint->GeneratedClass));
    if (Blueprint->Status == BS_Error) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor event signature compile failed"));
        return false;
    }

    UK2Node_VariableGet* PanelGet = AddVariableGet(Graph, PanelVariableName, -620, 1640);
    UK2Node_CallFunction* PanelValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -380, 1640);
    UK2Node_IfThenElse* PanelBranch = AddBranch(Graph, -120, 1520);
    UK2Node_CallFunction* RemovePanel = AddStaticCall(Graph, UUserWidget::StaticClass(), TEXT("RemoveFromParent"), 140, 1440);
    UK2Node_VariableSet* ClearPanel = AddVariableSet(Graph, PanelVariableName, 420, 1440);
    UK2Node_CallFunction* CloseController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), 420, 1600);
    UK2Node_Self* CloseWorldContext = AddSelfNode(Graph, 140, 1680);
    UK2Node_VariableSet* HideCursor = AddExternalVariableSet(Graph, TEXT("bShowMouseCursor"), APlayerController::StaticClass(), 700, 1440);
    UK2Node_CallFunction* GameOnly = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("SetInputMode_GameOnly"), 980, 1440);

    UK2Node_CallFunction* OpenController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), 140, 1920);
    UK2Node_Self* OpenWorldContext = AddSelfNode(Graph, -120, 2000);
    UK2Node_CallFunction* CreatePanel = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), 420, 1760);
    UK2Node_DynamicCast* CastPanel = AddDynamicCast(Graph, PanelClass, 700, 1760);
    UK2Node_VariableSet* StorePanel = AddVariableSet(Graph, PanelVariableName, 980, 1760);
    UK2Node_VariableSet* SetPanelBridge = AddExternalVariableSet(Graph, PanelBridgeVariableName, PanelClass, 1260, 1760);
    UK2Node_Self* ActorSelf = AddSelfNode(Graph, 980, 1960);
    UK2Node_CallFunction* AddPanelToViewport = AddStaticCall(Graph, UUserWidget::StaticClass(), TEXT("AddToViewport"), 1540, 1760);
    UK2Node_VariableSet* ShowCursor = AddExternalVariableSet(Graph, TEXT("bShowMouseCursor"), APlayerController::StaticClass(), 1820, 1760);
    UK2Node_CallFunction* GameAndUi = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("SetInputMode_GameAndUIEx"), 2100, 1760);

    if (!PanelGet || !PanelValid || !PanelBranch || !RemovePanel || !ClearPanel || !CloseController || !CloseWorldContext
        || !HideCursor || !GameOnly || !OpenController || !OpenWorldContext || !CreatePanel || !CastPanel || !StorePanel
        || !SetPanelBridge || !ActorSelf || !AddPanelToViewport || !ShowCursor || !GameAndUi
        || !SetClassPin(CreatePanel, TEXT("WidgetType"), PanelClass)
        || !SetPinDefault(CloseController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetPinDefault(OpenController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetPinDefault(HideCursor, TEXT("bShowMouseCursor"), TEXT("false"))
        || !SetPinDefault(ShowCursor, TEXT("bShowMouseCursor"), TEXT("true"))
        || !SetPinDefault(AddPanelToViewport, TEXT("ZOrder"), TEXT("100"))
        || !Link(TogglePanel, UEdGraphSchema_K2::PN_Then, PanelBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(PanelGet, PanelVariableName, PanelValid, TEXT("Object"))
        || !Link(PanelValid, UEdGraphSchema_K2::PN_ReturnValue, PanelBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(PanelBranch, UEdGraphSchema_K2::PN_Then, RemovePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(PanelGet, PanelVariableName, RemovePanel, UEdGraphSchema_K2::PN_Self)
        || !Link(RemovePanel, UEdGraphSchema_K2::PN_Then, ClearPanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearPanel, UEdGraphSchema_K2::PN_Then, HideCursor, UEdGraphSchema_K2::PN_Execute)
        || !Link(CloseWorldContext, UEdGraphSchema_K2::PN_Self, CloseController, TEXT("WorldContextObject"))
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, HideCursor, UEdGraphSchema_K2::PN_Self)
        || !Link(HideCursor, UEdGraphSchema_K2::PN_Then, GameOnly, UEdGraphSchema_K2::PN_Execute)
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, GameOnly, TEXT("PlayerController"))
        || !Link(PanelBranch, UEdGraphSchema_K2::PN_Else, CreatePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenWorldContext, UEdGraphSchema_K2::PN_Self, OpenController, TEXT("WorldContextObject"))
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, CreatePanel, TEXT("OwningPlayer"))
        || !Link(CreatePanel, UEdGraphSchema_K2::PN_Then, CastPanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreatePanel, UEdGraphSchema_K2::PN_ReturnValue, CastPanel, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastPanel, UEdGraphSchema_K2::PN_CastSucceeded, StorePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, StorePanel, PanelVariableName)
        || !Link(StorePanel, UEdGraphSchema_K2::PN_Then, SetPanelBridge, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, SetPanelBridge, UEdGraphSchema_K2::PN_Self)
        || !Link(ActorSelf, UEdGraphSchema_K2::PN_Self, SetPanelBridge, PanelBridgeVariableName)
        || !Link(SetPanelBridge, UEdGraphSchema_K2::PN_Then, AddPanelToViewport, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, AddPanelToViewport, UEdGraphSchema_K2::PN_Self)
        || !Link(AddPanelToViewport, UEdGraphSchema_K2::PN_Then, ShowCursor, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, ShowCursor, UEdGraphSchema_K2::PN_Self)
        || !Link(ShowCursor, UEdGraphSchema_K2::PN_Then, GameAndUi, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, GameAndUi, TEXT("PlayerController"))
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, GameAndUi, TEXT("InWidgetToFocus"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor panel toggle graph failed"));
        return false;
    }

    // PostBeginPlay stays passive. Lua discovers this actor and invokes SetTarget only after gameplay is active.
    UK2Node_VariableGet* OverlayGet = AddVariableGet(Graph, OverlayVariableName, -620, 760);
    UK2Node_CallFunction* OverlayValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -380, 760);
    UK2Node_IfThenElse* OverlayBranch = AddBranch(Graph, -120, 640);
    UK2Node_CallFunction* CreateWidget = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), 140, 760);
    UK2Node_DynamicCast* CastOverlay = AddDynamicCast(Graph, OverlayClass, 420, 760);
    UK2Node_VariableSet* StoreOverlay = AddVariableSet(Graph, OverlayVariableName, 700, 760);
    UK2Node_CallFunction* AddToViewport = AddStaticCall(Graph, UUserWidget::StaticClass(), TEXT("AddToViewport"), 980, 760);
    UK2Node_CallFunction* AddExistingTarget = AddStaticCall(Graph, OverlayClass, *OverlayAddTargetEventName.ToString(), 140, 560);
    UK2Node_CallFunction* AddNewTarget = AddStaticCall(Graph, OverlayClass, *OverlayAddTargetEventName.ToString(), 1260, 760);
    UK2Node_VariableGet* ExistingGenderDiagnostic = AddExternalVariableGet(Graph, OverlayGenderDiagnosticVariableName, OverlayClass, 420, 520);
    UK2Node_VariableSet* StoreExistingGenderDiagnostic = AddVariableSet(Graph, BridgeGenderDiagnosticVariableName, 700, 520);
    UK2Node_VariableGet* ExistingGenderDiagnosticCode = AddExternalVariableGet(Graph, OverlayGenderDiagnosticCodeVariableName, OverlayClass, 420, 640);
    UK2Node_VariableSet* StoreExistingGenderDiagnosticCode = AddVariableSet(Graph, BridgeGenderDiagnosticCodeVariableName, 980, 520);
    UK2Node_VariableGet* NewGenderDiagnostic = AddExternalVariableGet(Graph, OverlayGenderDiagnosticVariableName, OverlayClass, 1540, 720);
    UK2Node_VariableSet* StoreNewGenderDiagnostic = AddVariableSet(Graph, BridgeGenderDiagnosticVariableName, 1820, 720);
    UK2Node_VariableGet* NewGenderDiagnosticCode = AddExternalVariableGet(Graph, OverlayGenderDiagnosticCodeVariableName, OverlayClass, 1540, 880);
    UK2Node_VariableSet* StoreNewGenderDiagnosticCode = AddVariableSet(Graph, BridgeGenderDiagnosticCodeVariableName, 2100, 720);
    if (!OverlayGet || !OverlayValid || !OverlayBranch || !CreateWidget || !CastOverlay || !StoreOverlay || !AddToViewport
        || !AddExistingTarget || !AddNewTarget || !ExistingGenderDiagnostic || !StoreExistingGenderDiagnostic
        || !ExistingGenderDiagnosticCode || !StoreExistingGenderDiagnosticCode
        || !NewGenderDiagnostic || !StoreNewGenderDiagnostic || !NewGenderDiagnosticCode || !StoreNewGenderDiagnosticCode
        || !SetClassPin(CreateWidget, TEXT("WidgetType"), OverlayClass)
        || !Link(SetTarget, UEdGraphSchema_K2::PN_Then, OverlayBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, OverlayValid, TEXT("Object"))
        || !Link(OverlayValid, UEdGraphSchema_K2::PN_ReturnValue, OverlayBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(OverlayBranch, UEdGraphSchema_K2::PN_Then, AddExistingTarget, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, AddExistingTarget, UEdGraphSchema_K2::PN_Self)
        || !Link(SetTarget, TEXT("Target"), AddExistingTarget, TEXT("Target"))
        || !Link(OverlayBranch, UEdGraphSchema_K2::PN_Else, CreateWidget, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateWidget, UEdGraphSchema_K2::PN_Then, CastOverlay, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateWidget, UEdGraphSchema_K2::PN_ReturnValue, CastOverlay, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastOverlay, UEdGraphSchema_K2::PN_CastSucceeded, StoreOverlay, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreOverlay, OverlayVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, AddToViewport, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreOverlay, UEdGraphSchema_K2::PN_Then, AddToViewport, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddToViewport, UEdGraphSchema_K2::PN_Then, AddNewTarget, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, AddNewTarget, UEdGraphSchema_K2::PN_Self)
        || !Link(SetTarget, TEXT("Target"), AddNewTarget, TEXT("Target"))
        || !Link(AddExistingTarget, UEdGraphSchema_K2::PN_Then, StoreExistingGenderDiagnostic, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, ExistingGenderDiagnostic, UEdGraphSchema_K2::PN_Self)
        || !Link(ExistingGenderDiagnostic, OverlayGenderDiagnosticVariableName, StoreExistingGenderDiagnostic, BridgeGenderDiagnosticVariableName)
        || !Link(StoreExistingGenderDiagnostic, UEdGraphSchema_K2::PN_Then, StoreExistingGenderDiagnosticCode, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, ExistingGenderDiagnosticCode, UEdGraphSchema_K2::PN_Self)
        || !Link(ExistingGenderDiagnosticCode, OverlayGenderDiagnosticCodeVariableName, StoreExistingGenderDiagnosticCode, BridgeGenderDiagnosticCodeVariableName)
        || !Link(AddNewTarget, UEdGraphSchema_K2::PN_Then, StoreNewGenderDiagnostic, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, NewGenderDiagnostic, UEdGraphSchema_K2::PN_Self)
        || !Link(NewGenderDiagnostic, OverlayGenderDiagnosticVariableName, StoreNewGenderDiagnostic, BridgeGenderDiagnosticVariableName)
        || !Link(StoreNewGenderDiagnostic, UEdGraphSchema_K2::PN_Then, StoreNewGenderDiagnosticCode, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, NewGenderDiagnosticCode, UEdGraphSchema_K2::PN_Self)
        || !Link(NewGenderDiagnosticCode, OverlayGenderDiagnosticCodeVariableName, StoreNewGenderDiagnosticCode, BridgeGenderDiagnosticCodeVariableName)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor SetTarget overlay graph failed"));
        return false;
    }

    auto AddOverlayCleanup = [&](UK2Node_CustomEvent* Event, int32 Y) -> bool {
        UK2Node_VariableGet* Get = AddVariableGet(Graph, OverlayVariableName, -620, Y + 120);
        UK2Node_CallFunction* IsValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -380, Y + 120);
        UK2Node_IfThenElse* Branch = AddBranch(Graph, -120, Y);
        UK2Node_CallFunction* Remove = AddStaticCall(Graph, UUserWidget::StaticClass(), TEXT("RemoveFromParent"), 140, Y);
        UK2Node_VariableSet* Clear = AddVariableSet(Graph, OverlayVariableName, 420, Y);
        return Get && IsValid && Branch && Remove && Clear
            && Link(Event, UEdGraphSchema_K2::PN_Then, Branch, UEdGraphSchema_K2::PN_Execute)
            && Link(Get, OverlayVariableName, IsValid, TEXT("Object"))
            && Link(IsValid, UEdGraphSchema_K2::PN_ReturnValue, Branch, UEdGraphSchema_K2::PN_Condition)
            && Link(Branch, UEdGraphSchema_K2::PN_Then, Remove, UEdGraphSchema_K2::PN_Execute)
            && Link(Get, OverlayVariableName, Remove, UEdGraphSchema_K2::PN_Self)
            && Link(Remove, UEdGraphSchema_K2::PN_Then, Clear, UEdGraphSchema_K2::PN_Execute);
    };
    // __DEPRECATED_20260716__ [reason: ClearTarget now preserves the single overlay and empties its target array]
    // if (!AddOverlayCleanup(Reset, 360) || !AddOverlayCleanup(ClearTarget, 1120)) {
    if (!AddOverlayCleanup(Reset, 360)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor overlay cleanup graph failed"));
        return false;
    }

    UK2Node_VariableGet* ClearOverlayGet = AddVariableGet(Graph, OverlayVariableName, -620, 1240);
    UK2Node_CallFunction* ClearOverlayValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -380, 1240);
    UK2Node_IfThenElse* ClearOverlayBranch = AddBranch(Graph, -120, 1120);
    UK2Node_CallFunction* ClearOverlayTargets = AddStaticCall(Graph, OverlayClass, *OverlayClearTargetsEventName.ToString(), 140, 1120);
    if (!ClearOverlayGet || !ClearOverlayValid || !ClearOverlayBranch || !ClearOverlayTargets
        || !Link(ClearTarget, UEdGraphSchema_K2::PN_Then, ClearOverlayBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearOverlayGet, OverlayVariableName, ClearOverlayValid, TEXT("Object"))
        || !Link(ClearOverlayValid, UEdGraphSchema_K2::PN_ReturnValue, ClearOverlayBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ClearOverlayBranch, UEdGraphSchema_K2::PN_Then, ClearOverlayTargets, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearOverlayGet, OverlayVariableName, ClearOverlayTargets, UEdGraphSchema_K2::PN_Self)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor target clear graph failed"));
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor compiling nodes_after=%d"), Graph->Nodes.Num());
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor compile status=%d"), static_cast<int32>(Blueprint->Status));
    return Blueprint->Status != BS_Error;
}

} // namespace

bool UESPBlueprintAutomationLibrary::BuildPalworldResourceESPAssets() {
    UBlueprint* ModActor = LoadBlueprint(TEXT("/Game/Mods/PalworldResourceESP/ModActor"));
    UBlueprint* Bridge = LoadBlueprint(TEXT("/Game/Mods/PalworldResourceESP/BP_ESPBridge"));
    UWidgetBlueprint* Overlay = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPOverlay"));
    UWidgetBlueprint* Panel = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPanel"));
    UClass* PalMonsterClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalMonsterCharacter"));
    UClass* CharacterParameterComponentClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalCharacterParameterComponent"));
    UClass* IndividualParameterClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalIndividualCharacterParameter"));
    UEnum* GenderEnum = LoadObject<UEnum>(nullptr, TEXT("/Script/Pal.EPalGenderType"));
    UClass* PalUtilityClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalUtility"));
    UClass* OverlayClass = Overlay ? Overlay->GeneratedClass : nullptr;
    if (!ModActor || !Bridge || !Overlay || !Panel || !PalMonsterClass || !CharacterParameterComponentClass
        || !IndividualParameterClass || !GenderEnum || !PalUtilityClass) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] required asset or class missing"));
        return false;
    }

    if (!PrepareModActorControls(ModActor)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor control preparation failed"));
        return false;
    }
    if (!BuildOverlay(Overlay, PalMonsterClass, CharacterParameterComponentClass, IndividualParameterClass, GenderEnum, PalUtilityClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] overlay build failed"));
        return false;
    }
    UClass* ModActorClass = ModActor->GeneratedClass;
    if (!BuildPanel(Panel, ModActorClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] panel build failed"));
        return false;
    }
    Overlay->MarkPackageDirty();
    Panel->MarkPackageDirty();
    TArray<UPackage*> WidgetPackages{Overlay->GetOutermost(), Panel->GetOutermost()};
    if (!UEditorLoadingAndSavingUtils::SavePackages(WidgetPackages, true)) {
        return false;
    }
    OverlayClass = Overlay->GeneratedClass;
    UClass* PanelClass = Panel->GeneratedClass;
    if (!BuildModActor(ModActor, PalMonsterClass, OverlayClass, PanelClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor build failed"));
        return false;
    }
    ModActor->MarkPackageDirty();
    Bridge->MarkPackageDirty();
    TArray<UPackage*> Packages{ModActor->GetOutermost(), Bridge->GetOutermost()};
    return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}
