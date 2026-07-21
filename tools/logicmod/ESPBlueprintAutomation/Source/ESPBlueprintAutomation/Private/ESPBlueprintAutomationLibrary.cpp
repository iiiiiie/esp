#include "ESPBlueprintAutomationLibrary.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintGraph/Classes/K2Node_CallFunction.h"
#include "BlueprintGraph/Classes/K2Node_CallArrayFunction.h"
#include "BlueprintGraph/Classes/K2Node_BreakStruct.h"
#include "BlueprintGraph/Classes/K2Node_ClassDynamicCast.h"
#include "BlueprintGraph/Classes/K2Node_ComponentBoundEvent.h"
#include "BlueprintGraph/Classes/K2Node_CustomEvent.h"
#include "BlueprintGraph/Classes/K2Node_DynamicCast.h"
#include "BlueprintGraph/Classes/K2Node_Event.h"
#include "BlueprintGraph/Classes/K2Node_ExecutionSequence.h"
#include "BlueprintGraph/Classes/K2Node_FunctionEntry.h"
#include "BlueprintGraph/Classes/K2Node_FunctionResult.h"
#include "BlueprintGraph/Classes/K2Node_IfThenElse.h"
#include "BlueprintGraph/Classes/K2Node_GetDataTableRow.h"
#include "BlueprintGraph/Classes/K2Node_MacroInstance.h"
#include "BlueprintGraph/Classes/K2Node_MakeArray.h"
#include "BlueprintGraph/Classes/K2Node_Self.h"
#include "BlueprintGraph/Classes/K2Node_SwitchEnum.h"
#include "BlueprintGraph/Classes/K2Node_VariableGet.h"
#include "BlueprintGraph/Classes/K2Node_VariableSet.h"
#include "BlueprintGraph/Classes/K2Node.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ExpandableArea.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/RichTextBlock.h"
#include "Components/RichTextBlockDecorator.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Widget.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompilerModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetInputLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet/DataTableFunctionLibrary.h"
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
const FName OverlayTargetParametersVariableName(TEXT("ESP_TargetParameters"));
const FName OverlayTargetLevelsVariableName(TEXT("ESP_TargetLevels"));
const FName OverlayTargetDistancesVariableName(TEXT("ESP_TargetDistances"));
const FName OverlayTargetGendersVariableName(TEXT("ESP_TargetGenders"));
const FName OverlayTargetLuckyStatesVariableName(TEXT("ESP_TargetLuckyStates"));
const FName OverlayTargetBossStatesVariableName(TEXT("ESP_TargetBossStates"));
const FName OverlayTargetCaptureCountsVariableName(TEXT("ESP_TargetCaptureCounts"));
const FName OverlayTargetCharacterIdsVariableName(TEXT("ESP_TargetCharacterIds"));
const FName OverlayTargetElementMasksVariableName(TEXT("ESP_TargetElementMasks"));
const FName OverlayTargetIvHpVariableName(TEXT("ESP_TargetIvHp"));
const FName OverlayTargetIvAttackVariableName(TEXT("ESP_TargetIvAttack"));
const FName OverlayTargetIvDefenseVariableName(TEXT("ESP_TargetIvDefense"));
const FName OverlayTargetNamesVariableName(TEXT("ESP_TargetNames"));
const FName OverlayTargetPassiveTextsVariableName(TEXT("ESP_TargetPassiveTexts"));
const FName OverlayTargetPassiveIdTextsVariableName(TEXT("ESP_TargetPassiveIdTexts"));
const FName OverlayPassiveBuildTextVariableName(TEXT("ESP_PassiveBuildText"));
const FName OverlayPassiveIdBuildTextVariableName(TEXT("ESP_PassiveIdBuildText"));
const FName OverlayCaptureCountBuildVariableName(TEXT("ESP_CaptureCountBuild"));
const FName OverlayPassiveFilterIdsVariableName(TEXT("ESP_PassiveFilterIds"));
const FName OverlayPassiveFilterMatchVariableName(TEXT("ESP_PassiveFilterMatch"));
const FName OverlayPassiveExcludeIdsVariableName(TEXT("ESP_PassiveExcludeIds"));
const FName OverlaySpeciesFilterIdsVariableName(TEXT("ESP_SpeciesFilterIds"));
const FName OverlayPassiveExcludeMatchVariableName(TEXT("ESP_PassiveExcludeMatch"));
const FName OverlayGenderLoggedVariableName(TEXT("ESP_GenderLogged"));
const FName OverlayGenderDiagnosticVariableName(TEXT("ESP_GenderDiagnostic"));
const FName OverlayGenderDiagnosticCodeVariableName(TEXT("ESP_GenderDiagnosticCode"));
const FName OverlayTopGuideEnabledVariableName(TEXT("ESP_ShowTopGuideLine"));
const FName OverlayLanguageIdVariableName(TEXT("ESP_LanguageId"));
const FName OverlayShowNameVariableName(TEXT("ESP_ShowName"));
const FName OverlayShowLevelVariableName(TEXT("ESP_ShowLevel"));
const FName OverlayShowDistanceVariableName(TEXT("ESP_ShowDistance"));
const FName OverlayShowIvVariableName(TEXT("ESP_ShowIV"));
const FName OverlayShowPassiveSkillsVariableName(TEXT("ESP_ShowPassiveSkills"));
const FName OverlayIvMinVariableName(TEXT("ESP_IvMin"));
const FName OverlayIvHpMinVariableName(TEXT("ESP_IvHpMin"));
const FName OverlayIvAttackMinVariableName(TEXT("ESP_IvAttackMin"));
const FName OverlayIvDefenseMinVariableName(TEXT("ESP_IvDefenseMin"));
const FName OverlayGenderFilterIdVariableName(TEXT("ESP_GenderFilterId"));
const FName OverlayLuckyFilterIdVariableName(TEXT("ESP_LuckyFilterId"));
const FName OverlayBossFilterIdVariableName(TEXT("ESP_BossFilterId"));
const FName OverlayCollectionFilterIdVariableName(TEXT("ESP_CollectionFilterId"));
const FName OverlayElementFilterMaskVariableName(TEXT("ESP_ElementFilterMask"));
const FName OverlayAddTargetEventName(TEXT("PalworldResourceESP_WidgetAddTarget"));
const FName OverlayRemoveTargetEventName(TEXT("PalworldResourceESP_WidgetRemoveTarget"));
const FName OverlayClearTargetsEventName(TEXT("PalworldResourceESP_WidgetClearTargets"));
const FName PanelBridgeVariableName(TEXT("ESP_Bridge"));
const FName PanelVariableName(TEXT("ESP_PanelWidget"));
const FName ControlRevisionVariableName(TEXT("ESP_ControlRevision"));
const FName RuntimeEnabledVariableName(TEXT("ESP_RuntimeEnabled"));
const FName ProfileIdVariableName(TEXT("ESP_ProfileId"));
const FName PresetIdVariableName(TEXT("ESP_PresetId"));
const FName CaptureRequestedVariableName(TEXT("ESP_CaptureRequested"));
const FName LanguageIdVariableName(TEXT("ESP_LanguageId"));
const FName LevelMinVariableName(TEXT("ESP_LevelMin"));
const FName LevelMaxVariableName(TEXT("ESP_LevelMax"));
// __DEPRECATED_20260717__ [reason: retained on ModActor only so older assets remain reversible]
const FName DistanceMinVariableName(TEXT("ESP_DistanceMin"));
const FName DistanceMaxVariableName(TEXT("ESP_DistanceMax"));
const FName ShowTopGuideLineVariableName(TEXT("ESP_ShowTopGuideLine"));
const FName ShowNameVariableName(TEXT("ESP_ShowName"));
const FName ShowLevelVariableName(TEXT("ESP_ShowLevel"));
const FName ShowDistanceVariableName(TEXT("ESP_ShowDistance"));
const FName ShowIvVariableName(TEXT("ESP_ShowIV"));
const FName ShowPassiveSkillsVariableName(TEXT("ESP_ShowPassiveSkills"));
const FName IvMinVariableName(TEXT("ESP_IvMin"));
const FName IvHpMinVariableName(TEXT("ESP_IvHpMin"));
const FName IvAttackMinVariableName(TEXT("ESP_IvAttackMin"));
const FName IvDefenseMinVariableName(TEXT("ESP_IvDefenseMin"));
const FName PassiveFilterIdsVariableName(TEXT("ESP_PassiveFilterIds"));
const FName PassiveExcludeIdsVariableName(TEXT("ESP_PassiveExcludeIds"));
const FName PassiveIncludeTextVariableName(TEXT("ESP_PassiveIncludeText"));
const FName PassiveExcludeTextVariableName(TEXT("ESP_PassiveExcludeText"));
const FName PassiveFilterRevisionVariableName(TEXT("ESP_PassiveFilterRevision"));
const FName SpeciesFilterIdsVariableName(TEXT("ESP_SpeciesFilterIds"));
const FName SpeciesFilterTextVariableName(TEXT("ESP_SpeciesFilterText"));
const FName PanelMainPageVariableName(TEXT("ESP_PanelMainPage"));
const FName PanelFilterPageVariableName(TEXT("ESP_PanelFilterPage"));
const FName PassiveRainbowExpandedVariableName(TEXT("ESP_PassiveRainbowExpanded"));
const FName PassiveLegendExpandedVariableName(TEXT("ESP_PassiveLegendExpanded"));
const FName PassiveGold3ExpandedVariableName(TEXT("ESP_PassiveGold3Expanded"));
const FName PassiveGold2ExpandedVariableName(TEXT("ESP_PassiveGold2Expanded"));
const FName PassiveNormalExpandedVariableName(TEXT("ESP_PassiveNormalExpanded"));
const FName PassiveNegative1ExpandedVariableName(TEXT("ESP_PassiveNegative1Expanded"));
const FName PassiveNegative2ExpandedVariableName(TEXT("ESP_PassiveNegative2Expanded"));
const FName PassiveNegative3ExpandedVariableName(TEXT("ESP_PassiveNegative3Expanded"));
const FName GenderFilterIdVariableName(TEXT("ESP_GenderFilterId"));
const FName LuckyFilterIdVariableName(TEXT("ESP_LuckyFilterId"));
const FName BossFilterIdVariableName(TEXT("ESP_BossFilterId"));
const FName CollectionFilterIdVariableName(TEXT("ESP_CollectionFilterId"));
const FName ElementNormalVariableName(TEXT("ESP_ElementNormal"));
const FName ElementFireVariableName(TEXT("ESP_ElementFire"));
const FName ElementWaterVariableName(TEXT("ESP_ElementWater"));
const FName ElementLeafVariableName(TEXT("ESP_ElementLeaf"));
const FName ElementElectricityVariableName(TEXT("ESP_ElementElectricity"));
const FName ElementIceVariableName(TEXT("ESP_ElementIce"));
const FName ElementEarthVariableName(TEXT("ESP_ElementEarth"));
const FName ElementDarkVariableName(TEXT("ESP_ElementDark"));
const FName ElementDragonVariableName(TEXT("ESP_ElementDragon"));
const FName DisplayTargetLimitVariableName(TEXT("ESP_DisplayTargetLimit"));
const FName PanelEscapeClosePendingVariableName(TEXT("ESP_EscapeClosePending"));
const FName PanelInitializeControlsEventName(TEXT("PalworldResourceESP_InitializeControls"));
const FName PanelInitializeControlsV2EventName(TEXT("PalworldResourceESP_InitializeControlsV2"));
const FName PanelSetPageStateEventName(TEXT("PalworldResourceESP_SetPageState"));
const FName PanelInitializeLanguageEventName(TEXT("PalworldResourceESP_InitializeLanguage"));
const FName PanelPopulatePassiveCatalogEventName(TEXT("PalworldResourceESP_PopulatePassiveCatalog"));
const FName PanelPopulatePalCatalogEventName(TEXT("PalworldResourceESP_PopulatePalCatalog"));
const FName PanelRenderPalCatalogEventName(TEXT("PalworldResourceESP_RenderPalCatalog"));
const FName ApplyPersistedPanelStateEventName(TEXT("PalworldResourceESP_ApplyPersistedPanelState"));
const FName PanelToggleEventName(TEXT("PalworldResourceESP_TogglePanel"));
const FName PassiveTooltipInitializeEventName(TEXT("PalworldResourceESP_InitializePassiveTooltip"));
const FName PassiveEntryInitializeEventName(TEXT("PalworldResourceESP_InitializePassiveEntry"));
const FName PassiveEntryBridgeVariableName(TEXT("ESP_Bridge"));
const FName PassiveEntrySkillIdVariableName(TEXT("ESP_SkillId"));
const FName PassiveEntrySkillNameVariableName(TEXT("ESP_SkillName"));
const FName PassiveEntrySelectedVariableName(TEXT("ESP_Selected"));
const FName PassiveEntryExcludedVariableName(TEXT("ESP_Excluded"));
const FName PalEntryInitializeEventName(TEXT("PalworldResourceESP_InitializePalEntry"));
const FName PalEntryBridgeVariableName(TEXT("ESP_Bridge"));
const FName PalEntryCharacterIdVariableName(TEXT("ESP_CharacterId"));
const FName PalEntryDisplayNameVariableName(TEXT("ESP_DisplayName"));
const FName PalEntrySelectionSummaryVariableName(TEXT("ESP_SelectionSummary"));
const FName PalEntrySelectedVariableName(TEXT("ESP_Selected"));
const FName PalEntryInitializingVariableName(TEXT("ESP_Initializing"));
const FName PalCatalogLoadedVariableName(TEXT("ESP_PalCatalogLoaded"));
const FName PalCatalogIdsVariableName(TEXT("ESP_PalCatalogIds"));
const FName PalCatalogZukanIndicesVariableName(TEXT("ESP_PalCatalogZukanIndices"));
const FName PalCatalogTableVariableName(TEXT("ESP_PalCatalogTable"));
const FName PalCatalogElementQueryVariableName(TEXT("ESP_PalCatalogElementQuery"));
const FName PalCatalogWorkQueryVariableName(TEXT("ESP_PalCatalogWorkQuery"));
const FName PalCatalogResultCountVariableName(TEXT("ESP_PalCatalogResultCount"));
const FName PalCatalogSuppressEventsVariableName(TEXT("ESP_PalCatalogSuppressEvents"));
const FName PalCatalogInsertIndexVariableName(TEXT("ESP_PalCatalogInsertIndex"));
const FName PalCatalogInsertFoundVariableName(TEXT("ESP_PalCatalogInsertFound"));
const FName PalCatalogSelectedNamesVariableName(TEXT("ESP_PalCatalogSelectedNames"));
const FName PalCatalogSelectedCountVariableName(TEXT("ESP_PalCatalogSelectedCount"));
const FName PalCatalogSortKeysVariableName(TEXT("ESP_PalCatalogSortKeys"));
const FName PalCatalogSortModeVariableName(TEXT("ESP_PalCatalogSortMode"));
const FName PalCatalogSortDescendingVariableName(TEXT("ESP_PalCatalogSortDescending"));
const FName RichTextAuditBufferVariableName(TEXT("ESP_RichTextAuditBuffer"));
const TCHAR* PassiveRichTextStylePath = TEXT("/Game/Mods/PalworldResourceESP/DT_ESPRichTextStyle.DT_ESPRichTextStyle");
const TCHAR* GameRichTextStylePath = TEXT("/Game/Pal/DataTable/Text/RchTextData/DT_PalRichTextStyle.DT_PalRichTextStyle");
const TCHAR* PalMonsterParameterTablePath = TEXT("/Game/Pal/DataTable/Character/DT_PalMonsterParameter.DT_PalMonsterParameter");
const TCHAR* PanelInputDisableFlag = TEXT("PalworldResourceESP_Panel");
constexpr bool EnablePartnerRichTextAudit = false;

struct FPalWorkDefinition {
    FName EnumName;
    FName FieldName;
    FString Chinese;
    FString English;
};

const TArray<FPalWorkDefinition>& GetPalWorkDefinitions() {
    static const TArray<FPalWorkDefinition> Definitions = {
        {TEXT("EmitFlame"), TEXT("WorkSuitability_EmitFlame"), TEXT("生火"), TEXT("Kindling")},
        {TEXT("Watering"), TEXT("WorkSuitability_Watering"), TEXT("浇水"), TEXT("Watering")},
        {TEXT("Seeding"), TEXT("WorkSuitability_Seeding"), TEXT("播种"), TEXT("Planting")},
        {TEXT("GenerateElectricity"), TEXT("WorkSuitability_GenerateElectricity"), TEXT("发电"), TEXT("Electricity")},
        {TEXT("Handcraft"), TEXT("WorkSuitability_Handcraft"), TEXT("手工作业"), TEXT("Handiwork")},
        {TEXT("Collection"), TEXT("WorkSuitability_Collection"), TEXT("采集"), TEXT("Gathering")},
        {TEXT("Deforest"), TEXT("WorkSuitability_Deforest"), TEXT("伐木"), TEXT("Lumbering")},
        {TEXT("Mining"), TEXT("WorkSuitability_Mining"), TEXT("采矿"), TEXT("Mining")},
        // __DEPRECATED_20260719__ [reason: OilExtraction is an internal SDK field, not a player-visible work suitability]
        // {TEXT("OilExtraction"), TEXT("WorkSuitability_OilExtraction"), TEXT("采油"), TEXT("Oil extraction")},
        {TEXT("ProductMedicine"), TEXT("WorkSuitability_ProductMedicine"), TEXT("制药"), TEXT("Medicine")},
        {TEXT("Cool"), TEXT("WorkSuitability_Cool"), TEXT("冷却"), TEXT("Cooling")},
        {TEXT("Transport"), TEXT("WorkSuitability_Transport"), TEXT("搬运"), TEXT("Transporting")},
        {TEXT("MonsterFarm"), TEXT("WorkSuitability_MonsterFarm"), TEXT("牧场"), TEXT("Ranching")},
    };
    return Definitions;
}

struct FPassiveDescriptionFallback {
    const TCHAR* SkillId;
    const TCHAR* Chinese;
    const TCHAR* English;
};

const TArray<FPassiveDescriptionFallback> PassiveDescriptionFallbacks = {
    {TEXT("Deffence_down2"), TEXT("防御 -20%"), TEXT("Defense -20%")},
    {TEXT("CraftSpeed_down2"), TEXT("工作速度 -30%"), TEXT("Work Speed -30%")},
    {TEXT("PAL_ALLAttack_down2"), TEXT("攻击 -20%"), TEXT("Attack -20%")},
    {TEXT("CraftSpeed_down1"), TEXT("工作速度 -10%"), TEXT("Work Speed -10%")},
    {TEXT("PAL_ALLAttack_down1"), TEXT("攻击 -10%"), TEXT("Attack -10%")},
    {TEXT("Deffence_down1"), TEXT("防御 -10%"), TEXT("Defense -10%")},
    {TEXT("PAL_CorporateSlave"), TEXT("工作速度 +30%，攻击 -30%"), TEXT("Work Speed +30%, Attack -30%")},
    {TEXT("PAL_masochist"), TEXT("防御 +15%，攻击 -15%"), TEXT("Defense +15%, Attack -15%")},
    {TEXT("PAL_ALLAttack_up2"), TEXT("攻击 +20%"), TEXT("Attack +20%")},
    {TEXT("PAL_ALLAttack_up1"), TEXT("攻击 +10%"), TEXT("Attack +10%")},
    {TEXT("PAL_conceited"), TEXT("工作速度 +10%，防御 -10%"), TEXT("Work Speed +10%, Defense -10%")},
    {TEXT("PAL_sadist"), TEXT("攻击 +15%，防御 -15%"), TEXT("Attack +15%, Defense -15%")},
    {TEXT("PAL_oraora"), TEXT("攻击 +10%，防御 -10%"), TEXT("Attack +10%, Defense -10%")},
    {TEXT("CraftSpeed_up1"), TEXT("工作速度 +20%"), TEXT("Work Speed +20%")},
    {TEXT("Deffence_up1"), TEXT("防御 +10%"), TEXT("Defense +10%")},
    {TEXT("CraftSpeed_up2"), TEXT("工作速度 +50%"), TEXT("Work Speed +50%")},
    {TEXT("PAL_rude"), TEXT("攻击 +15%，工作速度 -10%"), TEXT("Attack +15%, Work Speed -10%")},
    {TEXT("CraftSpeed_up3"), TEXT("工作速度 +75%"), TEXT("Work Speed +75%")},
    {TEXT("PAL_ALLAttack_up3"), TEXT("攻击 +30%，防御 +5%"), TEXT("Attack +30%, Defense +5%")},
    {TEXT("Noukin"), TEXT("攻击 +30%，工作速度 -50%"), TEXT("Attack +30%, Work Speed -50%")},
    {TEXT("Legend"), TEXT("攻击 +20%，防御 +20%，移动速度 +20%"), TEXT("Attack +20%, Defense +20%, Movement Speed +20%")},
    {TEXT("Rare"), TEXT("攻击 +15%，工作速度 +15%"), TEXT("Attack +15%, Work Speed +15%")},
    {TEXT("MutationPal_ExplosionResist"), TEXT("免疫爆破伤害"), TEXT("Immune to explosion damage")},
    {TEXT("WorkSuitabilityAddRank_MonsterFarm_2"), TEXT("牧场工作适应性 +2"), TEXT("Farming work suitability +2")},
    {TEXT("WorkSuitabilityAddRank_MonsterFarm_1"), TEXT("牧场工作适应性 +1"), TEXT("Farming work suitability +1")},
    {TEXT("Vampire"), TEXT("吸收造成伤害的一部分恢复自身 HP；夜晚也不会睡眠，可持续工作"), TEXT("Restores HP from part of damage dealt; remains awake and working at night")},
    {TEXT("MutationPal_Mutant"), TEXT("帕鲁和玩家生命值自然恢复量 +50%，防御 +25%，免疫中毒与灼烧伤害"), TEXT("Pal and player auto HP recovery +50%, Defense +25%, immune to poison and burn damage")},
};

UDataTable* EnsurePassiveRichTextStyleTable(const FSlateFontInfo& SourceFont) {
    UDataTable* Table = LoadObject<UDataTable>(nullptr, PassiveRichTextStylePath);
    if (!Table) {
        UPackage* Package = CreatePackage(TEXT("/Game/Mods/PalworldResourceESP/DT_ESPRichTextStyle"));
        if (!Package) {
            return nullptr;
        }
        Table = NewObject<UDataTable>(
            Package,
            TEXT("DT_ESPRichTextStyle"),
            RF_Public | RF_Standalone);
        if (!Table) {
            return nullptr;
        }
        FAssetRegistryModule::AssetCreated(Table);
    }

    Table->RowStruct = FRichTextStyleRow::StaticStruct();
    Table->EmptyTable();
    auto AddStyle = [&](const FName& Name, const FLinearColor& Color) {
        FRichTextStyleRow Row;
        FSlateFontInfo Font = SourceFont;
        Font.Size = 13;
        Row.TextStyle = FTextBlockStyle::GetDefault();
        Row.TextStyle
            .SetFont(Font)
            .SetColorAndOpacity(FSlateColor(Color))
            .SetShadowOffset(FVector2D(1.0f, 1.0f))
            .SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
        Table->AddRow(Name, Row);
    };
    AddStyle(TEXT("Default"), FLinearColor(0.95f, 0.955f, 0.96f, 1.0f));
    AddStyle(TEXT("NumRed_13"), FLinearColor(1.0f, 0.36f, 0.36f, 1.0f));
    AddStyle(TEXT("NumBlue_13"), FLinearColor(0.36f, 0.72f, 1.0f, 1.0f));
    AddStyle(TEXT("NumGreen_13"), FLinearColor(0.38f, 0.92f, 0.58f, 1.0f));
    AddStyle(TEXT("NumYellow_13"), FLinearColor(1.0f, 0.82f, 0.30f, 1.0f));
    AddStyle(TEXT("Status_Up"), FLinearColor(0.38f, 0.92f, 0.58f, 1.0f));
    AddStyle(TEXT("Status_Down"), FLinearColor(1.0f, 0.38f, 0.38f, 1.0f));
    AddStyle(TEXT("Status_Keyword"), FLinearColor(1.0f, 0.82f, 0.30f, 1.0f));
    Table->MarkPackageDirty();
    return Table;
}

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

FEdGraphPinType IntArrayPin() {
    FEdGraphPinType PinType = IntPin();
    PinType.ContainerType = EPinContainerType::Array;
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

FEdGraphPinType StringArrayPin() {
    FEdGraphPinType PinType = StringPin();
    PinType.ContainerType = EPinContainerType::Array;
    return PinType;
}

FEdGraphPinType NamePin() {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
    return PinType;
}

FEdGraphPinType NameArrayPin() {
    FEdGraphPinType PinType = NamePin();
    PinType.ContainerType = EPinContainerType::Array;
    return PinType;
}

FEdGraphPinType TextPin() {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
    return PinType;
}

FEdGraphPinType StructPin(UScriptStruct* Struct) {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
    PinType.PinSubCategoryObject = Struct;
    return PinType;
}

FEdGraphPinType SoftObjectPin(UClass* Class) {
    FEdGraphPinType PinType;
    PinType.PinCategory = UEdGraphSchema_K2::PC_SoftObject;
    PinType.PinSubCategoryObject = Class;
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

bool PrepareOverrideFunctionGraph(
    UWidgetBlueprint* Blueprint,
    UClass* OwnerClass,
    const FName& FunctionName,
    UEdGraph*& OutGraph,
    UK2Node_FunctionEntry*& OutEntry,
    UK2Node_FunctionResult*& OutResult) {
    OutGraph = nullptr;
    OutEntry = nullptr;
    OutResult = nullptr;
    if (!Blueprint || !OwnerClass || !OwnerClass->FindFunctionByName(FunctionName)) {
        return false;
    }

    for (UEdGraph* ExistingGraph : Blueprint->FunctionGraphs) {
        if (ExistingGraph && ExistingGraph->GetFName() == FunctionName) {
            OutGraph = ExistingGraph;
            break;
        }
    }
    if (!OutGraph) {
        OutGraph = FBlueprintEditorUtils::CreateNewGraph(
            Blueprint,
            FunctionName,
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());
        if (!OutGraph) {
            return false;
        }
        FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, OutGraph, false, OwnerClass);
    } else {
        ClearGraph(OutGraph);
        OutGraph->GetSchema()->CreateDefaultNodesForGraph(*OutGraph);
        const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(OutGraph->GetSchema());
        if (!K2Schema) {
            return false;
        }
        K2Schema->CreateFunctionGraphTerminators(*OutGraph, OwnerClass);
    }

    TArray<UK2Node_FunctionEntry*> Entries;
    TArray<UK2Node_FunctionResult*> Results;
    OutGraph->GetNodesOfClass(Entries);
    OutGraph->GetNodesOfClass(Results);
    if (Entries.Num() != 1 || Results.Num() != 1) {
        return false;
    }
    OutEntry = Entries[0];
    OutResult = Results[0];
    OutEntry->NodePosX = -1600;
    OutEntry->NodePosY = 1120;
    OutResult->NodePosX = 3320;
    OutResult->NodePosY = 1120;
    if (UEdGraphPin* EntryExec = OutEntry->FindPin(UEdGraphSchema_K2::PN_Then)) {
        EntryExec->BreakAllPinLinks();
    }
    return true;
}

UK2Node_FunctionResult* AddFunctionResultNode(
    UEdGraph* Graph,
    UK2Node_FunctionEntry* Entry,
    int32 X,
    int32 Y) {
    if (!Graph || !Entry) {
        return nullptr;
    }
    UK2Node_FunctionResult* Result = NewObject<UK2Node_FunctionResult>(Graph);
    if (!Result) {
        return nullptr;
    }
    Result->FunctionReference = Entry->FunctionReference;
    Result->NodePosX = X;
    Result->NodePosY = Y;
    Graph->AddNode(Result, true, false);
    Result->CreateNewGuid();
    Result->AllocateDefaultPins();
    return Result;
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

UK2Node_GetDataTableRow* AddGetDataTableRow(
    UEdGraph* Graph,
    UScriptStruct* RowStruct,
    int32 X,
    int32 Y) {
    if (!Graph || !RowStruct) {
        return nullptr;
    }
    UK2Node_GetDataTableRow* Node = NewObject<UK2Node_GetDataTableRow>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
    UEdGraphPin* ResultPin = Node->GetResultPin();
    if (!ResultPin) {
        return nullptr;
    }
    ResultPin->PinType = StructPin(RowStruct);
    return Node;
}

UK2Node_BreakStruct* AddBreakStruct(
    UEdGraph* Graph,
    UScriptStruct* Struct,
    int32 X,
    int32 Y) {
    if (!Graph || !Struct) {
        return nullptr;
    }
    UK2Node_BreakStruct* Node = NewObject<UK2Node_BreakStruct>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->StructType = Struct;
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    Node->CreateNewGuid();
    Node->AllocateDefaultPins();
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

UK2Node_ExecutionSequence* AddSequence(UEdGraph* Graph, int32 X, int32 Y) {
    if (!Graph) {
        return nullptr;
    }
    UK2Node_ExecutionSequence* Node = NewObject<UK2Node_ExecutionSequence>(Graph);
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

UK2Node_DynamicCast* AddPureDynamicCast(UEdGraph* Graph, UClass* TargetClass, int32 X, int32 Y) {
    UK2Node_DynamicCast* Node = AddDynamicCast(Graph, TargetClass, X, Y);
    if (Node) {
        Node->SetPurity(true);
        Node->ReconstructNode();
    }
    return Node;
}

UK2Node_ClassDynamicCast* AddPureClassDynamicCast(
    UEdGraph* Graph,
    UClass* TargetClass,
    int32 X,
    int32 Y) {
    if (!Graph || !TargetClass) {
        return nullptr;
    }
    UK2Node_ClassDynamicCast* Node = NewObject<UK2Node_ClassDynamicCast>(Graph);
    if (!Node) {
        return nullptr;
    }
    Node->TargetType = TargetClass;
    Node->SetPurity(true);
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

UK2Node_ComponentBoundEvent* AddSpinBoxValueChangedEvent(
    UWidgetBlueprint* Blueprint,
    USpinBox* SpinBox,
    int32 X,
    int32 Y) {
    if (!Blueprint || !SpinBox || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        SpinBox->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        USpinBox::StaticClass(),
        GET_MEMBER_NAME_CHECKED(USpinBox, OnValueChanged)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddSpinBoxEvent missing graph/property spinbox=%s"), *GetNameSafe(SpinBox));
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

UK2Node_ComponentBoundEvent* AddSpinBoxValueCommittedEvent(
    UWidgetBlueprint* Blueprint,
    USpinBox* SpinBox,
    int32 X,
    int32 Y) {
    if (!Blueprint || !SpinBox || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        SpinBox->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        USpinBox::StaticClass(),
        GET_MEMBER_NAME_CHECKED(USpinBox, OnValueCommitted)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
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

UK2Node_ComponentBoundEvent* AddSliderValueChangedEvent(
    UWidgetBlueprint* Blueprint,
    USlider* Slider,
    int32 X,
    int32 Y) {
    if (!Blueprint || !Slider || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        Slider->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        USlider::StaticClass(),
        GET_MEMBER_NAME_CHECKED(USlider, OnValueChanged)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddSliderEvent missing graph/property slider=%s"), *GetNameSafe(Slider));
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

UK2Node_ComponentBoundEvent* AddSliderCaptureEndEvent(
    UWidgetBlueprint* Blueprint,
    USlider* Slider,
    const FName& DelegateName,
    int32 X,
    int32 Y) {
    if (!Blueprint || !Slider || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        Slider->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        USlider::StaticClass(),
        DelegateName
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
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

UK2Node_ComponentBoundEvent* AddCheckBoxStateChangedEvent(
    UWidgetBlueprint* Blueprint,
    UCheckBox* CheckBox,
    int32 X,
    int32 Y) {
    if (!Blueprint || !CheckBox || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        CheckBox->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        UCheckBox::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UCheckBox, OnCheckStateChanged)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddCheckBoxEvent missing graph/property checkbox=%s"), *GetNameSafe(CheckBox));
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

UK2Node_ComponentBoundEvent* AddExpandableAreaExpansionChangedEvent(
    UWidgetBlueprint* Blueprint,
    UExpandableArea* Area,
    int32 X,
    int32 Y) {
    if (!Blueprint || !Area || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        Area->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        UExpandableArea::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UExpandableArea, OnExpansionChanged)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] AddExpansionEvent missing graph/property area=%s"), *GetNameSafe(Area));
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

UK2Node_ComponentBoundEvent* AddEditableTextBoxCommittedEvent(
    UWidgetBlueprint* Blueprint,
    UEditableTextBox* TextBox,
    int32 X,
    int32 Y) {
    if (!Blueprint || !TextBox || !Blueprint->SkeletonGeneratedClass) {
        return nullptr;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    FObjectProperty* ComponentProperty = FindFProperty<FObjectProperty>(
        Blueprint->SkeletonGeneratedClass,
        TextBox->GetFName()
    );
    FMulticastDelegateProperty* DelegateProperty = FindFProperty<FMulticastDelegateProperty>(
        UEditableTextBox::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UEditableTextBox, OnTextCommitted)
    );
    if (!Graph || !ComponentProperty || !DelegateProperty) {
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
    UScriptStruct* IndividualSaveParameterStruct,
    UEnum* GenderEnum,
    UEnum* ElementEnum,
    UClass* PalUtilityClass,
    UClass* PalUIUtilityClass,
    UClass* DatabaseCharacterParameterClass,
    UClass* PlayerRecordDataClass,
    UClass* PlayerRecordDataUtilityClass) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildOverlay dynamic begin blueprint=%s"), *GetNameSafe(Blueprint));
    if (!Blueprint || !Blueprint->WidgetTree || !PalMonsterClass || !CharacterParameterComponentClass
        || !IndividualParameterClass || !IndividualSaveParameterStruct || !GenderEnum || !ElementEnum
        || !PalUtilityClass || !PalUIUtilityClass || !DatabaseCharacterParameterClass
        || !PlayerRecordDataClass || !PlayerRecordDataUtilityClass) {
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

    const int32 ExistingTargetParametersIndex = FBlueprintEditorUtils::FindNewVariableIndex(
        Blueprint, OverlayTargetParametersVariableName);
    if (ExistingTargetParametersIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(
                Blueprint, OverlayTargetParametersVariableName, ObjectArrayPin(IndividualParameterClass))) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetParametersIndex].VarType = ObjectArrayPin(IndividualParameterClass);
    }

    const int32 ExistingTargetCharacterIdsIndex = FBlueprintEditorUtils::FindNewVariableIndex(
        Blueprint, OverlayTargetCharacterIdsVariableName);
    if (ExistingTargetCharacterIdsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(
                Blueprint, OverlayTargetCharacterIdsVariableName, NameArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetCharacterIdsIndex].VarType = NameArrayPin();
    }

    const int32 ExistingTargetLevelsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetLevelsVariableName);
    if (ExistingTargetLevelsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetLevelsVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetLevelsIndex].VarType = IntArrayPin();
    }

    const int32 ExistingTargetDistancesIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetDistancesVariableName);
    if (ExistingTargetDistancesIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetDistancesVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetDistancesIndex].VarType = IntArrayPin();
    }

    const int32 ExistingTargetGendersIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetGendersVariableName);
    if (ExistingTargetGendersIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetGendersVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetGendersIndex].VarType = IntArrayPin();
    }

    const int32 ExistingTargetLuckyStatesIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetLuckyStatesVariableName);
    if (ExistingTargetLuckyStatesIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetLuckyStatesVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetLuckyStatesIndex].VarType = IntArrayPin();
    }

    const int32 ExistingTargetBossStatesIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetBossStatesVariableName);
    if (ExistingTargetBossStatesIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetBossStatesVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetBossStatesIndex].VarType = IntArrayPin();
    }

    const int32 ExistingTargetCaptureCountsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetCaptureCountsVariableName);
    if (ExistingTargetCaptureCountsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetCaptureCountsVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetCaptureCountsIndex].VarType = IntArrayPin();
    }

    const int32 ExistingTargetElementMasksIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetElementMasksVariableName);
    if (ExistingTargetElementMasksIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetElementMasksVariableName, IntArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetElementMasksIndex].VarType = IntArrayPin();
    }

    const TArray<FName> IvArrayNames = {
        OverlayTargetIvHpVariableName,
        OverlayTargetIvAttackVariableName,
        OverlayTargetIvDefenseVariableName,
    };
    for (const FName& IvArrayName : IvArrayNames) {
        const int32 ExistingIvArrayIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, IvArrayName);
        if (ExistingIvArrayIndex == INDEX_NONE) {
            if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, IvArrayName, IntArrayPin())) {
                return false;
            }
        } else {
            Blueprint->NewVariables[ExistingIvArrayIndex].VarType = IntArrayPin();
        }
    }

    const int32 ExistingTargetNamesIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetNamesVariableName);
    if (ExistingTargetNamesIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetNamesVariableName, StringArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetNamesIndex].VarType = StringArrayPin();
    }

    const int32 ExistingTargetPassiveTextsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetPassiveTextsVariableName);
    if (ExistingTargetPassiveTextsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetPassiveTextsVariableName, StringArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetPassiveTextsIndex].VarType = StringArrayPin();
    }

    const int32 ExistingTargetPassiveIdTextsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTargetPassiveIdTextsVariableName);
    if (ExistingTargetPassiveIdTextsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTargetPassiveIdTextsVariableName, StringArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingTargetPassiveIdTextsIndex].VarType = StringArrayPin();
    }

    const int32 ExistingPassiveBuildTextIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveBuildTextVariableName);
    if (ExistingPassiveBuildTextIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayPassiveBuildTextVariableName, StringPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingPassiveBuildTextIndex].VarType = StringPin();
    }

    const int32 ExistingPassiveIdBuildTextIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveIdBuildTextVariableName);
    if (ExistingPassiveIdBuildTextIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayPassiveIdBuildTextVariableName, StringPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingPassiveIdBuildTextIndex].VarType = StringPin();
    }

    int32 ExistingCaptureCountBuildIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayCaptureCountBuildVariableName);
    if (ExistingCaptureCountBuildIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayCaptureCountBuildVariableName, IntPin())) {
            return false;
        }
        ExistingCaptureCountBuildIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayCaptureCountBuildVariableName);
    }
    if (ExistingCaptureCountBuildIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingCaptureCountBuildIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingCaptureCountBuildIndex].DefaultValue = TEXT("-1");

    const int32 ExistingPassiveFilterIdsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveFilterIdsVariableName);
    if (ExistingPassiveFilterIdsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayPassiveFilterIdsVariableName, NameArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingPassiveFilterIdsIndex].VarType = NameArrayPin();
    }

    const int32 ExistingPassiveExcludeIdsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveExcludeIdsVariableName);
    if (ExistingPassiveExcludeIdsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayPassiveExcludeIdsVariableName, NameArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingPassiveExcludeIdsIndex].VarType = NameArrayPin();
    }

    const int32 ExistingSpeciesFilterIdsIndex = FBlueprintEditorUtils::FindNewVariableIndex(
        Blueprint, OverlaySpeciesFilterIdsVariableName);
    if (ExistingSpeciesFilterIdsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(
                Blueprint, OverlaySpeciesFilterIdsVariableName, NameArrayPin())) {
            return false;
        }
    } else {
        Blueprint->NewVariables[ExistingSpeciesFilterIdsIndex].VarType = NameArrayPin();
    }

    int32 ExistingPassiveFilterMatchIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveFilterMatchVariableName);
    if (ExistingPassiveFilterMatchIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayPassiveFilterMatchVariableName, BoolPin())) {
            return false;
        }
        ExistingPassiveFilterMatchIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveFilterMatchVariableName);
    }
    if (ExistingPassiveFilterMatchIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingPassiveFilterMatchIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingPassiveFilterMatchIndex].DefaultValue = TEXT("true");

    int32 ExistingPassiveExcludeMatchIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveExcludeMatchVariableName);
    if (ExistingPassiveExcludeMatchIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayPassiveExcludeMatchVariableName, BoolPin())) {
            return false;
        }
        ExistingPassiveExcludeMatchIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayPassiveExcludeMatchVariableName);
    }
    if (ExistingPassiveExcludeMatchIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingPassiveExcludeMatchIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingPassiveExcludeMatchIndex].DefaultValue = TEXT("true");

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

    int32 ExistingTopGuideEnabledIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTopGuideEnabledVariableName);
    if (ExistingTopGuideEnabledIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayTopGuideEnabledVariableName, BoolPin())) {
            return false;
        }
        ExistingTopGuideEnabledIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayTopGuideEnabledVariableName);
    }
    if (ExistingTopGuideEnabledIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingTopGuideEnabledIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingTopGuideEnabledIndex].DefaultValue = TEXT("true");

    int32 ExistingLanguageIdIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayLanguageIdVariableName);
    if (ExistingLanguageIdIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayLanguageIdVariableName, IntPin())) {
            return false;
        }
        ExistingLanguageIdIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayLanguageIdVariableName);
    }
    if (ExistingLanguageIdIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingLanguageIdIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingLanguageIdIndex].DefaultValue = TEXT("0");

    int32 ExistingShowNameIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowNameVariableName);
    if (ExistingShowNameIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayShowNameVariableName, BoolPin())) {
            return false;
        }
        ExistingShowNameIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowNameVariableName);
    }
    if (ExistingShowNameIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingShowNameIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingShowNameIndex].DefaultValue = TEXT("true");

    int32 ExistingShowLevelIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowLevelVariableName);
    if (ExistingShowLevelIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayShowLevelVariableName, BoolPin())) {
            return false;
        }
        ExistingShowLevelIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowLevelVariableName);
    }
    int32 ExistingShowDistanceIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowDistanceVariableName);
    if (ExistingShowDistanceIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayShowDistanceVariableName, BoolPin())) {
            return false;
        }
        ExistingShowDistanceIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowDistanceVariableName);
    }
    if (ExistingShowLevelIndex == INDEX_NONE || ExistingShowDistanceIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingShowLevelIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingShowLevelIndex].DefaultValue = TEXT("true");
    Blueprint->NewVariables[ExistingShowDistanceIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingShowDistanceIndex].DefaultValue = TEXT("true");

    int32 ExistingShowIvIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowIvVariableName);
    if (ExistingShowIvIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayShowIvVariableName, BoolPin())) {
            return false;
        }
        ExistingShowIvIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowIvVariableName);
    }
    if (ExistingShowIvIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingShowIvIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingShowIvIndex].DefaultValue = TEXT("false");

    int32 ExistingShowPassiveSkillsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowPassiveSkillsVariableName);
    if (ExistingShowPassiveSkillsIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayShowPassiveSkillsVariableName, BoolPin())) {
            return false;
        }
        ExistingShowPassiveSkillsIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayShowPassiveSkillsVariableName);
    }
    if (ExistingShowPassiveSkillsIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingShowPassiveSkillsIndex].VarType = BoolPin();
    Blueprint->NewVariables[ExistingShowPassiveSkillsIndex].DefaultValue = TEXT("false");

    int32 ExistingIvMinIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayIvMinVariableName);
    if (ExistingIvMinIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayIvMinVariableName, IntPin())) {
            return false;
        }
        ExistingIvMinIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayIvMinVariableName);
    }
    if (ExistingIvMinIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingIvMinIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingIvMinIndex].DefaultValue = TEXT("0");

    const TArray<FName> IvMinimumNames = {
        OverlayIvHpMinVariableName,
        OverlayIvAttackMinVariableName,
        OverlayIvDefenseMinVariableName,
    };
    for (const FName& IvMinimumName : IvMinimumNames) {
        int32 ExistingIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, IvMinimumName);
        if (ExistingIndex == INDEX_NONE) {
            if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, IvMinimumName, IntPin())) {
                return false;
            }
            ExistingIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, IvMinimumName);
        }
        if (ExistingIndex == INDEX_NONE) {
            return false;
        }
        Blueprint->NewVariables[ExistingIndex].VarType = IntPin();
        Blueprint->NewVariables[ExistingIndex].DefaultValue = TEXT("0");
    }

    int32 ExistingGenderFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayGenderFilterIdVariableName);
    if (ExistingGenderFilterIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayGenderFilterIdVariableName, IntPin())) {
            return false;
        }
        ExistingGenderFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayGenderFilterIdVariableName);
    }
    if (ExistingGenderFilterIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingGenderFilterIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingGenderFilterIndex].DefaultValue = TEXT("0");

    int32 ExistingLuckyFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayLuckyFilterIdVariableName);
    if (ExistingLuckyFilterIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayLuckyFilterIdVariableName, IntPin())) {
            return false;
        }
        ExistingLuckyFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayLuckyFilterIdVariableName);
    }
    if (ExistingLuckyFilterIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingLuckyFilterIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingLuckyFilterIndex].DefaultValue = TEXT("0");

    int32 ExistingBossFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayBossFilterIdVariableName);
    if (ExistingBossFilterIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayBossFilterIdVariableName, IntPin())) {
            return false;
        }
        ExistingBossFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayBossFilterIdVariableName);
    }
    if (ExistingBossFilterIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingBossFilterIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingBossFilterIndex].DefaultValue = TEXT("0");

    int32 ExistingCollectionFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayCollectionFilterIdVariableName);
    if (ExistingCollectionFilterIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayCollectionFilterIdVariableName, IntPin())) {
            return false;
        }
        ExistingCollectionFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayCollectionFilterIdVariableName);
    }
    if (ExistingCollectionFilterIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingCollectionFilterIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingCollectionFilterIndex].DefaultValue = TEXT("0");

    int32 ExistingElementFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayElementFilterMaskVariableName);
    if (ExistingElementFilterIndex == INDEX_NONE) {
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, OverlayElementFilterMaskVariableName, IntPin())) {
            return false;
        }
        ExistingElementFilterIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, OverlayElementFilterMaskVariableName);
    }
    if (ExistingElementFilterIndex == INDEX_NONE) {
        return false;
    }
    Blueprint->NewVariables[ExistingElementFilterIndex].VarType = IntPin();
    Blueprint->NewVariables[ExistingElementFilterIndex].DefaultValue = TEXT("0");

    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);

    UK2Node_CustomEvent* AddTarget = AddCustomEvent(Blueprint, Graph, *OverlayAddTargetEventName.ToString(), -1500, -600, {
        TPair<FName, FEdGraphPinType>(FName("Target"), ObjectPin(PalMonsterClass)),
        TPair<FName, FEdGraphPinType>(FName("Level"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("DistanceMeters"), IntPin())
    });
    UK2Node_VariableGet* AddTargetsGet = AddVariableGet(Graph, OverlayTargetsVariableName, -1240, -440);
    UK2Node_CallArrayFunction* AddTargetItem = AddArrayCall(Graph, TEXT("Array_Add"), -960, -600);
    UK2Node_VariableGet* AddLevelsGet = AddVariableGet(Graph, OverlayTargetLevelsVariableName, -700, -440);
    UK2Node_CallArrayFunction* AddLevelItem = AddArrayCall(Graph, TEXT("Array_Add"), -420, -600);
    UK2Node_VariableGet* AddDistancesGet = AddVariableGet(Graph, OverlayTargetDistancesVariableName, -160, -440);
    UK2Node_CallArrayFunction* AddDistanceItem = AddArrayCall(Graph, TEXT("Array_Add"), 120, -600);
    UK2Node_CustomEvent* ClearTargets = AddCustomEvent(Blueprint, Graph, *OverlayClearTargetsEventName.ToString(), -1500, -300, {});
    UK2Node_VariableGet* ClearTargetsGet = AddVariableGet(Graph, OverlayTargetsVariableName, -1240, -140);
    UK2Node_CallArrayFunction* ClearTargetArray = AddArrayCall(Graph, TEXT("Array_Clear"), -960, -300);
    UK2Node_VariableGet* ClearLevelsGet = AddVariableGet(Graph, OverlayTargetLevelsVariableName, -700, -140);
    UK2Node_CallArrayFunction* ClearLevelArray = AddArrayCall(Graph, TEXT("Array_Clear"), -420, -300);
    UK2Node_VariableGet* ClearDistancesGet = AddVariableGet(Graph, OverlayTargetDistancesVariableName, -160, -140);
    UK2Node_CallArrayFunction* ClearDistanceArray = AddArrayCall(Graph, TEXT("Array_Clear"), 120, -300);
    UK2Node_VariableGet* ClearGendersGet = AddVariableGet(Graph, OverlayTargetGendersVariableName, 380, -140);
    UK2Node_CallArrayFunction* ClearGenderArray = AddArrayCall(Graph, TEXT("Array_Clear"), 660, -300);
    UK2Node_VariableGet* ClearLuckyStatesGet = AddVariableGet(Graph, OverlayTargetLuckyStatesVariableName, 920, -140);
    UK2Node_CallArrayFunction* ClearLuckyStateArray = AddArrayCall(Graph, TEXT("Array_Clear"), 1200, -300);
    UK2Node_VariableGet* ClearBossStatesGet = AddVariableGet(Graph, OverlayTargetBossStatesVariableName, 1460, -140);
    UK2Node_CallArrayFunction* ClearBossStateArray = AddArrayCall(Graph, TEXT("Array_Clear"), 1740, -300);
    UK2Node_VariableGet* ClearCaptureCountsGet = AddVariableGet(Graph, OverlayTargetCaptureCountsVariableName, 2000, -740);
    UK2Node_CallArrayFunction* ClearCaptureCountArray = AddArrayCall(Graph, TEXT("Array_Clear"), 2280, -900);
    UK2Node_VariableGet* ClearElementMasksGet = AddVariableGet(Graph, OverlayTargetElementMasksVariableName, 2000, -140);
    UK2Node_CallArrayFunction* ClearElementMaskArray = AddArrayCall(Graph, TEXT("Array_Clear"), 2280, -300);
    UK2Node_VariableGet* ClearIvHpGet = AddVariableGet(Graph, OverlayTargetIvHpVariableName, 2540, -140);
    UK2Node_CallArrayFunction* ClearIvHpArray = AddArrayCall(Graph, TEXT("Array_Clear"), 2820, -300);
    UK2Node_VariableGet* ClearIvAttackGet = AddVariableGet(Graph, OverlayTargetIvAttackVariableName, 3080, -140);
    UK2Node_CallArrayFunction* ClearIvAttackArray = AddArrayCall(Graph, TEXT("Array_Clear"), 3360, -300);
    UK2Node_VariableGet* ClearIvDefenseGet = AddVariableGet(Graph, OverlayTargetIvDefenseVariableName, 3620, -140);
    UK2Node_CallArrayFunction* ClearIvDefenseArray = AddArrayCall(Graph, TEXT("Array_Clear"), 3900, -300);
    UK2Node_VariableGet* ClearPassiveTextsGet = AddVariableGet(Graph, OverlayTargetPassiveTextsVariableName, 4160, -140);
    UK2Node_CallArrayFunction* ClearPassiveTextArray = AddArrayCall(Graph, TEXT("Array_Clear"), 4440, -300);
    UK2Node_VariableGet* ClearPassiveIdTextsGet = AddVariableGet(Graph, OverlayTargetPassiveIdTextsVariableName, 4700, -140);
    UK2Node_CallArrayFunction* ClearPassiveIdTextArray = AddArrayCall(Graph, TEXT("Array_Clear"), 4980, -300);
    UK2Node_VariableGet* ClearNamesGet = AddVariableGet(Graph, OverlayTargetNamesVariableName, 5240, -140);
    UK2Node_CallArrayFunction* ClearNameArray = AddArrayCall(Graph, TEXT("Array_Clear"), 5520, -300);
    UK2Node_VariableGet* ClearCharacterIdsGet = AddVariableGet(
        Graph, OverlayTargetCharacterIdsVariableName, 5780, -140);
    UK2Node_CallArrayFunction* ClearCharacterIdArray = AddArrayCall(Graph, TEXT("Array_Clear"), 6060, -300);
    UK2Node_VariableGet* ClearParametersGet = AddVariableGet(
        Graph, OverlayTargetParametersVariableName, 6320, -140);
    UK2Node_CallArrayFunction* ClearParameterArray = AddArrayCall(Graph, TEXT("Array_Clear"), 6600, -300);
    if (!AddTarget || !AddTargetsGet || !AddTargetItem || !AddLevelsGet || !AddLevelItem
        || !AddDistancesGet || !AddDistanceItem || !ClearTargets || !ClearTargetsGet || !ClearTargetArray
        || !ClearLevelsGet || !ClearLevelArray || !ClearDistancesGet || !ClearDistanceArray
        || !ClearGendersGet || !ClearGenderArray || !ClearLuckyStatesGet || !ClearLuckyStateArray
        || !ClearBossStatesGet || !ClearBossStateArray
        || !ClearCaptureCountsGet || !ClearCaptureCountArray
        || !ClearElementMasksGet || !ClearElementMaskArray
        || !ClearIvHpGet || !ClearIvHpArray || !ClearIvAttackGet || !ClearIvAttackArray
        || !ClearIvDefenseGet || !ClearIvDefenseArray
        || !ClearPassiveTextsGet || !ClearPassiveTextArray
        || !ClearPassiveIdTextsGet || !ClearPassiveIdTextArray
        || !ClearNamesGet || !ClearNameArray || !ClearCharacterIdsGet || !ClearCharacterIdArray
        || !ClearParametersGet || !ClearParameterArray
        || !Link(AddTarget, UEdGraphSchema_K2::PN_Then, AddTargetItem, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddTargetsGet, OverlayTargetsVariableName, AddTargetItem, TEXT("TargetArray"))
        || !Link(AddTarget, TEXT("Target"), AddTargetItem, TEXT("NewItem"))
        || !Link(AddTargetItem, UEdGraphSchema_K2::PN_Then, AddLevelItem, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddLevelsGet, OverlayTargetLevelsVariableName, AddLevelItem, TEXT("TargetArray"))
        || !Link(AddTarget, TEXT("Level"), AddLevelItem, TEXT("NewItem"))
        || !Link(AddLevelItem, UEdGraphSchema_K2::PN_Then, AddDistanceItem, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddDistancesGet, OverlayTargetDistancesVariableName, AddDistanceItem, TEXT("TargetArray"))
        || !Link(AddTarget, TEXT("DistanceMeters"), AddDistanceItem, TEXT("NewItem"))
        || !Link(ClearTargets, UEdGraphSchema_K2::PN_Then, ClearTargetArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearTargetsGet, OverlayTargetsVariableName, ClearTargetArray, TEXT("TargetArray"))
        || !Link(ClearTargetArray, UEdGraphSchema_K2::PN_Then, ClearLevelArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearLevelsGet, OverlayTargetLevelsVariableName, ClearLevelArray, TEXT("TargetArray"))
        || !Link(ClearLevelArray, UEdGraphSchema_K2::PN_Then, ClearDistanceArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearDistancesGet, OverlayTargetDistancesVariableName, ClearDistanceArray, TEXT("TargetArray"))
        || !Link(ClearDistanceArray, UEdGraphSchema_K2::PN_Then, ClearGenderArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearGendersGet, OverlayTargetGendersVariableName, ClearGenderArray, TEXT("TargetArray"))
        || !Link(ClearGenderArray, UEdGraphSchema_K2::PN_Then, ClearLuckyStateArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearLuckyStatesGet, OverlayTargetLuckyStatesVariableName, ClearLuckyStateArray, TEXT("TargetArray"))
        || !Link(ClearLuckyStateArray, UEdGraphSchema_K2::PN_Then, ClearBossStateArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearBossStatesGet, OverlayTargetBossStatesVariableName, ClearBossStateArray, TEXT("TargetArray"))
        || !Link(ClearBossStateArray, UEdGraphSchema_K2::PN_Then, ClearCaptureCountArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearCaptureCountsGet, OverlayTargetCaptureCountsVariableName, ClearCaptureCountArray, TEXT("TargetArray"))
        || !Link(ClearCaptureCountArray, UEdGraphSchema_K2::PN_Then, ClearElementMaskArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearElementMasksGet, OverlayTargetElementMasksVariableName, ClearElementMaskArray, TEXT("TargetArray"))
        || !Link(ClearElementMaskArray, UEdGraphSchema_K2::PN_Then, ClearIvHpArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearIvHpGet, OverlayTargetIvHpVariableName, ClearIvHpArray, TEXT("TargetArray"))
        || !Link(ClearIvHpArray, UEdGraphSchema_K2::PN_Then, ClearIvAttackArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearIvAttackGet, OverlayTargetIvAttackVariableName, ClearIvAttackArray, TEXT("TargetArray"))
        || !Link(ClearIvAttackArray, UEdGraphSchema_K2::PN_Then, ClearIvDefenseArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearIvDefenseGet, OverlayTargetIvDefenseVariableName, ClearIvDefenseArray, TEXT("TargetArray"))
        || !Link(ClearIvDefenseArray, UEdGraphSchema_K2::PN_Then, ClearPassiveTextArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearPassiveTextsGet, OverlayTargetPassiveTextsVariableName, ClearPassiveTextArray, TEXT("TargetArray"))
        || !Link(ClearPassiveTextArray, UEdGraphSchema_K2::PN_Then, ClearPassiveIdTextArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearPassiveIdTextsGet, OverlayTargetPassiveIdTextsVariableName, ClearPassiveIdTextArray, TEXT("TargetArray"))
        || !Link(ClearPassiveIdTextArray, UEdGraphSchema_K2::PN_Then, ClearNameArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearNamesGet, OverlayTargetNamesVariableName, ClearNameArray, TEXT("TargetArray"))
        || !Link(ClearNameArray, UEdGraphSchema_K2::PN_Then, ClearCharacterIdArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearCharacterIdsGet, OverlayTargetCharacterIdsVariableName, ClearCharacterIdArray, TEXT("TargetArray"))
        || !Link(ClearCharacterIdArray, UEdGraphSchema_K2::PN_Then, ClearParameterArray, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearParametersGet, OverlayTargetParametersVariableName, ClearParameterArray, TEXT("TargetArray"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay target array graph failed"));
        return false;
    }

    UK2Node_CustomEvent* RemoveTarget = AddCustomEvent(
        Blueprint, Graph, *OverlayRemoveTargetEventName.ToString(), -1500, 1600, {
            TPair<FName, FEdGraphPinType>(FName("Target"), ObjectPin(PalMonsterClass))
        });
    UK2Node_VariableGet* RemoveTargetsFindGet = AddVariableGet(
        Graph, OverlayTargetsVariableName, -1240, 1760);
    UK2Node_CallArrayFunction* RemoveTargetFind = AddArrayCall(Graph, TEXT("Array_Find"), -960, 1600);
    UK2Node_CallFunction* RemoveIndexValid = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -680, 1760);
    UK2Node_IfThenElse* RemoveIndexBranch = AddBranch(Graph, -400, 1600);
    if (!RemoveTarget || !RemoveTargetsFindGet || !RemoveTargetFind || !RemoveIndexValid || !RemoveIndexBranch
        || !Link(RemoveTarget, UEdGraphSchema_K2::PN_Then, RemoveIndexBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(RemoveTargetsFindGet, OverlayTargetsVariableName, RemoveTargetFind, TEXT("TargetArray"))
        || !Link(RemoveTarget, TEXT("Target"), RemoveTargetFind, TEXT("ItemToFind"))
        || !Link(RemoveTargetFind, UEdGraphSchema_K2::PN_ReturnValue, RemoveIndexValid, TEXT("A"))
        || !SetPinDefault(RemoveIndexValid, TEXT("B"), TEXT("0"))
        || !Link(RemoveIndexValid, UEdGraphSchema_K2::PN_ReturnValue, RemoveIndexBranch, UEdGraphSchema_K2::PN_Condition)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay target removal lookup graph failed"));
        return false;
    }

    const TArray<FName> RemoveArrayNames = {
        OverlayTargetsVariableName,
        OverlayTargetLevelsVariableName,
        OverlayTargetDistancesVariableName,
        OverlayTargetGendersVariableName,
        OverlayTargetLuckyStatesVariableName,
        OverlayTargetBossStatesVariableName,
        OverlayTargetCaptureCountsVariableName,
        OverlayTargetElementMasksVariableName,
        OverlayTargetIvHpVariableName,
        OverlayTargetIvAttackVariableName,
        OverlayTargetIvDefenseVariableName,
        OverlayTargetPassiveTextsVariableName,
        OverlayTargetPassiveIdTextsVariableName,
        OverlayTargetNamesVariableName,
        OverlayTargetCharacterIdsVariableName,
        OverlayTargetParametersVariableName,
    };
    UEdGraphNode* RemoveExecTail = RemoveIndexBranch;
    int32 RemoveNodeX = -120;
    for (const FName& ArrayName : RemoveArrayNames) {
        UK2Node_VariableGet* ArrayGet = AddVariableGet(Graph, ArrayName, RemoveNodeX, 1840);
        UK2Node_CallArrayFunction* RemoveAt = AddArrayCall(Graph, TEXT("Array_Remove"), RemoveNodeX + 260, 1600);
        if (!ArrayGet || !RemoveAt
            || !Link(RemoveExecTail, UEdGraphSchema_K2::PN_Then, RemoveAt, UEdGraphSchema_K2::PN_Execute)
            || !Link(ArrayGet, ArrayName, RemoveAt, TEXT("TargetArray"))
            || !Link(RemoveTargetFind, UEdGraphSchema_K2::PN_ReturnValue, RemoveAt, TEXT("IndexToRemove"))) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay target removal array failed array=%s"), *ArrayName.ToString());
            return false;
        }
        RemoveExecTail = RemoveAt;
        RemoveNodeX += 520;
    }

    // __DEPRECATED_20260717__ [reason: every target now needs a gender code; logging alone remains one-shot]
    // UK2Node_VariableGet* GenderLoggedGet = AddVariableGet(Graph, OverlayGenderLoggedVariableName, -700, -780);
    // UK2Node_CallFunction* GenderNot = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -460, -780);
    // UK2Node_IfThenElse* GenderGate = AddBranch(Graph, -180, -600);
    UK2Node_CallFunction* GenderTargetValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 60, -440);
    UK2Node_IfThenElse* GenderTargetBranch = AddBranch(Graph, 300, -600);
    UK2Node_CallFunction* CharacterParameter = AddStaticCall(Graph, PalMonsterClass, TEXT("GetCharacterParameterComponent"), 60, -280);
    UK2Node_CallFunction* CharacterParameterValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 540, -280);
    UK2Node_IfThenElse* CharacterParameterBranch = AddBranch(Graph, 780, -600);
    UK2Node_CallFunction* IndividualParameter = AddStaticCall(Graph, CharacterParameterComponentClass, TEXT("GetIndividualParameter"), 540, -120);
    UK2Node_CallFunction* IndividualParameterValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 1020, -120);
    UK2Node_IfThenElse* IndividualParameterBranch = AddBranch(Graph, 1260, -600);
    UK2Node_CallFunction* GenderType = AddStaticCall(Graph, IndividualParameterClass, TEXT("GetGenderType"), 1020, 40);
    UK2Node_CallFunction* IsRarePal = AddStaticCall(Graph, IndividualParameterClass, TEXT("IsRarePal"), 1260, 120);
    UK2Node_CallFunction* CharacterId = AddStaticCall(Graph, IndividualParameterClass, TEXT("GetCharacterID"), 1020, 200);
    UK2Node_CallFunction* CharacterDatabase = AddStaticCall(Graph, PalUtilityClass, TEXT("GetDatabaseCharacterParameter"), 1260, 280);
    UK2Node_CallFunction* IsBoss = AddStaticCall(Graph, DatabaseCharacterParameterClass, TEXT("GetIsBoss"), 1500, 280);
    UK2Node_CallFunction* GetSaveParameter = AddStaticCall(Graph, IndividualParameterClass, TEXT("GetSaveParameter"), 1500, 440);
    UK2Node_BreakStruct* BreakSaveParameter = NewObject<UK2Node_BreakStruct>(Graph);
    if (BreakSaveParameter) {
        BreakSaveParameter->StructType = IndividualSaveParameterStruct;
        BreakSaveParameter->NodePosX = 1780;
        BreakSaveParameter->NodePosY = 440;
        Graph->AddNode(BreakSaveParameter, true, false);
        BreakSaveParameter->CreateNewGuid();
        BreakSaveParameter->AllocateDefaultPins();
    }
    UK2Node_CallFunction* IvHpToInt = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_ByteToInt"), 2060, 440);
    UK2Node_CallFunction* IvAttackToInt = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_ByteToInt"), 2060, 520);
    UK2Node_CallFunction* IvDefenseToInt = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_ByteToInt"), 2060, 600);
    UK2Node_CallFunction* GetPassiveSkillList = AddStaticCall(Graph, IndividualParameterClass, TEXT("GetPassiveSkillList"), 1500, 680);
    UK2Node_VariableSet* ResetPassiveBuildText = AddVariableSet(Graph, OverlayPassiveBuildTextVariableName, 1780, 680);
    UK2Node_VariableSet* ResetPassiveIdBuildText = AddVariableSet(Graph, OverlayPassiveIdBuildTextVariableName, 1780, 760);
    UK2Node_MacroInstance* ForEachPassiveSkill = AddForEachLoop(Graph, 2060, 680);
    UK2Node_VariableGet* PassiveBuildTextGet = AddVariableGet(Graph, OverlayPassiveBuildTextVariableName, 2340, 840);
    UK2Node_CallFunction* PassiveBuildTextEmpty = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("EqualEqual_StrStr"), 2600, 840);
    UK2Node_CallFunction* PassiveSeparator = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 2860, 840);
    UK2Node_CallFunction* PassiveWithSeparator = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 3120, 840);
    UK2Node_CallFunction* GetPassiveSkillName = AddStaticCall(Graph, PalUIUtilityClass, TEXT("GetPassiveSkillName"), 2860, 1040);
    UK2Node_CallFunction* PassiveNameToString = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), 3120, 1040);
    UK2Node_CallFunction* PassiveWithName = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 3380, 920);
    UK2Node_VariableSet* StorePassiveBuildText = AddVariableSet(Graph, OverlayPassiveBuildTextVariableName, 3640, 680);
    UK2Node_VariableGet* PassiveIdBuildTextGet = AddVariableGet(Graph, OverlayPassiveIdBuildTextVariableName, 2340, 1200);
    UK2Node_CallFunction* PassiveIdToString = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), 2600, 1200);
    UK2Node_CallFunction* PassiveIdWithPrefix = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2860, 1200);
    UK2Node_CallFunction* PassiveIdWrapped = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 3120, 1200);
    UK2Node_CallFunction* PassiveIdAppended = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 3380, 1200);
    UK2Node_VariableSet* StorePassiveIdBuildText = AddVariableSet(Graph, OverlayPassiveIdBuildTextVariableName, 3900, 680);
    UK2Node_Self* CaptureWorldContext = AddSelfNode(Graph, 4160, 920);
    UK2Node_CallFunction* LocalRecordData = AddStaticCall(Graph, PalUtilityClass, TEXT("GetLocalRecordData"), 4420, 840);
    UK2Node_CallFunction* LocalRecordDataValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), 4680, 840);
    UK2Node_VariableGet* PalCaptureCounts = AddExternalVariableGet(
        Graph, TEXT("PalCaptureCount"), PlayerRecordDataClass, 4680, 1000);
    UK2Node_CallFunction* GetCaptureCount = AddStaticCall(
        Graph, PlayerRecordDataUtilityClass, TEXT("GetRecordData_Int"), 4940, 920);
    UK2Node_CallFunction* SelectCaptureCount = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), 5200, 840);
    UK2Node_VariableSet* StoreCaptureCount = AddVariableSet(Graph, OverlayCaptureCountBuildVariableName, 5460, 680);
    UK2Node_SwitchEnum* GenderSwitch = AddSwitchEnum(Graph, GenderEnum, 1500, -600);
    UK2Node_Self* GenderSelf = AddSelfNode(Graph, 1500, 160);
    if (!GenderTargetValid || !GenderTargetBranch
        || !CharacterParameter || !CharacterParameterValid || !CharacterParameterBranch
        || !IndividualParameter || !IndividualParameterValid || !IndividualParameterBranch
        || !GenderType || !IsRarePal || !CharacterId || !CharacterDatabase || !IsBoss
        || !GetSaveParameter || !BreakSaveParameter || !IvHpToInt || !IvAttackToInt || !IvDefenseToInt
        || !GetPassiveSkillList || !ResetPassiveBuildText || !ResetPassiveIdBuildText || !ForEachPassiveSkill || !PassiveBuildTextGet
        || !PassiveBuildTextEmpty || !PassiveSeparator || !PassiveWithSeparator || !GetPassiveSkillName
        || !PassiveNameToString || !PassiveWithName || !StorePassiveBuildText
        || !PassiveIdBuildTextGet || !PassiveIdToString || !PassiveIdWithPrefix || !PassiveIdWrapped
        || !PassiveIdAppended || !StorePassiveIdBuildText
        || !CaptureWorldContext || !LocalRecordData || !LocalRecordDataValid || !PalCaptureCounts
        || !GetCaptureCount || !SelectCaptureCount || !StoreCaptureCount
        || !GenderSwitch || !GenderSelf
        // __DEPRECATED_20260717__ [reason: the one-shot gate skipped gender normalization for later targets]
        // || !Link(AddDistanceItem, UEdGraphSchema_K2::PN_Then, GenderGate, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddDistanceItem, UEdGraphSchema_K2::PN_Then, GenderTargetBranch, UEdGraphSchema_K2::PN_Execute)
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
        // __DEPRECATED_20260718__ [reason: passive names are normalized before the gender result path]
        // || !Link(IndividualParameterBranch, UEdGraphSchema_K2::PN_Then, GenderSwitch, UEdGraphSchema_K2::PN_Execute)
        || !Link(IndividualParameterBranch, UEdGraphSchema_K2::PN_Then, ResetPassiveBuildText, UEdGraphSchema_K2::PN_Execute)
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, GenderType, UEdGraphSchema_K2::PN_Self)
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, IsRarePal, UEdGraphSchema_K2::PN_Self)
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, CharacterId, UEdGraphSchema_K2::PN_Self)
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, GetSaveParameter, UEdGraphSchema_K2::PN_Self)
        || !Link(GetSaveParameter, UEdGraphSchema_K2::PN_ReturnValue, BreakSaveParameter, IndividualSaveParameterStruct->GetFName())
        || !Link(BreakSaveParameter, TEXT("Talent_HP"), IvHpToInt, TEXT("InByte"))
        || !Link(BreakSaveParameter, TEXT("Talent_Shot"), IvAttackToInt, TEXT("InByte"))
        || !Link(BreakSaveParameter, TEXT("Talent_Defense"), IvDefenseToInt, TEXT("InByte"))
        || !Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, GetPassiveSkillList, UEdGraphSchema_K2::PN_Self)
        || !SetPinDefault(ResetPassiveBuildText, OverlayPassiveBuildTextVariableName, TEXT(""))
        || !Link(ResetPassiveBuildText, UEdGraphSchema_K2::PN_Then, ResetPassiveIdBuildText, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(ResetPassiveIdBuildText, OverlayPassiveIdBuildTextVariableName, TEXT(""))
        || !Link(ResetPassiveIdBuildText, UEdGraphSchema_K2::PN_Then, ForEachPassiveSkill, TEXT("Exec"))
        || !Link(GetPassiveSkillList, UEdGraphSchema_K2::PN_ReturnValue, ForEachPassiveSkill, TEXT("Array"))
        || !Link(ForEachPassiveSkill, TEXT("LoopBody"), StorePassiveBuildText, UEdGraphSchema_K2::PN_Execute)
        || !Link(PassiveBuildTextGet, OverlayPassiveBuildTextVariableName, PassiveBuildTextEmpty, TEXT("A"))
        || !SetPinDefault(PassiveBuildTextEmpty, TEXT("B"), TEXT(""))
        || !SetPinDefault(PassiveSeparator, TEXT("A"), TEXT(""))
        || !SetPinDefault(PassiveSeparator, TEXT("B"), TEXT(" / "))
        || !Link(PassiveBuildTextEmpty, UEdGraphSchema_K2::PN_ReturnValue, PassiveSeparator, TEXT("bPickA"))
        || !Link(PassiveBuildTextGet, OverlayPassiveBuildTextVariableName, PassiveWithSeparator, TEXT("A"))
        || !Link(PassiveSeparator, UEdGraphSchema_K2::PN_ReturnValue, PassiveWithSeparator, TEXT("B"))
        || !Link(GenderSelf, UEdGraphSchema_K2::PN_Self, GetPassiveSkillName, TEXT("WorldContextObject"))
        || !Link(ForEachPassiveSkill, TEXT("Array Element"), GetPassiveSkillName, TEXT("PassiveSkillId"))
        || !Link(GetPassiveSkillName, TEXT("outName"), PassiveNameToString, TEXT("InText"))
        || !Link(PassiveWithSeparator, UEdGraphSchema_K2::PN_ReturnValue, PassiveWithName, TEXT("A"))
        || !Link(PassiveNameToString, UEdGraphSchema_K2::PN_ReturnValue, PassiveWithName, TEXT("B"))
        || !Link(PassiveWithName, UEdGraphSchema_K2::PN_ReturnValue, StorePassiveBuildText, OverlayPassiveBuildTextVariableName)
        || !Link(StorePassiveBuildText, UEdGraphSchema_K2::PN_Then, StorePassiveIdBuildText, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachPassiveSkill, TEXT("Array Element"), PassiveIdToString, TEXT("InName"))
        || !SetPinDefault(PassiveIdWithPrefix, TEXT("A"), TEXT("|"))
        || !Link(PassiveIdToString, UEdGraphSchema_K2::PN_ReturnValue, PassiveIdWithPrefix, TEXT("B"))
        || !Link(PassiveIdWithPrefix, UEdGraphSchema_K2::PN_ReturnValue, PassiveIdWrapped, TEXT("A"))
        || !SetPinDefault(PassiveIdWrapped, TEXT("B"), TEXT("|"))
        || !Link(PassiveIdBuildTextGet, OverlayPassiveIdBuildTextVariableName, PassiveIdAppended, TEXT("A"))
        || !Link(PassiveIdWrapped, UEdGraphSchema_K2::PN_ReturnValue, PassiveIdAppended, TEXT("B"))
        || !Link(PassiveIdAppended, UEdGraphSchema_K2::PN_ReturnValue, StorePassiveIdBuildText, OverlayPassiveIdBuildTextVariableName)
        || !Link(CaptureWorldContext, UEdGraphSchema_K2::PN_Self, LocalRecordData, TEXT("WorldContextObject"))
        || !Link(LocalRecordData, UEdGraphSchema_K2::PN_ReturnValue, LocalRecordDataValid, TEXT("Object"))
        || !Link(LocalRecordData, UEdGraphSchema_K2::PN_ReturnValue, PalCaptureCounts, UEdGraphSchema_K2::PN_Self)
        || !Link(PalCaptureCounts, TEXT("PalCaptureCount"), GetCaptureCount, TEXT("RecordData"))
        || !Link(CharacterId, UEdGraphSchema_K2::PN_ReturnValue, GetCaptureCount, TEXT("Key"))
        || !Link(GetCaptureCount, UEdGraphSchema_K2::PN_ReturnValue, SelectCaptureCount, TEXT("A"))
        || !SetPinDefault(SelectCaptureCount, TEXT("B"), TEXT("-1"))
        || !Link(LocalRecordDataValid, UEdGraphSchema_K2::PN_ReturnValue, SelectCaptureCount, TEXT("bPickA"))
        || !Link(ForEachPassiveSkill, TEXT("Completed"), StoreCaptureCount, UEdGraphSchema_K2::PN_Execute)
        || !Link(SelectCaptureCount, UEdGraphSchema_K2::PN_ReturnValue, StoreCaptureCount, OverlayCaptureCountBuildVariableName)
        || !Link(StoreCaptureCount, UEdGraphSchema_K2::PN_Then, GenderSwitch, UEdGraphSchema_K2::PN_Execute)
        || !Link(GenderSelf, UEdGraphSchema_K2::PN_Self, CharacterDatabase, TEXT("WorldContextObject"))
        || !Link(CharacterDatabase, UEdGraphSchema_K2::PN_ReturnValue, IsBoss, UEdGraphSchema_K2::PN_Self)
        || !Link(CharacterId, UEdGraphSchema_K2::PN_ReturnValue, IsBoss, TEXT("RowName"))
        || !Link(GenderType, UEdGraphSchema_K2::PN_ReturnValue, GenderSwitch, TEXT("Selection"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay gender adapter graph failed"));
        return false;
    }

    const TArray<TPair<FString, int32>> ElementDefinitions = {
        {TEXT("Normal"), 1},
        {TEXT("Fire"), 2},
        {TEXT("Water"), 4},
        {TEXT("Leaf"), 8},
        {TEXT("Electricity"), 16},
        {TEXT("Ice"), 32},
        {TEXT("Earth"), 64},
        {TEXT("Dark"), 128},
        {TEXT("Dragon"), 256},
    };
    UEdGraphNode* ElementMaskValue = nullptr;
    int32 ElementX = 1780;
    for (const TPair<FString, int32>& Definition : ElementDefinitions) {
        if (ElementEnum->GetValueByNameString(Definition.Key) == INDEX_NONE) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] unknown Pal element enum=%s"), *Definition.Key);
            return false;
        }
        UK2Node_CallFunction* HasElement = AddStaticCall(
            Graph, CharacterParameterComponentClass, TEXT("HasElementType"), ElementX, 320);
        UK2Node_CallFunction* SelectElementBit = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), ElementX + 240, 320);
        if (!HasElement || !SelectElementBit
            || !Link(CharacterParameter, UEdGraphSchema_K2::PN_ReturnValue, HasElement, UEdGraphSchema_K2::PN_Self)
            || !SetPinDefault(HasElement, TEXT("ElementType"), Definition.Key)
            || !SetPinDefault(SelectElementBit, TEXT("A"), FString::FromInt(Definition.Value))
            || !SetPinDefault(SelectElementBit, TEXT("B"), TEXT("0"))
            || !Link(HasElement, UEdGraphSchema_K2::PN_ReturnValue, SelectElementBit, TEXT("bPickA"))) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] element mask source failed element=%s"), *Definition.Key);
            return false;
        }
        if (ElementMaskValue == nullptr) {
            ElementMaskValue = SelectElementBit;
        } else {
            UK2Node_CallFunction* CombineElementBits = AddStaticCall(
                Graph, UKismetMathLibrary::StaticClass(), TEXT("Or_IntInt"), ElementX + 480, 320);
            if (!CombineElementBits
                || !Link(ElementMaskValue, UEdGraphSchema_K2::PN_ReturnValue, CombineElementBits, TEXT("A"))
                || !Link(SelectElementBit, UEdGraphSchema_K2::PN_ReturnValue, CombineElementBits, TEXT("B"))) {
                UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] element mask combine failed element=%s"), *Definition.Key);
                return false;
            }
            ElementMaskValue = CombineElementBits;
        }
        ElementX += 760;
    }
    if (!ElementMaskValue) {
        return false;
    }

    // __DEPRECATED_20260717__ [reason: diagnostic-only paths did not retain one code per target]
#if 0
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
#endif

    auto AddGenderResultPath = [&](UEdGraphNode* ExecNode, const FName& ExecPin, const TCHAR* Value, int32 Code,
                                   bool bUseName, bool bLuckyKnown, bool bBossKnown, bool bCaptureKnown,
                                   bool bElementsKnown, bool bIvKnown,
                                   bool bPassiveKnown,
                                   int32 X, int32 Y) -> bool {
        UK2Node_VariableGet* GendersGet = AddVariableGet(Graph, OverlayTargetGendersVariableName, X, Y + 120);
        UK2Node_CallArrayFunction* AddGenderItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 260, Y);
        UK2Node_VariableGet* LuckyStatesGet = AddVariableGet(Graph, OverlayTargetLuckyStatesVariableName, X + 520, Y + 120);
        UK2Node_CallArrayFunction* AddLuckyStateItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 780, Y);
        UK2Node_CallFunction* SelectLuckyState = bLuckyKnown
            ? AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), X + 520, Y + 300)
            : nullptr;
        UK2Node_VariableGet* BossStatesGet = AddVariableGet(Graph, OverlayTargetBossStatesVariableName, X + 1040, Y + 120);
        UK2Node_CallArrayFunction* AddBossStateItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 1300, Y);
        UK2Node_CallFunction* SelectBossState = bBossKnown
            ? AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), X + 1040, Y + 300)
            : nullptr;
        UK2Node_VariableGet* CaptureCountsGet = AddVariableGet(Graph, OverlayTargetCaptureCountsVariableName, X + 1040, Y + 480);
        UK2Node_CallArrayFunction* AddCaptureCountItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 1300, Y + 480);
        UK2Node_VariableGet* CaptureCountBuildSource = AddVariableGet(Graph, OverlayCaptureCountBuildVariableName, X + 1040, Y + 620);
        UK2Node_VariableGet* ElementMasksGet = AddVariableGet(Graph, OverlayTargetElementMasksVariableName, X + 1560, Y + 120);
        UK2Node_CallArrayFunction* AddElementMaskItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 1820, Y);
        UK2Node_VariableGet* IvHpGet = AddVariableGet(Graph, OverlayTargetIvHpVariableName, X + 2080, Y + 120);
        UK2Node_CallArrayFunction* AddIvHpItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 2340, Y);
        UK2Node_VariableGet* IvAttackGet = AddVariableGet(Graph, OverlayTargetIvAttackVariableName, X + 2600, Y + 120);
        UK2Node_CallArrayFunction* AddIvAttackItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 2860, Y);
        UK2Node_VariableGet* IvDefenseGet = AddVariableGet(Graph, OverlayTargetIvDefenseVariableName, X + 3120, Y + 120);
        UK2Node_CallArrayFunction* AddIvDefenseItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 3380, Y);
        UK2Node_VariableGet* PassiveTextsGet = AddVariableGet(Graph, OverlayTargetPassiveTextsVariableName, X + 3640, Y + 120);
        UK2Node_CallArrayFunction* AddPassiveTextItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 3900, Y);
        UK2Node_VariableGet* PassiveBuildTextSource = AddVariableGet(Graph, OverlayPassiveBuildTextVariableName, X + 3640, Y + 260);
        UK2Node_VariableGet* PassiveIdTextsGet = AddVariableGet(Graph, OverlayTargetPassiveIdTextsVariableName, X + 4160, Y + 120);
        UK2Node_CallArrayFunction* AddPassiveIdTextItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 4420, Y);
        UK2Node_VariableGet* PassiveIdBuildTextSource = AddVariableGet(Graph, OverlayPassiveIdBuildTextVariableName, X + 4160, Y + 260);
        UK2Node_VariableGet* NamesGet = AddVariableGet(Graph, OverlayTargetNamesVariableName, X + 4680, Y + 120);
        UK2Node_CallArrayFunction* AddNameItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 4940, Y);
        UK2Node_CallFunction* Nickname = bUseName
            ? AddStaticCall(Graph, CharacterParameterComponentClass, TEXT("GetNickname"), X + 4680, Y + 260)
            : nullptr;
        UK2Node_VariableGet* CharacterIdsGet = AddVariableGet(
            Graph, OverlayTargetCharacterIdsVariableName, X + 5200, Y + 120);
        UK2Node_CallArrayFunction* AddCharacterIdItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 5460, Y);
        UK2Node_VariableGet* ParametersGet = AddVariableGet(
            Graph, OverlayTargetParametersVariableName, X + 5720, Y + 120);
        UK2Node_CallArrayFunction* AddParameterItem = AddArrayCall(Graph, TEXT("Array_Add"), X + 5980, Y);
        UK2Node_VariableGet* LoggedGet = AddVariableGet(Graph, OverlayGenderLoggedVariableName, X + 6240, Y + 160);
        UK2Node_CallFunction* NotLogged = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), X + 6500, Y + 160);
        UK2Node_IfThenElse* LogBranch = AddBranch(Graph, X + 6760, Y);
        UK2Node_VariableSet* StoreDiagnostic = AddVariableSet(Graph, OverlayGenderDiagnosticVariableName, X + 7020, Y);
        UK2Node_VariableSet* StoreDiagnosticCode = AddVariableSet(Graph, OverlayGenderDiagnosticCodeVariableName, X + 7280, Y);
        UK2Node_CallFunction* Print = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("PrintString"), X + 7540, Y);
        UK2Node_VariableSet* MarkLogged = AddVariableSet(Graph, OverlayGenderLoggedVariableName, X + 7800, Y);
        const FString Message = FString::Printf(TEXT("[PalworldResourceESP] BLUEPRINT_GENDER value=%s"), Value);
        const bool bLuckySourceReady = bLuckyKnown
            ? SelectLuckyState
                && SetPinDefault(SelectLuckyState, TEXT("A"), TEXT("1"))
                && SetPinDefault(SelectLuckyState, TEXT("B"), TEXT("0"))
                && Link(IsRarePal, UEdGraphSchema_K2::PN_ReturnValue, SelectLuckyState, TEXT("bPickA"))
                && Link(SelectLuckyState, UEdGraphSchema_K2::PN_ReturnValue, AddLuckyStateItem, TEXT("NewItem"))
            : SetPinDefault(AddLuckyStateItem, TEXT("NewItem"), TEXT("-1"));
        const bool bBossSourceReady = bBossKnown
            ? SelectBossState
                && SetPinDefault(SelectBossState, TEXT("A"), TEXT("1"))
                && SetPinDefault(SelectBossState, TEXT("B"), TEXT("0"))
                && Link(IsBoss, UEdGraphSchema_K2::PN_ReturnValue, SelectBossState, TEXT("bPickA"))
                && Link(SelectBossState, UEdGraphSchema_K2::PN_ReturnValue, AddBossStateItem, TEXT("NewItem"))
            : SetPinDefault(AddBossStateItem, TEXT("NewItem"), TEXT("-1"));
        const bool bCaptureSourceReady = bCaptureKnown
            ? Link(CaptureCountBuildSource, OverlayCaptureCountBuildVariableName, AddCaptureCountItem, TEXT("NewItem"))
            : SetPinDefault(AddCaptureCountItem, TEXT("NewItem"), TEXT("-1"));
        const bool bElementSourceReady = bElementsKnown
            ? Link(ElementMaskValue, UEdGraphSchema_K2::PN_ReturnValue, AddElementMaskItem, TEXT("NewItem"))
            : SetPinDefault(AddElementMaskItem, TEXT("NewItem"), TEXT("-1"));
        const bool bIvSourceReady = bIvKnown
            ? Link(IvHpToInt, UEdGraphSchema_K2::PN_ReturnValue, AddIvHpItem, TEXT("NewItem"))
                && Link(IvAttackToInt, UEdGraphSchema_K2::PN_ReturnValue, AddIvAttackItem, TEXT("NewItem"))
                && Link(IvDefenseToInt, UEdGraphSchema_K2::PN_ReturnValue, AddIvDefenseItem, TEXT("NewItem"))
            : SetPinDefault(AddIvHpItem, TEXT("NewItem"), TEXT("-1"))
                && SetPinDefault(AddIvAttackItem, TEXT("NewItem"), TEXT("-1"))
                && SetPinDefault(AddIvDefenseItem, TEXT("NewItem"), TEXT("-1"));
        const bool bNameSourceReady = bUseName
            ? Nickname
                && Link(CharacterParameter, UEdGraphSchema_K2::PN_ReturnValue, Nickname, UEdGraphSchema_K2::PN_Self)
                && Link(Nickname, TEXT("outName"), AddNameItem, TEXT("NewItem"))
            : SetPinDefault(AddNameItem, TEXT("NewItem"), TEXT(""));
        const bool bPassiveSourceReady = bPassiveKnown
            ? Link(PassiveBuildTextSource, OverlayPassiveBuildTextVariableName, AddPassiveTextItem, TEXT("NewItem"))
            : SetPinDefault(AddPassiveTextItem, TEXT("NewItem"), TEXT(""));
        const bool bPassiveIdSourceReady = bPassiveKnown
            ? Link(PassiveIdBuildTextSource, OverlayPassiveIdBuildTextVariableName, AddPassiveIdTextItem, TEXT("NewItem"))
            : SetPinDefault(AddPassiveIdTextItem, TEXT("NewItem"), TEXT(""));
        const bool bCharacterIdSourceReady = bLuckyKnown
            ? Link(CharacterId, UEdGraphSchema_K2::PN_ReturnValue, AddCharacterIdItem, TEXT("NewItem"))
            : SetPinDefault(AddCharacterIdItem, TEXT("NewItem"), TEXT("None"));
        const bool bParameterSourceReady = bLuckyKnown
            ? Link(IndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, AddParameterItem, TEXT("NewItem"))
            : SetPinDefault(AddParameterItem, TEXT("NewItem"), TEXT("None"));
        return GendersGet && AddGenderItem && LuckyStatesGet && AddLuckyStateItem && bLuckySourceReady
            && BossStatesGet && AddBossStateItem && bBossSourceReady
            && CaptureCountsGet && AddCaptureCountItem && CaptureCountBuildSource && bCaptureSourceReady
            && ElementMasksGet && AddElementMaskItem && bElementSourceReady
            && IvHpGet && AddIvHpItem && IvAttackGet && AddIvAttackItem
            && IvDefenseGet && AddIvDefenseItem && bIvSourceReady
            && PassiveTextsGet && AddPassiveTextItem && PassiveBuildTextSource && bPassiveSourceReady
            && PassiveIdTextsGet && AddPassiveIdTextItem && PassiveIdBuildTextSource && bPassiveIdSourceReady
            && NamesGet && AddNameItem && bNameSourceReady
            && CharacterIdsGet && AddCharacterIdItem && bCharacterIdSourceReady
            && ParametersGet && AddParameterItem && bParameterSourceReady
            && LoggedGet && NotLogged && LogBranch
            && StoreDiagnostic && StoreDiagnosticCode && Print && MarkLogged
            && Link(ExecNode, ExecPin, AddGenderItem, UEdGraphSchema_K2::PN_Execute)
            && Link(GendersGet, OverlayTargetGendersVariableName, AddGenderItem, TEXT("TargetArray"))
            && SetPinDefault(AddGenderItem, TEXT("NewItem"), FString::FromInt(Code))
            && Link(AddGenderItem, UEdGraphSchema_K2::PN_Then, AddLuckyStateItem, UEdGraphSchema_K2::PN_Execute)
            && Link(LuckyStatesGet, OverlayTargetLuckyStatesVariableName, AddLuckyStateItem, TEXT("TargetArray"))
            && Link(AddLuckyStateItem, UEdGraphSchema_K2::PN_Then, AddBossStateItem, UEdGraphSchema_K2::PN_Execute)
            && Link(BossStatesGet, OverlayTargetBossStatesVariableName, AddBossStateItem, TEXT("TargetArray"))
            && Link(AddBossStateItem, UEdGraphSchema_K2::PN_Then, AddCaptureCountItem, UEdGraphSchema_K2::PN_Execute)
            && Link(CaptureCountsGet, OverlayTargetCaptureCountsVariableName, AddCaptureCountItem, TEXT("TargetArray"))
            && Link(AddCaptureCountItem, UEdGraphSchema_K2::PN_Then, AddElementMaskItem, UEdGraphSchema_K2::PN_Execute)
            && Link(ElementMasksGet, OverlayTargetElementMasksVariableName, AddElementMaskItem, TEXT("TargetArray"))
            && Link(AddElementMaskItem, UEdGraphSchema_K2::PN_Then, AddIvHpItem, UEdGraphSchema_K2::PN_Execute)
            && Link(IvHpGet, OverlayTargetIvHpVariableName, AddIvHpItem, TEXT("TargetArray"))
            && Link(AddIvHpItem, UEdGraphSchema_K2::PN_Then, AddIvAttackItem, UEdGraphSchema_K2::PN_Execute)
            && Link(IvAttackGet, OverlayTargetIvAttackVariableName, AddIvAttackItem, TEXT("TargetArray"))
            && Link(AddIvAttackItem, UEdGraphSchema_K2::PN_Then, AddIvDefenseItem, UEdGraphSchema_K2::PN_Execute)
            && Link(IvDefenseGet, OverlayTargetIvDefenseVariableName, AddIvDefenseItem, TEXT("TargetArray"))
            && Link(AddIvDefenseItem, UEdGraphSchema_K2::PN_Then, AddPassiveTextItem, UEdGraphSchema_K2::PN_Execute)
            && Link(PassiveTextsGet, OverlayTargetPassiveTextsVariableName, AddPassiveTextItem, TEXT("TargetArray"))
            && Link(AddPassiveTextItem, UEdGraphSchema_K2::PN_Then, AddPassiveIdTextItem, UEdGraphSchema_K2::PN_Execute)
            && Link(PassiveIdTextsGet, OverlayTargetPassiveIdTextsVariableName, AddPassiveIdTextItem, TEXT("TargetArray"))
            && Link(AddPassiveIdTextItem, UEdGraphSchema_K2::PN_Then, AddNameItem, UEdGraphSchema_K2::PN_Execute)
            && Link(NamesGet, OverlayTargetNamesVariableName, AddNameItem, TEXT("TargetArray"))
            && Link(AddNameItem, UEdGraphSchema_K2::PN_Then, AddCharacterIdItem, UEdGraphSchema_K2::PN_Execute)
            && Link(CharacterIdsGet, OverlayTargetCharacterIdsVariableName, AddCharacterIdItem, TEXT("TargetArray"))
            && Link(AddCharacterIdItem, UEdGraphSchema_K2::PN_Then, AddParameterItem, UEdGraphSchema_K2::PN_Execute)
            && Link(ParametersGet, OverlayTargetParametersVariableName, AddParameterItem, TEXT("TargetArray"))
            && Link(AddParameterItem, UEdGraphSchema_K2::PN_Then, LogBranch, UEdGraphSchema_K2::PN_Execute)
            && Link(LoggedGet, OverlayGenderLoggedVariableName, NotLogged, TEXT("A"))
            && Link(NotLogged, UEdGraphSchema_K2::PN_ReturnValue, LogBranch, UEdGraphSchema_K2::PN_Condition)
            && Link(LogBranch, UEdGraphSchema_K2::PN_Then, StoreDiagnostic, UEdGraphSchema_K2::PN_Execute)
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
    if (!AddGenderResultPath(GenderTargetBranch, UEdGraphSchema_K2::PN_Else, TEXT("unknown"), 0, false, false, false, false, false, false, false, 1740, -1040)
        || !AddGenderResultPath(CharacterParameterBranch, UEdGraphSchema_K2::PN_Else, TEXT("unknown"), 0, false, false, false, false, false, false, false, 1740, -880)
        || !AddGenderResultPath(IndividualParameterBranch, UEdGraphSchema_K2::PN_Else, TEXT("unknown"), 0, true, false, false, false, true, false, false, 1740, -720)
        || !AddGenderResultPath(GenderSwitch, TEXT("None"), TEXT("unknown"), 0, true, true, true, true, true, true, true, 1740, -560)
        || !AddGenderResultPath(GenderSwitch, TEXT("Male"), TEXT("male"), 1, true, true, true, true, true, true, true, 1740, -400)
        || !AddGenderResultPath(GenderSwitch, TEXT("Female"), TEXT("female"), 2, true, true, true, true, true, true, true, 1740, -240)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay gender/Lucky/Boss/element/IV result graph failed"));
        return false;
    }

    // Materialize the custom-event signatures before wiring the OnPaint self-repair calls.
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay pre-paint compile failed status=%d"), static_cast<int32>(Blueprint->Status));
        return false;
    }
    UClass* OverlayRuntimeClass = Blueprint->GeneratedClass;

    UK2Node_Event* OnPaint = AddOverrideEvent(Blueprint, Graph, UUserWidget::StaticClass(), TEXT("OnPaint"), -1500, 80);
    UK2Node_VariableGet* TopGuideEnabledGet = AddVariableGet(Graph, OverlayTopGuideEnabledVariableName, -1500, 200);
    UK2Node_VariableGet* ShowNameGet = AddVariableGet(Graph, OverlayShowNameVariableName, -1500, 280);
    UK2Node_VariableGet* ShowLevelGet = AddVariableGet(Graph, OverlayShowLevelVariableName, -1500, 360);
    UK2Node_VariableGet* ShowDistanceGet = AddVariableGet(Graph, OverlayShowDistanceVariableName, -1500, 520);
    UK2Node_VariableGet* ShowIvGet = AddVariableGet(Graph, OverlayShowIvVariableName, -1500, 600);
    UK2Node_VariableGet* ShowPassiveSkillsGet = AddVariableGet(Graph, OverlayShowPassiveSkillsVariableName, -1500, 640);
    UK2Node_VariableGet* LanguageIdGet = AddVariableGet(Graph, OverlayLanguageIdVariableName, -1500, 560);
    UK2Node_CallFunction* LanguageIsEnglish = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -1220, 560);
    UK2Node_VariableGet* GenderFilterGet = AddVariableGet(Graph, OverlayGenderFilterIdVariableName, -1500, 680);
    UK2Node_VariableGet* LuckyFilterGet = AddVariableGet(Graph, OverlayLuckyFilterIdVariableName, -1500, 760);
    UK2Node_VariableGet* BossFilterGet = AddVariableGet(Graph, OverlayBossFilterIdVariableName, -1500, 840);
    UK2Node_VariableGet* CollectionFilterGet = AddVariableGet(Graph, OverlayCollectionFilterIdVariableName, -1500, 1320);
    UK2Node_VariableGet* ElementFilterGet = AddVariableGet(Graph, OverlayElementFilterMaskVariableName, -1500, 920);
    // __DEPRECATED_20260718__ [reason: unified IV threshold was replaced by HP/ATK/DEF thresholds]
    UK2Node_VariableGet* IvMinGet = AddVariableGet(Graph, OverlayIvMinVariableName, -1500, 1000);
    UK2Node_VariableGet* IvHpMinGet = AddVariableGet(Graph, OverlayIvHpMinVariableName, -1500, 1080);
    UK2Node_VariableGet* IvAttackMinGet = AddVariableGet(Graph, OverlayIvAttackMinVariableName, -1500, 1160);
    UK2Node_VariableGet* IvDefenseMinGet = AddVariableGet(Graph, OverlayIvDefenseMinVariableName, -1500, 1240);
    UK2Node_CallFunction* BasicMetadataEnabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), -300, 320);
    UK2Node_CallFunction* MetadataEnabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), -20, 320);
    UK2Node_CallFunction* AllMetadataEnabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 260, 480);
    UK2Node_CallFunction* VisibleNameEnabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), -20, 480);
    UK2Node_CallFunction* LabelsEnabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 260, 320);
    UK2Node_ExecutionSequence* PaintSequence = AddSequence(Graph, 0, 80);
    UK2Node_IfThenElse* LabelEnabledBranch = AddBranch(Graph, 520, 260);
    UK2Node_IfThenElse* TopGuideEnabledBranch = AddBranch(Graph, 520, 80);
    UK2Node_VariableGet* TargetsGet = AddVariableGet(Graph, OverlayTargetsVariableName, -1500, 300);
    UK2Node_MacroInstance* ForEachTarget = AddForEachLoop(Graph, -1000, 80);
    UK2Node_VariableGet* TargetLevelsGet = AddVariableGet(Graph, OverlayTargetLevelsVariableName, -1000, 1040);
    UK2Node_CallArrayFunction* TargetLevelGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 1040);
    UK2Node_VariableGet* TargetNamesGet = AddVariableGet(Graph, OverlayTargetNamesVariableName, -1000, 880);
    UK2Node_CallArrayFunction* TargetNameGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 880);
    UK2Node_CallFunction* NameNotEmpty = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), -440, 880);
    UK2Node_VariableGet* TargetCharacterIdsGet = AddVariableGet(
        Graph, OverlayTargetCharacterIdsVariableName, -1000, 1280);
    UK2Node_CallArrayFunction* TargetCharacterIdGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 1280);
    UK2Node_VariableGet* TargetParametersGet = AddVariableGet(
        Graph, OverlayTargetParametersVariableName, -1000, 1360);
    UK2Node_CallArrayFunction* TargetParameterGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 1360);
    UK2Node_VariableGet* SpeciesFilterIdsGet = AddVariableGet(
        Graph, OverlaySpeciesFilterIdsVariableName, -440, 1120);
    UK2Node_CallArrayFunction* SpeciesFilterCount = AddArrayCall(Graph, TEXT("Array_Length"), -160, 1120);
    UK2Node_CallFunction* SpeciesFilterAll = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 120, 1120);
    UK2Node_CallArrayFunction* SpeciesFilterContains = AddArrayCall(Graph, TEXT("Array_Contains"), -160, 1280);
    UK2Node_CallFunction* SpeciesFilterAccepted = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 400, 1200);
    UK2Node_VariableGet* TargetGendersGet = AddVariableGet(Graph, OverlayTargetGendersVariableName, -1000, 1440);
    UK2Node_CallArrayFunction* TargetGenderGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 1440);
    UK2Node_CallFunction* GenderFilterAll = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 1440);
    UK2Node_CallFunction* GenderFilterMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -160, 1440);
    UK2Node_CallFunction* GenderFilterAccepted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 120, 1440);
    UK2Node_VariableGet* TargetLuckyStatesGet = AddVariableGet(Graph, OverlayTargetLuckyStatesVariableName, -1000, 1600);
    UK2Node_CallArrayFunction* TargetLuckyStateGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 1600);
    UK2Node_CallFunction* LuckyFilterAll = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 1600);
    UK2Node_CallFunction* LuckyFilterOnly = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 1760);
    UK2Node_CallFunction* LuckyFilterExclude = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 1920);
    UK2Node_CallFunction* LuckyFilterRestricted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), -160, 1840);
    UK2Node_CallFunction* LuckyExpectedState = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), -160, 1600);
    UK2Node_CallFunction* LuckyFilterMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 120, 1600);
    UK2Node_CallFunction* LuckyRestrictedMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 400, 1680);
    UK2Node_CallFunction* LuckyFilterAccepted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 680, 1600);
    UK2Node_VariableGet* TargetBossStatesGet = AddVariableGet(Graph, OverlayTargetBossStatesVariableName, -1000, 2080);
    UK2Node_CallArrayFunction* TargetBossStateGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 2080);
    UK2Node_CallFunction* BossFilterAll = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 2080);
    UK2Node_CallFunction* BossFilterOnly = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 2240);
    UK2Node_CallFunction* BossFilterExclude = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 2400);
    UK2Node_CallFunction* BossFilterRestricted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), -160, 2320);
    UK2Node_CallFunction* BossExpectedState = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), -160, 2080);
    UK2Node_CallFunction* BossFilterMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 120, 2080);
    UK2Node_CallFunction* BossRestrictedMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 400, 2160);
    UK2Node_CallFunction* BossFilterAccepted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 680, 2080);
    UK2Node_VariableGet* TargetCaptureCountsGet = AddVariableGet(Graph, OverlayTargetCaptureCountsVariableName, -1000, 4480);
    UK2Node_CallArrayFunction* TargetCaptureCountGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 4480);
    UK2Node_CallFunction* CollectionFilterAll = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 4480);
    UK2Node_CallFunction* CollectionFilterComplete = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 4640);
    UK2Node_CallFunction* TargetCollectionKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -160, 4480);
    UK2Node_CallFunction* CollectionComplete = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -160, 4640);
    UK2Node_CallFunction* CollectionCompleteState = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), 120, 4640);
    UK2Node_CallFunction* CollectionExpectedState = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), 120, 4480);
    UK2Node_CallFunction* CollectionFilterMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 400, 4480);
    UK2Node_CallFunction* CollectionKnownMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 680, 4560);
    UK2Node_CallFunction* CollectionFilterAccepted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 960, 4480);
    UK2Node_VariableGet* TargetElementMasksGet = AddVariableGet(Graph, OverlayTargetElementMasksVariableName, -1000, 2560);
    UK2Node_CallArrayFunction* TargetElementMaskGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 2560);
    UK2Node_CallFunction* ElementFilterAll = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 2560);
    UK2Node_CallFunction* TargetElementKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 2720);
    UK2Node_CallFunction* ElementMaskOverlap = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("And_IntInt"), -160, 2560);
    UK2Node_CallFunction* ElementMaskMatches = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("NotEqual_IntInt"), 120, 2560);
    UK2Node_CallFunction* ElementKnownMatch = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 400, 2640);
    UK2Node_CallFunction* ElementFilterAccepted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 680, 2560);
    UK2Node_VariableGet* TargetIvHpGet = AddVariableGet(Graph, OverlayTargetIvHpVariableName, -1000, 2880);
    UK2Node_CallArrayFunction* TargetIvHpValue = AddArrayCall(Graph, TEXT("Array_Get"), -720, 2880);
    UK2Node_VariableGet* TargetIvAttackGet = AddVariableGet(Graph, OverlayTargetIvAttackVariableName, -1000, 3040);
    UK2Node_CallArrayFunction* TargetIvAttackValue = AddArrayCall(Graph, TEXT("Array_Get"), -720, 3040);
    UK2Node_VariableGet* TargetIvDefenseGet = AddVariableGet(Graph, OverlayTargetIvDefenseVariableName, -1000, 3200);
    UK2Node_CallArrayFunction* TargetIvDefenseValue = AddArrayCall(Graph, TEXT("Array_Get"), -720, 3200);
    UK2Node_CallFunction* IvHpKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 2880);
    UK2Node_CallFunction* IvAttackKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 3040);
    UK2Node_CallFunction* IvDefenseKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 3200);
    UK2Node_CallFunction* IvHpAttackKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), -160, 2960);
    UK2Node_CallFunction* IvKnown = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 120, 3040);
    UK2Node_CallFunction* IvVisible = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 400, 3040);
    UK2Node_CallFunction* IvHpMinimumDisabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 3360);
    UK2Node_CallFunction* IvAttackMinimumDisabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 3440);
    UK2Node_CallFunction* IvDefenseMinimumDisabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -440, 3520);
    UK2Node_CallFunction* IvHpAttackMinimumDisabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), -160, 3400);
    UK2Node_CallFunction* IvAllMinimumDisabled = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 120, 3440);
    UK2Node_CallFunction* IvHpMeetsMinimum = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 3520);
    UK2Node_CallFunction* IvAttackMeetsMinimum = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 3680);
    UK2Node_CallFunction* IvDefenseMeetsMinimum = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), -440, 3840);
    UK2Node_CallFunction* IvHpAttackMeetsMinimum = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), -160, 3600);
    UK2Node_CallFunction* IvAllMeetMinimum = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 120, 3680);
    UK2Node_CallFunction* IvMinimumAccepted = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 400, 3520);
    UK2Node_VariableGet* TargetPassiveTextsGet = AddVariableGet(Graph, OverlayTargetPassiveTextsVariableName, -1000, 4000);
    UK2Node_CallArrayFunction* TargetPassiveTextGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 4000);
    UK2Node_CallFunction* PassiveTextNotEmpty = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), -440, 4000);
    UK2Node_CallFunction* PassiveVisible = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), -160, 4000);
    UK2Node_VariableGet* TargetPassiveIdTextsGet = AddVariableGet(Graph, OverlayTargetPassiveIdTextsVariableName, -1000, 4160);
    UK2Node_CallArrayFunction* TargetPassiveIdTextGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 4160);
    UK2Node_VariableSet* ResetPassiveFilterMatch = AddVariableSet(Graph, OverlayPassiveFilterMatchVariableName, 1240, 80);
    UK2Node_VariableGet* PassiveFilterIdsGet = AddVariableGet(Graph, OverlayPassiveFilterIdsVariableName, 1240, 240);
    UK2Node_MacroInstance* ForEachPassiveFilterId = AddForEachLoop(Graph, 1500, 80);
    UK2Node_VariableGet* PassiveFilterMatchGet = AddVariableGet(Graph, OverlayPassiveFilterMatchVariableName, 1780, 240);
    UK2Node_CallFunction* PassiveFilterIdToString = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), 1780, 400);
    UK2Node_CallFunction* PassiveFilterIdWithPrefix = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2040, 400);
    UK2Node_CallFunction* PassiveFilterIdWrapped = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2300, 400);
    UK2Node_CallFunction* TargetContainsPassiveId = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Contains"), 2560, 320);
    UK2Node_CallFunction* PassiveFilterMatchAnd = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 2820, 240);
    UK2Node_VariableSet* StorePassiveFilterMatch = AddVariableSet(Graph, OverlayPassiveFilterMatchVariableName, 3080, 80);
    UK2Node_VariableGet* FinalPassiveFilterMatch = AddVariableGet(Graph, OverlayPassiveFilterMatchVariableName, 1780, 560);
    UK2Node_IfThenElse* PassiveFilterBranch = AddBranch(Graph, 2040, 80);
    UK2Node_VariableSet* ResetPassiveExcludeMatch = AddVariableSet(Graph, OverlayPassiveExcludeMatchVariableName, 3340, 80);
    UK2Node_VariableGet* PassiveExcludeIdsGet = AddVariableGet(Graph, OverlayPassiveExcludeIdsVariableName, 3340, 240);
    UK2Node_MacroInstance* ForEachPassiveExcludeId = AddForEachLoop(Graph, 3600, 80);
    UK2Node_VariableGet* PassiveExcludeMatchGet = AddVariableGet(Graph, OverlayPassiveExcludeMatchVariableName, 3880, 240);
    UK2Node_CallFunction* PassiveExcludeIdToString = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), 3880, 400);
    UK2Node_CallFunction* PassiveExcludeIdWithPrefix = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 4140, 400);
    UK2Node_CallFunction* PassiveExcludeIdWrapped = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 4400, 400);
    UK2Node_CallFunction* TargetContainsExcludedPassiveId = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Contains"), 4660, 320);
    UK2Node_CallFunction* TargetDoesNotContainExcludedPassiveId = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 4920, 320);
    UK2Node_CallFunction* PassiveExcludeMatchAnd = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 5180, 240);
    UK2Node_VariableSet* StorePassiveExcludeMatch = AddVariableSet(Graph, OverlayPassiveExcludeMatchVariableName, 5440, 80);
    UK2Node_VariableGet* FinalPassiveExcludeMatch = AddVariableGet(Graph, OverlayPassiveExcludeMatchVariableName, 3880, 560);
    UK2Node_IfThenElse* PassiveExcludeBranch = AddBranch(Graph, 4140, 80);
    // __DEPRECATED_20260717__ [reason: displayed distance now follows live player and target positions]
    // UK2Node_VariableGet* TargetDistancesGet = AddVariableGet(Graph, OverlayTargetDistancesVariableName, -1000, 1200);
    // UK2Node_CallArrayFunction* TargetDistanceGet = AddArrayCall(Graph, TEXT("Array_Get"), -720, 1200);
    UK2Node_CallFunction* BuildLevelText = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), -400, 1040);
    UK2Node_CallFunction* LevelPrefix = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), -680, 1120);
    UK2Node_CallFunction* PlayerPawn = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerPawn"), -720, 1280);
    UK2Node_CallFunction* PlayerLocation = AddStaticCall(Graph, AActor::StaticClass(), TEXT("K2_GetActorLocation"), -400, 1280);
    UK2Node_CallFunction* LiveDistance = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Vector_Distance"), -80, 1280);
    UK2Node_CallFunction* DistanceMeters = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Divide_DoubleDouble"), 200, 1280);
    UK2Node_CallFunction* RoundDistance = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Round"), 480, 1280);
    UK2Node_CallFunction* BuildDistanceText = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), 760, 1280);
    UK2Node_CallFunction* DistanceSuffix = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 480, 1440);
    UK2Node_CallFunction* LevelWithSpacing = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 200, 1040);
    UK2Node_CallFunction* LevelAndDistance = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1040, 1120);
    UK2Node_CallFunction* SelectWithLevel = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1320, 1040);
    UK2Node_CallFunction* SelectWithoutLevel = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1320, 1280);
    UK2Node_CallFunction* SelectLabel = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1600, 1120);
    UK2Node_CallFunction* BuildIvHp = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), 1040, 1440);
    UK2Node_CallFunction* BuildIvAttack = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), 1320, 1440);
    UK2Node_CallFunction* BuildIvDefense = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), 1600, 1440);
    UK2Node_CallFunction* IvHpPrefix = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1040, 1600);
    UK2Node_CallFunction* IvAttackPrefix = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1320, 1600);
    UK2Node_CallFunction* IvDefensePrefix = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1600, 1600);
    UK2Node_CallFunction* IvSeparator = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 1880, 1440);
    UK2Node_CallFunction* MetadataWithSeparator = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2160, 1360);
    UK2Node_CallFunction* MetadataWithIv = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2440, 1360);
    UK2Node_CallFunction* SelectMetadata = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 2720, 1360);
    UK2Node_CallFunction* MetadataNotEmpty = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), 3000, 1360);
    UK2Node_CallFunction* PassiveLineSeparator = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 3280, 1360);
    UK2Node_CallFunction* MetadataWithPassiveSeparator = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 3560, 1360);
    UK2Node_CallFunction* MetadataWithPassive = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 3840, 1360);
    UK2Node_CallFunction* SelectMetadataWithPassive = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 4120, 1360);
    UK2Node_CallFunction* NameWithNewline = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1880, 880);
    UK2Node_CallFunction* NameAndMetadata = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2160, 960);
    UK2Node_CallFunction* SelectNameLabel = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 2440, 960);
    UK2Node_CallFunction* SelectFinalLabel = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 2720, 1040);
    UK2Node_CallFunction* TargetValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -940, 300);
    UK2Node_IfThenElse* TargetBranch = AddBranch(Graph, -700, 80);
    UK2Node_CallFunction* TargetDead = AddStaticCall(Graph, PalUtilityClass, TEXT("IsDead"), -700, 440);
    UK2Node_CallFunction* TargetNotDead = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -440, 440);
    UK2Node_IfThenElse* TargetAliveBranch = AddBranch(Graph, -420, 80);
    UK2Node_CallFunction* LiveCharacterParameter = AddStaticCall(
        Graph, PalMonsterClass, TEXT("GetCharacterParameterComponent"), -940, 520);
    UK2Node_CallFunction* LiveIndividualParameter = AddStaticCall(
        Graph, CharacterParameterComponentClass, TEXT("GetIndividualParameter"), -680, 520);
    UK2Node_CallFunction* LiveParameterValid = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -420, 600);
    UK2Node_IfThenElse* LiveParameterValidBranch = AddBranch(Graph, -160, -80);
    UK2Node_CallFunction* ParameterIdentityMatch = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_ObjectObject"), -160, 600);
    UK2Node_CallFunction* LiveCharacterId = AddStaticCall(
        Graph, IndividualParameterClass, TEXT("GetCharacterID"), 100, 600);
    UK2Node_CallFunction* CharacterIdIdentityMatch = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_NameName"), 360, 600);
    UK2Node_CallFunction* IdentityMatch = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 620, 600);
    UK2Node_IfThenElse* IdentityMatchBranch = AddBranch(Graph, 880, -80);
    // __DEPRECATED_20260721__ [reason: mutating parallel target arrays during ForEachTarget corrupts iteration]
    // UK2Node_Self* RebindSelf = AddSelfNode(Graph, 880, 760);
    // UK2Node_CallFunction* RebindRemoveTarget = AddStaticCall(
    //     Graph, OverlayRuntimeClass, *OverlayRemoveTargetEventName.ToString(), 1140, 680);
    // UK2Node_CallFunction* RebindAddTarget = AddStaticCall(
    //     Graph, OverlayRuntimeClass, *OverlayAddTargetEventName.ToString(), 1400, 680);
    // UK2Node_CallFunction* LiveLevel = AddStaticCall(
    //     Graph, IndividualParameterClass, TEXT("GetLevel"), 1140, 840);
    UK2Node_IfThenElse* SpeciesFilterBranch = AddBranch(Graph, -300, 80);
    UK2Node_IfThenElse* GenderFilterBranch = AddBranch(Graph, -160, 80);
    UK2Node_IfThenElse* LuckyFilterBranch = AddBranch(Graph, 120, 80);
    UK2Node_IfThenElse* BossFilterBranch = AddBranch(Graph, 400, 80);
    UK2Node_IfThenElse* CollectionFilterBranch = AddBranch(Graph, 540, 80);
    UK2Node_IfThenElse* ElementFilterBranch = AddBranch(Graph, 680, 80);
    UK2Node_IfThenElse* IvMinimumBranch = AddBranch(Graph, 960, 80);
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
    UK2Node_CallFunction* MakeLabelOffset = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("MakeVector2D"), 240, 1040);
    UK2Node_CallFunction* LabelPosition = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_Vector2DVector2D"), 520, 1040);
    UK2Node_CallFunction* DrawText = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("DrawText"), 240, 260);
    const TArray<FVector2D> OutlineOffsets = {
        FVector2D(-1.0f, 0.0f),
        FVector2D(1.0f, 0.0f),
        FVector2D(0.0f, -1.0f),
        FVector2D(0.0f, 1.0f),
    };
    UEdGraphNode* OutlineExecTail = LabelEnabledBranch;
    int32 OutlineX = 800;
    for (const FVector2D& Offset : OutlineOffsets) {
        UK2Node_CallFunction* MakeOutlineOffset = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("MakeVector2D"), OutlineX, 560);
        UK2Node_CallFunction* OutlinePosition = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_Vector2DVector2D"), OutlineX + 260, 560);
        UK2Node_CallFunction* DrawOutline = AddStaticCall(
            Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("DrawText"), OutlineX + 520, 260);
        if (!MakeOutlineOffset || !OutlinePosition || !DrawOutline
            || !Link(OutlineExecTail, UEdGraphSchema_K2::PN_Then, DrawOutline, UEdGraphSchema_K2::PN_Execute)
            || !Link(OnPaint, TEXT("Context"), DrawOutline, TEXT("Context"))
            || !Link(SelectFinalLabel, UEdGraphSchema_K2::PN_ReturnValue, DrawOutline, TEXT("InString"))
            || !Link(LabelPosition, UEdGraphSchema_K2::PN_ReturnValue, OutlinePosition, TEXT("A"))
            || !Link(MakeOutlineOffset, UEdGraphSchema_K2::PN_ReturnValue, OutlinePosition, TEXT("B"))
            || !Link(OutlinePosition, UEdGraphSchema_K2::PN_ReturnValue, DrawOutline, TEXT("Position"))
            || !SetPinDefault(MakeOutlineOffset, TEXT("X"), FString::SanitizeFloat(Offset.X))
            || !SetPinDefault(MakeOutlineOffset, TEXT("Y"), FString::SanitizeFloat(Offset.Y))
            || !SetPinDefault(DrawOutline, TEXT("Tint"), TEXT("(R=0.0,G=0.0,B=0.0,A=1.0)"))) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildOverlay text outline graph failed"));
            return false;
        }
        OutlineExecTail = DrawOutline;
        OutlineX += 840;
    }
    if (!OnPaint || !TopGuideEnabledGet || !ShowNameGet || !ShowLevelGet || !ShowDistanceGet || !ShowIvGet
        || !ShowPassiveSkillsGet || !LanguageIdGet || !LanguageIsEnglish
        || !IvMinGet || !IvHpMinGet || !IvAttackMinGet || !IvDefenseMinGet
        || !GenderFilterGet || !LuckyFilterGet || !BossFilterGet || !CollectionFilterGet || !ElementFilterGet
        || !BasicMetadataEnabled || !MetadataEnabled || !AllMetadataEnabled || !VisibleNameEnabled || !LabelsEnabled
        || !PaintSequence || !LabelEnabledBranch || !TopGuideEnabledBranch || !TargetsGet || !ForEachTarget
        || !TargetLevelsGet || !TargetLevelGet || !TargetNamesGet || !TargetNameGet || !NameNotEmpty
        || !TargetCharacterIdsGet || !TargetCharacterIdGet || !TargetParametersGet || !TargetParameterGet
        || !SpeciesFilterIdsGet
        || !SpeciesFilterCount || !SpeciesFilterAll || !SpeciesFilterContains || !SpeciesFilterAccepted
        || !TargetGendersGet || !TargetGenderGet
        || !GenderFilterAll || !GenderFilterMatch || !GenderFilterAccepted
        || !TargetLuckyStatesGet || !TargetLuckyStateGet || !LuckyFilterAll || !LuckyFilterOnly
        || !LuckyFilterExclude || !LuckyFilterRestricted || !LuckyExpectedState || !LuckyFilterMatch
        || !LuckyRestrictedMatch || !LuckyFilterAccepted
        || !TargetBossStatesGet || !TargetBossStateGet || !BossFilterAll || !BossFilterOnly
        || !BossFilterExclude || !BossFilterRestricted || !BossExpectedState || !BossFilterMatch
        || !BossRestrictedMatch || !BossFilterAccepted
        || !TargetCaptureCountsGet || !TargetCaptureCountGet || !CollectionFilterAll || !CollectionFilterComplete
        || !TargetCollectionKnown || !CollectionComplete || !CollectionCompleteState || !CollectionExpectedState
        || !CollectionFilterMatch || !CollectionKnownMatch || !CollectionFilterAccepted
        || !TargetElementMasksGet || !TargetElementMaskGet || !ElementFilterAll || !TargetElementKnown
        || !ElementMaskOverlap || !ElementMaskMatches || !ElementKnownMatch || !ElementFilterAccepted
        || !TargetIvHpGet || !TargetIvHpValue || !TargetIvAttackGet || !TargetIvAttackValue
        || !TargetIvDefenseGet || !TargetIvDefenseValue || !IvHpKnown || !IvAttackKnown || !IvDefenseKnown
        || !IvHpAttackKnown || !IvKnown || !IvVisible
        || !IvHpMinimumDisabled || !IvAttackMinimumDisabled || !IvDefenseMinimumDisabled
        || !IvHpAttackMinimumDisabled || !IvAllMinimumDisabled
        || !IvHpMeetsMinimum || !IvAttackMeetsMinimum || !IvDefenseMeetsMinimum
        || !IvHpAttackMeetsMinimum || !IvAllMeetMinimum || !IvMinimumAccepted
        || !TargetPassiveTextsGet || !TargetPassiveTextGet || !PassiveTextNotEmpty || !PassiveVisible
        || !TargetPassiveIdTextsGet || !TargetPassiveIdTextGet || !ResetPassiveFilterMatch
        || !PassiveFilterIdsGet || !ForEachPassiveFilterId || !PassiveFilterMatchGet
        || !PassiveFilterIdToString || !PassiveFilterIdWithPrefix || !PassiveFilterIdWrapped
        || !TargetContainsPassiveId || !PassiveFilterMatchAnd || !StorePassiveFilterMatch
        || !FinalPassiveFilterMatch || !PassiveFilterBranch
        || !ResetPassiveExcludeMatch || !PassiveExcludeIdsGet || !ForEachPassiveExcludeId
        || !PassiveExcludeMatchGet || !PassiveExcludeIdToString || !PassiveExcludeIdWithPrefix
        || !PassiveExcludeIdWrapped || !TargetContainsExcludedPassiveId
        || !TargetDoesNotContainExcludedPassiveId || !PassiveExcludeMatchAnd
        || !StorePassiveExcludeMatch || !FinalPassiveExcludeMatch || !PassiveExcludeBranch
        || !BuildLevelText || !LevelPrefix || !PlayerPawn || !PlayerLocation
        || !LiveDistance || !DistanceMeters || !RoundDistance || !BuildDistanceText || !DistanceSuffix || !LevelWithSpacing
        || !LevelAndDistance || !SelectWithLevel || !SelectWithoutLevel || !SelectLabel
        || !BuildIvHp || !BuildIvAttack || !BuildIvDefense
        || !IvHpPrefix || !IvAttackPrefix || !IvDefensePrefix || !IvSeparator
        || !MetadataWithSeparator || !MetadataWithIv || !SelectMetadata || !MetadataNotEmpty
        || !PassiveLineSeparator || !MetadataWithPassiveSeparator || !MetadataWithPassive || !SelectMetadataWithPassive
        || !NameWithNewline || !NameAndMetadata || !SelectNameLabel || !SelectFinalLabel
        || !TargetValid || !TargetBranch || !TargetDead || !TargetNotDead
        || !TargetAliveBranch || !LiveCharacterParameter || !LiveIndividualParameter || !LiveParameterValid
        || !LiveParameterValidBranch || !ParameterIdentityMatch || !LiveCharacterId
        || !CharacterIdIdentityMatch || !IdentityMatch || !IdentityMatchBranch
        // __DEPRECATED_20260721__ [reason: identity mismatch now fails closed until Lua atomically rebuilds]
        // || !RebindSelf || !RebindRemoveTarget || !RebindAddTarget || !LiveLevel
        || !SpeciesFilterBranch || !GenderFilterBranch || !LuckyFilterBranch || !BossFilterBranch
        || !CollectionFilterBranch || !ElementFilterBranch
        || !IvMinimumBranch || !PassiveFilterBranch
        || !ActorLocation || !Self || !PlayerController
        || !Project || !ProjectBranch || !ViewportSize || !ViewportScale || !BreakViewport || !RemoveScale
        || !HalfWidth || !MakeStart || !DrawLine || !MakeLabelOffset || !LabelPosition || !DrawText
        || !Link(OnPaint, UEdGraphSchema_K2::PN_Then, ForEachTarget, TEXT("Exec"))
        || !Link(LanguageIdGet, OverlayLanguageIdVariableName, LanguageIsEnglish, TEXT("A"))
        || !SetPinDefault(LanguageIsEnglish, TEXT("B"), TEXT("1"))
        || !Link(TopGuideEnabledGet, OverlayTopGuideEnabledVariableName, TopGuideEnabledBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ShowLevelGet, OverlayShowLevelVariableName, BasicMetadataEnabled, TEXT("A"))
        || !Link(ShowDistanceGet, OverlayShowDistanceVariableName, BasicMetadataEnabled, TEXT("B"))
        || !Link(BasicMetadataEnabled, UEdGraphSchema_K2::PN_ReturnValue, MetadataEnabled, TEXT("A"))
        || !Link(IvVisible, UEdGraphSchema_K2::PN_ReturnValue, MetadataEnabled, TEXT("B"))
        || !Link(MetadataEnabled, UEdGraphSchema_K2::PN_ReturnValue, AllMetadataEnabled, TEXT("A"))
        || !Link(PassiveVisible, UEdGraphSchema_K2::PN_ReturnValue, AllMetadataEnabled, TEXT("B"))
        || !Link(ShowNameGet, OverlayShowNameVariableName, VisibleNameEnabled, TEXT("A"))
        || !Link(NameNotEmpty, UEdGraphSchema_K2::PN_ReturnValue, VisibleNameEnabled, TEXT("B"))
        || !Link(VisibleNameEnabled, UEdGraphSchema_K2::PN_ReturnValue, LabelsEnabled, TEXT("A"))
        || !Link(AllMetadataEnabled, UEdGraphSchema_K2::PN_ReturnValue, LabelsEnabled, TEXT("B"))
        || !Link(LabelsEnabled, UEdGraphSchema_K2::PN_ReturnValue, LabelEnabledBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetsGet, OverlayTargetsVariableName, ForEachTarget, TEXT("Array"))
        || !Link(TargetNamesGet, OverlayTargetNamesVariableName, TargetNameGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetNameGet, TEXT("Index"))
        || !Link(TargetNameGet, TEXT("Item"), NameNotEmpty, TEXT("A"))
        || !SetPinDefault(NameNotEmpty, TEXT("B"), TEXT(""))
        || !Link(TargetLevelsGet, OverlayTargetLevelsVariableName, TargetLevelGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetLevelGet, TEXT("Index"))
        || !Link(TargetLevelGet, TEXT("Item"), BuildLevelText, TEXT("InInt"))
        || !SetPinDefault(LevelPrefix, TEXT("A"), TEXT("Lv."))
        || !SetPinDefault(LevelPrefix, TEXT("B"), TEXT("等级 "))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, LevelPrefix, TEXT("bPickA"))
        || !Link(LevelPrefix, UEdGraphSchema_K2::PN_ReturnValue, BuildLevelText, TEXT("Prefix"))
        || !Link(TargetCharacterIdsGet, OverlayTargetCharacterIdsVariableName, TargetCharacterIdGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetCharacterIdGet, TEXT("Index"))
        || !Link(TargetParametersGet, OverlayTargetParametersVariableName, TargetParameterGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetParameterGet, TEXT("Index"))
        || !Link(SpeciesFilterIdsGet, OverlaySpeciesFilterIdsVariableName, SpeciesFilterCount, TEXT("TargetArray"))
        || !Link(SpeciesFilterCount, UEdGraphSchema_K2::PN_ReturnValue, SpeciesFilterAll, TEXT("A"))
        || !SetPinDefault(SpeciesFilterAll, TEXT("B"), TEXT("0"))
        || !Link(SpeciesFilterIdsGet, OverlaySpeciesFilterIdsVariableName, SpeciesFilterContains, TEXT("TargetArray"))
        || !Link(TargetCharacterIdGet, TEXT("Item"), SpeciesFilterContains, TEXT("ItemToFind"))
        || !Link(SpeciesFilterAll, UEdGraphSchema_K2::PN_ReturnValue, SpeciesFilterAccepted, TEXT("A"))
        || !Link(SpeciesFilterContains, UEdGraphSchema_K2::PN_ReturnValue, SpeciesFilterAccepted, TEXT("B"))
        || !Link(SpeciesFilterAccepted, UEdGraphSchema_K2::PN_ReturnValue, SpeciesFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetGendersGet, OverlayTargetGendersVariableName, TargetGenderGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetGenderGet, TEXT("Index"))
        || !Link(GenderFilterGet, OverlayGenderFilterIdVariableName, GenderFilterAll, TEXT("A"))
        || !SetPinDefault(GenderFilterAll, TEXT("B"), TEXT("0"))
        || !Link(GenderFilterGet, OverlayGenderFilterIdVariableName, GenderFilterMatch, TEXT("A"))
        || !Link(TargetGenderGet, TEXT("Item"), GenderFilterMatch, TEXT("B"))
        || !Link(GenderFilterAll, UEdGraphSchema_K2::PN_ReturnValue, GenderFilterAccepted, TEXT("A"))
        || !Link(GenderFilterMatch, UEdGraphSchema_K2::PN_ReturnValue, GenderFilterAccepted, TEXT("B"))
        || !Link(GenderFilterAccepted, UEdGraphSchema_K2::PN_ReturnValue, GenderFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetLuckyStatesGet, OverlayTargetLuckyStatesVariableName, TargetLuckyStateGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetLuckyStateGet, TEXT("Index"))
        || !Link(LuckyFilterGet, OverlayLuckyFilterIdVariableName, LuckyFilterAll, TEXT("A"))
        || !SetPinDefault(LuckyFilterAll, TEXT("B"), TEXT("0"))
        || !Link(LuckyFilterGet, OverlayLuckyFilterIdVariableName, LuckyFilterOnly, TEXT("A"))
        || !SetPinDefault(LuckyFilterOnly, TEXT("B"), TEXT("1"))
        || !Link(LuckyFilterGet, OverlayLuckyFilterIdVariableName, LuckyFilterExclude, TEXT("A"))
        || !SetPinDefault(LuckyFilterExclude, TEXT("B"), TEXT("2"))
        || !Link(LuckyFilterOnly, UEdGraphSchema_K2::PN_ReturnValue, LuckyFilterRestricted, TEXT("A"))
        || !Link(LuckyFilterExclude, UEdGraphSchema_K2::PN_ReturnValue, LuckyFilterRestricted, TEXT("B"))
        || !SetPinDefault(LuckyExpectedState, TEXT("A"), TEXT("1"))
        || !SetPinDefault(LuckyExpectedState, TEXT("B"), TEXT("0"))
        || !Link(LuckyFilterOnly, UEdGraphSchema_K2::PN_ReturnValue, LuckyExpectedState, TEXT("bPickA"))
        || !Link(TargetLuckyStateGet, TEXT("Item"), LuckyFilterMatch, TEXT("A"))
        || !Link(LuckyExpectedState, UEdGraphSchema_K2::PN_ReturnValue, LuckyFilterMatch, TEXT("B"))
        || !Link(LuckyFilterRestricted, UEdGraphSchema_K2::PN_ReturnValue, LuckyRestrictedMatch, TEXT("A"))
        || !Link(LuckyFilterMatch, UEdGraphSchema_K2::PN_ReturnValue, LuckyRestrictedMatch, TEXT("B"))
        || !Link(LuckyFilterAll, UEdGraphSchema_K2::PN_ReturnValue, LuckyFilterAccepted, TEXT("A"))
        || !Link(LuckyRestrictedMatch, UEdGraphSchema_K2::PN_ReturnValue, LuckyFilterAccepted, TEXT("B"))
        || !Link(LuckyFilterAccepted, UEdGraphSchema_K2::PN_ReturnValue, LuckyFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetBossStatesGet, OverlayTargetBossStatesVariableName, TargetBossStateGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetBossStateGet, TEXT("Index"))
        || !Link(BossFilterGet, OverlayBossFilterIdVariableName, BossFilterAll, TEXT("A"))
        || !SetPinDefault(BossFilterAll, TEXT("B"), TEXT("0"))
        || !Link(BossFilterGet, OverlayBossFilterIdVariableName, BossFilterOnly, TEXT("A"))
        || !SetPinDefault(BossFilterOnly, TEXT("B"), TEXT("1"))
        || !Link(BossFilterGet, OverlayBossFilterIdVariableName, BossFilterExclude, TEXT("A"))
        || !SetPinDefault(BossFilterExclude, TEXT("B"), TEXT("2"))
        || !Link(BossFilterOnly, UEdGraphSchema_K2::PN_ReturnValue, BossFilterRestricted, TEXT("A"))
        || !Link(BossFilterExclude, UEdGraphSchema_K2::PN_ReturnValue, BossFilterRestricted, TEXT("B"))
        || !SetPinDefault(BossExpectedState, TEXT("A"), TEXT("1"))
        || !SetPinDefault(BossExpectedState, TEXT("B"), TEXT("0"))
        || !Link(BossFilterOnly, UEdGraphSchema_K2::PN_ReturnValue, BossExpectedState, TEXT("bPickA"))
        || !Link(TargetBossStateGet, TEXT("Item"), BossFilterMatch, TEXT("A"))
        || !Link(BossExpectedState, UEdGraphSchema_K2::PN_ReturnValue, BossFilterMatch, TEXT("B"))
        || !Link(BossFilterRestricted, UEdGraphSchema_K2::PN_ReturnValue, BossRestrictedMatch, TEXT("A"))
        || !Link(BossFilterMatch, UEdGraphSchema_K2::PN_ReturnValue, BossRestrictedMatch, TEXT("B"))
        || !Link(BossFilterAll, UEdGraphSchema_K2::PN_ReturnValue, BossFilterAccepted, TEXT("A"))
        || !Link(BossRestrictedMatch, UEdGraphSchema_K2::PN_ReturnValue, BossFilterAccepted, TEXT("B"))
        || !Link(BossFilterAccepted, UEdGraphSchema_K2::PN_ReturnValue, BossFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetCaptureCountsGet, OverlayTargetCaptureCountsVariableName, TargetCaptureCountGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetCaptureCountGet, TEXT("Index"))
        || !Link(CollectionFilterGet, OverlayCollectionFilterIdVariableName, CollectionFilterAll, TEXT("A"))
        || !SetPinDefault(CollectionFilterAll, TEXT("B"), TEXT("0"))
        || !Link(CollectionFilterGet, OverlayCollectionFilterIdVariableName, CollectionFilterComplete, TEXT("A"))
        || !SetPinDefault(CollectionFilterComplete, TEXT("B"), TEXT("2"))
        || !Link(TargetCaptureCountGet, TEXT("Item"), TargetCollectionKnown, TEXT("A"))
        || !SetPinDefault(TargetCollectionKnown, TEXT("B"), TEXT("0"))
        || !Link(TargetCaptureCountGet, TEXT("Item"), CollectionComplete, TEXT("A"))
        || !SetPinDefault(CollectionComplete, TEXT("B"), TEXT("5"))
        || !SetPinDefault(CollectionCompleteState, TEXT("A"), TEXT("1"))
        || !SetPinDefault(CollectionCompleteState, TEXT("B"), TEXT("0"))
        || !Link(CollectionComplete, UEdGraphSchema_K2::PN_ReturnValue, CollectionCompleteState, TEXT("bPickA"))
        || !SetPinDefault(CollectionExpectedState, TEXT("A"), TEXT("1"))
        || !SetPinDefault(CollectionExpectedState, TEXT("B"), TEXT("0"))
        || !Link(CollectionFilterComplete, UEdGraphSchema_K2::PN_ReturnValue, CollectionExpectedState, TEXT("bPickA"))
        || !Link(CollectionCompleteState, UEdGraphSchema_K2::PN_ReturnValue, CollectionFilterMatch, TEXT("A"))
        || !Link(CollectionExpectedState, UEdGraphSchema_K2::PN_ReturnValue, CollectionFilterMatch, TEXT("B"))
        || !Link(TargetCollectionKnown, UEdGraphSchema_K2::PN_ReturnValue, CollectionKnownMatch, TEXT("A"))
        || !Link(CollectionFilterMatch, UEdGraphSchema_K2::PN_ReturnValue, CollectionKnownMatch, TEXT("B"))
        || !Link(CollectionFilterAll, UEdGraphSchema_K2::PN_ReturnValue, CollectionFilterAccepted, TEXT("A"))
        || !Link(CollectionKnownMatch, UEdGraphSchema_K2::PN_ReturnValue, CollectionFilterAccepted, TEXT("B"))
        || !Link(CollectionFilterAccepted, UEdGraphSchema_K2::PN_ReturnValue, CollectionFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetElementMasksGet, OverlayTargetElementMasksVariableName, TargetElementMaskGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetElementMaskGet, TEXT("Index"))
        || !Link(ElementFilterGet, OverlayElementFilterMaskVariableName, ElementFilterAll, TEXT("A"))
        || !SetPinDefault(ElementFilterAll, TEXT("B"), TEXT("0"))
        || !Link(TargetElementMaskGet, TEXT("Item"), TargetElementKnown, TEXT("A"))
        || !SetPinDefault(TargetElementKnown, TEXT("B"), TEXT("0"))
        || !Link(TargetElementMaskGet, TEXT("Item"), ElementMaskOverlap, TEXT("A"))
        || !Link(ElementFilterGet, OverlayElementFilterMaskVariableName, ElementMaskOverlap, TEXT("B"))
        || !Link(ElementMaskOverlap, UEdGraphSchema_K2::PN_ReturnValue, ElementMaskMatches, TEXT("A"))
        || !SetPinDefault(ElementMaskMatches, TEXT("B"), TEXT("0"))
        || !Link(TargetElementKnown, UEdGraphSchema_K2::PN_ReturnValue, ElementKnownMatch, TEXT("A"))
        || !Link(ElementMaskMatches, UEdGraphSchema_K2::PN_ReturnValue, ElementKnownMatch, TEXT("B"))
        || !Link(ElementFilterAll, UEdGraphSchema_K2::PN_ReturnValue, ElementFilterAccepted, TEXT("A"))
        || !Link(ElementKnownMatch, UEdGraphSchema_K2::PN_ReturnValue, ElementFilterAccepted, TEXT("B"))
        || !Link(ElementFilterAccepted, UEdGraphSchema_K2::PN_ReturnValue, ElementFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetIvHpGet, OverlayTargetIvHpVariableName, TargetIvHpValue, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetIvHpValue, TEXT("Index"))
        || !Link(TargetIvAttackGet, OverlayTargetIvAttackVariableName, TargetIvAttackValue, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetIvAttackValue, TEXT("Index"))
        || !Link(TargetIvDefenseGet, OverlayTargetIvDefenseVariableName, TargetIvDefenseValue, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetIvDefenseValue, TEXT("Index"))
        || !Link(TargetIvHpValue, TEXT("Item"), IvHpKnown, TEXT("A"))
        || !SetPinDefault(IvHpKnown, TEXT("B"), TEXT("0"))
        || !Link(TargetIvAttackValue, TEXT("Item"), IvAttackKnown, TEXT("A"))
        || !SetPinDefault(IvAttackKnown, TEXT("B"), TEXT("0"))
        || !Link(TargetIvDefenseValue, TEXT("Item"), IvDefenseKnown, TEXT("A"))
        || !SetPinDefault(IvDefenseKnown, TEXT("B"), TEXT("0"))
        || !Link(IvHpKnown, UEdGraphSchema_K2::PN_ReturnValue, IvHpAttackKnown, TEXT("A"))
        || !Link(IvAttackKnown, UEdGraphSchema_K2::PN_ReturnValue, IvHpAttackKnown, TEXT("B"))
        || !Link(IvHpAttackKnown, UEdGraphSchema_K2::PN_ReturnValue, IvKnown, TEXT("A"))
        || !Link(IvDefenseKnown, UEdGraphSchema_K2::PN_ReturnValue, IvKnown, TEXT("B"))
        || !Link(ShowIvGet, OverlayShowIvVariableName, IvVisible, TEXT("A"))
        || !Link(IvKnown, UEdGraphSchema_K2::PN_ReturnValue, IvVisible, TEXT("B"))
        || !Link(IvHpMinGet, OverlayIvHpMinVariableName, IvHpMinimumDisabled, TEXT("A"))
        || !SetPinDefault(IvHpMinimumDisabled, TEXT("B"), TEXT("0"))
        || !Link(IvAttackMinGet, OverlayIvAttackMinVariableName, IvAttackMinimumDisabled, TEXT("A"))
        || !SetPinDefault(IvAttackMinimumDisabled, TEXT("B"), TEXT("0"))
        || !Link(IvDefenseMinGet, OverlayIvDefenseMinVariableName, IvDefenseMinimumDisabled, TEXT("A"))
        || !SetPinDefault(IvDefenseMinimumDisabled, TEXT("B"), TEXT("0"))
        || !Link(IvHpMinimumDisabled, UEdGraphSchema_K2::PN_ReturnValue, IvHpAttackMinimumDisabled, TEXT("A"))
        || !Link(IvAttackMinimumDisabled, UEdGraphSchema_K2::PN_ReturnValue, IvHpAttackMinimumDisabled, TEXT("B"))
        || !Link(IvHpAttackMinimumDisabled, UEdGraphSchema_K2::PN_ReturnValue, IvAllMinimumDisabled, TEXT("A"))
        || !Link(IvDefenseMinimumDisabled, UEdGraphSchema_K2::PN_ReturnValue, IvAllMinimumDisabled, TEXT("B"))
        || !Link(TargetIvHpValue, TEXT("Item"), IvHpMeetsMinimum, TEXT("A"))
        || !Link(IvHpMinGet, OverlayIvHpMinVariableName, IvHpMeetsMinimum, TEXT("B"))
        || !Link(TargetIvAttackValue, TEXT("Item"), IvAttackMeetsMinimum, TEXT("A"))
        || !Link(IvAttackMinGet, OverlayIvAttackMinVariableName, IvAttackMeetsMinimum, TEXT("B"))
        || !Link(TargetIvDefenseValue, TEXT("Item"), IvDefenseMeetsMinimum, TEXT("A"))
        || !Link(IvDefenseMinGet, OverlayIvDefenseMinVariableName, IvDefenseMeetsMinimum, TEXT("B"))
        || !Link(IvHpMeetsMinimum, UEdGraphSchema_K2::PN_ReturnValue, IvHpAttackMeetsMinimum, TEXT("A"))
        || !Link(IvAttackMeetsMinimum, UEdGraphSchema_K2::PN_ReturnValue, IvHpAttackMeetsMinimum, TEXT("B"))
        || !Link(IvHpAttackMeetsMinimum, UEdGraphSchema_K2::PN_ReturnValue, IvAllMeetMinimum, TEXT("A"))
        || !Link(IvDefenseMeetsMinimum, UEdGraphSchema_K2::PN_ReturnValue, IvAllMeetMinimum, TEXT("B"))
        || !Link(IvAllMinimumDisabled, UEdGraphSchema_K2::PN_ReturnValue, IvMinimumAccepted, TEXT("A"))
        || !Link(IvAllMeetMinimum, UEdGraphSchema_K2::PN_ReturnValue, IvMinimumAccepted, TEXT("B"))
        || !Link(TargetPassiveTextsGet, OverlayTargetPassiveTextsVariableName, TargetPassiveTextGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetPassiveTextGet, TEXT("Index"))
        || !Link(TargetPassiveTextGet, TEXT("Item"), PassiveTextNotEmpty, TEXT("A"))
        || !SetPinDefault(PassiveTextNotEmpty, TEXT("B"), TEXT(""))
        || !Link(ShowPassiveSkillsGet, OverlayShowPassiveSkillsVariableName, PassiveVisible, TEXT("A"))
        || !Link(PassiveTextNotEmpty, UEdGraphSchema_K2::PN_ReturnValue, PassiveVisible, TEXT("B"))
        || !Link(TargetPassiveIdTextsGet, OverlayTargetPassiveIdTextsVariableName, TargetPassiveIdTextGet, TEXT("TargetArray"))
        || !Link(ForEachTarget, TEXT("Array Index"), TargetPassiveIdTextGet, TEXT("Index"))
        || !SetPinDefault(ResetPassiveFilterMatch, OverlayPassiveFilterMatchVariableName, TEXT("true"))
        || !Link(PassiveFilterIdsGet, OverlayPassiveFilterIdsVariableName, ForEachPassiveFilterId, TEXT("Array"))
        || !Link(ForEachPassiveFilterId, TEXT("LoopBody"), StorePassiveFilterMatch, UEdGraphSchema_K2::PN_Execute)
        || !Link(PassiveFilterMatchGet, OverlayPassiveFilterMatchVariableName, PassiveFilterMatchAnd, TEXT("A"))
        || !Link(ForEachPassiveFilterId, TEXT("Array Element"), PassiveFilterIdToString, TEXT("InName"))
        || !SetPinDefault(PassiveFilterIdWithPrefix, TEXT("A"), TEXT("|"))
        || !Link(PassiveFilterIdToString, UEdGraphSchema_K2::PN_ReturnValue, PassiveFilterIdWithPrefix, TEXT("B"))
        || !Link(PassiveFilterIdWithPrefix, UEdGraphSchema_K2::PN_ReturnValue, PassiveFilterIdWrapped, TEXT("A"))
        || !SetPinDefault(PassiveFilterIdWrapped, TEXT("B"), TEXT("|"))
        || !Link(TargetPassiveIdTextGet, TEXT("Item"), TargetContainsPassiveId, TEXT("SearchIn"))
        || !Link(PassiveFilterIdWrapped, UEdGraphSchema_K2::PN_ReturnValue, TargetContainsPassiveId, TEXT("Substring"))
        || !SetPinDefault(TargetContainsPassiveId, TEXT("bUseCase"), TEXT("true"))
        || !SetPinDefault(TargetContainsPassiveId, TEXT("bSearchFromEnd"), TEXT("false"))
        || !Link(TargetContainsPassiveId, UEdGraphSchema_K2::PN_ReturnValue, PassiveFilterMatchAnd, TEXT("B"))
        || !Link(PassiveFilterMatchAnd, UEdGraphSchema_K2::PN_ReturnValue, StorePassiveFilterMatch, OverlayPassiveFilterMatchVariableName)
        || !Link(FinalPassiveFilterMatch, OverlayPassiveFilterMatchVariableName, PassiveFilterBranch, UEdGraphSchema_K2::PN_Condition)
        || !SetPinDefault(ResetPassiveExcludeMatch, OverlayPassiveExcludeMatchVariableName, TEXT("true"))
        || !Link(PassiveExcludeIdsGet, OverlayPassiveExcludeIdsVariableName, ForEachPassiveExcludeId, TEXT("Array"))
        || !Link(ForEachPassiveExcludeId, TEXT("LoopBody"), StorePassiveExcludeMatch, UEdGraphSchema_K2::PN_Execute)
        || !Link(PassiveExcludeMatchGet, OverlayPassiveExcludeMatchVariableName, PassiveExcludeMatchAnd, TEXT("A"))
        || !Link(ForEachPassiveExcludeId, TEXT("Array Element"), PassiveExcludeIdToString, TEXT("InName"))
        || !SetPinDefault(PassiveExcludeIdWithPrefix, TEXT("A"), TEXT("|"))
        || !Link(PassiveExcludeIdToString, UEdGraphSchema_K2::PN_ReturnValue, PassiveExcludeIdWithPrefix, TEXT("B"))
        || !Link(PassiveExcludeIdWithPrefix, UEdGraphSchema_K2::PN_ReturnValue, PassiveExcludeIdWrapped, TEXT("A"))
        || !SetPinDefault(PassiveExcludeIdWrapped, TEXT("B"), TEXT("|"))
        || !Link(TargetPassiveIdTextGet, TEXT("Item"), TargetContainsExcludedPassiveId, TEXT("SearchIn"))
        || !Link(PassiveExcludeIdWrapped, UEdGraphSchema_K2::PN_ReturnValue, TargetContainsExcludedPassiveId, TEXT("Substring"))
        || !SetPinDefault(TargetContainsExcludedPassiveId, TEXT("bUseCase"), TEXT("true"))
        || !SetPinDefault(TargetContainsExcludedPassiveId, TEXT("bSearchFromEnd"), TEXT("false"))
        || !Link(TargetContainsExcludedPassiveId, UEdGraphSchema_K2::PN_ReturnValue, TargetDoesNotContainExcludedPassiveId, TEXT("A"))
        || !Link(TargetDoesNotContainExcludedPassiveId, UEdGraphSchema_K2::PN_ReturnValue, PassiveExcludeMatchAnd, TEXT("B"))
        || !Link(PassiveExcludeMatchAnd, UEdGraphSchema_K2::PN_ReturnValue, StorePassiveExcludeMatch, OverlayPassiveExcludeMatchVariableName)
        || !Link(FinalPassiveExcludeMatch, OverlayPassiveExcludeMatchVariableName, PassiveExcludeBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(Self, UEdGraphSchema_K2::PN_Self, PlayerPawn, TEXT("WorldContextObject"))
        || !Link(PlayerPawn, UEdGraphSchema_K2::PN_ReturnValue, PlayerLocation, UEdGraphSchema_K2::PN_Self)
        || !Link(ActorLocation, UEdGraphSchema_K2::PN_ReturnValue, LiveDistance, TEXT("V1"))
        || !Link(PlayerLocation, UEdGraphSchema_K2::PN_ReturnValue, LiveDistance, TEXT("V2"))
        || !Link(LiveDistance, UEdGraphSchema_K2::PN_ReturnValue, DistanceMeters, TEXT("A"))
        || !Link(DistanceMeters, UEdGraphSchema_K2::PN_ReturnValue, RoundDistance, TEXT("A"))
        || !Link(RoundDistance, UEdGraphSchema_K2::PN_ReturnValue, BuildDistanceText, TEXT("InInt"))
        || !SetPinDefault(DistanceSuffix, TEXT("A"), TEXT("m"))
        || !SetPinDefault(DistanceSuffix, TEXT("B"), TEXT("米"))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, DistanceSuffix, TEXT("bPickA"))
        || !Link(DistanceSuffix, UEdGraphSchema_K2::PN_ReturnValue, BuildDistanceText, TEXT("Suffix"))
        || !Link(BuildLevelText, UEdGraphSchema_K2::PN_ReturnValue, LevelWithSpacing, TEXT("A"))
        || !Link(LevelWithSpacing, UEdGraphSchema_K2::PN_ReturnValue, LevelAndDistance, TEXT("A"))
        || !Link(BuildDistanceText, UEdGraphSchema_K2::PN_ReturnValue, LevelAndDistance, TEXT("B"))
        || !Link(LevelAndDistance, UEdGraphSchema_K2::PN_ReturnValue, SelectWithLevel, TEXT("A"))
        || !Link(BuildLevelText, UEdGraphSchema_K2::PN_ReturnValue, SelectWithLevel, TEXT("B"))
        || !Link(ShowDistanceGet, OverlayShowDistanceVariableName, SelectWithLevel, TEXT("bPickA"))
        || !Link(BuildDistanceText, UEdGraphSchema_K2::PN_ReturnValue, SelectWithoutLevel, TEXT("A"))
        || !Link(ShowDistanceGet, OverlayShowDistanceVariableName, SelectWithoutLevel, TEXT("bPickA"))
        || !Link(SelectWithLevel, UEdGraphSchema_K2::PN_ReturnValue, SelectLabel, TEXT("A"))
        || !Link(SelectWithoutLevel, UEdGraphSchema_K2::PN_ReturnValue, SelectLabel, TEXT("B"))
        || !Link(ShowLevelGet, OverlayShowLevelVariableName, SelectLabel, TEXT("bPickA"))
        || !Link(TargetIvHpValue, TEXT("Item"), BuildIvHp, TEXT("InInt"))
        || !SetPinDefault(IvHpPrefix, TEXT("A"), TEXT("IV HP "))
        || !SetPinDefault(IvHpPrefix, TEXT("B"), TEXT("个体值 生命 "))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, IvHpPrefix, TEXT("bPickA"))
        || !Link(IvHpPrefix, UEdGraphSchema_K2::PN_ReturnValue, BuildIvHp, TEXT("Prefix"))
        || !Link(BuildIvHp, UEdGraphSchema_K2::PN_ReturnValue, BuildIvAttack, TEXT("AppendTo"))
        || !Link(TargetIvAttackValue, TEXT("Item"), BuildIvAttack, TEXT("InInt"))
        || !SetPinDefault(IvAttackPrefix, TEXT("A"), TEXT(" / ATK "))
        || !SetPinDefault(IvAttackPrefix, TEXT("B"), TEXT(" / 攻击 "))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, IvAttackPrefix, TEXT("bPickA"))
        || !Link(IvAttackPrefix, UEdGraphSchema_K2::PN_ReturnValue, BuildIvAttack, TEXT("Prefix"))
        || !Link(BuildIvAttack, UEdGraphSchema_K2::PN_ReturnValue, BuildIvDefense, TEXT("AppendTo"))
        || !Link(TargetIvDefenseValue, TEXT("Item"), BuildIvDefense, TEXT("InInt"))
        || !SetPinDefault(IvDefensePrefix, TEXT("A"), TEXT(" / DEF "))
        || !SetPinDefault(IvDefensePrefix, TEXT("B"), TEXT(" / 防御 "))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, IvDefensePrefix, TEXT("bPickA"))
        || !Link(IvDefensePrefix, UEdGraphSchema_K2::PN_ReturnValue, BuildIvDefense, TEXT("Prefix"))
        || !Link(BasicMetadataEnabled, UEdGraphSchema_K2::PN_ReturnValue, IvSeparator, TEXT("bPickA"))
        || !Link(SelectLabel, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithSeparator, TEXT("A"))
        || !Link(IvSeparator, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithSeparator, TEXT("B"))
        || !Link(MetadataWithSeparator, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithIv, TEXT("A"))
        || !Link(BuildIvDefense, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithIv, TEXT("B"))
        || !Link(MetadataWithIv, UEdGraphSchema_K2::PN_ReturnValue, SelectMetadata, TEXT("A"))
        || !Link(SelectLabel, UEdGraphSchema_K2::PN_ReturnValue, SelectMetadata, TEXT("B"))
        || !Link(IvVisible, UEdGraphSchema_K2::PN_ReturnValue, SelectMetadata, TEXT("bPickA"))
        || !Link(SelectMetadata, UEdGraphSchema_K2::PN_ReturnValue, MetadataNotEmpty, TEXT("A"))
        || !SetPinDefault(MetadataNotEmpty, TEXT("B"), TEXT(""))
        || !SetPinDefault(PassiveLineSeparator, TEXT("A"), TEXT("\n"))
        || !SetPinDefault(PassiveLineSeparator, TEXT("B"), TEXT(""))
        || !Link(MetadataNotEmpty, UEdGraphSchema_K2::PN_ReturnValue, PassiveLineSeparator, TEXT("bPickA"))
        || !Link(SelectMetadata, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithPassiveSeparator, TEXT("A"))
        || !Link(PassiveLineSeparator, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithPassiveSeparator, TEXT("B"))
        || !Link(MetadataWithPassiveSeparator, UEdGraphSchema_K2::PN_ReturnValue, MetadataWithPassive, TEXT("A"))
        || !Link(TargetPassiveTextGet, TEXT("Item"), MetadataWithPassive, TEXT("B"))
        || !Link(MetadataWithPassive, UEdGraphSchema_K2::PN_ReturnValue, SelectMetadataWithPassive, TEXT("A"))
        || !Link(SelectMetadata, UEdGraphSchema_K2::PN_ReturnValue, SelectMetadataWithPassive, TEXT("B"))
        || !Link(PassiveVisible, UEdGraphSchema_K2::PN_ReturnValue, SelectMetadataWithPassive, TEXT("bPickA"))
        || !Link(TargetNameGet, TEXT("Item"), NameWithNewline, TEXT("A"))
        || !SetPinDefault(NameWithNewline, TEXT("B"), TEXT("\n"))
        || !Link(NameWithNewline, UEdGraphSchema_K2::PN_ReturnValue, NameAndMetadata, TEXT("A"))
        || !Link(SelectMetadataWithPassive, UEdGraphSchema_K2::PN_ReturnValue, NameAndMetadata, TEXT("B"))
        || !Link(NameAndMetadata, UEdGraphSchema_K2::PN_ReturnValue, SelectNameLabel, TEXT("A"))
        || !Link(TargetNameGet, TEXT("Item"), SelectNameLabel, TEXT("B"))
        || !Link(MetadataEnabled, UEdGraphSchema_K2::PN_ReturnValue, SelectNameLabel, TEXT("bPickA"))
        || !Link(SelectNameLabel, UEdGraphSchema_K2::PN_ReturnValue, SelectFinalLabel, TEXT("A"))
        || !Link(SelectMetadataWithPassive, UEdGraphSchema_K2::PN_ReturnValue, SelectFinalLabel, TEXT("B"))
        || !Link(VisibleNameEnabled, UEdGraphSchema_K2::PN_ReturnValue, SelectFinalLabel, TEXT("bPickA"))
        // __DEPRECATED_20260716__ [reason: UE 5.1 StandardMacros exposes this pin as LoopBody]
        // || !Link(ForEachTarget, TEXT("Loop Body"), TargetBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("LoopBody"), TargetBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("Array Element"), TargetValid, TEXT("Object"))
        || !Link(TargetValid, UEdGraphSchema_K2::PN_ReturnValue, TargetBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(TargetBranch, UEdGraphSchema_K2::PN_Then, TargetAliveBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("Array Element"), TargetDead, TEXT("Actor"))
        || !Link(TargetDead, UEdGraphSchema_K2::PN_ReturnValue, TargetNotDead, TEXT("A"))
        || !Link(TargetNotDead, UEdGraphSchema_K2::PN_ReturnValue, TargetAliveBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ForEachTarget, TEXT("Array Element"), LiveCharacterParameter, UEdGraphSchema_K2::PN_Self)
        || !Link(LiveCharacterParameter, UEdGraphSchema_K2::PN_ReturnValue, LiveIndividualParameter, UEdGraphSchema_K2::PN_Self)
        || !Link(LiveIndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, LiveParameterValid, TEXT("Object"))
        || !Link(LiveParameterValid, UEdGraphSchema_K2::PN_ReturnValue, LiveParameterValidBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(LiveIndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, ParameterIdentityMatch, TEXT("A"))
        || !Link(TargetParameterGet, TEXT("Item"), ParameterIdentityMatch, TEXT("B"))
        || !Link(LiveIndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, LiveCharacterId, UEdGraphSchema_K2::PN_Self)
        || !Link(LiveCharacterId, UEdGraphSchema_K2::PN_ReturnValue, CharacterIdIdentityMatch, TEXT("A"))
        || !Link(TargetCharacterIdGet, TEXT("Item"), CharacterIdIdentityMatch, TEXT("B"))
        || !Link(ParameterIdentityMatch, UEdGraphSchema_K2::PN_ReturnValue, IdentityMatch, TEXT("A"))
        || !Link(CharacterIdIdentityMatch, UEdGraphSchema_K2::PN_ReturnValue, IdentityMatch, TEXT("B"))
        || !Link(TargetAliveBranch, UEdGraphSchema_K2::PN_Then, LiveParameterValidBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(LiveParameterValidBranch, UEdGraphSchema_K2::PN_Then, IdentityMatchBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(IdentityMatch, UEdGraphSchema_K2::PN_ReturnValue, IdentityMatchBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(IdentityMatchBranch, UEdGraphSchema_K2::PN_Then, SpeciesFilterBranch, UEdGraphSchema_K2::PN_Execute)
        // __DEPRECATED_20260721__ [reason: never mutate ESP_Targets while ForEachTarget is executing]
        // || !Link(IdentityMatchBranch, UEdGraphSchema_K2::PN_Else, RebindRemoveTarget, UEdGraphSchema_K2::PN_Execute)
        // || !Link(RebindSelf, UEdGraphSchema_K2::PN_Self, RebindRemoveTarget, UEdGraphSchema_K2::PN_Self)
        // || !Link(ForEachTarget, TEXT("Array Element"), RebindRemoveTarget, TEXT("Target"))
        // || !Link(RebindRemoveTarget, UEdGraphSchema_K2::PN_Then, RebindAddTarget, UEdGraphSchema_K2::PN_Execute)
        // || !Link(RebindSelf, UEdGraphSchema_K2::PN_Self, RebindAddTarget, UEdGraphSchema_K2::PN_Self)
        // || !Link(ForEachTarget, TEXT("Array Element"), RebindAddTarget, TEXT("Target"))
        // || !Link(LiveIndividualParameter, UEdGraphSchema_K2::PN_ReturnValue, LiveLevel, UEdGraphSchema_K2::PN_Self)
        // || !Link(LiveLevel, UEdGraphSchema_K2::PN_ReturnValue, RebindAddTarget, TEXT("Level"))
        // || !Link(RoundDistance, UEdGraphSchema_K2::PN_ReturnValue, RebindAddTarget, TEXT("DistanceMeters"))
        || !Link(SpeciesFilterBranch, UEdGraphSchema_K2::PN_Then, GenderFilterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(GenderFilterBranch, UEdGraphSchema_K2::PN_Then, LuckyFilterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(LuckyFilterBranch, UEdGraphSchema_K2::PN_Then, BossFilterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BossFilterBranch, UEdGraphSchema_K2::PN_Then, CollectionFilterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(CollectionFilterBranch, UEdGraphSchema_K2::PN_Then, ElementFilterBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ElementFilterBranch, UEdGraphSchema_K2::PN_Then, IvMinimumBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(IvMinimumAccepted, UEdGraphSchema_K2::PN_ReturnValue, IvMinimumBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(IvMinimumBranch, UEdGraphSchema_K2::PN_Then, ResetPassiveFilterMatch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ResetPassiveFilterMatch, UEdGraphSchema_K2::PN_Then, ForEachPassiveFilterId, TEXT("Exec"))
        || !Link(ForEachPassiveFilterId, TEXT("Completed"), PassiveFilterBranch, UEdGraphSchema_K2::PN_Execute)
        // __DEPRECATED_20260718__ [reason: exclusion filters now run after include-AND filters]
        // || !Link(PassiveFilterBranch, UEdGraphSchema_K2::PN_Then, ProjectBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(PassiveFilterBranch, UEdGraphSchema_K2::PN_Then, ResetPassiveExcludeMatch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ResetPassiveExcludeMatch, UEdGraphSchema_K2::PN_Then, ForEachPassiveExcludeId, TEXT("Exec"))
        || !Link(ForEachPassiveExcludeId, TEXT("Completed"), PassiveExcludeBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(PassiveExcludeBranch, UEdGraphSchema_K2::PN_Then, ProjectBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachTarget, TEXT("Array Element"), ActorLocation, UEdGraphSchema_K2::PN_Self)
        || !Link(Self, UEdGraphSchema_K2::PN_Self, PlayerController, TEXT("WorldContextObject"))
        || !Link(PlayerController, UEdGraphSchema_K2::PN_ReturnValue, Project, TEXT("PlayerController"))
        || !Link(ActorLocation, UEdGraphSchema_K2::PN_ReturnValue, Project, TEXT("WorldLocation"))
        || !Link(Project, UEdGraphSchema_K2::PN_ReturnValue, ProjectBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ProjectBranch, UEdGraphSchema_K2::PN_Then, PaintSequence, UEdGraphSchema_K2::PN_Execute)
        || !Link(PaintSequence, TEXT("Then_0"), LabelEnabledBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(OutlineExecTail, UEdGraphSchema_K2::PN_Then, DrawText, UEdGraphSchema_K2::PN_Execute)
        || !Link(PaintSequence, TEXT("Then_1"), TopGuideEnabledBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(TopGuideEnabledBranch, UEdGraphSchema_K2::PN_Then, DrawLine, UEdGraphSchema_K2::PN_Execute)
        || !Link(OnPaint, TEXT("Context"), DrawText, TEXT("Context"))
        || !Link(SelectFinalLabel, UEdGraphSchema_K2::PN_ReturnValue, DrawText, TEXT("InString"))
        || !Link(Project, TEXT("ScreenPosition"), LabelPosition, TEXT("A"))
        || !Link(MakeLabelOffset, UEdGraphSchema_K2::PN_ReturnValue, LabelPosition, TEXT("B"))
        || !Link(LabelPosition, UEdGraphSchema_K2::PN_ReturnValue, DrawText, TEXT("Position"))
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
        || !SetPinDefault(BuildLevelText, TEXT("AppendTo"), TEXT(""))
        || !SetPinDefault(BuildLevelText, TEXT("Suffix"), TEXT(""))
        || !SetPinDefault(PlayerPawn, TEXT("PlayerIndex"), TEXT("0"))
        || !SetPinDefault(DistanceMeters, TEXT("B"), TEXT("100.0"))
        || !SetPinDefault(BuildDistanceText, TEXT("Prefix"), TEXT(""))
        || !SetPinDefault(LevelWithSpacing, TEXT("B"), TEXT("  "))
        || !SetPinDefault(SelectWithoutLevel, TEXT("B"), TEXT(""))
        || !SetPinDefault(BuildIvHp, TEXT("AppendTo"), TEXT(""))
        || !SetPinDefault(BuildIvHp, TEXT("Suffix"), TEXT(""))
        || !SetPinDefault(BuildIvAttack, TEXT("Suffix"), TEXT(""))
        || !SetPinDefault(BuildIvDefense, TEXT("Suffix"), TEXT(""))
        || !SetPinDefault(IvSeparator, TEXT("A"), TEXT("\n"))
        || !SetPinDefault(IvSeparator, TEXT("B"), TEXT(""))
        || !SetPinDefault(MakeLabelOffset, TEXT("X"), TEXT("8.0"))
        || !SetPinDefault(MakeLabelOffset, TEXT("Y"), TEXT("12.0"))
        || !SetPinDefault(DrawText, TEXT("Tint"), TEXT("(R=1.0,G=1.0,B=1.0,A=0.95)"))
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

bool BuildHandledKeyOverride(UWidgetBlueprint* Blueprint, const FName& FunctionName) {
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    if (!PrepareOverrideFunctionGraph(
            Blueprint, UUserWidget::StaticClass(), FunctionName, Graph, Entry, Result)) {
        return false;
    }
    UK2Node_CallFunction* HandledReply = AddStaticCall(
        Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Handled"), -1040, 1120);
    return HandledReply
        && Link(Entry, UEdGraphSchema_K2::PN_Then, Result, UEdGraphSchema_K2::PN_Execute)
        && Link(HandledReply, UEdGraphSchema_K2::PN_ReturnValue, Result, UEdGraphSchema_K2::PN_ReturnValue);
}

bool BuildPanelKeyDownOverride(UWidgetBlueprint* Blueprint) {
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* GenericHandledResult = nullptr;
    if (!EnsureMemberVariable(Blueprint, PanelEscapeClosePendingVariableName, BoolPin(), TEXT("false"))
        || !PrepareOverrideFunctionGraph(
            Blueprint, UUserWidget::StaticClass(), TEXT("OnKeyDown"), Graph, Entry, GenericHandledResult)) {
        return false;
    }

    UK2Node_FunctionResult* EscapeHandledResult = AddFunctionResultNode(Graph, Entry, 520, 880);
    UK2Node_CallFunction* GetKey = AddStaticCall(
        Graph, UKismetInputLibrary::StaticClass(), TEXT("GetKey"), -1320, 1280);
    UK2Node_CallFunction* IsEscape = AddStaticCall(
        Graph, UKismetInputLibrary::StaticClass(), TEXT("EqualEqual_KeyKey"), -1040, 1280);
    UK2Node_IfThenElse* EscapeBranch = AddBranch(Graph, -760, 1120);
    UK2Node_VariableSet* SetPending = AddVariableSet(
        Graph, PanelEscapeClosePendingVariableName, -480, 880);
    UK2Node_CallFunction* HandledReply = AddStaticCall(
        Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Handled"), 0, 1280);
    return EscapeHandledResult && GetKey && IsEscape && EscapeBranch && SetPending && HandledReply
        && Link(Entry, TEXT("InKeyEvent"), GetKey, TEXT("Input"))
        && Link(GetKey, UEdGraphSchema_K2::PN_ReturnValue, IsEscape, TEXT("A"))
        && SetPinDefault(IsEscape, TEXT("B"), TEXT("Escape"))
        && Link(Entry, UEdGraphSchema_K2::PN_Then, EscapeBranch, UEdGraphSchema_K2::PN_Execute)
        && Link(IsEscape, UEdGraphSchema_K2::PN_ReturnValue, EscapeBranch, UEdGraphSchema_K2::PN_Condition)
        && Link(EscapeBranch, UEdGraphSchema_K2::PN_Then, SetPending, UEdGraphSchema_K2::PN_Execute)
        && SetPinDefault(SetPending, PanelEscapeClosePendingVariableName, TEXT("true"))
        && Link(SetPending, UEdGraphSchema_K2::PN_Then, EscapeHandledResult, UEdGraphSchema_K2::PN_Execute)
        && Link(EscapeBranch, UEdGraphSchema_K2::PN_Else, GenericHandledResult, UEdGraphSchema_K2::PN_Execute)
        && Link(HandledReply, UEdGraphSchema_K2::PN_ReturnValue, EscapeHandledResult, UEdGraphSchema_K2::PN_ReturnValue)
        && Link(HandledReply, UEdGraphSchema_K2::PN_ReturnValue, GenericHandledResult, UEdGraphSchema_K2::PN_ReturnValue);
}

bool BuildPanelKeyUpOverride(UWidgetBlueprint* Blueprint, UClass* ModActorClass) {
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* GenericHandledResult = nullptr;
    if (!ModActorClass || !ModActorClass->FindFunctionByName(PanelToggleEventName)
        || !EnsureMemberVariable(Blueprint, PanelEscapeClosePendingVariableName, BoolPin(), TEXT("false"))
        || !PrepareOverrideFunctionGraph(
            Blueprint, UUserWidget::StaticClass(), TEXT("OnKeyUp"), Graph, Entry, GenericHandledResult)) {
        return false;
    }

    UK2Node_FunctionResult* EscapeHandledResult = AddFunctionResultNode(Graph, Entry, 1080, 880);
    UK2Node_CallFunction* GetKey = AddStaticCall(
        Graph, UKismetInputLibrary::StaticClass(), TEXT("GetKey"), -1320, 1280);
    UK2Node_CallFunction* IsEscape = AddStaticCall(
        Graph, UKismetInputLibrary::StaticClass(), TEXT("EqualEqual_KeyKey"), -1040, 1280);
    UK2Node_VariableGet* PendingGet = AddVariableGet(
        Graph, PanelEscapeClosePendingVariableName, -1040, 1440);
    UK2Node_CallFunction* ShouldClose = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), -760, 1280);
    UK2Node_IfThenElse* EscapeBranch = AddBranch(Graph, -480, 1120);
    UK2Node_VariableSet* ClearPending = AddVariableSet(
        Graph, PanelEscapeClosePendingVariableName, -200, 880);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, 80, 1280);
    UK2Node_CallFunction* TogglePanel = AddStaticCall(
        Graph, ModActorClass, *PanelToggleEventName.ToString(), 360, 880);
    UK2Node_CallFunction* HandledReply = AddStaticCall(
        Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Handled"), 800, 1280);
    return EscapeHandledResult && GetKey && IsEscape && PendingGet && ShouldClose && EscapeBranch
        && ClearPending && BridgeGet && TogglePanel && HandledReply
        && Link(Entry, TEXT("InKeyEvent"), GetKey, TEXT("Input"))
        && Link(GetKey, UEdGraphSchema_K2::PN_ReturnValue, IsEscape, TEXT("A"))
        && SetPinDefault(IsEscape, TEXT("B"), TEXT("Escape"))
        && Link(IsEscape, UEdGraphSchema_K2::PN_ReturnValue, ShouldClose, TEXT("A"))
        && Link(PendingGet, PanelEscapeClosePendingVariableName, ShouldClose, TEXT("B"))
        && Link(Entry, UEdGraphSchema_K2::PN_Then, EscapeBranch, UEdGraphSchema_K2::PN_Execute)
        && Link(ShouldClose, UEdGraphSchema_K2::PN_ReturnValue, EscapeBranch, UEdGraphSchema_K2::PN_Condition)
        && Link(EscapeBranch, UEdGraphSchema_K2::PN_Then, ClearPending, UEdGraphSchema_K2::PN_Execute)
        && SetPinDefault(ClearPending, PanelEscapeClosePendingVariableName, TEXT("false"))
        && Link(ClearPending, UEdGraphSchema_K2::PN_Then, TogglePanel, UEdGraphSchema_K2::PN_Execute)
        && Link(BridgeGet, PanelBridgeVariableName, TogglePanel, UEdGraphSchema_K2::PN_Self)
        && Link(TogglePanel, UEdGraphSchema_K2::PN_Then, EscapeHandledResult, UEdGraphSchema_K2::PN_Execute)
        && Link(EscapeBranch, UEdGraphSchema_K2::PN_Else, GenericHandledResult, UEdGraphSchema_K2::PN_Execute)
        && Link(HandledReply, UEdGraphSchema_K2::PN_ReturnValue, EscapeHandledResult, UEdGraphSchema_K2::PN_ReturnValue)
        && Link(HandledReply, UEdGraphSchema_K2::PN_ReturnValue, GenericHandledResult, UEdGraphSchema_K2::PN_ReturnValue);
}

bool PrepareModActorControls(UBlueprint* Blueprint) {
    if (!Blueprint
        || !EnsureMemberVariable(Blueprint, ControlRevisionVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, RuntimeEnabledVariableName, BoolPin(), TEXT("true"))
        || !EnsureMemberVariable(Blueprint, ProfileIdVariableName, IntPin(), TEXT("2"))
        || !EnsureMemberVariable(Blueprint, PresetIdVariableName, IntPin(), TEXT("1"))
        || !EnsureMemberVariable(Blueprint, CaptureRequestedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, LanguageIdVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, LevelMinVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, LevelMaxVariableName, IntPin(), TEXT("0"))
        // __DEPRECATED_20260717__ [reason: retained on ModActor only so older assets remain reversible]
        || !EnsureMemberVariable(Blueprint, DistanceMinVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, DistanceMaxVariableName, IntPin(), TEXT("330"))
        || !EnsureMemberVariable(Blueprint, DisplayTargetLimitVariableName, IntPin(), TEXT("64"))
        || !EnsureMemberVariable(Blueprint, ShowTopGuideLineVariableName, BoolPin(), TEXT("true"))
        || !EnsureMemberVariable(Blueprint, ShowNameVariableName, BoolPin(), TEXT("true"))
        || !EnsureMemberVariable(Blueprint, ShowLevelVariableName, BoolPin(), TEXT("true"))
        || !EnsureMemberVariable(Blueprint, ShowDistanceVariableName, BoolPin(), TEXT("true"))
        || !EnsureMemberVariable(Blueprint, ShowIvVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ShowPassiveSkillsVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, IvMinVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, IvHpMinVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, IvAttackMinVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, IvDefenseMinVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PassiveFilterIdsVariableName, NameArrayPin())
        || !EnsureMemberVariable(Blueprint, PassiveExcludeIdsVariableName, NameArrayPin())
        || !EnsureMemberVariable(Blueprint, PassiveIncludeTextVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, PassiveExcludeTextVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, PassiveFilterRevisionVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, SpeciesFilterIdsVariableName, NameArrayPin())
        || !EnsureMemberVariable(Blueprint, SpeciesFilterTextVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, RichTextAuditBufferVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, PanelMainPageVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PanelFilterPageVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PassiveRainbowExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveLegendExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveGold3ExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveGold2ExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveNormalExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveNegative1ExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveNegative2ExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveNegative3ExpandedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, GenderFilterIdVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, LuckyFilterIdVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, BossFilterIdVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, CollectionFilterIdVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, OverlayElementFilterMaskVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, ElementNormalVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementFireVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementWaterVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementLeafVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementElectricityVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementIceVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementEarthVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementDarkVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, ElementDragonVariableName, BoolPin(), TEXT("false"))) {
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

USpinBox* AddPanelSpinBox(
    UWidgetBlueprint* Blueprint,
    UPanelWidget* Parent,
    const FName& Name,
    float InitialValue,
    float Minimum,
    float Maximum,
    float Delta) {
    USpinBox* SpinBox = Blueprint->WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), Name);
    if (!SpinBox) {
        return nullptr;
    }
    SpinBox->bIsVariable = true;
    SpinBox->SetValue(InitialValue);
    SpinBox->SetMinValue(Minimum);
    SpinBox->SetMaxValue(Maximum);
    SpinBox->SetMinSliderValue(Minimum);
    SpinBox->SetMaxSliderValue(Maximum);
    SpinBox->SetDelta(Delta);
    SpinBox->SetMinFractionalDigits(0);
    SpinBox->SetMaxFractionalDigits(0);
    SpinBox->SetAlwaysUsesDeltaSnap(false);
    SpinBox->MinDesiredWidth = 180.0f;
    SpinBox->ForegroundColor = FSlateColor(FLinearColor(0.96f, 0.97f, 0.98f, 1.0f));
    Parent->AddChild(SpinBox);
    return SpinBox;
}

namespace PanelV2Style {
const FLinearColor Surface(0.055f, 0.058f, 0.064f, 0.985f);
const FLinearColor SurfaceRaised(0.105f, 0.11f, 0.12f, 1.0f);
const FLinearColor SurfaceHover(0.15f, 0.155f, 0.165f, 1.0f);
const FLinearColor Border(0.22f, 0.225f, 0.235f, 1.0f);
const FLinearColor PrimaryText(0.95f, 0.955f, 0.96f, 1.0f);
const FLinearColor SecondaryText(0.67f, 0.69f, 0.71f, 1.0f);
const FLinearColor Accent(0.16f, 0.72f, 0.45f, 1.0f);
const FLinearColor AccentHover(0.20f, 0.82f, 0.52f, 1.0f);
const FLinearColor AccentText(0.025f, 0.04f, 0.03f, 1.0f);
const FLinearColor Disabled(0.18f, 0.185f, 0.195f, 0.72f);
const FLinearColor ToggleUnchecked(0.26f, 0.27f, 0.29f, 1.0f);
const FLinearColor ToggleUncheckedHover(0.34f, 0.35f, 0.37f, 1.0f);
const FLinearColor ToggleOutline(0.82f, 0.83f, 0.85f, 1.0f);
}

void SetVerticalPadding(UWidget* Widget, const FMargin& Padding) {
    if (Widget) {
        if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Widget->Slot)) {
            Slot->SetPadding(Padding);
        }
    }
}

void SetHorizontalLayout(
    UWidget* Widget,
    const FMargin& Padding,
    ESlateSizeRule::Type SizeRule,
    EHorizontalAlignment HorizontalAlignment = HAlign_Fill) {
    if (Widget) {
        if (UHorizontalBoxSlot* Slot = Cast<UHorizontalBoxSlot>(Widget->Slot)) {
            Slot->SetPadding(Padding);
            Slot->SetSize(FSlateChildSize(SizeRule));
            Slot->SetHorizontalAlignment(HorizontalAlignment);
            Slot->SetVerticalAlignment(VAlign_Center);
        }
    }
}

UTextBlock* AddPanelTextV2(
    UWidgetBlueprint* Blueprint,
    UPanelWidget* Parent,
    const FName& Name,
    const TCHAR* Text,
    int32 FontSize,
    bool bSecondary = false) {
    UTextBlock* Widget = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    if (!Widget) {
        return nullptr;
    }
    Widget->bIsVariable = true;
    Widget->SetText(FText::FromString(Text));
    Widget->SetColorAndOpacity(FSlateColor(bSecondary ? PanelV2Style::SecondaryText : PanelV2Style::PrimaryText));
    FSlateFontInfo Font = Widget->Font;
    Font.Size = FontSize;
    Widget->SetFont(Font);
    Widget->SetAutoWrapText(true);
    Parent->AddChild(Widget);
    return Widget;
}

UButton* AddPanelButtonV2(
    UWidgetBlueprint* Blueprint,
    UPanelWidget* Parent,
    const FName& ButtonName,
    const FName& TextName,
    const TCHAR* Text,
    bool bAccent = false) {
    UButton* Button = Blueprint->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
    UTextBlock* Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
    if (!Button || !Label) {
        return nullptr;
    }
    const FLinearColor Normal = bAccent ? PanelV2Style::Accent : PanelV2Style::SurfaceRaised;
    const FLinearColor Hovered = bAccent ? PanelV2Style::AccentHover : PanelV2Style::SurfaceHover;
    FButtonStyle Style = FButtonStyle::GetDefault();
    Style.SetNormal(FSlateRoundedBoxBrush(Normal, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetHovered(FSlateRoundedBoxBrush(Hovered, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetPressed(FSlateRoundedBoxBrush(PanelV2Style::Accent, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetDisabled(FSlateRoundedBoxBrush(PanelV2Style::Disabled, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetNormalPadding(FMargin(12.0f, 7.0f))
        .SetPressedPadding(FMargin(12.0f, 7.0f));
    Button->bIsVariable = true;
    Button->IsFocusable = true;
    Button->SetStyle(Style);
    Button->SetBackgroundColor(FLinearColor::White);
    Label->bIsVariable = true;
    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(PanelV2Style::PrimaryText));
    FSlateFontInfo Font = Label->Font;
    Font.Size = 13;
    Label->SetFont(Font);
    Label->SetJustification(ETextJustify::Center);
    Button->AddChild(Label);
    Parent->AddChild(Button);
    if (Parent->IsA<UHorizontalBox>()) {
        SetHorizontalLayout(Button, FMargin(0.0f, 0.0f, 6.0f, 0.0f), ESlateSizeRule::Fill);
    } else {
        SetVerticalPadding(Button, FMargin(0.0f, 4.0f));
    }
    return Button;
}

UEditableTextBox* AddPanelSearchBoxV2(
    UWidgetBlueprint* Blueprint,
    UHorizontalBox* Parent,
    const FName& Name) {
    if (!Blueprint || !Parent) {
        return nullptr;
    }
    UEditableTextBox* SearchBox = Blueprint->WidgetTree->ConstructWidget<UEditableTextBox>(
        UEditableTextBox::StaticClass(), Name);
    if (!SearchBox) {
        return nullptr;
    }
    FEditableTextBoxStyle Style = FEditableTextBoxStyle::GetDefault();
    FSlateFontInfo Font = Style.TextStyle.Font;
    Font.Size = 13;
    FTextBlockStyle TextStyle = Style.TextStyle;
    TextStyle
        .SetFont(Font)
        .SetColorAndOpacity(FSlateColor(PanelV2Style::PrimaryText));
    Style
        .SetTextStyle(TextStyle)
        .SetForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetFocusedForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetBackgroundImageNormal(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, PanelV2Style::Border, 1.0f))
        .SetBackgroundImageHovered(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetBackgroundImageFocused(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, PanelV2Style::Accent, 1.5f))
        .SetBackgroundImageReadOnly(FSlateRoundedBoxBrush(PanelV2Style::Disabled, 5.0f, PanelV2Style::Border, 1.0f))
        .SetPadding(FMargin(10.0f, 6.0f));
    SearchBox->bIsVariable = true;
    SearchBox->WidgetStyle = Style;
    SearchBox->SetText(FText::GetEmpty());
    SearchBox->SetHintText(FText::GetEmpty());
    SearchBox->SetMinDesiredWidth(360.0f);
    Parent->AddChild(SearchBox);
    SetHorizontalLayout(SearchBox, FMargin(0.0f, 0.0f, 6.0f, 0.0f), ESlateSizeRule::Fill);
    return SearchBox;
}

void ConfigurePanelSegmentButtonV2(UButton* Button, bool bSelected) {
    if (!Button) {
        return;
    }
    const FLinearColor HoverMultiplier(1.08f, 1.08f, 1.08f, 1.0f);
    const FLinearColor PressedMultiplier(0.88f, 0.88f, 0.88f, 1.0f);
    FButtonStyle Style = FButtonStyle::GetDefault();
    Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor::White, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetHovered(FSlateRoundedBoxBrush(HoverMultiplier, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetPressed(FSlateRoundedBoxBrush(PressedMultiplier, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetDisabled(FSlateRoundedBoxBrush(PanelV2Style::Disabled, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetNormalPadding(FMargin(12.0f, 7.0f))
        .SetPressedPadding(FMargin(12.0f, 7.0f));
    Button->SetStyle(Style);
    Button->SetBackgroundColor(bSelected ? PanelV2Style::Accent : PanelV2Style::SurfaceRaised);
}

UCheckBox* AddPanelToggleV2(
    UWidgetBlueprint* Blueprint,
    UVerticalBox* Parent,
    const FName& RowName,
    const FName& TextName,
    const FName& ToggleName,
    const TCHAR* Text,
    bool bInitialValue) {
    UHorizontalBox* Row = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
    if (!Row) {
        return nullptr;
    }
    Parent->AddChild(Row);
    SetVerticalPadding(Row, FMargin(0.0f, 2.0f));
    UTextBlock* Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
    UCheckBox* Toggle = Blueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), ToggleName);
    if (!Label || !Toggle) {
        return nullptr;
    }
    FCheckBoxStyle Style = FCheckBoxStyle::GetDefault();
    Style.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
        .SetUncheckedImage(FSlateRoundedBoxBrush(PanelV2Style::ToggleUnchecked, 5.0f, PanelV2Style::ToggleOutline, 1.5f, FVector2D(28.0f, 24.0f)))
        .SetUncheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::ToggleUncheckedHover, 5.0f, PanelV2Style::ToggleOutline, 1.5f, FVector2D(28.0f, 24.0f)))
        .SetUncheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 5.0f, PanelV2Style::ToggleOutline, 1.5f, FVector2D(28.0f, 24.0f)))
        .SetCheckedImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 5.0f, PanelV2Style::ToggleOutline, 1.5f, FVector2D(28.0f, 24.0f)))
        .SetCheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::AccentHover, 5.0f, PanelV2Style::ToggleOutline, 1.5f, FVector2D(28.0f, 24.0f)))
        .SetCheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 5.0f, PanelV2Style::ToggleOutline, 1.5f, FVector2D(28.0f, 24.0f)))
        .SetPadding(FMargin(0.0f));
    Toggle->bIsVariable = true;
    Toggle->SetWidgetStyle(Style);
    Toggle->SetIsChecked(bInitialValue);
    Row->AddChild(Toggle);
    Label->bIsVariable = true;
    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(PanelV2Style::PrimaryText));
    FSlateFontInfo LabelFont = Label->Font;
    LabelFont.Size = 14;
    Label->SetFont(LabelFont);
    Label->SetAutoWrapText(true);
    Row->AddChild(Label);
    SetHorizontalLayout(Toggle, FMargin(0.0f, 0.0f, 8.0f, 0.0f), ESlateSizeRule::Automatic, HAlign_Left);
    SetHorizontalLayout(Label, FMargin(0.0f), ESlateSizeRule::Automatic, HAlign_Left);
    return Toggle;
}

UCheckBox* AddPanelFilterChipV2(
    UWidgetBlueprint* Blueprint,
    UPanelWidget* Parent,
    const FName& ToggleName,
    const FName& TextName,
    const TCHAR* Text) {
    UCheckBox* Toggle = Blueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), ToggleName);
    UTextBlock* Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
    if (!Toggle || !Label || !Parent) {
        return nullptr;
    }

    FCheckBoxStyle Style = FCheckBoxStyle::GetDefault();
    Style.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
        .SetUncheckedImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetUncheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetUncheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::Border, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetCheckedImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetCheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::AccentHover, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetCheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 6.0f, FVector2D(0.0f, 34.0f)))
        .SetPadding(FMargin(10.0f, 6.0f));
    Toggle->bIsVariable = true;
    Toggle->SetWidgetStyle(Style);
    Toggle->SetIsChecked(false);

    Label->bIsVariable = true;
    Label->SetText(FText::FromString(Text));
    Label->SetColorAndOpacity(FSlateColor(PanelV2Style::PrimaryText));
    FSlateFontInfo Font = Label->Font;
    Font.Size = 13;
    Label->SetFont(Font);
    Label->SetJustification(ETextJustify::Center);
    Toggle->AddChild(Label);
    Parent->AddChild(Toggle);
    SetHorizontalLayout(Toggle, FMargin(0.0f, 0.0f, 6.0f, 0.0f), ESlateSizeRule::Fill);
    return Toggle;
}

struct FPanelNumericControlV2 {
    USlider* Slider = nullptr;
    USpinBox* SpinBox = nullptr;
};

FPanelNumericControlV2 AddPanelNumericControlV2(
    UWidgetBlueprint* Blueprint,
    UVerticalBox* Parent,
    const FName& LabelName,
    const FName& RowName,
    const FName& SliderName,
    const FName& SpinBoxName,
    const FName& UnitName,
    const TCHAR* LabelText,
    const TCHAR* UnitText,
    float InitialValue,
    float Minimum,
    float Maximum,
    float Step) {
    FPanelNumericControlV2 Result;
    UTextBlock* Label = AddPanelTextV2(Blueprint, Parent, LabelName, LabelText, 13, true);
    UHorizontalBox* Row = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
    if (!Label || !Row) {
        return Result;
    }
    Parent->AddChild(Row);
    SetVerticalPadding(Label, FMargin(0.0f, 5.0f, 0.0f, 1.0f));
    SetVerticalPadding(Row, FMargin(0.0f, 0.0f, 0.0f, 4.0f));

    USlider* Slider = Blueprint->WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), SliderName);
    USpinBox* SpinBox = Blueprint->WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), SpinBoxName);
    if (!Slider || !SpinBox) {
        return Result;
    }
    FSliderStyle SliderStyle = FSliderStyle::GetDefault();
    SliderStyle.SetNormalBarImage(FSlateRoundedBoxBrush(PanelV2Style::Border, 2.0f, FVector2D(8.0f, 4.0f)))
        .SetHoveredBarImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 2.0f, FVector2D(8.0f, 4.0f)))
        .SetDisabledBarImage(FSlateRoundedBoxBrush(PanelV2Style::Disabled, 2.0f, FVector2D(8.0f, 4.0f)))
        .SetNormalThumbImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 8.0f, FVector2D(16.0f, 16.0f)))
        .SetHoveredThumbImage(FSlateRoundedBoxBrush(PanelV2Style::AccentHover, 9.0f, FVector2D(18.0f, 18.0f)))
        .SetDisabledThumbImage(FSlateRoundedBoxBrush(PanelV2Style::Disabled, 8.0f, FVector2D(16.0f, 16.0f)))
        .SetBarThickness(4.0f);
    Slider->bIsVariable = true;
    Slider->SetWidgetStyle(SliderStyle);
    Slider->SetMinValue(Minimum);
    Slider->SetMaxValue(Maximum);
    Slider->SetStepSize(Step);
    Slider->SetValue(InitialValue);
    Slider->SetIndentHandle(true);
    Slider->SetLocked(false);
    Slider->MouseUsesStep = true;
    Slider->IsFocusable = true;

    FSpinBoxStyle SpinStyle = FSpinBoxStyle::GetDefault();
    SpinStyle.SetBackgroundBrush(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, FVector2D(72.0f, 32.0f)))
        .SetActiveBackgroundBrush(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, FVector2D(72.0f, 32.0f)))
        .SetHoveredBackgroundBrush(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 5.0f, FVector2D(72.0f, 32.0f)))
        .SetActiveFillBrush(FSlateRoundedBoxBrush(PanelV2Style::Accent, 5.0f, FVector2D(72.0f, 32.0f)))
        .SetHoveredFillBrush(FSlateRoundedBoxBrush(PanelV2Style::Border, 5.0f, FVector2D(72.0f, 32.0f)))
        .SetInactiveFillBrush(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, FVector2D(72.0f, 32.0f)))
        .SetForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetTextPadding(FMargin(8.0f, 5.0f));
    SpinBox->bIsVariable = true;
    SpinBox->WidgetStyle = SpinStyle;
    SpinBox->SetValue(InitialValue);
    SpinBox->SetMinValue(Minimum);
    SpinBox->SetMaxValue(Maximum);
    SpinBox->SetMinSliderValue(Minimum);
    SpinBox->SetMaxSliderValue(Maximum);
    SpinBox->SetDelta(Step);
    SpinBox->SetMinFractionalDigits(0);
    SpinBox->SetMaxFractionalDigits(0);
    SpinBox->SetAlwaysUsesDeltaSnap(true);
    SpinBox->bEnableSlider = false;
    SpinBox->MinDesiredWidth = 72.0f;
    SpinBox->ForegroundColor = FSlateColor(PanelV2Style::PrimaryText);
    FSlateFontInfo SpinFont = SpinBox->Font;
    SpinFont.Size = 13;
    SpinBox->Font = SpinFont;

    Row->AddChild(Slider);
    Row->AddChild(SpinBox);
    SetHorizontalLayout(Slider, FMargin(0.0f, 0.0f, 12.0f, 0.0f), ESlateSizeRule::Fill);
    SetHorizontalLayout(SpinBox, FMargin(0.0f), ESlateSizeRule::Automatic, HAlign_Right);
    if (UnitText && UnitText[0] != '\0') {
        UTextBlock* Unit = AddPanelTextV2(Blueprint, Row, UnitName, UnitText, 13, true);
        if (!Unit) {
            return Result;
        }
        SetHorizontalLayout(Unit, FMargin(5.0f, 0.0f, 0.0f, 0.0f), ESlateSizeRule::Automatic, HAlign_Right);
    }
    Result.Slider = Slider;
    Result.SpinBox = SpinBox;
    return Result;
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

bool AppendPanelSegmentVisualsConstant(
    UEdGraph* Graph,
    const TArray<UButton*>& Buttons,
    int32 SelectedIndex,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    if (!Graph || Buttons.Num() == 0 || !Buttons.IsValidIndex(SelectedIndex)) {
        return false;
    }
    for (int32 Index = 0; Index < Buttons.Num(); ++Index) {
        UButton* Button = Buttons[Index];
        UK2Node_VariableGet* ButtonGet = Button
            ? AddVariableGet(Graph, Button->GetFName(), X, Y + 140)
            : nullptr;
        UK2Node_CallFunction* SetBackground = AddStaticCall(
            Graph, UButton::StaticClass(), TEXT("SetBackgroundColor"), X + 260, Y);
        const FLinearColor Color = Index == SelectedIndex
            ? PanelV2Style::Accent
            : PanelV2Style::SurfaceRaised;
        if (!ButtonGet || !SetBackground
            || !SetPinDefault(SetBackground, TEXT("InBackgroundColor"), Color.ToString())
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetBackground, UEdGraphSchema_K2::PN_Execute)
            || !Link(ButtonGet, Button->GetFName(), SetBackground, UEdGraphSchema_K2::PN_Self)) {
            return false;
        }
        ExecTail = SetBackground;
        X += 520;
    }
    return true;
}

bool AppendPanelSegmentVisualsDynamic(
    UEdGraph* Graph,
    UK2Node_CustomEvent* Event,
    const FName& InputName,
    const TArray<UButton*>& Buttons,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    if (!Graph || !Event || Buttons.Num() != 3) {
        return false;
    }

    UK2Node_CallFunction* ClampedId = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Clamp"), X, Y + 210);
    UK2Node_CallFunction* IsMale = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 260, Y + 140);
    UK2Node_CallFunction* IsFemale = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 260, Y + 280);
    UK2Node_CallFunction* IsRestricted = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), X + 520, Y + 210);
    UK2Node_CallFunction* IsAll = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), X + 780, Y + 210);
    if (!ClampedId || !IsMale || !IsFemale || !IsRestricted || !IsAll
        || !Link(Event, InputName, ClampedId, TEXT("Value"))
        || !SetPinDefault(ClampedId, TEXT("Min"), TEXT("0"))
        || !SetPinDefault(ClampedId, TEXT("Max"), TEXT("2"))
        || !Link(ClampedId, UEdGraphSchema_K2::PN_ReturnValue, IsMale, TEXT("A"))
        || !SetPinDefault(IsMale, TEXT("B"), TEXT("1"))
        || !Link(ClampedId, UEdGraphSchema_K2::PN_ReturnValue, IsFemale, TEXT("A"))
        || !SetPinDefault(IsFemale, TEXT("B"), TEXT("2"))
        || !Link(IsMale, UEdGraphSchema_K2::PN_ReturnValue, IsRestricted, TEXT("A"))
        || !Link(IsFemale, UEdGraphSchema_K2::PN_ReturnValue, IsRestricted, TEXT("B"))
        || !Link(IsRestricted, UEdGraphSchema_K2::PN_ReturnValue, IsAll, TEXT("A"))) {
        return false;
    }

    const TArray<UK2Node_CallFunction*> Conditions = {IsAll, IsMale, IsFemale};
    X += 1040;
    for (int32 Index = 0; Index < Buttons.Num(); ++Index) {
        UButton* Button = Buttons[Index];
        UK2Node_CallFunction* SelectAlpha = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectFloat"), X, Y + 280);
        UK2Node_CallFunction* SelectColor = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("LinearColorLerp"), X + 260, Y + 280);
        UK2Node_VariableGet* ButtonGet = Button
            ? AddVariableGet(Graph, Button->GetFName(), X + 520, Y + 420)
            : nullptr;
        UK2Node_CallFunction* SetBackground = AddStaticCall(
            Graph, UButton::StaticClass(), TEXT("SetBackgroundColor"), X + 780, Y);
        if (!ButtonGet || !SelectAlpha || !SelectColor || !SetBackground
            || !SetPinDefault(SelectAlpha, TEXT("A"), TEXT("1.0"))
            || !SetPinDefault(SelectAlpha, TEXT("B"), TEXT("0.0"))
            || !Link(Conditions[Index], UEdGraphSchema_K2::PN_ReturnValue, SelectAlpha, TEXT("bPickA"))
            || !SetPinDefault(SelectColor, TEXT("A"), PanelV2Style::SurfaceRaised.ToString())
            || !SetPinDefault(SelectColor, TEXT("B"), PanelV2Style::Accent.ToString())
            || !Link(SelectAlpha, UEdGraphSchema_K2::PN_ReturnValue, SelectColor, TEXT("Alpha"))
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetBackground, UEdGraphSchema_K2::PN_Execute)
            || !Link(ButtonGet, Button->GetFName(), SetBackground, UEdGraphSchema_K2::PN_Self)
            || !Link(SelectColor, UEdGraphSchema_K2::PN_ReturnValue, SetBackground, TEXT("InBackgroundColor"))) {
            return false;
        }
        ExecTail = SetBackground;
        X += 1040;
    }
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
    int32 Y,
    const TArray<UButton*>* SegmentButtons = nullptr,
    int32 SelectedSegmentIndex = INDEX_NONE) {
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
    if (SegmentButtons) {
        if (!AppendPanelSegmentVisualsConstant(
                Graph, *SegmentButtons, SelectedSegmentIndex, ExecTail, X, Y)) {
            return false;
        }
        X += SegmentButtons->Num() * 520;
    }
    return AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, X, Y);
}

bool BuildPanelNumericEvent(
    UWidgetBlueprint* Blueprint,
    USpinBox* SpinBox,
    UClass* ModActorClass,
    const FName& VariableName,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddSpinBoxValueChangedEvent(Blueprint, SpinBox, -1300, Y);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1040, Y + 120);
    UK2Node_CallFunction* Round = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Round"), -1040, Y);
    UK2Node_VariableSet* Store = AddExternalVariableSet(Graph, VariableName, ModActorClass, -760, Y);
    UEdGraphNode* ExecTail = Store;
    if (!Graph || !Event || !BridgeGet || !Round || !Store
        || !Link(Event, TEXT("InValue"), Round, TEXT("A"))
        || !Link(Round, UEdGraphSchema_K2::PN_ReturnValue, Store, VariableName)
        || !Link(Event, UEdGraphSchema_K2::PN_Then, Store, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, Store, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    return AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, -460, Y);
}

bool BuildPanelBooleanEvent(
    UWidgetBlueprint* Blueprint,
    UCheckBox* CheckBox,
    UClass* ModActorClass,
    const FName& VariableName,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddCheckBoxStateChangedEvent(Blueprint, CheckBox, -1300, Y);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1040, Y + 120);
    UK2Node_VariableSet* Store = AddExternalVariableSet(Graph, VariableName, ModActorClass, -760, Y);
    UEdGraphNode* ExecTail = Store;
    if (!Graph || !Event || !BridgeGet || !Store
        || !Link(Event, TEXT("bIsChecked"), Store, VariableName)
        || !Link(Event, UEdGraphSchema_K2::PN_Then, Store, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, Store, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    return AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, -460, Y);
}

bool BuildPanelExpansionEvent(
    UWidgetBlueprint* Blueprint,
    UExpandableArea* Area,
    UClass* ModActorClass,
    const FName& VariableName,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddExpandableAreaExpansionChangedEvent(Blueprint, Area, -1300, Y);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1040, Y + 120);
    UK2Node_VariableSet* Store = AddExternalVariableSet(Graph, VariableName, ModActorClass, -760, Y);
    UEdGraphNode* ExecTail = Store;
    if (!Graph || !Event || !BridgeGet || !Store
        || !Link(Event, TEXT("bIsExpanded"), Store, VariableName)
        || !Link(Event, UEdGraphSchema_K2::PN_Then, Store, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, Store, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    return AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, -460, Y);
}

bool BuildPanelInitializeControls(
    UWidgetBlueprint* Blueprint,
    USpinBox* DisplayLimit,
    USpinBox* LevelMin,
    USpinBox* LevelMax,
    USpinBox* DistanceMin,
    USpinBox* DistanceMax,
    UCheckBox* ShowLevel,
    UCheckBox* ShowDistance,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_CustomEvent* Event = AddCustomEvent(Blueprint, Graph, *PanelInitializeControlsEventName.ToString(), -1400, Y, {
        TPair<FName, FEdGraphPinType>(FName("DisplayTargetLimit"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("LevelMin"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("LevelMax"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("DistanceMin"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("DistanceMax"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowLevel"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowDistance"), BoolPin())
    });
    if (!Graph || !Event) {
        return false;
    }

    struct FInitializer {
        USpinBox* SpinBox;
        FName InputName;
    };
    const TArray<FInitializer> Initializers = {
        {DisplayLimit, TEXT("DisplayTargetLimit")},
        {LevelMin, TEXT("LevelMin")},
        {LevelMax, TEXT("LevelMax")},
        {DistanceMin, TEXT("DistanceMin")},
        {DistanceMax, TEXT("DistanceMax")},
    };
    UEdGraphNode* ExecTail = Event;
    int32 X = -1120;
    for (const FInitializer& Initializer : Initializers) {
        UK2Node_VariableGet* SpinGet = AddVariableGet(Graph, Initializer.SpinBox->GetFName(), X, Y + 140);
        UK2Node_CallFunction* ToFloat = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_IntToFloat"), X, Y + 280);
        UK2Node_CallFunction* SetValue = AddStaticCall(Graph, USpinBox::StaticClass(), TEXT("SetValue"), X + 260, Y);
        if (!SpinGet || !ToFloat || !SetValue
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetValue, UEdGraphSchema_K2::PN_Execute)
            || !Link(SpinGet, Initializer.SpinBox->GetFName(), SetValue, UEdGraphSchema_K2::PN_Self)
            || !Link(Event, Initializer.InputName, ToFloat, TEXT("InInt"))
            || !Link(ToFloat, UEdGraphSchema_K2::PN_ReturnValue, SetValue, TEXT("NewValue"))) {
            return false;
        }
        ExecTail = SetValue;
        X += 560;
    }
    struct FBooleanInitializer {
        UCheckBox* CheckBox;
        FName InputName;
    };
    const TArray<FBooleanInitializer> BooleanInitializers = {
        {ShowLevel, TEXT("ShowLevel")},
        {ShowDistance, TEXT("ShowDistance")},
    };
    for (const FBooleanInitializer& Initializer : BooleanInitializers) {
        UK2Node_VariableGet* CheckBoxGet = AddVariableGet(Graph, Initializer.CheckBox->GetFName(), X, Y + 140);
        UK2Node_CallFunction* SetChecked = AddStaticCall(Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), X + 260, Y);
        if (!CheckBoxGet || !SetChecked
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetChecked, UEdGraphSchema_K2::PN_Execute)
            || !Link(CheckBoxGet, Initializer.CheckBox->GetFName(), SetChecked, UEdGraphSchema_K2::PN_Self)
            || !Link(Event, Initializer.InputName, SetChecked, TEXT("InIsChecked"))) {
            return false;
        }
        ExecTail = SetChecked;
        X += 560;
    }
    return true;
}

bool BuildPanelNumericEventV2(
    UWidgetBlueprint* Blueprint,
    const FPanelNumericControlV2& Control,
    UClass* ModActorClass,
    const FName& VariableName,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* SliderEvent = AddSliderValueChangedEvent(Blueprint, Control.Slider, -1500, Y);
    UK2Node_VariableGet* SpinBoxGet = AddVariableGet(Graph, Control.SpinBox->GetFName(), -1240, Y + 280);
    UK2Node_CallFunction* SyncSpinBox = AddStaticCall(Graph, USpinBox::StaticClass(), TEXT("SetValue"), -980, Y);
    if (!Graph || !SliderEvent || !SpinBoxGet || !SyncSpinBox
        || !Link(SliderEvent, UEdGraphSchema_K2::PN_Then, SyncSpinBox, UEdGraphSchema_K2::PN_Execute)
        || !Link(SliderEvent, TEXT("Value"), SyncSpinBox, TEXT("NewValue"))
        || !Link(SpinBoxGet, Control.SpinBox->GetFName(), SyncSpinBox, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }

    auto BuildSliderCommit = [&](const FName& DelegateName, int32 CommitY) -> bool {
        UK2Node_ComponentBoundEvent* CommitEvent = AddSliderCaptureEndEvent(
            Blueprint, Control.Slider, DelegateName, -1500, CommitY);
        UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1240, CommitY + 160);
        UK2Node_VariableGet* SliderGet = AddVariableGet(Graph, Control.Slider->GetFName(), -1240, CommitY + 280);
        UK2Node_CallFunction* GetValue = AddStaticCall(Graph, USlider::StaticClass(), TEXT("GetValue"), -980, CommitY + 160);
        UK2Node_CallFunction* Round = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Round"), -700, CommitY + 160);
        UK2Node_VariableSet* Store = AddExternalVariableSet(Graph, VariableName, ModActorClass, -420, CommitY);
        UEdGraphNode* ExecTail = Store;
        return CommitEvent && BridgeGet && SliderGet && GetValue && Round && Store
            && Link(CommitEvent, UEdGraphSchema_K2::PN_Then, Store, UEdGraphSchema_K2::PN_Execute)
            && Link(SliderGet, Control.Slider->GetFName(), GetValue, UEdGraphSchema_K2::PN_Self)
            && Link(GetValue, UEdGraphSchema_K2::PN_ReturnValue, Round, TEXT("A"))
            && Link(Round, UEdGraphSchema_K2::PN_ReturnValue, Store, VariableName)
            && Link(BridgeGet, PanelBridgeVariableName, Store, UEdGraphSchema_K2::PN_Self)
            && AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, -140, CommitY);
    };
    if (!BuildSliderCommit(GET_MEMBER_NAME_CHECKED(USlider, OnMouseCaptureEnd), Y + 420)
        || !BuildSliderCommit(GET_MEMBER_NAME_CHECKED(USlider, OnControllerCaptureEnd), Y + 840)) {
        return false;
    }

    UK2Node_ComponentBoundEvent* SpinEvent = AddSpinBoxValueCommittedEvent(Blueprint, Control.SpinBox, -1500, Y + 1260);
    UK2Node_VariableGet* SpinBridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1240, Y + 580);
    UK2Node_VariableGet* SliderGet = AddVariableGet(Graph, Control.Slider->GetFName(), -1240, Y + 700);
    UK2Node_CallFunction* SyncSlider = AddStaticCall(Graph, USlider::StaticClass(), TEXT("SetValue"), -980, Y + 1260);
    UK2Node_CallFunction* SpinRound = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Round"), -980, Y + 580);
    UK2Node_VariableSet* SpinStore = AddExternalVariableSet(Graph, VariableName, ModActorClass, -700, Y + 1260);
    UEdGraphNode* SpinExecTail = SpinStore;
    if (!SpinEvent || !SpinBridgeGet || !SliderGet || !SyncSlider || !SpinRound || !SpinStore
        || !Link(SpinEvent, UEdGraphSchema_K2::PN_Then, SyncSlider, UEdGraphSchema_K2::PN_Execute)
        || !Link(SpinEvent, TEXT("InValue"), SyncSlider, TEXT("InValue"))
        || !Link(SliderGet, Control.Slider->GetFName(), SyncSlider, UEdGraphSchema_K2::PN_Self)
        || !Link(SpinEvent, TEXT("InValue"), SpinRound, TEXT("A"))
        || !Link(SyncSlider, UEdGraphSchema_K2::PN_Then, SpinStore, UEdGraphSchema_K2::PN_Execute)
        || !Link(SpinRound, UEdGraphSchema_K2::PN_ReturnValue, SpinStore, VariableName)
        || !Link(SpinBridgeGet, PanelBridgeVariableName, SpinStore, UEdGraphSchema_K2::PN_Self)
        || !AppendRevisionIncrement(Graph, ModActorClass, SpinBridgeGet, SpinExecTail, -420, Y + 1260)) {
        return false;
    }
    return true;
}

bool BuildPanelInitializeControlsV2(
    UWidgetBlueprint* Blueprint,
    const FPanelNumericControlV2& DisplayLimit,
    const FPanelNumericControlV2& LevelMin,
    const FPanelNumericControlV2& LevelMax,
    const FPanelNumericControlV2& DistanceMax,
    const FPanelNumericControlV2& IvHpMin,
    const FPanelNumericControlV2& IvAttackMin,
    const FPanelNumericControlV2& IvDefenseMin,
    UCheckBox* RuntimeEnabled,
    UCheckBox* ShowTopGuideLine,
    UCheckBox* ShowName,
    UCheckBox* ShowLevel,
    UCheckBox* ShowDistance,
    UCheckBox* ShowIv,
    UCheckBox* ShowPassiveSkills,
    const TArray<UCheckBox*>& ElementToggles,
    UTextBlock* GenderStatus,
    const TArray<UButton*>& GenderButtons,
    UTextBlock* LuckyStatus,
    const TArray<UButton*>& LuckyButtons,
    UTextBlock* BossStatus,
    const TArray<UButton*>& BossButtons,
    UTextBlock* CollectionStatus,
    const TArray<UButton*>& CollectionButtons,
    const TArray<UExpandableArea*>& PassiveAreas,
    int32 Y,
    UK2Node_CustomEvent* ExistingEvent = nullptr) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_CustomEvent* Event = ExistingEvent;
    if (!Event) {
        Event = AddCustomEvent(Blueprint, Graph, *PanelInitializeControlsV2EventName.ToString(), -1600, Y, {
            TPair<FName, FEdGraphPinType>(FName("DisplayTargetLimit"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("DistanceMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("IvHpMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("IvAttackMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("IvDefenseMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("RuntimeEnabled"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowTopGuideLine"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowName"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowLevel"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowDistance"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowIV"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowPassiveSkills"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementNormal"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementFire"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementWater"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementLeaf"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementElectricity"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementIce"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementEarth"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementDark"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementDragon"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("GenderFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LuckyFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("BossFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("CollectionFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LanguageId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandRainbow"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandLegend"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandGold3"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandGold2"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNormal"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNegative1"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNegative2"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNegative3"), BoolPin())
        });
    }
    if (!Graph || !Event || !CollectionStatus || CollectionButtons.Num() != 3
        || ElementToggles.Num() != 9 || PassiveAreas.Num() != 8 || PassiveAreas.Contains(nullptr)) {
        return false;
    }

    struct FNumericInitializerV2 {
        FPanelNumericControlV2 Control;
        FName InputName;
    };
    const TArray<FNumericInitializerV2> NumericInitializers = {
        {DisplayLimit, TEXT("DisplayTargetLimit")},
        {LevelMin, TEXT("LevelMin")},
        {LevelMax, TEXT("LevelMax")},
        {DistanceMax, TEXT("DistanceMax")},
        {IvHpMin, TEXT("IvHpMin")},
        {IvAttackMin, TEXT("IvAttackMin")},
        {IvDefenseMin, TEXT("IvDefenseMin")},
    };
    UEdGraphNode* ExecTail = Event;
    int32 X = -1320;
    for (const FNumericInitializerV2& Initializer : NumericInitializers) {
        UK2Node_VariableGet* SliderGet = AddVariableGet(Graph, Initializer.Control.Slider->GetFName(), X, Y + 160);
        UK2Node_VariableGet* SpinGet = AddVariableGet(Graph, Initializer.Control.SpinBox->GetFName(), X, Y + 280);
        UK2Node_CallFunction* ToFloat = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_IntToFloat"), X, Y + 400);
        UK2Node_CallFunction* SetSliderValue = AddStaticCall(Graph, USlider::StaticClass(), TEXT("SetValue"), X + 260, Y);
        UK2Node_CallFunction* SetSpinValue = AddStaticCall(Graph, USpinBox::StaticClass(), TEXT("SetValue"), X + 520, Y);
        if (!SliderGet || !SpinGet || !ToFloat || !SetSliderValue || !SetSpinValue
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetSliderValue, UEdGraphSchema_K2::PN_Execute)
            || !Link(SliderGet, Initializer.Control.Slider->GetFName(), SetSliderValue, UEdGraphSchema_K2::PN_Self)
            || !Link(Event, Initializer.InputName, ToFloat, TEXT("InInt"))
            || !Link(ToFloat, UEdGraphSchema_K2::PN_ReturnValue, SetSliderValue, TEXT("InValue"))
            || !Link(SetSliderValue, UEdGraphSchema_K2::PN_Then, SetSpinValue, UEdGraphSchema_K2::PN_Execute)
            || !Link(SpinGet, Initializer.Control.SpinBox->GetFName(), SetSpinValue, UEdGraphSchema_K2::PN_Self)
            || !Link(ToFloat, UEdGraphSchema_K2::PN_ReturnValue, SetSpinValue, TEXT("NewValue"))) {
            return false;
        }
        ExecTail = SetSpinValue;
        X += 820;
    }

    struct FBooleanInitializerV2 {
        UCheckBox* CheckBox;
        FName InputName;
    };
    const TArray<FBooleanInitializerV2> BooleanInitializers = {
        {RuntimeEnabled, TEXT("RuntimeEnabled")},
        {ShowTopGuideLine, TEXT("ShowTopGuideLine")},
        {ShowName, TEXT("ShowName")},
        {ShowLevel, TEXT("ShowLevel")},
        {ShowDistance, TEXT("ShowDistance")},
        {ShowIv, TEXT("ShowIV")},
        {ShowPassiveSkills, TEXT("ShowPassiveSkills")},
        {ElementToggles[0], TEXT("ElementNormal")},
        {ElementToggles[1], TEXT("ElementFire")},
        {ElementToggles[2], TEXT("ElementWater")},
        {ElementToggles[3], TEXT("ElementLeaf")},
        {ElementToggles[4], TEXT("ElementElectricity")},
        {ElementToggles[5], TEXT("ElementIce")},
        {ElementToggles[6], TEXT("ElementEarth")},
        {ElementToggles[7], TEXT("ElementDark")},
        {ElementToggles[8], TEXT("ElementDragon")},
    };
    for (const FBooleanInitializerV2& Initializer : BooleanInitializers) {
        UK2Node_VariableGet* CheckBoxGet = AddVariableGet(Graph, Initializer.CheckBox->GetFName(), X, Y + 160);
        UK2Node_CallFunction* SetChecked = AddStaticCall(Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), X + 260, Y);
        if (!CheckBoxGet || !SetChecked
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetChecked, UEdGraphSchema_K2::PN_Execute)
            || !Link(CheckBoxGet, Initializer.CheckBox->GetFName(), SetChecked, UEdGraphSchema_K2::PN_Self)
            || !Link(Event, Initializer.InputName, SetChecked, TEXT("InIsChecked"))) {
            return false;
        }
        ExecTail = SetChecked;
        X += 560;
    }

    const TArray<FName> ExpansionInputs = {
        TEXT("ExpandRainbow"), TEXT("ExpandLegend"), TEXT("ExpandGold3"), TEXT("ExpandGold2"),
        TEXT("ExpandNormal"), TEXT("ExpandNegative1"), TEXT("ExpandNegative2"), TEXT("ExpandNegative3"),
    };
    for (int32 Index = 0; Index < PassiveAreas.Num(); ++Index) {
        UK2Node_VariableGet* AreaGet = AddVariableGet(Graph, PassiveAreas[Index]->GetFName(), X, Y + 160);
        UK2Node_CallFunction* SetExpanded = AddStaticCall(
            Graph, UExpandableArea::StaticClass(), TEXT("SetIsExpanded"), X + 260, Y);
        if (!AreaGet || !SetExpanded
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetExpanded, UEdGraphSchema_K2::PN_Execute)
            || !Link(AreaGet, PassiveAreas[Index]->GetFName(), SetExpanded, UEdGraphSchema_K2::PN_Self)
            || !Link(Event, ExpansionInputs[Index], SetExpanded, TEXT("IsExpanded"))) {
            return false;
        }
        ExecTail = SetExpanded;
        X += 560;
    }

    UK2Node_CallFunction* IsMale = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 160);
    UK2Node_CallFunction* IsFemale = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 320);
    UK2Node_CallFunction* SelectFemale = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 280, Y + 240);
    UK2Node_CallFunction* SelectMale = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 560, Y + 160);
    UK2Node_CallFunction* ToText = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 840, Y + 160);
    UK2Node_VariableGet* GenderStatusGet = AddVariableGet(Graph, GenderStatus->GetFName(), X + 840, Y + 320);
    UK2Node_CallFunction* SetGenderStatus = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 1120, Y);
    if (!IsMale || !IsFemale || !SelectFemale || !SelectMale || !ToText || !GenderStatusGet || !SetGenderStatus
        || !Link(Event, TEXT("GenderFilterId"), IsMale, TEXT("A"))
        || !SetPinDefault(IsMale, TEXT("B"), TEXT("1"))
        || !Link(Event, TEXT("GenderFilterId"), IsFemale, TEXT("A"))
        || !SetPinDefault(IsFemale, TEXT("B"), TEXT("2"))
        || !SetPinDefault(SelectFemale, TEXT("A"), TEXT("当前：雌性"))
        || !SetPinDefault(SelectFemale, TEXT("B"), TEXT("当前：全部"))
        || !Link(IsFemale, UEdGraphSchema_K2::PN_ReturnValue, SelectFemale, TEXT("bPickA"))
        || !SetPinDefault(SelectMale, TEXT("A"), TEXT("当前：雄性"))
        || !Link(SelectFemale, UEdGraphSchema_K2::PN_ReturnValue, SelectMale, TEXT("B"))
        || !Link(IsMale, UEdGraphSchema_K2::PN_ReturnValue, SelectMale, TEXT("bPickA"))
        || !Link(SelectMale, UEdGraphSchema_K2::PN_ReturnValue, ToText, TEXT("InString"))
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetGenderStatus, UEdGraphSchema_K2::PN_Execute)
        || !Link(GenderStatusGet, GenderStatus->GetFName(), SetGenderStatus, UEdGraphSchema_K2::PN_Self)
        || !Link(ToText, UEdGraphSchema_K2::PN_ReturnValue, SetGenderStatus, TEXT("InText"))) {
        return false;
    }
    ExecTail = SetGenderStatus;
    if (!AppendPanelSegmentVisualsDynamic(
            Graph, Event, TEXT("GenderFilterId"), GenderButtons, ExecTail, X + 1400, Y)) {
        return false;
    }

    X += 5000;
    UK2Node_CallFunction* IsOnlyLucky = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 160);
    UK2Node_CallFunction* IsExcludeLucky = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 320);
    UK2Node_CallFunction* SelectExcludeLucky = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 280, Y + 240);
    UK2Node_CallFunction* SelectOnlyLucky = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 560, Y + 160);
    UK2Node_CallFunction* LuckyToText = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 840, Y + 160);
    UK2Node_VariableGet* LuckyStatusGet = AddVariableGet(Graph, LuckyStatus->GetFName(), X + 840, Y + 320);
    UK2Node_CallFunction* SetLuckyStatus = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 1120, Y);
    if (!IsOnlyLucky || !IsExcludeLucky || !SelectExcludeLucky || !SelectOnlyLucky
        || !LuckyToText || !LuckyStatusGet || !SetLuckyStatus
        || !Link(Event, TEXT("LuckyFilterId"), IsOnlyLucky, TEXT("A"))
        || !SetPinDefault(IsOnlyLucky, TEXT("B"), TEXT("1"))
        || !Link(Event, TEXT("LuckyFilterId"), IsExcludeLucky, TEXT("A"))
        || !SetPinDefault(IsExcludeLucky, TEXT("B"), TEXT("2"))
        || !SetPinDefault(SelectExcludeLucky, TEXT("A"), TEXT("当前：排除闪光"))
        || !SetPinDefault(SelectExcludeLucky, TEXT("B"), TEXT("当前：全部"))
        || !Link(IsExcludeLucky, UEdGraphSchema_K2::PN_ReturnValue, SelectExcludeLucky, TEXT("bPickA"))
        || !SetPinDefault(SelectOnlyLucky, TEXT("A"), TEXT("当前：仅闪光"))
        || !Link(SelectExcludeLucky, UEdGraphSchema_K2::PN_ReturnValue, SelectOnlyLucky, TEXT("B"))
        || !Link(IsOnlyLucky, UEdGraphSchema_K2::PN_ReturnValue, SelectOnlyLucky, TEXT("bPickA"))
        || !Link(SelectOnlyLucky, UEdGraphSchema_K2::PN_ReturnValue, LuckyToText, TEXT("InString"))
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetLuckyStatus, UEdGraphSchema_K2::PN_Execute)
        || !Link(LuckyStatusGet, LuckyStatus->GetFName(), SetLuckyStatus, UEdGraphSchema_K2::PN_Self)
        || !Link(LuckyToText, UEdGraphSchema_K2::PN_ReturnValue, SetLuckyStatus, TEXT("InText"))) {
        return false;
    }
    ExecTail = SetLuckyStatus;
    if (!AppendPanelSegmentVisualsDynamic(
            Graph, Event, TEXT("LuckyFilterId"), LuckyButtons, ExecTail, X + 1400, Y)) {
        return false;
    }

    X += 5000;
    UK2Node_CallFunction* IsOnlyBoss = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 160);
    UK2Node_CallFunction* IsExcludeBoss = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 320);
    UK2Node_CallFunction* SelectExcludeBoss = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 280, Y + 240);
    UK2Node_CallFunction* SelectOnlyBoss = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 560, Y + 160);
    UK2Node_CallFunction* BossToText = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 840, Y + 160);
    UK2Node_VariableGet* BossStatusGet = AddVariableGet(Graph, BossStatus->GetFName(), X + 840, Y + 320);
    UK2Node_CallFunction* SetBossStatus = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 1120, Y);
    if (!IsOnlyBoss || !IsExcludeBoss || !SelectExcludeBoss || !SelectOnlyBoss
        || !BossToText || !BossStatusGet || !SetBossStatus
        || !Link(Event, TEXT("BossFilterId"), IsOnlyBoss, TEXT("A"))
        || !SetPinDefault(IsOnlyBoss, TEXT("B"), TEXT("1"))
        || !Link(Event, TEXT("BossFilterId"), IsExcludeBoss, TEXT("A"))
        || !SetPinDefault(IsExcludeBoss, TEXT("B"), TEXT("2"))
        || !SetPinDefault(SelectExcludeBoss, TEXT("A"), TEXT("当前：排除首领"))
        || !SetPinDefault(SelectExcludeBoss, TEXT("B"), TEXT("当前：全部"))
        || !Link(IsExcludeBoss, UEdGraphSchema_K2::PN_ReturnValue, SelectExcludeBoss, TEXT("bPickA"))
        || !SetPinDefault(SelectOnlyBoss, TEXT("A"), TEXT("当前：仅首领"))
        || !Link(SelectExcludeBoss, UEdGraphSchema_K2::PN_ReturnValue, SelectOnlyBoss, TEXT("B"))
        || !Link(IsOnlyBoss, UEdGraphSchema_K2::PN_ReturnValue, SelectOnlyBoss, TEXT("bPickA"))
        || !Link(SelectOnlyBoss, UEdGraphSchema_K2::PN_ReturnValue, BossToText, TEXT("InString"))
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetBossStatus, UEdGraphSchema_K2::PN_Execute)
        || !Link(BossStatusGet, BossStatus->GetFName(), SetBossStatus, UEdGraphSchema_K2::PN_Self)
        || !Link(BossToText, UEdGraphSchema_K2::PN_ReturnValue, SetBossStatus, TEXT("InText"))) {
        return false;
    }
    ExecTail = SetBossStatus;
    if (!AppendPanelSegmentVisualsDynamic(
            Graph, Event, TEXT("BossFilterId"), BossButtons, ExecTail, X + 1400, Y)) {
        return false;
    }

    X += 5000;
    UK2Node_CallFunction* IsIncompleteCollection = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 160);
    UK2Node_CallFunction* IsCompleteCollection = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X, Y + 320);
    UK2Node_CallFunction* SelectCompleteCollection = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 280, Y + 240);
    UK2Node_CallFunction* SelectIncompleteCollection = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 560, Y + 160);
    UK2Node_CallFunction* CollectionToText = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 840, Y + 160);
    UK2Node_VariableGet* CollectionStatusGet = AddVariableGet(Graph, CollectionStatus->GetFName(), X + 840, Y + 320);
    UK2Node_CallFunction* SetCollectionStatus = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 1120, Y);
    if (!IsIncompleteCollection || !IsCompleteCollection || !SelectCompleteCollection || !SelectIncompleteCollection
        || !CollectionToText || !CollectionStatusGet || !SetCollectionStatus
        || !Link(Event, TEXT("CollectionFilterId"), IsIncompleteCollection, TEXT("A"))
        || !SetPinDefault(IsIncompleteCollection, TEXT("B"), TEXT("1"))
        || !Link(Event, TEXT("CollectionFilterId"), IsCompleteCollection, TEXT("A"))
        || !SetPinDefault(IsCompleteCollection, TEXT("B"), TEXT("2"))
        || !SetPinDefault(SelectCompleteCollection, TEXT("A"), TEXT("当前：已完成"))
        || !SetPinDefault(SelectCompleteCollection, TEXT("B"), TEXT("当前：全部"))
        || !Link(IsCompleteCollection, UEdGraphSchema_K2::PN_ReturnValue, SelectCompleteCollection, TEXT("bPickA"))
        || !SetPinDefault(SelectIncompleteCollection, TEXT("A"), TEXT("当前：未完成"))
        || !Link(SelectCompleteCollection, UEdGraphSchema_K2::PN_ReturnValue, SelectIncompleteCollection, TEXT("B"))
        || !Link(IsIncompleteCollection, UEdGraphSchema_K2::PN_ReturnValue, SelectIncompleteCollection, TEXT("bPickA"))
        || !Link(SelectIncompleteCollection, UEdGraphSchema_K2::PN_ReturnValue, CollectionToText, TEXT("InString"))
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetCollectionStatus, UEdGraphSchema_K2::PN_Execute)
        || !Link(CollectionStatusGet, CollectionStatus->GetFName(), SetCollectionStatus, UEdGraphSchema_K2::PN_Self)
        || !Link(CollectionToText, UEdGraphSchema_K2::PN_ReturnValue, SetCollectionStatus, TEXT("InText"))) {
        return false;
    }
    ExecTail = SetCollectionStatus;
    return AppendPanelSegmentVisualsDynamic(
        Graph, Event, TEXT("CollectionFilterId"), CollectionButtons, ExecTail, X + 1400, Y);
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
    int32 Y,
    const TArray<UButton*>* LanguageButtons = nullptr,
    bool bRefreshCatalogs = false) {
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
    if (LanguageButtons
        && !AppendPanelSegmentVisualsConstant(
            Graph, *LanguageButtons, LanguageId, ExecTail, X, Y)) {
        return false;
    }
    X += LanguageButtons ? LanguageButtons->Num() * 520 : 0;
    if (!AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, ExecTail, X, Y)) {
        return false;
    }
    if (!bRefreshCatalogs) {
        return true;
    }
    UK2Node_Self* PanelSelf = AddSelfNode(Graph, X + 300, Y + 180);
    UK2Node_CallFunction* RefreshPassiveCatalog = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelPopulatePassiveCatalogEventName.ToString(), X + 560, Y);
    UK2Node_CallFunction* RefreshPalCatalog = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelRenderPalCatalogEventName.ToString(), X + 820, Y);
    return PanelSelf && RefreshPassiveCatalog && RefreshPalCatalog
        && Link(ExecTail, UEdGraphSchema_K2::PN_Then, RefreshPassiveCatalog, UEdGraphSchema_K2::PN_Execute)
        && Link(PanelSelf, UEdGraphSchema_K2::PN_Self, RefreshPassiveCatalog, UEdGraphSchema_K2::PN_Self)
        && Link(RefreshPassiveCatalog, UEdGraphSchema_K2::PN_Then, RefreshPalCatalog, UEdGraphSchema_K2::PN_Execute)
        && Link(PanelSelf, UEdGraphSchema_K2::PN_Self, RefreshPalCatalog, UEdGraphSchema_K2::PN_Self);
}

bool BuildPanelInitializeLanguage(
    UWidgetBlueprint* Blueprint,
    const TArray<FPanelTranslation>& Translations,
    const TArray<UButton*>& LanguageButtons,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_CustomEvent* Event = AddCustomEvent(
        Blueprint,
        Graph,
        *PanelInitializeLanguageEventName.ToString(),
        -1600,
        Y,
        {TPair<FName, FEdGraphPinType>(FName("LanguageId"), IntPin())}
    );
    UK2Node_CallFunction* IsEnglish = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -1320, Y + 160);
    UK2Node_CallFunction* IsChinese = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -1040, Y + 160);
    if (!Graph || !Event || !IsEnglish || !IsChinese
        || !Link(Event, TEXT("LanguageId"), IsEnglish, TEXT("A"))
        || !SetPinDefault(IsEnglish, TEXT("B"), TEXT("1"))
        || !Link(IsEnglish, UEdGraphSchema_K2::PN_ReturnValue, IsChinese, TEXT("A"))) {
        return false;
    }

    UEdGraphNode* ExecTail = Event;
    int32 X = -760;
    for (const FPanelTranslation& Translation : Translations) {
        UK2Node_CallFunction* Select = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X, Y + 220);
        UK2Node_CallFunction* ToText = AddStaticCall(
            Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 260, Y + 220);
        UK2Node_VariableGet* TextGet = AddVariableGet(Graph, Translation.TextName, X + 260, Y + 380);
        UK2Node_CallFunction* SetText = AddStaticCall(
            Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 520, Y);
        if (!Select || !ToText || !TextGet || !SetText
            || !SetPinDefault(Select, TEXT("A"), Translation.English)
            || !SetPinDefault(Select, TEXT("B"), Translation.Chinese)
            || !Link(IsEnglish, UEdGraphSchema_K2::PN_ReturnValue, Select, TEXT("bPickA"))
            || !Link(Select, UEdGraphSchema_K2::PN_ReturnValue, ToText, TEXT("InString"))
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetText, UEdGraphSchema_K2::PN_Execute)
            || !Link(TextGet, Translation.TextName, SetText, UEdGraphSchema_K2::PN_Self)
            || !Link(ToText, UEdGraphSchema_K2::PN_ReturnValue, SetText, TEXT("InText"))) {
            return false;
        }
        ExecTail = SetText;
        X += 820;
    }

    if (LanguageButtons.Num() != 2) {
        return false;
    }
    const TArray<UK2Node_CallFunction*> Conditions = {IsChinese, IsEnglish};
    for (int32 Index = 0; Index < LanguageButtons.Num(); ++Index) {
        UButton* Button = LanguageButtons[Index];
        UK2Node_CallFunction* SelectAlpha = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectFloat"), X, Y + 220);
        UK2Node_CallFunction* SelectColor = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("LinearColorLerp"), X + 260, Y + 220);
        UK2Node_VariableGet* ButtonGet = Button
            ? AddVariableGet(Graph, Button->GetFName(), X + 520, Y + 380)
            : nullptr;
        UK2Node_CallFunction* SetBackground = AddStaticCall(
            Graph, UButton::StaticClass(), TEXT("SetBackgroundColor"), X + 780, Y);
        if (!SelectAlpha || !SelectColor || !ButtonGet || !SetBackground
            || !SetPinDefault(SelectAlpha, TEXT("A"), TEXT("1.0"))
            || !SetPinDefault(SelectAlpha, TEXT("B"), TEXT("0.0"))
            || !Link(Conditions[Index], UEdGraphSchema_K2::PN_ReturnValue, SelectAlpha, TEXT("bPickA"))
            || !SetPinDefault(SelectColor, TEXT("A"), PanelV2Style::SurfaceRaised.ToString())
            || !SetPinDefault(SelectColor, TEXT("B"), PanelV2Style::Accent.ToString())
            || !Link(SelectAlpha, UEdGraphSchema_K2::PN_ReturnValue, SelectColor, TEXT("Alpha"))
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetBackground, UEdGraphSchema_K2::PN_Execute)
            || !Link(ButtonGet, Button->GetFName(), SetBackground, UEdGraphSchema_K2::PN_Self)
            || !Link(SelectColor, UEdGraphSchema_K2::PN_ReturnValue, SetBackground, TEXT("InBackgroundColor"))) {
            return false;
        }
        ExecTail = SetBackground;
        X += 1040;
    }
    return true;
}

bool AppendExternalIntegerIncrement(
    UEdGraph* Graph,
    UClass* OwnerClass,
    UK2Node_VariableGet* BridgeGet,
    const FName& VariableName,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    UK2Node_VariableGet* ValueGet = AddExternalVariableGet(Graph, VariableName, OwnerClass, X, Y + 120);
    UK2Node_CallFunction* AddOne = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_IntInt"), X + 260, Y + 120);
    UK2Node_VariableSet* ValueSet = AddExternalVariableSet(Graph, VariableName, OwnerClass, X + 520, Y);
    if (!ValueGet || !AddOne || !ValueSet
        || !SetPinDefault(AddOne, TEXT("B"), TEXT("1"))
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, ValueGet, UEdGraphSchema_K2::PN_Self)
        || !Link(ValueGet, VariableName, AddOne, TEXT("A"))
        || !Link(AddOne, UEdGraphSchema_K2::PN_ReturnValue, ValueSet, VariableName)
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, ValueSet, UEdGraphSchema_K2::PN_Self)
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, ValueSet, UEdGraphSchema_K2::PN_Execute)) {
        return false;
    }
    ExecTail = ValueSet;
    return true;
}

bool AppendPassiveTokenMutation(
    UEdGraph* Graph,
    UClass* ModActorClass,
    UK2Node_VariableGet* BridgeGet,
    UK2Node_VariableGet* SkillIdGet,
    const FName& TextVariableName,
    bool bAdd,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    UK2Node_CallFunction* SkillIdToString = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), X, Y + 240);
    UK2Node_CallFunction* TokenPrefix = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 260, Y + 240);
    UK2Node_CallFunction* TokenSuffix = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 520, Y + 240);
    UK2Node_VariableGet* TextGet = AddExternalVariableGet(
        Graph, TextVariableName, ModActorClass, X, Y + 120);
    UK2Node_CallFunction* RemoveToken = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), X + 780, Y + 120);
    UK2Node_CallFunction* AddToken = bAdd
        ? AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 1040, Y + 120)
        : nullptr;
    UK2Node_VariableSet* StoreText = AddExternalVariableSet(
        Graph, TextVariableName, ModActorClass, X + 1300, Y);
    UEdGraphNode* ValueSource = bAdd ? static_cast<UEdGraphNode*>(AddToken) : static_cast<UEdGraphNode*>(RemoveToken);
    if (!Graph || !ModActorClass || !BridgeGet || !SkillIdGet || !ExecTail
        || !SkillIdToString || !TokenPrefix || !TokenSuffix || !TextGet || !RemoveToken || !StoreText
        || (bAdd && !AddToken)
        || !Link(SkillIdGet, SkillIdGet->GetVarName(), SkillIdToString, TEXT("InName"))
        || !SetPinDefault(TokenPrefix, TEXT("A"), TEXT("|"))
        || !Link(SkillIdToString, UEdGraphSchema_K2::PN_ReturnValue, TokenPrefix, TEXT("B"))
        || !Link(TokenPrefix, UEdGraphSchema_K2::PN_ReturnValue, TokenSuffix, TEXT("A"))
        || !SetPinDefault(TokenSuffix, TEXT("B"), TEXT("|"))
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, TextGet, UEdGraphSchema_K2::PN_Self)
        || !Link(TextGet, TextVariableName, RemoveToken, TEXT("SourceString"))
        || !Link(TokenSuffix, UEdGraphSchema_K2::PN_ReturnValue, RemoveToken, TEXT("From"))
        || !SetPinDefault(RemoveToken, TEXT("To"), TEXT(""))
        || (bAdd && (!Link(RemoveToken, UEdGraphSchema_K2::PN_ReturnValue, AddToken, TEXT("A"))
            || !Link(TokenSuffix, UEdGraphSchema_K2::PN_ReturnValue, AddToken, TEXT("B"))))
        || !Link(ValueSource, UEdGraphSchema_K2::PN_ReturnValue, StoreText, TextVariableName)
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, StoreText, UEdGraphSchema_K2::PN_Self)
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, StoreText, UEdGraphSchema_K2::PN_Execute)) {
        return false;
    }
    ExecTail = StoreText;
    return true;
}

bool BuildPassiveTooltip(UWidgetBlueprint* Blueprint) {
    if (!Blueprint || !Blueprint->WidgetTree) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);

    UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("ESP_PassiveTooltipBorder"));
    USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("ESP_PassiveTooltipSize"));
    URichTextBlock* RichText = Blueprint->WidgetTree->ConstructWidget<URichTextBlock>(
        URichTextBlock::StaticClass(), TEXT("ESP_PassiveTooltipText"));
    UTextBlock* FontSource = NewObject<UTextBlock>(GetTransientPackage());
    if (!Border || !Size || !RichText || !FontSource) {
        return false;
    }

    FSlateFontInfo Font = FontSource->GetFont();
    Font.Size = 13;
    UDataTable* RichTextStyle = EnsurePassiveRichTextStyleTable(Font);
    if (!RichTextStyle) {
        return false;
    }
    FTextBlockStyle DefaultStyle = FTextBlockStyle::GetDefault();
    DefaultStyle
        .SetFont(Font)
        .SetColorAndOpacity(FSlateColor(PanelV2Style::PrimaryText))
        .SetShadowOffset(FVector2D(1.0f, 1.0f))
        .SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
    RichText->bIsVariable = true;
    RichText->SetText(FText::GetEmpty());
    RichText->SetTextStyleSet(RichTextStyle);
    RichText->SetDefaultTextStyle(DefaultStyle);
    RichText->SetAutoWrapText(true);
    RichText->SetMinDesiredWidth(280.0f);
    Size->SetMinDesiredWidth(300.0f);
    Size->SetMaxDesiredWidth(460.0f);
    Size->AddChild(RichText);
    Border->SetBrush(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, PanelV2Style::Border, 1.0f));
    Border->SetBrushColor(FLinearColor::White);
    Border->SetPadding(FMargin(12.0f, 9.0f));
    Border->AddChild(Size);
    Blueprint->WidgetTree->RootWidget = Border;

    const bool bWidgetPropertyExists = Blueprint->GeneratedClass
        && Blueprint->GeneratedClass->FindPropertyByName(RichText->GetFName());
    if (!bWidgetPropertyExists) {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (Blueprint->Status == BS_Error) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] passive tooltip WidgetTree compile failed"));
            return false;
        }
    }

    UK2Node_CustomEvent* Initialize = AddCustomEvent(
        Blueprint,
        Graph,
        *PassiveTooltipInitializeEventName.ToString(),
        -3000,
        0,
        {{FName("Description"), TextPin()}}
    );
    UK2Node_CallFunction* MakeGameStylePath = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("MakeSoftObjectPath"), -2700, -420);
    UK2Node_CallFunction* MakeGameStyleReference = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("Conv_SoftObjPathToSoftObjRef"), -2400, -420);
    UK2Node_CallFunction* LoadGameStyle = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("LoadAsset_Blocking"), -2100, 0);
    UK2Node_DynamicCast* CastGameStyle = AddPureDynamicCast(
        Graph, UDataTable::StaticClass(), -1800, -420);
    UK2Node_CallFunction* GameStyleIsValid = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -1500, -420);
    UK2Node_CallFunction* SelectStyle = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectObject"), -1200, -420);
    UK2Node_DynamicCast* CastSelectedStyle = AddPureDynamicCast(
        Graph, UDataTable::StaticClass(), -900, -420);
    UK2Node_VariableGet* RichTextStyleGet = AddVariableGet(
        Graph, RichText->GetFName(), -900, -240);
    UK2Node_CallFunction* SetRuntimeStyle = AddStaticCall(
        Graph, URichTextBlock::StaticClass(), TEXT("SetTextStyleSet"), -600, 0);
    UK2Node_MakeArray* DecoratorArray = NewObject<UK2Node_MakeArray>(Graph);
    UEdGraphPin* FallbackStylePin = Pin(SelectStyle, TEXT("B"));
    if (!Initialize || !MakeGameStylePath || !MakeGameStyleReference || !LoadGameStyle
        || !CastGameStyle || !GameStyleIsValid || !SelectStyle || !CastSelectedStyle
        || !RichTextStyleGet || !SetRuntimeStyle || !DecoratorArray || !FallbackStylePin
        || !SetPinDefault(MakeGameStylePath, TEXT("PathString"), GameRichTextStylePath)
        || !Link(MakeGameStylePath, UEdGraphSchema_K2::PN_ReturnValue, MakeGameStyleReference, TEXT("SoftObjectPath"))
        || !Link(MakeGameStyleReference, UEdGraphSchema_K2::PN_ReturnValue, LoadGameStyle, TEXT("Asset"))
        || !Link(LoadGameStyle, UEdGraphSchema_K2::PN_ReturnValue, CastGameStyle, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastGameStyle, CastGameStyle->GetCastResultPin()->PinName, GameStyleIsValid, TEXT("Object"))
        || !Link(CastGameStyle, CastGameStyle->GetCastResultPin()->PinName, SelectStyle, TEXT("A"))
        || !Link(GameStyleIsValid, UEdGraphSchema_K2::PN_ReturnValue, SelectStyle, TEXT("bSelectA"))
        || !Link(SelectStyle, UEdGraphSchema_K2::PN_ReturnValue, CastSelectedStyle, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(RichTextStyleGet, RichText->GetFName(), SetRuntimeStyle, UEdGraphSchema_K2::PN_Self)
        || !Link(CastSelectedStyle, CastSelectedStyle->GetCastResultPin()->PinName, SetRuntimeStyle, TEXT("NewTextStyleSet"))) {
        return false;
    }
    FallbackStylePin->DefaultObject = RichTextStyle;
    constexpr const TCHAR* DecoratorPaths[] = {
        TEXT("/Game/Pal/Blueprint/UI/PalTextBlock/BP_PalRichTextIconDecorator.BP_PalRichTextIconDecorator_C"),
        TEXT("/Game/Pal/Blueprint/UI/PalTextBlock/BP_PalRichTextDecorator_ItemName.BP_PalRichTextDecorator_ItemName_C"),
        TEXT("/Game/Pal/Blueprint/UI/PalTextBlock/BP_PalRichTextDecorator_CharacterName.BP_PalRichTextDecorator_CharacterName_C"),
        TEXT("/Game/Pal/Blueprint/UI/PalTextBlock/BP_PalRichTextDecorator_MapObject.BP_PalRichTextDecorator_MapObject_C"),
        TEXT("/Game/Pal/Blueprint/UI/PalTextBlock/BP_PalRichTextDecorator_ActiveSkillName.BP_PalRichTextDecorator_ActiveSkillName_C"),
        TEXT("/Game/Pal/Blueprint/UI/PalTextBlock/BP_PalRichTextDecorator_UICommon.BP_PalRichTextDecorator_UICommon_C"),
    };
    DecoratorArray->NumInputs = UE_ARRAY_COUNT(DecoratorPaths);
    DecoratorArray->NodePosX = -300;
    DecoratorArray->NodePosY = 420;
    Graph->AddNode(DecoratorArray, true, false);
    DecoratorArray->CreateNewGuid();
    DecoratorArray->AllocateDefaultPins();

    FEdGraphPinType DecoratorClassPin;
    DecoratorClassPin.PinCategory = UEdGraphSchema_K2::PC_Class;
    DecoratorClassPin.PinSubCategoryObject = URichTextBlockDecorator::StaticClass();
    FEdGraphPinType DecoratorArrayPin = DecoratorClassPin;
    DecoratorArrayPin.ContainerType = EPinContainerType::Array;
    DecoratorArray->GetOutputPin()->PinType = DecoratorArrayPin;
    TArray<UK2Node_CallFunction*> LoadDecoratorClasses;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(DecoratorPaths); ++Index) {
        if (UEdGraphPin* InputPin = Pin(DecoratorArray, DecoratorArray->GetPinName(Index))) {
            InputPin->PinType = DecoratorClassPin;
        } else {
            return false;
        }
        UK2Node_CallFunction* MakePath = AddStaticCall(
            Graph, UKismetSystemLibrary::StaticClass(), TEXT("MakeSoftClassPath"), -3000, 240 + Index * 180);
        UK2Node_CallFunction* ToSoftClass = AddStaticCall(
            Graph, UKismetSystemLibrary::StaticClass(), TEXT("Conv_SoftClassPathToSoftClassRef"), -2700, 240 + Index * 180);
        UK2Node_CallFunction* LoadClass = AddStaticCall(
            Graph, UKismetSystemLibrary::StaticClass(), TEXT("LoadClassAsset_Blocking"), -2400 + Index * 300, 0);
        UK2Node_ClassDynamicCast* CastDecorator = AddPureClassDynamicCast(
            Graph, URichTextBlockDecorator::StaticClass(), -2250 + Index * 300, 180);
        if (!MakePath || !ToSoftClass || !LoadClass || !CastDecorator) {
            return false;
        }
        // __DEPRECATED_20260719__ [reason: reflected function pins cannot be manually narrowed]
        // SoftClassOutput->PinType.PinSubCategoryObject = URichTextBlockDecorator::StaticClass();
        // LoadClassInput->PinType.PinSubCategoryObject = URichTextBlockDecorator::StaticClass();
        // LoadClassOutput->PinType = DecoratorClassPin;
        if (!SetPinDefault(MakePath, TEXT("PathString"), DecoratorPaths[Index])
            || !Link(MakePath, UEdGraphSchema_K2::PN_ReturnValue, ToSoftClass, TEXT("SoftClassPath"))
            || !Link(ToSoftClass, UEdGraphSchema_K2::PN_ReturnValue, LoadClass, TEXT("AssetClass"))
            || !Link(LoadClass, UEdGraphSchema_K2::PN_ReturnValue, CastDecorator, TEXT("Class"))
            || !Link(
                CastDecorator, CastDecorator->GetCastResultPin()->PinName,
                DecoratorArray, DecoratorArray->GetPinName(Index))) {
            return false;
        }
        LoadDecoratorClasses.Add(LoadClass);
    }

    UK2Node_VariableGet* RichTextGet = AddVariableGet(Graph, RichText->GetFName(), -600, 240);
    UK2Node_CallFunction* SetDecorators = AddStaticCall(
        Graph, URichTextBlock::StaticClass(), TEXT("SetDecorators"), -300, 0);
    UK2Node_CallFunction* DescriptionToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), 0, 300);
    // __MISTAKE_20260719__ [reason: replacing every pipe also corrupted visible " | " separators]
    // const previous transform was Replace("|", "\"");
    UK2Node_CallFunction* NormalizeStyleAttributePair = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), 300, 300);
    UK2Node_CallFunction* NormalizeAttributeOpen = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), 600, 300);
    UK2Node_CallFunction* NormalizeSelfClosingAttribute = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), 900, 300);
    UK2Node_CallFunction* NormalizedDescriptionToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 1200, 300);
    UK2Node_CallFunction* SetText = AddStaticCall(Graph, URichTextBlock::StaticClass(), TEXT("SetText"), 1500, 0);
    if (LoadDecoratorClasses.Num() != UE_ARRAY_COUNT(DecoratorPaths)
        || !RichTextGet || !SetDecorators || !DescriptionToString || !NormalizeStyleAttributePair
        || !NormalizeAttributeOpen || !NormalizeSelfClosingAttribute || !NormalizedDescriptionToText || !SetText
        || !SetPinDefault(NormalizeStyleAttributePair, TEXT("From"), TEXT("| style=|"))
        || !SetPinDefault(NormalizeStyleAttributePair, TEXT("To"), TEXT("\" style=\""))
        || !SetPinDefault(NormalizeAttributeOpen, TEXT("From"), TEXT("=|"))
        || !SetPinDefault(NormalizeAttributeOpen, TEXT("To"), TEXT("=\""))
        || !SetPinDefault(NormalizeSelfClosingAttribute, TEXT("From"), TEXT("|/>"))
        || !SetPinDefault(NormalizeSelfClosingAttribute, TEXT("To"), TEXT("\"/>"))
        || !Link(Initialize, UEdGraphSchema_K2::PN_Then, LoadGameStyle, UEdGraphSchema_K2::PN_Execute)
        || !Link(LoadGameStyle, UEdGraphSchema_K2::PN_Then, SetRuntimeStyle, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetRuntimeStyle, UEdGraphSchema_K2::PN_Then, LoadDecoratorClasses[0], UEdGraphSchema_K2::PN_Execute)
        || !Link(RichTextGet, RichText->GetFName(), SetDecorators, UEdGraphSchema_K2::PN_Self)
        || !Link(DecoratorArray, DecoratorArray->GetOutputPin()->PinName, SetDecorators, TEXT("InDecoratorClasses"))
        || !Link(LoadDecoratorClasses.Last(), UEdGraphSchema_K2::PN_Then, SetDecorators, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDecorators, UEdGraphSchema_K2::PN_Then, SetText, UEdGraphSchema_K2::PN_Execute)
        || !Link(RichTextGet, RichText->GetFName(), SetText, UEdGraphSchema_K2::PN_Self)
        || !Link(Initialize, TEXT("Description"), DescriptionToString, TEXT("InText"))
        || !Link(DescriptionToString, UEdGraphSchema_K2::PN_ReturnValue, NormalizeStyleAttributePair, TEXT("SourceString"))
        || !Link(NormalizeStyleAttributePair, UEdGraphSchema_K2::PN_ReturnValue, NormalizeAttributeOpen, TEXT("SourceString"))
        || !Link(NormalizeAttributeOpen, UEdGraphSchema_K2::PN_ReturnValue, NormalizeSelfClosingAttribute, TEXT("SourceString"))
        || !Link(NormalizeSelfClosingAttribute, UEdGraphSchema_K2::PN_ReturnValue, NormalizedDescriptionToText, TEXT("InString"))
        || !Link(NormalizedDescriptionToText, UEdGraphSchema_K2::PN_ReturnValue, SetText, TEXT("InText"))) {
        return false;
    }
    for (int32 Index = 0; Index + 1 < LoadDecoratorClasses.Num(); ++Index) {
        if (!Link(
                LoadDecoratorClasses[Index], UEdGraphSchema_K2::PN_Then,
                LoadDecoratorClasses[Index + 1], UEdGraphSchema_K2::PN_Execute)) {
            return false;
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPassiveTooltip compile status=%d nodes=%d"),
        static_cast<int32>(Blueprint->Status), Graph->Nodes.Num());
    return Blueprint->Status != BS_Error;
}

bool BuildPassiveEntry(UWidgetBlueprint* Blueprint, UClass* ModActorClass, UClass* PassiveTooltipClass) {
    if (!Blueprint || !Blueprint->WidgetTree || !ModActorClass || !PassiveTooltipClass) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);
    if (!EnsureMemberVariable(Blueprint, PassiveEntryBridgeVariableName, ObjectPin(ModActorClass))
        || !EnsureMemberVariable(Blueprint, PassiveEntrySkillIdVariableName, NamePin())
        || !EnsureMemberVariable(Blueprint, PassiveEntrySkillNameVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, PassiveEntrySelectedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PassiveEntryExcludedVariableName, BoolPin(), TEXT("false"))) {
        return false;
    }

    USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ESP_PassiveEntrySize"));
    UCheckBox* Toggle = Blueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("ESP_PassiveEntryToggle"));
    UTextBlock* Label = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ESP_PassiveEntryText"));
    if (!Size || !Toggle || !Label) {
        return false;
    }
    Size->SetMinDesiredWidth(210.0f);
    Size->SetMinDesiredHeight(36.0f);
    Toggle->bIsVariable = true;
    Label->bIsVariable = true;
    Label->SetText(FText::FromString(TEXT("Passive")));
    FSlateFontInfo LabelFont = Label->GetFont();
    LabelFont.Size = 13;
    Label->SetFont(LabelFont);
    Label->SetColorAndOpacity(FSlateColor::UseForeground());
    Label->SetJustification(ETextJustify::Center);
    Label->SetAutoWrapText(true);
    FCheckBoxStyle Style = FCheckBoxStyle::GetDefault();
    Style.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
        .SetUncheckedImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetUncheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::ToggleUncheckedHover, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetUncheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetCheckedImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetCheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::AccentHover, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetCheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::Accent, 5.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetHoveredForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetPressedForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetCheckedForegroundColor(FSlateColor(PanelV2Style::AccentText))
        .SetCheckedHoveredForegroundColor(FSlateColor(PanelV2Style::AccentText))
        .SetCheckedPressedForegroundColor(FSlateColor(PanelV2Style::AccentText))
        .SetPadding(FMargin(10.0f, 6.0f));
    Toggle->SetWidgetStyle(Style);
    Toggle->AddChild(Label);
    Size->AddChild(Toggle);
    Blueprint->WidgetTree->RootWidget = Size;

    const bool bWidgetPropertiesExist = Blueprint->GeneratedClass
        && Blueprint->GeneratedClass->FindPropertyByName(Toggle->GetFName())
        && Blueprint->GeneratedClass->FindPropertyByName(Label->GetFName());
    if (!bWidgetPropertiesExist) {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (Blueprint->Status == BS_Error) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] passive entry WidgetTree compile failed"));
            return false;
        }
    }

    UK2Node_CustomEvent* Initialize = AddCustomEvent(
        Blueprint,
        Graph,
        *PassiveEntryInitializeEventName.ToString(),
        -1600,
        -600,
        {
            {FName("Bridge"), ObjectPin(ModActorClass)},
            {FName("SkillId"), NamePin()},
            {FName("SkillName"), StringPin()},
            {FName("Description"), TextPin()},
            {FName("Selected"), BoolPin()},
            {FName("Excluded"), BoolPin()},
        }
    );
    UK2Node_VariableSet* StoreBridge = AddVariableSet(Graph, PassiveEntryBridgeVariableName, -1320, -600);
    UK2Node_VariableSet* StoreSkillId = AddVariableSet(Graph, PassiveEntrySkillIdVariableName, -1040, -600);
    UK2Node_VariableSet* StoreSkillName = AddVariableSet(Graph, PassiveEntrySkillNameVariableName, -760, -600);
    UK2Node_VariableSet* StoreSelected = AddVariableSet(Graph, PassiveEntrySelectedVariableName, -480, -600);
    UK2Node_VariableSet* StoreExcluded = AddVariableSet(Graph, PassiveEntryExcludedVariableName, -200, -600);
    UK2Node_CallFunction* InitialExcludedPrefix = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), -760, -400);
    UK2Node_CallFunction* InitialDisplayName = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), -480, -400);
    UK2Node_VariableGet* LabelGet = AddVariableGet(Graph, Label->GetFName(), -200, -400);
    UK2Node_CallFunction* NameToText = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 80, -400);
    UK2Node_CallFunction* SetLabel = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), 360, -600);
    UK2Node_Self* Self = AddSelfNode(Graph, -200, -280);
    UK2Node_CallFunction* GetController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), 80, -280);
    UK2Node_CallFunction* CreateTooltip = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), 80, -600);
    UK2Node_DynamicCast* CastTooltip = AddDynamicCast(Graph, PassiveTooltipClass, 360, -600);
    UK2Node_CallFunction* InitializeTooltip = AddStaticCall(
        Graph, PassiveTooltipClass, *PassiveTooltipInitializeEventName.ToString(), 640, -600);
    UK2Node_VariableGet* ToggleGet = AddVariableGet(Graph, Toggle->GetFName(), 640, -360);
    UK2Node_CallFunction* SetTooltip = AddStaticCall(Graph, UWidget::StaticClass(), TEXT("SetToolTip"), 920, -600);
    UK2Node_CallFunction* SetChecked = AddStaticCall(Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), 1200, -600);
    if (!Initialize || !StoreBridge || !StoreSkillId || !StoreSkillName || !StoreSelected || !StoreExcluded
        || !InitialExcludedPrefix || !InitialDisplayName || !LabelGet || !NameToText
        || !SetLabel || !Self || !GetController || !CreateTooltip || !CastTooltip || !InitializeTooltip
        || !ToggleGet || !SetTooltip || !SetChecked
        || !Link(Initialize, UEdGraphSchema_K2::PN_Then, StoreBridge, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("Bridge"), StoreBridge, PassiveEntryBridgeVariableName)
        || !Link(StoreBridge, UEdGraphSchema_K2::PN_Then, StoreSkillId, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("SkillId"), StoreSkillId, PassiveEntrySkillIdVariableName)
        || !Link(StoreSkillId, UEdGraphSchema_K2::PN_Then, StoreSkillName, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("SkillName"), StoreSkillName, PassiveEntrySkillNameVariableName)
        || !Link(StoreSkillName, UEdGraphSchema_K2::PN_Then, StoreSelected, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("Selected"), StoreSelected, PassiveEntrySelectedVariableName)
        || !Link(StoreSelected, UEdGraphSchema_K2::PN_Then, StoreExcluded, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("Excluded"), StoreExcluded, PassiveEntryExcludedVariableName)
        || !Link(StoreExcluded, UEdGraphSchema_K2::PN_Then, SetLabel, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(InitialExcludedPrefix, TEXT("A"), TEXT("[排除] "))
        || !SetPinDefault(InitialExcludedPrefix, TEXT("B"), TEXT(""))
        || !Link(Initialize, TEXT("Excluded"), InitialExcludedPrefix, TEXT("bPickA"))
        || !Link(InitialExcludedPrefix, UEdGraphSchema_K2::PN_ReturnValue, InitialDisplayName, TEXT("A"))
        || !Link(Initialize, TEXT("SkillName"), InitialDisplayName, TEXT("B"))
        || !Link(InitialDisplayName, UEdGraphSchema_K2::PN_ReturnValue, NameToText, TEXT("InString"))
        || !Link(LabelGet, Label->GetFName(), SetLabel, UEdGraphSchema_K2::PN_Self)
        || !Link(NameToText, UEdGraphSchema_K2::PN_ReturnValue, SetLabel, TEXT("InText"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, GetController, TEXT("WorldContextObject"))
        || !SetPinDefault(GetController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetClassPin(CreateTooltip, TEXT("WidgetType"), PassiveTooltipClass)
        || !Link(GetController, UEdGraphSchema_K2::PN_ReturnValue, CreateTooltip, TEXT("OwningPlayer"))
        || !Link(SetLabel, UEdGraphSchema_K2::PN_Then, CreateTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateTooltip, UEdGraphSchema_K2::PN_Then, CastTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateTooltip, UEdGraphSchema_K2::PN_ReturnValue, CastTooltip, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastTooltip, UEdGraphSchema_K2::PN_CastSucceeded, InitializeTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastTooltip, CastTooltip->GetCastResultPin()->PinName, InitializeTooltip, UEdGraphSchema_K2::PN_Self)
        || !Link(Initialize, TEXT("Description"), InitializeTooltip, TEXT("Description"))
        || !Link(InitializeTooltip, UEdGraphSchema_K2::PN_Then, SetTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(ToggleGet, Toggle->GetFName(), SetTooltip, UEdGraphSchema_K2::PN_Self)
        || !Link(CastTooltip, CastTooltip->GetCastResultPin()->PinName, SetTooltip, TEXT("Widget"))
        || !Link(SetTooltip, UEdGraphSchema_K2::PN_Then, SetChecked, UEdGraphSchema_K2::PN_Execute)
        || !Link(ToggleGet, Toggle->GetFName(), SetChecked, UEdGraphSchema_K2::PN_Self)
        || !Link(Initialize, TEXT("Selected"), SetChecked, TEXT("InIsChecked"))) {
        return false;
    }

    UK2Node_ComponentBoundEvent* Changed = AddCheckBoxStateChangedEvent(Blueprint, Toggle, -1600, 200);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PassiveEntryBridgeVariableName, -1320, 360);
    UK2Node_VariableGet* SkillIdGet = AddVariableGet(Graph, PassiveEntrySkillIdVariableName, -1320, 480);
    UK2Node_VariableGet* FilterIdsGetAdd = AddExternalVariableGet(Graph, PassiveFilterIdsVariableName, ModActorClass, -1040, 80);
    UK2Node_CallArrayFunction* FilterCount = AddArrayCall(Graph, TEXT("Array_Length"), -760, -120);
    UK2Node_CallFunction* HasCapacity = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Less_IntInt"), -480, -120);
    UK2Node_IfThenElse* CapacityBranch = AddBranch(Graph, -760, 200);
    UK2Node_CallArrayFunction* AddUnique = AddArrayCall(Graph, TEXT("Array_AddUnique"), -760, 80);
    UK2Node_VariableGet* FilterIdsGetRemove = AddExternalVariableGet(Graph, PassiveFilterIdsVariableName, ModActorClass, -1040, 600);
    UK2Node_CallArrayFunction* RemoveItem = AddArrayCall(Graph, TEXT("Array_RemoveItem"), -760, 600);
    UK2Node_IfThenElse* ChangedBranch = AddBranch(Graph, -1040, 200);
    UK2Node_VariableGet* CapacityToggleGet = AddVariableGet(Graph, Toggle->GetFName(), -480, 360);
    UK2Node_CallFunction* RejectFifthSelection = AddStaticCall(Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), -200, 280);
    UK2Node_VariableGet* ExcludeIdsGetForInclude = AddExternalVariableGet(Graph, PassiveExcludeIdsVariableName, ModActorClass, -760, -360);
    UK2Node_CallArrayFunction* RemoveExclusionForInclude = AddArrayCall(Graph, TEXT("Array_RemoveItem"), -480, -360);
    UK2Node_VariableSet* StoreNotExcludedForInclude = AddVariableSet(Graph, PassiveEntryExcludedVariableName, -200, -360);
    UK2Node_VariableGet* IncludeSkillNameGet = AddVariableGet(Graph, PassiveEntrySkillNameVariableName, 80, -200);
    UK2Node_CallFunction* IncludeSkillNameToText = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 360, -200);
    UK2Node_VariableGet* IncludeLabelGet = AddVariableGet(Graph, Label->GetFName(), 360, -80);
    UK2Node_CallFunction* SetIncludeLabel = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), 640, 80);
    if (!Changed || !BridgeGet || !SkillIdGet || !FilterIdsGetAdd || !FilterCount || !HasCapacity
        || !CapacityBranch || !AddUnique || !CapacityToggleGet || !RejectFifthSelection
        || !ExcludeIdsGetForInclude || !RemoveExclusionForInclude || !StoreNotExcludedForInclude
        || !IncludeSkillNameGet || !IncludeSkillNameToText || !IncludeLabelGet || !SetIncludeLabel
        || !FilterIdsGetRemove || !RemoveItem || !ChangedBranch
        || !Link(Changed, UEdGraphSchema_K2::PN_Then, ChangedBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(Changed, TEXT("bIsChecked"), ChangedBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ChangedBranch, UEdGraphSchema_K2::PN_Then, CapacityBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, FilterIdsGetAdd, UEdGraphSchema_K2::PN_Self)
        || !Link(FilterIdsGetAdd, PassiveFilterIdsVariableName, FilterCount, TEXT("TargetArray"))
        || !Link(FilterCount, UEdGraphSchema_K2::PN_ReturnValue, HasCapacity, TEXT("A"))
        || !SetPinDefault(HasCapacity, TEXT("B"), TEXT("4"))
        || !Link(HasCapacity, UEdGraphSchema_K2::PN_ReturnValue, CapacityBranch, UEdGraphSchema_K2::PN_Condition)
        // __DEPRECATED_20260718__ [reason: selecting a passive first removes the mutually-exclusive exclusion]
        // || !Link(CapacityBranch, UEdGraphSchema_K2::PN_Then, AddUnique, UEdGraphSchema_K2::PN_Execute)
        || !Link(CapacityBranch, UEdGraphSchema_K2::PN_Then, RemoveExclusionForInclude, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, ExcludeIdsGetForInclude, UEdGraphSchema_K2::PN_Self)
        || !Link(ExcludeIdsGetForInclude, PassiveExcludeIdsVariableName, RemoveExclusionForInclude, TEXT("TargetArray"))
        || !Link(SkillIdGet, PassiveEntrySkillIdVariableName, RemoveExclusionForInclude, TEXT("Item"))
        || !Link(RemoveExclusionForInclude, UEdGraphSchema_K2::PN_Then, StoreNotExcludedForInclude, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(StoreNotExcludedForInclude, PassiveEntryExcludedVariableName, TEXT("false"))
        || !Link(StoreNotExcludedForInclude, UEdGraphSchema_K2::PN_Then, AddUnique, UEdGraphSchema_K2::PN_Execute)
        || !Link(FilterIdsGetAdd, PassiveFilterIdsVariableName, AddUnique, TEXT("TargetArray"))
        || !Link(SkillIdGet, PassiveEntrySkillIdVariableName, AddUnique, TEXT("NewItem"))
        || !Link(AddUnique, UEdGraphSchema_K2::PN_Then, SetIncludeLabel, UEdGraphSchema_K2::PN_Execute)
        || !Link(IncludeSkillNameGet, PassiveEntrySkillNameVariableName, IncludeSkillNameToText, TEXT("InString"))
        || !Link(IncludeLabelGet, Label->GetFName(), SetIncludeLabel, UEdGraphSchema_K2::PN_Self)
        || !Link(IncludeSkillNameToText, UEdGraphSchema_K2::PN_ReturnValue, SetIncludeLabel, TEXT("InText"))
        || !Link(CapacityBranch, UEdGraphSchema_K2::PN_Else, RejectFifthSelection, UEdGraphSchema_K2::PN_Execute)
        || !Link(CapacityToggleGet, Toggle->GetFName(), RejectFifthSelection, UEdGraphSchema_K2::PN_Self)
        || !SetPinDefault(RejectFifthSelection, TEXT("InIsChecked"), TEXT("false"))
        || !Link(ChangedBranch, UEdGraphSchema_K2::PN_Else, RemoveItem, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PassiveEntryBridgeVariableName, FilterIdsGetRemove, UEdGraphSchema_K2::PN_Self)
        || !Link(FilterIdsGetRemove, PassiveFilterIdsVariableName, RemoveItem, TEXT("TargetArray"))
        || !Link(SkillIdGet, PassiveEntrySkillIdVariableName, RemoveItem, TEXT("Item"))) {
        return false;
    }
    UEdGraphNode* AddTail = SetIncludeLabel;
    UEdGraphNode* RemoveTail = RemoveItem;
    if (!AppendPassiveTokenMutation(
            Graph, ModActorClass, BridgeGet, SkillIdGet, PassiveExcludeTextVariableName, false, AddTail, 920, -360)
        || !AppendPassiveTokenMutation(
            Graph, ModActorClass, BridgeGet, SkillIdGet, PassiveIncludeTextVariableName, true, AddTail, 2520, -360)
        || !AppendPassiveTokenMutation(
            Graph, ModActorClass, BridgeGet, SkillIdGet, PassiveIncludeTextVariableName, false, RemoveTail, 920, 600)
        || !AppendExternalIntegerIncrement(Graph, ModActorClass, BridgeGet, PassiveFilterRevisionVariableName, AddTail, 4120, 80)
        || !AppendExternalIntegerIncrement(Graph, ModActorClass, BridgeGet, ControlRevisionVariableName, AddTail, 320, 80)
        || !AppendExternalIntegerIncrement(Graph, ModActorClass, BridgeGet, PassiveFilterRevisionVariableName, RemoveTail, 2520, 600)
        || !AppendExternalIntegerIncrement(Graph, ModActorClass, BridgeGet, ControlRevisionVariableName, RemoveTail, 320, 600)) {
        return false;
    }

    // __DEPRECATED_20260718__ [reason: OnMouseButtonDown must be an override function graph in UE 5.1]
    // UK2Node_Event* MouseButtonDown = AddOverrideEvent(
    //     Blueprint, Graph, UUserWidget::StaticClass(), TEXT("OnMouseButtonDown"), -1600, 1120);
    UEdGraph* MouseGraph = nullptr;
    UK2Node_FunctionEntry* MouseButtonDown = nullptr;
    UK2Node_FunctionResult* MouseUnhandledResult = nullptr;
    if (!PrepareOverrideFunctionGraph(
            Blueprint,
            UUserWidget::StaticClass(),
            TEXT("OnMouseButtonDown"),
            MouseGraph,
            MouseButtonDown,
            MouseUnhandledResult)) {
        return false;
    }
    UK2Node_FunctionResult* MouseExcludedResult = AddFunctionResultNode(MouseGraph, MouseButtonDown, 3320, 1520);
    UK2Node_FunctionResult* MouseUnexcludedResult = AddFunctionResultNode(MouseGraph, MouseButtonDown, 3320, 1040);
    UK2Node_CallFunction* IsRightMouseButton = AddStaticCall(
        MouseGraph, UKismetInputLibrary::StaticClass(), TEXT("PointerEvent_IsMouseButtonDown"), -1320, 1280);
    UK2Node_IfThenElse* RightMouseBranch = AddBranch(MouseGraph, -1040, 1120);
    UK2Node_CallFunction* UnhandledReply = AddStaticCall(
        MouseGraph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Unhandled"), -1320, 1440);
    UK2Node_VariableGet* RightExcludedGet = AddVariableGet(MouseGraph, PassiveEntryExcludedVariableName, -760, 1280);
    UK2Node_IfThenElse* RightExcludedBranch = AddBranch(MouseGraph, -760, 1120);
    UK2Node_VariableGet* RightBridgeGet = AddVariableGet(MouseGraph, PassiveEntryBridgeVariableName, -760, 1440);
    UK2Node_VariableGet* RightSkillIdGet = AddVariableGet(MouseGraph, PassiveEntrySkillIdVariableName, -760, 1560);
    UK2Node_VariableGet* RightSkillNameGet = AddVariableGet(MouseGraph, PassiveEntrySkillNameVariableName, -760, 1680);

    UK2Node_VariableGet* ExcludeIdsGetForRemove = AddExternalVariableGet(
        MouseGraph, PassiveExcludeIdsVariableName, ModActorClass, -480, 1040);
    UK2Node_CallArrayFunction* RemoveRightExclusion = AddArrayCall(MouseGraph, TEXT("Array_RemoveItem"), -200, 1040);
    UK2Node_VariableSet* StoreRightNotExcluded = AddVariableSet(MouseGraph, PassiveEntryExcludedVariableName, 80, 1040);
    UK2Node_CallFunction* RightUnexcludedNameToText = AddStaticCall(
        MouseGraph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 360, 1200);
    UK2Node_VariableGet* RightUnexcludedLabelGet = AddVariableGet(MouseGraph, Label->GetFName(), 360, 1320);
    UK2Node_CallFunction* SetRightUnexcludedLabel = AddStaticCall(
        MouseGraph, UTextBlock::StaticClass(), TEXT("SetText"), 640, 1040);

    UK2Node_VariableGet* ExcludeIdsGetForAdd = AddExternalVariableGet(
        MouseGraph, PassiveExcludeIdsVariableName, ModActorClass, -480, 1520);
    UK2Node_CallArrayFunction* AddRightExclusion = AddArrayCall(MouseGraph, TEXT("Array_AddUnique"), -200, 1520);
    UK2Node_VariableGet* IncludeIdsGetForExclude = AddExternalVariableGet(
        MouseGraph, PassiveFilterIdsVariableName, ModActorClass, 80, 1680);
    UK2Node_CallArrayFunction* RemoveRightInclusion = AddArrayCall(MouseGraph, TEXT("Array_RemoveItem"), 80, 1520);
    UK2Node_VariableSet* StoreRightExcluded = AddVariableSet(MouseGraph, PassiveEntryExcludedVariableName, 360, 1520);
    UK2Node_VariableGet* RightToggleGet = AddVariableGet(MouseGraph, Toggle->GetFName(), 360, 1680);
    UK2Node_CallFunction* UncheckRightExcluded = AddStaticCall(
        MouseGraph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), 640, 1520);
    UK2Node_CallFunction* RightExcludedPrefix = AddStaticCall(
        MouseGraph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 640, 1760);
    UK2Node_CallFunction* RightExcludedNameToText = AddStaticCall(
        MouseGraph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 920, 1760);
    UK2Node_VariableGet* RightExcludedLabelGet = AddVariableGet(MouseGraph, Label->GetFName(), 920, 1880);
    UK2Node_CallFunction* SetRightExcludedLabel = AddStaticCall(
        MouseGraph, UTextBlock::StaticClass(), TEXT("SetText"), 1200, 1520);

    if (!MouseGraph || !MouseButtonDown || !MouseUnhandledResult || !MouseExcludedResult || !MouseUnexcludedResult
        || !IsRightMouseButton || !RightMouseBranch || !UnhandledReply
        || !RightExcludedGet || !RightExcludedBranch || !RightBridgeGet || !RightSkillIdGet || !RightSkillNameGet
        || !ExcludeIdsGetForRemove || !RemoveRightExclusion || !StoreRightNotExcluded
        || !RightUnexcludedNameToText || !RightUnexcludedLabelGet || !SetRightUnexcludedLabel
        || !ExcludeIdsGetForAdd || !AddRightExclusion || !IncludeIdsGetForExclude || !RemoveRightInclusion
        || !StoreRightExcluded || !RightToggleGet || !UncheckRightExcluded
        || !RightExcludedPrefix || !RightExcludedNameToText || !RightExcludedLabelGet || !SetRightExcludedLabel
        || !Link(MouseButtonDown, TEXT("MouseEvent"), IsRightMouseButton, TEXT("Input"))
        || !SetPinDefault(IsRightMouseButton, TEXT("MouseButton"), TEXT("RightMouseButton"))
        || !Link(MouseButtonDown, UEdGraphSchema_K2::PN_Then, RightMouseBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(IsRightMouseButton, UEdGraphSchema_K2::PN_ReturnValue, RightMouseBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(UnhandledReply, UEdGraphSchema_K2::PN_ReturnValue, MouseUnhandledResult, UEdGraphSchema_K2::PN_ReturnValue)
        || !Link(UnhandledReply, UEdGraphSchema_K2::PN_ReturnValue, MouseExcludedResult, UEdGraphSchema_K2::PN_ReturnValue)
        || !Link(UnhandledReply, UEdGraphSchema_K2::PN_ReturnValue, MouseUnexcludedResult, UEdGraphSchema_K2::PN_ReturnValue)
        || !Link(RightMouseBranch, UEdGraphSchema_K2::PN_Else, MouseUnhandledResult, UEdGraphSchema_K2::PN_Execute)
        || !Link(RightMouseBranch, UEdGraphSchema_K2::PN_Then, RightExcludedBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(RightExcludedGet, PassiveEntryExcludedVariableName, RightExcludedBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(RightBridgeGet, PassiveEntryBridgeVariableName, ExcludeIdsGetForRemove, UEdGraphSchema_K2::PN_Self)
        || !Link(ExcludeIdsGetForRemove, PassiveExcludeIdsVariableName, RemoveRightExclusion, TEXT("TargetArray"))
        || !Link(RightSkillIdGet, PassiveEntrySkillIdVariableName, RemoveRightExclusion, TEXT("Item"))
        || !Link(RightExcludedBranch, UEdGraphSchema_K2::PN_Then, RemoveRightExclusion, UEdGraphSchema_K2::PN_Execute)
        || !Link(RemoveRightExclusion, UEdGraphSchema_K2::PN_Then, StoreRightNotExcluded, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(StoreRightNotExcluded, PassiveEntryExcludedVariableName, TEXT("false"))
        || !Link(StoreRightNotExcluded, UEdGraphSchema_K2::PN_Then, SetRightUnexcludedLabel, UEdGraphSchema_K2::PN_Execute)
        || !Link(RightSkillNameGet, PassiveEntrySkillNameVariableName, RightUnexcludedNameToText, TEXT("InString"))
        || !Link(RightUnexcludedLabelGet, Label->GetFName(), SetRightUnexcludedLabel, UEdGraphSchema_K2::PN_Self)
        || !Link(RightUnexcludedNameToText, UEdGraphSchema_K2::PN_ReturnValue, SetRightUnexcludedLabel, TEXT("InText"))
        || !Link(RightBridgeGet, PassiveEntryBridgeVariableName, ExcludeIdsGetForAdd, UEdGraphSchema_K2::PN_Self)
        || !Link(ExcludeIdsGetForAdd, PassiveExcludeIdsVariableName, AddRightExclusion, TEXT("TargetArray"))
        || !Link(RightSkillIdGet, PassiveEntrySkillIdVariableName, AddRightExclusion, TEXT("NewItem"))
        || !Link(RightExcludedBranch, UEdGraphSchema_K2::PN_Else, AddRightExclusion, UEdGraphSchema_K2::PN_Execute)
        || !Link(AddRightExclusion, UEdGraphSchema_K2::PN_Then, RemoveRightInclusion, UEdGraphSchema_K2::PN_Execute)
        || !Link(RightBridgeGet, PassiveEntryBridgeVariableName, IncludeIdsGetForExclude, UEdGraphSchema_K2::PN_Self)
        || !Link(IncludeIdsGetForExclude, PassiveFilterIdsVariableName, RemoveRightInclusion, TEXT("TargetArray"))
        || !Link(RightSkillIdGet, PassiveEntrySkillIdVariableName, RemoveRightInclusion, TEXT("Item"))
        || !Link(RemoveRightInclusion, UEdGraphSchema_K2::PN_Then, StoreRightExcluded, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(StoreRightExcluded, PassiveEntryExcludedVariableName, TEXT("true"))
        || !Link(StoreRightExcluded, UEdGraphSchema_K2::PN_Then, UncheckRightExcluded, UEdGraphSchema_K2::PN_Execute)
        || !Link(RightToggleGet, Toggle->GetFName(), UncheckRightExcluded, UEdGraphSchema_K2::PN_Self)
        || !SetPinDefault(UncheckRightExcluded, TEXT("InIsChecked"), TEXT("false"))
        || !Link(UncheckRightExcluded, UEdGraphSchema_K2::PN_Then, SetRightExcludedLabel, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(RightExcludedPrefix, TEXT("A"), TEXT("[排除] "))
        || !Link(RightSkillNameGet, PassiveEntrySkillNameVariableName, RightExcludedPrefix, TEXT("B"))
        || !Link(RightExcludedPrefix, UEdGraphSchema_K2::PN_ReturnValue, RightExcludedNameToText, TEXT("InString"))
        || !Link(RightExcludedLabelGet, Label->GetFName(), SetRightExcludedLabel, UEdGraphSchema_K2::PN_Self)
        || !Link(RightExcludedNameToText, UEdGraphSchema_K2::PN_ReturnValue, SetRightExcludedLabel, TEXT("InText"))) {
        return false;
    }
    UEdGraphNode* RightExcludeAddTail = SetRightExcludedLabel;
    UEdGraphNode* RightExcludeRemoveTail = SetRightUnexcludedLabel;
    if (!AppendPassiveTokenMutation(
            MouseGraph, ModActorClass, RightBridgeGet, RightSkillIdGet,
            PassiveIncludeTextVariableName, false, RightExcludeAddTail, 1480, 1680)
        || !AppendPassiveTokenMutation(
            MouseGraph, ModActorClass, RightBridgeGet, RightSkillIdGet,
            PassiveExcludeTextVariableName, true, RightExcludeAddTail, 3080, 1680)
        || !AppendPassiveTokenMutation(
            MouseGraph, ModActorClass, RightBridgeGet, RightSkillIdGet,
            PassiveExcludeTextVariableName, false, RightExcludeRemoveTail, 920, 1040)
        || !AppendExternalIntegerIncrement(
            MouseGraph, ModActorClass, RightBridgeGet, PassiveFilterRevisionVariableName, RightExcludeAddTail, 1480, 1520)
        || !AppendExternalIntegerIncrement(
            MouseGraph, ModActorClass, RightBridgeGet, ControlRevisionVariableName, RightExcludeAddTail, 2280, 1520)
        || !AppendExternalIntegerIncrement(
            MouseGraph, ModActorClass, RightBridgeGet, PassiveFilterRevisionVariableName, RightExcludeRemoveTail, 920, 1040)
        || !AppendExternalIntegerIncrement(
            MouseGraph, ModActorClass, RightBridgeGet, ControlRevisionVariableName, RightExcludeRemoveTail, 1720, 1040)
        || !Link(RightExcludeAddTail, UEdGraphSchema_K2::PN_Then, MouseExcludedResult, UEdGraphSchema_K2::PN_Execute)
        || !Link(RightExcludeRemoveTail, UEdGraphSchema_K2::PN_Then, MouseUnexcludedResult, UEdGraphSchema_K2::PN_Execute)) {
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPassiveEntry compile status=%d event_nodes=%d mouse_nodes=%d"),
        static_cast<int32>(Blueprint->Status), Graph->Nodes.Num(), MouseGraph->Nodes.Num());
    return Blueprint->Status != BS_Error;
}

bool AppendPalSelectionSummaryUpdate(
    UEdGraph* Graph,
    UClass* ModActorClass,
    UK2Node_VariableGet* BridgeGet,
    bool bAdding,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    UK2Node_VariableGet* SummaryGet = AddVariableGet(Graph, PalEntrySelectionSummaryVariableName, X, Y + 120);
    UK2Node_CallFunction* GetCurrentText = AddStaticCall(
        Graph, UTextBlock::StaticClass(), TEXT("GetText"), X + 260, Y + 120);
    UK2Node_CallFunction* CurrentTextToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), X + 520, Y + 120);
    UK2Node_VariableGet* DisplayNameGet = AddVariableGet(Graph, PalEntryDisplayNameVariableName, X + 520, Y + 260);
    UK2Node_VariableGet* LanguageGet = AddExternalVariableGet(
        Graph, LanguageIdVariableName, ModActorClass, X, Y + 380);
    UK2Node_CallFunction* IsEnglish = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 260, Y + 380);
    UK2Node_CallFunction* Prefix = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 520, Y + 380);
    UK2Node_CallFunction* Delimiter = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 520, Y + 500);
    UK2Node_CallFunction* NoneText = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 520, Y + 620);
    UK2Node_VariableGet* SpeciesGet = AddExternalVariableGet(
        Graph, SpeciesFilterIdsVariableName, ModActorClass, X, Y + 760);
    UK2Node_CallArrayFunction* SpeciesCount = AddArrayCall(Graph, TEXT("Array_Length"), X + 260, Y + 760);
    UK2Node_CallFunction* BoundaryCount = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 520, Y + 760);
    UK2Node_CallFunction* ResultToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 1820, Y + 120);
    UK2Node_CallFunction* SetSummary = AddStaticCall(
        Graph, UTextBlock::StaticClass(), TEXT("SetText"), X + 2080, Y);
    if (!Graph || !ModActorClass || !BridgeGet || !ExecTail || !SummaryGet || !GetCurrentText
        || !CurrentTextToString || !DisplayNameGet || !LanguageGet || !IsEnglish
        || !Prefix || !Delimiter || !NoneText || !SpeciesGet || !SpeciesCount || !BoundaryCount
        || !ResultToText || !SetSummary
        || !Link(SummaryGet, PalEntrySelectionSummaryVariableName, GetCurrentText, UEdGraphSchema_K2::PN_Self)
        || !Link(GetCurrentText, UEdGraphSchema_K2::PN_ReturnValue, CurrentTextToString, TEXT("InText"))
        || !Link(BridgeGet, PalEntryBridgeVariableName, LanguageGet, UEdGraphSchema_K2::PN_Self)
        || !Link(LanguageGet, LanguageIdVariableName, IsEnglish, TEXT("A"))
        || !SetPinDefault(IsEnglish, TEXT("B"), TEXT("1"))
        || !SetPinDefault(Prefix, TEXT("A"), TEXT("Selected: "))
        || !SetPinDefault(Prefix, TEXT("B"), TEXT("已选中："))
        || !Link(IsEnglish, UEdGraphSchema_K2::PN_ReturnValue, Prefix, TEXT("bPickA"))
        || !SetPinDefault(Delimiter, TEXT("A"), TEXT(", "))
        || !SetPinDefault(Delimiter, TEXT("B"), TEXT("、"))
        || !Link(IsEnglish, UEdGraphSchema_K2::PN_ReturnValue, Delimiter, TEXT("bPickA"))
        || !SetPinDefault(NoneText, TEXT("A"), TEXT("Selected: none"))
        || !SetPinDefault(NoneText, TEXT("B"), TEXT("已选中：无"))
        || !Link(IsEnglish, UEdGraphSchema_K2::PN_ReturnValue, NoneText, TEXT("bPickA"))
        || !Link(BridgeGet, PalEntryBridgeVariableName, SpeciesGet, UEdGraphSchema_K2::PN_Self)
        || !Link(SpeciesGet, SpeciesFilterIdsVariableName, SpeciesCount, TEXT("TargetArray"))
        || !Link(SpeciesCount, UEdGraphSchema_K2::PN_ReturnValue, BoundaryCount, TEXT("A"))
        || !SetPinDefault(BoundaryCount, TEXT("B"), bAdding ? TEXT("1") : TEXT("0"))
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetSummary, UEdGraphSchema_K2::PN_Execute)
        || !Link(SummaryGet, PalEntrySelectionSummaryVariableName, SetSummary, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }

    UEdGraphNode* ResultSource = nullptr;
    if (bAdding) {
        UK2Node_CallFunction* PrefixAndName = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 780, Y + 260);
        UK2Node_CallFunction* DelimiterAndName = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 780, Y + 420);
        UK2Node_CallFunction* AppendName = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 1040, Y + 420);
        UK2Node_CallFunction* SelectAddResult = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 1300, Y + 300);
        if (!PrefixAndName || !DelimiterAndName || !AppendName || !SelectAddResult
            || !Link(Prefix, UEdGraphSchema_K2::PN_ReturnValue, PrefixAndName, TEXT("A"))
            || !Link(DisplayNameGet, PalEntryDisplayNameVariableName, PrefixAndName, TEXT("B"))
            || !Link(Delimiter, UEdGraphSchema_K2::PN_ReturnValue, DelimiterAndName, TEXT("A"))
            || !Link(DisplayNameGet, PalEntryDisplayNameVariableName, DelimiterAndName, TEXT("B"))
            || !Link(CurrentTextToString, UEdGraphSchema_K2::PN_ReturnValue, AppendName, TEXT("A"))
            || !Link(DelimiterAndName, UEdGraphSchema_K2::PN_ReturnValue, AppendName, TEXT("B"))
            || !Link(PrefixAndName, UEdGraphSchema_K2::PN_ReturnValue, SelectAddResult, TEXT("A"))
            || !Link(AppendName, UEdGraphSchema_K2::PN_ReturnValue, SelectAddResult, TEXT("B"))
            || !Link(BoundaryCount, UEdGraphSchema_K2::PN_ReturnValue, SelectAddResult, TEXT("bPickA"))) {
            return false;
        }
        ResultSource = SelectAddResult;
    } else {
        UK2Node_CallFunction* DelimiterAndName = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 780, Y + 260);
        UK2Node_CallFunction* NameAndDelimiter = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), X + 780, Y + 420);
        UK2Node_CallFunction* RemoveSuffix = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), X + 1040, Y + 260);
        UK2Node_CallFunction* RemovePrefix = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), X + 1300, Y + 260);
        UK2Node_CallFunction* SelectRemoveResult = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 1560, Y + 260);
        if (!DelimiterAndName || !NameAndDelimiter || !RemoveSuffix || !RemovePrefix || !SelectRemoveResult
            || !Link(Delimiter, UEdGraphSchema_K2::PN_ReturnValue, DelimiterAndName, TEXT("A"))
            || !Link(DisplayNameGet, PalEntryDisplayNameVariableName, DelimiterAndName, TEXT("B"))
            || !Link(DisplayNameGet, PalEntryDisplayNameVariableName, NameAndDelimiter, TEXT("A"))
            || !Link(Delimiter, UEdGraphSchema_K2::PN_ReturnValue, NameAndDelimiter, TEXT("B"))
            || !Link(CurrentTextToString, UEdGraphSchema_K2::PN_ReturnValue, RemoveSuffix, TEXT("SourceString"))
            || !Link(DelimiterAndName, UEdGraphSchema_K2::PN_ReturnValue, RemoveSuffix, TEXT("From"))
            || !SetPinDefault(RemoveSuffix, TEXT("To"), TEXT(""))
            || !Link(RemoveSuffix, UEdGraphSchema_K2::PN_ReturnValue, RemovePrefix, TEXT("SourceString"))
            || !Link(NameAndDelimiter, UEdGraphSchema_K2::PN_ReturnValue, RemovePrefix, TEXT("From"))
            || !SetPinDefault(RemovePrefix, TEXT("To"), TEXT(""))
            || !Link(NoneText, UEdGraphSchema_K2::PN_ReturnValue, SelectRemoveResult, TEXT("A"))
            || !Link(RemovePrefix, UEdGraphSchema_K2::PN_ReturnValue, SelectRemoveResult, TEXT("B"))
            || !Link(BoundaryCount, UEdGraphSchema_K2::PN_ReturnValue, SelectRemoveResult, TEXT("bPickA"))) {
            return false;
        }
        ResultSource = SelectRemoveResult;
    }
    if (!ResultSource
        || !Link(ResultSource, UEdGraphSchema_K2::PN_ReturnValue, ResultToText, TEXT("InString"))
        || !Link(ResultToText, UEdGraphSchema_K2::PN_ReturnValue, SetSummary, TEXT("InText"))) {
        return false;
    }
    ExecTail = SetSummary;
    return true;
}

bool BuildPalEntry(
    UWidgetBlueprint* Blueprint,
    UClass* ModActorClass,
    UClass* PalUtilityClass,
    UClass* DatabaseCharacterParameterClass,
    UClass* PassiveTooltipClass) {
    if (!Blueprint || !Blueprint->WidgetTree || !ModActorClass
        || !PalUtilityClass || !DatabaseCharacterParameterClass || !PassiveTooltipClass) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);
    if (!EnsureMemberVariable(Blueprint, PalEntryBridgeVariableName, ObjectPin(ModActorClass))
        || !EnsureMemberVariable(Blueprint, PalEntryCharacterIdVariableName, NamePin())
        || !EnsureMemberVariable(Blueprint, PalEntryDisplayNameVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, PalEntrySelectionSummaryVariableName, ObjectPin(UTextBlock::StaticClass()))
        || !EnsureMemberVariable(Blueprint, PalEntrySelectedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PalEntryInitializingVariableName, BoolPin(), TEXT("false"))) {
        return false;
    }

    USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("ESP_PalEntrySize"));
    UCheckBox* Toggle = Blueprint->WidgetTree->ConstructWidget<UCheckBox>(
        UCheckBox::StaticClass(), TEXT("ESP_PalEntryToggle"));
    UHorizontalBox* Row = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ESP_PalEntryRow"));
    USizeBox* IconSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(), TEXT("ESP_PalEntryIconSize"));
    UImage* Icon = Blueprint->WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("ESP_PalEntryIcon"));
    UVerticalBox* TextColumn = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ESP_PalEntryTextColumn"));
    UTextBlock* Title = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ESP_PalEntryTitle"));
    UTextBlock* Elements = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ESP_PalEntryElements"));
    UTextBlock* Work = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("ESP_PalEntryWork"));
    if (!Size || !Toggle || !Row || !IconSize || !Icon || !TextColumn
        || !Title || !Elements || !Work) {
        return false;
    }
    Size->SetWidthOverride(350.0f);
    Size->SetHeightOverride(128.0f);
    Toggle->bIsVariable = true;
    Icon->bIsVariable = true;
    Title->bIsVariable = true;
    Elements->bIsVariable = true;
    Work->bIsVariable = true;
    FCheckBoxStyle Style = FCheckBoxStyle::GetDefault();
    Style.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
        .SetUncheckedImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceRaised, 6.0f, PanelV2Style::ToggleOutline, 1.0f))
        .SetUncheckedHoveredImage(FSlateRoundedBoxBrush(PanelV2Style::ToggleUncheckedHover, 6.0f, PanelV2Style::AccentHover, 1.0f))
        .SetUncheckedPressedImage(FSlateRoundedBoxBrush(PanelV2Style::SurfaceHover, 6.0f, PanelV2Style::Accent, 1.0f))
        .SetCheckedImage(FSlateRoundedBoxBrush(FLinearColor(0.07f, 0.30f, 0.18f, 1.0f), 6.0f, PanelV2Style::Accent, 3.0f))
        .SetCheckedHoveredImage(FSlateRoundedBoxBrush(FLinearColor(0.09f, 0.36f, 0.22f, 1.0f), 6.0f, PanelV2Style::AccentHover, 3.0f))
        .SetCheckedPressedImage(FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.25f, 0.15f, 1.0f), 6.0f, PanelV2Style::Accent, 3.0f))
        .SetForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetHoveredForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetPressedForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetCheckedForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetCheckedHoveredForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetCheckedPressedForegroundColor(FSlateColor(PanelV2Style::PrimaryText))
        .SetPadding(FMargin(10.0f, 8.0f));
    Toggle->SetWidgetStyle(Style);
    IconSize->SetWidthOverride(86.0f);
    IconSize->SetHeightOverride(86.0f);
    IconSize->AddChild(Icon);
    Row->AddChild(IconSize);
    Row->AddChild(TextColumn);
    SetHorizontalLayout(IconSize, FMargin(0.0f, 0.0f, 10.0f, 0.0f), ESlateSizeRule::Automatic);
    SetHorizontalLayout(TextColumn, FMargin(0.0f), ESlateSizeRule::Fill);
    auto ConfigureText = [](UTextBlock* Text, int32 SizeValue, const FLinearColor& Color) {
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = SizeValue;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetAutoWrapText(true);
    };
    ConfigureText(Title, 16, PanelV2Style::PrimaryText);
    ConfigureText(Elements, 12, FLinearColor(0.60f, 0.80f, 1.0f, 1.0f));
    ConfigureText(Work, 11, PanelV2Style::SecondaryText);
    TextColumn->AddChild(Title);
    TextColumn->AddChild(Elements);
    TextColumn->AddChild(Work);
    SetVerticalPadding(Elements, FMargin(0.0f, 3.0f, 0.0f, 2.0f));
    Toggle->AddChild(Row);
    Size->AddChild(Toggle);
    Blueprint->WidgetTree->RootWidget = Size;

    const bool bWidgetPropertiesExist = Blueprint->GeneratedClass
        && Blueprint->GeneratedClass->FindPropertyByName(Toggle->GetFName())
        && Blueprint->GeneratedClass->FindPropertyByName(Icon->GetFName())
        && Blueprint->GeneratedClass->FindPropertyByName(Title->GetFName())
        && Blueprint->GeneratedClass->FindPropertyByName(Elements->GetFName())
        && Blueprint->GeneratedClass->FindPropertyByName(Work->GetFName());
    if (!bWidgetPropertiesExist) {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        if (Blueprint->Status == BS_Error) {
            UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] Pal entry WidgetTree compile failed"));
            return false;
        }
    }

    UK2Node_CustomEvent* Initialize = AddCustomEvent(
        Blueprint, Graph, *PalEntryInitializeEventName.ToString(), -1800, -600, {
            {FName("Bridge"), ObjectPin(ModActorClass)},
            {FName("CharacterId"), NamePin()},
            {FName("PaldexNumber"), StringPin()},
            {FName("DisplayName"), StringPin()},
            {FName("SelectionSummary"), ObjectPin(UTextBlock::StaticClass())},
            {FName("ElementText"), StringPin()},
            {FName("WorkText"), StringPin()},
            {FName("Tooltip"), TextPin()},
            {FName("Selected"), BoolPin()},
        });
    UK2Node_VariableSet* StoreInitializing = AddVariableSet(Graph, PalEntryInitializingVariableName, -1520, -600);
    UK2Node_VariableSet* StoreBridge = AddVariableSet(Graph, PalEntryBridgeVariableName, -1240, -600);
    UK2Node_VariableSet* StoreCharacterId = AddVariableSet(Graph, PalEntryCharacterIdVariableName, -960, -600);
    UK2Node_VariableSet* StoreDisplayName = AddVariableSet(Graph, PalEntryDisplayNameVariableName, -680, -600);
    UK2Node_VariableSet* StoreSelectionSummary = AddVariableSet(Graph, PalEntrySelectionSummaryVariableName, -400, -600);
    UK2Node_VariableSet* StoreSelected = AddVariableSet(Graph, PalEntrySelectedVariableName, -120, -600);
    UK2Node_CallFunction* NumberPrefix = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), -1240, -360);
    UK2Node_CallFunction* TitleText = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), -960, -360);
    UK2Node_CallFunction* TitleToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), -680, -360);
    UK2Node_VariableGet* TitleGet = AddVariableGet(Graph, Title->GetFName(), -680, -240);
    UK2Node_CallFunction* SetTitle = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), -400, -600);
    UK2Node_CallFunction* ElementToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), -400, -360);
    UK2Node_VariableGet* ElementGet = AddVariableGet(Graph, Elements->GetFName(), -400, -240);
    UK2Node_CallFunction* SetElements = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), -120, -600);
    UK2Node_CallFunction* WorkToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), -120, -360);
    UK2Node_VariableGet* WorkGet = AddVariableGet(Graph, Work->GetFName(), -120, -240);
    UK2Node_CallFunction* SetWork = AddStaticCall(Graph, UTextBlock::StaticClass(), TEXT("SetText"), 160, -600);
    UK2Node_VariableGet* ToggleGet = AddVariableGet(Graph, Toggle->GetFName(), 160, -360);
    // __DEPRECATED_20260719__ [reason: Pal cards now use the game-aware rich-text tooltip widget]
    // UK2Node_CallFunction* LegacyTextTooltip = AddStaticCall(
    //     Graph, UWidget::StaticClass(), TEXT("SetToolTip" "Text"), 440, -600);
    UK2Node_Self* EntrySelf = AddSelfNode(Graph, 160, -120);
    UK2Node_CallFunction* GetController = AddStaticCall(
        Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), 440, -360);
    UK2Node_CallFunction* CreateTooltip = AddStaticCall(
        Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), 440, -600);
    UK2Node_DynamicCast* CastTooltip = AddDynamicCast(Graph, PassiveTooltipClass, 720, -600);
    UK2Node_CallFunction* InitializeTooltip = AddStaticCall(
        Graph, PassiveTooltipClass, *PassiveTooltipInitializeEventName.ToString(), 1000, -600);
    UK2Node_CallFunction* SetTooltip = AddStaticCall(Graph, UWidget::StaticClass(), TEXT("SetToolTip"), 1280, -600);
    UEdGraphNode* TooltipSetupTail = InitializeTooltip;
    if (EnablePartnerRichTextAudit) {
        UK2Node_CallFunction* AuditCharacterId = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), 1000, -960);
        UK2Node_CallFunction* AuditTooltipText = AddStaticCall(
            Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), 1000, -840);
        UK2Node_CallFunction* AuditEscapeCarriageReturns = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), 1280, -840);
        UK2Node_CallFunction* AuditEscapeNewlines = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), 1560, -840);
        UK2Node_CallFunction* AuditPrefix = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1280, -960);
        UK2Node_CallFunction* AuditTextPrefix = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1560, -960);
        UK2Node_CallFunction* AuditMessage = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1840, -960);
        UK2Node_CallFunction* AuditPrint = AddStaticCall(
            Graph, UKismetSystemLibrary::StaticClass(), TEXT("PrintString"), 2120, -600);
        UK2Node_CallFunction* AuditLine = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2120, -960);
        UK2Node_VariableGet* AuditBridgeGet = AddVariableGet(
            Graph, PalEntryBridgeVariableName, 2120, -840);
        UK2Node_VariableGet* AuditBufferGet = AddExternalVariableGet(
            Graph, RichTextAuditBufferVariableName, ModActorClass, 2400, -840);
        UK2Node_CallFunction* AuditAppend = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 2400, -960);
        UK2Node_VariableSet* AuditBufferSet = AddExternalVariableSet(
            Graph, RichTextAuditBufferVariableName, ModActorClass, 2680, -600);
        if (!AuditCharacterId || !AuditTooltipText || !AuditEscapeCarriageReturns || !AuditEscapeNewlines
            || !AuditPrefix || !AuditTextPrefix || !AuditMessage || !AuditPrint || !AuditLine
            || !AuditBridgeGet || !AuditBufferGet || !AuditAppend || !AuditBufferSet
            || !Link(Initialize, TEXT("CharacterId"), AuditCharacterId, TEXT("InName"))
            || !Link(Initialize, TEXT("Tooltip"), AuditTooltipText, TEXT("InText"))
            || !Link(AuditTooltipText, UEdGraphSchema_K2::PN_ReturnValue, AuditEscapeCarriageReturns, TEXT("SourceString"))
            || !SetPinDefault(AuditEscapeCarriageReturns, TEXT("From"), TEXT("\r"))
            || !SetPinDefault(AuditEscapeCarriageReturns, TEXT("To"), TEXT("\\r"))
            || !Link(AuditEscapeCarriageReturns, UEdGraphSchema_K2::PN_ReturnValue, AuditEscapeNewlines, TEXT("SourceString"))
            || !SetPinDefault(AuditEscapeNewlines, TEXT("From"), TEXT("\n"))
            || !SetPinDefault(AuditEscapeNewlines, TEXT("To"), TEXT("\\n"))
            || !SetPinDefault(AuditPrefix, TEXT("A"), TEXT("character_id="))
            || !Link(AuditCharacterId, UEdGraphSchema_K2::PN_ReturnValue, AuditPrefix, TEXT("B"))
            || !Link(AuditPrefix, UEdGraphSchema_K2::PN_ReturnValue, AuditTextPrefix, TEXT("A"))
            || !SetPinDefault(AuditTextPrefix, TEXT("B"), TEXT(" text="))
            || !Link(AuditTextPrefix, UEdGraphSchema_K2::PN_ReturnValue, AuditMessage, TEXT("A"))
            || !Link(AuditEscapeNewlines, UEdGraphSchema_K2::PN_ReturnValue, AuditMessage, TEXT("B"))
            || !Link(AuditMessage, UEdGraphSchema_K2::PN_ReturnValue, AuditLine, TEXT("A"))
            || !SetPinDefault(AuditLine, TEXT("B"), TEXT("\n"))
            || !Link(AuditBridgeGet, PalEntryBridgeVariableName, AuditBufferGet, UEdGraphSchema_K2::PN_Self)
            || !Link(AuditBufferGet, RichTextAuditBufferVariableName, AuditAppend, TEXT("A"))
            || !Link(AuditLine, UEdGraphSchema_K2::PN_ReturnValue, AuditAppend, TEXT("B"))
            || !Link(AuditBridgeGet, PalEntryBridgeVariableName, AuditBufferSet, UEdGraphSchema_K2::PN_Self)
            || !Link(AuditAppend, UEdGraphSchema_K2::PN_ReturnValue, AuditBufferSet, RichTextAuditBufferVariableName)
            || !Link(InitializeTooltip, UEdGraphSchema_K2::PN_Then, AuditBufferSet, UEdGraphSchema_K2::PN_Execute)
            || !Link(AuditBufferSet, UEdGraphSchema_K2::PN_Then, AuditPrint, UEdGraphSchema_K2::PN_Execute)
            || !Link(EntrySelf, UEdGraphSchema_K2::PN_Self, AuditPrint, TEXT("WorldContextObject"))
            || !Link(AuditMessage, UEdGraphSchema_K2::PN_ReturnValue, AuditPrint, TEXT("InString"))
            || !SetPinDefault(AuditPrint, TEXT("bPrintToScreen"), TEXT("false"))
            || !SetPinDefault(AuditPrint, TEXT("bPrintToLog"), TEXT("true"))) {
            return false;
        }
        TooltipSetupTail = AuditPrint;
    }
    UK2Node_CallFunction* SetChecked = AddStaticCall(Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), 1560, -600);
    UK2Node_VariableSet* ClearInitializing = AddVariableSet(Graph, PalEntryInitializingVariableName, 1840, -600);
    UK2Node_CallFunction* Database = AddStaticCall(
        Graph, PalUtilityClass, TEXT("GetDatabaseCharacterParameter"), 440, -120);
    UK2Node_CallFunction* IconTexture = AddStaticCall(
        Graph, DatabaseCharacterParameterClass, TEXT("GetCharacterIconTexture"), 720, -120);
    UK2Node_VariableGet* CharacterIdGetForIcon = AddVariableGet(
        Graph, PalEntryCharacterIdVariableName, 720, 40);
    UK2Node_VariableGet* IconGet = AddVariableGet(Graph, Icon->GetFName(), 1000, 40);
    UK2Node_CallFunction* SetIcon = AddStaticCall(Graph, UImage::StaticClass(), TEXT("SetBrushFromSoftTexture"), 1280, -600);
    if (!Initialize || !StoreInitializing || !StoreBridge || !StoreCharacterId || !StoreDisplayName
        || !StoreSelectionSummary || !StoreSelected || !NumberPrefix || !TitleText
        || !TitleToText || !TitleGet || !SetTitle || !ElementToText || !ElementGet || !SetElements
        || !WorkToText || !WorkGet || !SetWork || !ToggleGet || !EntrySelf || !GetController
        || !CreateTooltip || !CastTooltip || !InitializeTooltip || !SetTooltip || !SetChecked || !ClearInitializing
        || !Database || !IconTexture || !CharacterIdGetForIcon || !IconGet || !SetIcon
        || !Link(Initialize, UEdGraphSchema_K2::PN_Then, StoreInitializing, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(StoreInitializing, PalEntryInitializingVariableName, TEXT("true"))
        || !Link(StoreInitializing, UEdGraphSchema_K2::PN_Then, StoreBridge, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("Bridge"), StoreBridge, PalEntryBridgeVariableName)
        || !Link(StoreBridge, UEdGraphSchema_K2::PN_Then, StoreCharacterId, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("CharacterId"), StoreCharacterId, PalEntryCharacterIdVariableName)
        || !Link(StoreCharacterId, UEdGraphSchema_K2::PN_Then, StoreDisplayName, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("DisplayName"), StoreDisplayName, PalEntryDisplayNameVariableName)
        || !Link(StoreDisplayName, UEdGraphSchema_K2::PN_Then, StoreSelectionSummary, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("SelectionSummary"), StoreSelectionSummary, PalEntrySelectionSummaryVariableName)
        || !Link(StoreSelectionSummary, UEdGraphSchema_K2::PN_Then, StoreSelected, UEdGraphSchema_K2::PN_Execute)
        || !Link(Initialize, TEXT("Selected"), StoreSelected, PalEntrySelectedVariableName)
        || !SetPinDefault(NumberPrefix, TEXT("A"), TEXT("#"))
        || !Link(Initialize, TEXT("PaldexNumber"), NumberPrefix, TEXT("B"))
        || !Link(NumberPrefix, UEdGraphSchema_K2::PN_ReturnValue, TitleText, TEXT("A"))
        || !Link(Initialize, TEXT("DisplayName"), TitleText, TEXT("B"))
        || !Link(TitleText, UEdGraphSchema_K2::PN_ReturnValue, TitleToText, TEXT("InString"))
        || !Link(StoreSelected, UEdGraphSchema_K2::PN_Then, SetTitle, UEdGraphSchema_K2::PN_Execute)
        || !Link(TitleGet, Title->GetFName(), SetTitle, UEdGraphSchema_K2::PN_Self)
        || !Link(TitleToText, UEdGraphSchema_K2::PN_ReturnValue, SetTitle, TEXT("InText"))
        || !Link(Initialize, TEXT("ElementText"), ElementToText, TEXT("InString"))
        || !Link(SetTitle, UEdGraphSchema_K2::PN_Then, SetElements, UEdGraphSchema_K2::PN_Execute)
        || !Link(ElementGet, Elements->GetFName(), SetElements, UEdGraphSchema_K2::PN_Self)
        || !Link(ElementToText, UEdGraphSchema_K2::PN_ReturnValue, SetElements, TEXT("InText"))
        || !Link(Initialize, TEXT("WorkText"), WorkToText, TEXT("InString"))
        || !Link(SetElements, UEdGraphSchema_K2::PN_Then, SetWork, UEdGraphSchema_K2::PN_Execute)
        || !Link(WorkGet, Work->GetFName(), SetWork, UEdGraphSchema_K2::PN_Self)
        || !Link(WorkToText, UEdGraphSchema_K2::PN_ReturnValue, SetWork, TEXT("InText"))
        || !Link(EntrySelf, UEdGraphSchema_K2::PN_Self, GetController, TEXT("WorldContextObject"))
        || !SetPinDefault(GetController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetClassPin(CreateTooltip, TEXT("WidgetType"), PassiveTooltipClass)
        || !Link(GetController, UEdGraphSchema_K2::PN_ReturnValue, CreateTooltip, TEXT("OwningPlayer"))
        || !Link(SetWork, UEdGraphSchema_K2::PN_Then, CreateTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateTooltip, UEdGraphSchema_K2::PN_Then, CastTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateTooltip, UEdGraphSchema_K2::PN_ReturnValue, CastTooltip, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastTooltip, UEdGraphSchema_K2::PN_CastSucceeded, InitializeTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastTooltip, CastTooltip->GetCastResultPin()->PinName, InitializeTooltip, UEdGraphSchema_K2::PN_Self)
        || !Link(Initialize, TEXT("Tooltip"), InitializeTooltip, TEXT("Description"))
        || !Link(TooltipSetupTail, UEdGraphSchema_K2::PN_Then, SetTooltip, UEdGraphSchema_K2::PN_Execute)
        || !Link(ToggleGet, Toggle->GetFName(), SetTooltip, UEdGraphSchema_K2::PN_Self)
        || !Link(CastTooltip, CastTooltip->GetCastResultPin()->PinName, SetTooltip, TEXT("Widget"))
        || !Link(SetTooltip, UEdGraphSchema_K2::PN_Then, SetChecked, UEdGraphSchema_K2::PN_Execute)
        || !Link(ToggleGet, Toggle->GetFName(), SetChecked, UEdGraphSchema_K2::PN_Self)
        || !Link(Initialize, TEXT("Selected"), SetChecked, TEXT("InIsChecked"))
        || !Link(EntrySelf, UEdGraphSchema_K2::PN_Self, Database, TEXT("WorldContextObject"))
        || !Link(Database, UEdGraphSchema_K2::PN_ReturnValue, IconTexture, UEdGraphSchema_K2::PN_Self)
        || !Link(CharacterIdGetForIcon, PalEntryCharacterIdVariableName, IconTexture, TEXT("CharacterID"))
        || !Link(SetChecked, UEdGraphSchema_K2::PN_Then, ClearInitializing, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(ClearInitializing, PalEntryInitializingVariableName, TEXT("false"))
        || !Link(ClearInitializing, UEdGraphSchema_K2::PN_Then, SetIcon, UEdGraphSchema_K2::PN_Execute)
        || !Link(IconGet, Icon->GetFName(), SetIcon, UEdGraphSchema_K2::PN_Self)
        || !Link(IconTexture, UEdGraphSchema_K2::PN_ReturnValue, SetIcon, TEXT("SoftTexture"))
        || !SetPinDefault(SetIcon, TEXT("bMatchSize"), TEXT("false"))) {
        return false;
    }

    UK2Node_ComponentBoundEvent* Changed = AddCheckBoxStateChangedEvent(Blueprint, Toggle, -1800, 480);
    UK2Node_VariableGet* InitializingGet = AddVariableGet(Graph, PalEntryInitializingVariableName, -1800, 640);
    UK2Node_CallFunction* NotInitializing = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -1520, 640);
    UK2Node_IfThenElse* ChangedAllowed = AddBranch(Graph, -1240, 480);
    UK2Node_IfThenElse* SelectedBranch = AddBranch(Graph, -1520, 480);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PalEntryBridgeVariableName, -1520, 680);
    UK2Node_VariableGet* CharacterIdGet = AddVariableGet(Graph, PalEntryCharacterIdVariableName, -1520, 800);
    UK2Node_VariableGet* SpeciesGetForAdd = AddExternalVariableGet(
        Graph, SpeciesFilterIdsVariableName, ModActorClass, -1240, 360);
    UK2Node_CallArrayFunction* SpeciesCount = AddArrayCall(Graph, TEXT("Array_Length"), -960, 280);
    UK2Node_CallFunction* HasCapacity = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Less_IntInt"), -680, 280);
    UK2Node_IfThenElse* CapacityBranch = AddBranch(Graph, -960, 480);
    UK2Node_CallArrayFunction* AddSpecies = AddArrayCall(Graph, TEXT("Array_AddUnique"), -680, 480);
    UK2Node_VariableGet* SpeciesGetForRemove = AddExternalVariableGet(
        Graph, SpeciesFilterIdsVariableName, ModActorClass, -1240, 920);
    UK2Node_CallArrayFunction* RemoveSpecies = AddArrayCall(Graph, TEXT("Array_RemoveItem"), -960, 920);
    UK2Node_VariableGet* ToggleGetForReject = AddVariableGet(Graph, Toggle->GetFName(), -680, 760);
    UK2Node_CallFunction* RejectOverflow = AddStaticCall(
        Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), -400, 760);
    if (!Changed || !InitializingGet || !NotInitializing || !ChangedAllowed || !SelectedBranch
        || !BridgeGet || !CharacterIdGet || !SpeciesGetForAdd
        || !SpeciesCount || !HasCapacity || !CapacityBranch || !AddSpecies
        || !SpeciesGetForRemove || !RemoveSpecies || !ToggleGetForReject || !RejectOverflow
        || !Link(Changed, UEdGraphSchema_K2::PN_Then, ChangedAllowed, UEdGraphSchema_K2::PN_Execute)
        || !Link(InitializingGet, PalEntryInitializingVariableName, NotInitializing, TEXT("A"))
        || !Link(NotInitializing, UEdGraphSchema_K2::PN_ReturnValue, ChangedAllowed, UEdGraphSchema_K2::PN_Condition)
        || !Link(ChangedAllowed, UEdGraphSchema_K2::PN_Then, SelectedBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(Changed, TEXT("bIsChecked"), SelectedBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(SelectedBranch, UEdGraphSchema_K2::PN_Then, CapacityBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PalEntryBridgeVariableName, SpeciesGetForAdd, UEdGraphSchema_K2::PN_Self)
        || !Link(SpeciesGetForAdd, SpeciesFilterIdsVariableName, SpeciesCount, TEXT("TargetArray"))
        || !Link(SpeciesCount, UEdGraphSchema_K2::PN_ReturnValue, HasCapacity, TEXT("A"))
        || !SetPinDefault(HasCapacity, TEXT("B"), TEXT("512"))
        || !Link(HasCapacity, UEdGraphSchema_K2::PN_ReturnValue, CapacityBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(CapacityBranch, UEdGraphSchema_K2::PN_Then, AddSpecies, UEdGraphSchema_K2::PN_Execute)
        || !Link(SpeciesGetForAdd, SpeciesFilterIdsVariableName, AddSpecies, TEXT("TargetArray"))
        || !Link(CharacterIdGet, PalEntryCharacterIdVariableName, AddSpecies, TEXT("NewItem"))
        || !Link(CapacityBranch, UEdGraphSchema_K2::PN_Else, RejectOverflow, UEdGraphSchema_K2::PN_Execute)
        || !Link(ToggleGetForReject, Toggle->GetFName(), RejectOverflow, UEdGraphSchema_K2::PN_Self)
        || !SetPinDefault(RejectOverflow, TEXT("InIsChecked"), TEXT("false"))
        || !Link(SelectedBranch, UEdGraphSchema_K2::PN_Else, RemoveSpecies, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PalEntryBridgeVariableName, SpeciesGetForRemove, UEdGraphSchema_K2::PN_Self)
        || !Link(SpeciesGetForRemove, SpeciesFilterIdsVariableName, RemoveSpecies, TEXT("TargetArray"))
        || !Link(CharacterIdGet, PalEntryCharacterIdVariableName, RemoveSpecies, TEXT("Item"))) {
        return false;
    }
    UEdGraphNode* AddTail = AddSpecies;
    UEdGraphNode* RemoveTail = RemoveSpecies;
    if (!AppendPassiveTokenMutation(
            Graph, ModActorClass, BridgeGet, CharacterIdGet,
            SpeciesFilterTextVariableName, true, AddTail, -400, 480)
        || !AppendPassiveTokenMutation(
            Graph, ModActorClass, BridgeGet, CharacterIdGet,
            SpeciesFilterTextVariableName, false, RemoveTail, -680, 920)
        || !AppendExternalIntegerIncrement(
            Graph, ModActorClass, BridgeGet, ControlRevisionVariableName, AddTail, 1200, 480)
        || !AppendExternalIntegerIncrement(
            Graph, ModActorClass, BridgeGet, ControlRevisionVariableName, RemoveTail, 920, 920)
        || !AppendPalSelectionSummaryUpdate(Graph, ModActorClass, BridgeGet, true, AddTail, 2000, 480)
        || !AppendPalSelectionSummaryUpdate(Graph, ModActorClass, BridgeGet, false, RemoveTail, 1720, 920)) {
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPalEntry compile status=%d nodes=%d"),
        static_cast<int32>(Blueprint->Status), Graph->Nodes.Num());
    return Blueprint->Status != BS_Error;
}

bool BuildPanelTabEvent(
    UWidgetBlueprint* Blueprint,
    UButton* Button,
    UWidgetSwitcher* Switcher,
    const TArray<UButton*>& TabButtons,
    int32 ActiveIndex,
    int32 Y,
    UClass* ModActorClass = nullptr,
    const FName& PageVariableName = NAME_None) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddButtonEvent(Blueprint, Button, -1200, Y);
    UK2Node_VariableGet* SwitcherGet = AddVariableGet(Graph, Switcher->GetFName(), -940, Y + 120);
    UK2Node_CallFunction* SetIndex = AddStaticCall(Graph, UWidgetSwitcher::StaticClass(), TEXT("SetActiveWidgetIndex"), -660, Y);
    UEdGraphNode* ExecTail = SetIndex;
    if (!Graph || !Event || !SwitcherGet || !SetIndex
        || !SetPinDefault(SetIndex, TEXT("Index"), FString::FromInt(ActiveIndex))
        || !Link(Event, UEdGraphSchema_K2::PN_Then, SetIndex, UEdGraphSchema_K2::PN_Execute)
        || !Link(SwitcherGet, Switcher->GetFName(), SetIndex, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    if (ModActorClass && PageVariableName != NAME_None) {
        UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -380, Y + 120);
        UK2Node_VariableSet* SetPage = AddExternalVariableSet(Graph, PageVariableName, ModActorClass, -120, Y);
        UEdGraphNode* PageTail = SetPage;
        if (!BridgeGet || !SetPage
            || !SetPinDefault(SetPage, PageVariableName, FString::FromInt(ActiveIndex))
            || !Link(SetIndex, UEdGraphSchema_K2::PN_Then, SetPage, UEdGraphSchema_K2::PN_Execute)
            || !Link(BridgeGet, PanelBridgeVariableName, SetPage, UEdGraphSchema_K2::PN_Self)
            || !AppendRevisionIncrement(Graph, ModActorClass, BridgeGet, PageTail, 160, Y)) {
            return false;
        }
        ExecTail = PageTail;
    }
    return AppendPanelSegmentVisualsConstant(Graph, TabButtons, ActiveIndex, ExecTail, 420, Y);
}

bool AppendPanelSegmentVisualsInput(
    UEdGraph* Graph,
    UK2Node_CustomEvent* Event,
    const FName& InputName,
    const TArray<UButton*>& Buttons,
    UEdGraphNode*& ExecTail,
    int32 X,
    int32 Y) {
    if (!Graph || !Event || Buttons.Num() == 0 || Buttons.Contains(nullptr)) {
        return false;
    }
    UK2Node_CallFunction* Clamped = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Clamp"), X, Y + 260);
    if (!Clamped
        || !Link(Event, InputName, Clamped, TEXT("Value"))
        || !SetPinDefault(Clamped, TEXT("Min"), TEXT("0"))
        || !SetPinDefault(Clamped, TEXT("Max"), FString::FromInt(Buttons.Num() - 1))) {
        return false;
    }
    for (int32 Index = 0; Index < Buttons.Num(); ++Index) {
        UK2Node_CallFunction* IsSelected = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 260, Y + Index * 180);
        UK2Node_CallFunction* SelectAlpha = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectFloat"), X + 520, Y + Index * 180);
        UK2Node_CallFunction* SelectColor = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("LinearColorLerp"), X + 780, Y + Index * 180);
        UK2Node_VariableGet* ButtonGet = AddVariableGet(
            Graph, Buttons[Index]->GetFName(), X + 1040, Y + Index * 180 + 300);
        UK2Node_CallFunction* SetBackground = AddStaticCall(
            Graph, UButton::StaticClass(), TEXT("SetBackgroundColor"), X + 1300, Y + Index * 180);
        if (!IsSelected || !SelectAlpha || !SelectColor || !ButtonGet || !SetBackground
            || !SetPinDefault(IsSelected, TEXT("B"), FString::FromInt(Index))
            || !Link(Clamped, UEdGraphSchema_K2::PN_ReturnValue, IsSelected, TEXT("A"))
            || !SetPinDefault(SelectAlpha, TEXT("A"), TEXT("1.0"))
            || !SetPinDefault(SelectAlpha, TEXT("B"), TEXT("0.0"))
            || !Link(IsSelected, UEdGraphSchema_K2::PN_ReturnValue, SelectAlpha, TEXT("bPickA"))
            || !SetPinDefault(SelectColor, TEXT("A"), PanelV2Style::SurfaceRaised.ToString())
            || !SetPinDefault(SelectColor, TEXT("B"), PanelV2Style::Accent.ToString())
            || !Link(SelectAlpha, UEdGraphSchema_K2::PN_ReturnValue, SelectColor, TEXT("Alpha"))
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetBackground, UEdGraphSchema_K2::PN_Execute)
            || !Link(ButtonGet, Buttons[Index]->GetFName(), SetBackground, UEdGraphSchema_K2::PN_Self)
            || !Link(SelectColor, UEdGraphSchema_K2::PN_ReturnValue, SetBackground, TEXT("InBackgroundColor"))) {
            return false;
        }
        ExecTail = SetBackground;
    }
    return true;
}

bool BuildPanelSetPageStateEvent(
    UWidgetBlueprint* Blueprint,
    UWidgetSwitcher* Switcher,
    UWidgetSwitcher* FilterSubSwitcher,
    const TArray<UButton*>& TabButtons,
    const TArray<UButton*>& FilterSubTabButtons,
    int32 Y) {
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_CustomEvent* Event = AddCustomEvent(
        Blueprint, Graph, *PanelSetPageStateEventName.ToString(), -1200, Y,
        {
            TPair<FName, FEdGraphPinType>(FName("MainPage"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("FilterPage"), IntPin())
        });
    UK2Node_VariableGet* SwitcherGet = AddVariableGet(Graph, Switcher->GetFName(), -940, Y + 140);
    UK2Node_VariableGet* FilterSwitcherGet = AddVariableGet(Graph, FilterSubSwitcher->GetFName(), -940, Y + 320);
    UK2Node_CallFunction* SetMain = AddStaticCall(Graph, UWidgetSwitcher::StaticClass(), TEXT("SetActiveWidgetIndex"), -680, Y);
    UK2Node_CallFunction* SetFilter = AddStaticCall(Graph, UWidgetSwitcher::StaticClass(), TEXT("SetActiveWidgetIndex"), -420, Y + 220);
    UEdGraphNode* ExecTail = SetFilter;
    return Graph && Event && SwitcherGet && FilterSwitcherGet && SetMain && SetFilter
        && Link(Event, UEdGraphSchema_K2::PN_Then, SetMain, UEdGraphSchema_K2::PN_Execute)
        && Link(SetMain, UEdGraphSchema_K2::PN_Then, SetFilter, UEdGraphSchema_K2::PN_Execute)
        && Link(SwitcherGet, Switcher->GetFName(), SetMain, UEdGraphSchema_K2::PN_Self)
        && Link(FilterSwitcherGet, FilterSubSwitcher->GetFName(), SetFilter, UEdGraphSchema_K2::PN_Self)
        && Link(Event, TEXT("MainPage"), SetMain, TEXT("Index"))
        && Link(Event, TEXT("FilterPage"), SetFilter, TEXT("Index"))
        && AppendPanelSegmentVisualsInput(Graph, Event, TEXT("MainPage"), TabButtons, ExecTail, -120, Y)
        && AppendPanelSegmentVisualsInput(Graph, Event, TEXT("FilterPage"), FilterSubTabButtons, ExecTail, 1800, Y);
}

bool BuildPanelPalCatalog(
    UWidgetBlueprint* Blueprint,
    UClass* ModActorClass,
    UClass* PalEntryClass,
    UClass* PalUtilityClass,
    UClass* PalUIUtilityClass,
    UClass* DatabaseCharacterParameterClass,
    UScriptStruct* CharacterParameterDatabaseRowStruct,
    UScriptStruct* DropItemDatabaseRowStruct,
    UEnum* ElementEnum,
    UEnum* WorkSuitabilityEnum,
    UClass* MasterDataTablesUtilityClass,
    UEditableTextBox* SearchBox,
    UButton* SearchButton,
    UButton* ClearQueryButton,
    UButton* ClearSelectionButton,
    const TArray<UCheckBox*>& ElementToggles,
    const TArray<UCheckBox*>& WorkToggles,
    UWrapBox* ResultsWrap,
    UTextBlock* ResultCountText,
    UTextBlock* SelectedSummaryText,
    UK2Node_CustomEvent* PopulateEvent,
    UK2Node_CustomEvent* RenderEvent,
    int32 Y) {
    if (!Blueprint || !ModActorClass || !PalEntryClass || !PalUtilityClass || !PalUIUtilityClass
        || !DatabaseCharacterParameterClass || !CharacterParameterDatabaseRowStruct || !DropItemDatabaseRowStruct
        || !ElementEnum || !WorkSuitabilityEnum || !MasterDataTablesUtilityClass
        || !SearchBox || !SearchButton || !ClearQueryButton || !ClearSelectionButton
        || ElementToggles.Num() != 9 || ElementToggles.Contains(nullptr)
        || WorkToggles.Num() != GetPalWorkDefinitions().Num() || WorkToggles.Contains(nullptr)
        || !ResultsWrap || !ResultCountText || !SelectedSummaryText || !PopulateEvent || !RenderEvent) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }

    struct FElementQueryDefinition {
        FString EnumName;
        int32 EnumValue;
    };
    const TArray<FElementQueryDefinition> ElementDefinitions = {
        {TEXT("Normal"), 1}, {TEXT("Fire"), 2}, {TEXT("Water"), 3},
        {TEXT("Leaf"), 4}, {TEXT("Electricity"), 5}, {TEXT("Ice"), 6},
        {TEXT("Earth"), 7}, {TEXT("Dark"), 8}, {TEXT("Dragon"), 9},
    };
    const TArray<FPalWorkDefinition>& WorkDefinitions = GetPalWorkDefinitions();
    for (const FElementQueryDefinition& Definition : ElementDefinitions) {
        if (ElementEnum->GetValueByNameString(Definition.EnumName) == INDEX_NONE) {
            return false;
        }
    }
    for (const FPalWorkDefinition& Definition : WorkDefinitions) {
        if (WorkSuitabilityEnum->GetValueByNameString(Definition.EnumName.ToString()) == INDEX_NONE
            || !CharacterParameterDatabaseRowStruct->FindPropertyByName(Definition.FieldName)) {
            return false;
        }
    }

    // Populate the cached species IDs from the running game's current monster table.
    UK2Node_CallFunction* MakeTablePath = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("MakeSoftObjectPath"), -1640, Y + 160);
    UK2Node_CallFunction* MakeTableReference = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("Conv_SoftObjPathToSoftObjRef"), -1380, Y + 160);
    UK2Node_CallFunction* LoadTable = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("LoadAsset_Blocking"), -1120, Y);
    UK2Node_DynamicCast* CastTable = AddDynamicCast(Graph, UDataTable::StaticClass(), -840, Y);
    UK2Node_VariableSet* StoreTable = AddVariableSet(Graph, PalCatalogTableVariableName, -560, Y);
    UK2Node_VariableGet* CatalogIdsForClear = AddVariableGet(Graph, PalCatalogIdsVariableName, -560, Y + 180);
    UK2Node_CallArrayFunction* ClearCatalogIds = AddArrayCall(Graph, TEXT("Array_Clear"), -280, Y);
    UK2Node_VariableGet* ZukanIndicesForClear = AddVariableGet(Graph, PalCatalogZukanIndicesVariableName, -280, Y + 180);
    UK2Node_CallArrayFunction* ClearZukanIndices = AddArrayCall(Graph, TEXT("Array_Clear"), 0, Y);
    UK2Node_VariableGet* SortKeysForClear = AddVariableGet(Graph, PalCatalogSortKeysVariableName, 0, Y + 180);
    UK2Node_CallArrayFunction* ClearSortKeys = AddArrayCall(Graph, TEXT("Array_Clear"), 280, Y);
    UK2Node_VariableGet* TableForRows = AddVariableGet(Graph, PalCatalogTableVariableName, 0, Y + 180);
    UK2Node_CallFunction* GetRowNames = AddStaticCall(
        Graph, UDataTableFunctionLibrary::StaticClass(), TEXT("GetDataTableRowNames"), 280, Y);
    UK2Node_MacroInstance* ForEachRowName = AddForEachLoop(Graph, 560, Y);
    UK2Node_VariableGet* TableForRow = AddVariableGet(Graph, PalCatalogTableVariableName, 820, Y + 220);
    UK2Node_GetDataTableRow* GetCatalogRow = AddGetDataTableRow(
        Graph, CharacterParameterDatabaseRowStruct, 1080, Y);
    UK2Node_BreakStruct* BreakCatalogRow = AddBreakStruct(
        Graph, CharacterParameterDatabaseRowStruct, 1340, Y + 260);
    UK2Node_VariableGet* SortModeGet = AddVariableGet(Graph, PalCatalogSortModeVariableName, 1600, Y + 820);
    UK2Node_CallFunction* SortModeIsRunSpeed = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 1860, Y + 820);
    UK2Node_CallFunction* SelectRunSpeed = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"), 2900, Y + 700);
    TArray<UK2Node_CallFunction*> SortModeIsWork;
    TArray<UK2Node_CallFunction*> SelectWork;
    for (int32 Index = 0; Index < WorkDefinitions.Num(); ++Index) {
        SortModeIsWork.Add(AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"),
            2120 + Index * 260, Y + 940));
        SelectWork.Add(AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectInt"),
            3160 + Index * 260, Y + 700));
    }
    UK2Node_CallFunction* FinalSortKey = SelectWork.Num() > 0 ? SelectWork.Last() : nullptr;
    UK2Node_CallFunction* ZukanPositive = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Greater_IntInt"), 1600, Y + 200);
    UK2Node_CallFunction* NotBoss = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 1600, Y + 320);
    UK2Node_CallFunction* NotTowerBoss = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 1600, Y + 420);
    UK2Node_CallFunction* NotRaidBoss = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 1600, Y + 520);
    UK2Node_VariableGet* ExistingCatalogIdsForDuplicate = AddVariableGet(
        Graph, PalCatalogIdsVariableName, 1600, Y + 640);
    UK2Node_CallArrayFunction* CatalogIdAlreadyAdded = AddArrayCall(
        Graph, TEXT("Array_Contains"), 1860, Y + 640);
    UK2Node_CallFunction* CatalogIdNotAdded = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 2120, Y + 640);
    TArray<UK2Node_CallFunction*> AdmissionAndNodes;
    for (int32 Index = 0; Index < 5; ++Index) {
        AdmissionAndNodes.Add(AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 1860 + Index * 260, Y + 200));
    }
    UK2Node_IfThenElse* AdmissionBranch = AddBranch(Graph, 3420, Y);
    UK2Node_VariableGet* ZukanIndicesForLength = AddVariableGet(
        Graph, PalCatalogZukanIndicesVariableName, 3680, Y + 160);
    UK2Node_CallArrayFunction* ZukanCount = AddArrayCall(Graph, TEXT("Array_Length"), 3940, Y + 160);
    UK2Node_VariableSet* StoreInsertIndex = AddVariableSet(Graph, PalCatalogInsertIndexVariableName, 4200, Y);
    UK2Node_VariableSet* StoreInsertFound = AddVariableSet(Graph, PalCatalogInsertFoundVariableName, 4460, Y);
    UK2Node_VariableGet* SortKeysForInsertionLoop = AddVariableGet(
        Graph, PalCatalogSortKeysVariableName, 4460, Y + 180);
    UK2Node_MacroInstance* ForEachExistingSortKey = AddForEachLoop(Graph, 4720, Y);
    UK2Node_CallFunction* NewSortKeyGreaterThanExisting = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Greater_IntInt"), 4980, Y + 180);
    UK2Node_CallFunction* NewSortKeyLessThanExisting = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Less_IntInt"), 4980, Y + 300);
    UK2Node_VariableGet* SortDescendingGet = AddVariableGet(Graph, PalCatalogSortDescendingVariableName, 5240, Y + 420);
    UK2Node_CallFunction* NotSortDescending = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 5500, Y + 420);
    UK2Node_CallFunction* DescendingGreater = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 5760, Y + 200);
    UK2Node_CallFunction* AscendingLess = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 5760, Y + 320);
    UK2Node_CallFunction* SortOrderMatches = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 6020, Y + 260);
    UK2Node_VariableGet* InsertFoundGet = AddVariableGet(Graph, PalCatalogInsertFoundVariableName, 4980, Y + 300);
    UK2Node_CallFunction* InsertNotFound = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), 5240, Y + 300);
    UK2Node_CallFunction* ShouldStoreInsertIndex = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), 5500, Y + 220);
    UK2Node_IfThenElse* StoreInsertBranch = AddBranch(Graph, 5760, Y);
    UK2Node_VariableSet* UpdateInsertIndex = AddVariableSet(Graph, PalCatalogInsertIndexVariableName, 6020, Y);
    UK2Node_VariableSet* MarkInsertFound = AddVariableSet(Graph, PalCatalogInsertFoundVariableName, 6280, Y);
    UK2Node_VariableGet* ZukanIndicesForInsert = AddVariableGet(
        Graph, PalCatalogZukanIndicesVariableName, 4980, Y + 520);
    UK2Node_VariableGet* InsertIndexForZukan = AddVariableGet(Graph, PalCatalogInsertIndexVariableName, 5240, Y + 520);
    UK2Node_CallArrayFunction* InsertZukanIndex = AddArrayCall(Graph, TEXT("Array_Insert"), 5500, Y + 440);
    UK2Node_VariableGet* SortKeysForInsert = AddVariableGet(Graph, PalCatalogSortKeysVariableName, 5760, Y + 760);
    UK2Node_VariableGet* InsertIndexForSortKey = AddVariableGet(Graph, PalCatalogInsertIndexVariableName, 6020, Y + 760);
    UK2Node_CallArrayFunction* InsertSortKey = AddArrayCall(Graph, TEXT("Array_Insert"), 6540, Y + 620);
    UK2Node_VariableGet* CatalogIdsForInsert = AddVariableGet(Graph, PalCatalogIdsVariableName, 5760, Y + 620);
    UK2Node_VariableGet* InsertIndexForId = AddVariableGet(Graph, PalCatalogInsertIndexVariableName, 6020, Y + 620);
    UK2Node_CallArrayFunction* InsertCatalogId = AddArrayCall(Graph, TEXT("Array_Insert"), 6280, Y + 440);
    UK2Node_VariableSet* StoreCatalogLoaded = AddVariableSet(Graph, PalCatalogLoadedVariableName, 820, Y + 760);
    UK2Node_Self* PopulateSelf = AddSelfNode(Graph, 1080, Y + 900);
    UK2Node_CallFunction* RenderAfterPopulate = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelRenderPalCatalogEventName.ToString(), 1340, Y + 760);
    if (!MakeTablePath || !MakeTableReference || !LoadTable || !CastTable || !StoreTable
        || !CatalogIdsForClear || !ClearCatalogIds || !ZukanIndicesForClear || !ClearZukanIndices
        || !SortKeysForClear || !ClearSortKeys
        || !TableForRows || !GetRowNames || !ForEachRowName || !TableForRow || !GetCatalogRow
        || !BreakCatalogRow || !SortModeGet || !SortModeIsRunSpeed || !SelectRunSpeed
        || SortModeIsWork.Num() != WorkDefinitions.Num() || SortModeIsWork.Contains(nullptr)
        || SelectWork.Num() != WorkDefinitions.Num() || SelectWork.Contains(nullptr) || !FinalSortKey
        || !ZukanPositive || !NotBoss || !NotTowerBoss || !NotRaidBoss
        || !ExistingCatalogIdsForDuplicate || !CatalogIdAlreadyAdded || !CatalogIdNotAdded
        || AdmissionAndNodes.Num() != 5 || AdmissionAndNodes.Contains(nullptr)
        || !AdmissionBranch || !ZukanIndicesForLength || !ZukanCount || !StoreInsertIndex || !StoreInsertFound
        || !SortKeysForInsertionLoop || !ForEachExistingSortKey || !NewSortKeyGreaterThanExisting
        || !NewSortKeyLessThanExisting || !SortDescendingGet || !NotSortDescending || !DescendingGreater
        || !AscendingLess || !SortOrderMatches || !InsertFoundGet || !InsertNotFound || !ShouldStoreInsertIndex || !StoreInsertBranch
        || !UpdateInsertIndex || !MarkInsertFound || !ZukanIndicesForInsert || !InsertIndexForZukan
        || !InsertZukanIndex || !SortKeysForInsert || !InsertIndexForSortKey || !InsertSortKey
        || !CatalogIdsForInsert || !InsertIndexForId || !InsertCatalogId
        || !StoreCatalogLoaded || !PopulateSelf || !RenderAfterPopulate
        || !SetPinDefault(MakeTablePath, TEXT("PathString"), PalMonsterParameterTablePath)
        || !Link(MakeTablePath, UEdGraphSchema_K2::PN_ReturnValue, MakeTableReference, TEXT("SoftObjectPath"))
        || !Link(PopulateEvent, UEdGraphSchema_K2::PN_Then, LoadTable, UEdGraphSchema_K2::PN_Execute)
        || !Link(MakeTableReference, UEdGraphSchema_K2::PN_ReturnValue, LoadTable, TEXT("Asset"))
        || !Link(LoadTable, UEdGraphSchema_K2::PN_Then, CastTable, UEdGraphSchema_K2::PN_Execute)
        || !Link(LoadTable, UEdGraphSchema_K2::PN_ReturnValue, CastTable, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastTable, CastTable->GetValidCastPin()->PinName, StoreTable, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastTable, CastTable->GetCastResultPin()->PinName, StoreTable, PalCatalogTableVariableName)
        || !Link(StoreTable, UEdGraphSchema_K2::PN_Then, ClearCatalogIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(CatalogIdsForClear, PalCatalogIdsVariableName, ClearCatalogIds, TEXT("TargetArray"))
        || !Link(ClearCatalogIds, UEdGraphSchema_K2::PN_Then, ClearZukanIndices, UEdGraphSchema_K2::PN_Execute)
        || !Link(ZukanIndicesForClear, PalCatalogZukanIndicesVariableName, ClearZukanIndices, TEXT("TargetArray"))
        || !Link(ClearZukanIndices, UEdGraphSchema_K2::PN_Then, ClearSortKeys, UEdGraphSchema_K2::PN_Execute)
        || !Link(SortKeysForClear, PalCatalogSortKeysVariableName, ClearSortKeys, TEXT("TargetArray"))
        || !Link(ClearSortKeys, UEdGraphSchema_K2::PN_Then, GetRowNames, UEdGraphSchema_K2::PN_Execute)
        || !Link(TableForRows, PalCatalogTableVariableName, GetRowNames, TEXT("Table"))
        || !Link(GetRowNames, UEdGraphSchema_K2::PN_Then, ForEachRowName, TEXT("Exec"))
        || !Link(GetRowNames, TEXT("OutRowNames"), ForEachRowName, TEXT("Array"))
        || !Link(ForEachRowName, TEXT("LoopBody"), GetCatalogRow, UEdGraphSchema_K2::PN_Execute)
        || !Link(TableForRow, PalCatalogTableVariableName, GetCatalogRow, TEXT("DataTable"))
        || !Link(ForEachRowName, TEXT("Array Element"), GetCatalogRow, TEXT("RowName"))
        || !Link(GetCatalogRow, UEdGraphSchema_K2::PN_ReturnValue, BreakCatalogRow, CharacterParameterDatabaseRowStruct->GetFName())
        || !Link(SortModeGet, PalCatalogSortModeVariableName, SortModeIsRunSpeed, TEXT("A"))
        || !SetPinDefault(SortModeIsRunSpeed, TEXT("B"), TEXT("1"))
        || !Link(BreakCatalogRow, TEXT("RunSpeed"), SelectRunSpeed, TEXT("A"))
        || !Link(BreakCatalogRow, TEXT("ZukanIndex"), SelectRunSpeed, TEXT("B"))
        || !Link(SortModeIsRunSpeed, UEdGraphSchema_K2::PN_ReturnValue, SelectRunSpeed, TEXT("bPickA"))
        || !Link(BreakCatalogRow, TEXT("ZukanIndex"), ZukanPositive, TEXT("A"))
        || !SetPinDefault(ZukanPositive, TEXT("B"), TEXT("0"))
        || !Link(BreakCatalogRow, TEXT("IsBoss"), NotBoss, TEXT("A"))
        || !Link(BreakCatalogRow, TEXT("IsTowerBoss"), NotTowerBoss, TEXT("A"))
        || !Link(BreakCatalogRow, TEXT("IsRaidBoss"), NotRaidBoss, TEXT("A"))
        || !Link(ExistingCatalogIdsForDuplicate, PalCatalogIdsVariableName, CatalogIdAlreadyAdded, TEXT("TargetArray"))
        || !Link(ForEachRowName, TEXT("Array Element"), CatalogIdAlreadyAdded, TEXT("ItemToFind"))
        || !Link(CatalogIdAlreadyAdded, UEdGraphSchema_K2::PN_ReturnValue, CatalogIdNotAdded, TEXT("A"))
        || !Link(BreakCatalogRow, TEXT("IsPal"), AdmissionAndNodes[0], TEXT("A"))
        || !Link(ZukanPositive, UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[0], TEXT("B"))
        || !Link(AdmissionAndNodes[0], UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[1], TEXT("A"))
        || !Link(NotBoss, UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[1], TEXT("B"))
        || !Link(AdmissionAndNodes[1], UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[2], TEXT("A"))
        || !Link(NotTowerBoss, UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[2], TEXT("B"))
        || !Link(AdmissionAndNodes[2], UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[3], TEXT("A"))
        || !Link(NotRaidBoss, UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[3], TEXT("B"))
        || !Link(AdmissionAndNodes[3], UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[4], TEXT("A"))
        || !Link(CatalogIdNotAdded, UEdGraphSchema_K2::PN_ReturnValue, AdmissionAndNodes[4], TEXT("B"))
        || !Link(GetCatalogRow, UEdGraphSchema_K2::PN_Then, AdmissionBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(AdmissionAndNodes[4], UEdGraphSchema_K2::PN_ReturnValue, AdmissionBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(ZukanIndicesForLength, PalCatalogZukanIndicesVariableName, ZukanCount, TEXT("TargetArray"))
        || !Link(AdmissionBranch, UEdGraphSchema_K2::PN_Then, StoreInsertIndex, UEdGraphSchema_K2::PN_Execute)
        || !Link(ZukanCount, UEdGraphSchema_K2::PN_ReturnValue, StoreInsertIndex, PalCatalogInsertIndexVariableName)
        || !Link(StoreInsertIndex, UEdGraphSchema_K2::PN_Then, StoreInsertFound, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(StoreInsertFound, PalCatalogInsertFoundVariableName, TEXT("false"))
        || !Link(StoreInsertFound, UEdGraphSchema_K2::PN_Then, ForEachExistingSortKey, TEXT("Exec"))
        || !Link(SortKeysForInsertionLoop, PalCatalogSortKeysVariableName, ForEachExistingSortKey, TEXT("Array"))
        || !Link(FinalSortKey, UEdGraphSchema_K2::PN_ReturnValue, NewSortKeyGreaterThanExisting, TEXT("A"))
        || !Link(ForEachExistingSortKey, TEXT("Array Element"), NewSortKeyGreaterThanExisting, TEXT("B"))
        || !Link(FinalSortKey, UEdGraphSchema_K2::PN_ReturnValue, NewSortKeyLessThanExisting, TEXT("A"))
        || !Link(ForEachExistingSortKey, TEXT("Array Element"), NewSortKeyLessThanExisting, TEXT("B"))
        || !Link(SortDescendingGet, PalCatalogSortDescendingVariableName, NotSortDescending, TEXT("A"))
        || !Link(SortDescendingGet, PalCatalogSortDescendingVariableName, DescendingGreater, TEXT("A"))
        || !Link(NewSortKeyGreaterThanExisting, UEdGraphSchema_K2::PN_ReturnValue, DescendingGreater, TEXT("B"))
        || !Link(NotSortDescending, UEdGraphSchema_K2::PN_ReturnValue, AscendingLess, TEXT("A"))
        || !Link(NewSortKeyLessThanExisting, UEdGraphSchema_K2::PN_ReturnValue, AscendingLess, TEXT("B"))
        || !Link(DescendingGreater, UEdGraphSchema_K2::PN_ReturnValue, SortOrderMatches, TEXT("A"))
        || !Link(AscendingLess, UEdGraphSchema_K2::PN_ReturnValue, SortOrderMatches, TEXT("B"))
        || !Link(InsertFoundGet, PalCatalogInsertFoundVariableName, InsertNotFound, TEXT("A"))
        || !Link(SortOrderMatches, UEdGraphSchema_K2::PN_ReturnValue, ShouldStoreInsertIndex, TEXT("A"))
        || !Link(InsertNotFound, UEdGraphSchema_K2::PN_ReturnValue, ShouldStoreInsertIndex, TEXT("B"))
        || !Link(ForEachExistingSortKey, TEXT("LoopBody"), StoreInsertBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(ShouldStoreInsertIndex, UEdGraphSchema_K2::PN_ReturnValue, StoreInsertBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(StoreInsertBranch, UEdGraphSchema_K2::PN_Then, UpdateInsertIndex, UEdGraphSchema_K2::PN_Execute)
        || !Link(ForEachExistingSortKey, TEXT("Array Index"), UpdateInsertIndex, PalCatalogInsertIndexVariableName)
        || !Link(UpdateInsertIndex, UEdGraphSchema_K2::PN_Then, MarkInsertFound, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(MarkInsertFound, PalCatalogInsertFoundVariableName, TEXT("true"))
        || !Link(ForEachExistingSortKey, TEXT("Completed"), InsertZukanIndex, UEdGraphSchema_K2::PN_Execute)
        || !Link(ZukanIndicesForInsert, PalCatalogZukanIndicesVariableName, InsertZukanIndex, TEXT("TargetArray"))
        || !Link(BreakCatalogRow, TEXT("ZukanIndex"), InsertZukanIndex, TEXT("NewItem"))
        || !Link(InsertIndexForZukan, PalCatalogInsertIndexVariableName, InsertZukanIndex, TEXT("Index"))
        || !Link(InsertZukanIndex, UEdGraphSchema_K2::PN_Then, InsertSortKey, UEdGraphSchema_K2::PN_Execute)
        || !Link(SortKeysForInsert, PalCatalogSortKeysVariableName, InsertSortKey, TEXT("TargetArray"))
        || !Link(FinalSortKey, UEdGraphSchema_K2::PN_ReturnValue, InsertSortKey, TEXT("NewItem"))
        || !Link(InsertIndexForSortKey, PalCatalogInsertIndexVariableName, InsertSortKey, TEXT("Index"))
        || !Link(InsertSortKey, UEdGraphSchema_K2::PN_Then, InsertCatalogId, UEdGraphSchema_K2::PN_Execute)
        || !Link(CatalogIdsForInsert, PalCatalogIdsVariableName, InsertCatalogId, TEXT("TargetArray"))
        || !Link(ForEachRowName, TEXT("Array Element"), InsertCatalogId, TEXT("NewItem"))
        || !Link(InsertIndexForId, PalCatalogInsertIndexVariableName, InsertCatalogId, TEXT("Index"))
        || !Link(ForEachRowName, TEXT("Completed"), StoreCatalogLoaded, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(StoreCatalogLoaded, PalCatalogLoadedVariableName, TEXT("true"))
        || !Link(StoreCatalogLoaded, UEdGraphSchema_K2::PN_Then, RenderAfterPopulate, UEdGraphSchema_K2::PN_Execute)
        || !Link(PopulateSelf, UEdGraphSchema_K2::PN_Self, RenderAfterPopulate, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }

    for (int32 Index = 0; Index < WorkDefinitions.Num(); ++Index) {
        UK2Node_CallFunction* PreviousSelect = Index == 0 ? SelectRunSpeed : SelectWork[Index - 1];
        if (!Link(SortModeGet, PalCatalogSortModeVariableName, SortModeIsWork[Index], TEXT("A"))
            || !SetPinDefault(SortModeIsWork[Index], TEXT("B"), FString::FromInt(Index + 2))
            || !Link(BreakCatalogRow, WorkDefinitions[Index].FieldName, SelectWork[Index], TEXT("A"))
            || !Link(PreviousSelect, UEdGraphSchema_K2::PN_ReturnValue, SelectWork[Index], TEXT("B"))
            || !Link(SortModeIsWork[Index], UEdGraphSchema_K2::PN_ReturnValue, SelectWork[Index], TEXT("bPickA"))) {
            return false;
        }
    }

    // Render uses only cached IDs; changing a query does not reload or snapshot the table.
    const int32 RenderY = Y + 1400;
    UK2Node_VariableGet* ResultsWrapForClear = AddVariableGet(Graph, ResultsWrap->GetFName(), -1640, RenderY + 180);
    UK2Node_CallFunction* ClearResults = AddStaticCall(
        Graph, UPanelWidget::StaticClass(), TEXT("ClearChildren"), -1380, RenderY);
    UK2Node_VariableSet* ResetResultCount = AddVariableSet(Graph, PalCatalogResultCountVariableName, -1120, RenderY);
    UK2Node_VariableSet* ClearSelectedNames = AddVariableSet(Graph, PalCatalogSelectedNamesVariableName, -860, RenderY);
    UK2Node_VariableSet* ResetSelectedCount = AddVariableSet(Graph, PalCatalogSelectedCountVariableName, -600, RenderY);
    UK2Node_VariableGet* BridgeForSelectedSummary = AddVariableGet(Graph, PanelBridgeVariableName, -860, RenderY + 320);
    UK2Node_VariableGet* SelectedIdsForSummary = AddExternalVariableGet(
        Graph, SpeciesFilterIdsVariableName, ModActorClass, -600, RenderY + 320);
    UK2Node_MacroInstance* ForEachSelectedId = AddForEachLoop(Graph, -340, RenderY);
    UK2Node_VariableGet* CatalogIdsForRender = AddVariableGet(Graph, PalCatalogIdsVariableName, -1120, RenderY + 180);
    UK2Node_MacroInstance* ForEachCatalogId = AddForEachLoop(Graph, -860, RenderY);
    UK2Node_VariableGet* TableForRenderRow = AddVariableGet(Graph, PalCatalogTableVariableName, -600, RenderY + 220);
    UK2Node_GetDataTableRow* GetRenderRow = AddGetDataTableRow(
        Graph, CharacterParameterDatabaseRowStruct, -340, RenderY);
    UK2Node_BreakStruct* BreakRenderRow = AddBreakStruct(
        Graph, CharacterParameterDatabaseRowStruct, -80, RenderY + 280);
    UK2Node_Self* RenderSelf = AddSelfNode(Graph, -600, RenderY + 620);
    UK2Node_VariableGet* PanelBridgeForLanguage = AddVariableGet(Graph, PanelBridgeVariableName, -600, RenderY + 980);
    UK2Node_VariableGet* LanguageIdForRender = AddExternalVariableGet(
        Graph, LanguageIdVariableName, ModActorClass, -340, RenderY + 980);
    UK2Node_CallFunction* LanguageIsEnglish = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), -80, RenderY + 980);
    UK2Node_CallFunction* CharacterDatabase = AddStaticCall(
        Graph, PalUtilityClass, TEXT("GetDatabaseCharacterParameter"), -340, RenderY + 620);
    UK2Node_CallFunction* GetLocalizedName = AddStaticCall(
        Graph, DatabaseCharacterParameterClass, TEXT("GetLocalizedCharacterName"), -80, RenderY + 620);
    UK2Node_CallFunction* NameToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), 180, RenderY + 620);
    UK2Node_CallFunction* CharacterIdToString = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), 440, RenderY + 620);
    UK2Node_CallFunction* VariantPrefix = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 700, RenderY + 500);
    UK2Node_CallFunction* VariantName = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 960, RenderY + 500);
    UK2Node_CallFunction* NameWithVariant = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1220, RenderY + 500);
    UK2Node_CallFunction* VariantSuffix = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1220, RenderY + 620);
    UK2Node_CallFunction* GetSelectedLocalizedName = AddStaticCall(
        Graph, DatabaseCharacterParameterClass, TEXT("GetLocalizedCharacterName"), -80, RenderY - 420);
    UK2Node_CallFunction* SelectedNameToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), 180, RenderY - 420);
    UK2Node_VariableGet* SelectedCountForSeparator = AddVariableGet(
        Graph, PalCatalogSelectedCountVariableName, 180, RenderY - 300);
    UK2Node_CallFunction* SelectedCountIsZero = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 440, RenderY - 300);
    UK2Node_CallFunction* SelectedPrefix = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 440, RenderY - 540);
    UK2Node_CallFunction* SelectedDelimiter = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 440, RenderY - 420);
    UK2Node_CallFunction* SelectedSeparator = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 700, RenderY - 420);
    UK2Node_CallFunction* SelectedSeparatorAndName = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 960, RenderY - 420);
    UK2Node_VariableGet* CurrentSelectedNames = AddVariableGet(
        Graph, PalCatalogSelectedNamesVariableName, 960, RenderY - 300);
    UK2Node_CallFunction* AppendSelectedName = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), 1220, RenderY - 360);
    UK2Node_VariableSet* StoreSelectedNames = AddVariableSet(
        Graph, PalCatalogSelectedNamesVariableName, 1480, RenderY - 480);
    UK2Node_VariableGet* SelectedCountForIncrement = AddVariableGet(
        Graph, PalCatalogSelectedCountVariableName, 1220, RenderY - 180);
    UK2Node_CallFunction* IncrementSelectedCount = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_IntInt"), 1480, RenderY - 180);
    UK2Node_VariableSet* StoreSelectedCount = AddVariableSet(
        Graph, PalCatalogSelectedCountVariableName, 1740, RenderY - 480);
    UK2Node_VariableGet* FinalSelectedCount = AddVariableGet(
        Graph, PalCatalogSelectedCountVariableName, 1740, RenderY - 180);
    UK2Node_CallFunction* NoSelectedIds = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), 2000, RenderY - 180);
    UK2Node_CallFunction* NoSelectionSummary = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 2000, RenderY - 420);
    UK2Node_VariableGet* FinalSelectedNames = AddVariableGet(
        Graph, PalCatalogSelectedNamesVariableName, 2260, RenderY - 180);
    UK2Node_CallFunction* FinalSelectedSummary = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), 2260, RenderY - 360);
    UK2Node_CallFunction* SelectedSummaryToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), 2520, RenderY - 360);
    UK2Node_VariableGet* SelectedSummaryTextGet = AddVariableGet(
        Graph, SelectedSummaryText->GetFName(), 2520, RenderY - 180);
    UK2Node_CallFunction* SetSelectedSummaryText = AddStaticCall(
        Graph, UTextBlock::StaticClass(), TEXT("SetText"), 2780, RenderY - 480);
    UK2Node_VariableGet* SearchBoxGet = AddVariableGet(Graph, SearchBox->GetFName(), 180, RenderY + 760);
    UK2Node_CallFunction* GetSearchText = AddStaticCall(
        Graph, UEditableTextBox::StaticClass(), TEXT("GetText"), 440, RenderY + 760);
    UK2Node_CallFunction* SearchToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), 700, RenderY + 760);
    UK2Node_CallFunction* SearchEmpty = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("IsEmpty"), 960, RenderY + 760);
    UK2Node_CallFunction* NameContainsSearch = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Contains"), 960, RenderY + 620);
    UK2Node_CallFunction* PaldexNumber = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), 700, RenderY + 880);
    UK2Node_CallFunction* NumberContainsSearch = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Contains"), 960, RenderY + 880);
    UK2Node_CallFunction* NameOrNumberMatches = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 1220, RenderY + 800);
    UK2Node_CallFunction* SearchMatches = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), 1480, RenderY + 740);
    UK2Node_CallFunction* Element1ToInt = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_ByteToInt"), 180, RenderY + 920);
    UK2Node_CallFunction* Element2ToInt = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Conv_ByteToInt"), 440, RenderY + 920);
    if (!ResultsWrapForClear || !ClearResults || !ResetResultCount || !ClearSelectedNames || !ResetSelectedCount
        || !BridgeForSelectedSummary || !SelectedIdsForSummary || !ForEachSelectedId
        || !CatalogIdsForRender || !ForEachCatalogId
        || !TableForRenderRow || !GetRenderRow || !BreakRenderRow || !RenderSelf
        || !PanelBridgeForLanguage || !LanguageIdForRender || !LanguageIsEnglish || !CharacterDatabase
        || !GetLocalizedName || !NameToString || !CharacterIdToString || !VariantPrefix || !VariantName
        || !VariantSuffix || !NameWithVariant || !SearchBoxGet || !GetSearchText || !SearchToString
        || !GetSelectedLocalizedName || !SelectedNameToString || !SelectedCountForSeparator || !SelectedCountIsZero
        || !SelectedPrefix || !SelectedDelimiter || !SelectedSeparator || !SelectedSeparatorAndName
        || !CurrentSelectedNames || !AppendSelectedName || !StoreSelectedNames
        || !SelectedCountForIncrement || !IncrementSelectedCount || !StoreSelectedCount
        || !FinalSelectedCount || !NoSelectedIds || !NoSelectionSummary || !FinalSelectedNames
        || !FinalSelectedSummary || !SelectedSummaryToText || !SelectedSummaryTextGet || !SetSelectedSummaryText
        || !SearchEmpty || !NameContainsSearch || !PaldexNumber || !NumberContainsSearch
        || !NameOrNumberMatches || !SearchMatches || !Element1ToInt || !Element2ToInt
        || !Link(RenderEvent, UEdGraphSchema_K2::PN_Then, ClearResults, UEdGraphSchema_K2::PN_Execute)
        || !Link(ResultsWrapForClear, ResultsWrap->GetFName(), ClearResults, UEdGraphSchema_K2::PN_Self)
        || !Link(ClearResults, UEdGraphSchema_K2::PN_Then, ResetResultCount, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(ResetResultCount, PalCatalogResultCountVariableName, TEXT("0"))
        || !Link(ResetResultCount, UEdGraphSchema_K2::PN_Then, ClearSelectedNames, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(ClearSelectedNames, PalCatalogSelectedNamesVariableName, TEXT(""))
        || !Link(ClearSelectedNames, UEdGraphSchema_K2::PN_Then, ResetSelectedCount, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(ResetSelectedCount, PalCatalogSelectedCountVariableName, TEXT("0"))
        || !Link(ResetSelectedCount, UEdGraphSchema_K2::PN_Then, ForEachSelectedId, TEXT("Exec"))
        || !Link(BridgeForSelectedSummary, PanelBridgeVariableName, SelectedIdsForSummary, UEdGraphSchema_K2::PN_Self)
        || !Link(SelectedIdsForSummary, SpeciesFilterIdsVariableName, ForEachSelectedId, TEXT("Array"))
        || !Link(CharacterDatabase, UEdGraphSchema_K2::PN_ReturnValue, GetSelectedLocalizedName, UEdGraphSchema_K2::PN_Self)
        || !Link(ForEachSelectedId, TEXT("Array Element"), GetSelectedLocalizedName, TEXT("CharacterID"))
        || !Link(GetSelectedLocalizedName, TEXT("OutText"), SelectedNameToString, TEXT("InText"))
        || !Link(SelectedCountForSeparator, PalCatalogSelectedCountVariableName, SelectedCountIsZero, TEXT("A"))
        || !SetPinDefault(SelectedCountIsZero, TEXT("B"), TEXT("0"))
        || !SetPinDefault(SelectedPrefix, TEXT("A"), TEXT("Selected: "))
        || !SetPinDefault(SelectedPrefix, TEXT("B"), TEXT("已选中："))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, SelectedPrefix, TEXT("bPickA"))
        || !SetPinDefault(SelectedDelimiter, TEXT("A"), TEXT(", "))
        || !SetPinDefault(SelectedDelimiter, TEXT("B"), TEXT("、"))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, SelectedDelimiter, TEXT("bPickA"))
        || !Link(SelectedPrefix, UEdGraphSchema_K2::PN_ReturnValue, SelectedSeparator, TEXT("A"))
        || !Link(SelectedDelimiter, UEdGraphSchema_K2::PN_ReturnValue, SelectedSeparator, TEXT("B"))
        || !Link(SelectedCountIsZero, UEdGraphSchema_K2::PN_ReturnValue, SelectedSeparator, TEXT("bPickA"))
        || !Link(SelectedSeparator, UEdGraphSchema_K2::PN_ReturnValue, SelectedSeparatorAndName, TEXT("A"))
        || !Link(SelectedNameToString, UEdGraphSchema_K2::PN_ReturnValue, SelectedSeparatorAndName, TEXT("B"))
        || !Link(CurrentSelectedNames, PalCatalogSelectedNamesVariableName, AppendSelectedName, TEXT("A"))
        || !Link(SelectedSeparatorAndName, UEdGraphSchema_K2::PN_ReturnValue, AppendSelectedName, TEXT("B"))
        || !Link(ForEachSelectedId, TEXT("LoopBody"), StoreSelectedNames, UEdGraphSchema_K2::PN_Execute)
        || !Link(AppendSelectedName, UEdGraphSchema_K2::PN_ReturnValue, StoreSelectedNames, PalCatalogSelectedNamesVariableName)
        || !Link(StoreSelectedNames, UEdGraphSchema_K2::PN_Then, StoreSelectedCount, UEdGraphSchema_K2::PN_Execute)
        || !Link(SelectedCountForIncrement, PalCatalogSelectedCountVariableName, IncrementSelectedCount, TEXT("A"))
        || !SetPinDefault(IncrementSelectedCount, TEXT("B"), TEXT("1"))
        || !Link(IncrementSelectedCount, UEdGraphSchema_K2::PN_ReturnValue, StoreSelectedCount, PalCatalogSelectedCountVariableName)
        || !Link(FinalSelectedCount, PalCatalogSelectedCountVariableName, NoSelectedIds, TEXT("A"))
        || !SetPinDefault(NoSelectedIds, TEXT("B"), TEXT("0"))
        || !SetPinDefault(NoSelectionSummary, TEXT("A"), TEXT("Selected: none"))
        || !SetPinDefault(NoSelectionSummary, TEXT("B"), TEXT("已选中：无"))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, NoSelectionSummary, TEXT("bPickA"))
        || !Link(NoSelectionSummary, UEdGraphSchema_K2::PN_ReturnValue, FinalSelectedSummary, TEXT("A"))
        || !Link(FinalSelectedNames, PalCatalogSelectedNamesVariableName, FinalSelectedSummary, TEXT("B"))
        || !Link(NoSelectedIds, UEdGraphSchema_K2::PN_ReturnValue, FinalSelectedSummary, TEXT("bPickA"))
        || !Link(FinalSelectedSummary, UEdGraphSchema_K2::PN_ReturnValue, SelectedSummaryToText, TEXT("InString"))
        || !Link(ForEachSelectedId, TEXT("Completed"), SetSelectedSummaryText, UEdGraphSchema_K2::PN_Execute)
        || !Link(SelectedSummaryTextGet, SelectedSummaryText->GetFName(), SetSelectedSummaryText, UEdGraphSchema_K2::PN_Self)
        || !Link(SelectedSummaryToText, UEdGraphSchema_K2::PN_ReturnValue, SetSelectedSummaryText, TEXT("InText"))
        || !Link(SetSelectedSummaryText, UEdGraphSchema_K2::PN_Then, ForEachCatalogId, TEXT("Exec"))
        || !Link(CatalogIdsForRender, PalCatalogIdsVariableName, ForEachCatalogId, TEXT("Array"))
        || !Link(ForEachCatalogId, TEXT("LoopBody"), GetRenderRow, UEdGraphSchema_K2::PN_Execute)
        || !Link(TableForRenderRow, PalCatalogTableVariableName, GetRenderRow, TEXT("DataTable"))
        || !Link(ForEachCatalogId, TEXT("Array Element"), GetRenderRow, TEXT("RowName"))
        || !Link(GetRenderRow, UEdGraphSchema_K2::PN_ReturnValue, BreakRenderRow, CharacterParameterDatabaseRowStruct->GetFName())
        || !Link(PanelBridgeForLanguage, PanelBridgeVariableName, LanguageIdForRender, UEdGraphSchema_K2::PN_Self)
        || !Link(LanguageIdForRender, LanguageIdVariableName, LanguageIsEnglish, TEXT("A"))
        || !SetPinDefault(LanguageIsEnglish, TEXT("B"), TEXT("1"))
        || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, CharacterDatabase, TEXT("WorldContextObject"))
        || !Link(CharacterDatabase, UEdGraphSchema_K2::PN_ReturnValue, GetLocalizedName, UEdGraphSchema_K2::PN_Self)
        || !Link(ForEachCatalogId, TEXT("Array Element"), GetLocalizedName, TEXT("CharacterID"))
        || !Link(GetLocalizedName, TEXT("OutText"), NameToString, TEXT("InText"))
        || !Link(ForEachCatalogId, TEXT("Array Element"), CharacterIdToString, TEXT("InName"))
        || !SetPinDefault(VariantPrefix, TEXT("A"), TEXT(" [ID: "))
        || !Link(CharacterIdToString, UEdGraphSchema_K2::PN_ReturnValue, VariantPrefix, TEXT("B"))
        || !Link(VariantPrefix, UEdGraphSchema_K2::PN_ReturnValue, VariantName, TEXT("A"))
        || !SetPinDefault(VariantName, TEXT("B"), TEXT(""))
        || !Link(VariantName, UEdGraphSchema_K2::PN_ReturnValue, VariantSuffix, TEXT("A"))
        || !SetPinDefault(VariantSuffix, TEXT("B"), TEXT("]"))
        || !Link(NameToString, UEdGraphSchema_K2::PN_ReturnValue, NameWithVariant, TEXT("A"))
        || !Link(VariantSuffix, UEdGraphSchema_K2::PN_ReturnValue, NameWithVariant, TEXT("B"))
        || !Link(SearchBoxGet, SearchBox->GetFName(), GetSearchText, UEdGraphSchema_K2::PN_Self)
        || !Link(GetSearchText, UEdGraphSchema_K2::PN_ReturnValue, SearchToString, TEXT("InText"))
        || !Link(SearchToString, UEdGraphSchema_K2::PN_ReturnValue, SearchEmpty, TEXT("InString"))
        || !Link(NameToString, UEdGraphSchema_K2::PN_ReturnValue, NameContainsSearch, TEXT("SearchIn"))
        || !Link(SearchToString, UEdGraphSchema_K2::PN_ReturnValue, NameContainsSearch, TEXT("Substring"))
        || !SetPinDefault(NameContainsSearch, TEXT("bUseCase"), TEXT("false"))
        || !SetPinDefault(NameContainsSearch, TEXT("bSearchFromEnd"), TEXT("false"))
        || !SetPinDefault(PaldexNumber, TEXT("AppendTo"), TEXT(""))
        || !SetPinDefault(PaldexNumber, TEXT("Prefix"), TEXT(""))
        || !Link(BreakRenderRow, TEXT("ZukanIndex"), PaldexNumber, TEXT("InInt"))
        || !Link(BreakRenderRow, TEXT("ZukanIndexSuffix"), PaldexNumber, TEXT("Suffix"))
        || !Link(PaldexNumber, UEdGraphSchema_K2::PN_ReturnValue, NumberContainsSearch, TEXT("SearchIn"))
        || !Link(SearchToString, UEdGraphSchema_K2::PN_ReturnValue, NumberContainsSearch, TEXT("Substring"))
        || !SetPinDefault(NumberContainsSearch, TEXT("bUseCase"), TEXT("false"))
        || !SetPinDefault(NumberContainsSearch, TEXT("bSearchFromEnd"), TEXT("false"))
        || !Link(NameContainsSearch, UEdGraphSchema_K2::PN_ReturnValue, NameOrNumberMatches, TEXT("A"))
        || !Link(NumberContainsSearch, UEdGraphSchema_K2::PN_ReturnValue, NameOrNumberMatches, TEXT("B"))
        || !Link(SearchEmpty, UEdGraphSchema_K2::PN_ReturnValue, SearchMatches, TEXT("A"))
        || !Link(NameOrNumberMatches, UEdGraphSchema_K2::PN_ReturnValue, SearchMatches, TEXT("B"))
        || !Link(BreakRenderRow, TEXT("ElementType1"), Element1ToInt, TEXT("InByte"))
        || !Link(BreakRenderRow, TEXT("ElementType2"), Element2ToInt, TEXT("InByte"))) {
        return false;
    }

    UEdGraphNode* ElementQueryAccepted = nullptr;
    int32 QueryX = 700;
    for (int32 Index = 0; Index < ElementDefinitions.Num(); ++Index) {
        UCheckBox* Toggle = ElementToggles[Index];
        UK2Node_VariableGet* ToggleGet = AddVariableGet(Graph, Toggle->GetFName(), QueryX, RenderY + 1040);
        UK2Node_CallFunction* IsChecked = AddStaticCall(
            Graph, UCheckBox::StaticClass(), TEXT("IsChecked"), QueryX + 240, RenderY + 1040);
        UK2Node_CallFunction* NotChecked = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), QueryX + 480, RenderY + 1040);
        UK2Node_CallFunction* Element1Matches = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), QueryX + 240, RenderY + 1160);
        UK2Node_CallFunction* Element2Matches = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), QueryX + 480, RenderY + 1160);
        UK2Node_CallFunction* EitherElementMatches = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), QueryX + 720, RenderY + 1160);
        UK2Node_CallFunction* ElementCondition = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), QueryX + 960, RenderY + 1100);
        if (!ToggleGet || !IsChecked || !NotChecked || !Element1Matches || !Element2Matches
            || !EitherElementMatches || !ElementCondition
            || !Link(ToggleGet, Toggle->GetFName(), IsChecked, UEdGraphSchema_K2::PN_Self)
            || !Link(IsChecked, UEdGraphSchema_K2::PN_ReturnValue, NotChecked, TEXT("A"))
            || !Link(Element1ToInt, UEdGraphSchema_K2::PN_ReturnValue, Element1Matches, TEXT("A"))
            || !SetPinDefault(Element1Matches, TEXT("B"), FString::FromInt(ElementDefinitions[Index].EnumValue))
            || !Link(Element2ToInt, UEdGraphSchema_K2::PN_ReturnValue, Element2Matches, TEXT("A"))
            || !SetPinDefault(Element2Matches, TEXT("B"), FString::FromInt(ElementDefinitions[Index].EnumValue))
            || !Link(Element1Matches, UEdGraphSchema_K2::PN_ReturnValue, EitherElementMatches, TEXT("A"))
            || !Link(Element2Matches, UEdGraphSchema_K2::PN_ReturnValue, EitherElementMatches, TEXT("B"))
            || !Link(NotChecked, UEdGraphSchema_K2::PN_ReturnValue, ElementCondition, TEXT("A"))
            || !Link(EitherElementMatches, UEdGraphSchema_K2::PN_ReturnValue, ElementCondition, TEXT("B"))) {
            return false;
        }
        if (ElementQueryAccepted) {
            UK2Node_CallFunction* Combine = AddStaticCall(
                Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), QueryX + 1200, RenderY + 1100);
            if (!Combine
                || !Link(ElementQueryAccepted, UEdGraphSchema_K2::PN_ReturnValue, Combine, TEXT("A"))
                || !Link(ElementCondition, UEdGraphSchema_K2::PN_ReturnValue, Combine, TEXT("B"))) {
                return false;
            }
            ElementQueryAccepted = Combine;
        } else {
            ElementQueryAccepted = ElementCondition;
        }
        QueryX += 1440;
    }

    UEdGraphNode* WorkQueryAccepted = nullptr;
    UEdGraphNode* WorkTextSource = nullptr;
    QueryX = 700;
    for (int32 Index = 0; Index < WorkDefinitions.Num(); ++Index) {
        UCheckBox* Toggle = WorkToggles[Index];
        const FPalWorkDefinition& Definition = WorkDefinitions[Index];
        UK2Node_VariableGet* ToggleGet = AddVariableGet(Graph, Toggle->GetFName(), QueryX, RenderY + 1420);
        UK2Node_CallFunction* IsChecked = AddStaticCall(
            Graph, UCheckBox::StaticClass(), TEXT("IsChecked"), QueryX + 240, RenderY + 1420);
        UK2Node_CallFunction* NotChecked = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), QueryX + 480, RenderY + 1420);
        UK2Node_CallFunction* HasWork = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("Greater_IntInt"), QueryX + 240, RenderY + 1540);
        UK2Node_CallFunction* WorkCondition = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), QueryX + 720, RenderY + 1480);
        UK2Node_CallFunction* GetWorkName = AddStaticCall(
            Graph, PalUIUtilityClass, TEXT("GetWorkSuitabilityName"), QueryX, RenderY + 1660);
        UK2Node_CallFunction* WorkNameToString = AddStaticCall(
            Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), QueryX + 240, RenderY + 1660);
        UK2Node_CallFunction* WorkRankPrefix = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 480, RenderY + 1780);
        UK2Node_CallFunction* WorkWithRank = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), QueryX + 480, RenderY + 1660);
        UK2Node_CallFunction* SelectWorkText = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 720, RenderY + 1660);
        UK2Node_CallFunction* AppendWorkText = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), QueryX + 960, RenderY + 1660);
        if (!ToggleGet || !IsChecked || !NotChecked || !HasWork || !WorkCondition
            || !GetWorkName || !WorkNameToString || !WorkRankPrefix || !WorkWithRank || !SelectWorkText || !AppendWorkText
            || !Link(ToggleGet, Toggle->GetFName(), IsChecked, UEdGraphSchema_K2::PN_Self)
            || !Link(IsChecked, UEdGraphSchema_K2::PN_ReturnValue, NotChecked, TEXT("A"))
            || !Link(BreakRenderRow, Definition.FieldName, HasWork, TEXT("A"))
            || !SetPinDefault(HasWork, TEXT("B"), TEXT("0"))
            || !Link(NotChecked, UEdGraphSchema_K2::PN_ReturnValue, WorkCondition, TEXT("A"))
            || !Link(HasWork, UEdGraphSchema_K2::PN_ReturnValue, WorkCondition, TEXT("B"))
            || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetWorkName, TEXT("WorldContextObject"))
            || !SetPinDefault(GetWorkName, TEXT("WorkSuitability"), Definition.EnumName.ToString())
            || !Link(GetWorkName, TEXT("outName"), WorkNameToString, TEXT("InText"))
            || !Link(WorkNameToString, UEdGraphSchema_K2::PN_ReturnValue, WorkWithRank, TEXT("AppendTo"))
            || !SetPinDefault(WorkRankPrefix, TEXT("A"), TEXT(" Lv."))
            || !SetPinDefault(WorkRankPrefix, TEXT("B"), TEXT(" "))
            || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, WorkRankPrefix, TEXT("bPickA"))
            || !Link(WorkRankPrefix, UEdGraphSchema_K2::PN_ReturnValue, WorkWithRank, TEXT("Prefix"))
            || !Link(BreakRenderRow, Definition.FieldName, WorkWithRank, TEXT("InInt"))
            || !SetPinDefault(WorkWithRank, TEXT("Suffix"), Index + 1 < WorkDefinitions.Num() ? TEXT(" | ") : TEXT(""))
            || !Link(WorkWithRank, UEdGraphSchema_K2::PN_ReturnValue, SelectWorkText, TEXT("A"))
            || !SetPinDefault(SelectWorkText, TEXT("B"), TEXT(""))
            || !Link(HasWork, UEdGraphSchema_K2::PN_ReturnValue, SelectWorkText, TEXT("bPickA"))
            || !Link(SelectWorkText, UEdGraphSchema_K2::PN_ReturnValue, AppendWorkText, TEXT("B"))) {
            return false;
        }
        if (WorkTextSource) {
            if (!Link(WorkTextSource, UEdGraphSchema_K2::PN_ReturnValue, AppendWorkText, TEXT("A"))) {
                return false;
            }
        } else if (!SetPinDefault(AppendWorkText, TEXT("A"), TEXT(""))) {
            return false;
        }
        WorkTextSource = AppendWorkText;
        if (WorkQueryAccepted) {
            UK2Node_CallFunction* Combine = AddStaticCall(
                Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), QueryX + 960, RenderY + 1480);
            if (!Combine
                || !Link(WorkQueryAccepted, UEdGraphSchema_K2::PN_ReturnValue, Combine, TEXT("A"))
                || !Link(WorkCondition, UEdGraphSchema_K2::PN_ReturnValue, Combine, TEXT("B"))) {
                return false;
            }
            WorkQueryAccepted = Combine;
        } else {
            WorkQueryAccepted = WorkCondition;
        }
        QueryX += 1200;
    }
    if (!ElementQueryAccepted || !WorkQueryAccepted || !WorkTextSource) {
        return false;
    }

    UK2Node_CallFunction* SearchAndElement = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), QueryX, RenderY + 680);
    UK2Node_CallFunction* AllQueriesMatch = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), QueryX + 260, RenderY + 680);
    UK2Node_IfThenElse* QueryBranch = AddBranch(Graph, QueryX + 520, RenderY);
    UK2Node_CallFunction* GetElement1Name = AddStaticCall(
        Graph, PalUIUtilityClass, TEXT("GetPalElementTypeName"), QueryX, RenderY + 1900);
    UK2Node_CallFunction* Element1NameToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), QueryX + 260, RenderY + 1900);
    UK2Node_CallFunction* Element1IsNormal = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), QueryX + 520, RenderY + 1900);
    UK2Node_CallFunction* Element1NormalName = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 780, RenderY + 1900);
    UK2Node_CallFunction* Element1DisplayName = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 1040, RenderY + 1900);
    UK2Node_CallFunction* GetElement2Name = AddStaticCall(
        Graph, PalUIUtilityClass, TEXT("GetPalElementTypeName"), QueryX, RenderY + 2020);
    UK2Node_CallFunction* Element2NameToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), QueryX + 260, RenderY + 2020);
    UK2Node_CallFunction* Element2IsNormal = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), QueryX + 520, RenderY + 2020);
    UK2Node_CallFunction* Element2NormalName = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 780, RenderY + 2020);
    UK2Node_CallFunction* Element2DisplayName = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 1040, RenderY + 2020);
    UK2Node_CallFunction* Element2Valid = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Greater_IntInt"), QueryX + 520, RenderY + 2020);
    UK2Node_CallFunction* Element2Prefix = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), QueryX + 520, RenderY + 1900);
    UK2Node_CallFunction* SelectElement2 = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 780, RenderY + 1960);
    UK2Node_CallFunction* ElementDisplayText = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), QueryX + 1040, RenderY + 1900);
    const int32 NormalElementValue = ElementEnum->GetValueByNameString(TEXT("Normal"));
    if (!SearchAndElement || !AllQueriesMatch || !QueryBranch || !GetElement1Name || !Element1NameToString
        || !Element1IsNormal || !Element1NormalName || !Element1DisplayName
        || !GetElement2Name || !Element2NameToString || !Element2IsNormal || !Element2NormalName || !Element2DisplayName
        || !Element2Valid || !Element2Prefix
        || !SelectElement2 || !ElementDisplayText || !PaldexNumber
        || !Link(SearchMatches, UEdGraphSchema_K2::PN_ReturnValue, SearchAndElement, TEXT("A"))
        || !Link(ElementQueryAccepted, UEdGraphSchema_K2::PN_ReturnValue, SearchAndElement, TEXT("B"))
        || !Link(SearchAndElement, UEdGraphSchema_K2::PN_ReturnValue, AllQueriesMatch, TEXT("A"))
        || !Link(WorkQueryAccepted, UEdGraphSchema_K2::PN_ReturnValue, AllQueriesMatch, TEXT("B"))
        || !Link(GetRenderRow, UEdGraphSchema_K2::PN_Then, QueryBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(AllQueriesMatch, UEdGraphSchema_K2::PN_ReturnValue, QueryBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetElement1Name, TEXT("WorldContextObject"))
        || !Link(BreakRenderRow, TEXT("ElementType1"), GetElement1Name, TEXT("ElementType"))
        || NormalElementValue == INDEX_NONE
        || !Link(GetElement1Name, TEXT("outName"), Element1NameToString, TEXT("InText"))
        || !Link(Element1ToInt, UEdGraphSchema_K2::PN_ReturnValue, Element1IsNormal, TEXT("A"))
        || !SetPinDefault(Element1IsNormal, TEXT("B"), FString::FromInt(NormalElementValue))
        || !SetPinDefault(Element1NormalName, TEXT("A"), TEXT("Normal"))
        || !SetPinDefault(Element1NormalName, TEXT("B"), TEXT("普通"))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, Element1NormalName, TEXT("bPickA"))
        || !Link(Element1NormalName, UEdGraphSchema_K2::PN_ReturnValue, Element1DisplayName, TEXT("A"))
        || !Link(Element1NameToString, UEdGraphSchema_K2::PN_ReturnValue, Element1DisplayName, TEXT("B"))
        || !Link(Element1IsNormal, UEdGraphSchema_K2::PN_ReturnValue, Element1DisplayName, TEXT("bPickA"))
        || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetElement2Name, TEXT("WorldContextObject"))
        || !Link(BreakRenderRow, TEXT("ElementType2"), GetElement2Name, TEXT("ElementType"))
        || !Link(GetElement2Name, TEXT("outName"), Element2NameToString, TEXT("InText"))
        || !Link(Element2ToInt, UEdGraphSchema_K2::PN_ReturnValue, Element2IsNormal, TEXT("A"))
        || !SetPinDefault(Element2IsNormal, TEXT("B"), FString::FromInt(NormalElementValue))
        || !SetPinDefault(Element2NormalName, TEXT("A"), TEXT("Normal"))
        || !SetPinDefault(Element2NormalName, TEXT("B"), TEXT("普通"))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, Element2NormalName, TEXT("bPickA"))
        || !Link(Element2NormalName, UEdGraphSchema_K2::PN_ReturnValue, Element2DisplayName, TEXT("A"))
        || !Link(Element2NameToString, UEdGraphSchema_K2::PN_ReturnValue, Element2DisplayName, TEXT("B"))
        || !Link(Element2IsNormal, UEdGraphSchema_K2::PN_ReturnValue, Element2DisplayName, TEXT("bPickA"))
        || !Link(Element2ToInt, UEdGraphSchema_K2::PN_ReturnValue, Element2Valid, TEXT("A"))
        || !SetPinDefault(Element2Valid, TEXT("B"), TEXT("0"))
        || !SetPinDefault(Element2Prefix, TEXT("A"), TEXT(" / "))
        || !Link(Element2DisplayName, UEdGraphSchema_K2::PN_ReturnValue, Element2Prefix, TEXT("B"))
        || !Link(Element2Prefix, UEdGraphSchema_K2::PN_ReturnValue, SelectElement2, TEXT("A"))
        || !SetPinDefault(SelectElement2, TEXT("B"), TEXT(""))
        || !Link(Element2Valid, UEdGraphSchema_K2::PN_ReturnValue, SelectElement2, TEXT("bPickA"))
        || !Link(Element1DisplayName, UEdGraphSchema_K2::PN_ReturnValue, ElementDisplayText, TEXT("A"))
        || !Link(SelectElement2, UEdGraphSchema_K2::PN_ReturnValue, ElementDisplayText, TEXT("B"))
        ) {
        return false;
    }

    UEdGraphNode* TooltipSource = nullptr;
    int32 TooltipX = QueryX;
    struct FTooltipFieldDefinition {
        FName FieldName;
        FString Chinese;
        FString English;
    };
    const TArray<FTooltipFieldDefinition> TooltipFields = {
        {TEXT("Hp"), TEXT("基础：生命 "), TEXT("Base: HP ")},
        {TEXT("ShotAttack"), TEXT("攻击 "), TEXT("ATK ")},
        {TEXT("Defense"), TEXT("防御 "), TEXT("DEF ")},
        {TEXT("CraftSpeed"), TEXT("工作速度 "), TEXT("Work speed ")},
        {TEXT("WalkSpeed"), TEXT("移动：步行 "), TEXT("Movement: Walk ")},
        {TEXT("RunSpeed"), TEXT("奔跑 "), TEXT("Run ")},
        {TEXT("SwimSpeed"), TEXT("游泳 "), TEXT("Swim ")},
        {TEXT("Stamina"), TEXT("耐力 "), TEXT("Stamina ")},
        {TEXT("FoodAmount"), TEXT("进食量 "), TEXT("Food ")},
    };
    const TArray<FString> TooltipSuffixes = {
        TEXT(" | "), TEXT(" | "), TEXT(" | "), TEXT("\n"),
        TEXT(" | "), TEXT(" | "), TEXT(" | "), TEXT("\n"), TEXT("\n"),
    };
    for (int32 Index = 0; Index < TooltipFields.Num(); ++Index) {
        UK2Node_CallFunction* SelectFieldPrefix = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), TooltipX, RenderY + 2180);
        UK2Node_CallFunction* AppendField = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), TooltipX, RenderY + 2300);
        if (!SelectFieldPrefix || !AppendField
            || !SetPinDefault(SelectFieldPrefix, TEXT("A"), TooltipFields[Index].English)
            || !SetPinDefault(SelectFieldPrefix, TEXT("B"), TooltipFields[Index].Chinese)
            || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, SelectFieldPrefix, TEXT("bPickA"))
            || !Link(SelectFieldPrefix, UEdGraphSchema_K2::PN_ReturnValue, AppendField, TEXT("Prefix"))
            || !Link(BreakRenderRow, TooltipFields[Index].FieldName, AppendField, TEXT("InInt"))
            || !SetPinDefault(AppendField, TEXT("Suffix"), TooltipSuffixes[Index])) {
            return false;
        }
        if (TooltipSource) {
            if (!Link(TooltipSource, UEdGraphSchema_K2::PN_ReturnValue, AppendField, TEXT("AppendTo"))) {
                return false;
            }
        } else if (!SetPinDefault(AppendField, TEXT("AppendTo"), TEXT(""))) {
            return false;
        }
        TooltipSource = AppendField;
        TooltipX += 260;
    }
    UK2Node_CallFunction* GetPartnerSkillName = AddStaticCall(
        Graph, PalUIUtilityClass, TEXT("GetPartnerSkillName"), TooltipX, RenderY + 2300);
    UK2Node_CallFunction* PartnerSkillNameToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), TooltipX + 260, RenderY + 2300);
    UK2Node_CallFunction* PartnerLabelPrefix = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), TooltipX + 520, RenderY + 2180);
    UK2Node_CallFunction* PartnerLabel = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), TooltipX + 520, RenderY + 2300);
    UK2Node_CallFunction* AppendPartner = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), TooltipX + 780, RenderY + 2300);
    UK2Node_CallFunction* GetPartnerDescription = AddStaticCall(
        Graph, PalUIUtilityClass, TEXT("GetFormatedFirstActivatedInfoTextFixedRank"), TooltipX, RenderY + 2440);
    UK2Node_CallFunction* PartnerDescriptionToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), TooltipX + 260, RenderY + 2440);
    UK2Node_CallFunction* PartnerDescriptionNotEmpty = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), TooltipX + 520, RenderY + 2560);
    UK2Node_CallFunction* MissingPartnerDescription = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), TooltipX + 780, RenderY + 2680);
    UK2Node_CallFunction* SelectPartnerDescription = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), TooltipX + 1040, RenderY + 2440);
    UK2Node_CallFunction* EffectLabelPrefix = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), TooltipX + 2080, RenderY + 2560);
    UK2Node_CallFunction* EffectLabel = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), TooltipX + 2080, RenderY + 2440);
    UK2Node_CallFunction* AppendEffect = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), TooltipX + 2340, RenderY + 2360);
    UK2Node_CallFunction* GetDropData = AddStaticCall(
        Graph, DatabaseCharacterParameterClass, TEXT("GetDropItemData"), TooltipX + 2600, RenderY + 2440);
    UK2Node_BreakStruct* BreakDropData = AddBreakStruct(
        Graph, DropItemDatabaseRowStruct, TooltipX + 2860, RenderY + 2500);
    if (!TooltipSource || !GetPartnerSkillName || !PartnerSkillNameToString || !PartnerLabelPrefix || !PartnerLabel || !AppendPartner
        || !GetPartnerDescription || !PartnerDescriptionToString || !PartnerDescriptionNotEmpty
        || !MissingPartnerDescription || !SelectPartnerDescription
        || !EffectLabelPrefix || !EffectLabel || !AppendEffect
        || !GetDropData || !BreakDropData
        || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetPartnerSkillName, TEXT("WorldContextObject"))
        || !Link(ForEachCatalogId, TEXT("Array Element"), GetPartnerSkillName, TEXT("CharacterID"))
        || !Link(GetPartnerSkillName, TEXT("OutText"), PartnerSkillNameToString, TEXT("InText"))
        || !SetPinDefault(PartnerLabelPrefix, TEXT("A"), TEXT("Partner skill: "))
        || !SetPinDefault(PartnerLabelPrefix, TEXT("B"), TEXT("伙伴技能："))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, PartnerLabelPrefix, TEXT("bPickA"))
        || !Link(PartnerLabelPrefix, UEdGraphSchema_K2::PN_ReturnValue, PartnerLabel, TEXT("A"))
        || !Link(PartnerSkillNameToString, UEdGraphSchema_K2::PN_ReturnValue, PartnerLabel, TEXT("B"))
        || !Link(TooltipSource, UEdGraphSchema_K2::PN_ReturnValue, AppendPartner, TEXT("A"))
        || !Link(PartnerLabel, UEdGraphSchema_K2::PN_ReturnValue, AppendPartner, TEXT("B"))
        || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetPartnerDescription, TEXT("WorldContextObject"))
        || !Link(ForEachCatalogId, TEXT("Array Element"), GetPartnerDescription, TEXT("CharacterID"))
        || !SetPinDefault(GetPartnerDescription, TEXT("Rank"), TEXT("1"))
        || !Link(GetPartnerDescription, TEXT("outFormatedText"), PartnerDescriptionToString, TEXT("InText"))
        || !Link(PartnerDescriptionToString, UEdGraphSchema_K2::PN_ReturnValue, PartnerDescriptionNotEmpty, TEXT("A"))
        || !SetPinDefault(PartnerDescriptionNotEmpty, TEXT("B"), TEXT(""))
        || !SetPinDefault(MissingPartnerDescription, TEXT("A"), TEXT("Partner skill effect unavailable"))
        || !SetPinDefault(MissingPartnerDescription, TEXT("B"), TEXT("当前游戏未提供伙伴技能说明"))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, MissingPartnerDescription, TEXT("bPickA"))
        || !Link(PartnerDescriptionToString, UEdGraphSchema_K2::PN_ReturnValue, SelectPartnerDescription, TEXT("A"))
        || !Link(MissingPartnerDescription, UEdGraphSchema_K2::PN_ReturnValue, SelectPartnerDescription, TEXT("B"))
        || !Link(PartnerDescriptionNotEmpty, UEdGraphSchema_K2::PN_ReturnValue, SelectPartnerDescription, TEXT("bPickA"))
        || !SetPinDefault(EffectLabelPrefix, TEXT("A"), TEXT("\nEffect: "))
        || !SetPinDefault(EffectLabelPrefix, TEXT("B"), TEXT("\n效果："))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, EffectLabelPrefix, TEXT("bPickA"))
        || !Link(EffectLabelPrefix, UEdGraphSchema_K2::PN_ReturnValue, EffectLabel, TEXT("A"))
        || !Link(SelectPartnerDescription, UEdGraphSchema_K2::PN_ReturnValue, EffectLabel, TEXT("B"))
        || !Link(AppendPartner, UEdGraphSchema_K2::PN_ReturnValue, AppendEffect, TEXT("A"))
        || !Link(EffectLabel, UEdGraphSchema_K2::PN_ReturnValue, AppendEffect, TEXT("B"))
        || !Link(CharacterDatabase, UEdGraphSchema_K2::PN_ReturnValue, GetDropData, UEdGraphSchema_K2::PN_Self)
        || !Link(ForEachCatalogId, TEXT("Array Element"), GetDropData, TEXT("CharacterID"))
        || !SetPinDefault(GetDropData, TEXT("Level"), TEXT("1"))
        || !Link(GetDropData, TEXT("OutData"), BreakDropData, DropItemDatabaseRowStruct->GetFName())) {
        return false;
    }

    UEdGraphNode* DropTextSource = AppendEffect;
    UK2Node_CallFunction* DropLabelPrefix = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), TooltipX + 3380, RenderY + 2360);
    if (!DropLabelPrefix
        || !SetPinDefault(DropLabelPrefix, TEXT("A"), TEXT("\nDrops: "))
        || !SetPinDefault(DropLabelPrefix, TEXT("B"), TEXT("\n掉落："))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, DropLabelPrefix, TEXT("bPickA"))) {
        return false;
    }
    int32 DropX = TooltipX + 3640;
    for (int32 Index = 1; Index <= 10; ++Index) {
        const FName ItemPin(*FString::Printf(TEXT("ItemId%d"), Index));
        UK2Node_CallFunction* ItemIdToString = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), DropX, RenderY + 2500);
        UK2Node_CallFunction* ItemAvailable = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), DropX + 240, RenderY + 2500);
        UK2Node_CallFunction* GetItemName = AddStaticCall(
            Graph, PalUIUtilityClass, TEXT("GetItemName"), DropX, RenderY + 2640);
        UK2Node_CallFunction* ItemNameToString = AddStaticCall(
            Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), DropX + 240, RenderY + 2640);
        UK2Node_CallFunction* PrefixItem = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), DropX + 480, RenderY + 2640);
        UK2Node_CallFunction* SelectItem = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), DropX + 720, RenderY + 2580);
        UK2Node_CallFunction* AppendItem = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Concat_StrStr"), DropX + 960, RenderY + 2580);
        if (!ItemIdToString || !ItemAvailable || !GetItemName || !ItemNameToString
            || !PrefixItem || !SelectItem || !AppendItem
            || !Link(BreakDropData, ItemPin, ItemIdToString, TEXT("InName"))
            || !Link(ItemIdToString, UEdGraphSchema_K2::PN_ReturnValue, ItemAvailable, TEXT("A"))
            || !SetPinDefault(ItemAvailable, TEXT("B"), TEXT("None"))
            || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetItemName, TEXT("WorldContextObject"))
            || !Link(BreakDropData, ItemPin, GetItemName, TEXT("StaticItemId"))
            || !Link(GetItemName, TEXT("outName"), ItemNameToString, TEXT("InText"))
            || !(Index == 1
                ? Link(DropLabelPrefix, UEdGraphSchema_K2::PN_ReturnValue, PrefixItem, TEXT("A"))
                : SetPinDefault(PrefixItem, TEXT("A"), TEXT(" / ")))
            || !Link(ItemNameToString, UEdGraphSchema_K2::PN_ReturnValue, PrefixItem, TEXT("B"))
            || !Link(PrefixItem, UEdGraphSchema_K2::PN_ReturnValue, SelectItem, TEXT("A"))
            || !SetPinDefault(SelectItem, TEXT("B"), TEXT(""))
            || !Link(ItemAvailable, UEdGraphSchema_K2::PN_ReturnValue, SelectItem, TEXT("bPickA"))
            || !Link(DropTextSource, UEdGraphSchema_K2::PN_ReturnValue, AppendItem, TEXT("A"))
            || !Link(SelectItem, UEdGraphSchema_K2::PN_ReturnValue, AppendItem, TEXT("B"))) {
            return false;
        }
        DropTextSource = AppendItem;
        DropX += 1200;
    }
    UK2Node_CallFunction* TooltipToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), DropX, RenderY + 2580);
    if (!TooltipToText
        || !Link(DropTextSource, UEdGraphSchema_K2::PN_ReturnValue, TooltipToText, TEXT("InString"))) {
        return false;
    }

    UK2Node_CallFunction* GetController = AddStaticCall(
        Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), QueryX + 780, RenderY + 160);
    UK2Node_CallFunction* CreateEntry = AddStaticCall(
        Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), QueryX + 1040, RenderY);
    UK2Node_DynamicCast* CastEntry = AddDynamicCast(Graph, PalEntryClass, QueryX + 1300, RenderY);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, QueryX + 1040, RenderY + 320);
    UK2Node_VariableGet* SelectedIdsGet = AddExternalVariableGet(
        Graph, SpeciesFilterIdsVariableName, ModActorClass, QueryX + 1300, RenderY + 320);
    UK2Node_CallArrayFunction* IsSelected = AddArrayCall(Graph, TEXT("Array_Contains"), QueryX + 1560, RenderY + 320);
    UK2Node_CallFunction* InitializeEntry = AddStaticCall(
        Graph, PalEntryClass, *PalEntryInitializeEventName.ToString(), QueryX + 1560, RenderY);
    UK2Node_VariableGet* ResultsWrapForAdd = AddVariableGet(Graph, ResultsWrap->GetFName(), QueryX + 1820, RenderY + 320);
    UK2Node_CallFunction* AddEntry = AddStaticCall(
        Graph, UPanelWidget::StaticClass(), TEXT("AddChild"), QueryX + 1820, RenderY);
    UK2Node_VariableGet* ResultCountGet = AddVariableGet(
        Graph, PalCatalogResultCountVariableName, QueryX + 2080, RenderY + 320);
    UK2Node_CallFunction* IncrementCount = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_IntInt"), QueryX + 2340, RenderY + 320);
    UK2Node_VariableSet* StoreResultCount = AddVariableSet(
        Graph, PalCatalogResultCountVariableName, QueryX + 2340, RenderY);
    UK2Node_VariableGet* FinalCountGet = AddVariableGet(
        Graph, PalCatalogResultCountVariableName, QueryX + 2600, RenderY + 520);
    UK2Node_CallFunction* ResultCountPrefix = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), QueryX + 2860, RenderY + 680);
    UK2Node_CallFunction* ResultCountString = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("BuildString_Int"), QueryX + 2860, RenderY + 520);
    UK2Node_CallFunction* ResultCountToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), QueryX + 3120, RenderY + 520);
    UK2Node_VariableGet* ResultCountTextGet = AddVariableGet(
        Graph, ResultCountText->GetFName(), QueryX + 3120, RenderY + 680);
    UK2Node_CallFunction* SetResultCountText = AddStaticCall(
        Graph, UTextBlock::StaticClass(), TEXT("SetText"), QueryX + 3380, RenderY + 520);
    if (!GetController || !CreateEntry || !CastEntry || !BridgeGet || !SelectedIdsGet || !IsSelected
        || !InitializeEntry || !ResultsWrapForAdd || !AddEntry || !ResultCountGet || !IncrementCount
        || !StoreResultCount || !FinalCountGet || !ResultCountPrefix || !ResultCountString || !ResultCountToText
        || !ResultCountTextGet || !SetResultCountText
        || !SetClassPin(CreateEntry, TEXT("WidgetType"), PalEntryClass)
        || !Link(QueryBranch, UEdGraphSchema_K2::PN_Then, CreateEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(RenderSelf, UEdGraphSchema_K2::PN_Self, GetController, TEXT("WorldContextObject"))
        || !SetPinDefault(GetController, TEXT("PlayerIndex"), TEXT("0"))
        || !Link(GetController, UEdGraphSchema_K2::PN_ReturnValue, CreateEntry, TEXT("OwningPlayer"))
        || !Link(CreateEntry, UEdGraphSchema_K2::PN_Then, CastEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateEntry, UEdGraphSchema_K2::PN_ReturnValue, CastEntry, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastEntry, CastEntry->GetValidCastPin()->PinName, InitializeEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastEntry, CastEntry->GetCastResultPin()->PinName, InitializeEntry, UEdGraphSchema_K2::PN_Self)
        || !Link(BridgeGet, PanelBridgeVariableName, InitializeEntry, TEXT("Bridge"))
        || !Link(ForEachCatalogId, TEXT("Array Element"), InitializeEntry, TEXT("CharacterId"))
        || !Link(PaldexNumber, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("PaldexNumber"))
        || !Link(NameWithVariant, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("DisplayName"))
        || !Link(SelectedSummaryTextGet, SelectedSummaryText->GetFName(), InitializeEntry, TEXT("SelectionSummary"))
        || !Link(ElementDisplayText, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("ElementText"))
        || !Link(WorkTextSource, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("WorkText"))
        || !Link(TooltipToText, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("Tooltip"))
        || !Link(BridgeGet, PanelBridgeVariableName, SelectedIdsGet, UEdGraphSchema_K2::PN_Self)
        || !Link(SelectedIdsGet, SpeciesFilterIdsVariableName, IsSelected, TEXT("TargetArray"))
        || !Link(ForEachCatalogId, TEXT("Array Element"), IsSelected, TEXT("ItemToFind"))
        || !Link(IsSelected, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("Selected"))
        || !Link(InitializeEntry, UEdGraphSchema_K2::PN_Then, AddEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(ResultsWrapForAdd, ResultsWrap->GetFName(), AddEntry, UEdGraphSchema_K2::PN_Self)
        || !Link(CastEntry, CastEntry->GetCastResultPin()->PinName, AddEntry, TEXT("Content"))
        || !Link(AddEntry, UEdGraphSchema_K2::PN_Then, StoreResultCount, UEdGraphSchema_K2::PN_Execute)
        || !Link(ResultCountGet, PalCatalogResultCountVariableName, IncrementCount, TEXT("A"))
        || !SetPinDefault(IncrementCount, TEXT("B"), TEXT("1"))
        || !Link(IncrementCount, UEdGraphSchema_K2::PN_ReturnValue, StoreResultCount, PalCatalogResultCountVariableName)
        || !Link(ForEachCatalogId, TEXT("Completed"), SetResultCountText, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(ResultCountString, TEXT("AppendTo"), TEXT(""))
        || !SetPinDefault(ResultCountPrefix, TEXT("A"), TEXT("Results: "))
        || !SetPinDefault(ResultCountPrefix, TEXT("B"), TEXT("结果："))
        || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, ResultCountPrefix, TEXT("bPickA"))
        || !Link(ResultCountPrefix, UEdGraphSchema_K2::PN_ReturnValue, ResultCountString, TEXT("Prefix"))
        || !Link(FinalCountGet, PalCatalogResultCountVariableName, ResultCountString, TEXT("InInt"))
        || !SetPinDefault(ResultCountString, TEXT("Suffix"), TEXT(""))
        || !Link(ResultCountString, UEdGraphSchema_K2::PN_ReturnValue, ResultCountToText, TEXT("InString"))
        || !Link(ResultCountTextGet, ResultCountText->GetFName(), SetResultCountText, UEdGraphSchema_K2::PN_Self)
        || !Link(ResultCountToText, UEdGraphSchema_K2::PN_ReturnValue, SetResultCountText, TEXT("InText"))) {
        return false;
    }

    return true;
}

bool BuildPanelPalCatalogEvents(
    UWidgetBlueprint* Blueprint,
    UClass* ModActorClass,
    UEditableTextBox* SearchBox,
    UButton* SearchButton,
    UButton* ClearQueryButton,
    UButton* ClearSelectionButton,
    const TArray<UButton*>& SortModeButtons,
    UButton* SortAscendingButton,
    UButton* SortDescendingButton,
    const TArray<UCheckBox*>& ElementToggles,
    const TArray<UCheckBox*>& WorkToggles,
    int32 Y) {
    if (!Blueprint || !ModActorClass || !SearchBox || !SearchButton || !ClearQueryButton
        || !ClearSelectionButton
        || SortModeButtons.Num() != GetPalWorkDefinitions().Num() + 2 || SortModeButtons.Contains(nullptr)
        || !SortAscendingButton || !SortDescendingButton
        || ElementToggles.Num() != 9 || ElementToggles.Contains(nullptr)
        || WorkToggles.Num() != GetPalWorkDefinitions().Num() || WorkToggles.Contains(nullptr)) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    auto AddRenderCall = [&](UEdGraphNode* ExecSource, const FName& ExecPin, int32 X, int32 EventY) -> bool {
        UK2Node_Self* Self = AddSelfNode(Graph, X, EventY + 160);
        UK2Node_CallFunction* Render = AddStaticCall(
            Graph, Blueprint->GeneratedClass, *PanelRenderPalCatalogEventName.ToString(), X + 260, EventY);
        return Self && Render
            && Link(ExecSource, ExecPin, Render, UEdGraphSchema_K2::PN_Execute)
            && Link(Self, UEdGraphSchema_K2::PN_Self, Render, UEdGraphSchema_K2::PN_Self);
    };

    UK2Node_ComponentBoundEvent* SearchCommitted = AddEditableTextBoxCommittedEvent(Blueprint, SearchBox, -1600, Y);
    UK2Node_ComponentBoundEvent* SearchClicked = AddButtonEvent(Blueprint, SearchButton, -1600, Y + 360);
    if (!SearchCommitted || !SearchClicked
        || !AddRenderCall(SearchCommitted, UEdGraphSchema_K2::PN_Then, -1320, Y)
        || !AddRenderCall(SearchClicked, UEdGraphSchema_K2::PN_Then, -1320, Y + 360)) {
        return false;
    }

    TArray<UCheckBox*> QueryToggles = ElementToggles;
    QueryToggles.Append(WorkToggles);
    int32 ToggleY = Y + 720;
    for (UCheckBox* Toggle : QueryToggles) {
        UK2Node_ComponentBoundEvent* Changed = AddCheckBoxStateChangedEvent(Blueprint, Toggle, -1600, ToggleY);
        UK2Node_VariableGet* SuppressGet = AddVariableGet(
            Graph, PalCatalogSuppressEventsVariableName, -1360, ToggleY + 160);
        UK2Node_CallFunction* NotSuppressed = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"), -1100, ToggleY + 160);
        UK2Node_IfThenElse* ShouldRender = AddBranch(Graph, -840, ToggleY);
        if (!Changed || !SuppressGet || !NotSuppressed || !ShouldRender
            || !Link(Changed, UEdGraphSchema_K2::PN_Then, ShouldRender, UEdGraphSchema_K2::PN_Execute)
            || !Link(SuppressGet, PalCatalogSuppressEventsVariableName, NotSuppressed, TEXT("A"))
            || !Link(NotSuppressed, UEdGraphSchema_K2::PN_ReturnValue, ShouldRender, UEdGraphSchema_K2::PN_Condition)
            || !AddRenderCall(ShouldRender, UEdGraphSchema_K2::PN_Then, -560, ToggleY)) {
            return false;
        }
        ToggleY += 360;
    }

    UK2Node_ComponentBoundEvent* ClearQuery = AddButtonEvent(Blueprint, ClearQueryButton, -1600, ToggleY);
    UK2Node_VariableSet* SuppressOn = AddVariableSet(Graph, PalCatalogSuppressEventsVariableName, -1320, ToggleY);
    UK2Node_VariableGet* SearchBoxGet = AddVariableGet(Graph, SearchBox->GetFName(), -1040, ToggleY + 160);
    UK2Node_CallFunction* ClearSearch = AddStaticCall(
        Graph, UEditableTextBox::StaticClass(), TEXT("SetText"), -780, ToggleY);
    if (!ClearQuery || !SuppressOn || !SearchBoxGet || !ClearSearch
        || !Link(ClearQuery, UEdGraphSchema_K2::PN_Then, SuppressOn, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(SuppressOn, PalCatalogSuppressEventsVariableName, TEXT("true"))
        || !Link(SuppressOn, UEdGraphSchema_K2::PN_Then, ClearSearch, UEdGraphSchema_K2::PN_Execute)
        || !Link(SearchBoxGet, SearchBox->GetFName(), ClearSearch, UEdGraphSchema_K2::PN_Self)
        || !SetPinDefaultText(ClearSearch, TEXT("InText"), FText::GetEmpty())) {
        return false;
    }
    UEdGraphNode* ClearTail = ClearSearch;
    int32 ClearX = -500;
    for (UCheckBox* Toggle : QueryToggles) {
        UK2Node_VariableGet* ToggleGet = AddVariableGet(Graph, Toggle->GetFName(), ClearX, ToggleY + 160);
        UK2Node_CallFunction* SetUnchecked = AddStaticCall(
            Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), ClearX + 260, ToggleY);
        if (!ToggleGet || !SetUnchecked
            || !Link(ClearTail, UEdGraphSchema_K2::PN_Then, SetUnchecked, UEdGraphSchema_K2::PN_Execute)
            || !Link(ToggleGet, Toggle->GetFName(), SetUnchecked, UEdGraphSchema_K2::PN_Self)
            || !SetPinDefault(SetUnchecked, TEXT("InIsChecked"), TEXT("false"))) {
            return false;
        }
        ClearTail = SetUnchecked;
        ClearX += 520;
    }
    UK2Node_VariableSet* SuppressOff = AddVariableSet(
        Graph, PalCatalogSuppressEventsVariableName, ClearX, ToggleY);
    if (!SuppressOff
        || !Link(ClearTail, UEdGraphSchema_K2::PN_Then, SuppressOff, UEdGraphSchema_K2::PN_Execute)
        || !SetPinDefault(SuppressOff, PalCatalogSuppressEventsVariableName, TEXT("false"))
        || !AddRenderCall(SuppressOff, UEdGraphSchema_K2::PN_Then, ClearX + 280, ToggleY)) {
        return false;
    }

    const int32 SelectionY = ToggleY + 480;
    UK2Node_ComponentBoundEvent* ClearSelection = AddButtonEvent(
        Blueprint, ClearSelectionButton, -1600, SelectionY);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1360, SelectionY + 160);
    UK2Node_VariableGet* SpeciesIdsGet = AddExternalVariableGet(
        Graph, SpeciesFilterIdsVariableName, ModActorClass, -1100, SelectionY + 160);
    UK2Node_CallArrayFunction* ClearSpeciesIds = AddArrayCall(Graph, TEXT("Array_Clear"), -840, SelectionY);
    UEdGraphNode* SelectionTail = ClearSpeciesIds;
    if (!ClearSelection || !BridgeGet || !SpeciesIdsGet || !ClearSpeciesIds
        || !Link(ClearSelection, UEdGraphSchema_K2::PN_Then, ClearSpeciesIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, SpeciesIdsGet, UEdGraphSchema_K2::PN_Self)
        || !Link(SpeciesIdsGet, SpeciesFilterIdsVariableName, ClearSpeciesIds, TEXT("TargetArray"))
        || !AppendExternalAssignment(
            Graph, ModActorClass, BridgeGet, SelectionTail,
            SpeciesFilterTextVariableName, TEXT(""), -560, SelectionY)
        || !AppendExternalIntegerIncrement(
            Graph, ModActorClass, BridgeGet, ControlRevisionVariableName, SelectionTail, 0, SelectionY)
        || !AddRenderCall(SelectionTail, UEdGraphSchema_K2::PN_Then, 820, SelectionY)) {
        return false;
    }

    const TArray<UButton*> SortDirectionButtons = {SortAscendingButton, SortDescendingButton};
    auto AddSortEvent = [&](UButton* Button, bool bSetMode, int32 SortMode, bool bDescending, int32 EventY) -> bool {
        UK2Node_ComponentBoundEvent* Clicked = AddButtonEvent(Blueprint, Button, -1600, EventY);
        UK2Node_VariableSet* SetMode = bSetMode
            ? AddVariableSet(Graph, PalCatalogSortModeVariableName, -1320, EventY)
            : nullptr;
        UK2Node_VariableSet* SetDirection = AddVariableSet(Graph, PalCatalogSortDescendingVariableName, -1060, EventY);
        UK2Node_Self* Self = AddSelfNode(Graph, -800, EventY + 160);
        UK2Node_CallFunction* Repopulate = AddStaticCall(
            Graph, Blueprint->GeneratedClass, *PanelPopulatePalCatalogEventName.ToString(), -540, EventY);
        if (!Clicked || !SetDirection || !Self || !Repopulate
            || (bSetMode && !SetMode)
            || !Link(Clicked, UEdGraphSchema_K2::PN_Then,
                bSetMode ? static_cast<UEdGraphNode*>(SetMode) : static_cast<UEdGraphNode*>(SetDirection), UEdGraphSchema_K2::PN_Execute)) {
            return false;
        }
        UEdGraphNode* Tail = bSetMode ? static_cast<UEdGraphNode*>(SetMode) : static_cast<UEdGraphNode*>(Clicked);
        if (bSetMode) {
            if (!SetPinDefault(SetMode, PalCatalogSortModeVariableName, FString::FromInt(SortMode))
                || !Link(SetMode, UEdGraphSchema_K2::PN_Then, SetDirection, UEdGraphSchema_K2::PN_Execute)) {
                return false;
            }
            Tail = SetMode;
        }
        if (!SetPinDefault(SetDirection, PalCatalogSortDescendingVariableName, bDescending ? TEXT("true") : TEXT("false"))
            || !Link(SetDirection, UEdGraphSchema_K2::PN_Then, Repopulate, UEdGraphSchema_K2::PN_Execute)
            || !Link(Self, UEdGraphSchema_K2::PN_Self, Repopulate, UEdGraphSchema_K2::PN_Self)) {
            return false;
        }
        UEdGraphNode* VisualTail = Repopulate;
        if (!AppendPanelSegmentVisualsConstant(
                Graph,
                bSetMode ? SortModeButtons : SortDirectionButtons,
                bSetMode ? SortMode : (bDescending ? 1 : 0),
                VisualTail,
                120,
                EventY)) {
            return false;
        }
        return true;
    };
    const int32 SortY = SelectionY + 520;
    for (int32 Index = 0; Index < SortModeButtons.Num(); ++Index) {
        if (!AddSortEvent(
                SortModeButtons[Index], true, Index, Index != 0, SortY + Index * 320)) {
            return false;
        }
    }
    const int32 DirectionY = SortY + SortModeButtons.Num() * 320;
    if (!AddSortEvent(SortAscendingButton, false, 0, false, DirectionY)
        || !AddSortEvent(SortDescendingButton, false, 0, true, DirectionY + 320)) {
        return false;
    }
    return true;
}

bool BuildPanelPassiveCatalog(
    UWidgetBlueprint* Blueprint,
    UClass* ModActorClass,
    UClass* PassiveEntryClass,
    UClass* PalUtilityClass,
    UClass* PalUIUtilityClass,
    UClass* PassiveSkillManagerClass,
    UScriptStruct* PassiveSkillDatabaseRowStruct,
    UClass* MasterDataTablesUtilityClass,
    const TArray<UWrapBox*>& Groups,
    UEditableTextBox* SearchBox,
    UK2Node_CustomEvent* ExistingEvent,
    int32 Y) {
    if (!Blueprint || !ModActorClass || !PassiveEntryClass || !PalUtilityClass || !PalUIUtilityClass
        || !PassiveSkillManagerClass || !PassiveSkillDatabaseRowStruct || !MasterDataTablesUtilityClass
        || Groups.Num() != 8 || Groups.Contains(nullptr) || !SearchBox) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_CustomEvent* Event = ExistingEvent ? ExistingEvent : AddCustomEvent(
        Blueprint, Graph, *PanelPopulatePassiveCatalogEventName.ToString(), -1800, Y, {});
    if (!Graph || !Event) {
        return false;
    }

    UEdGraphNode* ExecTail = Event;
    int32 X = -1520;
    for (UWrapBox* Group : Groups) {
        UK2Node_VariableGet* GroupGet = AddVariableGet(Graph, Group->GetFName(), X, Y + 180);
        UK2Node_CallFunction* ClearChildren = AddStaticCall(Graph, UPanelWidget::StaticClass(), TEXT("ClearChildren"), X + 260, Y);
        if (!GroupGet || !ClearChildren
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, ClearChildren, UEdGraphSchema_K2::PN_Execute)
            || !Link(GroupGet, Group->GetFName(), ClearChildren, UEdGraphSchema_K2::PN_Self)) {
            return false;
        }
        ExecTail = ClearChildren;
        X += 520;
    }

    UK2Node_Self* Self = AddSelfNode(Graph, -1520, Y + 500);
    UK2Node_CallFunction* GetSortedSkills = AddStaticCall(Graph, PalUIUtilityClass, TEXT("GetSortedPassiveSkillNameArray"), -1260, Y + 500);
    UK2Node_MacroInstance* ForEachSkillId = AddForEachLoop(Graph, X, Y);
    UK2Node_CallFunction* GetLocalizedSkillName = AddStaticCall(Graph, PalUIUtilityClass, TEXT("GetPassiveSkillName"), X, Y + 520);
    UK2Node_CallFunction* LocalizedSkillNameToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), X + 260, Y + 520);
    UK2Node_CallFunction* SkillIdToString = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), X + 520, Y + 400);
    UK2Node_CallFunction* SkillIdNotNone = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), X + 780, Y + 400);
    UK2Node_CallFunction* NameNotEmpty = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), X + 520, Y + 520);
    UK2Node_CallFunction* NameNotNone = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), X + 780, Y + 520);
    UK2Node_VariableGet* SearchBoxGet = AddVariableGet(Graph, SearchBox->GetFName(), X + 520, Y + 600);
    UK2Node_CallFunction* GetSearchText = AddStaticCall(
        Graph, UEditableTextBox::StaticClass(), TEXT("GetText"), X + 780, Y + 600);
    UK2Node_CallFunction* SearchTextToString = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), X + 1040, Y + 600);
    UK2Node_CallFunction* SearchTextIsEmpty = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("IsEmpty"), X + 1300, Y + 640);
    UK2Node_CallFunction* NameContainsSearch = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Contains"), X + 1300, Y + 520);
    UK2Node_CallFunction* SearchQueryMatches = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), X + 1560, Y + 560);
    UK2Node_CallFunction* GetPassiveManager = AddStaticCall(Graph, PalUtilityClass, TEXT("GetPassiveSkillManager"), X, Y + 680);
    UK2Node_CallFunction* GetSkillData = AddStaticCall(Graph, PassiveSkillManagerClass, TEXT("GetSkillData"), X + 260, Y + 680);
    UK2Node_BreakStruct* BreakSkillData = NewObject<UK2Node_BreakStruct>(Graph);
    if (BreakSkillData) {
        BreakSkillData->StructType = PassiveSkillDatabaseRowStruct;
        BreakSkillData->NodePosX = X + 520;
        BreakSkillData->NodePosY = Y + 680;
        Graph->AddNode(BreakSkillData, true, false);
        BreakSkillData->CreateNewGuid();
        BreakSkillData->AllocateDefaultPins();
    }
    UK2Node_CallFunction* NameValid = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), X + 1040, Y + 400);
    UK2Node_CallFunction* IdAndDataValid = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), X + 1300, Y + 400);
    UK2Node_CallFunction* NameAndSearchValid = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), X + 1560, Y + 400);
    UK2Node_CallFunction* CatalogEntryValid = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), X + 1820, Y + 400);
    UK2Node_IfThenElse* CatalogEntryBranch = AddBranch(Graph, X + 2080, Y);
    UK2Node_CallFunction* DescIdToString = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), X + 780, Y + 840);
    UK2Node_CallFunction* SummaryIdToString = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_NameToString"), X + 780, Y + 960);
    UK2Node_CallFunction* DescIdAvailable = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), X + 1040, Y + 840);
    UK2Node_CallFunction* SummaryIdAvailable = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), X + 1040, Y + 960);
    UK2Node_CallFunction* AnyDescriptionId = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanOR"), X + 1300, Y + 1020);
    UK2Node_CallFunction* SelectDescriptionId = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 1300, Y + 900);
    UK2Node_CallFunction* SelectAvailableDescriptionId = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 1560, Y + 900);
    UK2Node_CallFunction* DescriptionStringToName = AddStaticCall(Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_StringToName"), X + 1820, Y + 900);
    UK2Node_CallFunction* GetLocalizedDescription = AddStaticCall(Graph, MasterDataTablesUtilityClass, TEXT("GetLocalizedText"), X + 2080, Y + 900);
    UK2Node_CallFunction* DescriptionTextToString = AddStaticCall(Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_TextToString"), X + 2340, Y + 900);
    TArray<UK2Node_CallFunction*> EffectValueToStrings;
    TArray<UK2Node_CallFunction*> ReplaceEffectValues;
    for (int32 Index = 0; Index < 4; ++Index) {
        EffectValueToStrings.Add(AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_DoubleToString"), X + 2340 + Index * 260, Y + 1080));
        ReplaceEffectValues.Add(AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("Replace"), X + 2600 + Index * 260, Y + 900));
    }
    UK2Node_CallFunction* SelectFormattedDescription = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), X + 3640, Y + 900);
    UK2Node_CallFunction* FormattedDescriptionToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), X + 3900, Y + 900);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, X, Y + 1120);
    UK2Node_VariableGet* LanguageIdGet = AddExternalVariableGet(Graph, LanguageIdVariableName, ModActorClass, X + 260, Y + 1040);
    UK2Node_CallFunction* LanguageIsEnglish = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 520, Y + 1040);
    UK2Node_VariableGet* SelectedIdsGet = AddExternalVariableGet(Graph, PassiveFilterIdsVariableName, ModActorClass, X + 260, Y + 1120);
    UK2Node_CallArrayFunction* IsSelected = AddArrayCall(Graph, TEXT("Array_Contains"), X + 520, Y + 1120);
    UK2Node_VariableGet* ExcludedIdsGet = AddExternalVariableGet(Graph, PassiveExcludeIdsVariableName, ModActorClass, X + 260, Y + 1200);
    UK2Node_CallArrayFunction* IsExcluded = AddArrayCall(Graph, TEXT("Array_Contains"), X + 520, Y + 1200);
    UK2Node_CallFunction* GetController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), X, Y + 1280);
    UK2Node_CallFunction* CreateEntry = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), X + 260, Y);
    UK2Node_DynamicCast* CastEntry = AddDynamicCast(Graph, PassiveEntryClass, X + 520, Y);
    UK2Node_CallFunction* InitializeEntry = AddStaticCall(Graph, PassiveEntryClass, *PassiveEntryInitializeEventName.ToString(), X + 780, Y);

    UK2Node_CallFunction* RankRainbow = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("GreaterEqual_IntInt"), X + 1040, Y + 1120);
    // __DEPRECATED_20260718__ [reason: LotteryWeight does not define the visible rarity tier]
    // UK2Node_CallFunction* RankPositive = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("Greater_IntInt"), X + 1040, Y + 1240);
    // UK2Node_CallFunction* WeightSpecial = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("LessEqual_IntInt"), X + 1040, Y + 1360);
    // UK2Node_CallFunction* RankSpecial = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("BooleanAND"), X + 1300, Y + 1300);
    UK2Node_CallFunction* RankLegend = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1240);
    UK2Node_CallFunction* RankGold3 = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1360);
    UK2Node_CallFunction* RankGold2 = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1480);
    UK2Node_CallFunction* RankNormal = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1600);
    UK2Node_CallFunction* RankNegative1 = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1720);
    UK2Node_CallFunction* RankNegative2 = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1840);
    UK2Node_CallFunction* RankNegative3 = AddStaticCall(Graph, UKismetMathLibrary::StaticClass(), TEXT("EqualEqual_IntInt"), X + 1040, Y + 1960);
    UK2Node_IfThenElse* RainbowBranch = AddBranch(Graph, X + 1040, Y);
    // __DEPRECATED_20260718__ [reason: the former special branch mixed ranks by lottery weight]
    // UK2Node_IfThenElse* SpecialBranch = AddBranch(Graph, X + 1300, Y + 120);
    UK2Node_IfThenElse* LegendBranch = AddBranch(Graph, X + 1300, Y + 120);
    UK2Node_IfThenElse* Gold3Branch = AddBranch(Graph, X + 1560, Y + 240);
    UK2Node_IfThenElse* Gold2Branch = AddBranch(Graph, X + 1820, Y + 360);
    UK2Node_IfThenElse* NormalBranch = AddBranch(Graph, X + 2080, Y + 480);
    UK2Node_IfThenElse* Negative1Branch = AddBranch(Graph, X + 2340, Y + 600);
    UK2Node_IfThenElse* Negative2Branch = AddBranch(Graph, X + 2600, Y + 720);
    UK2Node_IfThenElse* Negative3Branch = AddBranch(Graph, X + 2860, Y + 840);

    TArray<UK2Node_CallFunction*> AddToGroup;
    for (int32 Index = 0; Index < Groups.Num(); ++Index) {
        AddToGroup.Add(AddStaticCall(Graph, UPanelWidget::StaticClass(), TEXT("AddChild"), X + 3380, Y + Index * 180));
    }

    UEdGraphNode* FallbackDescriptionSource = nullptr;
    int32 FallbackX = X + 4160;
    for (int32 Index = 0; Index < PassiveDescriptionFallbacks.Num(); ++Index) {
        const FPassiveDescriptionFallback& Fallback = PassiveDescriptionFallbacks[Index];
        UK2Node_CallFunction* IdMatches = AddStaticCall(
            Graph, UKismetStringLibrary::StaticClass(), TEXT("EqualEqual_StrStr"), FallbackX, Y + 1040);
        UK2Node_CallFunction* LocalizedFallback = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), FallbackX + 260, Y + 1120);
        UK2Node_CallFunction* SelectFallback = AddStaticCall(
            Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), FallbackX + 520, Y + 1040);
        if (!IdMatches || !LocalizedFallback || !SelectFallback
            || !Link(SkillIdToString, UEdGraphSchema_K2::PN_ReturnValue, IdMatches, TEXT("A"))
            || !SetPinDefault(IdMatches, TEXT("B"), Fallback.SkillId)
            || !SetPinDefault(LocalizedFallback, TEXT("A"), Fallback.English)
            || !SetPinDefault(LocalizedFallback, TEXT("B"), Fallback.Chinese)
            || !Link(LanguageIsEnglish, UEdGraphSchema_K2::PN_ReturnValue, LocalizedFallback, TEXT("bPickA"))
            || !Link(LocalizedFallback, UEdGraphSchema_K2::PN_ReturnValue, SelectFallback, TEXT("A"))
            || !Link(IdMatches, UEdGraphSchema_K2::PN_ReturnValue, SelectFallback, TEXT("bPickA"))) {
            return false;
        }
        if (FallbackDescriptionSource) {
            if (!Link(FallbackDescriptionSource, UEdGraphSchema_K2::PN_ReturnValue, SelectFallback, TEXT("B"))) {
                return false;
            }
        } else if (!SetPinDefault(SelectFallback, TEXT("B"), TEXT(""))) {
            return false;
        }
        FallbackDescriptionSource = SelectFallback;
        FallbackX += 780;
    }
    UK2Node_CallFunction* FallbackDescriptionNotEmpty = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("NotEqual_StrStr"), FallbackX, Y + 1040);
    UK2Node_CallFunction* SelectResolvedDescription = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("SelectString"), FallbackX + 260, Y + 1040);
    UK2Node_CallFunction* ResolvedDescriptionToText = AddStaticCall(
        Graph, UKismetTextLibrary::StaticClass(), TEXT("Conv_StringToText"), FallbackX + 520, Y + 1040);
    if (!Self || !GetSortedSkills || !ForEachSkillId || !GetLocalizedSkillName || !LocalizedSkillNameToString
        || !SkillIdToString || !SkillIdNotNone || !NameNotEmpty || !NameNotNone || !SearchBoxGet
        || !GetSearchText || !SearchTextToString || !SearchTextIsEmpty || !NameContainsSearch
        || !SearchQueryMatches || !GetPassiveManager
        || !GetSkillData || !BreakSkillData || !NameValid || !IdAndDataValid || !NameAndSearchValid
        || !CatalogEntryValid || !CatalogEntryBranch || !DescIdToString || !SummaryIdToString
        || !DescIdAvailable || !SummaryIdAvailable || !AnyDescriptionId || !SelectDescriptionId
        || !SelectAvailableDescriptionId || !DescriptionStringToName || !GetLocalizedDescription || !DescriptionTextToString
        || EffectValueToStrings.Contains(nullptr) || ReplaceEffectValues.Contains(nullptr)
        || !SelectFormattedDescription || !FormattedDescriptionToText || !BridgeGet
        || !LanguageIdGet || !LanguageIsEnglish || !SelectedIdsGet || !IsSelected || !ExcludedIdsGet || !IsExcluded
        || !FallbackDescriptionSource || !FallbackDescriptionNotEmpty || !SelectResolvedDescription || !ResolvedDescriptionToText
        || !GetController || !CreateEntry || !CastEntry || !InitializeEntry
        || !RankRainbow || !RankLegend || !RankGold3 || !RankGold2 || !RankNormal
        || !RankNegative1 || !RankNegative2 || !RankNegative3
        || !RainbowBranch || !LegendBranch || !Gold3Branch || !Gold2Branch
        || !NormalBranch || !Negative1Branch || !Negative2Branch || !Negative3Branch || AddToGroup.Contains(nullptr)
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, ForEachSkillId, TEXT("Exec"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, GetSortedSkills, TEXT("WorldContextObject"))
        || !Link(GetSortedSkills, TEXT("OutPassiveIdArray"), ForEachSkillId, TEXT("Array"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, GetLocalizedSkillName, TEXT("WorldContextObject"))
        || !Link(ForEachSkillId, TEXT("Array Element"), GetLocalizedSkillName, TEXT("PassiveSkillId"))
        || !Link(GetLocalizedSkillName, TEXT("outName"), LocalizedSkillNameToString, TEXT("InText"))
        || !Link(ForEachSkillId, TEXT("Array Element"), SkillIdToString, TEXT("InName"))
        || !Link(SkillIdToString, UEdGraphSchema_K2::PN_ReturnValue, SkillIdNotNone, TEXT("A"))
        || !SetPinDefault(SkillIdNotNone, TEXT("B"), TEXT("None"))
        || !Link(LocalizedSkillNameToString, UEdGraphSchema_K2::PN_ReturnValue, NameNotEmpty, TEXT("A"))
        || !SetPinDefault(NameNotEmpty, TEXT("B"), TEXT(""))
        || !Link(LocalizedSkillNameToString, UEdGraphSchema_K2::PN_ReturnValue, NameNotNone, TEXT("A"))
        || !SetPinDefault(NameNotNone, TEXT("B"), TEXT("None"))
        || !Link(SearchBoxGet, SearchBox->GetFName(), GetSearchText, UEdGraphSchema_K2::PN_Self)
        || !Link(GetSearchText, UEdGraphSchema_K2::PN_ReturnValue, SearchTextToString, TEXT("InText"))
        || !Link(SearchTextToString, UEdGraphSchema_K2::PN_ReturnValue, SearchTextIsEmpty, TEXT("InString"))
        || !Link(LocalizedSkillNameToString, UEdGraphSchema_K2::PN_ReturnValue, NameContainsSearch, TEXT("SearchIn"))
        || !Link(SearchTextToString, UEdGraphSchema_K2::PN_ReturnValue, NameContainsSearch, TEXT("Substring"))
        || !SetPinDefault(NameContainsSearch, TEXT("bUseCase"), TEXT("false"))
        || !SetPinDefault(NameContainsSearch, TEXT("bSearchFromEnd"), TEXT("false"))
        || !Link(SearchTextIsEmpty, UEdGraphSchema_K2::PN_ReturnValue, SearchQueryMatches, TEXT("A"))
        || !Link(NameContainsSearch, UEdGraphSchema_K2::PN_ReturnValue, SearchQueryMatches, TEXT("B"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, GetPassiveManager, TEXT("WorldContextObject"))
        || !Link(GetPassiveManager, UEdGraphSchema_K2::PN_ReturnValue, GetSkillData, UEdGraphSchema_K2::PN_Self)
        || !Link(ForEachSkillId, TEXT("Array Element"), GetSkillData, TEXT("SkillName"))
        || !Link(GetSkillData, TEXT("outSkillData"), BreakSkillData, PassiveSkillDatabaseRowStruct->GetFName())
        || !Link(NameNotEmpty, UEdGraphSchema_K2::PN_ReturnValue, NameValid, TEXT("A"))
        || !Link(NameNotNone, UEdGraphSchema_K2::PN_ReturnValue, NameValid, TEXT("B"))
        || !Link(SkillIdNotNone, UEdGraphSchema_K2::PN_ReturnValue, IdAndDataValid, TEXT("A"))
        || !Link(GetSkillData, UEdGraphSchema_K2::PN_ReturnValue, IdAndDataValid, TEXT("B"))
        || !Link(NameValid, UEdGraphSchema_K2::PN_ReturnValue, NameAndSearchValid, TEXT("A"))
        || !Link(SearchQueryMatches, UEdGraphSchema_K2::PN_ReturnValue, NameAndSearchValid, TEXT("B"))
        || !Link(IdAndDataValid, UEdGraphSchema_K2::PN_ReturnValue, CatalogEntryValid, TEXT("A"))
        || !Link(NameAndSearchValid, UEdGraphSchema_K2::PN_ReturnValue, CatalogEntryValid, TEXT("B"))
        || !Link(ForEachSkillId, TEXT("LoopBody"), CatalogEntryBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(CatalogEntryValid, UEdGraphSchema_K2::PN_ReturnValue, CatalogEntryBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(BreakSkillData, TEXT("OverrideDescMsgID"), DescIdToString, TEXT("InName"))
        || !Link(BreakSkillData, TEXT("OverrideSummaryTextId"), SummaryIdToString, TEXT("InName"))
        || !Link(DescIdToString, UEdGraphSchema_K2::PN_ReturnValue, DescIdAvailable, TEXT("A"))
        || !SetPinDefault(DescIdAvailable, TEXT("B"), TEXT("None"))
        || !Link(SummaryIdToString, UEdGraphSchema_K2::PN_ReturnValue, SummaryIdAvailable, TEXT("A"))
        || !SetPinDefault(SummaryIdAvailable, TEXT("B"), TEXT("None"))
        || !Link(DescIdAvailable, UEdGraphSchema_K2::PN_ReturnValue, AnyDescriptionId, TEXT("A"))
        || !Link(SummaryIdAvailable, UEdGraphSchema_K2::PN_ReturnValue, AnyDescriptionId, TEXT("B"))
        || !Link(DescIdToString, UEdGraphSchema_K2::PN_ReturnValue, SelectDescriptionId, TEXT("A"))
        || !Link(SummaryIdToString, UEdGraphSchema_K2::PN_ReturnValue, SelectDescriptionId, TEXT("B"))
        || !Link(DescIdAvailable, UEdGraphSchema_K2::PN_ReturnValue, SelectDescriptionId, TEXT("bPickA"))
        || !Link(SelectDescriptionId, UEdGraphSchema_K2::PN_ReturnValue, SelectAvailableDescriptionId, TEXT("A"))
        || !SetPinDefault(SelectAvailableDescriptionId, TEXT("B"), TEXT(""))
        || !Link(AnyDescriptionId, UEdGraphSchema_K2::PN_ReturnValue, SelectAvailableDescriptionId, TEXT("bPickA"))
        || !Link(SelectAvailableDescriptionId, UEdGraphSchema_K2::PN_ReturnValue, DescriptionStringToName, TEXT("InString"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, GetLocalizedDescription, TEXT("WorldContextObject"))
        || !SetPinDefault(GetLocalizedDescription, TEXT("TextCategory"), TEXT("SkillDesc"))
        || !Link(DescriptionStringToName, UEdGraphSchema_K2::PN_ReturnValue, GetLocalizedDescription, TEXT("TextId"))
        || !Link(GetLocalizedDescription, UEdGraphSchema_K2::PN_ReturnValue, DescriptionTextToString, TEXT("InText"))
        || !Link(BridgeGet, PanelBridgeVariableName, LanguageIdGet, UEdGraphSchema_K2::PN_Self)
        || !Link(LanguageIdGet, LanguageIdVariableName, LanguageIsEnglish, TEXT("A"))
        || !SetPinDefault(LanguageIsEnglish, TEXT("B"), TEXT("1"))
        || !Link(BridgeGet, PanelBridgeVariableName, SelectedIdsGet, UEdGraphSchema_K2::PN_Self)
        || !Link(SelectedIdsGet, PassiveFilterIdsVariableName, IsSelected, TEXT("TargetArray"))
        || !Link(ForEachSkillId, TEXT("Array Element"), IsSelected, TEXT("ItemToFind"))
        || !Link(BridgeGet, PanelBridgeVariableName, ExcludedIdsGet, UEdGraphSchema_K2::PN_Self)
        || !Link(ExcludedIdsGet, PassiveExcludeIdsVariableName, IsExcluded, TEXT("TargetArray"))
        || !Link(ForEachSkillId, TEXT("Array Element"), IsExcluded, TEXT("ItemToFind"))
        || !Link(Self, UEdGraphSchema_K2::PN_Self, GetController, TEXT("WorldContextObject"))
        || !SetPinDefault(GetController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetClassPin(CreateEntry, TEXT("WidgetType"), PassiveEntryClass)
        || !Link(CatalogEntryBranch, UEdGraphSchema_K2::PN_Then, CreateEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(GetController, UEdGraphSchema_K2::PN_ReturnValue, CreateEntry, TEXT("OwningPlayer"))
        || !Link(CreateEntry, UEdGraphSchema_K2::PN_Then, CastEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreateEntry, UEdGraphSchema_K2::PN_ReturnValue, CastEntry, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastEntry, UEdGraphSchema_K2::PN_CastSucceeded, InitializeEntry, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastEntry, CastEntry->GetCastResultPin()->PinName, InitializeEntry, UEdGraphSchema_K2::PN_Self)
        || !Link(BridgeGet, PanelBridgeVariableName, InitializeEntry, TEXT("Bridge"))
        || !Link(ForEachSkillId, TEXT("Array Element"), InitializeEntry, TEXT("SkillId"))
        || !Link(LocalizedSkillNameToString, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("SkillName"))
        // __DEPRECATED_20260718__ [reason: selected internal IDs can override empty or malformed game descriptions]
        // || !Link(FormattedDescriptionToText, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("Description"))
        || !Link(ResolvedDescriptionToText, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("Description"))
        || !Link(IsSelected, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("Selected"))
        || !Link(IsExcluded, UEdGraphSchema_K2::PN_ReturnValue, InitializeEntry, TEXT("Excluded"))
        // __DEPRECATED_20260718__ [reason: rarity is an exact Rank mapping, not a lottery-weight branch]
        // || !Link(InitializeEntry, UEdGraphSchema_K2::PN_Then, SpecialBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(InitializeEntry, UEdGraphSchema_K2::PN_Then, RainbowBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankRainbow, TEXT("A"))
        || !SetPinDefault(RankRainbow, TEXT("B"), TEXT("5"))
        || !Link(RankRainbow, UEdGraphSchema_K2::PN_ReturnValue, RainbowBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(RainbowBranch, UEdGraphSchema_K2::PN_Else, LegendBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankLegend, TEXT("A"))
        || !SetPinDefault(RankLegend, TEXT("B"), TEXT("4"))
        || !Link(RankLegend, UEdGraphSchema_K2::PN_ReturnValue, LegendBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(LegendBranch, UEdGraphSchema_K2::PN_Else, Gold3Branch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankGold3, TEXT("A"))
        || !SetPinDefault(RankGold3, TEXT("B"), TEXT("3"))
        || !Link(RankGold3, UEdGraphSchema_K2::PN_ReturnValue, Gold3Branch, UEdGraphSchema_K2::PN_Condition)
        || !Link(Gold3Branch, UEdGraphSchema_K2::PN_Else, Gold2Branch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankGold2, TEXT("A"))
        || !SetPinDefault(RankGold2, TEXT("B"), TEXT("2"))
        || !Link(RankGold2, UEdGraphSchema_K2::PN_ReturnValue, Gold2Branch, UEdGraphSchema_K2::PN_Condition)
        || !Link(Gold2Branch, UEdGraphSchema_K2::PN_Else, NormalBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankNormal, TEXT("A"))
        || !SetPinDefault(RankNormal, TEXT("B"), TEXT("1"))
        || !Link(RankNormal, UEdGraphSchema_K2::PN_ReturnValue, NormalBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(NormalBranch, UEdGraphSchema_K2::PN_Else, Negative1Branch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankNegative1, TEXT("A"))
        || !SetPinDefault(RankNegative1, TEXT("B"), TEXT("-1"))
        || !Link(RankNegative1, UEdGraphSchema_K2::PN_ReturnValue, Negative1Branch, UEdGraphSchema_K2::PN_Condition)
        || !Link(Negative1Branch, UEdGraphSchema_K2::PN_Else, Negative2Branch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankNegative2, TEXT("A"))
        || !SetPinDefault(RankNegative2, TEXT("B"), TEXT("-2"))
        || !Link(RankNegative2, UEdGraphSchema_K2::PN_ReturnValue, Negative2Branch, UEdGraphSchema_K2::PN_Condition)
        || !Link(Negative2Branch, UEdGraphSchema_K2::PN_Else, Negative3Branch, UEdGraphSchema_K2::PN_Execute)
        || !Link(BreakSkillData, TEXT("Rank"), RankNegative3, TEXT("A"))
        || !SetPinDefault(RankNegative3, TEXT("B"), TEXT("-3"))
        || !Link(RankNegative3, UEdGraphSchema_K2::PN_ReturnValue, Negative3Branch, UEdGraphSchema_K2::PN_Condition)) {
        return false;
    }

    UEdGraphNode* DescriptionSource = DescriptionTextToString;
    for (int32 Index = 0; Index < 4; ++Index) {
        const FName EffectValuePin(*FString::Printf(TEXT("EffectValue%d"), Index + 1));
        const FString Placeholder = FString::Printf(TEXT("{EffectValue%d}"), Index + 1);
        if (!Link(BreakSkillData, EffectValuePin, EffectValueToStrings[Index], TEXT("InDouble"))
            || !Link(DescriptionSource, UEdGraphSchema_K2::PN_ReturnValue, ReplaceEffectValues[Index], TEXT("SourceString"))
            || !SetPinDefault(ReplaceEffectValues[Index], TEXT("From"), Placeholder)
            || !Link(EffectValueToStrings[Index], UEdGraphSchema_K2::PN_ReturnValue, ReplaceEffectValues[Index], TEXT("To"))) {
            return false;
        }
        DescriptionSource = ReplaceEffectValues[Index];
    }
    if (!Link(DescriptionSource, UEdGraphSchema_K2::PN_ReturnValue, SelectFormattedDescription, TEXT("A"))
        || !SetPinDefault(SelectFormattedDescription, TEXT("B"), TEXT(""))
        || !Link(AnyDescriptionId, UEdGraphSchema_K2::PN_ReturnValue, SelectFormattedDescription, TEXT("bPickA"))
        || !Link(SelectFormattedDescription, UEdGraphSchema_K2::PN_ReturnValue, FormattedDescriptionToText, TEXT("InString"))
        || !Link(FallbackDescriptionSource, UEdGraphSchema_K2::PN_ReturnValue, FallbackDescriptionNotEmpty, TEXT("A"))
        || !SetPinDefault(FallbackDescriptionNotEmpty, TEXT("B"), TEXT(""))
        || !Link(FallbackDescriptionSource, UEdGraphSchema_K2::PN_ReturnValue, SelectResolvedDescription, TEXT("A"))
        || !Link(SelectFormattedDescription, UEdGraphSchema_K2::PN_ReturnValue, SelectResolvedDescription, TEXT("B"))
        || !Link(FallbackDescriptionNotEmpty, UEdGraphSchema_K2::PN_ReturnValue, SelectResolvedDescription, TEXT("bPickA"))
        || !Link(SelectResolvedDescription, UEdGraphSchema_K2::PN_ReturnValue, ResolvedDescriptionToText, TEXT("InString"))) {
        return false;
    }

    const TArray<UK2Node_IfThenElse*> Branches = {
        RainbowBranch, LegendBranch, Gold3Branch, Gold2Branch,
        NormalBranch, Negative1Branch, Negative2Branch, Negative3Branch,
    };
    for (int32 Index = 0; Index < Groups.Num(); ++Index) {
        UK2Node_CallFunction* AddChild = AddToGroup[Index];
        UEdGraphNode* Source = Branches[Index];
        UK2Node_VariableGet* GroupGet = AddVariableGet(Graph, Groups[Index]->GetFName(), X + 3120, Y + Index * 180 + 100);
        if (!GroupGet
            || !Link(Source, UEdGraphSchema_K2::PN_Then, AddChild, UEdGraphSchema_K2::PN_Execute)
            || !Link(GroupGet, Groups[Index]->GetFName(), AddChild, UEdGraphSchema_K2::PN_Self)
            || !Link(CastEntry, CastEntry->GetCastResultPin()->PinName, AddChild, TEXT("Content"))) {
            return false;
        }
    }
    return true;
}

bool BuildPanelPassiveSearchEvents(
    UWidgetBlueprint* Blueprint,
    UEditableTextBox* SearchBox,
    UButton* SearchButton,
    UButton* ClearSearchButton,
    int32 Y) {
    if (!Blueprint || !SearchBox || !SearchButton || !ClearSearchButton) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Committed = AddEditableTextBoxCommittedEvent(Blueprint, SearchBox, -1600, Y);
    UK2Node_Self* CommittedSelf = AddSelfNode(Graph, -1360, Y + 160);
    UK2Node_CallFunction* PopulateFromCommit = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelPopulatePassiveCatalogEventName.ToString(), -1080, Y);

    UK2Node_ComponentBoundEvent* SearchClicked = AddButtonEvent(Blueprint, SearchButton, -1600, Y + 360);
    UK2Node_Self* SearchSelf = AddSelfNode(Graph, -1360, Y + 520);
    UK2Node_CallFunction* PopulateFromButton = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelPopulatePassiveCatalogEventName.ToString(), -1080, Y + 360);

    UK2Node_ComponentBoundEvent* ClearClicked = AddButtonEvent(Blueprint, ClearSearchButton, -1600, Y + 720);
    UK2Node_VariableGet* SearchBoxGet = AddVariableGet(Graph, SearchBox->GetFName(), -1360, Y + 880);
    UK2Node_CallFunction* ClearSearchText = AddStaticCall(
        Graph, UEditableTextBox::StaticClass(), TEXT("SetText"), -1080, Y + 720);
    UK2Node_Self* ClearSelf = AddSelfNode(Graph, -820, Y + 880);
    UK2Node_CallFunction* PopulateAfterClear = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelPopulatePassiveCatalogEventName.ToString(), -540, Y + 720);

    return Graph && Committed && CommittedSelf && PopulateFromCommit
        && SearchClicked && SearchSelf && PopulateFromButton
        && ClearClicked && SearchBoxGet && ClearSearchText && ClearSelf && PopulateAfterClear
        && Link(Committed, UEdGraphSchema_K2::PN_Then, PopulateFromCommit, UEdGraphSchema_K2::PN_Execute)
        && Link(CommittedSelf, UEdGraphSchema_K2::PN_Self, PopulateFromCommit, UEdGraphSchema_K2::PN_Self)
        && Link(SearchClicked, UEdGraphSchema_K2::PN_Then, PopulateFromButton, UEdGraphSchema_K2::PN_Execute)
        && Link(SearchSelf, UEdGraphSchema_K2::PN_Self, PopulateFromButton, UEdGraphSchema_K2::PN_Self)
        && Link(ClearClicked, UEdGraphSchema_K2::PN_Then, ClearSearchText, UEdGraphSchema_K2::PN_Execute)
        && Link(SearchBoxGet, SearchBox->GetFName(), ClearSearchText, UEdGraphSchema_K2::PN_Self)
        && SetPinDefaultText(ClearSearchText, TEXT("InText"), FText::GetEmpty())
        && Link(ClearSearchText, UEdGraphSchema_K2::PN_Then, PopulateAfterClear, UEdGraphSchema_K2::PN_Execute)
        && Link(ClearSelf, UEdGraphSchema_K2::PN_Self, PopulateAfterClear, UEdGraphSchema_K2::PN_Self);
}

bool BuildPanelClearFiltersEvent(
    UWidgetBlueprint* Blueprint,
    UButton* Button,
    UClass* ModActorClass,
    bool bClearAll,
    const TArray<FPanelNumericControlV2>& FilterNumericControls,
    const TArray<UCheckBox*>& ElementToggles,
    UTextBlock* GenderStatus,
    const TArray<UButton*>& GenderButtons,
    UTextBlock* LuckyStatus,
    const TArray<UButton*>& LuckyButtons,
    UTextBlock* BossStatus,
    const TArray<UButton*>& BossButtons,
    UTextBlock* CollectionStatus,
    const TArray<UButton*>& CollectionButtons,
    int32 Y) {
    if (!Blueprint || !Button || !ModActorClass
        || (bClearAll && (FilterNumericControls.Num() != 6 || ElementToggles.Num() != 9
            || !CollectionStatus || CollectionButtons.Num() != 3))) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    UK2Node_ComponentBoundEvent* Event = AddButtonEvent(Blueprint, Button, -1600, Y);
    UK2Node_VariableGet* BridgeGet = AddVariableGet(Graph, PanelBridgeVariableName, -1360, Y + 160);
    UK2Node_VariableGet* FilterIdsGet = AddExternalVariableGet(
        Graph, PassiveFilterIdsVariableName, ModActorClass, -1080, Y + 160);
    UK2Node_CallArrayFunction* ClearFilterIds = AddArrayCall(Graph, TEXT("Array_Clear"), -800, Y);
    UK2Node_VariableGet* ExcludeIdsGet = AddExternalVariableGet(
        Graph, PassiveExcludeIdsVariableName, ModActorClass, -800, Y + 160);
    UK2Node_CallArrayFunction* ClearExcludeIds = AddArrayCall(Graph, TEXT("Array_Clear"), -520, Y);
    if (!Graph || !Event || !BridgeGet || !FilterIdsGet || !ClearFilterIds || !ExcludeIdsGet || !ClearExcludeIds
        || !Link(Event, UEdGraphSchema_K2::PN_Then, ClearFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, FilterIdsGet, UEdGraphSchema_K2::PN_Self)
        || !Link(FilterIdsGet, PassiveFilterIdsVariableName, ClearFilterIds, TEXT("TargetArray"))
        || !Link(ClearFilterIds, UEdGraphSchema_K2::PN_Then, ClearExcludeIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(BridgeGet, PanelBridgeVariableName, ExcludeIdsGet, UEdGraphSchema_K2::PN_Self)
        || !Link(ExcludeIdsGet, PassiveExcludeIdsVariableName, ClearExcludeIds, TEXT("TargetArray"))) {
        return false;
    }

    UEdGraphNode* ExecTail = ClearExcludeIds;
    int32 X = -240;
    if (!AppendExternalAssignment(
            Graph, ModActorClass, BridgeGet, ExecTail, PassiveIncludeTextVariableName, TEXT(""), X, Y)
        || !AppendExternalAssignment(
            Graph, ModActorClass, BridgeGet, ExecTail, PassiveExcludeTextVariableName, TEXT(""), X + 300, Y)) {
        return false;
    }
    X += 600;
    if (bClearAll) {
        UK2Node_VariableGet* SpeciesIdsGet = AddExternalVariableGet(
            Graph, SpeciesFilterIdsVariableName, ModActorClass, X, Y + 160);
        UK2Node_CallArrayFunction* ClearSpeciesIds = AddArrayCall(Graph, TEXT("Array_Clear"), X + 280, Y);
        if (!SpeciesIdsGet || !ClearSpeciesIds
            || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, ClearSpeciesIds, UEdGraphSchema_K2::PN_Execute)
            || !Link(BridgeGet, PanelBridgeVariableName, SpeciesIdsGet, UEdGraphSchema_K2::PN_Self)
            || !Link(SpeciesIdsGet, SpeciesFilterIdsVariableName, ClearSpeciesIds, TEXT("TargetArray"))) {
            return false;
        }
        ExecTail = ClearSpeciesIds;
        if (!AppendExternalAssignment(
                Graph, ModActorClass, BridgeGet, ExecTail,
                SpeciesFilterTextVariableName, TEXT(""), X + 560, Y)) {
            return false;
        }
        X += 860;
        const TArray<FPanelControlAssignment> Assignments = {
            {LevelMinVariableName, TEXT("0")},
            {LevelMaxVariableName, TEXT("0")},
            {DistanceMaxVariableName, TEXT("330")},
            {IvMinVariableName, TEXT("0")},
            {IvHpMinVariableName, TEXT("0")},
            {IvAttackMinVariableName, TEXT("0")},
            {IvDefenseMinVariableName, TEXT("0")},
            {ElementNormalVariableName, TEXT("false")},
            {ElementFireVariableName, TEXT("false")},
            {ElementWaterVariableName, TEXT("false")},
            {ElementLeafVariableName, TEXT("false")},
            {ElementElectricityVariableName, TEXT("false")},
            {ElementIceVariableName, TEXT("false")},
            {ElementEarthVariableName, TEXT("false")},
            {ElementDarkVariableName, TEXT("false")},
            {ElementDragonVariableName, TEXT("false")},
            {GenderFilterIdVariableName, TEXT("0")},
            {LuckyFilterIdVariableName, TEXT("0")},
            {BossFilterIdVariableName, TEXT("0")},
            {CollectionFilterIdVariableName, TEXT("0")},
        };
        for (const FPanelControlAssignment& Assignment : Assignments) {
            if (!AppendExternalAssignment(
                    Graph, ModActorClass, BridgeGet, ExecTail,
                    Assignment.Name, Assignment.Value, X, Y)) {
                return false;
            }
            X += 300;
        }
    }
    if (!AppendExternalIntegerIncrement(
            Graph, ModActorClass, BridgeGet, PassiveFilterRevisionVariableName, ExecTail, X, Y)
        || !AppendExternalIntegerIncrement(
            Graph, ModActorClass, BridgeGet, ControlRevisionVariableName, ExecTail, X + 800, Y)) {
        return false;
    }
    X += 1600;

    if (bClearAll) {
        const TArray<float> NumericValues = {0.0f, 0.0f, 330.0f, 0.0f, 0.0f, 0.0f};
        for (int32 Index = 0; Index < FilterNumericControls.Num(); ++Index) {
            const FPanelNumericControlV2& Control = FilterNumericControls[Index];
            UK2Node_VariableGet* SliderGet = AddVariableGet(Graph, Control.Slider->GetFName(), X, Y + 160);
            UK2Node_CallFunction* SetSlider = AddStaticCall(Graph, USlider::StaticClass(), TEXT("SetValue"), X + 260, Y);
            UK2Node_VariableGet* SpinGet = AddVariableGet(Graph, Control.SpinBox->GetFName(), X + 520, Y + 160);
            UK2Node_CallFunction* SetSpin = AddStaticCall(Graph, USpinBox::StaticClass(), TEXT("SetValue"), X + 780, Y);
            const FString Value = FString::SanitizeFloat(NumericValues[Index]);
            if (!SliderGet || !SetSlider || !SpinGet || !SetSpin
                || !SetPinDefault(SetSlider, TEXT("InValue"), Value)
                || !SetPinDefault(SetSpin, TEXT("NewValue"), Value)
                || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetSlider, UEdGraphSchema_K2::PN_Execute)
                || !Link(SliderGet, Control.Slider->GetFName(), SetSlider, UEdGraphSchema_K2::PN_Self)
                || !Link(SetSlider, UEdGraphSchema_K2::PN_Then, SetSpin, UEdGraphSchema_K2::PN_Execute)
                || !Link(SpinGet, Control.SpinBox->GetFName(), SetSpin, UEdGraphSchema_K2::PN_Self)) {
                return false;
            }
            ExecTail = SetSpin;
            X += 1080;
        }
        for (UCheckBox* Toggle : ElementToggles) {
            UK2Node_VariableGet* ToggleGet = AddVariableGet(Graph, Toggle->GetFName(), X, Y + 160);
            UK2Node_CallFunction* SetUnchecked = AddStaticCall(Graph, UCheckBox::StaticClass(), TEXT("SetIsChecked"), X + 260, Y);
            if (!ToggleGet || !SetUnchecked
                || !SetPinDefault(SetUnchecked, TEXT("InIsChecked"), TEXT("false"))
                || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, SetUnchecked, UEdGraphSchema_K2::PN_Execute)
                || !Link(ToggleGet, Toggle->GetFName(), SetUnchecked, UEdGraphSchema_K2::PN_Self)) {
                return false;
            }
            ExecTail = SetUnchecked;
            X += 540;
        }
        if (!AppendTextAssignment(Graph, ExecTail, GenderStatus->GetFName(), TEXT("当前：全部"), X, Y)
            || !AppendPanelSegmentVisualsConstant(Graph, GenderButtons, 0, ExecTail, X + 560, Y)
            || !AppendTextAssignment(Graph, ExecTail, LuckyStatus->GetFName(), TEXT("当前：全部"), X + 2300, Y)
            || !AppendPanelSegmentVisualsConstant(Graph, LuckyButtons, 0, ExecTail, X + 2860, Y)
            || !AppendTextAssignment(Graph, ExecTail, BossStatus->GetFName(), TEXT("当前：全部"), X + 4600, Y)
            || !AppendPanelSegmentVisualsConstant(Graph, BossButtons, 0, ExecTail, X + 5160, Y)
            || !AppendTextAssignment(Graph, ExecTail, CollectionStatus->GetFName(), TEXT("当前：全部"), X + 6900, Y)
            || !AppendPanelSegmentVisualsConstant(Graph, CollectionButtons, 0, ExecTail, X + 7460, Y)) {
            return false;
        }
        X += 9200;
    }

    UK2Node_Self* Self = AddSelfNode(Graph, X, Y + 160);
    UK2Node_CallFunction* PopulateCatalog = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelPopulatePassiveCatalogEventName.ToString(), X + 260, Y);
    if (!Self || !PopulateCatalog
        || !Link(ExecTail, UEdGraphSchema_K2::PN_Then, PopulateCatalog, UEdGraphSchema_K2::PN_Execute)
        || !Link(Self, UEdGraphSchema_K2::PN_Self, PopulateCatalog, UEdGraphSchema_K2::PN_Self)) {
        return false;
    }
    if (!bClearAll) {
        return true;
    }
    UK2Node_CallFunction* RenderPalCatalog = AddStaticCall(
        Graph, Blueprint->GeneratedClass, *PanelRenderPalCatalogEventName.ToString(), X + 520, Y);
    return RenderPalCatalog
        && Link(PopulateCatalog, UEdGraphSchema_K2::PN_Then, RenderPalCatalog, UEdGraphSchema_K2::PN_Execute)
        && Link(Self, UEdGraphSchema_K2::PN_Self, RenderPalCatalog, UEdGraphSchema_K2::PN_Self);
}

// __DEPRECATED_20260717__ [reason: replaced by the compact slider-and-toggle panel; retained for rollback]
bool BuildPanelDeprecated20260717(UWidgetBlueprint* Blueprint, UClass* ModActorClass) {
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
    Size->SetHeightOverride(820.0f);
    Border->SetBrushColor(FLinearColor(0.045f, 0.055f, 0.065f, 0.97f));
    Border->SetPadding(FMargin(18.0f));
    Border->AddChild(Content);
    Size->AddChild(Border);
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Size);
    PanelSlot->SetPosition(FVector2D(28.0f, 72.0f));
    PanelSlot->SetSize(FVector2D(540.0f, 980.0f));
    Blueprint->WidgetTree->RootWidget = Canvas;

    UTextBlock* Title = AddPanelText(Blueprint, Content, TEXT("ESP_TitleText"), TEXT("帕鲁资源 ESP"), 24);
    UTextBlock* RuntimeStatus = AddPanelText(Blueprint, Content, TEXT("ESP_RuntimeStatusText"), TEXT("Mod 状态 / Mod: 开启 / On"), 16);
    UHorizontalBox* RuntimeRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_RuntimeRow"));
    Content->AddChild(RuntimeRow);
    UButton* RuntimeOn = AddPanelButton(Blueprint, RuntimeRow, TEXT("ESP_RuntimeOnButton"), TEXT("ESP_RuntimeOnText"), TEXT("开启 Mod"));
    UButton* RuntimeOff = AddPanelButton(Blueprint, RuntimeRow, TEXT("ESP_RuntimeOffButton"), TEXT("ESP_RuntimeOffText"), TEXT("关闭 Mod"));

    UTextBlock* StyleHeading = AddPanelText(Blueprint, Content, TEXT("ESP_StyleHeadingText"), TEXT("显示样式"), 18);
    UHorizontalBox* TopGuideRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_TopGuideRow"));
    Content->AddChild(TopGuideRow);
    UButton* TopGuideOn = AddPanelButton(Blueprint, TopGuideRow, TEXT("ESP_TopGuideOnButton"), TEXT("ESP_TopGuideOnText"), TEXT("显示顶部引导线"));
    UButton* TopGuideOff = AddPanelButton(Blueprint, TopGuideRow, TEXT("ESP_TopGuideOffButton"), TEXT("ESP_TopGuideOffText"), TEXT("隐藏顶部引导线"));
    UHorizontalBox* MetadataStyleRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_MetadataStyleRow"));
    Content->AddChild(MetadataStyleRow);
    UCheckBox* ShowLevel = Blueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("ESP_ShowLevelCheckBox"));
    ShowLevel->bIsVariable = true;
    ShowLevel->SetIsChecked(true);
    MetadataStyleRow->AddChild(ShowLevel);
    UTextBlock* ShowLevelText = AddPanelText(Blueprint, MetadataStyleRow, TEXT("ESP_ShowLevelText"), TEXT("显示等级"), 14);
    USpacer* MetadataSpacer = Blueprint->WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("ESP_MetadataSpacer"));
    MetadataSpacer->SetSize(FVector2D(28.0f, 1.0f));
    MetadataStyleRow->AddChild(MetadataSpacer);
    UCheckBox* ShowDistance = Blueprint->WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("ESP_ShowDistanceCheckBox"));
    ShowDistance->bIsVariable = true;
    ShowDistance->SetIsChecked(true);
    MetadataStyleRow->AddChild(ShowDistance);
    UTextBlock* ShowDistanceText = AddPanelText(Blueprint, MetadataStyleRow, TEXT("ESP_ShowDistanceText"), TEXT("显示距离"), 14);

    UTextBlock* PresetHeading = AddPanelText(Blueprint, Content, TEXT("ESP_PresetHeadingText"), TEXT("目标显示上限 (1-512)"), 18);
    UHorizontalBox* PresetRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_PresetRow"));
    Content->AddChild(PresetRow);
    USpinBox* DisplayTargetLimit = AddPanelSpinBox(Blueprint, PresetRow, TEXT("ESP_DisplayTargetLimitInput"), 64.0f, 1.0f, 512.0f, 1.0f);

    UTextBlock* LevelHeading = AddPanelText(Blueprint, Content, TEXT("ESP_LevelHeadingText"), TEXT("等级筛选"), 18);
    UTextBlock* LevelMinHeading = AddPanelText(Blueprint, Content, TEXT("ESP_LevelMinHeadingText"), TEXT("等级下限 (0=不限)"), 14);
    UHorizontalBox* LevelMinRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_LevelMinRow"));
    Content->AddChild(LevelMinRow);
    USpinBox* LevelMin = AddPanelSpinBox(Blueprint, LevelMinRow, TEXT("ESP_LevelMinInput"), 0.0f, 0.0f, 100.0f, 1.0f);
    UTextBlock* LevelMaxHeading = AddPanelText(Blueprint, Content, TEXT("ESP_LevelMaxHeadingText"), TEXT("等级上限 (0=不限)"), 14);
    UHorizontalBox* LevelMaxRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_LevelMaxRow"));
    Content->AddChild(LevelMaxRow);
    USpinBox* LevelMax = AddPanelSpinBox(Blueprint, LevelMaxRow, TEXT("ESP_LevelMaxInput"), 0.0f, 0.0f, 100.0f, 1.0f);

    UTextBlock* DistanceHeading = AddPanelText(Blueprint, Content, TEXT("ESP_DistanceHeadingText"), TEXT("距离筛选"), 18);
    UTextBlock* DistanceMinHeading = AddPanelText(Blueprint, Content, TEXT("ESP_DistanceMinHeadingText"), TEXT("距离下限 (0=不限，米)"), 14);
    UHorizontalBox* DistanceMinRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_DistanceMinRow"));
    Content->AddChild(DistanceMinRow);
    USpinBox* DistanceMin = AddPanelSpinBox(Blueprint, DistanceMinRow, TEXT("ESP_DistanceMinInput"), 0.0f, 0.0f, 50000.0f, 10.0f);
    UTextBlock* DistanceMaxHeading = AddPanelText(Blueprint, Content, TEXT("ESP_DistanceMaxHeadingText"), TEXT("距离上限 (0=不限，米)"), 14);
    UHorizontalBox* DistanceMaxRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_DistanceMaxRow"));
    Content->AddChild(DistanceMaxRow);
    USpinBox* DistanceMax = AddPanelSpinBox(Blueprint, DistanceMaxRow, TEXT("ESP_DistanceMaxInput"), 0.0f, 0.0f, 50000.0f, 10.0f);

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
    UTextBlock* ModeStatus = AddPanelText(Blueprint, Advanced, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 安全快照 / Safe snapshot"), 15);
    UHorizontalBox* ModeRowA = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ModeRowA"));
    UHorizontalBox* ModeRowB = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ModeRowB"));
    Advanced->AddChild(ModeRowA);
    Advanced->AddChild(ModeRowB);
    UButton* ModeOff = AddPanelButton(Blueprint, ModeRowA, TEXT("ESP_ModeOffButton"), TEXT("ESP_ModeOffText"), TEXT("关闭"));
    UButton* ModeSnapshot = AddPanelButton(Blueprint, ModeRowA, TEXT("ESP_ModeSnapshotButton"), TEXT("ESP_ModeSnapshotText"), TEXT("单次快照"));
    UButton* ModeChunked = AddPanelButton(Blueprint, ModeRowB, TEXT("ESP_ModeChunkedButton"), TEXT("ESP_ModeChunkedText"), TEXT("安全快照"));
    UButton* ModeEvent = AddPanelButton(Blueprint, ModeRowB, TEXT("ESP_ModeEventButton"), TEXT("ESP_ModeEventText"), TEXT("事件优先"));
    UTextBlock* CaptureStatus = AddPanelText(Blueprint, Advanced, TEXT("ESP_CaptureStatusText"), TEXT("采集状态 / Capture: 已停止 / Stopped"), 15);
    UHorizontalBox* CaptureRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_CaptureRow"));
    Advanced->AddChild(CaptureRow);
    UButton* CaptureStart = AddPanelButton(Blueprint, CaptureRow, TEXT("ESP_CaptureStartButton"), TEXT("ESP_CaptureStartText"), TEXT("开始采集"));
    UButton* CaptureStop = AddPanelButton(Blueprint, CaptureRow, TEXT("ESP_CaptureStopButton"), TEXT("ESP_CaptureStopText"), TEXT("停止采集"));
    UButton* AdvancedCollapse = AddPanelButton(Blueprint, Advanced, TEXT("ESP_AdvancedCollapseButton"), TEXT("ESP_AdvancedCollapseText"), TEXT("收起高级诊断"));

    if (!Title || !RuntimeStatus || !RuntimeOn || !RuntimeOff || !StyleHeading || !TopGuideOn || !TopGuideOff
        || !MetadataStyleRow || !ShowLevel || !ShowLevelText || !MetadataSpacer || !ShowDistance || !ShowDistanceText
        || !PresetHeading || !DisplayTargetLimit || !LevelHeading || !LevelMinHeading || !LevelMin
        || !LevelMaxHeading || !LevelMax || !DistanceHeading || !DistanceMinHeading || !DistanceMin
        || !DistanceMaxHeading || !DistanceMax
        || !LanguageHeading || !Chinese || !English || !AdvancedExpand || !ModeHeading
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
        || !Control(TopGuideOn, {{ShowTopGuideLineVariableName, TEXT("true")}}, NAME_None, TEXT(""))
        || !Control(TopGuideOff, {{ShowTopGuideLineVariableName, TEXT("false")}}, NAME_None, TEXT(""))
        || !Control(ModeOff, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("0")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 关闭 / Off"))
        || !Control(ModeSnapshot, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("1")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 单次快照 / Snapshot once"))
        || !Control(ModeChunked, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("2")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 安全快照 / Safe snapshot"))
        || !Control(ModeEvent, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("3")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式 / Mode: 事件优先 / Event first"))
        || !Control(CaptureStart, {{CaptureRequestedVariableName, TEXT("true")}}, TEXT("ESP_CaptureStatusText"), TEXT("采集状态 / Capture: 采集中 / Recording"))
        || !Control(CaptureStop, {{CaptureRequestedVariableName, TEXT("false")}}, TEXT("ESP_CaptureStatusText"), TEXT("采集状态 / Capture: 已停止 / Stopped"))) {
        return false;
    }

    if (!BuildPanelNumericEvent(Blueprint, DisplayTargetLimit, ModActorClass, DisplayTargetLimitVariableName, Y)
        || !BuildPanelNumericEvent(Blueprint, LevelMin, ModActorClass, LevelMinVariableName, Y + 360)
        || !BuildPanelNumericEvent(Blueprint, LevelMax, ModActorClass, LevelMaxVariableName, Y + 720)
        || !BuildPanelNumericEvent(Blueprint, DistanceMin, ModActorClass, DistanceMinVariableName, Y + 1080)
        || !BuildPanelNumericEvent(Blueprint, DistanceMax, ModActorClass, DistanceMaxVariableName, Y + 1440)
        || !BuildPanelBooleanEvent(Blueprint, ShowLevel, ModActorClass, ShowLevelVariableName, Y + 1800)
        || !BuildPanelBooleanEvent(Blueprint, ShowDistance, ModActorClass, ShowDistanceVariableName, Y + 2160)) {
        return false;
    }
    Y += 2520;
    if (!BuildPanelInitializeControls(
        Blueprint,
        DisplayTargetLimit,
        LevelMin,
        LevelMax,
        DistanceMin,
        DistanceMax,
        ShowLevel,
        ShowDistance,
        Y)) {
        return false;
    }
    Y += 720;

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
        {TEXT("ESP_StyleHeadingText"), TEXT("显示样式"), TEXT("Display style")},
        {TEXT("ESP_TopGuideOnText"), TEXT("显示顶部引导线"), TEXT("Show top guides")},
        {TEXT("ESP_TopGuideOffText"), TEXT("隐藏顶部引导线"), TEXT("Hide top guides")},
        {TEXT("ESP_ShowLevelText"), TEXT("显示等级"), TEXT("Show level")},
        {TEXT("ESP_ShowDistanceText"), TEXT("显示距离"), TEXT("Show distance")},
        {TEXT("ESP_PresetHeadingText"), TEXT("目标显示上限 (1-512)"), TEXT("Display target limit (1-512)")},
        {TEXT("ESP_LevelHeadingText"), TEXT("等级筛选"), TEXT("Level filter")},
        {TEXT("ESP_LevelMinHeadingText"), TEXT("等级下限 (0=不限)"), TEXT("Minimum level (0=any)")},
        {TEXT("ESP_LevelMaxHeadingText"), TEXT("等级上限 (0=不限)"), TEXT("Maximum level (0=any)")},
        {TEXT("ESP_DistanceHeadingText"), TEXT("距离筛选"), TEXT("Distance filter")},
        {TEXT("ESP_DistanceMinHeadingText"), TEXT("距离下限 (0=不限，米)"), TEXT("Minimum distance (0=any, meters)")},
        {TEXT("ESP_DistanceMaxHeadingText"), TEXT("距离上限 (0=不限，米)"), TEXT("Maximum distance (0=any, meters)")},
        {TEXT("ESP_AdvancedExpandText"), TEXT("高级诊断"), TEXT("Advanced diagnostics")},
        {TEXT("ESP_ModeHeadingText"), TEXT("实验模式"), TEXT("Experiment mode")},
        {TEXT("ESP_ModeOffText"), TEXT("关闭"), TEXT("Off")},
        {TEXT("ESP_ModeSnapshotText"), TEXT("单次快照"), TEXT("Snapshot once")},
        {TEXT("ESP_ModeChunkedText"), TEXT("安全快照"), TEXT("Safe snapshot")},
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

bool BuildPanel(
    UWidgetBlueprint* Blueprint,
    UClass* ModActorClass,
    UClass* PassiveEntryClass,
    UClass* PalEntryClass,
    UClass* PalUtilityClass,
    UClass* PalUIUtilityClass,
    UClass* DatabaseCharacterParameterClass,
    UClass* PassiveSkillManagerClass,
    UScriptStruct* PassiveSkillDatabaseRowStruct,
    UScriptStruct* CharacterParameterDatabaseRowStruct,
    UScriptStruct* DropItemDatabaseRowStruct,
    UEnum* ElementEnum,
    UEnum* WorkSuitabilityEnum,
    UClass* MasterDataTablesUtilityClass) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPanelV2 begin blueprint=%s"), *GetNameSafe(Blueprint));
    if (!Blueprint || !Blueprint->WidgetTree || !ModActorClass || !ModActorClass->FindFunctionByName(PanelToggleEventName)
        || !PassiveEntryClass || !PalEntryClass
        || !PalUtilityClass || !PalUIUtilityClass || !DatabaseCharacterParameterClass || !PassiveSkillManagerClass
        || !PassiveSkillDatabaseRowStruct || !CharacterParameterDatabaseRowStruct || !DropItemDatabaseRowStruct
        || !ElementEnum || !WorkSuitabilityEnum || !MasterDataTablesUtilityClass) {
        return false;
    }
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph) {
        return false;
    }
    ClearGraph(Graph);
    if (!EnsureMemberVariable(Blueprint, PanelBridgeVariableName, ObjectPin(ModActorClass))
        || !EnsureMemberVariable(Blueprint, PalCatalogLoadedVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PalCatalogIdsVariableName, NameArrayPin())
        || !EnsureMemberVariable(Blueprint, PalCatalogZukanIndicesVariableName, IntArrayPin())
        || !EnsureMemberVariable(Blueprint, PalCatalogTableVariableName, ObjectPin(UDataTable::StaticClass()))
        || !EnsureMemberVariable(Blueprint, PalCatalogResultCountVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PalCatalogSuppressEventsVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PalCatalogInsertIndexVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PalCatalogInsertFoundVariableName, BoolPin(), TEXT("false"))
        || !EnsureMemberVariable(Blueprint, PalCatalogSelectedNamesVariableName, StringPin())
        || !EnsureMemberVariable(Blueprint, PalCatalogSelectedCountVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PalCatalogSortKeysVariableName, IntArrayPin())
        || !EnsureMemberVariable(Blueprint, PalCatalogSortModeVariableName, IntPin(), TEXT("0"))
        || !EnsureMemberVariable(Blueprint, PalCatalogSortDescendingVariableName, BoolPin(), TEXT("false"))) {
        return false;
    }

    UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ESP_PanelCanvas"));
    USizeBox* Size = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ESP_PanelSize"));
    UBorder* Border = Blueprint->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ESP_PanelBorder"));
    UVerticalBox* Content = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_PanelContent"));
    UHorizontalBox* HeaderRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_HeaderRow"));
    UHorizontalBox* TabRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_TabRow"));
    UWidgetSwitcher* Switcher = Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ESP_PageSwitcher"));
    UScrollBox* DisplayScroll = Blueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ESP_DisplayScroll"));
    UVerticalBox* DisplayContent = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_DisplayContent"));
    UVerticalBox* FilterRoot = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_FilterRoot"));
    UHorizontalBox* FilterSubTabRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_FilterSubTabRow"));
    UWidgetSwitcher* FilterSubSwitcher = Blueprint->WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ESP_FilterSubSwitcher"));
    UHorizontalBox* FilterPage = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_FilterPage"));
    USizeBox* PassiveColumnSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ESP_PassiveColumnSize"));
    UVerticalBox* PassiveColumn = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_PassiveColumn"));
    UScrollBox* PassiveScroll = Blueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ESP_PassiveScroll"));
    UVerticalBox* PassiveGroups = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_PassiveGroups"));
    USizeBox* FilterColumnSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ESP_FilterColumnSize"));
    UScrollBox* FilterScroll = Blueprint->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ESP_FilterScroll"));
    UVerticalBox* FilterContent = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_FilterContent"));
    UHorizontalBox* FilterActions = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_FilterActions"));
    UVerticalBox* PalPage = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_PalPage"));
    UVerticalBox* StylePage = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_StylePage"));
    if (!Canvas || !Size || !Border || !Content || !HeaderRow || !TabRow || !Switcher
        || !DisplayScroll || !DisplayContent || !FilterRoot || !FilterSubTabRow || !FilterSubSwitcher
        || !FilterPage || !FilterActions || !PassiveColumnSize || !PassiveColumn
        || !PassiveScroll || !PassiveGroups || !FilterColumnSize || !FilterScroll || !FilterContent
        || !PalPage || !StylePage) {
        return false;
    }
    Size->SetWidthOverride(-1.0f);
    Size->SetHeightOverride(-1.0f);
    Border->SetBrush(FSlateRoundedBoxBrush(PanelV2Style::Surface, 7.0f, PanelV2Style::Border, 1.0f));
    Border->SetBrushColor(FLinearColor::White);
    Border->SetPadding(FMargin(18.0f, 14.0f, 14.0f, 14.0f));
    auto ConfigureScroll = [](UScrollBox* Scroll) {
        Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
        Scroll->SetScrollbarThickness(FVector2D(3.0f, 3.0f));
        Scroll->SetAlwaysShowScrollbar(false);
        Scroll->SetAnimateWheelScrolling(true);
        Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
    };
    ConfigureScroll(DisplayScroll);
    ConfigureScroll(PassiveScroll);
    ConfigureScroll(FilterScroll);
    DisplayScroll->AddChild(DisplayContent);
    PassiveScroll->AddChild(PassiveGroups);
    // __DEPRECATED_20260718__ [reason: passive header and clear controls must precede the scrolling catalog]
    // PassiveColumn->AddChild(PassiveScroll);
    PassiveColumnSize->SetWidthOverride(690.0f);
    PassiveColumnSize->AddChild(PassiveColumn);
    FilterScroll->AddChild(FilterContent);
    FilterColumnSize->SetWidthOverride(430.0f);
    FilterColumnSize->AddChild(FilterScroll);
    FilterPage->AddChild(PassiveColumnSize);
    FilterPage->AddChild(FilterColumnSize);
    SetHorizontalLayout(PassiveColumnSize, FMargin(0.0f, 0.0f, 14.0f, 0.0f), ESlateSizeRule::Automatic);
    SetHorizontalLayout(FilterColumnSize, FMargin(0.0f), ESlateSizeRule::Fill);
    FilterSubSwitcher->bIsVariable = true;
    FilterSubSwitcher->AddChild(FilterPage);
    FilterSubSwitcher->AddChild(PalPage);
    FilterSubSwitcher->SetActiveWidgetIndex(0);
    FilterRoot->AddChild(FilterActions);
    FilterRoot->AddChild(FilterSubTabRow);
    SetVerticalPadding(FilterActions, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    FilterRoot->AddChild(FilterSubSwitcher);
    if (UVerticalBoxSlot* FilterSubSwitcherSlot = Cast<UVerticalBoxSlot>(FilterSubSwitcher->Slot)) {
        FilterSubSwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        FilterSubSwitcherSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    }
    Switcher->bIsVariable = true;
    Switcher->AddChild(DisplayScroll);
    Switcher->AddChild(FilterRoot);
    Switcher->AddChild(StylePage);
    Switcher->SetActiveWidgetIndex(0);
    Content->AddChild(HeaderRow);
    Content->AddChild(TabRow);
    Content->AddChild(Switcher);
    if (UVerticalBoxSlot* SwitcherSlot = Cast<UVerticalBoxSlot>(Switcher->Slot)) {
        SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        SwitcherSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
    }
    Border->AddChild(Content);
    Size->AddChild(Border);
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Size);
    PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    PanelSlot->SetOffsets(FMargin(16.0f));
    Blueprint->WidgetTree->RootWidget = Canvas;

    UTextBlock* Title = AddPanelTextV2(Blueprint, HeaderRow, TEXT("ESP_TitleText"), TEXT("帕鲁资源 ESP"), 22);
    UCheckBox* RuntimeEnabled = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_RuntimeRow"), TEXT("ESP_RuntimeText"), TEXT("ESP_RuntimeToggle"), TEXT("启用模组"), true);
    SetHorizontalLayout(Title, FMargin(0.0f), ESlateSizeRule::Fill);

    UButton* DisplayTab = AddPanelButtonV2(Blueprint, TabRow, TEXT("ESP_DisplayTabButton"), TEXT("ESP_DisplayTabText"), TEXT("显示内容"));
    UButton* FilterTab = AddPanelButtonV2(Blueprint, TabRow, TEXT("ESP_FilterTabButton"), TEXT("ESP_FilterTabText"), TEXT("筛选"));
    UButton* StyleTab = AddPanelButtonV2(Blueprint, TabRow, TEXT("ESP_StyleTabButton"), TEXT("ESP_StyleTabText"), TEXT("显示样式（开发中）"));
    ConfigurePanelSegmentButtonV2(DisplayTab, true);
    ConfigurePanelSegmentButtonV2(FilterTab, false);
    ConfigurePanelSegmentButtonV2(StyleTab, false);
    StyleTab->SetVisibility(ESlateVisibility::Collapsed);
    StylePage->SetVisibility(ESlateVisibility::Collapsed);
    UButton* FilterConditionsTab = AddPanelButtonV2(
        Blueprint, FilterSubTabRow, TEXT("ESP_FilterConditionsTabButton"), TEXT("ESP_FilterConditionsTabText"), TEXT("常规筛选"));
    UButton* FilterPalTab = AddPanelButtonV2(
        Blueprint, FilterSubTabRow, TEXT("ESP_FilterPalTabButton"), TEXT("ESP_FilterPalTabText"), TEXT("帕鲁种类"));
    ConfigurePanelSegmentButtonV2(FilterConditionsTab, true);
    ConfigurePanelSegmentButtonV2(FilterPalTab, false);

    UTextBlock* StyleHeading = AddPanelTextV2(Blueprint, DisplayContent, TEXT("ESP_StyleHeadingText"), TEXT("显示内容"), 15);
    UCheckBox* TopGuide = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_TopGuideRow"), TEXT("ESP_TopGuideText"), TEXT("ESP_TopGuideToggle"), TEXT("顶部引导线"), true);
    UCheckBox* ShowName = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_ShowNameRow"), TEXT("ESP_ShowNameText"), TEXT("ESP_ShowNameToggle"), TEXT("姓名"), true);
    UCheckBox* ShowLevel = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_ShowLevelRow"), TEXT("ESP_ShowLevelText"), TEXT("ESP_ShowLevelToggle"), TEXT("等级"), true);
    UCheckBox* ShowDistance = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_ShowDistanceRow"), TEXT("ESP_ShowDistanceText"), TEXT("ESP_ShowDistanceToggle"), TEXT("距离"), true);
    UCheckBox* ShowIv = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_ShowIvRow"), TEXT("ESP_ShowIvText"), TEXT("ESP_ShowIvToggle"), TEXT("个体值"), false);
    UCheckBox* ShowPassiveSkills = AddPanelToggleV2(
        Blueprint, DisplayContent, TEXT("ESP_ShowPassiveSkillsRow"), TEXT("ESP_ShowPassiveSkillsText"), TEXT("ESP_ShowPassiveSkillsToggle"), TEXT("词条"), false);

    UTextBlock* StylePlaceholder = AddPanelTextV2(Blueprint, StylePage, TEXT("ESP_StylePlaceholderText"), TEXT("显示样式将在后续版本提供"), 16, true);

    UTextBlock* PalHeading = AddPanelTextV2(
        Blueprint, PalPage, TEXT("ESP_PalHeadingText"), TEXT("帕鲁种类筛选"), 16);
    UTextBlock* PalSummary = AddPanelTextV2(
        Blueprint, PalPage, TEXT("ESP_PalSummaryText"),
        TEXT("查询条件必须全部命中；卡片多选用于世界目标，未选择时显示全部种类"), 12, true);
    UTextBlock* PalSortHeading = AddPanelTextV2(
        Blueprint, PalPage, TEXT("ESP_PalSortHeadingText"), TEXT("排序（重新读取当前游戏数据）"), 12, true);
    // __DEPRECATED_20260719__ [reason: ESP_PalSortRow remains the old HorizontalBox object in existing assets]
    // New widget types require a new UObject name during same-process WidgetTree regeneration.
    UWrapBox* PalSortRow = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
        UWrapBox::StaticClass(), TEXT("ESP_PalSortWrap"));
    if (PalSortRow) {
        PalSortRow->SetInnerSlotPadding(FVector2D(5.0f, 4.0f));
        PalPage->AddChild(PalSortRow);
        SetVerticalPadding(PalSortRow, FMargin(0.0f, 3.0f, 0.0f, 5.0f));
    }
    TArray<UButton*> PalSortModeButtons;
    if (PalSortRow) {
        PalSortModeButtons.Add(AddPanelButtonV2(
            Blueprint, PalSortRow, TEXT("ESP_PalSortIdButton"), TEXT("ESP_PalSortIdText"), TEXT("图鉴编号"), true));
        PalSortModeButtons.Add(AddPanelButtonV2(
            Blueprint, PalSortRow, TEXT("ESP_PalSortSprintButton"), TEXT("ESP_PalSortSprintText"), TEXT("冲刺速度")));
        for (const FPalWorkDefinition& Definition : GetPalWorkDefinitions()) {
            const FName ButtonName(*FString::Printf(
                TEXT("ESP_PalSortWork%sButton"), *Definition.EnumName.ToString()));
            const FName TextName(*FString::Printf(
                TEXT("ESP_PalSortWork%sText"), *Definition.EnumName.ToString()));
            PalSortModeButtons.Add(AddPanelButtonV2(
                Blueprint, PalSortRow, ButtonName, TextName, *Definition.Chinese));
        }
    }
    UButton* PalSortAscendingButton = PalSortRow
        ? AddPanelButtonV2(Blueprint, PalSortRow, TEXT("ESP_PalSortAscendingButton"), TEXT("ESP_PalSortAscendingText"), TEXT("升序"))
        : nullptr;
    UButton* PalSortDescendingButton = PalSortRow
        ? AddPanelButtonV2(Blueprint, PalSortRow, TEXT("ESP_PalSortDescendingButton"), TEXT("ESP_PalSortDescendingText"), TEXT("降序"))
        : nullptr;
    if (PalSortAscendingButton) {
        ConfigurePanelSegmentButtonV2(PalSortAscendingButton, true);
    }
    UTextBlock* PalSearchLabel = AddPanelTextV2(
        Blueprint, PalPage, TEXT("ESP_PalSearchLabelText"), TEXT("按名称或图鉴编号搜索"), 12, true);
    UHorizontalBox* PalSearchRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ESP_PalSearchRow"));
    if (PalSearchRow) {
        PalPage->AddChild(PalSearchRow);
        SetVerticalPadding(PalSearchRow, FMargin(0.0f, 4.0f, 0.0f, 5.0f));
    }
    UEditableTextBox* PalSearchBox = PalSearchRow
        ? AddPanelSearchBoxV2(Blueprint, PalSearchRow, TEXT("ESP_PalSearchBox"))
        : nullptr;
    UButton* PalSearchButton = PalSearchRow
        ? AddPanelButtonV2(Blueprint, PalSearchRow, TEXT("ESP_PalSearchButton"), TEXT("ESP_PalSearchText"), TEXT("搜索"))
        : nullptr;
    UButton* PalClearQueryButton = PalSearchRow
        ? AddPanelButtonV2(Blueprint, PalSearchRow, TEXT("ESP_PalClearQueryButton"), TEXT("ESP_PalClearQueryText"), TEXT("清空查询"))
        : nullptr;
    UButton* PalClearSelectionButton = PalSearchRow
        ? AddPanelButtonV2(Blueprint, PalSearchRow, TEXT("ESP_PalClearSelectionButton"), TEXT("ESP_PalClearSelectionText"), TEXT("清空已选帕鲁"))
        : nullptr;
    SetHorizontalLayout(PalSearchButton, FMargin(0.0f, 0.0f, 6.0f, 0.0f), ESlateSizeRule::Automatic);
    SetHorizontalLayout(PalClearQueryButton, FMargin(0.0f, 0.0f, 6.0f, 0.0f), ESlateSizeRule::Automatic);
    SetHorizontalLayout(PalClearSelectionButton, FMargin(0.0f), ESlateSizeRule::Automatic);
    UTextBlock* PalSelectedSummary = AddPanelTextV2(
        Blueprint, PalPage, TEXT("ESP_PalSelectedSummaryText"), TEXT("已选中：无"), 12, true);

    UHorizontalBox* PalQueryRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ESP_PalQueryRow"));
    UVerticalBox* PalElementColumn = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ESP_PalElementColumn"));
    UVerticalBox* PalWorkColumn = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("ESP_PalWorkColumn"));
    UTextBlock* PalElementHeading = PalElementColumn
        ? AddPanelTextV2(Blueprint, PalElementColumn, TEXT("ESP_PalElementHeadingText"), TEXT("属性（全部命中）"), 12, true)
        : nullptr;
    UTextBlock* PalWorkHeading = PalWorkColumn
        ? AddPanelTextV2(Blueprint, PalWorkColumn, TEXT("ESP_PalWorkHeadingText"), TEXT("工作适应性（全部具备）"), 12, true)
        : nullptr;
    UWrapBox* PalElementWrap = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
        UWrapBox::StaticClass(), TEXT("ESP_PalElementWrap"));
    UWrapBox* PalWorkWrap = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
        UWrapBox::StaticClass(), TEXT("ESP_PalWorkWrap"));
    if (PalQueryRow && PalElementColumn && PalWorkColumn && PalElementWrap && PalWorkWrap) {
        PalElementWrap->SetInnerSlotPadding(FVector2D(5.0f, 4.0f));
        PalWorkWrap->SetInnerSlotPadding(FVector2D(5.0f, 4.0f));
        PalElementColumn->AddChild(PalElementWrap);
        PalWorkColumn->AddChild(PalWorkWrap);
        PalQueryRow->AddChild(PalElementColumn);
        PalQueryRow->AddChild(PalWorkColumn);
        SetHorizontalLayout(PalElementColumn, FMargin(0.0f, 0.0f, 16.0f, 0.0f), ESlateSizeRule::Fill);
        SetHorizontalLayout(PalWorkColumn, FMargin(0.0f), ESlateSizeRule::Fill);
        PalPage->AddChild(PalQueryRow);
        SetVerticalPadding(PalQueryRow, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    }
    const TArray<TPair<FName, FString>> PalElementDefinitions = {
        {TEXT("Normal"), TEXT("普通")}, {TEXT("Fire"), TEXT("火")},
        {TEXT("Water"), TEXT("水")}, {TEXT("Leaf"), TEXT("草")},
        {TEXT("Electricity"), TEXT("雷")}, {TEXT("Ice"), TEXT("冰")},
        {TEXT("Earth"), TEXT("地")}, {TEXT("Dark"), TEXT("暗")},
        {TEXT("Dragon"), TEXT("龙")},
    };
    TArray<UCheckBox*> PalElementToggles;
    for (const TPair<FName, FString>& Definition : PalElementDefinitions) {
        const FName ToggleName(*FString::Printf(TEXT("ESP_PalElement%sToggle"), *Definition.Key.ToString()));
        const FName TextName(*FString::Printf(TEXT("ESP_PalElement%sText"), *Definition.Key.ToString()));
        PalElementToggles.Add(AddPanelFilterChipV2(
            Blueprint, PalElementWrap, ToggleName, TextName, *Definition.Value));
    }
    const TArray<FPalWorkDefinition>& PalWorkDefinitions = GetPalWorkDefinitions();
    TArray<UCheckBox*> PalWorkToggles;
    for (const FPalWorkDefinition& Definition : PalWorkDefinitions) {
        const FName ToggleName(*FString::Printf(TEXT("ESP_PalWork%sToggle"), *Definition.EnumName.ToString()));
        const FName TextName(*FString::Printf(TEXT("ESP_PalWork%sText"), *Definition.EnumName.ToString()));
        PalWorkToggles.Add(AddPanelFilterChipV2(
            Blueprint, PalWorkWrap, ToggleName, TextName, *Definition.Chinese));
    }
    UTextBlock* PalResultCount = AddPanelTextV2(
        Blueprint, PalPage, TEXT("ESP_PalResultCountText"), TEXT("结果：0"), 12, true);
    UScrollBox* PalResultsScroll = Blueprint->WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("ESP_PalResultsScroll"));
    UWrapBox* PalResultsWrap = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(
        UWrapBox::StaticClass(), TEXT("ESP_PalResultsWrap"));
    if (PalResultsScroll && PalResultsWrap) {
        ConfigureScroll(PalResultsScroll);
        PalResultsWrap->bIsVariable = true;
        PalResultsWrap->SetInnerSlotPadding(FVector2D(10.0f, 10.0f));
        PalResultsScroll->AddChild(PalResultsWrap);
        PalPage->AddChild(PalResultsScroll);
        if (UVerticalBoxSlot* ResultsSlot = Cast<UVerticalBoxSlot>(PalResultsScroll->Slot)) {
            ResultsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    UTextBlock* PassiveHeading = AddPanelTextV2(Blueprint, PassiveColumn, TEXT("ESP_PassiveHeadingText"), TEXT("被动技能"), 16);
    UTextBlock* PassiveSummary = AddPanelTextV2(
        Blueprint, PassiveColumn, TEXT("ESP_PassiveSummaryText"), TEXT("左键：必须包含（最多 4 个，全部命中）；右键：排除"), 12, true);
    UTextBlock* PassiveSearchLabel = AddPanelTextV2(
        Blueprint, PassiveColumn, TEXT("ESP_PassiveSearchLabelText"), TEXT("搜索被动技能"), 12, true);
    UHorizontalBox* PassiveSearchRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ESP_PassiveSearchRow"));
    if (PassiveSearchRow) {
        PassiveColumn->AddChild(PassiveSearchRow);
        SetVerticalPadding(PassiveSearchRow, FMargin(0.0f, 3.0f, 0.0f, 3.0f));
    }
    UEditableTextBox* PassiveSearchBox = PassiveSearchRow
        ? AddPanelSearchBoxV2(Blueprint, PassiveSearchRow, TEXT("ESP_PassiveSearchBox"))
        : nullptr;
    UButton* PassiveSearchButton = PassiveSearchRow
        ? AddPanelButtonV2(Blueprint, PassiveSearchRow, TEXT("ESP_PassiveSearchButton"), TEXT("ESP_PassiveSearchText"), TEXT("搜索"))
        : nullptr;
    UButton* PassiveClearSearchButton = PassiveSearchRow
        ? AddPanelButtonV2(Blueprint, PassiveSearchRow, TEXT("ESP_PassiveClearSearchButton"), TEXT("ESP_PassiveClearSearchText"), TEXT("清空"))
        : nullptr;
    SetHorizontalLayout(PassiveSearchButton, FMargin(0.0f, 0.0f, 6.0f, 0.0f), ESlateSizeRule::Automatic);
    SetHorizontalLayout(PassiveClearSearchButton, FMargin(0.0f), ESlateSizeRule::Automatic);
    UHorizontalBox* PassiveActions = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_PassiveActions"));
    if (PassiveActions) {
        PassiveColumn->AddChild(PassiveActions);
        SetVerticalPadding(PassiveActions, FMargin(0.0f, 4.0f, 0.0f, 8.0f));
    }
    UButton* ClearAllFilters = FilterActions
        ? AddPanelButtonV2(Blueprint, FilterActions, TEXT("ESP_ClearAllFiltersButton"), TEXT("ESP_ClearAllFiltersText"), TEXT("筛选设置为默认"), true)
        : nullptr;
    UButton* ClearPassiveFilters = PassiveActions
        ? AddPanelButtonV2(Blueprint, PassiveActions, TEXT("ESP_ClearPassiveFiltersButton"), TEXT("ESP_ClearPassiveFiltersText"), TEXT("清空词条"))
        : nullptr;
    PassiveColumn->AddChild(PassiveScroll);
    if (UVerticalBoxSlot* PassiveScrollSlot = Cast<UVerticalBoxSlot>(PassiveScroll->Slot)) {
        PassiveScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    TArray<UExpandableArea*> PassiveAreas;
    auto AddPassiveGroup = [&](const FName& AreaName, const FName& HeaderName, const FName& WrapName,
                               const TCHAR* HeaderText, bool bExpanded) -> UWrapBox* {
        UExpandableArea* Area = Blueprint->WidgetTree->ConstructWidget<UExpandableArea>(UExpandableArea::StaticClass(), AreaName);
        UTextBlock* Header = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), HeaderName);
        UWrapBox* Wrap = Blueprint->WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), WrapName);
        if (!Area || !Header || !Wrap) {
            return nullptr;
        }
        Header->bIsVariable = true;
        Header->SetText(FText::FromString(HeaderText));
        FSlateFontInfo HeaderFont = Header->GetFont();
        HeaderFont.Size = 14;
        Header->SetFont(HeaderFont);
        Header->SetColorAndOpacity(FSlateColor(PanelV2Style::PrimaryText));
        Area->bIsVariable = true;
        Wrap->bIsVariable = true;
        Wrap->SetInnerSlotPadding(FVector2D(6.0f, 6.0f));
        Area->SetContentForSlot(TEXT("Header"), Header);
        Area->SetContentForSlot(TEXT("Body"), Wrap);
        Area->HeaderPadding = FMargin(10.0f, 8.0f);
        Area->AreaPadding = FMargin(8.0f, 4.0f, 8.0f, 8.0f);
        Area->MaxHeight = 1200.0f;
        Area->SetIsExpanded(bExpanded);
        PassiveAreas.Add(Area);
        PassiveGroups->AddChild(Area);
        SetVerticalPadding(Area, FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        return Wrap;
    };
    UWrapBox* PassiveRainbow = AddPassiveGroup(TEXT("ESP_PassiveRainbowArea"), TEXT("ESP_PassiveRainbowHeaderText"), TEXT("ESP_PassiveRainbowWrap"), TEXT("彩虹"), false);
    UWrapBox* PassiveSpecial = AddPassiveGroup(TEXT("ESP_PassiveSpecialArea"), TEXT("ESP_PassiveSpecialHeaderText"), TEXT("ESP_PassiveSpecialWrap"), TEXT("传说"), false);
    UWrapBox* PassiveGold = AddPassiveGroup(TEXT("ESP_PassiveGoldArea"), TEXT("ESP_PassiveGoldHeaderText"), TEXT("ESP_PassiveGoldWrap"), TEXT("金色 III"), false);
    UWrapBox* PassiveGold2 = AddPassiveGroup(TEXT("ESP_PassiveGold2Area"), TEXT("ESP_PassiveGold2HeaderText"), TEXT("ESP_PassiveGold2Wrap"), TEXT("金色 II"), false);
    UWrapBox* PassiveNormal = AddPassiveGroup(TEXT("ESP_PassiveNormalArea"), TEXT("ESP_PassiveNormalHeaderText"), TEXT("ESP_PassiveNormalWrap"), TEXT("普通"), false);
    UWrapBox* PassiveNegative1 = AddPassiveGroup(TEXT("ESP_PassiveNegative1Area"), TEXT("ESP_PassiveNegative1HeaderText"), TEXT("ESP_PassiveNegative1Wrap"), TEXT("负面 I"), false);
    UWrapBox* PassiveNegative2 = AddPassiveGroup(TEXT("ESP_PassiveNegative2Area"), TEXT("ESP_PassiveNegative2HeaderText"), TEXT("ESP_PassiveNegative2Wrap"), TEXT("负面 II"), false);
    UWrapBox* PassiveNegative3 = AddPassiveGroup(TEXT("ESP_PassiveNegative3Area"), TEXT("ESP_PassiveNegative3HeaderText"), TEXT("ESP_PassiveNegative3Wrap"), TEXT("负面 III"), false);

    UTextBlock* FilterHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_FilterHeadingText"), TEXT("常规筛选"), 15);
    const FPanelNumericControlV2 DisplayLimit = AddPanelNumericControlV2(
        Blueprint, DisplayContent,
        TEXT("ESP_DisplayTargetLimitLabelText"), TEXT("ESP_DisplayTargetLimitRow"),
        TEXT("ESP_DisplayTargetLimitSlider"), TEXT("ESP_DisplayTargetLimitInput"), TEXT("ESP_DisplayTargetLimitUnitText"),
        TEXT("目标显示上限"), TEXT(""), 64.0f, 1.0f, 100.0f, 1.0f);
    UTextBlock* LevelHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_LevelHeadingText"), TEXT("等级范围"), 13, true);
    const FPanelNumericControlV2 LevelMin = AddPanelNumericControlV2(
        Blueprint, FilterContent,
        TEXT("ESP_LevelMinLabelText"), TEXT("ESP_LevelMinRow"),
        TEXT("ESP_LevelMinSlider"), TEXT("ESP_LevelMinInput"), TEXT("ESP_LevelMinUnitText"),
        TEXT("最低等级（0 = 不限）"), TEXT(""), 0.0f, 0.0f, 100.0f, 1.0f);
    const FPanelNumericControlV2 LevelMax = AddPanelNumericControlV2(
        Blueprint, FilterContent,
        TEXT("ESP_LevelMaxLabelText"), TEXT("ESP_LevelMaxRow"),
        TEXT("ESP_LevelMaxSlider"), TEXT("ESP_LevelMaxInput"), TEXT("ESP_LevelMaxUnitText"),
        TEXT("最高等级（0 = 不限）"), TEXT(""), 0.0f, 0.0f, 100.0f, 1.0f);
    const FPanelNumericControlV2 DistanceMax = AddPanelNumericControlV2(
        Blueprint, FilterContent,
        TEXT("ESP_DistanceMaxLabelText"), TEXT("ESP_DistanceMaxRow"),
        TEXT("ESP_DistanceMaxSlider"), TEXT("ESP_DistanceMaxInput"), TEXT("ESP_DistanceMaxUnitText"),
        TEXT("最大距离"), TEXT("米"), 330.0f, 0.0f, 330.0f, 10.0f);
    UTextBlock* IvHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_IvHeadingText"), TEXT("个体值下限（0 = 不限）"), 13, true);
    const FPanelNumericControlV2 IvHpMin = AddPanelNumericControlV2(
        Blueprint, FilterContent,
        TEXT("ESP_IvHpMinLabelText"), TEXT("ESP_IvHpMinRow"),
        TEXT("ESP_IvHpMinSlider"), TEXT("ESP_IvHpMinInput"), TEXT("ESP_IvHpMinUnitText"),
        TEXT("生命"), TEXT(""), 0.0f, 0.0f, 100.0f, 1.0f);
    const FPanelNumericControlV2 IvAttackMin = AddPanelNumericControlV2(
        Blueprint, FilterContent,
        TEXT("ESP_IvAttackMinLabelText"), TEXT("ESP_IvAttackMinRow"),
        TEXT("ESP_IvAttackMinSlider"), TEXT("ESP_IvAttackMinInput"), TEXT("ESP_IvAttackMinUnitText"),
        TEXT("攻击"), TEXT(""), 0.0f, 0.0f, 100.0f, 1.0f);
    const FPanelNumericControlV2 IvDefenseMin = AddPanelNumericControlV2(
        Blueprint, FilterContent,
        TEXT("ESP_IvDefenseMinLabelText"), TEXT("ESP_IvDefenseMinRow"),
        TEXT("ESP_IvDefenseMinSlider"), TEXT("ESP_IvDefenseMinInput"), TEXT("ESP_IvDefenseMinUnitText"),
        TEXT("防御"), TEXT(""), 0.0f, 0.0f, 100.0f, 1.0f);

    UTextBlock* ElementHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_ElementHeadingText"), TEXT("属性（任一匹配）"), 13, true);
    UTextBlock* ElementStatus = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_ElementStatusText"), TEXT("未选择：全部"), 12, true);
    UHorizontalBox* ElementRowA = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ElementRowA"));
    UHorizontalBox* ElementRowB = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ElementRowB"));
    UHorizontalBox* ElementRowC = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ElementRowC"));
    if (ElementRowA && ElementRowB && ElementRowC) {
        FilterContent->AddChild(ElementRowA);
        FilterContent->AddChild(ElementRowB);
        FilterContent->AddChild(ElementRowC);
        SetVerticalPadding(ElementRowA, FMargin(0.0f, 2.0f, 0.0f, 3.0f));
        SetVerticalPadding(ElementRowB, FMargin(0.0f, 0.0f, 0.0f, 3.0f));
        SetVerticalPadding(ElementRowC, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    }
    UCheckBox* ElementNormal = AddPanelFilterChipV2(Blueprint, ElementRowA, TEXT("ESP_ElementNormalToggle"), TEXT("ESP_ElementNormalText"), TEXT("普通"));
    UCheckBox* ElementFire = AddPanelFilterChipV2(Blueprint, ElementRowA, TEXT("ESP_ElementFireToggle"), TEXT("ESP_ElementFireText"), TEXT("火"));
    UCheckBox* ElementWater = AddPanelFilterChipV2(Blueprint, ElementRowA, TEXT("ESP_ElementWaterToggle"), TEXT("ESP_ElementWaterText"), TEXT("水"));
    UCheckBox* ElementLeaf = AddPanelFilterChipV2(Blueprint, ElementRowB, TEXT("ESP_ElementLeafToggle"), TEXT("ESP_ElementLeafText"), TEXT("草"));
    UCheckBox* ElementElectricity = AddPanelFilterChipV2(Blueprint, ElementRowB, TEXT("ESP_ElementElectricityToggle"), TEXT("ESP_ElementElectricityText"), TEXT("雷"));
    UCheckBox* ElementIce = AddPanelFilterChipV2(Blueprint, ElementRowB, TEXT("ESP_ElementIceToggle"), TEXT("ESP_ElementIceText"), TEXT("冰"));
    UCheckBox* ElementEarth = AddPanelFilterChipV2(Blueprint, ElementRowC, TEXT("ESP_ElementEarthToggle"), TEXT("ESP_ElementEarthText"), TEXT("地"));
    UCheckBox* ElementDark = AddPanelFilterChipV2(Blueprint, ElementRowC, TEXT("ESP_ElementDarkToggle"), TEXT("ESP_ElementDarkText"), TEXT("暗"));
    UCheckBox* ElementDragon = AddPanelFilterChipV2(Blueprint, ElementRowC, TEXT("ESP_ElementDragonToggle"), TEXT("ESP_ElementDragonText"), TEXT("龙"));
    const TArray<UCheckBox*> ElementToggles = {
        ElementNormal, ElementFire, ElementWater, ElementLeaf, ElementElectricity,
        ElementIce, ElementEarth, ElementDark, ElementDragon,
    };

    UTextBlock* GenderHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_GenderHeadingText"), TEXT("性别"), 13, true);
    UTextBlock* GenderStatus = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_GenderStatusText"), TEXT("当前：全部"), 12, true);
    UHorizontalBox* GenderRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_GenderRow"));
    if (GenderRow) {
        FilterContent->AddChild(GenderRow);
        SetVerticalPadding(GenderRow, FMargin(0.0f, 2.0f, 0.0f, 6.0f));
    }
    UButton* GenderAll = GenderRow
        ? AddPanelButtonV2(Blueprint, GenderRow, TEXT("ESP_GenderAllButton"), TEXT("ESP_GenderAllText"), TEXT("全部"))
        : nullptr;
    UButton* GenderMale = GenderRow
        ? AddPanelButtonV2(Blueprint, GenderRow, TEXT("ESP_GenderMaleButton"), TEXT("ESP_GenderMaleText"), TEXT("雄性"))
        : nullptr;
    UButton* GenderFemale = GenderRow
        ? AddPanelButtonV2(Blueprint, GenderRow, TEXT("ESP_GenderFemaleButton"), TEXT("ESP_GenderFemaleText"), TEXT("雌性"))
        : nullptr;
    ConfigurePanelSegmentButtonV2(GenderAll, true);
    ConfigurePanelSegmentButtonV2(GenderMale, false);
    ConfigurePanelSegmentButtonV2(GenderFemale, false);

    UTextBlock* LuckyHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_LuckyHeadingText"), TEXT("闪光个体"), 13, true);
    UTextBlock* LuckyStatus = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_LuckyStatusText"), TEXT("当前：全部"), 12, true);
    UHorizontalBox* LuckyRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_LuckyRow"));
    if (LuckyRow) {
        FilterContent->AddChild(LuckyRow);
        SetVerticalPadding(LuckyRow, FMargin(0.0f, 2.0f, 0.0f, 6.0f));
    }
    UButton* LuckyAll = LuckyRow
        ? AddPanelButtonV2(Blueprint, LuckyRow, TEXT("ESP_LuckyAllButton"), TEXT("ESP_LuckyAllText"), TEXT("全部"))
        : nullptr;
    UButton* LuckyOnly = LuckyRow
        ? AddPanelButtonV2(Blueprint, LuckyRow, TEXT("ESP_LuckyOnlyButton"), TEXT("ESP_LuckyOnlyText"), TEXT("仅闪光"))
        : nullptr;
    UButton* LuckyExclude = LuckyRow
        ? AddPanelButtonV2(Blueprint, LuckyRow, TEXT("ESP_LuckyExcludeButton"), TEXT("ESP_LuckyExcludeText"), TEXT("排除闪光"))
        : nullptr;
    ConfigurePanelSegmentButtonV2(LuckyAll, true);
    ConfigurePanelSegmentButtonV2(LuckyOnly, false);
    ConfigurePanelSegmentButtonV2(LuckyExclude, false);

    UTextBlock* BossHeading = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_BossHeadingText"), TEXT("首领个体"), 13, true);
    UTextBlock* BossStatus = AddPanelTextV2(Blueprint, FilterContent, TEXT("ESP_BossStatusText"), TEXT("当前：全部"), 12, true);
    UHorizontalBox* BossRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_BossRow"));
    if (BossRow) {
        FilterContent->AddChild(BossRow);
        SetVerticalPadding(BossRow, FMargin(0.0f, 2.0f, 0.0f, 6.0f));
    }
    UButton* BossAll = BossRow
        ? AddPanelButtonV2(Blueprint, BossRow, TEXT("ESP_BossAllButton"), TEXT("ESP_BossAllText"), TEXT("全部"))
        : nullptr;
    UButton* BossOnly = BossRow
        ? AddPanelButtonV2(Blueprint, BossRow, TEXT("ESP_BossOnlyButton"), TEXT("ESP_BossOnlyText"), TEXT("仅首领"))
        : nullptr;
    UButton* BossExclude = BossRow
        ? AddPanelButtonV2(Blueprint, BossRow, TEXT("ESP_BossExcludeButton"), TEXT("ESP_BossExcludeText"), TEXT("排除首领"))
        : nullptr;
    ConfigurePanelSegmentButtonV2(BossAll, true);
    ConfigurePanelSegmentButtonV2(BossOnly, false);
    ConfigurePanelSegmentButtonV2(BossExclude, false);

    UTextBlock* CollectionHeading = AddPanelTextV2(
        Blueprint, FilterContent, TEXT("ESP_CollectionHeadingText"), TEXT("图鉴收集（捕捉 5 只完成）"), 13, true);
    UTextBlock* CollectionStatus = AddPanelTextV2(
        Blueprint, FilterContent, TEXT("ESP_CollectionStatusText"), TEXT("当前：全部"), 12, true);
    UHorizontalBox* CollectionRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("ESP_CollectionRow"));
    if (CollectionRow) {
        FilterContent->AddChild(CollectionRow);
        SetVerticalPadding(CollectionRow, FMargin(0.0f, 2.0f, 0.0f, 6.0f));
    }
    UButton* CollectionAll = CollectionRow
        ? AddPanelButtonV2(Blueprint, CollectionRow, TEXT("ESP_CollectionAllButton"), TEXT("ESP_CollectionAllText"), TEXT("全部"))
        : nullptr;
    UButton* CollectionIncomplete = CollectionRow
        ? AddPanelButtonV2(Blueprint, CollectionRow, TEXT("ESP_CollectionIncompleteButton"), TEXT("ESP_CollectionIncompleteText"), TEXT("未完成"))
        : nullptr;
    UButton* CollectionCompleteButton = CollectionRow
        ? AddPanelButtonV2(Blueprint, CollectionRow, TEXT("ESP_CollectionCompleteButton"), TEXT("ESP_CollectionCompleteText"), TEXT("已完成"))
        : nullptr;
    ConfigurePanelSegmentButtonV2(CollectionAll, true);
    ConfigurePanelSegmentButtonV2(CollectionIncomplete, false);
    ConfigurePanelSegmentButtonV2(CollectionCompleteButton, false);
    if (GenderStatus) GenderStatus->SetVisibility(ESlateVisibility::Collapsed);
    if (LuckyStatus) LuckyStatus->SetVisibility(ESlateVisibility::Collapsed);
    if (BossStatus) BossStatus->SetVisibility(ESlateVisibility::Collapsed);
    if (CollectionStatus) CollectionStatus->SetVisibility(ESlateVisibility::Collapsed);

    UTextBlock* LanguageHeading = AddPanelTextV2(Blueprint, DisplayContent, TEXT("ESP_LanguageHeadingText"), TEXT("语言"), 13, true);
    UHorizontalBox* LanguageRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_LanguageRow"));
    if (LanguageRow) {
        DisplayContent->AddChild(LanguageRow);
        SetVerticalPadding(LanguageRow, FMargin(0.0f, 2.0f, 0.0f, 6.0f));
    }
    UButton* Chinese = LanguageRow
        ? AddPanelButtonV2(Blueprint, LanguageRow, TEXT("ESP_ChineseButton"), TEXT("ESP_ChineseText"), TEXT("中文"))
        : nullptr;
    UButton* English = LanguageRow
        ? AddPanelButtonV2(Blueprint, LanguageRow, TEXT("ESP_EnglishButton"), TEXT("ESP_EnglishText"), TEXT("英文"))
        : nullptr;
    ConfigurePanelSegmentButtonV2(Chinese, true);
    ConfigurePanelSegmentButtonV2(English, false);

    UButton* AdvancedExpand = AddPanelButtonV2(
        Blueprint, DisplayContent, TEXT("ESP_AdvancedExpandButton"), TEXT("ESP_AdvancedExpandText"), TEXT("高级诊断"));
    UVerticalBox* Advanced = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ESP_AdvancedBox"));
    if (Advanced) {
        Advanced->bIsVariable = true;
        Advanced->SetVisibility(ESlateVisibility::Collapsed);
        DisplayContent->AddChild(Advanced);
        SetVerticalPadding(Advanced, FMargin(0.0f, 4.0f, 0.0f, 10.0f));
    }
    UTextBlock* ModeHeading = Advanced
        ? AddPanelTextV2(Blueprint, Advanced, TEXT("ESP_ModeHeadingText"), TEXT("实验模式"), 15)
        : nullptr;
    UTextBlock* ModeStatus = Advanced
        ? AddPanelTextV2(Blueprint, Advanced, TEXT("ESP_ModeStatusText"), TEXT("当前模式：安全快照"), 13, true)
        : nullptr;
    UHorizontalBox* ModeRowA = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ModeRowA"));
    UHorizontalBox* ModeRowB = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_ModeRowB"));
    if (Advanced && ModeRowA && ModeRowB) {
        Advanced->AddChild(ModeRowA);
        Advanced->AddChild(ModeRowB);
        SetVerticalPadding(ModeRowA, FMargin(0.0f, 4.0f, 0.0f, 3.0f));
        SetVerticalPadding(ModeRowB, FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    }
    UButton* ModeOff = ModeRowA
        ? AddPanelButtonV2(Blueprint, ModeRowA, TEXT("ESP_ModeOffButton"), TEXT("ESP_ModeOffText"), TEXT("关闭"))
        : nullptr;
    UButton* ModeSnapshot = ModeRowA
        ? AddPanelButtonV2(Blueprint, ModeRowA, TEXT("ESP_ModeSnapshotButton"), TEXT("ESP_ModeSnapshotText"), TEXT("单次快照"))
        : nullptr;
    UButton* ModeChunked = ModeRowB
        ? AddPanelButtonV2(Blueprint, ModeRowB, TEXT("ESP_ModeChunkedButton"), TEXT("ESP_ModeChunkedText"), TEXT("安全快照"))
        : nullptr;
    UButton* ModeEvent = ModeRowB
        ? AddPanelButtonV2(Blueprint, ModeRowB, TEXT("ESP_ModeEventButton"), TEXT("ESP_ModeEventText"), TEXT("事件优先"))
        : nullptr;
    UTextBlock* CaptureStatus = Advanced
        ? AddPanelTextV2(Blueprint, Advanced, TEXT("ESP_CaptureStatusText"), TEXT("性能采集：已停止"), 13, true)
        : nullptr;
    UHorizontalBox* CaptureRow = Blueprint->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ESP_CaptureRow"));
    if (Advanced && CaptureRow) {
        Advanced->AddChild(CaptureRow);
        SetVerticalPadding(CaptureRow, FMargin(0.0f, 4.0f, 0.0f, 6.0f));
    }
    UButton* CaptureStart = CaptureRow
        ? AddPanelButtonV2(Blueprint, CaptureRow, TEXT("ESP_CaptureStartButton"), TEXT("ESP_CaptureStartText"), TEXT("开始采集"), true)
        : nullptr;
    UButton* CaptureStop = CaptureRow
        ? AddPanelButtonV2(Blueprint, CaptureRow, TEXT("ESP_CaptureStopButton"), TEXT("ESP_CaptureStopText"), TEXT("停止采集"))
        : nullptr;
    UButton* AdvancedCollapse = Advanced
        ? AddPanelButtonV2(Blueprint, Advanced, TEXT("ESP_AdvancedCollapseButton"), TEXT("ESP_AdvancedCollapseText"), TEXT("收起"))
        : nullptr;

    if (!Title || !RuntimeEnabled || !DisplayTab || !FilterTab || !StyleTab
        || !FilterConditionsTab || !FilterPalTab || !StylePlaceholder
        || !StyleHeading || !TopGuide || !ShowName || !ShowLevel || !ShowDistance || !ShowIv
        || !ShowPassiveSkills
        || !PalHeading || !PalSummary || !PalSortHeading || !PalSortRow
        || PalSortModeButtons.Num() != PalWorkDefinitions.Num() + 2 || PalSortModeButtons.Contains(nullptr)
        || !PalSortAscendingButton || !PalSortDescendingButton
        || !PalSearchLabel || !PalSearchRow || !PalSearchBox || !PalSearchButton
        || !PalClearQueryButton || !PalClearSelectionButton || !PalQueryRow
        || !PalElementColumn || !PalWorkColumn || !PalElementHeading || !PalWorkHeading
        || !PalElementWrap || !PalWorkWrap || PalElementToggles.Num() != 9 || PalElementToggles.Contains(nullptr)
        || PalWorkToggles.Num() != PalWorkDefinitions.Num() || PalWorkToggles.Contains(nullptr)
        || !PalSelectedSummary || !PalResultCount || !PalResultsScroll || !PalResultsWrap
        || !PassiveHeading || !PassiveSummary || !PassiveSearchLabel || !PassiveSearchRow
        || !PassiveSearchBox || !PassiveSearchButton || !PassiveClearSearchButton
        || !FilterActions || !PassiveActions || !ClearAllFilters || !ClearPassiveFilters
        || !PassiveRainbow || !PassiveSpecial || !PassiveGold || !PassiveGold2 || !PassiveNormal
        || !PassiveNegative1 || !PassiveNegative2 || !PassiveNegative3
        || PassiveAreas.Num() != 8 || PassiveAreas.Contains(nullptr)
        || !FilterHeading || !DisplayLimit.Slider || !DisplayLimit.SpinBox || !LevelHeading
        || !LevelMin.Slider || !LevelMin.SpinBox || !LevelMax.Slider || !LevelMax.SpinBox
        || !DistanceMax.Slider || !DistanceMax.SpinBox || !IvHeading
        || !IvHpMin.Slider || !IvHpMin.SpinBox || !IvAttackMin.Slider || !IvAttackMin.SpinBox
        || !IvDefenseMin.Slider || !IvDefenseMin.SpinBox
        || !ElementHeading || !ElementStatus
        || !ElementRowA || !ElementRowB || !ElementRowC || ElementToggles.Contains(nullptr)
        || !GenderHeading || !GenderStatus || !GenderRow
        || !GenderAll || !GenderMale || !GenderFemale || !LuckyHeading || !LuckyStatus || !LuckyRow
        || !LuckyAll || !LuckyOnly || !LuckyExclude || !BossHeading || !BossStatus || !BossRow
        || !BossAll || !BossOnly || !BossExclude
        || !CollectionHeading || !CollectionStatus || !CollectionRow
        || !CollectionAll || !CollectionIncomplete || !CollectionCompleteButton
        || !LanguageHeading || !LanguageRow || !Chinese || !English
        || !AdvancedExpand || !Advanced || !ModeHeading || !ModeStatus || !ModeRowA || !ModeRowB
        || !ModeOff || !ModeSnapshot || !ModeChunked || !ModeEvent || !CaptureStatus || !CaptureRow
        || !CaptureStart || !CaptureStop || !AdvancedCollapse) {
        return false;
    }
    SetVerticalPadding(Title, FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    SetVerticalPadding(StyleHeading, FMargin(0.0f, 12.0f, 0.0f, 4.0f));
    SetVerticalPadding(FilterHeading, FMargin(0.0f, 12.0f, 0.0f, 2.0f));
    SetVerticalPadding(LevelHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(IvHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(ElementHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(GenderHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(LuckyHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(BossHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(CollectionHeading, FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    SetVerticalPadding(LanguageHeading, FMargin(0.0f, 8.0f, 0.0f, 0.0f));

    // __DEPRECATED_20260717__ [reason: signature-only bridge keeps a V1 ModActor compilable
    // during the same generation pass; it has no execution body or distance control]
    UK2Node_CustomEvent* LegacyInitializeCompatibility = AddCustomEvent(
        Blueprint,
        Graph,
        *PanelInitializeControlsEventName.ToString(),
        -1900,
        -2200,
        {
            TPair<FName, FEdGraphPinType>(FName("DisplayTargetLimit"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("DistanceMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("DistanceMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowLevel"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowDistance"), BoolPin())
        }
    );
    UK2Node_CustomEvent* PanelInitializeV2 = AddCustomEvent(
        Blueprint,
        Graph,
        *PanelInitializeControlsV2EventName.ToString(),
        -1900,
        -1800,
        {
            TPair<FName, FEdGraphPinType>(FName("DisplayTargetLimit"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("DistanceMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("IvHpMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("IvAttackMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("IvDefenseMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("RuntimeEnabled"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowTopGuideLine"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowName"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowLevel"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowDistance"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowIV"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowPassiveSkills"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementNormal"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementFire"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementWater"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementLeaf"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementElectricity"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementIce"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementEarth"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementDark"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ElementDragon"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("GenderFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LuckyFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("BossFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("CollectionFilterId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LanguageId"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandRainbow"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandLegend"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandGold3"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandGold2"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNormal"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNegative1"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNegative2"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ExpandNegative3"), BoolPin())
        }
    );
    UK2Node_CustomEvent* PopulatePassiveCatalog = AddCustomEvent(
        Blueprint,
        Graph,
        *PanelPopulatePassiveCatalogEventName.ToString(),
        -1900,
        -1400,
        {}
    );
    UK2Node_CustomEvent* PopulatePalCatalog = AddCustomEvent(
        Blueprint, Graph, *PanelPopulatePalCatalogEventName.ToString(), -1900, -1040, {});
    UK2Node_CustomEvent* RenderPalCatalog = AddCustomEvent(
        Blueprint, Graph, *PanelRenderPalCatalogEventName.ToString(), -1900, -680, {});
    if (!LegacyInitializeCompatibility || !PanelInitializeV2 || !PopulatePassiveCatalog
        || !PopulatePalCatalog || !RenderPalCatalog) {
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) {
        return false;
    }

    // __DEPRECATED_20260717__ [reason: moved before the first compile so dependent assets never see a missing function]
#if 0
    UK2Node_CustomEvent* LegacyInitializeCompatibilityAfterCompile = AddCustomEvent(
        Blueprint,
        Graph,
        *PanelInitializeControlsEventName.ToString(),
        -1900,
        -2200,
        {
            TPair<FName, FEdGraphPinType>(FName("DisplayTargetLimit"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("LevelMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("DistanceMin"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("DistanceMax"), IntPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowLevel"), BoolPin()),
            TPair<FName, FEdGraphPinType>(FName("ShowDistance"), BoolPin())
        }
    );
    if (!LegacyInitializeCompatibilityAfterCompile) {
        return false;
    }
#endif

    int32 Y = -1600;
    if (!BuildPanelBooleanEvent(Blueprint, RuntimeEnabled, ModActorClass, RuntimeEnabledVariableName, Y)
        || !BuildPanelBooleanEvent(Blueprint, TopGuide, ModActorClass, ShowTopGuideLineVariableName, Y + 360)
        || !BuildPanelBooleanEvent(Blueprint, ShowName, ModActorClass, ShowNameVariableName, Y + 720)
        || !BuildPanelBooleanEvent(Blueprint, ShowLevel, ModActorClass, ShowLevelVariableName, Y + 1080)
        || !BuildPanelBooleanEvent(Blueprint, ShowDistance, ModActorClass, ShowDistanceVariableName, Y + 1440)
        || !BuildPanelBooleanEvent(Blueprint, ShowIv, ModActorClass, ShowIvVariableName, Y + 1800)
        || !BuildPanelBooleanEvent(Blueprint, ShowPassiveSkills, ModActorClass, ShowPassiveSkillsVariableName, Y + 2160)
        || !BuildPanelNumericEventV2(Blueprint, DisplayLimit, ModActorClass, DisplayTargetLimitVariableName, Y + 2520)
        || !BuildPanelNumericEventV2(Blueprint, LevelMin, ModActorClass, LevelMinVariableName, Y + 3120)
        || !BuildPanelNumericEventV2(Blueprint, LevelMax, ModActorClass, LevelMaxVariableName, Y + 4080)
        || !BuildPanelNumericEventV2(Blueprint, DistanceMax, ModActorClass, DistanceMaxVariableName, Y + 5040)
        || !BuildPanelNumericEventV2(Blueprint, IvHpMin, ModActorClass, IvHpMinVariableName, Y + 6000)
        || !BuildPanelNumericEventV2(Blueprint, IvAttackMin, ModActorClass, IvAttackMinVariableName, Y + 6960)
        || !BuildPanelNumericEventV2(Blueprint, IvDefenseMin, ModActorClass, IvDefenseMinVariableName, Y + 7920)
        || !BuildPanelBooleanEvent(Blueprint, ElementNormal, ModActorClass, ElementNormalVariableName, Y + 8880)
        || !BuildPanelBooleanEvent(Blueprint, ElementFire, ModActorClass, ElementFireVariableName, Y + 9240)
        || !BuildPanelBooleanEvent(Blueprint, ElementWater, ModActorClass, ElementWaterVariableName, Y + 9600)
        || !BuildPanelBooleanEvent(Blueprint, ElementLeaf, ModActorClass, ElementLeafVariableName, Y + 9960)
        || !BuildPanelBooleanEvent(Blueprint, ElementElectricity, ModActorClass, ElementElectricityVariableName, Y + 10320)
        || !BuildPanelBooleanEvent(Blueprint, ElementIce, ModActorClass, ElementIceVariableName, Y + 10680)
        || !BuildPanelBooleanEvent(Blueprint, ElementEarth, ModActorClass, ElementEarthVariableName, Y + 11040)
        || !BuildPanelBooleanEvent(Blueprint, ElementDark, ModActorClass, ElementDarkVariableName, Y + 11400)
        || !BuildPanelBooleanEvent(Blueprint, ElementDragon, ModActorClass, ElementDragonVariableName, Y + 11760)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[0], ModActorClass, PassiveRainbowExpandedVariableName, Y + 12120)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[1], ModActorClass, PassiveLegendExpandedVariableName, Y + 12480)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[2], ModActorClass, PassiveGold3ExpandedVariableName, Y + 12840)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[3], ModActorClass, PassiveGold2ExpandedVariableName, Y + 13200)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[4], ModActorClass, PassiveNormalExpandedVariableName, Y + 13560)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[5], ModActorClass, PassiveNegative1ExpandedVariableName, Y + 13920)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[6], ModActorClass, PassiveNegative2ExpandedVariableName, Y + 14280)
        || !BuildPanelExpansionEvent(Blueprint, PassiveAreas[7], ModActorClass, PassiveNegative3ExpandedVariableName, Y + 14640)) {
        return false;
    }
    Y += 13200;
    auto Control = [&](UButton* Button, const TArray<FPanelControlAssignment>& Assignments, const FName& StatusName, const TCHAR* Status) {
        const bool bOk = BuildPanelControlEvent(Blueprint, Button, ModActorClass, Assignments, StatusName, Status, Y);
        Y += 360;
        return bOk;
    };
    if (!Control(ModeOff, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("0")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式：关闭"))
        || !Control(ModeSnapshot, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("1")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式：单次快照"))
        || !Control(ModeChunked, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("2")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式：安全快照"))
        || !Control(ModeEvent, {{RuntimeEnabledVariableName, TEXT("true")}, {ProfileIdVariableName, TEXT("3")}}, TEXT("ESP_ModeStatusText"), TEXT("当前模式：事件优先"))
        || !Control(CaptureStart, {{CaptureRequestedVariableName, TEXT("true")}}, TEXT("ESP_CaptureStatusText"), TEXT("性能采集：采集中"))
        || !Control(CaptureStop, {{CaptureRequestedVariableName, TEXT("false")}}, TEXT("ESP_CaptureStatusText"), TEXT("性能采集：已停止"))) {
        return false;
    }

    const TArray<UButton*> GenderButtons = {GenderAll, GenderMale, GenderFemale};
    if (!BuildPanelControlEvent(
            Blueprint, GenderAll, ModActorClass, {{GenderFilterIdVariableName, TEXT("0")}},
            TEXT("ESP_GenderStatusText"), TEXT("当前：全部"), Y, &GenderButtons, 0)
        || !BuildPanelControlEvent(
            Blueprint, GenderMale, ModActorClass, {{GenderFilterIdVariableName, TEXT("1")}},
            TEXT("ESP_GenderStatusText"), TEXT("当前：雄性"), Y + 360, &GenderButtons, 1)
        || !BuildPanelControlEvent(
            Blueprint, GenderFemale, ModActorClass, {{GenderFilterIdVariableName, TEXT("2")}},
            TEXT("ESP_GenderStatusText"), TEXT("当前：雌性"), Y + 720, &GenderButtons, 2)) {
        return false;
    }
    Y += 1080;

    const TArray<UButton*> LuckyButtons = {LuckyAll, LuckyOnly, LuckyExclude};
    if (!BuildPanelControlEvent(
            Blueprint, LuckyAll, ModActorClass, {{LuckyFilterIdVariableName, TEXT("0")}},
            TEXT("ESP_LuckyStatusText"), TEXT("当前：全部"), Y, &LuckyButtons, 0)
        || !BuildPanelControlEvent(
            Blueprint, LuckyOnly, ModActorClass, {{LuckyFilterIdVariableName, TEXT("1")}},
            TEXT("ESP_LuckyStatusText"), TEXT("当前：仅闪光"), Y + 360, &LuckyButtons, 1)
        || !BuildPanelControlEvent(
            Blueprint, LuckyExclude, ModActorClass, {{LuckyFilterIdVariableName, TEXT("2")}},
            TEXT("ESP_LuckyStatusText"), TEXT("当前：排除闪光"), Y + 720, &LuckyButtons, 2)) {
        return false;
    }
    Y += 1080;

    const TArray<UButton*> BossButtons = {BossAll, BossOnly, BossExclude};
    if (!BuildPanelControlEvent(
            Blueprint, BossAll, ModActorClass, {{BossFilterIdVariableName, TEXT("0")}},
            TEXT("ESP_BossStatusText"), TEXT("当前：全部"), Y, &BossButtons, 0)
        || !BuildPanelControlEvent(
            Blueprint, BossOnly, ModActorClass, {{BossFilterIdVariableName, TEXT("1")}},
            TEXT("ESP_BossStatusText"), TEXT("当前：仅首领"), Y + 360, &BossButtons, 1)
        || !BuildPanelControlEvent(
            Blueprint, BossExclude, ModActorClass, {{BossFilterIdVariableName, TEXT("2")}},
            TEXT("ESP_BossStatusText"), TEXT("当前：排除首领"), Y + 720, &BossButtons, 2)) {
        return false;
    }
    Y += 1080;

    const TArray<UButton*> CollectionButtons = {CollectionAll, CollectionIncomplete, CollectionCompleteButton};
    if (!BuildPanelControlEvent(
            Blueprint, CollectionAll, ModActorClass, {{CollectionFilterIdVariableName, TEXT("0")}},
            TEXT("ESP_CollectionStatusText"), TEXT("当前：全部"), Y, &CollectionButtons, 0)
        || !BuildPanelControlEvent(
            Blueprint, CollectionIncomplete, ModActorClass, {{CollectionFilterIdVariableName, TEXT("1")}},
            TEXT("ESP_CollectionStatusText"), TEXT("当前：未完成"), Y + 360, &CollectionButtons, 1)
        || !BuildPanelControlEvent(
            Blueprint, CollectionCompleteButton, ModActorClass, {{CollectionFilterIdVariableName, TEXT("2")}},
            TEXT("ESP_CollectionStatusText"), TEXT("当前：已完成"), Y + 720, &CollectionButtons, 2)) {
        return false;
    }
    Y += 1080;

    if (!BuildPanelInitializeControlsV2(
            Blueprint, DisplayLimit, LevelMin, LevelMax, DistanceMax, IvHpMin, IvAttackMin, IvDefenseMin,
            RuntimeEnabled, TopGuide, ShowName, ShowLevel, ShowDistance, ShowIv, ShowPassiveSkills,
            ElementToggles, GenderStatus, GenderButtons, LuckyStatus, LuckyButtons,
            BossStatus, BossButtons, CollectionStatus, CollectionButtons, PassiveAreas, Y, PanelInitializeV2)) {
        return false;
    }
    Y += 900;
    const TArray<UButton*> TabButtons = {DisplayTab, FilterTab};
    const TArray<UButton*> FilterSubTabButtons = {FilterConditionsTab, FilterPalTab};
    const TArray<UWrapBox*> PassiveCatalogGroups = {
        PassiveRainbow, PassiveSpecial, PassiveGold, PassiveGold2, PassiveNormal,
        PassiveNegative1, PassiveNegative2, PassiveNegative3,
    };
    const TArray<FPanelNumericControlV2> FilterNumericControls = {
        LevelMin, LevelMax, DistanceMax, IvHpMin, IvAttackMin, IvDefenseMin,
    };
    if (!BuildPanelTabEvent(Blueprint, DisplayTab, Switcher, TabButtons, 0, Y, ModActorClass, PanelMainPageVariableName)
        || !BuildPanelTabEvent(Blueprint, FilterTab, Switcher, TabButtons, 1, Y + 360, ModActorClass, PanelMainPageVariableName)
        || !BuildPanelTabEvent(Blueprint, FilterConditionsTab, FilterSubSwitcher, FilterSubTabButtons, 0, Y + 1080, ModActorClass, PanelFilterPageVariableName)
        || !BuildPanelTabEvent(Blueprint, FilterPalTab, FilterSubSwitcher, FilterSubTabButtons, 1, Y + 1440, ModActorClass, PanelFilterPageVariableName)
        || !BuildPanelSetPageStateEvent(Blueprint, Switcher, FilterSubSwitcher, TabButtons, FilterSubTabButtons, Y + 1800)
        || !BuildPanelPassiveCatalog(
            Blueprint, ModActorClass, PassiveEntryClass, PalUtilityClass, PalUIUtilityClass,
            PassiveSkillManagerClass, PassiveSkillDatabaseRowStruct, MasterDataTablesUtilityClass,
            PassiveCatalogGroups, PassiveSearchBox, PopulatePassiveCatalog, Y + 1440)
        || !BuildPanelPalCatalog(
            Blueprint, ModActorClass, PalEntryClass, PalUtilityClass, PalUIUtilityClass,
            DatabaseCharacterParameterClass, CharacterParameterDatabaseRowStruct, DropItemDatabaseRowStruct,
            ElementEnum, WorkSuitabilityEnum, MasterDataTablesUtilityClass,
            PalSearchBox, PalSearchButton, PalClearQueryButton, PalClearSelectionButton,
            PalElementToggles, PalWorkToggles, PalResultsWrap, PalResultCount, PalSelectedSummary,
            PopulatePalCatalog, RenderPalCatalog, Y + 1800)
        || !BuildPanelPalCatalogEvents(
            Blueprint, ModActorClass, PalSearchBox, PalSearchButton,
            PalClearQueryButton, PalClearSelectionButton,
            PalSortModeButtons, PalSortAscendingButton, PalSortDescendingButton,
            PalElementToggles, PalWorkToggles, Y + 2160)
        || !BuildPanelPassiveSearchEvents(
            Blueprint, PassiveSearchBox, PassiveSearchButton, PassiveClearSearchButton, Y + 2520)
        || !BuildPanelClearFiltersEvent(
            Blueprint, ClearPassiveFilters, ModActorClass, false, {}, ElementToggles,
            GenderStatus, GenderButtons, LuckyStatus, LuckyButtons, BossStatus, BossButtons,
            CollectionStatus, CollectionButtons, Y + 3240)
        || !BuildPanelClearFiltersEvent(
            Blueprint, ClearAllFilters, ModActorClass, true, FilterNumericControls, ElementToggles,
            GenderStatus, GenderButtons, LuckyStatus, LuckyButtons, BossStatus, BossButtons,
            CollectionStatus, CollectionButtons, Y + 3600)) {
        return false;
    }
    Y += 3600;
    if (!BuildPanelVisibilityEvent(Blueprint, AdvancedExpand, TEXT("ESP_AdvancedBox"), TEXT("Visible"), Y)
        || !BuildPanelVisibilityEvent(Blueprint, AdvancedCollapse, TEXT("ESP_AdvancedBox"), TEXT("Collapsed"), Y + 360)) {
        return false;
    }
    Y += 720;

    const TArray<FPanelTranslation> Translations = {
        {TEXT("ESP_TitleText"), TEXT("帕鲁资源 ESP"), TEXT("Pal & Resource ESP")},
        {TEXT("ESP_RuntimeText"), TEXT("启用模组"), TEXT("Enable Mod")},
        {TEXT("ESP_StyleHeadingText"), TEXT("显示"), TEXT("Display")},
        {TEXT("ESP_TopGuideText"), TEXT("顶部引导线"), TEXT("Top guide lines")},
        {TEXT("ESP_ShowNameText"), TEXT("姓名"), TEXT("Name")},
        {TEXT("ESP_ShowLevelText"), TEXT("等级"), TEXT("Level")},
        {TEXT("ESP_ShowDistanceText"), TEXT("距离"), TEXT("Distance")},
        {TEXT("ESP_ShowIvText"), TEXT("个体值"), TEXT("IVs")},
        {TEXT("ESP_ShowPassiveSkillsText"), TEXT("词条"), TEXT("Passive skills")},
        {TEXT("ESP_DisplayTabText"), TEXT("显示内容"), TEXT("Display")},
        {TEXT("ESP_FilterTabText"), TEXT("筛选"), TEXT("Filters")},
        {TEXT("ESP_FilterConditionsTabText"), TEXT("常规筛选"), TEXT("General")},
        {TEXT("ESP_FilterPalTabText"), TEXT("帕鲁种类"), TEXT("Pal species")},
        {TEXT("ESP_StyleTabText"), TEXT("显示样式（开发中）"), TEXT("Style (coming later)")},
        {TEXT("ESP_PalHeadingText"), TEXT("帕鲁种类筛选"), TEXT("Pal species filter")},
        {TEXT("ESP_PalSummaryText"), TEXT("查询条件必须全部命中；卡片多选用于世界目标，未选择时显示全部种类"), TEXT("Queries use AND; selected cards filter world targets, and no selection means all species")},
        {TEXT("ESP_PalSortHeadingText"), TEXT("排序（重新读取当前游戏数据）"), TEXT("Sort (reload current game data)")},
        {TEXT("ESP_PalSortIdText"), TEXT("图鉴编号"), TEXT("Paldex number")},
        {TEXT("ESP_PalSortSprintText"), TEXT("冲刺速度"), TEXT("Sprint speed")},
        {TEXT("ESP_PalSortWorkEmitFlameText"), TEXT("生火"), TEXT("Kindling")},
        {TEXT("ESP_PalSortWorkWateringText"), TEXT("浇水"), TEXT("Watering")},
        {TEXT("ESP_PalSortWorkSeedingText"), TEXT("播种"), TEXT("Planting")},
        {TEXT("ESP_PalSortWorkGenerateElectricityText"), TEXT("发电"), TEXT("Electricity")},
        {TEXT("ESP_PalSortWorkHandcraftText"), TEXT("手工作业"), TEXT("Handiwork")},
        {TEXT("ESP_PalSortWorkCollectionText"), TEXT("采集"), TEXT("Gathering")},
        {TEXT("ESP_PalSortWorkDeforestText"), TEXT("伐木"), TEXT("Lumbering")},
        {TEXT("ESP_PalSortWorkMiningText"), TEXT("采矿"), TEXT("Mining")},
        {TEXT("ESP_PalSortWorkProductMedicineText"), TEXT("制药"), TEXT("Medicine")},
        {TEXT("ESP_PalSortWorkCoolText"), TEXT("冷却"), TEXT("Cooling")},
        {TEXT("ESP_PalSortWorkTransportText"), TEXT("搬运"), TEXT("Transporting")},
        {TEXT("ESP_PalSortWorkMonsterFarmText"), TEXT("牧场"), TEXT("Ranching")},
        {TEXT("ESP_PalSortAscendingText"), TEXT("升序"), TEXT("Ascending")},
        {TEXT("ESP_PalSortDescendingText"), TEXT("降序"), TEXT("Descending")},
        {TEXT("ESP_PalSearchLabelText"), TEXT("按名称或图鉴编号搜索"), TEXT("Search by name or Paldex number")},
        {TEXT("ESP_PalSearchText"), TEXT("搜索"), TEXT("Search")},
        {TEXT("ESP_PalClearQueryText"), TEXT("清空查询"), TEXT("Clear query")},
        {TEXT("ESP_PalClearSelectionText"), TEXT("清空已选帕鲁"), TEXT("Clear selected Pals")},
        {TEXT("ESP_PalElementHeadingText"), TEXT("属性（全部命中）"), TEXT("Elements (match all)")},
        {TEXT("ESP_PalWorkHeadingText"), TEXT("工作适应性（全部具备）"), TEXT("Work suitability (match all)")},
        {TEXT("ESP_PalElementNormalText"), TEXT("普通"), TEXT("Normal")},
        {TEXT("ESP_PalElementFireText"), TEXT("火"), TEXT("Fire")},
        {TEXT("ESP_PalElementWaterText"), TEXT("水"), TEXT("Water")},
        {TEXT("ESP_PalElementLeafText"), TEXT("草"), TEXT("Grass")},
        {TEXT("ESP_PalElementElectricityText"), TEXT("雷"), TEXT("Electric")},
        {TEXT("ESP_PalElementIceText"), TEXT("冰"), TEXT("Ice")},
        {TEXT("ESP_PalElementEarthText"), TEXT("地"), TEXT("Ground")},
        {TEXT("ESP_PalElementDarkText"), TEXT("暗"), TEXT("Dark")},
        {TEXT("ESP_PalElementDragonText"), TEXT("龙"), TEXT("Dragon")},
        {TEXT("ESP_PalWorkEmitFlameText"), TEXT("生火"), TEXT("Kindling")},
        {TEXT("ESP_PalWorkWateringText"), TEXT("浇水"), TEXT("Watering")},
        {TEXT("ESP_PalWorkSeedingText"), TEXT("播种"), TEXT("Planting")},
        {TEXT("ESP_PalWorkGenerateElectricityText"), TEXT("发电"), TEXT("Electricity")},
        {TEXT("ESP_PalWorkHandcraftText"), TEXT("手工作业"), TEXT("Handiwork")},
        {TEXT("ESP_PalWorkCollectionText"), TEXT("采集"), TEXT("Gathering")},
        {TEXT("ESP_PalWorkDeforestText"), TEXT("伐木"), TEXT("Lumbering")},
        {TEXT("ESP_PalWorkMiningText"), TEXT("采矿"), TEXT("Mining")},
        {TEXT("ESP_PalWorkProductMedicineText"), TEXT("制药"), TEXT("Medicine")},
        {TEXT("ESP_PalWorkCoolText"), TEXT("冷却"), TEXT("Cooling")},
        {TEXT("ESP_PalWorkTransportText"), TEXT("搬运"), TEXT("Transporting")},
        {TEXT("ESP_PalWorkMonsterFarmText"), TEXT("牧场"), TEXT("Ranching")},
        {TEXT("ESP_StylePlaceholderText"), TEXT("显示样式将在后续版本提供"), TEXT("Display styles will be added later")},
        {TEXT("ESP_PassiveHeadingText"), TEXT("被动技能"), TEXT("Passive skills")},
        {TEXT("ESP_PassiveSummaryText"), TEXT("左键：必须包含（最多 4 个，全部命中）；右键：排除"), TEXT("Left: must include (max 4, AND); right: exclude")},
        {TEXT("ESP_PassiveSearchLabelText"), TEXT("搜索被动技能"), TEXT("Search passive skills")},
        {TEXT("ESP_PassiveSearchText"), TEXT("搜索"), TEXT("Search")},
        {TEXT("ESP_PassiveClearSearchText"), TEXT("清空"), TEXT("Clear")},
        {TEXT("ESP_ClearAllFiltersText"), TEXT("筛选设置为默认"), TEXT("Reset filters to defaults")},
        {TEXT("ESP_ClearPassiveFiltersText"), TEXT("清空词条"), TEXT("Clear passives")},
        {TEXT("ESP_PassiveRainbowHeaderText"), TEXT("彩虹"), TEXT("Rainbow")},
        {TEXT("ESP_PassiveSpecialHeaderText"), TEXT("传说"), TEXT("Legend")},
        {TEXT("ESP_PassiveGoldHeaderText"), TEXT("金色 III"), TEXT("Gold III")},
        {TEXT("ESP_PassiveGold2HeaderText"), TEXT("金色 II"), TEXT("Gold II")},
        {TEXT("ESP_PassiveNormalHeaderText"), TEXT("普通"), TEXT("Normal")},
        {TEXT("ESP_PassiveNegative1HeaderText"), TEXT("负面 I"), TEXT("Negative I")},
        {TEXT("ESP_PassiveNegative2HeaderText"), TEXT("负面 II"), TEXT("Negative II")},
        {TEXT("ESP_PassiveNegative3HeaderText"), TEXT("负面 III"), TEXT("Negative III")},
        {TEXT("ESP_FilterHeadingText"), TEXT("筛选与数量"), TEXT("Filters & count")},
        {TEXT("ESP_DisplayTargetLimitLabelText"), TEXT("目标显示上限"), TEXT("Visible target limit")},
        {TEXT("ESP_LevelHeadingText"), TEXT("等级范围"), TEXT("Level range")},
        {TEXT("ESP_LevelMinLabelText"), TEXT("最低等级（0 = 不限）"), TEXT("Minimum level (0 = any)")},
        {TEXT("ESP_LevelMaxLabelText"), TEXT("最高等级（0 = 不限）"), TEXT("Maximum level (0 = any)")},
        {TEXT("ESP_DistanceMaxLabelText"), TEXT("最大距离"), TEXT("Maximum distance")},
        {TEXT("ESP_DistanceMaxUnitText"), TEXT("米"), TEXT("m")},
        {TEXT("ESP_IvHeadingText"), TEXT("个体值下限（0 = 不限）"), TEXT("Minimum IVs (0 = any)")},
        {TEXT("ESP_IvHpMinLabelText"), TEXT("生命"), TEXT("HP")},
        {TEXT("ESP_IvAttackMinLabelText"), TEXT("攻击"), TEXT("Attack")},
        {TEXT("ESP_IvDefenseMinLabelText"), TEXT("防御"), TEXT("Defense")},
        {TEXT("ESP_ElementHeadingText"), TEXT("属性（任一匹配）"), TEXT("Elements (match any)")},
        {TEXT("ESP_ElementStatusText"), TEXT("未选择：全部"), TEXT("None selected: all")},
        {TEXT("ESP_ElementNormalText"), TEXT("普通"), TEXT("Normal")},
        {TEXT("ESP_ElementFireText"), TEXT("火"), TEXT("Fire")},
        {TEXT("ESP_ElementWaterText"), TEXT("水"), TEXT("Water")},
        {TEXT("ESP_ElementLeafText"), TEXT("草"), TEXT("Grass")},
        {TEXT("ESP_ElementElectricityText"), TEXT("雷"), TEXT("Electric")},
        {TEXT("ESP_ElementIceText"), TEXT("冰"), TEXT("Ice")},
        {TEXT("ESP_ElementEarthText"), TEXT("地"), TEXT("Ground")},
        {TEXT("ESP_ElementDarkText"), TEXT("暗"), TEXT("Dark")},
        {TEXT("ESP_ElementDragonText"), TEXT("龙"), TEXT("Dragon")},
        {TEXT("ESP_GenderHeadingText"), TEXT("性别"), TEXT("Gender")},
        {TEXT("ESP_GenderAllText"), TEXT("全部"), TEXT("All")},
        {TEXT("ESP_GenderMaleText"), TEXT("雄性"), TEXT("Male")},
        {TEXT("ESP_GenderFemaleText"), TEXT("雌性"), TEXT("Female")},
        {TEXT("ESP_LuckyHeadingText"), TEXT("闪光个体"), TEXT("Lucky Pal")},
        {TEXT("ESP_LuckyAllText"), TEXT("全部"), TEXT("All")},
        {TEXT("ESP_LuckyOnlyText"), TEXT("仅闪光"), TEXT("Only Lucky")},
        {TEXT("ESP_LuckyExcludeText"), TEXT("排除闪光"), TEXT("Exclude Lucky")},
        {TEXT("ESP_BossHeadingText"), TEXT("首领个体"), TEXT("Alpha / Boss Pal")},
        {TEXT("ESP_BossAllText"), TEXT("全部"), TEXT("All")},
        {TEXT("ESP_BossOnlyText"), TEXT("仅首领"), TEXT("Only Boss")},
        {TEXT("ESP_BossExcludeText"), TEXT("排除首领"), TEXT("Exclude Boss")},
        {TEXT("ESP_LanguageHeadingText"), TEXT("语言"), TEXT("Language")},
        {TEXT("ESP_ChineseText"), TEXT("中文"), TEXT("Chinese")},
        {TEXT("ESP_EnglishText"), TEXT("英文"), TEXT("English")},
        {TEXT("ESP_CollectionHeadingText"), TEXT("图鉴收集（捕捉 5 只完成）"), TEXT("Paldex collection (5 captures complete)")},
        {TEXT("ESP_CollectionAllText"), TEXT("全部"), TEXT("All")},
        {TEXT("ESP_CollectionIncompleteText"), TEXT("未完成"), TEXT("Incomplete")},
        {TEXT("ESP_CollectionCompleteText"), TEXT("已完成"), TEXT("Complete")},
        {TEXT("ESP_AdvancedExpandText"), TEXT("高级诊断"), TEXT("Advanced diagnostics")},
        {TEXT("ESP_ModeHeadingText"), TEXT("实验模式"), TEXT("Experiment mode")},
        {TEXT("ESP_ModeOffText"), TEXT("关闭"), TEXT("Off")},
        {TEXT("ESP_ModeSnapshotText"), TEXT("单次快照"), TEXT("Snapshot once")},
        {TEXT("ESP_ModeChunkedText"), TEXT("安全快照"), TEXT("Safe snapshot")},
        {TEXT("ESP_ModeEventText"), TEXT("事件优先"), TEXT("Event first")},
        {TEXT("ESP_CaptureStartText"), TEXT("开始采集"), TEXT("Start capture")},
        {TEXT("ESP_CaptureStopText"), TEXT("停止采集"), TEXT("Stop capture")},
        {TEXT("ESP_AdvancedCollapseText"), TEXT("收起"), TEXT("Collapse")},
    };
    const TArray<UButton*> LanguageButtons = {Chinese, English};
    if (!BuildPanelInitializeLanguage(Blueprint, Translations, LanguageButtons, Y)
        || !BuildPanelLanguageEvent(Blueprint, Chinese, ModActorClass, 0, Translations, Y + 720, &LanguageButtons, true)
        || !BuildPanelLanguageEvent(Blueprint, English, ModActorClass, 1, Translations, Y + 1440, &LanguageButtons, true)
        || !BuildPanelKeyDownOverride(Blueprint)
        || !BuildPanelKeyUpOverride(Blueprint, ModActorClass)) {
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildPanelV2 compile status=%d nodes=%d"), static_cast<int32>(Blueprint->Status), Graph->Nodes.Num());
    return Blueprint->Status != BS_Error;
}

bool BuildModActor(
    UBlueprint* Blueprint,
    UClass* PalMonsterClass,
    UClass* OverlayClass,
    UClass* PanelClass,
    UClass* PalPlayerControllerClass) {
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor begin blueprint=%s pal_class=%s overlay_class=%s panel_class=%s"), *GetNameSafe(Blueprint), *GetNameSafe(PalMonsterClass), *GetNameSafe(OverlayClass), *GetNameSafe(PanelClass));
    UEdGraph* Graph = EventGraph(Blueprint);
    if (!Graph || !PanelClass || !PalPlayerControllerClass
        || !PalPlayerControllerClass->FindFunctionByName(TEXT("SetDisableInputFlag"))
        || !PrepareModActorControls(Blueprint)) {
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
    if (!EnsureMemberVariable(Blueprint, PanelMainPageVariableName, IntPin())
        || !EnsureMemberVariable(Blueprint, PanelFilterPageVariableName, IntPin())) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor panel page variable creation failed"));
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
        TPair<FName, FEdGraphPinType>(FName("SessionIndex"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("Level"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("DistanceMeters"), IntPin())
    });
    UK2Node_CustomEvent* ClearTarget = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_ClearTarget"), -900, 920, {
        TPair<FName, FEdGraphPinType>(FName("SessionIndex"), IntPin())
    });
    UK2Node_CustomEvent* RemoveTarget = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_RemoveTarget"), -900, 1080, {
        TPair<FName, FEdGraphPinType>(FName("Target"), ObjectPin(PalMonsterClass)),
        TPair<FName, FEdGraphPinType>(FName("SessionIndex"), IntPin())
    });
    UK2Node_CustomEvent* SetDisplayStyle = AddCustomEvent(Blueprint, Graph, TEXT("PalworldResourceESP_SetDisplayStyle"), -900, 1200, {
        TPair<FName, FEdGraphPinType>(FName("ShowTopGuideLine"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowName"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowLevel"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowDistance"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowIV"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("ShowPassiveSkills"), BoolPin()),
        TPair<FName, FEdGraphPinType>(FName("IvMin"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("IvHpMin"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("IvAttackMin"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("IvDefenseMin"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("PassiveFilterRevision"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("GenderFilterId"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("LuckyFilterId"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("BossFilterId"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("CollectionFilterId"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("SpeciesFilterText"), StringPin()),
        TPair<FName, FEdGraphPinType>(FName("ElementFilterMask"), IntPin()),
        TPair<FName, FEdGraphPinType>(FName("LanguageId"), IntPin())
    });
    UK2Node_CustomEvent* TogglePanel = AddCustomEvent(
        Blueprint, Graph, *PanelToggleEventName.ToString(), -900, 1520, {});
    UK2Node_CustomEvent* ApplyPersistedPanelState = AddCustomEvent(
        Blueprint, Graph, *ApplyPersistedPanelStateEventName.ToString(), -900, 3200, {});
    if (!PostBeginPlay || !BridgeReady || !Reset || !SetTarget || !ClearTarget || !RemoveTarget || !SetDisplayStyle
        || !TogglePanel || !ApplyPersistedPanelState) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor custom event creation failed post=%s ready=%s reset=%s set=%s clear=%s toggle=%s"), PostBeginPlay ? TEXT("ok") : TEXT("null"), BridgeReady ? TEXT("ok") : TEXT("null"), Reset ? TEXT("ok") : TEXT("null"), SetTarget ? TEXT("ok") : TEXT("null"), ClearTarget ? TEXT("ok") : TEXT("null"), TogglePanel ? TEXT("ok") : TEXT("null"));
        return false;
    }

    UK2Node_VariableGet* RestoreFilterIdsClearGet = AddVariableGet(Graph, PassiveFilterIdsVariableName, -620, 3360);
    UK2Node_CallArrayFunction* RestoreFilterIdsClear = AddArrayCall(Graph, TEXT("Array_Clear"), -340, 3200);
    UK2Node_VariableGet* RestoreExcludeIdsClearGet = AddVariableGet(Graph, PassiveExcludeIdsVariableName, -340, 3360);
    UK2Node_CallArrayFunction* RestoreExcludeIdsClear = AddArrayCall(Graph, TEXT("Array_Clear"), -60, 3200);
    UK2Node_VariableGet* RestoreIncludeTextGet = AddVariableGet(Graph, PassiveIncludeTextVariableName, -60, 3520);
    UK2Node_CallFunction* RestoreParseIncludes = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("ParseIntoArray"), 220, 3440);
    UK2Node_MacroInstance* RestoreForEachInclude = AddForEachLoop(Graph, 500, 3200);
    UK2Node_CallFunction* RestoreIncludeToName = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_StringToName"), 780, 3440);
    UK2Node_VariableGet* RestoreFilterIdsAddGet = AddVariableGet(Graph, PassiveFilterIdsVariableName, 780, 3560);
    UK2Node_CallArrayFunction* RestoreFilterIdsAdd = AddArrayCall(Graph, TEXT("Array_AddUnique"), 1060, 3200);

    UK2Node_VariableGet* RestoreExcludeTextGet = AddVariableGet(Graph, PassiveExcludeTextVariableName, 1340, 3520);
    UK2Node_CallFunction* RestoreParseExcludes = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("ParseIntoArray"), 1620, 3440);
    UK2Node_MacroInstance* RestoreForEachExclude = AddForEachLoop(Graph, 1900, 3200);
    UK2Node_CallFunction* RestoreExcludeToName = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_StringToName"), 2180, 3440);
    UK2Node_VariableGet* RestoreExcludeIdsAddGet = AddVariableGet(Graph, PassiveExcludeIdsVariableName, 2180, 3560);
    UK2Node_CallArrayFunction* RestoreExcludeIdsAdd = AddArrayCall(Graph, TEXT("Array_AddUnique"), 2460, 3200);
    UK2Node_VariableGet* RestoreSpeciesIdsClearGet = AddVariableGet(
        Graph, SpeciesFilterIdsVariableName, 2740, 3520);
    UK2Node_CallArrayFunction* RestoreSpeciesIdsClear = AddArrayCall(Graph, TEXT("Array_Clear"), 3020, 3200);
    UK2Node_VariableGet* RestoreSpeciesTextGet = AddVariableGet(
        Graph, SpeciesFilterTextVariableName, 3020, 3520);
    UK2Node_CallFunction* RestoreParseSpecies = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("ParseIntoArray"), 3300, 3440);
    UK2Node_MacroInstance* RestoreForEachSpecies = AddForEachLoop(Graph, 3580, 3200);
    UK2Node_CallFunction* RestoreSpeciesToName = AddStaticCall(
        Graph, UKismetStringLibrary::StaticClass(), TEXT("Conv_StringToName"), 3860, 3440);
    UK2Node_VariableGet* RestoreSpeciesIdsAddGet = AddVariableGet(
        Graph, SpeciesFilterIdsVariableName, 3860, 3560);
    UK2Node_CallArrayFunction* RestoreSpeciesIdsAdd = AddArrayCall(Graph, TEXT("Array_AddUnique"), 4140, 3200);
    UK2Node_VariableGet* RestoreRevisionGet = AddVariableGet(Graph, PassiveFilterRevisionVariableName, 4420, 3360);
    UK2Node_CallFunction* RestoreRevisionAdd = AddStaticCall(
        Graph, UKismetMathLibrary::StaticClass(), TEXT("Add_IntInt"), 4700, 3360);
    UK2Node_VariableSet* RestoreRevisionSet = AddVariableSet(Graph, PassiveFilterRevisionVariableName, 4980, 3200);
    if (!RestoreFilterIdsClearGet || !RestoreFilterIdsClear || !RestoreExcludeIdsClearGet || !RestoreExcludeIdsClear
        || !RestoreIncludeTextGet || !RestoreParseIncludes || !RestoreForEachInclude || !RestoreIncludeToName
        || !RestoreFilterIdsAddGet || !RestoreFilterIdsAdd || !RestoreExcludeTextGet || !RestoreParseExcludes
        || !RestoreForEachExclude || !RestoreExcludeToName || !RestoreExcludeIdsAddGet || !RestoreExcludeIdsAdd
        || !RestoreSpeciesIdsClearGet || !RestoreSpeciesIdsClear || !RestoreSpeciesTextGet || !RestoreParseSpecies
        || !RestoreForEachSpecies || !RestoreSpeciesToName || !RestoreSpeciesIdsAddGet || !RestoreSpeciesIdsAdd
        || !RestoreRevisionGet || !RestoreRevisionAdd || !RestoreRevisionSet
        || !Link(ApplyPersistedPanelState, UEdGraphSchema_K2::PN_Then, RestoreFilterIdsClear, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreFilterIdsClearGet, PassiveFilterIdsVariableName, RestoreFilterIdsClear, TEXT("TargetArray"))
        || !Link(RestoreFilterIdsClear, UEdGraphSchema_K2::PN_Then, RestoreExcludeIdsClear, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreExcludeIdsClearGet, PassiveExcludeIdsVariableName, RestoreExcludeIdsClear, TEXT("TargetArray"))
        || !Link(RestoreIncludeTextGet, PassiveIncludeTextVariableName, RestoreParseIncludes, TEXT("SourceString"))
        || !SetPinDefault(RestoreParseIncludes, TEXT("Delimiter"), TEXT("|"))
        || !SetPinDefault(RestoreParseIncludes, TEXT("CullEmptyStrings"), TEXT("true"))
        || !Link(RestoreParseIncludes, UEdGraphSchema_K2::PN_ReturnValue, RestoreForEachInclude, TEXT("Array"))
        || !Link(RestoreExcludeIdsClear, UEdGraphSchema_K2::PN_Then, RestoreForEachInclude, TEXT("Exec"))
        || !Link(RestoreForEachInclude, TEXT("Array Element"), RestoreIncludeToName, TEXT("InString"))
        || !Link(RestoreForEachInclude, TEXT("LoopBody"), RestoreFilterIdsAdd, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreFilterIdsAddGet, PassiveFilterIdsVariableName, RestoreFilterIdsAdd, TEXT("TargetArray"))
        || !Link(RestoreIncludeToName, UEdGraphSchema_K2::PN_ReturnValue, RestoreFilterIdsAdd, TEXT("NewItem"))
        || !Link(RestoreExcludeTextGet, PassiveExcludeTextVariableName, RestoreParseExcludes, TEXT("SourceString"))
        || !SetPinDefault(RestoreParseExcludes, TEXT("Delimiter"), TEXT("|"))
        || !SetPinDefault(RestoreParseExcludes, TEXT("CullEmptyStrings"), TEXT("true"))
        || !Link(RestoreParseExcludes, UEdGraphSchema_K2::PN_ReturnValue, RestoreForEachExclude, TEXT("Array"))
        || !Link(RestoreForEachInclude, TEXT("Completed"), RestoreForEachExclude, TEXT("Exec"))
        || !Link(RestoreForEachExclude, TEXT("Array Element"), RestoreExcludeToName, TEXT("InString"))
        || !Link(RestoreForEachExclude, TEXT("LoopBody"), RestoreExcludeIdsAdd, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreExcludeIdsAddGet, PassiveExcludeIdsVariableName, RestoreExcludeIdsAdd, TEXT("TargetArray"))
        || !Link(RestoreExcludeToName, UEdGraphSchema_K2::PN_ReturnValue, RestoreExcludeIdsAdd, TEXT("NewItem"))
        || !Link(RestoreForEachExclude, TEXT("Completed"), RestoreSpeciesIdsClear, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreSpeciesIdsClearGet, SpeciesFilterIdsVariableName, RestoreSpeciesIdsClear, TEXT("TargetArray"))
        || !Link(RestoreSpeciesTextGet, SpeciesFilterTextVariableName, RestoreParseSpecies, TEXT("SourceString"))
        || !SetPinDefault(RestoreParseSpecies, TEXT("Delimiter"), TEXT("|"))
        || !SetPinDefault(RestoreParseSpecies, TEXT("CullEmptyStrings"), TEXT("true"))
        || !Link(RestoreParseSpecies, UEdGraphSchema_K2::PN_ReturnValue, RestoreForEachSpecies, TEXT("Array"))
        || !Link(RestoreSpeciesIdsClear, UEdGraphSchema_K2::PN_Then, RestoreForEachSpecies, TEXT("Exec"))
        || !Link(RestoreForEachSpecies, TEXT("Array Element"), RestoreSpeciesToName, TEXT("InString"))
        || !Link(RestoreForEachSpecies, TEXT("LoopBody"), RestoreSpeciesIdsAdd, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreSpeciesIdsAddGet, SpeciesFilterIdsVariableName, RestoreSpeciesIdsAdd, TEXT("TargetArray"))
        || !Link(RestoreSpeciesToName, UEdGraphSchema_K2::PN_ReturnValue, RestoreSpeciesIdsAdd, TEXT("NewItem"))
        || !Link(RestoreForEachSpecies, TEXT("Completed"), RestoreRevisionSet, UEdGraphSchema_K2::PN_Execute)
        || !Link(RestoreRevisionGet, PassiveFilterRevisionVariableName, RestoreRevisionAdd, TEXT("A"))
        || !SetPinDefault(RestoreRevisionAdd, TEXT("B"), TEXT("1"))
        || !Link(RestoreRevisionAdd, UEdGraphSchema_K2::PN_ReturnValue, RestoreRevisionSet, PassiveFilterRevisionVariableName)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor persisted passive restore graph failed"));
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
    UK2Node_CallFunction* EnableGameInput = AddStaticCall(Graph, AActor::StaticClass(), TEXT("EnableInput"), 140, 1320);
    UK2Node_CallFunction* RemovePanel = AddStaticCall(Graph, UUserWidget::StaticClass(), TEXT("RemoveFromParent"), 140, 1440);
    UK2Node_VariableSet* ClearPanel = AddVariableSet(Graph, PanelVariableName, 420, 1440);
    UK2Node_CallFunction* CloseController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), 420, 1600);
    UK2Node_Self* CloseWorldContext = AddSelfNode(Graph, 140, 1680);
    UK2Node_DynamicCast* CastClosePalController = AddDynamicCast(Graph, PalPlayerControllerClass, -100, 1320);
    UK2Node_CallFunction* ClearPalInputDisable = AddStaticCall(
        Graph, PalPlayerControllerClass, TEXT("SetDisableInputFlag"), 20, 1320);
    UK2Node_VariableSet* HideCursor = AddExternalVariableSet(Graph, TEXT("bShowMouseCursor"), APlayerController::StaticClass(), 700, 1440);
    UK2Node_CallFunction* GameOnly = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("SetInputMode_GameOnly"), 980, 1440);

    UK2Node_CallFunction* OpenController = AddStaticCall(Graph, UGameplayStatics::StaticClass(), TEXT("GetPlayerController"), 140, 1920);
    UK2Node_Self* OpenWorldContext = AddSelfNode(Graph, -120, 2000);
    UK2Node_DynamicCast* CastOpenPalController = AddDynamicCast(Graph, PalPlayerControllerClass, 160, 1760);
    UK2Node_CallFunction* CreatePanel = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("Create"), 420, 1760);
    UK2Node_DynamicCast* CastPanel = AddDynamicCast(Graph, PanelClass, 700, 1760);
    UK2Node_VariableSet* StorePanel = AddVariableSet(Graph, PanelVariableName, 980, 1760);
    UK2Node_VariableSet* SetPanelBridge = AddExternalVariableSet(Graph, PanelBridgeVariableName, PanelClass, 1260, 1760);
    UK2Node_Self* ActorSelf = AddSelfNode(Graph, 980, 1960);
    // __DEPRECATED_20260717__ [reason: V2 initializes sliders and all visible toggle states]
    // UK2Node_CallFunction* InitializePanel = AddStaticCall(Graph, PanelClass, *PanelInitializeControlsEventName.ToString(), 1540, 1760);
    UK2Node_CallFunction* InitializePanel = AddStaticCall(Graph, PanelClass, *PanelInitializeControlsV2EventName.ToString(), 1540, 1760);
    UK2Node_CallFunction* InitializePageState = AddStaticCall(Graph, PanelClass, *PanelSetPageStateEventName.ToString(), 1820, 1760);
    UK2Node_CallFunction* InitializeLanguage = AddStaticCall(Graph, PanelClass, *PanelInitializeLanguageEventName.ToString(), 1820, 1760);
    UK2Node_CallFunction* PopulatePassiveCatalog = AddStaticCall(Graph, PanelClass, *PanelPopulatePassiveCatalogEventName.ToString(), 2100, 1760);
    UK2Node_CallFunction* PopulatePalCatalog = AddStaticCall(
        Graph, PanelClass, *PanelPopulatePalCatalogEventName.ToString(), 2380, 1760);
    UK2Node_VariableGet* PanelDisplayLimit = AddVariableGet(Graph, DisplayTargetLimitVariableName, 1260, 2080);
    UK2Node_VariableGet* PanelLevelMin = AddVariableGet(Graph, LevelMinVariableName, 1540, 2080);
    UK2Node_VariableGet* PanelLevelMax = AddVariableGet(Graph, LevelMaxVariableName, 1820, 2080);
    // __DEPRECATED_20260717__ [reason: minimum distance is no longer a user-facing or runtime filter]
    // UK2Node_VariableGet* PanelDistanceMin = AddVariableGet(Graph, DistanceMinVariableName, 2100, 2080);
    UK2Node_VariableGet* PanelDistanceMax = AddVariableGet(Graph, DistanceMaxVariableName, 2380, 2080);
    UK2Node_VariableGet* PanelShowLevel = AddVariableGet(Graph, ShowLevelVariableName, 2660, 2080);
    UK2Node_VariableGet* PanelShowDistance = AddVariableGet(Graph, ShowDistanceVariableName, 2940, 2080);
    UK2Node_VariableGet* PanelShowIv = AddVariableGet(Graph, ShowIvVariableName, 3220, 2200);
    UK2Node_VariableGet* PanelShowPassiveSkills = AddVariableGet(Graph, ShowPassiveSkillsVariableName, 3220, 2320);
    UK2Node_VariableGet* PanelIvHpMin = AddVariableGet(Graph, IvHpMinVariableName, 3500, 2200);
    UK2Node_VariableGet* PanelIvAttackMin = AddVariableGet(Graph, IvAttackMinVariableName, 3780, 2200);
    UK2Node_VariableGet* PanelIvDefenseMin = AddVariableGet(Graph, IvDefenseMinVariableName, 4060, 2200);
    UK2Node_VariableGet* PanelRuntimeEnabled = AddVariableGet(Graph, RuntimeEnabledVariableName, 3220, 2080);
    UK2Node_VariableGet* PanelTopGuide = AddVariableGet(Graph, ShowTopGuideLineVariableName, 3500, 2080);
    UK2Node_VariableGet* PanelGenderFilter = AddVariableGet(Graph, GenderFilterIdVariableName, 3780, 2080);
    UK2Node_VariableGet* PanelLuckyFilter = AddVariableGet(Graph, LuckyFilterIdVariableName, 4060, 2080);
    UK2Node_VariableGet* PanelBossFilter = AddVariableGet(Graph, BossFilterIdVariableName, 4340, 2080);
    UK2Node_VariableGet* PanelCollectionFilter = AddVariableGet(Graph, CollectionFilterIdVariableName, 4340, 1960);
    UK2Node_VariableGet* PanelElementNormal = AddVariableGet(Graph, ElementNormalVariableName, 4620, 2200);
    UK2Node_VariableGet* PanelElementFire = AddVariableGet(Graph, ElementFireVariableName, 4900, 2200);
    UK2Node_VariableGet* PanelElementWater = AddVariableGet(Graph, ElementWaterVariableName, 5180, 2200);
    UK2Node_VariableGet* PanelElementLeaf = AddVariableGet(Graph, ElementLeafVariableName, 5460, 2200);
    UK2Node_VariableGet* PanelElementElectricity = AddVariableGet(Graph, ElementElectricityVariableName, 5740, 2200);
    UK2Node_VariableGet* PanelElementIce = AddVariableGet(Graph, ElementIceVariableName, 6020, 2200);
    UK2Node_VariableGet* PanelElementEarth = AddVariableGet(Graph, ElementEarthVariableName, 6300, 2200);
    UK2Node_VariableGet* PanelElementDark = AddVariableGet(Graph, ElementDarkVariableName, 6580, 2200);
    UK2Node_VariableGet* PanelElementDragon = AddVariableGet(Graph, ElementDragonVariableName, 6860, 2200);
    UK2Node_VariableGet* PanelShowName = AddVariableGet(Graph, ShowNameVariableName, 4620, 2080);
    UK2Node_VariableGet* PanelMainPage = AddVariableGet(Graph, PanelMainPageVariableName, 4900, 2200);
    UK2Node_VariableGet* PanelFilterPage = AddVariableGet(Graph, PanelFilterPageVariableName, 5180, 2200);
    UK2Node_VariableGet* PanelLanguage = AddVariableGet(Graph, LanguageIdVariableName, 4900, 2080);
    UK2Node_VariableGet* PanelExpandRainbow = AddVariableGet(Graph, PassiveRainbowExpandedVariableName, 5180, 2080);
    UK2Node_VariableGet* PanelExpandLegend = AddVariableGet(Graph, PassiveLegendExpandedVariableName, 5460, 2080);
    UK2Node_VariableGet* PanelExpandGold3 = AddVariableGet(Graph, PassiveGold3ExpandedVariableName, 5740, 2080);
    UK2Node_VariableGet* PanelExpandGold2 = AddVariableGet(Graph, PassiveGold2ExpandedVariableName, 6020, 2080);
    UK2Node_VariableGet* PanelExpandNormal = AddVariableGet(Graph, PassiveNormalExpandedVariableName, 6300, 2080);
    UK2Node_VariableGet* PanelExpandNegative1 = AddVariableGet(Graph, PassiveNegative1ExpandedVariableName, 6580, 2080);
    UK2Node_VariableGet* PanelExpandNegative2 = AddVariableGet(Graph, PassiveNegative2ExpandedVariableName, 6860, 2080);
    UK2Node_VariableGet* PanelExpandNegative3 = AddVariableGet(Graph, PassiveNegative3ExpandedVariableName, 7140, 2080);
    UK2Node_CallFunction* AddPanelToViewport = AddStaticCall(Graph, UUserWidget::StaticClass(), TEXT("AddToViewport"), 2100, 1760);
    UK2Node_VariableSet* ShowCursor = AddExternalVariableSet(Graph, TEXT("bShowMouseCursor"), APlayerController::StaticClass(), 2380, 1760);
    UK2Node_CallFunction* UiOnly = AddStaticCall(Graph, UWidgetBlueprintLibrary::StaticClass(), TEXT("SetInputMode_UIOnlyEx"), 2660, 1760);
    UK2Node_CallFunction* FocusPanel = AddStaticCall(Graph, UWidget::StaticClass(), TEXT("SetKeyboardFocus"), 2940, 1760);
    UK2Node_CallFunction* SetPalInputDisable = AddStaticCall(
        Graph, PalPlayerControllerClass, TEXT("SetDisableInputFlag"), 3080, 1760);
    UK2Node_CallFunction* DisableGameInput = AddStaticCall(Graph, AActor::StaticClass(), TEXT("DisableInput"), 2940, 1760);

    if (!PanelGet || !PanelValid || !PanelBranch || !EnableGameInput || !RemovePanel || !ClearPanel || !CloseController || !CloseWorldContext
        || !CastClosePalController || !ClearPalInputDisable
        || !HideCursor || !GameOnly || !OpenController || !OpenWorldContext || !CreatePanel || !CastPanel || !StorePanel
        || !CastOpenPalController
        || !SetPanelBridge || !ActorSelf || !InitializePanel || !InitializePageState || !InitializeLanguage || !PanelDisplayLimit || !PanelLevelMin
        || !PanelLevelMax || !PanelDistanceMax || !PanelShowLevel || !PanelShowDistance || !PanelShowIv
        || !PanelShowPassiveSkills || !PanelIvHpMin || !PanelIvAttackMin || !PanelIvDefenseMin
        || !PanelRuntimeEnabled || !PanelTopGuide || !PanelGenderFilter || !PanelLuckyFilter || !PanelBossFilter
        || !PanelCollectionFilter
        || !PanelElementNormal || !PanelElementFire || !PanelElementWater || !PanelElementLeaf
        || !PanelElementElectricity || !PanelElementIce || !PanelElementEarth || !PanelElementDark || !PanelElementDragon
        || !PanelShowName || !PanelMainPage || !PanelFilterPage || !PanelLanguage || !PanelExpandRainbow || !PanelExpandLegend || !PanelExpandGold3
        || !PanelExpandGold2 || !PanelExpandNormal || !PanelExpandNegative1 || !PanelExpandNegative2 || !PanelExpandNegative3
        || !PopulatePassiveCatalog || !PopulatePalCatalog
        || !AddPanelToViewport || !ShowCursor || !UiOnly || !FocusPanel || !SetPalInputDisable || !DisableGameInput
        || !SetClassPin(CreatePanel, TEXT("WidgetType"), PanelClass)
        || !SetPinDefault(CloseController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetPinDefault(OpenController, TEXT("PlayerIndex"), TEXT("0"))
        || !SetPinDefault(HideCursor, TEXT("bShowMouseCursor"), TEXT("false"))
        || !SetPinDefault(ShowCursor, TEXT("bShowMouseCursor"), TEXT("true"))
        || !SetPinDefault(GameOnly, TEXT("bFlushInput"), TEXT("true"))
        || !SetPinDefault(UiOnly, TEXT("bFlushInput"), TEXT("true"))
        || !SetPinDefault(ClearPalInputDisable, TEXT("flagName"), PanelInputDisableFlag)
        || !SetPinDefault(ClearPalInputDisable, TEXT("isDisable"), TEXT("false"))
        || !SetPinDefault(SetPalInputDisable, TEXT("flagName"), PanelInputDisableFlag)
        || !SetPinDefault(SetPalInputDisable, TEXT("isDisable"), TEXT("true"))
        || !SetPinDefault(AddPanelToViewport, TEXT("ZOrder"), TEXT("100"))
        || !Link(TogglePanel, UEdGraphSchema_K2::PN_Then, PanelBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(PanelGet, PanelVariableName, PanelValid, TEXT("Object"))
        || !Link(PanelValid, UEdGraphSchema_K2::PN_ReturnValue, PanelBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(PanelBranch, UEdGraphSchema_K2::PN_Then, CastClosePalController, UEdGraphSchema_K2::PN_Execute)
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, CastClosePalController, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastClosePalController, CastClosePalController->GetValidCastPin()->PinName, ClearPalInputDisable, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastClosePalController, CastClosePalController->GetCastResultPin()->PinName, ClearPalInputDisable, UEdGraphSchema_K2::PN_Self)
        || !Link(ClearPalInputDisable, UEdGraphSchema_K2::PN_Then, EnableGameInput, UEdGraphSchema_K2::PN_Execute)
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, EnableGameInput, UEdGraphSchema_K2::PN_Self)
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, EnableGameInput, TEXT("PlayerController"))
        || !Link(EnableGameInput, UEdGraphSchema_K2::PN_Then, RemovePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(PanelGet, PanelVariableName, RemovePanel, UEdGraphSchema_K2::PN_Self)
        || !Link(RemovePanel, UEdGraphSchema_K2::PN_Then, ClearPanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(ClearPanel, UEdGraphSchema_K2::PN_Then, HideCursor, UEdGraphSchema_K2::PN_Execute)
        || !Link(CloseWorldContext, UEdGraphSchema_K2::PN_Self, CloseController, TEXT("WorldContextObject"))
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, HideCursor, UEdGraphSchema_K2::PN_Self)
        || !Link(HideCursor, UEdGraphSchema_K2::PN_Then, GameOnly, UEdGraphSchema_K2::PN_Execute)
        || !Link(CloseController, UEdGraphSchema_K2::PN_ReturnValue, GameOnly, TEXT("PlayerController"))
        || !Link(PanelBranch, UEdGraphSchema_K2::PN_Else, CastOpenPalController, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenWorldContext, UEdGraphSchema_K2::PN_Self, OpenController, TEXT("WorldContextObject"))
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, CastOpenPalController, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastOpenPalController, CastOpenPalController->GetValidCastPin()->PinName, CreatePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, CreatePanel, TEXT("OwningPlayer"))
        || !Link(CreatePanel, UEdGraphSchema_K2::PN_Then, CastPanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(CreatePanel, UEdGraphSchema_K2::PN_ReturnValue, CastPanel, UEdGraphSchema_K2::PN_ObjectToCast)
        || !Link(CastPanel, UEdGraphSchema_K2::PN_CastSucceeded, StorePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, StorePanel, PanelVariableName)
        || !Link(StorePanel, UEdGraphSchema_K2::PN_Then, SetPanelBridge, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, SetPanelBridge, UEdGraphSchema_K2::PN_Self)
        || !Link(ActorSelf, UEdGraphSchema_K2::PN_Self, SetPanelBridge, PanelBridgeVariableName)
        || !Link(SetPanelBridge, UEdGraphSchema_K2::PN_Then, InitializePanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, InitializePanel, UEdGraphSchema_K2::PN_Self)
        || !Link(PanelDisplayLimit, DisplayTargetLimitVariableName, InitializePanel, TEXT("DisplayTargetLimit"))
        || !Link(PanelLevelMin, LevelMinVariableName, InitializePanel, TEXT("LevelMin"))
        || !Link(PanelLevelMax, LevelMaxVariableName, InitializePanel, TEXT("LevelMax"))
        // __DEPRECATED_20260717__ [reason: minimum distance is no longer passed to the V2 panel]
        // || !Link(PanelDistanceMin, DistanceMinVariableName, InitializePanel, TEXT("DistanceMin"))
        || !Link(PanelDistanceMax, DistanceMaxVariableName, InitializePanel, TEXT("DistanceMax"))
        || !Link(PanelRuntimeEnabled, RuntimeEnabledVariableName, InitializePanel, TEXT("RuntimeEnabled"))
        || !Link(PanelTopGuide, ShowTopGuideLineVariableName, InitializePanel, TEXT("ShowTopGuideLine"))
        || !Link(PanelShowName, ShowNameVariableName, InitializePanel, TEXT("ShowName"))
        || !Link(PanelShowLevel, ShowLevelVariableName, InitializePanel, TEXT("ShowLevel"))
        || !Link(PanelShowDistance, ShowDistanceVariableName, InitializePanel, TEXT("ShowDistance"))
        || !Link(PanelShowIv, ShowIvVariableName, InitializePanel, TEXT("ShowIV"))
        || !Link(PanelShowPassiveSkills, ShowPassiveSkillsVariableName, InitializePanel, TEXT("ShowPassiveSkills"))
        || !Link(PanelIvHpMin, IvHpMinVariableName, InitializePanel, TEXT("IvHpMin"))
        || !Link(PanelIvAttackMin, IvAttackMinVariableName, InitializePanel, TEXT("IvAttackMin"))
        || !Link(PanelIvDefenseMin, IvDefenseMinVariableName, InitializePanel, TEXT("IvDefenseMin"))
        || !Link(PanelElementNormal, ElementNormalVariableName, InitializePanel, TEXT("ElementNormal"))
        || !Link(PanelElementFire, ElementFireVariableName, InitializePanel, TEXT("ElementFire"))
        || !Link(PanelElementWater, ElementWaterVariableName, InitializePanel, TEXT("ElementWater"))
        || !Link(PanelElementLeaf, ElementLeafVariableName, InitializePanel, TEXT("ElementLeaf"))
        || !Link(PanelElementElectricity, ElementElectricityVariableName, InitializePanel, TEXT("ElementElectricity"))
        || !Link(PanelElementIce, ElementIceVariableName, InitializePanel, TEXT("ElementIce"))
        || !Link(PanelElementEarth, ElementEarthVariableName, InitializePanel, TEXT("ElementEarth"))
        || !Link(PanelElementDark, ElementDarkVariableName, InitializePanel, TEXT("ElementDark"))
        || !Link(PanelElementDragon, ElementDragonVariableName, InitializePanel, TEXT("ElementDragon"))
        || !Link(PanelGenderFilter, GenderFilterIdVariableName, InitializePanel, TEXT("GenderFilterId"))
        || !Link(PanelLuckyFilter, LuckyFilterIdVariableName, InitializePanel, TEXT("LuckyFilterId"))
        || !Link(PanelBossFilter, BossFilterIdVariableName, InitializePanel, TEXT("BossFilterId"))
        || !Link(PanelCollectionFilter, CollectionFilterIdVariableName, InitializePanel, TEXT("CollectionFilterId"))
        || !Link(PanelLanguage, LanguageIdVariableName, InitializePanel, TEXT("LanguageId"))
        || !Link(PanelExpandRainbow, PassiveRainbowExpandedVariableName, InitializePanel, TEXT("ExpandRainbow"))
        || !Link(PanelExpandLegend, PassiveLegendExpandedVariableName, InitializePanel, TEXT("ExpandLegend"))
        || !Link(PanelExpandGold3, PassiveGold3ExpandedVariableName, InitializePanel, TEXT("ExpandGold3"))
        || !Link(PanelExpandGold2, PassiveGold2ExpandedVariableName, InitializePanel, TEXT("ExpandGold2"))
        || !Link(PanelExpandNormal, PassiveNormalExpandedVariableName, InitializePanel, TEXT("ExpandNormal"))
        || !Link(PanelExpandNegative1, PassiveNegative1ExpandedVariableName, InitializePanel, TEXT("ExpandNegative1"))
        || !Link(PanelExpandNegative2, PassiveNegative2ExpandedVariableName, InitializePanel, TEXT("ExpandNegative2"))
        || !Link(PanelExpandNegative3, PassiveNegative3ExpandedVariableName, InitializePanel, TEXT("ExpandNegative3"))
         || !Link(InitializePanel, UEdGraphSchema_K2::PN_Then, InitializePageState, UEdGraphSchema_K2::PN_Execute)
         || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, InitializePageState, UEdGraphSchema_K2::PN_Self)
         || !Link(PanelMainPage, PanelMainPageVariableName, InitializePageState, TEXT("MainPage"))
         || !Link(PanelFilterPage, PanelFilterPageVariableName, InitializePageState, TEXT("FilterPage"))
         || !Link(InitializePageState, UEdGraphSchema_K2::PN_Then, InitializeLanguage, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, InitializeLanguage, UEdGraphSchema_K2::PN_Self)
        || !Link(PanelLanguage, LanguageIdVariableName, InitializeLanguage, TEXT("LanguageId"))
        || !Link(InitializeLanguage, UEdGraphSchema_K2::PN_Then, PopulatePassiveCatalog, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, PopulatePassiveCatalog, UEdGraphSchema_K2::PN_Self)
        || !Link(PopulatePassiveCatalog, UEdGraphSchema_K2::PN_Then, PopulatePalCatalog, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, PopulatePalCatalog, UEdGraphSchema_K2::PN_Self)
        || !Link(PopulatePalCatalog, UEdGraphSchema_K2::PN_Then, AddPanelToViewport, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, AddPanelToViewport, UEdGraphSchema_K2::PN_Self)
        || !Link(AddPanelToViewport, UEdGraphSchema_K2::PN_Then, ShowCursor, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, ShowCursor, UEdGraphSchema_K2::PN_Self)
        || !Link(ShowCursor, UEdGraphSchema_K2::PN_Then, UiOnly, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, UiOnly, TEXT("PlayerController"))
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, UiOnly, TEXT("InWidgetToFocus"))
        || !Link(UiOnly, UEdGraphSchema_K2::PN_Then, FocusPanel, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastPanel, CastPanel->GetCastResultPin()->PinName, FocusPanel, UEdGraphSchema_K2::PN_Self)
        || !Link(FocusPanel, UEdGraphSchema_K2::PN_Then, SetPalInputDisable, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOpenPalController, CastOpenPalController->GetCastResultPin()->PinName, SetPalInputDisable, UEdGraphSchema_K2::PN_Self)
        || !Link(SetPalInputDisable, UEdGraphSchema_K2::PN_Then, DisableGameInput, UEdGraphSchema_K2::PN_Execute)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, DisableGameInput, UEdGraphSchema_K2::PN_Self)
        || !Link(OpenController, UEdGraphSchema_K2::PN_ReturnValue, DisableGameInput, TEXT("PlayerController"))) {
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
    UK2Node_VariableGet* ExistingLanguageId = AddVariableGet(Graph, LanguageIdVariableName, 420, 120);
    UK2Node_VariableSet* StoreExistingLanguageId = AddExternalVariableSet(
        Graph, OverlayLanguageIdVariableName, OverlayClass, 700, 120);
    UK2Node_VariableGet* ExistingTopGuideEnabled = AddVariableGet(Graph, ShowTopGuideLineVariableName, 420, 360);
    UK2Node_VariableSet* StoreExistingTopGuideEnabled = AddExternalVariableSet(Graph, OverlayTopGuideEnabledVariableName, OverlayClass, 700, 360);
    UK2Node_VariableGet* ExistingShowName = AddVariableGet(Graph, ShowNameVariableName, 700, 200);
    UK2Node_VariableSet* StoreExistingShowName = AddExternalVariableSet(Graph, OverlayShowNameVariableName, OverlayClass, 980, 200);
    UK2Node_VariableGet* ExistingShowLevel = AddVariableGet(Graph, ShowLevelVariableName, 700, 280);
    UK2Node_VariableSet* StoreExistingShowLevel = AddExternalVariableSet(Graph, OverlayShowLevelVariableName, OverlayClass, 980, 360);
    UK2Node_VariableGet* ExistingShowDistance = AddVariableGet(Graph, ShowDistanceVariableName, 980, 280);
    UK2Node_VariableSet* StoreExistingShowDistance = AddExternalVariableSet(Graph, OverlayShowDistanceVariableName, OverlayClass, 1260, 360);
    UK2Node_VariableGet* ExistingShowIv = AddVariableGet(Graph, ShowIvVariableName, 1260, 200);
    UK2Node_VariableSet* StoreExistingShowIv = AddExternalVariableSet(Graph, OverlayShowIvVariableName, OverlayClass, 1540, 200);
    UK2Node_VariableGet* ExistingShowPassiveSkills = AddVariableGet(Graph, ShowPassiveSkillsVariableName, 1260, 120);
    UK2Node_VariableSet* StoreExistingShowPassiveSkills = AddExternalVariableSet(Graph, OverlayShowPassiveSkillsVariableName, OverlayClass, 1540, 120);
    UK2Node_VariableGet* ExistingIvMin = AddVariableGet(Graph, IvMinVariableName, 1540, 120);
    UK2Node_VariableSet* StoreExistingIvMin = AddExternalVariableSet(Graph, OverlayIvMinVariableName, OverlayClass, 1820, 120);
    UK2Node_VariableGet* ExistingIvHpMin = AddVariableGet(Graph, IvHpMinVariableName, 1820, 40);
    UK2Node_VariableSet* StoreExistingIvHpMin = AddExternalVariableSet(Graph, OverlayIvHpMinVariableName, OverlayClass, 2100, 40);
    UK2Node_VariableGet* ExistingIvAttackMin = AddVariableGet(Graph, IvAttackMinVariableName, 2100, -40);
    UK2Node_VariableSet* StoreExistingIvAttackMin = AddExternalVariableSet(Graph, OverlayIvAttackMinVariableName, OverlayClass, 2380, -40);
    UK2Node_VariableGet* ExistingIvDefenseMin = AddVariableGet(Graph, IvDefenseMinVariableName, 2380, -120);
    UK2Node_VariableSet* StoreExistingIvDefenseMin = AddExternalVariableSet(Graph, OverlayIvDefenseMinVariableName, OverlayClass, 2660, -120);
    UK2Node_VariableGet* ExistingPassiveFilterIds = AddVariableGet(Graph, PassiveFilterIdsVariableName, 2660, -200);
    UK2Node_VariableSet* StoreExistingPassiveFilterIds = AddExternalVariableSet(Graph, OverlayPassiveFilterIdsVariableName, OverlayClass, 2940, -200);
    UK2Node_VariableGet* ExistingPassiveExcludeIds = AddVariableGet(Graph, PassiveExcludeIdsVariableName, 2940, -280);
    UK2Node_VariableSet* StoreExistingPassiveExcludeIds = AddExternalVariableSet(Graph, OverlayPassiveExcludeIdsVariableName, OverlayClass, 3220, -280);
    UK2Node_VariableGet* ExistingSpeciesFilterIds = AddVariableGet(Graph, SpeciesFilterIdsVariableName, 3220, -360);
    UK2Node_VariableSet* StoreExistingSpeciesFilterIds = AddExternalVariableSet(
        Graph, OverlaySpeciesFilterIdsVariableName, OverlayClass, 3500, -360);
    UK2Node_VariableGet* ExistingGenderFilter = AddVariableGet(Graph, GenderFilterIdVariableName, 1260, 280);
    UK2Node_VariableSet* StoreExistingGenderFilter = AddExternalVariableSet(Graph, OverlayGenderFilterIdVariableName, OverlayClass, 1540, 360);
    UK2Node_VariableGet* ExistingLuckyFilter = AddVariableGet(Graph, LuckyFilterIdVariableName, 1540, 280);
    UK2Node_VariableSet* StoreExistingLuckyFilter = AddExternalVariableSet(Graph, OverlayLuckyFilterIdVariableName, OverlayClass, 1820, 360);
    UK2Node_VariableGet* ExistingBossFilter = AddVariableGet(Graph, BossFilterIdVariableName, 1820, 280);
    UK2Node_VariableSet* StoreExistingBossFilter = AddExternalVariableSet(Graph, OverlayBossFilterIdVariableName, OverlayClass, 2100, 360);
    UK2Node_VariableGet* ExistingCollectionFilter = AddVariableGet(Graph, CollectionFilterIdVariableName, 1820, 440);
    UK2Node_VariableSet* StoreExistingCollectionFilter = AddExternalVariableSet(Graph, OverlayCollectionFilterIdVariableName, OverlayClass, 2100, 440);
    UK2Node_VariableGet* ExistingElementFilter = AddVariableGet(Graph, OverlayElementFilterMaskVariableName, 2100, 280);
    UK2Node_VariableSet* StoreExistingElementFilter = AddExternalVariableSet(Graph, OverlayElementFilterMaskVariableName, OverlayClass, 2380, 360);
    UK2Node_VariableGet* ExistingGenderDiagnostic = AddExternalVariableGet(Graph, OverlayGenderDiagnosticVariableName, OverlayClass, 420, 520);
    UK2Node_VariableSet* StoreExistingGenderDiagnostic = AddVariableSet(Graph, BridgeGenderDiagnosticVariableName, 700, 520);
    UK2Node_VariableGet* ExistingGenderDiagnosticCode = AddExternalVariableGet(Graph, OverlayGenderDiagnosticCodeVariableName, OverlayClass, 420, 640);
    UK2Node_VariableSet* StoreExistingGenderDiagnosticCode = AddVariableSet(Graph, BridgeGenderDiagnosticCodeVariableName, 980, 520);
    UK2Node_VariableGet* NewLanguageId = AddVariableGet(Graph, LanguageIdVariableName, 1540, 320);
    UK2Node_VariableSet* StoreNewLanguageId = AddExternalVariableSet(
        Graph, OverlayLanguageIdVariableName, OverlayClass, 1820, 320);
    UK2Node_VariableGet* NewTopGuideEnabled = AddVariableGet(Graph, ShowTopGuideLineVariableName, 1540, 560);
    UK2Node_VariableSet* StoreNewTopGuideEnabled = AddExternalVariableSet(Graph, OverlayTopGuideEnabledVariableName, OverlayClass, 1820, 560);
    UK2Node_VariableGet* NewShowName = AddVariableGet(Graph, ShowNameVariableName, 1820, 400);
    UK2Node_VariableSet* StoreNewShowName = AddExternalVariableSet(Graph, OverlayShowNameVariableName, OverlayClass, 2100, 400);
    UK2Node_VariableGet* NewShowLevel = AddVariableGet(Graph, ShowLevelVariableName, 1820, 480);
    UK2Node_VariableSet* StoreNewShowLevel = AddExternalVariableSet(Graph, OverlayShowLevelVariableName, OverlayClass, 2100, 560);
    UK2Node_VariableGet* NewShowDistance = AddVariableGet(Graph, ShowDistanceVariableName, 2100, 480);
    UK2Node_VariableSet* StoreNewShowDistance = AddExternalVariableSet(Graph, OverlayShowDistanceVariableName, OverlayClass, 2380, 560);
    UK2Node_VariableGet* NewShowIv = AddVariableGet(Graph, ShowIvVariableName, 2380, 400);
    UK2Node_VariableSet* StoreNewShowIv = AddExternalVariableSet(Graph, OverlayShowIvVariableName, OverlayClass, 2660, 400);
    UK2Node_VariableGet* NewShowPassiveSkills = AddVariableGet(Graph, ShowPassiveSkillsVariableName, 2380, 320);
    UK2Node_VariableSet* StoreNewShowPassiveSkills = AddExternalVariableSet(Graph, OverlayShowPassiveSkillsVariableName, OverlayClass, 2660, 320);
    UK2Node_VariableGet* NewIvMin = AddVariableGet(Graph, IvMinVariableName, 2660, 320);
    UK2Node_VariableSet* StoreNewIvMin = AddExternalVariableSet(Graph, OverlayIvMinVariableName, OverlayClass, 2940, 320);
    UK2Node_VariableGet* NewIvHpMin = AddVariableGet(Graph, IvHpMinVariableName, 2940, 240);
    UK2Node_VariableSet* StoreNewIvHpMin = AddExternalVariableSet(Graph, OverlayIvHpMinVariableName, OverlayClass, 3220, 240);
    UK2Node_VariableGet* NewIvAttackMin = AddVariableGet(Graph, IvAttackMinVariableName, 3220, 160);
    UK2Node_VariableSet* StoreNewIvAttackMin = AddExternalVariableSet(Graph, OverlayIvAttackMinVariableName, OverlayClass, 3500, 160);
    UK2Node_VariableGet* NewIvDefenseMin = AddVariableGet(Graph, IvDefenseMinVariableName, 3500, 80);
    UK2Node_VariableSet* StoreNewIvDefenseMin = AddExternalVariableSet(Graph, OverlayIvDefenseMinVariableName, OverlayClass, 3780, 80);
    UK2Node_VariableGet* NewPassiveFilterIds = AddVariableGet(Graph, PassiveFilterIdsVariableName, 3780, 0);
    UK2Node_VariableSet* StoreNewPassiveFilterIds = AddExternalVariableSet(Graph, OverlayPassiveFilterIdsVariableName, OverlayClass, 4060, 0);
    UK2Node_VariableGet* NewPassiveExcludeIds = AddVariableGet(Graph, PassiveExcludeIdsVariableName, 4060, -80);
    UK2Node_VariableSet* StoreNewPassiveExcludeIds = AddExternalVariableSet(Graph, OverlayPassiveExcludeIdsVariableName, OverlayClass, 4340, -80);
    UK2Node_VariableGet* NewSpeciesFilterIds = AddVariableGet(Graph, SpeciesFilterIdsVariableName, 4340, -160);
    UK2Node_VariableSet* StoreNewSpeciesFilterIds = AddExternalVariableSet(
        Graph, OverlaySpeciesFilterIdsVariableName, OverlayClass, 4620, -160);
    UK2Node_VariableGet* NewGenderFilter = AddVariableGet(Graph, GenderFilterIdVariableName, 2380, 480);
    UK2Node_VariableSet* StoreNewGenderFilter = AddExternalVariableSet(Graph, OverlayGenderFilterIdVariableName, OverlayClass, 2660, 560);
    UK2Node_VariableGet* NewLuckyFilter = AddVariableGet(Graph, LuckyFilterIdVariableName, 2660, 480);
    UK2Node_VariableSet* StoreNewLuckyFilter = AddExternalVariableSet(Graph, OverlayLuckyFilterIdVariableName, OverlayClass, 2940, 560);
    UK2Node_VariableGet* NewBossFilter = AddVariableGet(Graph, BossFilterIdVariableName, 2940, 480);
    UK2Node_VariableSet* StoreNewBossFilter = AddExternalVariableSet(Graph, OverlayBossFilterIdVariableName, OverlayClass, 3220, 560);
    UK2Node_VariableGet* NewCollectionFilter = AddVariableGet(Graph, CollectionFilterIdVariableName, 2940, 640);
    UK2Node_VariableSet* StoreNewCollectionFilter = AddExternalVariableSet(Graph, OverlayCollectionFilterIdVariableName, OverlayClass, 3220, 640);
    UK2Node_VariableGet* NewElementFilter = AddVariableGet(Graph, OverlayElementFilterMaskVariableName, 3220, 480);
    UK2Node_VariableSet* StoreNewElementFilter = AddExternalVariableSet(Graph, OverlayElementFilterMaskVariableName, OverlayClass, 3500, 560);
    UK2Node_VariableGet* NewGenderDiagnostic = AddExternalVariableGet(Graph, OverlayGenderDiagnosticVariableName, OverlayClass, 1540, 720);
    UK2Node_VariableSet* StoreNewGenderDiagnostic = AddVariableSet(Graph, BridgeGenderDiagnosticVariableName, 1820, 720);
    UK2Node_VariableGet* NewGenderDiagnosticCode = AddExternalVariableGet(Graph, OverlayGenderDiagnosticCodeVariableName, OverlayClass, 1540, 880);
    UK2Node_VariableSet* StoreNewGenderDiagnosticCode = AddVariableSet(Graph, BridgeGenderDiagnosticCodeVariableName, 2100, 720);
    if (!OverlayGet || !OverlayValid || !OverlayBranch || !CreateWidget || !CastOverlay || !StoreOverlay || !AddToViewport
        || !AddExistingTarget || !AddNewTarget
        || !ExistingLanguageId || !StoreExistingLanguageId || !ExistingTopGuideEnabled || !StoreExistingTopGuideEnabled
        || !ExistingShowName || !StoreExistingShowName
        || !ExistingShowLevel || !StoreExistingShowLevel || !ExistingShowDistance || !StoreExistingShowDistance
        || !ExistingShowIv || !StoreExistingShowIv || !ExistingShowPassiveSkills || !StoreExistingShowPassiveSkills
        || !ExistingIvMin || !StoreExistingIvMin
        || !ExistingIvHpMin || !StoreExistingIvHpMin || !ExistingIvAttackMin || !StoreExistingIvAttackMin
        || !ExistingIvDefenseMin || !StoreExistingIvDefenseMin
        || !ExistingPassiveFilterIds || !StoreExistingPassiveFilterIds
        || !ExistingPassiveExcludeIds || !StoreExistingPassiveExcludeIds
        || !ExistingSpeciesFilterIds || !StoreExistingSpeciesFilterIds
        || !ExistingGenderFilter || !StoreExistingGenderFilter || !ExistingLuckyFilter || !StoreExistingLuckyFilter
        || !ExistingBossFilter || !StoreExistingBossFilter || !ExistingCollectionFilter || !StoreExistingCollectionFilter
        || !ExistingElementFilter || !StoreExistingElementFilter
        || !ExistingGenderDiagnostic || !StoreExistingGenderDiagnostic
        || !ExistingGenderDiagnosticCode || !StoreExistingGenderDiagnosticCode
        || !NewLanguageId || !StoreNewLanguageId || !NewTopGuideEnabled || !StoreNewTopGuideEnabled
        || !NewShowName || !StoreNewShowName
        || !NewShowLevel || !StoreNewShowLevel
        || !NewShowDistance || !StoreNewShowDistance || !NewShowIv || !StoreNewShowIv
        || !NewShowPassiveSkills || !StoreNewShowPassiveSkills || !NewIvMin || !StoreNewIvMin
        || !NewIvHpMin || !StoreNewIvHpMin || !NewIvAttackMin || !StoreNewIvAttackMin
        || !NewIvDefenseMin || !StoreNewIvDefenseMin || !NewPassiveFilterIds || !StoreNewPassiveFilterIds
        || !NewPassiveExcludeIds || !StoreNewPassiveExcludeIds
        || !NewSpeciesFilterIds || !StoreNewSpeciesFilterIds
        || !NewGenderFilter || !StoreNewGenderFilter
        || !NewLuckyFilter || !StoreNewLuckyFilter || !NewBossFilter || !StoreNewBossFilter
        || !NewCollectionFilter || !StoreNewCollectionFilter
        || !NewElementFilter || !StoreNewElementFilter
        || !NewGenderDiagnostic || !StoreNewGenderDiagnostic
        || !NewGenderDiagnosticCode || !StoreNewGenderDiagnosticCode
        || !SetClassPin(CreateWidget, TEXT("WidgetType"), OverlayClass)
        || !Link(SetTarget, UEdGraphSchema_K2::PN_Then, OverlayBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, OverlayValid, TEXT("Object"))
        || !Link(OverlayValid, UEdGraphSchema_K2::PN_ReturnValue, OverlayBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(OverlayBranch, UEdGraphSchema_K2::PN_Then, AddExistingTarget, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, AddExistingTarget, UEdGraphSchema_K2::PN_Self)
        || !Link(SetTarget, TEXT("Target"), AddExistingTarget, TEXT("Target"))
        || !Link(SetTarget, TEXT("Level"), AddExistingTarget, TEXT("Level"))
        || !Link(SetTarget, TEXT("DistanceMeters"), AddExistingTarget, TEXT("DistanceMeters"))
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
        || !Link(SetTarget, TEXT("Level"), AddNewTarget, TEXT("Level"))
        || !Link(SetTarget, TEXT("DistanceMeters"), AddNewTarget, TEXT("DistanceMeters"))
        || !Link(AddExistingTarget, UEdGraphSchema_K2::PN_Then, StoreExistingLanguageId, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingLanguageId, LanguageIdVariableName, StoreExistingLanguageId, OverlayLanguageIdVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingLanguageId, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingLanguageId, UEdGraphSchema_K2::PN_Then, StoreExistingTopGuideEnabled, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingTopGuideEnabled, ShowTopGuideLineVariableName, StoreExistingTopGuideEnabled, OverlayTopGuideEnabledVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingTopGuideEnabled, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingTopGuideEnabled, UEdGraphSchema_K2::PN_Then, StoreExistingShowName, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingShowName, ShowNameVariableName, StoreExistingShowName, OverlayShowNameVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingShowName, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingShowName, UEdGraphSchema_K2::PN_Then, StoreExistingShowLevel, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingShowLevel, ShowLevelVariableName, StoreExistingShowLevel, OverlayShowLevelVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingShowLevel, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingShowLevel, UEdGraphSchema_K2::PN_Then, StoreExistingShowDistance, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingShowDistance, ShowDistanceVariableName, StoreExistingShowDistance, OverlayShowDistanceVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingShowDistance, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingShowDistance, UEdGraphSchema_K2::PN_Then, StoreExistingShowIv, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingShowIv, ShowIvVariableName, StoreExistingShowIv, OverlayShowIvVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingShowIv, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingShowIv, UEdGraphSchema_K2::PN_Then, StoreExistingShowPassiveSkills, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingShowPassiveSkills, ShowPassiveSkillsVariableName, StoreExistingShowPassiveSkills, OverlayShowPassiveSkillsVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingShowPassiveSkills, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingShowPassiveSkills, UEdGraphSchema_K2::PN_Then, StoreExistingIvMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingIvMin, IvMinVariableName, StoreExistingIvMin, OverlayIvMinVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingIvMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingIvMin, UEdGraphSchema_K2::PN_Then, StoreExistingIvHpMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingIvHpMin, IvHpMinVariableName, StoreExistingIvHpMin, OverlayIvHpMinVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingIvHpMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingIvHpMin, UEdGraphSchema_K2::PN_Then, StoreExistingIvAttackMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingIvAttackMin, IvAttackMinVariableName, StoreExistingIvAttackMin, OverlayIvAttackMinVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingIvAttackMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingIvAttackMin, UEdGraphSchema_K2::PN_Then, StoreExistingIvDefenseMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingIvDefenseMin, IvDefenseMinVariableName, StoreExistingIvDefenseMin, OverlayIvDefenseMinVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingIvDefenseMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingIvDefenseMin, UEdGraphSchema_K2::PN_Then, StoreExistingPassiveFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingPassiveFilterIds, PassiveFilterIdsVariableName, StoreExistingPassiveFilterIds, OverlayPassiveFilterIdsVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingPassiveFilterIds, UEdGraphSchema_K2::PN_Self)
        // __DEPRECATED_20260718__ [reason: exclusion IDs must reach an existing Overlay before scalar filters]
        // || !Link(StoreExistingPassiveFilterIds, UEdGraphSchema_K2::PN_Then, StoreExistingGenderFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(StoreExistingPassiveFilterIds, UEdGraphSchema_K2::PN_Then, StoreExistingPassiveExcludeIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingPassiveExcludeIds, PassiveExcludeIdsVariableName, StoreExistingPassiveExcludeIds, OverlayPassiveExcludeIdsVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingPassiveExcludeIds, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingPassiveExcludeIds, UEdGraphSchema_K2::PN_Then, StoreExistingSpeciesFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingSpeciesFilterIds, SpeciesFilterIdsVariableName, StoreExistingSpeciesFilterIds, OverlaySpeciesFilterIdsVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingSpeciesFilterIds, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingSpeciesFilterIds, UEdGraphSchema_K2::PN_Then, StoreExistingGenderFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingGenderFilter, GenderFilterIdVariableName, StoreExistingGenderFilter, OverlayGenderFilterIdVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingGenderFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingGenderFilter, UEdGraphSchema_K2::PN_Then, StoreExistingLuckyFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingLuckyFilter, LuckyFilterIdVariableName, StoreExistingLuckyFilter, OverlayLuckyFilterIdVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingLuckyFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingLuckyFilter, UEdGraphSchema_K2::PN_Then, StoreExistingBossFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingBossFilter, BossFilterIdVariableName, StoreExistingBossFilter, OverlayBossFilterIdVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingBossFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingBossFilter, UEdGraphSchema_K2::PN_Then, StoreExistingCollectionFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingCollectionFilter, CollectionFilterIdVariableName, StoreExistingCollectionFilter, OverlayCollectionFilterIdVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingCollectionFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingCollectionFilter, UEdGraphSchema_K2::PN_Then, StoreExistingElementFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(ExistingElementFilter, OverlayElementFilterMaskVariableName, StoreExistingElementFilter, OverlayElementFilterMaskVariableName)
        || !Link(OverlayGet, OverlayVariableName, StoreExistingElementFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreExistingElementFilter, UEdGraphSchema_K2::PN_Then, StoreExistingGenderDiagnostic, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, ExistingGenderDiagnostic, UEdGraphSchema_K2::PN_Self)
        || !Link(ExistingGenderDiagnostic, OverlayGenderDiagnosticVariableName, StoreExistingGenderDiagnostic, BridgeGenderDiagnosticVariableName)
        || !Link(StoreExistingGenderDiagnostic, UEdGraphSchema_K2::PN_Then, StoreExistingGenderDiagnosticCode, UEdGraphSchema_K2::PN_Execute)
        || !Link(OverlayGet, OverlayVariableName, ExistingGenderDiagnosticCode, UEdGraphSchema_K2::PN_Self)
        || !Link(ExistingGenderDiagnosticCode, OverlayGenderDiagnosticCodeVariableName, StoreExistingGenderDiagnosticCode, BridgeGenderDiagnosticCodeVariableName)
        || !Link(AddNewTarget, UEdGraphSchema_K2::PN_Then, StoreNewLanguageId, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewLanguageId, LanguageIdVariableName, StoreNewLanguageId, OverlayLanguageIdVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewLanguageId, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewLanguageId, UEdGraphSchema_K2::PN_Then, StoreNewTopGuideEnabled, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewTopGuideEnabled, ShowTopGuideLineVariableName, StoreNewTopGuideEnabled, OverlayTopGuideEnabledVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewTopGuideEnabled, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewTopGuideEnabled, UEdGraphSchema_K2::PN_Then, StoreNewShowName, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewShowName, ShowNameVariableName, StoreNewShowName, OverlayShowNameVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewShowName, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewShowName, UEdGraphSchema_K2::PN_Then, StoreNewShowLevel, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewShowLevel, ShowLevelVariableName, StoreNewShowLevel, OverlayShowLevelVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewShowLevel, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewShowLevel, UEdGraphSchema_K2::PN_Then, StoreNewShowDistance, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewShowDistance, ShowDistanceVariableName, StoreNewShowDistance, OverlayShowDistanceVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewShowDistance, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewShowDistance, UEdGraphSchema_K2::PN_Then, StoreNewShowIv, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewShowIv, ShowIvVariableName, StoreNewShowIv, OverlayShowIvVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewShowIv, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewShowIv, UEdGraphSchema_K2::PN_Then, StoreNewShowPassiveSkills, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewShowPassiveSkills, ShowPassiveSkillsVariableName, StoreNewShowPassiveSkills, OverlayShowPassiveSkillsVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewShowPassiveSkills, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewShowPassiveSkills, UEdGraphSchema_K2::PN_Then, StoreNewIvMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewIvMin, IvMinVariableName, StoreNewIvMin, OverlayIvMinVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewIvMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewIvMin, UEdGraphSchema_K2::PN_Then, StoreNewIvHpMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewIvHpMin, IvHpMinVariableName, StoreNewIvHpMin, OverlayIvHpMinVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewIvHpMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewIvHpMin, UEdGraphSchema_K2::PN_Then, StoreNewIvAttackMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewIvAttackMin, IvAttackMinVariableName, StoreNewIvAttackMin, OverlayIvAttackMinVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewIvAttackMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewIvAttackMin, UEdGraphSchema_K2::PN_Then, StoreNewIvDefenseMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewIvDefenseMin, IvDefenseMinVariableName, StoreNewIvDefenseMin, OverlayIvDefenseMinVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewIvDefenseMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewIvDefenseMin, UEdGraphSchema_K2::PN_Then, StoreNewPassiveFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewPassiveFilterIds, PassiveFilterIdsVariableName, StoreNewPassiveFilterIds, OverlayPassiveFilterIdsVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewPassiveFilterIds, UEdGraphSchema_K2::PN_Self)
        // __DEPRECATED_20260718__ [reason: exclusion IDs must reach a newly-created Overlay before scalar filters]
        // || !Link(StoreNewPassiveFilterIds, UEdGraphSchema_K2::PN_Then, StoreNewGenderFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(StoreNewPassiveFilterIds, UEdGraphSchema_K2::PN_Then, StoreNewPassiveExcludeIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewPassiveExcludeIds, PassiveExcludeIdsVariableName, StoreNewPassiveExcludeIds, OverlayPassiveExcludeIdsVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewPassiveExcludeIds, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewPassiveExcludeIds, UEdGraphSchema_K2::PN_Then, StoreNewSpeciesFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewSpeciesFilterIds, SpeciesFilterIdsVariableName, StoreNewSpeciesFilterIds, OverlaySpeciesFilterIdsVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewSpeciesFilterIds, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewSpeciesFilterIds, UEdGraphSchema_K2::PN_Then, StoreNewGenderFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewGenderFilter, GenderFilterIdVariableName, StoreNewGenderFilter, OverlayGenderFilterIdVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewGenderFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewGenderFilter, UEdGraphSchema_K2::PN_Then, StoreNewLuckyFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewLuckyFilter, LuckyFilterIdVariableName, StoreNewLuckyFilter, OverlayLuckyFilterIdVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewLuckyFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewLuckyFilter, UEdGraphSchema_K2::PN_Then, StoreNewBossFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewBossFilter, BossFilterIdVariableName, StoreNewBossFilter, OverlayBossFilterIdVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewBossFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewBossFilter, UEdGraphSchema_K2::PN_Then, StoreNewCollectionFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewCollectionFilter, CollectionFilterIdVariableName, StoreNewCollectionFilter, OverlayCollectionFilterIdVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewCollectionFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewCollectionFilter, UEdGraphSchema_K2::PN_Then, StoreNewElementFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(NewElementFilter, OverlayElementFilterMaskVariableName, StoreNewElementFilter, OverlayElementFilterMaskVariableName)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, StoreNewElementFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StoreNewElementFilter, UEdGraphSchema_K2::PN_Then, StoreNewGenderDiagnostic, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, NewGenderDiagnostic, UEdGraphSchema_K2::PN_Self)
        || !Link(NewGenderDiagnostic, OverlayGenderDiagnosticVariableName, StoreNewGenderDiagnostic, BridgeGenderDiagnosticVariableName)
        || !Link(StoreNewGenderDiagnostic, UEdGraphSchema_K2::PN_Then, StoreNewGenderDiagnosticCode, UEdGraphSchema_K2::PN_Execute)
        || !Link(CastOverlay, CastOverlay->GetCastResultPin()->PinName, NewGenderDiagnosticCode, UEdGraphSchema_K2::PN_Self)
        || !Link(NewGenderDiagnosticCode, OverlayGenderDiagnosticCodeVariableName, StoreNewGenderDiagnosticCode, BridgeGenderDiagnosticCodeVariableName)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor SetTarget overlay graph failed"));
        return false;
    }

    UK2Node_VariableGet* StyleOverlayGet = AddVariableGet(Graph, OverlayVariableName, -620, 2440);
    UK2Node_CallFunction* StyleOverlayValid = AddStaticCall(Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -380, 2440);
    UK2Node_IfThenElse* StyleOverlayBranch = AddBranch(Graph, -120, 2320);
    UK2Node_VariableSet* StyleTopGuide = AddExternalVariableSet(Graph, OverlayTopGuideEnabledVariableName, OverlayClass, 140, 2320);
    UK2Node_VariableSet* StyleLanguage = AddExternalVariableSet(Graph, OverlayLanguageIdVariableName, OverlayClass, 280, 2400);
    UK2Node_VariableSet* StyleName = AddExternalVariableSet(Graph, OverlayShowNameVariableName, OverlayClass, 420, 2320);
    UK2Node_VariableSet* StyleLevel = AddExternalVariableSet(Graph, OverlayShowLevelVariableName, OverlayClass, 700, 2320);
    UK2Node_VariableSet* StyleDistance = AddExternalVariableSet(Graph, OverlayShowDistanceVariableName, OverlayClass, 980, 2320);
    UK2Node_VariableSet* StyleIv = AddExternalVariableSet(Graph, OverlayShowIvVariableName, OverlayClass, 1260, 2320);
    UK2Node_VariableSet* StylePassiveSkills = AddExternalVariableSet(Graph, OverlayShowPassiveSkillsVariableName, OverlayClass, 1400, 2240);
    UK2Node_VariableSet* StyleIvMin = AddExternalVariableSet(Graph, OverlayIvMinVariableName, OverlayClass, 1540, 2240);
    UK2Node_VariableSet* StyleIvHpMin = AddExternalVariableSet(Graph, OverlayIvHpMinVariableName, OverlayClass, 1680, 2160);
    UK2Node_VariableSet* StyleIvAttackMin = AddExternalVariableSet(Graph, OverlayIvAttackMinVariableName, OverlayClass, 1960, 2160);
    UK2Node_VariableSet* StyleIvDefenseMin = AddExternalVariableSet(Graph, OverlayIvDefenseMinVariableName, OverlayClass, 2240, 2160);
    UK2Node_VariableGet* StylePassiveFilterIdsGet = AddVariableGet(Graph, PassiveFilterIdsVariableName, 2520, 2080);
    UK2Node_VariableSet* StylePassiveFilterIds = AddExternalVariableSet(Graph, OverlayPassiveFilterIdsVariableName, OverlayClass, 2520, 2160);
    UK2Node_VariableGet* StylePassiveExcludeIdsGet = AddVariableGet(Graph, PassiveExcludeIdsVariableName, 2800, 2080);
    UK2Node_VariableSet* StylePassiveExcludeIds = AddExternalVariableSet(Graph, OverlayPassiveExcludeIdsVariableName, OverlayClass, 2800, 2160);
    UK2Node_VariableGet* StyleSpeciesFilterIdsGet = AddVariableGet(Graph, SpeciesFilterIdsVariableName, 3080, 2080);
    UK2Node_VariableSet* StyleSpeciesFilterIds = AddExternalVariableSet(
        Graph, OverlaySpeciesFilterIdsVariableName, OverlayClass, 3080, 2160);
    UK2Node_VariableSet* StyleGenderFilter = AddExternalVariableSet(Graph, OverlayGenderFilterIdVariableName, OverlayClass, 1540, 2320);
    UK2Node_VariableSet* StyleLuckyFilter = AddExternalVariableSet(Graph, OverlayLuckyFilterIdVariableName, OverlayClass, 1820, 2320);
    UK2Node_VariableSet* StyleBossFilter = AddExternalVariableSet(Graph, OverlayBossFilterIdVariableName, OverlayClass, 2100, 2320);
    UK2Node_VariableSet* StyleCollectionFilter = AddExternalVariableSet(Graph, OverlayCollectionFilterIdVariableName, OverlayClass, 2240, 2400);
    UK2Node_VariableSet* StyleElementFilter = AddExternalVariableSet(Graph, OverlayElementFilterMaskVariableName, OverlayClass, 2380, 2320);
    if (!StyleOverlayGet || !StyleOverlayValid || !StyleOverlayBranch || !StyleTopGuide || !StyleLanguage
        || !StyleName || !StyleLevel || !StyleDistance
        || !StyleIv || !StylePassiveSkills || !StyleIvMin || !StyleIvHpMin || !StyleIvAttackMin || !StyleIvDefenseMin
        || !StylePassiveFilterIdsGet || !StylePassiveFilterIds || !StylePassiveExcludeIdsGet || !StylePassiveExcludeIds
        || !StyleSpeciesFilterIdsGet || !StyleSpeciesFilterIds
        || !StyleGenderFilter || !StyleLuckyFilter || !StyleBossFilter || !StyleCollectionFilter || !StyleElementFilter
        || !Link(SetDisplayStyle, UEdGraphSchema_K2::PN_Then, StyleOverlayBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleOverlayValid, TEXT("Object"))
        || !Link(StyleOverlayValid, UEdGraphSchema_K2::PN_ReturnValue, StyleOverlayBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(StyleOverlayBranch, UEdGraphSchema_K2::PN_Then, StyleTopGuide, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ShowTopGuideLine"), StyleTopGuide, OverlayTopGuideEnabledVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleTopGuide, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleTopGuide, UEdGraphSchema_K2::PN_Then, StyleLanguage, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("LanguageId"), StyleLanguage, OverlayLanguageIdVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleLanguage, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleLanguage, UEdGraphSchema_K2::PN_Then, StyleName, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ShowName"), StyleName, OverlayShowNameVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleName, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleName, UEdGraphSchema_K2::PN_Then, StyleLevel, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ShowLevel"), StyleLevel, OverlayShowLevelVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleLevel, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleLevel, UEdGraphSchema_K2::PN_Then, StyleDistance, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ShowDistance"), StyleDistance, OverlayShowDistanceVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleDistance, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleDistance, UEdGraphSchema_K2::PN_Then, StyleIv, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ShowIV"), StyleIv, OverlayShowIvVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleIv, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleIv, UEdGraphSchema_K2::PN_Then, StylePassiveSkills, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ShowPassiveSkills"), StylePassiveSkills, OverlayShowPassiveSkillsVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StylePassiveSkills, UEdGraphSchema_K2::PN_Self)
        || !Link(StylePassiveSkills, UEdGraphSchema_K2::PN_Then, StyleIvMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("IvMin"), StyleIvMin, OverlayIvMinVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleIvMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleIvMin, UEdGraphSchema_K2::PN_Then, StyleIvHpMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("IvHpMin"), StyleIvHpMin, OverlayIvHpMinVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleIvHpMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleIvHpMin, UEdGraphSchema_K2::PN_Then, StyleIvAttackMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("IvAttackMin"), StyleIvAttackMin, OverlayIvAttackMinVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleIvAttackMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleIvAttackMin, UEdGraphSchema_K2::PN_Then, StyleIvDefenseMin, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("IvDefenseMin"), StyleIvDefenseMin, OverlayIvDefenseMinVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleIvDefenseMin, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleIvDefenseMin, UEdGraphSchema_K2::PN_Then, StylePassiveFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(StylePassiveFilterIdsGet, PassiveFilterIdsVariableName, StylePassiveFilterIds, OverlayPassiveFilterIdsVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StylePassiveFilterIds, UEdGraphSchema_K2::PN_Self)
        // __DEPRECATED_20260718__ [reason: actor-free style synchronization also carries exclusion IDs]
        // || !Link(StylePassiveFilterIds, UEdGraphSchema_K2::PN_Then, StyleGenderFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(StylePassiveFilterIds, UEdGraphSchema_K2::PN_Then, StylePassiveExcludeIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(StylePassiveExcludeIdsGet, PassiveExcludeIdsVariableName, StylePassiveExcludeIds, OverlayPassiveExcludeIdsVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StylePassiveExcludeIds, UEdGraphSchema_K2::PN_Self)
        || !Link(StylePassiveExcludeIds, UEdGraphSchema_K2::PN_Then, StyleSpeciesFilterIds, UEdGraphSchema_K2::PN_Execute)
        || !Link(StyleSpeciesFilterIdsGet, SpeciesFilterIdsVariableName, StyleSpeciesFilterIds, OverlaySpeciesFilterIdsVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleSpeciesFilterIds, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleSpeciesFilterIds, UEdGraphSchema_K2::PN_Then, StyleGenderFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("GenderFilterId"), StyleGenderFilter, OverlayGenderFilterIdVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleGenderFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleGenderFilter, UEdGraphSchema_K2::PN_Then, StyleLuckyFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("LuckyFilterId"), StyleLuckyFilter, OverlayLuckyFilterIdVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleLuckyFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleLuckyFilter, UEdGraphSchema_K2::PN_Then, StyleBossFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("BossFilterId"), StyleBossFilter, OverlayBossFilterIdVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleBossFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleBossFilter, UEdGraphSchema_K2::PN_Then, StyleCollectionFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("CollectionFilterId"), StyleCollectionFilter, OverlayCollectionFilterIdVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleCollectionFilter, UEdGraphSchema_K2::PN_Self)
        || !Link(StyleCollectionFilter, UEdGraphSchema_K2::PN_Then, StyleElementFilter, UEdGraphSchema_K2::PN_Execute)
        || !Link(SetDisplayStyle, TEXT("ElementFilterMask"), StyleElementFilter, OverlayElementFilterMaskVariableName)
        || !Link(StyleOverlayGet, OverlayVariableName, StyleElementFilter, UEdGraphSchema_K2::PN_Self)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor display style graph failed"));
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

    UK2Node_VariableGet* RemoveOverlayGet = AddVariableGet(Graph, OverlayVariableName, -620, 1520);
    UK2Node_CallFunction* RemoveOverlayValid = AddStaticCall(
        Graph, UKismetSystemLibrary::StaticClass(), TEXT("IsValid"), -380, 1520);
    UK2Node_IfThenElse* RemoveOverlayBranch = AddBranch(Graph, -120, 1400);
    UK2Node_CallFunction* RemoveOverlayTarget = AddStaticCall(
        Graph, OverlayClass, *OverlayRemoveTargetEventName.ToString(), 140, 1400);
    if (!RemoveOverlayGet || !RemoveOverlayValid || !RemoveOverlayBranch || !RemoveOverlayTarget
        || !Link(RemoveTarget, UEdGraphSchema_K2::PN_Then, RemoveOverlayBranch, UEdGraphSchema_K2::PN_Execute)
        || !Link(RemoveOverlayGet, OverlayVariableName, RemoveOverlayValid, TEXT("Object"))
        || !Link(RemoveOverlayValid, UEdGraphSchema_K2::PN_ReturnValue, RemoveOverlayBranch, UEdGraphSchema_K2::PN_Condition)
        || !Link(RemoveOverlayBranch, UEdGraphSchema_K2::PN_Then, RemoveOverlayTarget, UEdGraphSchema_K2::PN_Execute)
        || !Link(RemoveOverlayGet, OverlayVariableName, RemoveOverlayTarget, UEdGraphSchema_K2::PN_Self)
        || !Link(RemoveTarget, TEXT("Target"), RemoveOverlayTarget, TEXT("Target"))) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] BuildModActor target removal graph failed"));
        return false;
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor compiling nodes_after=%d"), Graph->Nodes.Num());
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] BuildModActor compile status=%d"), static_cast<int32>(Blueprint->Status));
    return Blueprint->Status != BS_Error;
}

} // namespace

bool UESPBlueprintAutomationLibrary::BuildPalworldResourceESPRichTextAssets() {
    UWidgetBlueprint* PassiveTooltip = LoadObject<UWidgetBlueprint>(
        nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPassiveTooltip"));
    if (!PassiveTooltip) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text tooltip-only dependency missing"));
        return false;
    }
    if (!BuildPassiveTooltip(PassiveTooltip)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text tooltip-only build failed"));
        return false;
    }
    PassiveTooltip->MarkPackageDirty();
    TArray<UPackage*> Packages{PassiveTooltip->GetOutermost()};
    const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] rich-text tooltip-only build saved=%s packages=%d"),
        bSaved ? TEXT("true") : TEXT("false"), Packages.Num());
    return bSaved;

#if 0
    // __DEPRECATED_20260719__ [reason: scoped rich-text generation must not recompile startup-path assets]
    UBlueprint* ModActor = LoadBlueprint(TEXT("/Game/Mods/PalworldResourceESP/ModActor"));
    UWidgetBlueprint* PassiveTooltip = LoadObject<UWidgetBlueprint>(
        nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPassiveTooltip"));
    UWidgetBlueprint* PalEntry = LoadObject<UWidgetBlueprint>(
        nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPalEntry"));
    UClass* PalUtilityClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalUtility"));
    UClass* DatabaseCharacterParameterClass = LoadClass<UObject>(
        nullptr, TEXT("/Script/Pal.PalDatabaseCharacterParameter"));
    if (!ModActor || !PassiveTooltip || !PalEntry
        || !PalUtilityClass || !DatabaseCharacterParameterClass) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text scoped build dependency missing"));
        return false;
    }
    if (!PrepareModActorControls(ModActor)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text scoped ModActor audit buffer preparation failed"));
        return false;
    }
    UClass* ModActorClass = ModActor->GeneratedClass;
    if (!ModActorClass || !ModActorClass->FindPropertyByName(RichTextAuditBufferVariableName)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text scoped ModActor audit buffer missing"));
        return false;
    }
    if (!BuildPassiveTooltip(PassiveTooltip)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text scoped tooltip build failed"));
        return false;
    }
    UClass* PassiveTooltipClass = PassiveTooltip->GeneratedClass;
    if (!PassiveTooltipClass || !BuildPalEntry(
            PalEntry, ModActorClass, PalUtilityClass, DatabaseCharacterParameterClass, PassiveTooltipClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text scoped Pal entry build failed"));
        return false;
    }
    UDataTable* PassiveRichTextStyle = LoadObject<UDataTable>(nullptr, PassiveRichTextStylePath);
    if (!PassiveRichTextStyle) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] rich-text scoped style table missing"));
        return false;
    }
    ModActor->MarkPackageDirty();
    PassiveTooltip->MarkPackageDirty();
    PalEntry->MarkPackageDirty();
    PassiveRichTextStyle->MarkPackageDirty();
    TArray<UPackage*> Packages{
        ModActor->GetOutermost(),
        PassiveTooltip->GetOutermost(),
        PalEntry->GetOutermost(),
        PassiveRichTextStyle->GetOutermost(),
    };
    const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
    UE_LOG(LogTemp, Display, TEXT("[ESP_AUTOMATION] rich-text scoped build saved=%s packages=%d"),
        bSaved ? TEXT("true") : TEXT("false"), Packages.Num());
    return bSaved;
#endif
}

bool UESPBlueprintAutomationLibrary::BuildPalworldResourceESPAssets() {
    UBlueprint* ModActor = LoadBlueprint(TEXT("/Game/Mods/PalworldResourceESP/ModActor"));
    UBlueprint* Bridge = LoadBlueprint(TEXT("/Game/Mods/PalworldResourceESP/BP_ESPBridge"));
    UWidgetBlueprint* Overlay = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPOverlay"));
    UWidgetBlueprint* Panel = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPanel"));
    UWidgetBlueprint* PassiveTooltip = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPassiveTooltip"));
    UWidgetBlueprint* PassiveEntry = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPassiveEntry"));
    UWidgetBlueprint* PalEntry = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Mods/PalworldResourceESP/WBP_ESPPalEntry"));
    UClass* PalMonsterClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalMonsterCharacter"));
    UClass* PalPlayerControllerClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalPlayerController"));
    UClass* CharacterParameterComponentClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalCharacterParameterComponent"));
    UClass* IndividualParameterClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalIndividualCharacterParameter"));
    UScriptStruct* IndividualSaveParameterStruct = LoadObject<UScriptStruct>(nullptr, TEXT("/Script/Pal.PalIndividualCharacterSaveParameter"));
    UEnum* GenderEnum = LoadObject<UEnum>(nullptr, TEXT("/Script/Pal.EPalGenderType"));
    UEnum* ElementEnum = LoadObject<UEnum>(nullptr, TEXT("/Script/Pal.EPalElementType"));
    UEnum* WorkSuitabilityEnum = LoadObject<UEnum>(nullptr, TEXT("/Script/Pal.EPalWorkSuitability"));
    UClass* PalUtilityClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalUtility"));
    UClass* PalUIUtilityClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalUIUtility"));
    UClass* DatabaseCharacterParameterClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalDatabaseCharacterParameter"));
    UClass* PlayerRecordDataClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalPlayerRecordData"));
    UClass* PlayerRecordDataUtilityClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalPlayerRecordDataUtility"));
    UClass* PassiveSkillManagerClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalPassiveSkillManager"));
    UScriptStruct* PassiveSkillDatabaseRowStruct = LoadObject<UScriptStruct>(nullptr, TEXT("/Script/Pal.PalPassiveSkillDatabaseRow"));
    UScriptStruct* CharacterParameterDatabaseRowStruct = LoadObject<UScriptStruct>(
        nullptr, TEXT("/Script/Pal.PalCharacterParameterDatabaseRow"));
    UScriptStruct* DropItemDatabaseRowStruct = LoadObject<UScriptStruct>(
        nullptr, TEXT("/Script/Pal.PalDropItemDatabaseRow"));
    UClass* MasterDataTablesUtilityClass = LoadClass<UObject>(nullptr, TEXT("/Script/Pal.PalMasterDataTablesUtility"));
    UClass* OverlayClass = Overlay ? Overlay->GeneratedClass : nullptr;
    if (!ModActor || !Bridge || !Overlay || !Panel || !PassiveTooltip || !PassiveEntry || !PalEntry
        || !PalMonsterClass || !PalPlayerControllerClass || !CharacterParameterComponentClass
        || !IndividualParameterClass || !IndividualSaveParameterStruct || !GenderEnum || !ElementEnum
        || !WorkSuitabilityEnum
        || !PalUtilityClass || !PalUIUtilityClass || !DatabaseCharacterParameterClass
        || !PlayerRecordDataClass || !PlayerRecordDataUtilityClass
        || !PassiveSkillManagerClass || !PassiveSkillDatabaseRowStruct
        || !CharacterParameterDatabaseRowStruct || !DropItemDatabaseRowStruct
        || !MasterDataTablesUtilityClass) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] required asset or class missing"));
        return false;
    }

    UEdGraph* ModActorGraph = EventGraph(ModActor);
    if (!ModActorGraph) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor dependency reset graph missing"));
        return false;
    }
    ClearGraph(ModActorGraph);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ModActor);
    FKismetEditorUtilities::CompileBlueprint(ModActor);
    if (ModActor->Status == BS_Error) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor dependency reset failed"));
        return false;
    }

    if (!PrepareModActorControls(ModActor)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor control preparation failed"));
        return false;
    }
    // WBP_ESPPanel calls this event, while the final implementation depends on the generated panel class.
    if (!AddCustomEvent(ModActor, ModActorGraph, *PanelToggleEventName.ToString(), -900, 1520, {})) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor panel-toggle signature preparation failed"));
        return false;
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ModActor);
    FKismetEditorUtilities::CompileBlueprint(ModActor);
    if (ModActor->Status == BS_Error) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor panel-toggle signature compile failed"));
        return false;
    }
    if (!BuildOverlay(Overlay, PalMonsterClass, CharacterParameterComponentClass, IndividualParameterClass,
            IndividualSaveParameterStruct,
            GenderEnum, ElementEnum, PalUtilityClass, PalUIUtilityClass, DatabaseCharacterParameterClass,
            PlayerRecordDataClass, PlayerRecordDataUtilityClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] overlay build failed"));
        return false;
    }
    UClass* ModActorClass = ModActor->GeneratedClass;
    if (!BuildPassiveTooltip(PassiveTooltip)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] passive tooltip build failed"));
        return false;
    }
    UClass* PassiveTooltipClass = PassiveTooltip->GeneratedClass;
    if (!BuildPassiveEntry(PassiveEntry, ModActorClass, PassiveTooltipClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] passive entry build failed"));
        return false;
    }
    // __DEPRECATED_20260719__ [reason: Pal entries now receive the shared rich-text tooltip class]
    // if (!BuildPalEntry(PalEntry, ModActorClass, PalUtilityClass, DatabaseCharacterParameterClass)) {
    if (!BuildPalEntry(
            PalEntry, ModActorClass, PalUtilityClass, DatabaseCharacterParameterClass, PassiveTooltipClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] Pal entry build failed"));
        return false;
    }
    UClass* PalEntryClass = PalEntry->GeneratedClass;
    UDataTable* PassiveRichTextStyle = LoadObject<UDataTable>(nullptr, PassiveRichTextStylePath);
    if (!PassiveRichTextStyle) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] passive rich-text style table missing"));
        return false;
    }
    UClass* PassiveEntryClass = PassiveEntry->GeneratedClass;
    if (!BuildPanel(
            Panel, ModActorClass, PassiveEntryClass, PalEntryClass, PalUtilityClass, PalUIUtilityClass,
            DatabaseCharacterParameterClass, PassiveSkillManagerClass, PassiveSkillDatabaseRowStruct,
            CharacterParameterDatabaseRowStruct, DropItemDatabaseRowStruct, ElementEnum,
            WorkSuitabilityEnum, MasterDataTablesUtilityClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] panel build failed"));
        return false;
    }
    if (UUserWidget* PanelDefaults = Panel->GeneratedClass
            ? Panel->GeneratedClass->GetDefaultObject<UUserWidget>()
            : nullptr) {
        PanelDefaults->bIsFocusable = true;
        PanelDefaults->bStopAction = true;
    }
    Overlay->MarkPackageDirty();
    Panel->MarkPackageDirty();
    PassiveTooltip->MarkPackageDirty();
    PassiveEntry->MarkPackageDirty();
    PalEntry->MarkPackageDirty();
    PassiveRichTextStyle->MarkPackageDirty();
    TArray<UPackage*> WidgetPackages{
        Overlay->GetOutermost(), Panel->GetOutermost(), PassiveTooltip->GetOutermost(), PassiveEntry->GetOutermost(),
        PalEntry->GetOutermost(),
        PassiveRichTextStyle->GetOutermost(),
    };
    if (!UEditorLoadingAndSavingUtils::SavePackages(WidgetPackages, true)) {
        return false;
    }
    OverlayClass = Overlay->GeneratedClass;
    UClass* PanelClass = Panel->GeneratedClass;
    if (!BuildModActor(ModActor, PalMonsterClass, OverlayClass, PanelClass, PalPlayerControllerClass)) {
        UE_LOG(LogTemp, Error, TEXT("[ESP_AUTOMATION] ModActor build failed"));
        return false;
    }
    ModActor->MarkPackageDirty();
    Bridge->MarkPackageDirty();
    TArray<UPackage*> Packages{ModActor->GetOutermost(), Bridge->GetOutermost()};
    return UEditorLoadingAndSavingUtils::SavePackages(Packages, true);
}
