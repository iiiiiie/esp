import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { lua, lauxlib, lualib, to_luastring, to_jsstring } from "fengari";
import luaparse from "luaparse";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const runtimeRoot = path.join(root, "PalworldResourceESP", "Scripts");
const testRoot = path.join(root, "tests", "lua");

function luaFiles(directory) {
  return fs.readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    const fullPath = path.join(directory, entry.name);
    return entry.isDirectory() ? luaFiles(fullPath) : entry.name.endsWith(".lua") ? [fullPath] : [];
  });
}

const files = [...luaFiles(runtimeRoot), ...luaFiles(testRoot)];
for (const file of files) {
  luaparse.parse(fs.readFileSync(file, "utf8"), { luaVersion: "5.3" });
}

const forbiddenRuntimeGlobals = /\b(?:FindAllOf|FindFirstOf|StaticFindObject|RegisterHook|NotifyOnNewObject)\b/;
for (const file of luaFiles(path.join(runtimeRoot, "core"))) {
  const source = fs.readFileSync(file, "utf8");
  if (forbiddenRuntimeGlobals.test(source)) {
    throw new Error(`Pure core imports a game runtime global: ${path.relative(root, file)}`);
  }
}

const generatorPath = path.join(
  root,
  "tools",
  "logicmod",
  "ESPBlueprintAutomation",
  "Source",
  "ESPBlueprintAutomation",
  "Private",
  "ESPBlueprintAutomationLibrary.cpp",
);
const generatorSource = fs.readFileSync(generatorPath, "utf8");
const mainSource = fs.readFileSync(path.join(runtimeRoot, "main.lua"), "utf8");
if (!generatorSource.includes('TEXT("ESP_TargetParameters")')
    || !generatorSource.includes('TEXT("EqualEqual_ObjectObject")')
    || !generatorSource.includes("IdentityMatchBranch")) {
  throw new Error("Blueprint live parameter-identity validation contract is incomplete");
}
if (/^\s*\|\| !Link\(IdentityMatchBranch, UEdGraphSchema_K2::PN_Else, RebindRemoveTarget/m.test(generatorSource)) {
  throw new Error("Overlay identity validation still mutates parallel target arrays during ForEachTarget");
}
const lifecycleHookStart = mainSource.indexOf("local function register_lifecycle_hooks()");
const lifecycleHookEnd = mainSource.indexOf("local function register_load_map_hooks()", lifecycleHookStart);
const lifecycleHookSource = mainSource.slice(lifecycleHookStart, lifecycleHookEnd);
const bridgeDiscoveryStart = mainSource.indexOf("local function register_blueprint_bridge_discovery()");
const bridgeDiscoveryEnd = mainSource.indexOf("local function call_bridge(", bridgeDiscoveryStart);
const bridgeDiscoverySource = mainSource.slice(bridgeDiscoveryStart, bridgeDiscoveryEnd);
if (lifecycleHookStart < 0 || lifecycleHookEnd < 0
    || !lifecycleHookSource.includes("enqueue_event_retry(")
    || !lifecycleHookSource.includes("remove_bridge_target(")
    || !lifecycleHookSource.includes("actor_from_hook_arguments")) {
  throw new Error("Lifecycle hooks do not use path-only admission and cached removal");
}
if (bridgeDiscoveryStart < 0 || bridgeDiscoveryEnd < 0
    || bridgeDiscoverySource.includes("admit_begin_play_target(")
    || !bridgeDiscoverySource.includes("enqueue_event_retry(")) {
  throw new Error("BeginPlay does not defer per-Pal admission by scalar object path");
}
const cachedSyncStart = mainSource.indexOf("local function atomic_sync_cached_nearest_targets(");
const cachedSyncEnd = mainSource.indexOf("local function process_cached_bridge_resync(", cachedSyncStart);
const cachedSyncSource = mainSource.slice(cachedSyncStart, cachedSyncEnd);
if (cachedSyncStart < 0 || cachedSyncEnd < 0
    || cachedSyncSource.includes("FindAllOf(")
    || cachedSyncSource.includes("BRIDGE_METHOD_REMOVE_TARGET")
    || !cachedSyncSource.includes("BRIDGE_METHOD_CLEAR_TARGET")
    || !cachedSyncSource.includes("BRIDGE_METHOD_SET_TARGET")) {
  throw new Error("Cached nearest-N sync is not an atomic clear/set path independent of FindAllOf");
}
const numericStart = generatorSource.indexOf("bool BuildPanelNumericEventV2(");
const numericEnd = generatorSource.indexOf("bool BuildPanelInitializeControlsV2(", numericStart);
const numericSource = generatorSource.slice(numericStart, numericEnd);
if (numericStart < 0 || numericEnd < 0
    || !numericSource.includes("OnMouseCaptureEnd")
    || !numericSource.includes("OnControllerCaptureEnd")
    || !numericSource.includes("AddSpinBoxValueCommittedEvent")
    || numericSource.includes("AddSpinBoxValueChangedEvent(Blueprint, Control.SpinBox")) {
  throw new Error("V2 numeric controls do not use release/commit-only application");
}
if (!generatorSource.includes('TEXT("GetNickname")')
    || !generatorSource.includes("const TArray<FVector2D> OutlineOffsets")
    || !generatorSource.includes('TEXT("(R=0.0,G=0.0,B=0.0,A=1.0)")')) {
  throw new Error("Blueprint name or outlined-label contract is incomplete");
}
if (!generatorSource.includes('TEXT("IsRarePal")')
    || !generatorSource.includes('TEXT("ESP_TargetLuckyStates")')
    || !generatorSource.includes('SetPinDefault(AddLuckyStateItem, TEXT("NewItem"), TEXT("-1"))')
    || !generatorSource.includes('TEXT("ESP_LuckyFilterId")')
    || !generatorSource.includes('TEXT("ESP_LuckyOnlyButton")')
    || !generatorSource.includes('TEXT("ESP_LuckyExcludeButton")')
    || !generatorSource.includes("LuckyFilterRestricted")
    || !generatorSource.includes("LuckyRestrictedMatch")) {
  throw new Error("Blueprint Lucky provider or fail-closed filter contract is incomplete");
}
if (!generatorSource.includes('TEXT("GetCharacterID")')
    || !generatorSource.includes('TEXT("GetDatabaseCharacterParameter")')
    || !generatorSource.includes('TEXT("GetIsBoss")')
    || !generatorSource.includes('TEXT("ESP_TargetBossStates")')
    || !generatorSource.includes('SetPinDefault(AddBossStateItem, TEXT("NewItem"), TEXT("-1"))')
    || !generatorSource.includes('TEXT("ESP_BossFilterId")')
    || !generatorSource.includes('TEXT("ESP_BossOnlyButton")')
    || !generatorSource.includes('TEXT("ESP_BossExcludeButton")')
    || !generatorSource.includes("BossFilterRestricted")
    || !generatorSource.includes("BossRestrictedMatch")) {
  throw new Error("Blueprint Boss provider or fail-closed filter contract is incomplete");
}
if (!generatorSource.includes('TEXT("GetLocalRecordData")')
    || !generatorSource.includes('TEXT("PalCaptureCount")')
    || !generatorSource.includes('TEXT("GetRecordData_Int")')
    || !generatorSource.includes('TEXT("ESP_TargetCaptureCounts")')
    || !generatorSource.includes('SetPinDefault(AddCaptureCountItem, TEXT("NewItem"), TEXT("-1"))')
    || !generatorSource.includes('TEXT("ESP_CollectionFilterId")')
    || !generatorSource.includes('TEXT("ESP_CollectionIncompleteButton")')
    || !generatorSource.includes('TEXT("ESP_CollectionCompleteButton")')
    || !generatorSource.includes("CollectionKnownMatch")
    || !generatorSource.includes("CollectionFilterBranch")
    || !generatorSource.includes('SetPinDefault(CollectionComplete, TEXT("B"), TEXT("5"))')) {
  throw new Error("Blueprint collection-count provider, five-capture threshold, or fail-closed filter contract is incomplete");
}
if (!generatorSource.includes('TEXT("HasElementType")')
    || !generatorSource.includes('TEXT("ESP_TargetElementMasks")')
    || !generatorSource.includes('TEXT("ESP_ElementFilterMask")')
    || !generatorSource.includes('SetPinDefault(AddElementMaskItem, TEXT("NewItem"), TEXT("-1"))')
    || !generatorSource.includes('TEXT("And_IntInt")')
    || !generatorSource.includes("ElementFilterAll")
    || !generatorSource.includes("TargetElementKnown")
    || !generatorSource.includes("ElementMaskMatches")
    || !generatorSource.includes('TEXT("ESP_ElementFireToggle")')
    || !generatorSource.includes('TEXT("ESP_ElementWaterToggle")')) {
  throw new Error("Blueprint element provider, match-any, or fail-closed filter contract is incomplete");
}
if (!generatorSource.includes('TEXT("GetSaveParameter")')
    || !generatorSource.includes("UK2Node_BreakStruct")
    || !generatorSource.includes('TEXT("Talent_HP")')
    || !generatorSource.includes('TEXT("Talent_Shot")')
    || !generatorSource.includes('TEXT("Talent_Defense")')
    || !generatorSource.includes('TEXT("ESP_TargetIvHp")')
    || !generatorSource.includes('TEXT("ESP_TargetIvAttack")')
    || !generatorSource.includes('TEXT("ESP_TargetIvDefense")')
    || !generatorSource.includes('SetPinDefault(AddIvHpItem, TEXT("NewItem"), TEXT("-1"))')
    || !generatorSource.includes('TEXT("ESP_ShowIV")')
    || !generatorSource.includes('TEXT("ESP_IvHpMin")')
    || !generatorSource.includes('TEXT("ESP_IvAttackMin")')
    || !generatorSource.includes('TEXT("ESP_IvDefenseMin")')
    || !generatorSource.includes("IvMinimumAccepted")
    || !generatorSource.includes("IvHpMeetsMinimum")
    || !generatorSource.includes("IvAttackMeetsMinimum")
    || !generatorSource.includes("IvDefenseMeetsMinimum")
    || !generatorSource.includes('TEXT("个体值 生命 ")')
    || !generatorSource.includes('TEXT(" / 攻击 ")')
    || !generatorSource.includes('TEXT(" / 防御 ")')
    || !generatorSource.includes('TEXT("IV HP ")')
    || !generatorSource.includes('TEXT(" / ATK ")')
    || !generatorSource.includes('TEXT(" / DEF ")')
    || !generatorSource.includes("OverlayLanguageIdVariableName")
    || !generatorSource.includes("LanguageIsEnglish")) {
  throw new Error("Blueprint IV provider, unknown sentinel, or display contract is incomplete");
}
if (!generatorSource.includes('TEXT("GetPassiveSkillList")')
    || !generatorSource.includes('TEXT("GetPassiveSkillName")')
    || !generatorSource.includes('TEXT("Conv_TextToString")')
    || !generatorSource.includes('TEXT("ESP_TargetPassiveTexts")')
    || !generatorSource.includes('TEXT("ESP_ShowPassiveSkills")')
    || !generatorSource.includes('TEXT("ESP_ShowPassiveSkillsToggle")')) {
  throw new Error("Blueprint passive-skill provider, localization, or display contract is incomplete");
}
if (!generatorSource.includes('TEXT("GetSortedPassiveSkillNameArray")')
    || !generatorSource.includes('TEXT("GetPassiveSkillManager")')
    || !generatorSource.includes('TEXT("GetSkillData")')
    || !generatorSource.includes('{FName("SkillName"), StringPin()}')
    || !generatorSource.includes('TEXT("Conv_DoubleToString")')
    || !generatorSource.includes('TEXT("Replace")')
    || !generatorSource.includes('TEXT("{EffectValue%d}")')
    || !generatorSource.includes('TEXT("GetLocalizedText")')
    || !generatorSource.includes('TEXT("SkillDesc")')
    || !generatorSource.includes('TEXT("ESP_PassiveFilterIds")')
    || !generatorSource.includes("ForEachPassiveFilterId")
    || !generatorSource.includes("PassiveFilterMatchAnd")
    || !generatorSource.includes('SetPinDefault(HasCapacity, TEXT("B"), TEXT("4"))')
    || !generatorSource.includes("SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f)")
    || !generatorSource.includes("SetOffsets(FMargin(16.0f))")
    || !generatorSource.includes("UWidgetSwitcher")
    || !generatorSource.includes('/Game/Mods/PalworldResourceESP/WBP_ESPPassiveEntry')) {
  throw new Error("Blueprint passive AND-filter, tooltip catalog, or tabbed-panel contract is incomplete");
}
if (!generatorSource.includes('FSlateColor::UseForeground()')
    || !generatorSource.includes('SetCheckedForegroundColor')
    || !generatorSource.includes('PanelV2Style::AccentText')
    || !generatorSource.includes('Label->GetFont()')
    || !generatorSource.includes('Header->GetFont()')
    || generatorSource.includes('FCoreStyle::GetDefaultFont()')) {
  throw new Error("Passive entry labels do not define readable unchecked and checked foreground colors");
}
const passiveEntrySource = generatorSource.slice(
  generatorSource.indexOf('bool BuildPassiveEntry('),
  generatorSource.indexOf('bool BuildPalEntry('),
);
const passiveTooltipSource = generatorSource.slice(
  generatorSource.indexOf('bool BuildPassiveTooltip('),
  generatorSource.indexOf('bool BuildPassiveEntry('),
);
const palEntrySource = generatorSource.slice(
  generatorSource.indexOf('bool BuildPalEntry('),
  generatorSource.indexOf('bool BuildPanelPalCatalog('),
);
const palCatalogSource = generatorSource.slice(
  generatorSource.indexOf('bool BuildPanelPalCatalog('),
  generatorSource.indexOf('bool BuildPanelPassiveCatalog('),
);
const expectedPalWorkFields = [
  "WorkSuitability_EmitFlame",
  "WorkSuitability_Watering",
  "WorkSuitability_Seeding",
  "WorkSuitability_GenerateElectricity",
  "WorkSuitability_Handcraft",
  "WorkSuitability_Collection",
  "WorkSuitability_Deforest",
  "WorkSuitability_Mining",
  "WorkSuitability_ProductMedicine",
  "WorkSuitability_Cool",
  "WorkSuitability_Transport",
  "WorkSuitability_MonsterFarm",
];
if (!generatorSource.includes('/Game/Pal/DataTable/Character/DT_PalMonsterParameter')
    || !palCatalogSource.includes('TEXT("GetDataTableRowNames")')
    || !palCatalogSource.includes('TEXT("IsPal")')
    || !palCatalogSource.includes('TEXT("IsTowerBoss")')
    || !palCatalogSource.includes('TEXT("IsRaidBoss")')
    || !palCatalogSource.includes('CatalogIdAlreadyAdded')
    || !palCatalogSource.includes('ExistingCatalogIdsForDuplicate')
    || !palCatalogSource.includes('TEXT("GetLocalizedCharacterName")')
    || !palCatalogSource.includes('TEXT("GetPalElementTypeName")')
    || !palCatalogSource.includes('TEXT("GetWorkSuitabilityName")')
    || !palCatalogSource.includes('ElementToggles.Num() != 9')
    || !palCatalogSource.includes('WorkToggles.Num() != GetPalWorkDefinitions().Num()')
    || expectedPalWorkFields.some((field) => !generatorSource.includes(`TEXT("${field}")`))
    || /^\s*\{TEXT\("OilExtraction"\)/m.test(generatorSource)
    || !palCatalogSource.includes('ElementQueryAccepted')
    || !palCatalogSource.includes('WorkQueryAccepted')
    || !palCatalogSource.includes('TEXT("GetDropItemData")')
    || !palCatalogSource.includes('TEXT("GetItemName")')
    || !palCatalogSource.includes('PalCatalogSuppressEventsVariableName')
    || !generatorSource.includes('TEXT("GetCharacterIconTexture")')
    || !generatorSource.includes('PalEntryInitializingVariableName')
    || ["SheepBall", "Kitsunebi", "Anubis"].some((id) => palCatalogSource.includes(`TEXT("${id}")`))) {
  throw new Error("Runtime Pal catalog, match-all query, details, or snapshot-free contract is incomplete");
}
if (!palCatalogSource.includes('TEXT("Array_Insert")')
    || !palCatalogSource.includes("InsertZukanIndex")
    || !palCatalogSource.includes("InsertCatalogId")
    || !palCatalogSource.includes("NewSortKeyLessThanExisting")
    || !palCatalogSource.includes("PalCatalogSortModeVariableName")
    || !palCatalogSource.includes("PalCatalogSortDescendingVariableName")
    || !palCatalogSource.includes("PalCatalogSortKeysVariableName")
    || !palCatalogSource.includes('Link(BreakCatalogRow, TEXT("RunSpeed"), SelectRunSpeed, TEXT("A"))')
    || !palCatalogSource.includes('Link(BreakCatalogRow, TEXT("ZukanIndex"), SelectRunSpeed, TEXT("B"))')
    || !palCatalogSource.includes('SortModeIsWork')
    || !palCatalogSource.includes('SelectWork')
    || !palCatalogSource.includes('FString::FromInt(Index + 2)')
    || !palCatalogSource.includes('FinalSortKey')
    || !palCatalogSource.includes("NameWithVariant")
    || !palCatalogSource.includes('TEXT(" [ID: ")')
    || !palCatalogSource.includes('TEXT("B"), TEXT(" "))')
    || !palCatalogSource.includes('Index + 1 < WorkDefinitions.Num() ? TEXT(" | ") : TEXT("")')
    || !palCatalogSource.includes("NameContainsSearch")
    || !palCatalogSource.includes("NumberContainsSearch")
    || !palCatalogSource.includes('TEXT("GetFormatedFirstActivatedInfoTextFixedRank")')
    || !palCatalogSource.includes('TEXT("outFormatedText")')
    || !palCatalogSource.includes('SetPinDefault(GetPartnerDescription, TEXT("Rank"), TEXT("1"))')
    || palCatalogSource.includes('TEXT("PartnerSkillAppendText")')
    || palCatalogSource.includes('TEXT("PalLongDescription")')
    || !palCatalogSource.includes("PalCatalogSelectedNamesVariableName")
    || !generatorSource.includes('TEXT("已选中：无")')
    || !generatorSource.includes("PanelV2Style::Accent, 3.0f")) {
  throw new Error("Pal catalog ordering, search, selected-state, summary, or partner-effect contract is incomplete");
}
const panelV2Start = generatorSource.indexOf("bool BuildPanel(\n");
const panelV2End = generatorSource.indexOf("bool BuildModActor(", panelV2Start);
const panelV2Source = generatorSource.slice(panelV2Start, panelV2End);
if (panelV2Start < 0 || panelV2End < 0
    || !panelV2Source.includes("FilterSubSwitcher->AddChild(FilterPage)")
    || !panelV2Source.includes("FilterSubSwitcher->AddChild(PalPage)")
    || !panelV2Source.includes("FilterRoot->AddChild(FilterActions)")
    || !panelV2Source.includes('AddPanelButtonV2(Blueprint, FilterActions, TEXT("ESP_ClearAllFiltersButton")')
    || panelV2Source.includes("\n    Switcher->AddChild(PalPage)")
    || !panelV2Source.includes('TEXT("ESP_FilterPalTabButton")')
    || !panelV2Source.includes('{TEXT("ESP_EnglishText"), TEXT("英文"), TEXT("English")}')) {
  throw new Error("Pal species filtering is not nested under Filters or default localization is mixed");
}
const panelKeyDownStart = generatorSource.indexOf("bool BuildPanelKeyDownOverride(");
const panelKeyDownEnd = generatorSource.indexOf("bool BuildPanelKeyUpOverride(", panelKeyDownStart);
const panelKeyDownSource = generatorSource.slice(panelKeyDownStart, panelKeyDownEnd);
const panelKeyUpStart = panelKeyDownEnd;
const panelKeyUpEnd = generatorSource.indexOf("bool PrepareModActorControls(", panelKeyUpStart);
const panelKeyUpSource = generatorSource.slice(panelKeyUpStart, panelKeyUpEnd);
if (!generatorSource.includes('TEXT("DisableInput")')
    || !generatorSource.includes('TEXT("EnableInput")')
    || !generatorSource.includes('TEXT("SetDisableInputFlag")')
    || !generatorSource.includes('TEXT("PalworldResourceESP_Panel")')
    || !generatorSource.includes("BuildPanelKeyDownOverride(Blueprint)")
    || !generatorSource.includes("BuildPanelKeyUpOverride(Blueprint, ModActorClass)")
    || !generatorSource.includes('UWidgetBlueprintLibrary::StaticClass(), TEXT("Handled")')
    || panelKeyDownStart < 0
    || panelKeyDownEnd < 0
    || panelKeyUpStart < 0
    || panelKeyUpEnd < 0
    || !panelKeyDownSource.includes('UKismetInputLibrary::StaticClass(), TEXT("GetKey")')
    || !panelKeyDownSource.includes('UKismetInputLibrary::StaticClass(), TEXT("EqualEqual_KeyKey")')
    || !panelKeyDownSource.includes('SetPinDefault(IsEscape, TEXT("B"), TEXT("Escape"))')
    || !panelKeyDownSource.includes("PanelEscapeClosePendingVariableName")
    || panelKeyDownSource.includes("PanelToggleEventName")
    || !panelKeyUpSource.includes("PanelEscapeClosePendingVariableName")
    || !panelKeyUpSource.includes("PanelToggleEventName")
    || !panelKeyUpSource.includes("Link(ClearPending, UEdGraphSchema_K2::PN_Then, TogglePanel")
    || !generatorSource.includes("AddCustomEvent(ModActor, ModActorGraph, *PanelToggleEventName.ToString()")
    || !generatorSource.includes("bRefreshCatalogs")
    || !generatorSource.includes("RefreshPalCatalog")) {
  throw new Error("Panel input isolation, Escape-close, or language-triggered catalog refresh contract is incomplete");
}
if (!generatorSource.includes('URichTextBlock')
    || !generatorSource.includes('DT_ESPRichTextStyle')
    || !generatorSource.includes('WBP_ESPPassiveTooltip')
    || !generatorSource.includes('PassiveTooltipInitializeEventName')
    || !generatorSource.includes('TEXT("NumRed_13")')
    || !generatorSource.includes('TEXT("NumBlue_13")')
    || !passiveEntrySource.includes('TEXT("SetToolTip")')
    || passiveEntrySource.includes('TEXT("SetToolTipText")')) {
  throw new Error("Passive tooltips do not use the project-owned rich-text rendering contract");
}
const expectedGameRichTextDecorators = [
  "BP_PalRichTextIconDecorator",
  "BP_PalRichTextDecorator_ItemName",
  "BP_PalRichTextDecorator_CharacterName",
  "BP_PalRichTextDecorator_MapObject",
  "BP_PalRichTextDecorator_ActiveSkillName",
  "BP_PalRichTextDecorator_UICommon",
];
if (!passiveTooltipSource.includes('TEXT("SetDecorators")')
    || !passiveTooltipSource.includes('TEXT("LoadClassAsset_Blocking")')
    || !passiveTooltipSource.includes('AddPureClassDynamicCast')
    || expectedGameRichTextDecorators.some((name) => !passiveTooltipSource.includes(name))
    || !palEntrySource.includes('TEXT("SetToolTip")')
    || !palEntrySource.includes('PassiveTooltipInitializeEventName')
    || palEntrySource.includes('TEXT("SetToolTipText")')) {
  throw new Error("Pal partner-effect tooltips do not use the current game's rich-text decorators");
}
if (!passiveTooltipSource.includes('TEXT("Conv_TextToString")')
    || !passiveTooltipSource.includes('NormalizeStyleAttributePair')
    || !passiveTooltipSource.includes('SetPinDefault(NormalizeStyleAttributePair, TEXT("From"), TEXT("| style=|"))')
    || !passiveTooltipSource.includes('SetPinDefault(NormalizeAttributeOpen, TEXT("From"), TEXT("=|"))')
    || !passiveTooltipSource.includes('SetPinDefault(NormalizeSelfClosingAttribute, TEXT("From"), TEXT("|/>"))')
    || !passiveTooltipSource.includes('TEXT("Conv_StringToText")')
    || passiveTooltipSource.includes('Link(Initialize, TEXT("Description"), SetText, TEXT("InText"))')) {
  throw new Error("Pal pipe-delimited rich-text attributes are not normalized before Unreal parses the tooltip");
}
const normalizePalRichTextFixture = (value) => value
  .replaceAll('| style=|', '" style="')
  .replaceAll('=|', '="')
  .replaceAll('|/>', '"/>');
const palRichTextFixture = 'HP 70 | ATK 70\n<itemName id=|Wool| style=|Status_Keyword|/>';
const normalizedPalRichTextFixture = normalizePalRichTextFixture(palRichTextFixture);
if (!normalizedPalRichTextFixture.startsWith('HP 70 | ATK 70\n')
    || !normalizedPalRichTextFixture.endsWith('<itemName id="Wool" style="Status_Keyword"/>')) {
  throw new Error("Pal rich-text normalization corrupts visible pipe separators or misses audited attributes");
}
const gameRichTextStylePath = "/Game/Pal/DataTable/Text/RchTextData/DT_PalRichTextStyle.DT_PalRichTextStyle";
const styleLoadIndex = passiveTooltipSource.indexOf('TEXT("LoadAsset_Blocking")');
const setStyleIndex = passiveTooltipSource.indexOf('TEXT("SetTextStyleSet")');
const setDecoratorsIndex = passiveTooltipSource.indexOf('TEXT("SetDecorators")');
if (!generatorSource.includes(gameRichTextStylePath)
    || !passiveTooltipSource.includes('TEXT("SelectObject")')
    || !passiveTooltipSource.includes('FallbackStylePin->DefaultObject = RichTextStyle')
    || styleLoadIndex < 0
    || setStyleIndex <= styleLoadIndex
    || setDecoratorsIndex <= setStyleIndex) {
  throw new Error("Partner tooltips do not prefer the running game's full rich-text style table before decorators");
}
if (!generatorSource.includes('constexpr bool EnablePartnerRichTextAudit = false')
    || !generatorSource.includes('RichTextAuditBufferVariableName(TEXT("ESP_RichTextAuditBuffer"))')
    || !generatorSource.includes('EnsureMemberVariable(Blueprint, RichTextAuditBufferVariableName, StringPin())')
    || !palEntrySource.includes('AuditBufferGet = AddExternalVariableGet')
    || !palEntrySource.includes('AuditBufferSet = AddExternalVariableSet')
    || !palEntrySource.includes('TEXT("PrintString")')
    || !palEntrySource.includes('TEXT("bPrintToScreen"), TEXT("false")')
    || !palEntrySource.includes('TEXT("bPrintToLog"), TEXT("true")')
    || !mainSource.includes('read_panel_string("ESP_RichTextAuditBuffer")')
    || !mainSource.includes('log_event("RICH_TEXT_AUDIT", line)')
    || !mainSource.includes('-- drain_rich_text_audit()')) {
  throw new Error("Completed partner rich-text audit is not disabled with its reversible diagnostic path retained");
}
const richTextScopedBuildSource = generatorSource.slice(
  generatorSource.indexOf('bool UESPBlueprintAutomationLibrary::BuildPalworldResourceESPRichTextAssets()'),
  generatorSource.indexOf('#if 0', generatorSource.indexOf('bool UESPBlueprintAutomationLibrary::BuildPalworldResourceESPRichTextAssets()')),
);
if (!richTextScopedBuildSource.includes('BuildPassiveTooltip(PassiveTooltip)')
    || !richTextScopedBuildSource.includes('TArray<UPackage*> Packages{PassiveTooltip->GetOutermost()}')
    || !richTextScopedBuildSource.includes('TEXT("[ESP_AUTOMATION] rich-text tooltip-only build saved=%s packages=%d")')
    || richTextScopedBuildSource.includes('PrepareModActorControls')
    || richTextScopedBuildSource.includes('BuildPalEntry(')) {
  throw new Error("Rich-text scoped generation is not isolated from startup-path assets");
}
const passiveDescriptionFallbackIds = [
  "Deffence_down2",
  "CraftSpeed_down2",
  "PAL_ALLAttack_down2",
  "CraftSpeed_down1",
  "PAL_ALLAttack_down1",
  "Deffence_down1",
  "PAL_CorporateSlave",
  "PAL_masochist",
  "PAL_ALLAttack_up2",
  "PAL_ALLAttack_up1",
  "PAL_conceited",
  "PAL_sadist",
  "PAL_oraora",
  "CraftSpeed_up1",
  "Deffence_up1",
  "CraftSpeed_up2",
  "PAL_rude",
  "CraftSpeed_up3",
  "PAL_ALLAttack_up3",
  "Noukin",
  "Legend",
  "Rare",
  "MutationPal_ExplosionResist",
  "WorkSuitabilityAddRank_MonsterFarm_2",
  "WorkSuitabilityAddRank_MonsterFarm_1",
  "Vampire",
  "MutationPal_Mutant",
];
if (passiveDescriptionFallbackIds.some((id) => !generatorSource.includes(`TEXT("${id}")`))
    || !generatorSource.includes('TEXT("Status_Up")')
    || !generatorSource.includes("FallbackDescriptionNotEmpty")
    || !generatorSource.includes("ResolvedDescriptionToText")) {
  throw new Error("Passive description fallback or rich-text status contract is incomplete");
}
if (!generatorSource.includes("Groups.Num() != 8")
    || !generatorSource.includes('SetPinDefault(RankRainbow, TEXT("B"), TEXT("5"))')
    || !generatorSource.includes('SetPinDefault(RankLegend, TEXT("B"), TEXT("4"))')
    || !generatorSource.includes('SetPinDefault(RankGold3, TEXT("B"), TEXT("3"))')
    || !generatorSource.includes('SetPinDefault(RankGold2, TEXT("B"), TEXT("2"))')
    || !generatorSource.includes('SetPinDefault(RankNormal, TEXT("B"), TEXT("1"))')
    || !generatorSource.includes('SetPinDefault(RankNegative1, TEXT("B"), TEXT("-1"))')
    || !generatorSource.includes('SetPinDefault(RankNegative2, TEXT("B"), TEXT("-2"))')
    || !generatorSource.includes('SetPinDefault(RankNegative3, TEXT("B"), TEXT("-3"))')
    || !generatorSource.includes("InitializeEntry, UEdGraphSchema_K2::PN_Then, RainbowBranch")
    || !generatorSource.includes("RainbowBranch, UEdGraphSchema_K2::PN_Else, LegendBranch")
    || !generatorSource.includes("LegendBranch, UEdGraphSchema_K2::PN_Else, Gold3Branch")
    || !generatorSource.includes("Gold3Branch, UEdGraphSchema_K2::PN_Else, Gold2Branch")
    || !generatorSource.includes('TEXT("ESP_PassiveGold2HeaderText")')
    || /^\s*\|\| !Link\(BreakSkillData, TEXT\("LotteryWeight"\)/m.test(generatorSource)) {
  throw new Error("Passive Rank 5..-3 category mapping is incomplete or still uses lottery weight");
}
if (!generatorSource.includes('TEXT("ESP_PassiveExcludeIds")')
    || !generatorSource.includes('TEXT("ESP_PassiveExcludeMatch")')
    || !generatorSource.includes('TEXT("OnMouseButtonDown")')
    || !generatorSource.includes('TEXT("PointerEvent_IsMouseButtonDown")')
    || !generatorSource.includes('TEXT("RightMouseButton")')
    || !generatorSource.includes("RemoveExclusionForInclude")
    || !generatorSource.includes("RemoveRightInclusion")
    || !generatorSource.includes("ClearExcludeIds")
    || !generatorSource.includes("PassiveExcludeMatchAnd")
    || !generatorSource.includes("PassiveExcludeBranch")) {
  throw new Error("Passive right-click exclusion, mutual exclusion, clear, or Overlay rejection contract is incomplete");
}
if (!generatorSource.includes('TEXT("ESP_PassiveIncludeText")')
    || !generatorSource.includes('TEXT("ESP_PassiveExcludeText")')
    || !generatorSource.includes('TEXT("PalworldResourceESP_ApplyPersistedPanelState")')
    || !generatorSource.includes('TEXT("ParseIntoArray")')
    || !generatorSource.includes("AppendPassiveTokenMutation")
    || !generatorSource.includes('TEXT("ESP_PassiveRainbowExpanded")')
    || !generatorSource.includes('TEXT("ESP_PassiveNegative3Expanded")')
    || !generatorSource.includes("AddExpandableAreaExpansionChangedEvent")
    || !generatorSource.includes('TEXT("筛选设置为默认")')
    || !generatorSource.includes('TEXT("Reset filters to defaults")')) {
  throw new Error("Passive selection, expansion, restore, or reset-default persistence contract is incomplete");
}
const panelToggleStart = generatorSource.indexOf("UCheckBox* AddPanelToggleV2(");
const panelToggleEnd = generatorSource.indexOf("UCheckBox* AddPanelFilterChipV2(", panelToggleStart);
const panelToggleSource = generatorSource.slice(panelToggleStart, panelToggleEnd);
if (panelToggleStart < 0 || panelToggleEnd < 0
    || panelToggleSource.indexOf("Row->AddChild(Toggle)") > panelToggleSource.indexOf("Row->AddChild(Label)")
    || !panelToggleSource.includes("ESlateSizeRule::Automatic, HAlign_Left")) {
  throw new Error("Display toggles are not left-aligned immediately before their labels");
}
if (!generatorSource.includes('TEXT("ESP_PassiveRainbowWrap"), TEXT("彩虹"), false')
    || !generatorSource.includes('TEXT("ESP_PassiveSpecialWrap"), TEXT("传说"), false')) {
  throw new Error("Passive categories do not default to collapsed");
}
if (!generatorSource.includes('UEditableTextBox')
    || !generatorSource.includes('TEXT("ESP_PassiveSearchBox")')
    || !generatorSource.includes('TEXT("Contains")')
    || !generatorSource.includes('TEXT("IsEmpty")')
    || !generatorSource.includes('SearchQueryMatches')
    || !generatorSource.includes('CatalogEntryValid')
    || !generatorSource.includes('SummaryIdAvailable')
    || !generatorSource.includes('SetPinDefault(NameNotNone, TEXT("B"), TEXT("None"))')) {
  throw new Error("Passive search and invalid-entry filtering contract is incomplete");
}
if (!generatorSource.includes("ToggleUnchecked")
    || !generatorSource.includes("ToggleOutline")
    || !generatorSource.includes("1.5f, FVector2D(28.0f, 24.0f)")) {
  throw new Error("Panel display toggles do not expose the high-contrast outlined state contract");
}

const normalize = (value) => value.replaceAll("\\", "/");
const packagePaths = [
  `${normalize(runtimeRoot)}/?.lua`,
  `${normalize(runtimeRoot)}/?/init.lua`,
  `${normalize(testRoot)}/?.lua`,
].join(";");
const entrypoint = normalize(path.join(testRoot, "run.lua"));
const runtimeEntrypoint = normalize(path.join(runtimeRoot, "main.lua"));
const source = `
package.path = [[${packagePaths}]] .. ";" .. package.path
dofile([[${entrypoint}]])

local runtime_logs = {}
local original_print = print
print = function(message)
    runtime_logs[#runtime_logs + 1] = tostring(message)
    original_print(message)
end

local fake_time = 1000
os.time = function()
    return fake_time
end

local runtime_settings_writes = {}
local runtime_settings_snapshot = "v1 runtime_enabled=true profile_id=2 preset_id=1 language_id=0 level_min=0 level_max=0 distance_max=330 display_limit=64 show_top=true show_name=true show_level=true show_distance=true gender=0"
local runtime_settings_open_path = nil
PalworldResourceESPSettingsIO = {
    open = function(path, mode)
        runtime_settings_open_path = path
        if mode == "r" then
            local emitted = false
            return {
                lines = function()
                    return function()
                        if emitted then
                            return nil
                        end
                        emitted = true
                        return runtime_settings_snapshot
                    end
                end,
                close = function() end,
            }
        end
        if mode == "a" then
            return {
                write = function(_, value)
                    runtime_settings_writes[#runtime_settings_writes + 1] = value
                    return true
                end,
                flush = function() end,
                close = function() end,
            }
        end
        return nil, "unsupported_mode"
    end,
}

local classes = {}
local static_objects = {}
local static_actor_find_count = 0
local static_actor_find_counts = {}
StaticFindObject = function(path)
    if static_objects[path] ~= nil then
        static_actor_find_count = static_actor_find_count + 1
        static_actor_find_counts[path] = (static_actor_find_counts[path] or 0) + 1
        return static_objects[path]
    end
    if path:match("^/Game/Test%.PersistentLevel%.") then
        return nil
    end
    classes[path] = classes[path] or { path = path }
    return classes[path]
end
RegisterCustomEvent = function() end
local bridge_begin_play_hook = nil
RegisterBeginPlayPostHook = function(callback)
    bridge_begin_play_hook = callback
    return 1
end
local monster_notification = nil
NotifyOnNewObject = function(_, callback)
    monster_notification = callback
end
local registered_hooks = {}
RegisterHook = function(path, pre_callback, post_callback)
    if type(pre_callback) ~= "function" then
        error("RegisterHook requires a pre callback")
    end
    registered_hooks[path] = { pre = pre_callback, post = post_callback }
    return 1, 2
end
local panel_keybind = nil
local panel_key = nil
local panel_modifiers = nil
Key = { Y = 89 }
ModifierKey = { SHIFT = 1 }
RegisterKeyBind = function(key, modifiers, callback)
    panel_key = key
    panel_modifiers = modifiers
    panel_keybind = callback
end
local load_map_pre_hook = nil
RegisterLoadMapPreHook = function(callback)
    load_map_pre_hook = callback
end
RegisterLoadMapPostHook = function() end

local reconcile_loop = nil
local delayed_callbacks = {}
LoopInGameThreadWithDelay = function(_, callback)
    reconcile_loop = callback
    return true
end
ExecuteInGameThreadWithDelay = function(_, callback)
    delayed_callbacks[#delayed_callbacks + 1] = callback
end

local player = {}
function player:IsA(class_object)
    return class_object.path == "/Script/Pal.PalPlayerCharacter"
end

local camera_x = 0
local camera_manager = {}
function camera_manager:GetCameraLocation()
    return { X = camera_x, Y = 0, Z = 0 }
end
local controller = { PlayerCameraManager = camera_manager }
FindFirstOf = function()
    return controller
end

local monsters = {}
local pal_runtime_class = {}
function pal_runtime_class:GetFullName()
    return "Class /Script/Pal.PalMonsterCharacter"
end
for index = 1, 4 do
    local component = { Trainer = nil, NPCSpawnedOtomoTrainer = nil }
    function component:IsDead()
        return false
    end

    local parameter = {
        SaveParameter = {
            IsPlayer = false,
            OwnerPlayerUId = { A = 0, B = 0, C = 0, D = 0 },
        },
    }
    function parameter:GetLevel()
        return index
    end

    local actor = {}
    function actor:GetFullName()
        return "PalMonsterCharacter /Game/Test.PersistentLevel.Pal_" .. tostring(index)
    end
    function actor:GetAddress()
        return 0x1000 + index
    end
    function actor:GetClass()
        return pal_runtime_class
    end
    function actor:IsA(class_object)
        return class_object.path == "/Script/Pal.PalMonsterCharacter"
    end
    function actor:GetCharacterParameterComponent()
        return component
    end
    function actor:GetIndividualParameter()
        return parameter
    end
    function actor:IsInitialized()
        return true
    end
    function actor:K2_GetActorLocation()
        return { X = index * 100, Y = 0, Z = 0 }
    end
    monsters[#monsters + 1] = actor
    static_objects["/Game/Test.PersistentLevel.Pal_" .. tostring(index)] = actor
end

local event_component = { Trainer = nil, NPCSpawnedOtomoTrainer = nil }
function event_component:IsDead()
    return false
end
local event_parameter = {
    SaveParameter = {
        IsPlayer = false,
        OwnerPlayerUId = { A = 0, B = 0, C = 0, D = 0 },
    },
}
local event_level = 5
local event_x = 500
function event_parameter:GetLevel()
    return event_level
end
local event_actor = {}
function event_actor:GetFullName()
    return "PalMonsterCharacter /Game/Test.PersistentLevel.EventPal_1"
end
function event_actor:GetAddress()
    return 0x2001
end
function event_actor:GetClass()
    return pal_runtime_class
end
function event_actor:IsA(class_object)
    return class_object.path == "/Script/Pal.PalMonsterCharacter"
end
function event_actor:GetCharacterParameterComponent()
    return event_component
end
function event_actor:GetIndividualParameter()
    return event_parameter
end
function event_actor:IsInitialized()
    return true
end
function event_actor:K2_GetActorLocation()
    return { X = event_x, Y = 0, Z = 0 }
end
function event_component:GetOwner()
    return event_actor
end
static_objects["/Game/Test.PersistentLevel.EventPal_1"] = event_actor

local delayed_event_parameter_ready = false
local delayed_event_initialized = false
local delayed_event_x = 600
local delayed_event_actor = {}
function delayed_event_actor:GetFullName()
    return "PalMonsterCharacter /Game/Test.PersistentLevel.DelayedEventPal_1"
end
function delayed_event_actor:GetAddress()
    return 0x2002
end
function delayed_event_actor:GetClass()
    return pal_runtime_class
end
function delayed_event_actor:IsA(class_object)
    return class_object.path == "/Script/Pal.PalMonsterCharacter"
end
function delayed_event_actor:GetCharacterParameterComponent()
    return event_component
end
function delayed_event_actor:GetIndividualParameter()
    if delayed_event_parameter_ready then
        return event_parameter
    end
    return nil
end
function delayed_event_actor:IsInitialized()
    return delayed_event_initialized
end
function delayed_event_actor:K2_GetActorLocation()
    return { X = delayed_event_x, Y = 0, Z = 0 }
end
static_objects["/Game/Test.PersistentLevel.DelayedEventPal_1"] = delayed_event_actor

local cancelled_event_ready = false
local cancelled_event_actor = {}
function cancelled_event_actor:GetFullName()
    return "PalMonsterCharacter /Game/Test.PersistentLevel.CancelledEventPal_1"
end
function cancelled_event_actor:GetAddress()
    return 0x2003
end
function cancelled_event_actor:GetClass()
    return pal_runtime_class
end
function cancelled_event_actor:IsA(class_object)
    return class_object.path == "/Script/Pal.PalMonsterCharacter"
end
function cancelled_event_actor:GetCharacterParameterComponent()
    return event_component
end
function cancelled_event_actor:GetIndividualParameter()
    if cancelled_event_ready then
        return event_parameter
    end
    return nil
end
function cancelled_event_actor:IsInitialized()
    return true
end
function cancelled_event_actor:K2_GetActorLocation()
    return { X = 700, Y = 0, Z = 0 }
end
static_objects["/Game/Test.PersistentLevel.CancelledEventPal_1"] = cancelled_event_actor

local function make_reused_path_actor(path_name, address, x, parameter_ready)
    local component = { Trainer = nil, NPCSpawnedOtomoTrainer = nil }
    function component:IsDead()
        return false
    end
    local parameter = {
        SaveParameter = {
            IsPlayer = false,
            OwnerPlayerUId = { A = 0, B = 0, C = 0, D = 0 },
        },
    }
    function parameter:GetLevel()
        return 6
    end
    local actor = {}
    function actor:GetFullName()
        return "PalMonsterCharacter /Game/Test.PersistentLevel." .. path_name
    end
    function actor:GetAddress()
        return address
    end
    function actor:GetClass()
        return pal_runtime_class
    end
    function actor:IsA(class_object)
        return class_object.path == "/Script/Pal.PalMonsterCharacter"
    end
    function actor:GetCharacterParameterComponent()
        return component
    end
    function actor:GetIndividualParameter()
        return parameter_ready and parameter or nil
    end
    function actor:IsInitialized()
        return true
    end
    function actor:K2_GetActorLocation()
        return { X = x, Y = 0, Z = 0 }
    end
    function component:GetOwner()
        return actor
    end
    return actor
end

local reused_path_actor_old = make_reused_path_actor("ReusedPathPal_1", 0x3001, 10, true)
local reused_path_actor_new = make_reused_path_actor("ReusedPathPal_1", 0x3002, 20, true)
static_objects["/Game/Test.PersistentLevel.ReusedPathPal_1"] = reused_path_actor_old
local queued_path_actor_old = make_reused_path_actor("QueuedReusedPathPal_1", 0x3101, 30, false)
local queued_path_actor_new = make_reused_path_actor("QueuedReusedPathPal_1", 0x3102, 40, true)
static_objects["/Game/Test.PersistentLevel.QueuedReusedPathPal_1"] = queued_path_actor_old
local reactivated_actor = make_reused_path_actor("ReactivatedPal_1", 0x3201, 50, true)
local reactivated_active = true
function reactivated_actor:GetActiveActorFlag()
    return reactivated_active
end
static_objects["/Game/Test.PersistentLevel.ReactivatedPal_1"] = reactivated_actor
local missed_stream_actor = make_reused_path_actor("MissedStreamPal_1", 0x3202, 120, true)
static_objects["/Game/Test.PersistentLevel.MissedStreamPal_1"] = missed_stream_actor

local bridge_class = {}
function bridge_class:GetFullName()
    return "BlueprintGeneratedClass /Game/Mods/PalworldResourceESP/ModActor.ModActor_C"
end
local panel_toggle_count = 0
local bridge_actor = {
    ESP_ControlRevision = 0,
    ESP_RuntimeEnabled = true,
    ESP_ProfileId = 2,
    ESP_PresetId = 1,
    ESP_CaptureRequested = false,
    ESP_LanguageId = 0,
    ESP_LevelMin = 0,
    ESP_LevelMax = 0,
    -- __DEPRECATED_20260717__ Legacy property remains only to prove the runtime ignores it.
    ESP_DistanceMin = 999,
    ESP_DistanceMax = 330,
    ESP_ShowTopGuideLine = true,
    ESP_ShowName = true,
    ESP_ShowLevel = true,
    ESP_ShowDistance = true,
    ESP_ShowIV = false,
    ESP_ShowPassiveSkills = false,
    ESP_IvMin = 0,
    ESP_IvHpMin = 0,
    ESP_IvAttackMin = 0,
    ESP_IvDefenseMin = 0,
    ESP_PassiveIncludeText = "",
    ESP_PassiveExcludeText = "",
    ESP_PassiveRainbowExpanded = false,
    ESP_PassiveLegendExpanded = false,
    ESP_PassiveGold3Expanded = false,
    ESP_PassiveGold2Expanded = false,
    ESP_PassiveNormalExpanded = false,
    ESP_PassiveNegative1Expanded = false,
    ESP_PassiveNegative2Expanded = false,
    ESP_PassiveNegative3Expanded = false,
    ESP_PassiveFilterRevision = 0,
    ESP_GenderFilterId = 0,
    ESP_LuckyFilterId = 0,
    ESP_BossFilterId = 0,
    ESP_CollectionFilterId = 0,
    ESP_SpeciesFilterText = "",
    ESP_ElementNormal = false,
    ESP_ElementFire = false,
    ESP_ElementWater = false,
    ESP_ElementLeaf = false,
    ESP_ElementElectricity = false,
    ESP_ElementIce = false,
    ESP_ElementEarth = false,
    ESP_ElementDark = false,
    ESP_ElementDragon = false,
    ESP_ElementFilterMask = 0,
    ESP_DisplayTargetLimit = 64,
    ESP_BridgeGenderDiagnosticCode = 0,
}
local bridge_target_payloads = {}
local bridge_style_payloads = {}
local bridge_removed_targets = {}
local bridge_clear_target_count = 0
function bridge_actor:GetClass()
    return bridge_class
end
function bridge_actor:PalworldResourceESP_ResetSession() end
function bridge_actor:PalworldResourceESP_SetTarget(actor, session_index, level, distance_meters)
    bridge_target_payloads[#bridge_target_payloads + 1] = {
        actor = actor,
        session_index = session_index,
        level = level,
        distance_meters = distance_meters,
    }
end
function bridge_actor:PalworldResourceESP_ClearTarget()
    bridge_clear_target_count = bridge_clear_target_count + 1
end
function bridge_actor:PalworldResourceESP_RemoveTarget(actor, session_index)
    bridge_removed_targets[#bridge_removed_targets + 1] = {
        actor = actor,
        session_index = session_index,
    }
end
local function bridge_removal_count(actor)
    local count = 0
    for _, removal in ipairs(bridge_removed_targets) do
        if removal.actor == actor then
            count = count + 1
        end
    end
    return count
end
local persisted_panel_restore_count = 0
function bridge_actor:PalworldResourceESP_ApplyPersistedPanelState()
    persisted_panel_restore_count = persisted_panel_restore_count + 1
    self.ESP_PassiveFilterRevision = self.ESP_PassiveFilterRevision + 1
end
function bridge_actor:PalworldResourceESP_SetDisplayStyle(show_top, show_name, show_level, show_distance, show_iv, show_passives, iv_min, iv_hp_min, iv_attack_min, iv_defense_min, passive_filter_revision, gender_filter_id, lucky_filter_id, boss_filter_id, collection_filter_id, species_filter_text, element_filter_mask, language_id)
    bridge_style_payloads[#bridge_style_payloads + 1] = {
        show_top = show_top,
        show_name = show_name,
        show_level = show_level,
        show_distance = show_distance,
        show_iv = show_iv,
        show_passives = show_passives,
        iv_min = iv_min,
        iv_hp_min = iv_hp_min,
        iv_attack_min = iv_attack_min,
        iv_defense_min = iv_defense_min,
        passive_filter_revision = passive_filter_revision,
        gender_filter_id = gender_filter_id,
        lucky_filter_id = lucky_filter_id,
        boss_filter_id = boss_filter_id,
        collection_filter_id = collection_filter_id,
        species_filter_text = species_filter_text,
        element_filter_mask = element_filter_mask,
        language_id = language_id,
    }
end
function bridge_actor:PalworldResourceESP_TogglePanel()
    panel_toggle_count = panel_toggle_count + 1
end

local request_toggle_during_monster_scan = false
local monster_find_all_count = 0
FindAllOf = function(class_name)
    if class_name == "PalPlayerCharacter" then
        return { player }
    end
    if class_name == "PalMonsterCharacter" then
        monster_find_all_count = monster_find_all_count + 1
        if request_toggle_during_monster_scan then
            request_toggle_during_monster_scan = false
            panel_keybind()
        end
        return monsters
    end
    return {}
end

local original_debug_getinfo = debug.getinfo
debug.getinfo = function()
    return { source = "=[C]" }
end
dofile([[${runtimeEntrypoint}]])
debug.getinfo = original_debug_getinfo
require("config").NEAREST_TARGET_REFRESH_INTERVAL_MS = 0
require("config").STREAM_INTEGRITY_SCAN_DISTANCE_METERS = 1
require("config").STREAM_INTEGRITY_SCAN_MIN_INTERVAL_MS = 0
require("config").ATOMIC_REBUILD_DEBOUNCE_MS = 0
require("config").ATOMIC_REBUILD_MOVEMENT_DEBOUNCE_MS = 0
require("config").ATOMIC_REBUILD_MAX_COALESCE_MS = 0
require("config").ATOMIC_REBUILD_MIN_INTERVAL_MS = 0
print("Runtime entrypoint load passed")

assert(runtime_settings_open_path == [[${normalize(path.join(runtimeRoot, "user-settings.log"))}]],
    "runtime did not resolve user settings beside config.lua")

assert(type(reconcile_loop) == "function", "reconcile loop was not captured")
local startup_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(startup_callbacks) do
    callback()
end
assert(#delayed_callbacks == 0, "safe reconcile retained actors in delayed callbacks")

local found_safe_result = false
for _, message in ipairs(runtime_logs) do
    if message:match("RECONCILE_IMMEDIATE.*wrapper_lifetime_safety") then
        found_safe_result = true
        break
    end
end
assert(found_safe_result, "wrapper-safe reconcile result was not logged")
print("Wrapper-safe reconcile stub passed")

assert(type(bridge_begin_play_hook) == "function", "bridge discovery hook was not captured")
assert(type(panel_keybind) == "function", "Shift+Y keybind was not captured")
assert(panel_key == Key.Y, "panel keybind did not use Y")
assert(panel_modifiers[1] == ModifierKey.SHIFT, "panel keybind did not require Shift")
bridge_begin_play_hook(bridge_actor)
assert(bridge_actor.ESP_ControlRevision == 1, "saved settings were not applied to the bridge")
assert(persisted_panel_restore_count == 1, "saved passive panel state was not rebuilt on the bridge")
assert(#bridge_target_payloads == 4, "bridge did not receive all initial display payloads")
assert(bridge_target_payloads[1].level == 1, "bridge level metadata did not match the snapshot")
assert(bridge_target_payloads[1].distance_meters == 1, "bridge distance metadata did not match the snapshot")
panel_keybind()
assert(panel_toggle_count == 0, "panel toggle ran inside the key callback")
assert(#delayed_callbacks == 0, "panel toggle scheduled a competing delayed GameThread callback")
reconcile_loop()
assert(panel_toggle_count == 1, "Shift+Y did not call the panel toggle bridge")
panel_keybind()
assert(panel_toggle_count == 1, "second panel toggle ran inside the key callback")
assert(#delayed_callbacks == 0, "second panel toggle scheduled a competing delayed GameThread callback")
reconcile_loop()
assert(panel_toggle_count == 2, "second Shift+Y did not call the panel toggle bridge")

reconcile_loop()
local scans_after_bootstrap = monster_find_all_count
for _ = 1, 3 do
    fake_time = fake_time + 6
    reconcile_loop()
end
assert(monster_find_all_count == scans_after_bootstrap,
    "event-driven steady state performed a periodic FindAllOf scan")
panel_keybind()
reconcile_loop()
assert(panel_toggle_count == 3, "event-driven runtime tick did not consume the third panel toggle")
--[[ __DEPRECATED_20260720__ Periodic reconciliation no longer exists, so a key request cannot race its scan.
request_toggle_during_monster_scan = true
]]

local toggle_requested_found = false
local toggle_completed_found = false
for _, message in ipairs(runtime_logs) do
    toggle_requested_found = toggle_requested_found or message:match("PANEL_TOGGLE_REQUESTED.*key=Shift%+Y") ~= nil
    toggle_completed_found = toggle_completed_found or message:match("PANEL_TOGGLE_COMPLETED.*status=ok") ~= nil
end
assert(toggle_requested_found and toggle_completed_found, "panel toggle diagnostics were incomplete")

bridge_actor.ESP_CaptureRequested = true
bridge_actor.ESP_ControlRevision = 1
reconcile_loop()
bridge_actor.ESP_RuntimeEnabled = false
bridge_actor.ESP_ControlRevision = 2
reconcile_loop()
assert(#delayed_callbacks == 0, "runtime-off mode scheduled discovery work")
assert(type(monster_notification) == "function", "monster notification was not captured")
monster_notification(event_actor)
assert(#delayed_callbacks == 0, "runtime-off notification scheduled admission work")

bridge_actor.ESP_CaptureRequested = false
bridge_actor.ESP_ControlRevision = 3
reconcile_loop()
bridge_actor.ESP_RuntimeEnabled = true
bridge_actor.ESP_ProfileId = 3
bridge_actor.ESP_ControlRevision = 4
reconcile_loop()
assert(#delayed_callbacks == 0, "profile entry retained actors in delayed callbacks")
bridge_actor.ESP_DisplayTargetLimit = 5
bridge_actor.ESP_ControlRevision = 5
reconcile_loop()
local scans_before_notification = monster_find_all_count
monster_notification(event_actor)
assert(#delayed_callbacks == 0, "event-first notification retained a UObject wrapper")
reconcile_loop()
assert(monster_find_all_count == scans_before_notification,
    "construction notification requested a full snapshot")
--[=[ __DEPRECATED_20260721__ [reason: lifecycle deltas now coalesce into one atomic full-target rebuild]
local payload_count_before_begin_play = #bridge_target_payloads
local static_finds_before_begin_play = static_actor_find_count
bridge_begin_play_hook(event_actor)
assert(#bridge_target_payloads == payload_count_before_begin_play + 1,
    "BeginPlay did not append exactly one event target")
assert(static_actor_find_count == static_finds_before_begin_play,
    "admission below the visible-target limit resolved existing candidate paths")

local capture_hook = registered_hooks["/Script/Pal.PalUtility:PalCaptureSuccess"].pre
local dead_hook = registered_hooks["/Script/Pal.PalCharacter:OnDeadCharacter"].pre
local end_play_hook = registered_hooks["/Script/Engine.Actor:ReceiveEndPlay"].pre
local parameter_replication_hook = registered_hooks[
    "/Script/Pal.PalCharacterParameterComponent:OnRep_IndividualParameter"
].post
local initialization_complete_hook = registered_hooks[
    "/Script/Pal.PalCharacter:BroadcastOnCompleteInitializeParameter"
].post
local active_actor_hook = registered_hooks["/Script/Pal.PalCharacter:OnRep_IsPalActiveActor"].post
local local_initialized_hook = registered_hooks["/Script/Pal.PalCharacter:LocalInitialized"].post
local parameter_initialized_hook = registered_hooks[
    "/Script/Pal.PalCharacterParameterComponent:OnInitialize_AfterSetIndividualParameter"
].post
assert(type(capture_hook) == "function" and type(dead_hook) == "function" and type(end_play_hook) == "function",
    "target lifecycle hooks were not registered")
assert(type(parameter_replication_hook) == "function" and type(initialization_complete_hook) == "function",
    "parameter lifecycle post-hooks were not registered")
assert(type(active_actor_hook) == "function" and type(local_initialized_hook) == "function"
        and type(parameter_initialized_hook) == "function",
    "pooled-actor lifecycle post-hooks were not registered")

local payload_count_before_path_reuse = #bridge_target_payloads
bridge_begin_play_hook(reused_path_actor_old)
static_objects["/Game/Test.PersistentLevel.ReusedPathPal_1"] = reused_path_actor_new
bridge_begin_play_hook(reused_path_actor_new)
assert(#bridge_target_payloads == payload_count_before_path_reuse + 2,
    "same-path replacement actor was rejected as a duplicate instance")
dead_hook(reused_path_actor_old)
dead_hook(reused_path_actor_new)

local payload_count_before_queued_path_reuse = #bridge_target_payloads
bridge_begin_play_hook(queued_path_actor_old)
assert(#delayed_callbacks == 1,
    "not-ready reused-path actor did not schedule one readiness retry")
static_objects["/Game/Test.PersistentLevel.QueuedReusedPathPal_1"] = queued_path_actor_new
local queued_path_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(queued_path_callbacks) do
    callback()
end
assert(#bridge_target_payloads == payload_count_before_queued_path_reuse
        and #delayed_callbacks == 0,
    "stale queued path admitted a replacement actor instance")
bridge_begin_play_hook(queued_path_actor_new)
assert(#bridge_target_payloads == payload_count_before_queued_path_reuse + 1,
    "replacement actor was not admitted through its own BeginPlay")
dead_hook(queued_path_actor_new)

local payload_count_before_refresh = #bridge_target_payloads
local removal_count_before_refresh = #bridge_removed_targets
parameter_replication_hook(event_component)
assert(#bridge_target_payloads == payload_count_before_refresh
        and #bridge_removed_targets == removal_count_before_refresh,
    "parameter replication refreshed metadata before initialization completed")
assert(#delayed_callbacks == 1,
    "parameter replication did not schedule one path-based initialization refresh")
event_level = 55
initialization_complete_hook(event_actor)
assert(#bridge_target_payloads == payload_count_before_refresh + 1,
    "initialization completion did not rebuild the displayed target")
assert(#bridge_removed_targets == removal_count_before_refresh + 1,
    "initialization completion did not remove exactly one stale metadata entry")
assert(bridge_target_payloads[#bridge_target_payloads].actor == event_actor
        and bridge_target_payloads[#bridge_target_payloads].level == 55,
    "initialization completion did not submit current metadata for the same actor")
local stale_refresh_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(stale_refresh_callbacks) do
    callback()
end
assert(#bridge_target_payloads == payload_count_before_refresh + 1
        and #bridge_removed_targets == removal_count_before_refresh + 1
        and #delayed_callbacks == 0,
    "successful initialization refresh did not cancel its pending path retry")

local payload_count_before_delayed = #bridge_target_payloads
local scans_before_delayed = monster_find_all_count
local removals_before_delayed = #bridge_removed_targets
local delayed_event_path = "/Game/Test.PersistentLevel.DelayedEventPal_1"
local static_finds_before_delayed = static_actor_find_counts[delayed_event_path] or 0
bridge_begin_play_hook(delayed_event_actor)
assert(#bridge_target_payloads == payload_count_before_delayed,
    "not-ready BeginPlay target was admitted before its parameter existed")
assert(#delayed_callbacks == 1,
    "not-ready BeginPlay target did not schedule one path-based retry")
delayed_event_parameter_ready = true
local readiness_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(readiness_callbacks) do
    callback()
end
assert(#bridge_target_payloads == payload_count_before_delayed,
    "parameter-ready target was admitted before character initialization completed")
assert(#delayed_callbacks == 1,
    "character initialization gate did not retain one bounded path retry")
delayed_event_initialized = true
readiness_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(readiness_callbacks) do
    callback()
end
assert(#bridge_target_payloads == payload_count_before_delayed,
    "farther initialized target incorrectly replaced a nearer displayed target")
assert(static_actor_find_counts[delayed_event_path] >= static_finds_before_delayed + 2,
    "readiness retries did not resolve the target by object path")
assert(monster_find_all_count == scans_before_delayed,
    "readiness retry performed a global FindAllOf scan")
assert(#bridge_removed_targets == removals_before_delayed,
    "farther initialized target evicted a nearer target by recency")

delayed_event_x = 50
for _ = 1, 8 do
    local static_finds_before_tick = static_actor_find_count
    reconcile_loop()
    assert(static_actor_find_count - static_finds_before_tick <= 2,
        "one nearest-N runtime tick exceeded the fixed path-resolution budget")
    if #bridge_target_payloads == payload_count_before_delayed + 1 then
        break
    end
end
assert(#bridge_target_payloads == payload_count_before_delayed + 1,
    "new nearest target did not enter the display set within the bounded convergence window")
assert(#bridge_removed_targets == removals_before_delayed + 1
        and bridge_removed_targets[#bridge_removed_targets].actor == event_actor,
    "nearest-N update did not remove the current farthest target")

event_x = 25
for _ = 1, 8 do
    local static_finds_before_tick = static_actor_find_count
    reconcile_loop()
    assert(static_actor_find_count - static_finds_before_tick <= 2,
        "distance reversal exceeded the fixed per-tick path-resolution budget")
    if #bridge_target_payloads == payload_count_before_delayed + 2 then
        break
    end
end
assert(#bridge_target_payloads == payload_count_before_delayed + 2,
    "distance order reversal did not converge on the now-nearest target")
assert(#bridge_removed_targets == removals_before_delayed + 2
        and bridge_removed_targets[#bridge_removed_targets].actor == monsters[4],
    "distance order reversal did not remove the new farthest boundary target")
assert(monster_find_all_count == scans_before_delayed,
    "nearest-N runtime updates performed a global FindAllOf scan")

local payload_count_before_cancel = #bridge_target_payloads
bridge_begin_play_hook(cancelled_event_actor)
assert(#delayed_callbacks == 1,
    "second not-ready target did not schedule a readiness retry")
dead_hook(cancelled_event_actor)
cancelled_event_ready = true
local cancelled_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(cancelled_callbacks) do
    callback()
end
assert(#bridge_target_payloads == payload_count_before_cancel,
    "death did not cancel a pending readiness retry")

local event_lifecycle_removals_before = bridge_removal_count(event_actor)
capture_hook(nil, nil, event_actor)
assert(bridge_removal_count(event_actor) == event_lifecycle_removals_before + 1,
    "capture did not remove the event target")
bridge_begin_play_hook(event_actor)
dead_hook(event_actor)
assert(bridge_removal_count(event_actor) == event_lifecycle_removals_before + 2,
    "death did not remove the event target")
bridge_begin_play_hook(event_actor)
end_play_hook(event_actor)
assert(bridge_removal_count(event_actor) == event_lifecycle_removals_before + 3,
    "EndPlay did not remove the event target")
bridge_begin_play_hook(event_actor)
monsters[#monsters + 1] = event_actor

local clear_count_before_repair = bridge_clear_target_count
local payload_count_before_repair = #bridge_target_payloads
local removal_count_before_repair = #bridge_removed_targets
local scans_before_repair = monster_find_all_count
static_objects["/Game/Test.PersistentLevel.Pal_2"] = nil
bridge_begin_play_hook(cancelled_event_actor)
for _ = 1, 40 do
    local static_finds_before_tick = static_actor_find_count
    reconcile_loop()
    assert(static_actor_find_count - static_finds_before_tick <= 2,
        "nearest repair exceeded the fixed per-tick path-resolution budget")
    if bridge_clear_target_count == clear_count_before_repair + 1
        and #bridge_target_payloads == payload_count_before_repair + 5 then
        break
    end
end
assert(bridge_clear_target_count == clear_count_before_repair + 1,
    "persistently unresolved nearest target did not clear the Blueprint target set")
assert(#bridge_removed_targets == removal_count_before_repair,
    "unresolved nearest target attempted removal with a guessed actor")
assert(#bridge_target_payloads == payload_count_before_repair + 5,
    "fail-closed nearest repair did not rebuild the five resolvable closest targets")
assert(monster_find_all_count == scans_before_repair,
    "fail-closed nearest repair performed a global FindAllOf scan")
assert(#delayed_callbacks == 0,
    "nearest repair scheduled an unnecessary delayed full snapshot")

local capture_start_found = false
local capture_mode_found = false
local capture_stop_found = false
local event_admission_found = false
local nearest_repair_found = false
for _, message in ipairs(runtime_logs) do
    capture_start_found = capture_start_found or message:match("PERF_SESSION_START") ~= nil
    capture_mode_found = capture_mode_found or message:match("PERF_MODE_CHANGED.*profile=off") ~= nil
    capture_stop_found = capture_stop_found or message:match("PERF_SESSION_STOP") ~= nil
    event_admission_found = event_admission_found or message:match("EVENT_TARGET_ADDED.*source=begin_play") ~= nil
    nearest_repair_found = nearest_repair_found or message:match("NEAREST_TARGETS_FAIL_CLOSED.*display_path_unresolved") ~= nil
end
assert(capture_start_found and capture_mode_found and capture_stop_found, "capture markers were incomplete")
assert(event_admission_found, "event-driven BeginPlay did not admit the new actor")
assert(nearest_repair_found, "fail-closed nearest repair diagnostic was not logged")
print("Panel controls and event-driven lifecycle passed")
]=]

local capture_hook = registered_hooks["/Script/Pal.PalUtility:PalCaptureSuccess"].pre
local dead_hook = registered_hooks["/Script/Pal.PalCharacter:OnDeadCharacter"].pre
local end_play_hook = registered_hooks["/Script/Engine.Actor:ReceiveEndPlay"].pre
local parameter_replication_hook = registered_hooks[
    "/Script/Pal.PalCharacterParameterComponent:OnRep_IndividualParameter"
].post
local initialization_complete_hook = registered_hooks[
    "/Script/Pal.PalCharacter:BroadcastOnCompleteInitializeParameter"
].post
local active_actor_hook = registered_hooks["/Script/Pal.PalCharacter:OnRep_IsPalActiveActor"].post
local local_initialized_hook = registered_hooks["/Script/Pal.PalCharacter:LocalInitialized"].post
local parameter_initialized_hook = registered_hooks[
    "/Script/Pal.PalCharacterParameterComponent:OnInitialize_AfterSetIndividualParameter"
].post
local component_initialized_hook = registered_hooks[
    "/Script/Pal.PalCharacterParameterComponent:OnInitializedCharacter"
].post
assert(type(capture_hook) == "function" and type(dead_hook) == "function" and type(end_play_hook) == "function",
    "target lifecycle hooks were not registered")
assert(type(parameter_replication_hook) == "function" and type(initialization_complete_hook) == "function"
        and type(active_actor_hook) == "function" and type(local_initialized_hook) == "function"
        and type(parameter_initialized_hook) == "function" and type(component_initialized_hook) == "function",
    "Pal lifecycle post-hooks were not registered")

local function remove_monster(target)
    for index, actor in ipairs(monsters) do
        if actor == target then
            table.remove(monsters, index)
            return true
        end
    end
    return false
end

--[==[ __DEPRECATED_20260721__ [reason: lifecycle events no longer coalesce into a full FindAllOf snapshot]
monsters[#monsters + 1] = event_actor
local scans_before_lifecycle_burst = monster_find_all_count
local clears_before_lifecycle_burst = bridge_clear_target_count
local payloads_before_lifecycle_burst = #bridge_target_payloads
local removals_before_lifecycle_burst = #bridge_removed_targets
bridge_begin_play_hook(event_actor)
parameter_replication_hook(event_component)
parameter_initialized_hook(event_component, event_actor)
component_initialized_hook(event_component, event_actor)
initialization_complete_hook(event_actor)
assert(monster_find_all_count == scans_before_lifecycle_burst
        and bridge_clear_target_count == clears_before_lifecycle_burst
        and #bridge_target_payloads == payloads_before_lifecycle_burst
        and #bridge_removed_targets == removals_before_lifecycle_burst
        and #delayed_callbacks == 0,
    "lifecycle hook performed synchronous bridge or discovery work")
reconcile_loop()
assert(monster_find_all_count == scans_before_lifecycle_burst + 1,
    "lifecycle burst did not coalesce into exactly one discovery pass")
assert(bridge_clear_target_count == clears_before_lifecycle_burst + 1
        and #bridge_target_payloads == payloads_before_lifecycle_burst + 5,
    "lifecycle burst did not atomically clear and rebuild all five targets")
assert(#bridge_removed_targets == removals_before_lifecycle_burst,
    "atomic lifecycle rebuild called single-target RemoveTarget")

local scans_before_parameter_burst = monster_find_all_count
local clears_before_parameter_burst = bridge_clear_target_count
local payloads_before_parameter_burst = #bridge_target_payloads
parameter_replication_hook(event_component)
parameter_initialized_hook(event_component, event_actor)
component_initialized_hook(event_component, event_actor)
local_initialized_hook(event_actor)
initialization_complete_hook(event_actor)
assert(monster_find_all_count == scans_before_parameter_burst
        and bridge_clear_target_count == clears_before_parameter_burst,
    "parameter hook burst rebuilt before the runtime queue drained")
reconcile_loop()
assert(monster_find_all_count == scans_before_parameter_burst + 1
        and bridge_clear_target_count == clears_before_parameter_burst + 1
        and #bridge_target_payloads == payloads_before_parameter_burst + 5,
    "duplicate parameter hooks were not coalesced into one atomic rebuild")

assert(remove_monster(event_actor), "event actor was missing before death simulation")
local scans_before_death = monster_find_all_count
local clears_before_death = bridge_clear_target_count
local payloads_before_death = #bridge_target_payloads
local removals_before_death = #bridge_removed_targets
dead_hook(event_actor)
assert(bridge_clear_target_count == clears_before_death
        and #bridge_target_payloads == payloads_before_death
        and #bridge_removed_targets == removals_before_death,
    "death hook mutated Blueprint targets before the atomic rebuild")
reconcile_loop()
assert(monster_find_all_count == scans_before_death + 1
        and bridge_clear_target_count == clears_before_death + 1
        and #bridge_target_payloads == payloads_before_death + 4,
    "death did not atomically rebuild the four surviving targets")
assert(#bridge_removed_targets == removals_before_death,
    "death called the unsafe single-target RemoveTarget path")

monsters[#monsters + 1] = event_actor
bridge_begin_play_hook(event_actor)
reconcile_loop()
local payloads_before_nearest = #bridge_target_payloads
local clears_before_nearest = bridge_clear_target_count
local scans_before_nearest = monster_find_all_count
delayed_event_parameter_ready = true
delayed_event_initialized = true
delayed_event_x = 50
monsters[#monsters + 1] = delayed_event_actor
bridge_begin_play_hook(delayed_event_actor)
reconcile_loop()
assert(monster_find_all_count == scans_before_nearest + 1
        and bridge_clear_target_count == clears_before_nearest + 1
        and #bridge_target_payloads == payloads_before_nearest + 5,
    "nearest-N change did not use one atomic five-target rebuild")
local delayed_is_in_latest_batch = false
for index = #bridge_target_payloads - 4, #bridge_target_payloads do
    delayed_is_in_latest_batch = delayed_is_in_latest_batch
        or bridge_target_payloads[index].actor == delayed_event_actor
end
assert(delayed_is_in_latest_batch, "atomic rebuild did not select the current nearest target")
assert(#bridge_removed_targets == removals_before_death,
    "nearest-N rebuild called single-target RemoveTarget")
assert(remove_monster(delayed_event_actor), "delayed event actor cleanup failed")

local capture_start_found = false
local capture_mode_found = false
local capture_stop_found = false
local atomic_rebuild_found = false
for _, message in ipairs(runtime_logs) do
    capture_start_found = capture_start_found or message:match("PERF_SESSION_START") ~= nil
    capture_mode_found = capture_mode_found or message:match("PERF_MODE_CHANGED.*profile=off") ~= nil
    capture_stop_found = capture_stop_found or message:match("PERF_SESSION_STOP") ~= nil
    atomic_rebuild_found = atomic_rebuild_found or message:match("ATOMIC_REBUILD_COMPLETED") ~= nil
end
assert(capture_start_found and capture_mode_found and capture_stop_found, "capture markers were incomplete")
assert(atomic_rebuild_found, "atomic lifecycle rebuild diagnostic was not logged")
print("Panel controls and atomic lifecycle rebuild passed")

require("config").ATOMIC_REBUILD_DEBOUNCE_MS = 500
require("config").ATOMIC_REBUILD_MAX_COALESCE_MS = 1500
require("config").ATOMIC_REBUILD_MIN_INTERVAL_MS = 2500
local scans_before_coalesce_deadline = monster_find_all_count
initialization_complete_hook(event_actor)
reconcile_loop()
assert(monster_find_all_count == scans_before_coalesce_deadline,
    "atomic rebuild ignored its initial debounce")
fake_time = fake_time + 0.6
initialization_complete_hook(event_actor)
reconcile_loop()
assert(monster_find_all_count == scans_before_coalesce_deadline,
    "atomic rebuild ran before the renewed quiet window")
fake_time = fake_time + 0.6
initialization_complete_hook(event_actor)
reconcile_loop()
assert(monster_find_all_count == scans_before_coalesce_deadline,
    "atomic rebuild ran before the maximum coalescing deadline")
fake_time = fake_time + 0.4
reconcile_loop()
assert(monster_find_all_count == scans_before_coalesce_deadline + 1,
    "continuous lifecycle signals starved the atomic rebuild past its hard deadline")
require("config").ATOMIC_REBUILD_DEBOUNCE_MS = 0
require("config").ATOMIC_REBUILD_MAX_COALESCE_MS = 0
require("config").ATOMIC_REBUILD_MIN_INTERVAL_MS = 0
fake_time = fake_time + 3
runtime_settings_writes = {}
print("Atomic rebuild hard coalescing deadline passed")
]==]

local function drain_delayed_callbacks(max_batches)
    local batches = 0
    while #delayed_callbacks > 0 do
        batches = batches + 1
        assert(batches <= (max_batches or 64), "delayed callback queue did not converge")
        local callbacks = delayed_callbacks
        delayed_callbacks = {}
        for _, callback in ipairs(callbacks) do
            callback()
        end
    end
end

monsters[#monsters + 1] = event_actor
local scans_before_lifecycle_delta = monster_find_all_count
local clears_before_lifecycle_delta = bridge_clear_target_count
local payloads_before_lifecycle_delta = #bridge_target_payloads
local removals_before_lifecycle_delta = #bridge_removed_targets
bridge_begin_play_hook(event_actor)
parameter_replication_hook(event_component)
parameter_initialized_hook(event_component, event_actor)
component_initialized_hook(event_component, event_actor)
initialization_complete_hook(event_actor)
assert(monster_find_all_count == scans_before_lifecycle_delta
        and bridge_clear_target_count == clears_before_lifecycle_delta
        and #bridge_target_payloads == payloads_before_lifecycle_delta
        and #bridge_removed_targets == removals_before_lifecycle_delta
        and #delayed_callbacks == 1,
    "lifecycle burst did not coalesce into one path-only queue")
drain_delayed_callbacks()
reconcile_loop()
assert(monster_find_all_count == scans_before_lifecycle_delta,
    "lifecycle delta performed a global FindAllOf scan")
assert(bridge_clear_target_count == clears_before_lifecycle_delta + 1
        and #bridge_target_payloads == payloads_before_lifecycle_delta + 5,
    "lifecycle delta did not atomically submit the five cached targets")
assert(#bridge_removed_targets == removals_before_lifecycle_delta,
    "lifecycle delta called single-target RemoveTarget")

local scans_before_parameter_delta = monster_find_all_count
local clears_before_parameter_delta = bridge_clear_target_count
local payloads_before_parameter_delta = #bridge_target_payloads
parameter_replication_hook(event_component)
parameter_initialized_hook(event_component, event_actor)
component_initialized_hook(event_component, event_actor)
local_initialized_hook(event_actor)
initialization_complete_hook(event_actor)
assert(#delayed_callbacks == 1,
    "duplicate parameter hooks were not coalesced by composite instance key")
drain_delayed_callbacks()
reconcile_loop()
assert(monster_find_all_count == scans_before_parameter_delta
        and bridge_clear_target_count == clears_before_parameter_delta + 1
        and #bridge_target_payloads == payloads_before_parameter_delta + 5,
    "parameter refresh did not use one cached atomic metadata resubmit")

assert(remove_monster(event_actor), "event actor was missing before cached death simulation")
local scans_before_cached_death = monster_find_all_count
local clears_before_cached_death = bridge_clear_target_count
local payloads_before_cached_death = #bridge_target_payloads
local removals_before_cached_death = #bridge_removed_targets
dead_hook(event_actor)
assert(bridge_clear_target_count == clears_before_cached_death,
    "death hook synchronously mutated Blueprint arrays")
reconcile_loop()
assert(monster_find_all_count == scans_before_cached_death
        and bridge_clear_target_count == clears_before_cached_death + 1
        and #bridge_target_payloads == payloads_before_cached_death + 4,
    "death did not atomically resubmit the four cached survivors")
assert(#bridge_removed_targets == removals_before_cached_death,
    "death called the unsafe single-target RemoveTarget path")

monsters[#monsters + 1] = event_actor
bridge_begin_play_hook(event_actor)
drain_delayed_callbacks()
reconcile_loop()

bridge_actor.ESP_DisplayTargetLimit = 3
bridge_actor.ESP_ControlRevision = 6
reconcile_loop()
local nearest_scans_before = monster_find_all_count
local nearest_clears_before = bridge_clear_target_count
local nearest_payloads_before = #bridge_target_payloads
local nearest_removals_before = #bridge_removed_targets
local nearest_static_finds_before = static_actor_find_count
require("config").STREAM_INTEGRITY_SCAN_DISTANCE_METERS = 10000
camera_x = 400
reconcile_loop()
assert(monster_find_all_count == nearest_scans_before,
    "camera-distance reordering performed a global FindAllOf scan")
assert(bridge_clear_target_count == nearest_clears_before + 1
        and #bridge_target_payloads == nearest_payloads_before + 3,
    "camera-distance reordering did not atomically submit nearest three")
assert(static_actor_find_count - nearest_static_finds_before <= 5,
    "nearest-three update resolved more than refresh-budget plus N paths")
assert(#bridge_removed_targets == nearest_removals_before,
    "nearest-three update called single-target RemoveTarget")
camera_x = 0
reconcile_loop()
require("config").STREAM_INTEGRITY_SCAN_DISTANCE_METERS = 1
reconcile_loop()

local stale_path_payloads_before = #bridge_target_payloads
local stale_path_scans_before = monster_find_all_count
bridge_begin_play_hook(queued_path_actor_old)
assert(#delayed_callbacks == 1, "not-ready actor did not enqueue one scalar path retry")
static_objects["/Game/Test.PersistentLevel.QueuedReusedPathPal_1"] = queued_path_actor_new
drain_delayed_callbacks()
reconcile_loop()
assert(#bridge_target_payloads == stale_path_payloads_before
        and monster_find_all_count == stale_path_scans_before,
    "queued path admitted a replacement instance under the stale composite key")
bridge_begin_play_hook(queued_path_actor_new)
drain_delayed_callbacks()
reconcile_loop()
assert(monster_find_all_count == stale_path_scans_before,
    "replacement instance admission performed a global FindAllOf scan")
-- __DEPRECATED_20260721__ [reason: no direct test-only lifecycle mutation is required]
-- remove_bridge_target = remove_bridge_target

local cached_atomic_found = false
for _, message in ipairs(runtime_logs) do
    cached_atomic_found = cached_atomic_found
        or message:match("CACHED_TARGETS_ATOMIC_SYNCED") ~= nil
end
assert(cached_atomic_found, "cached atomic target-sync diagnostic was not logged")
runtime_settings_writes = {}
print("Cached lifecycle and nearest-N atomic presentation passed")

bridge_actor.ESP_RuntimeEnabled = true
bridge_actor.ESP_ProfileId = 3
bridge_actor.ESP_LevelMin = 3
bridge_actor.ESP_LevelMax = 4
-- __DEPRECATED_20260717__ A nonzero legacy minimum proves it no longer affects filtering.
bridge_actor.ESP_DistanceMin = 400
bridge_actor.ESP_DistanceMax = 500
bridge_actor.ESP_ControlRevision = 41
reconcile_loop()
local filtered_result_found = false
local filtered_config_found = false
for _, message in ipairs(runtime_logs) do
    filtered_result_found = filtered_result_found or message:match("FILTER_RESULT.*admitted=5.*matched=2") ~= nil
    filtered_config_found = filtered_config_found or message:match("FILTER_CONFIG.*level_min=3.*level_max=4.*distance_min=0.*distance_max=330") ~= nil
end
assert(filtered_result_found, "panel range filters did not update the matched output")
assert(filtered_config_found, "panel range filter configuration was not logged")
print("Panel range filters passed")

bridge_actor.ESP_LevelMin = 0
bridge_actor.ESP_LevelMax = 0
-- __DEPRECATED_20260717__ The removed bound stays ignored during filter reset.
bridge_actor.ESP_DistanceMin = 999
bridge_actor.ESP_DistanceMax = 330
bridge_actor.ESP_ControlRevision = 42
reconcile_loop()
local reset_filter_found = false
for _, message in ipairs(runtime_logs) do
    reset_filter_found = reset_filter_found or message:match("FILTER_RESULT.*admitted=5.*matched=5") ~= nil
end
assert(reset_filter_found, "panel level filters did not reset while distance remained 0-330m")
print("Panel fixed 0m distance lower bound passed")

bridge_actor.ESP_DistanceMax = 0
bridge_actor.ESP_ControlRevision = 43
reconcile_loop()
local zero_distance_range_found = false
for _, message in ipairs(runtime_logs) do
    zero_distance_range_found = zero_distance_range_found
        or message:match("FILTER_RESULT.*admitted=5.*matched=0") ~= nil
end
assert(zero_distance_range_found, "0m maximum distance did not produce the fixed 0-0m range")
bridge_actor.ESP_DistanceMax = 330
bridge_actor.ESP_ControlRevision = 44
reconcile_loop()
print("Panel 0-330m maximum distance passed")

bridge_actor.ESP_DisplayTargetLimit = 3
bridge_actor.ESP_ControlRevision = 45
reconcile_loop()
local display_limit_found = false
local display_limit_result_found = false
for _, message in ipairs(runtime_logs) do
    display_limit_found = display_limit_found or message:match("DISPLAY_TARGET_LIMIT.*value=3") ~= nil
    display_limit_result_found = display_limit_result_found
        or message:match("FILTER_RESULT.*admitted=5.*matched=5.*displayed=3") ~= nil
end
assert(display_limit_found and display_limit_result_found, "numeric display target limit was not applied")
bridge_actor.ESP_DisplayTargetLimit = 64
bridge_actor.ESP_ControlRevision = 46
reconcile_loop()
print("Panel numeric display target limit passed")

bridge_actor.ESP_DisplayTargetLimit = 999
bridge_actor.ESP_ControlRevision = 47
reconcile_loop()
local display_limit_clamped_found = false
for _, message in ipairs(runtime_logs) do
    display_limit_clamped_found = display_limit_clamped_found or message:match("DISPLAY_TARGET_LIMIT.*value=100") ~= nil
end
assert(display_limit_clamped_found, "display target limit was not clamped to 100")
bridge_actor.ESP_DisplayTargetLimit = 64
bridge_actor.ESP_ControlRevision = 48
reconcile_loop()
print("Panel display target ceiling passed")

bridge_actor.ESP_ShowTopGuideLine = false
bridge_actor.ESP_ControlRevision = 49
reconcile_loop()
bridge_actor.ESP_ShowTopGuideLine = true
bridge_actor.ESP_ControlRevision = 50
reconcile_loop()
bridge_actor.ESP_ShowLevel = false
bridge_actor.ESP_ShowDistance = false
bridge_actor.ESP_ControlRevision = 51
reconcile_loop()
bridge_actor.ESP_ShowName = false
bridge_actor.ESP_ControlRevision = 511
reconcile_loop()
bridge_actor.ESP_ShowName = true
bridge_actor.ESP_ControlRevision = 512
reconcile_loop()
bridge_actor.ESP_ShowLevel = true
bridge_actor.ESP_ShowDistance = true
bridge_actor.ESP_ControlRevision = 52
reconcile_loop()
bridge_actor.ESP_ShowIV = true
bridge_actor.ESP_ShowPassiveSkills = true
bridge_actor.ESP_IvHpMin = 75
bridge_actor.ESP_IvAttackMin = 76
bridge_actor.ESP_IvDefenseMin = 77
local function fstring_proxy(value)
    return {
        type = function() return "FString" end,
        ToString = function() return value end,
    }
end
bridge_actor.ESP_PassiveIncludeText = fstring_proxy("|Legend|Rare|")
bridge_actor.ESP_PassiveExcludeText = fstring_proxy("|PAL_Coward|")
bridge_actor.ESP_PassiveLegendExpanded = true
bridge_actor.ESP_PassiveNegative1Expanded = true
bridge_actor.ESP_PassiveFilterRevision = 1
bridge_actor.ESP_ControlRevision = 521
reconcile_loop()
bridge_actor.ESP_GenderFilterId = 1
bridge_actor.ESP_ControlRevision = 53
reconcile_loop()
bridge_actor.ESP_GenderFilterId = 2
bridge_actor.ESP_ControlRevision = 54
reconcile_loop()
bridge_actor.ESP_GenderFilterId = 99
bridge_actor.ESP_ControlRevision = 55
reconcile_loop()
bridge_actor.ESP_LuckyFilterId = 1
bridge_actor.ESP_ControlRevision = 56
reconcile_loop()
bridge_actor.ESP_LuckyFilterId = 2
bridge_actor.ESP_ControlRevision = 57
reconcile_loop()
bridge_actor.ESP_LuckyFilterId = 99
bridge_actor.ESP_ControlRevision = 58
reconcile_loop()
bridge_actor.ESP_BossFilterId = 1
bridge_actor.ESP_ControlRevision = 59
reconcile_loop()
bridge_actor.ESP_BossFilterId = 2
bridge_actor.ESP_ControlRevision = 60
reconcile_loop()
bridge_actor.ESP_BossFilterId = 99
bridge_actor.ESP_ControlRevision = 61
reconcile_loop()
bridge_actor.ESP_CollectionFilterId = 1
bridge_actor.ESP_ControlRevision = 62
reconcile_loop()
bridge_actor.ESP_CollectionFilterId = 2
bridge_actor.ESP_ControlRevision = 63
reconcile_loop()
bridge_actor.ESP_CollectionFilterId = 99
bridge_actor.ESP_ControlRevision = 64
reconcile_loop()
bridge_actor.ESP_SpeciesFilterText = "|SheepBall|Kitsunebi|"
bridge_actor.ESP_ControlRevision = 65
reconcile_loop()
bridge_actor.ESP_ElementFire = true
bridge_actor.ESP_ElementWater = true
bridge_actor.ESP_ControlRevision = 66
reconcile_loop()
local top_guide_hidden_found = false
local top_guide_shown_found = false
local metadata_hidden_found = false
local metadata_shown_found = false
local name_hidden_found = false
local name_shown_found = false
local iv_shown_found = false
local passives_shown_found = false
local iv_min_found = false
local gender_male_found = false
local gender_female_found = false
local gender_clamped_found = false
local lucky_only_found = false
local lucky_excluded_found = false
local lucky_clamped_found = false
local boss_only_found = false
local boss_excluded_found = false
local boss_clamped_found = false
local collection_incomplete_found = false
local collection_complete_found = false
local collection_clamped_found = false
local species_filter_found = false
local element_mask_found = false
for _, message in ipairs(runtime_logs) do
    top_guide_hidden_found = top_guide_hidden_found or message:match("DISPLAY_STYLE.*top_guide_line=false") ~= nil
    top_guide_shown_found = top_guide_shown_found or message:match("DISPLAY_STYLE.*top_guide_line=true") ~= nil
    metadata_hidden_found = metadata_hidden_found
        or message:match("DISPLAY_STYLE.*show_level=false.*show_distance=false") ~= nil
    metadata_shown_found = metadata_shown_found
        or message:match("DISPLAY_STYLE.*show_level=true.*show_distance=true") ~= nil
    name_hidden_found = name_hidden_found or message:match("DISPLAY_STYLE.*show_name=false") ~= nil
    name_shown_found = name_shown_found or message:match("DISPLAY_STYLE.*show_name=true") ~= nil
    iv_shown_found = iv_shown_found or message:match("DISPLAY_STYLE.*show_iv=true") ~= nil
    passives_shown_found = passives_shown_found or message:match("DISPLAY_STYLE.*show_passives=true") ~= nil
    iv_min_found = iv_min_found
        or message:match("DISPLAY_STYLE.*iv_hp_min=75.*iv_attack_min=76.*iv_defense_min=77.*passive_filter_revision=1") ~= nil
    gender_male_found = gender_male_found or message:match("DISPLAY_STYLE.*gender_filter=male") ~= nil
    gender_female_found = gender_female_found or message:match("DISPLAY_STYLE.*gender_filter=female") ~= nil
    gender_clamped_found = gender_clamped_found
        or message:match("DISPLAY_STYLE.*gender_filter=female") ~= nil
    lucky_only_found = lucky_only_found or message:match("DISPLAY_STYLE.*lucky_filter=only_lucky") ~= nil
    lucky_excluded_found = lucky_excluded_found or message:match("DISPLAY_STYLE.*lucky_filter=exclude_lucky") ~= nil
    lucky_clamped_found = lucky_clamped_found
        or message:match("DISPLAY_STYLE.*lucky_filter=exclude_lucky") ~= nil
    boss_only_found = boss_only_found or message:match("DISPLAY_STYLE.*boss_filter=only_boss") ~= nil
    boss_excluded_found = boss_excluded_found or message:match("DISPLAY_STYLE.*boss_filter=exclude_boss") ~= nil
    boss_clamped_found = boss_clamped_found
        or message:match("DISPLAY_STYLE.*boss_filter=exclude_boss") ~= nil
    collection_incomplete_found = collection_incomplete_found
        or message:match("DISPLAY_STYLE.*collection_filter=incomplete") ~= nil
    collection_complete_found = collection_complete_found
        or message:match("DISPLAY_STYLE.*collection_filter=complete") ~= nil
    collection_clamped_found = collection_clamped_found
        or message:match("DISPLAY_STYLE.*collection_filter=complete") ~= nil
    species_filter_found = species_filter_found
        or message:match("DISPLAY_STYLE.*species_filter_count=2") ~= nil
    element_mask_found = element_mask_found or message:match("DISPLAY_STYLE.*element_filter_mask=6") ~= nil
end
assert(top_guide_hidden_found and top_guide_shown_found, "panel top-guide style did not round-trip")
assert(metadata_hidden_found and metadata_shown_found, "panel metadata styles did not round-trip")
assert(name_hidden_found and name_shown_found, "panel name style did not round-trip")
assert(iv_shown_found, "panel IV display style did not round-trip")
assert(passives_shown_found, "panel passive-skill display style did not round-trip")
assert(iv_min_found, "panel IV minimum did not round-trip")
assert(gender_male_found and gender_female_found, "panel gender filters did not round-trip")
assert(gender_clamped_found, "invalid gender filter was not clamped")
assert(lucky_only_found and lucky_excluded_found, "panel Lucky filters did not round-trip")
assert(lucky_clamped_found, "invalid Lucky filter was not clamped")
assert(boss_only_found and boss_excluded_found, "panel Boss filters did not round-trip")
assert(boss_clamped_found, "invalid Boss filter was not clamped")
assert(collection_incomplete_found and collection_complete_found, "panel collection filters did not round-trip")
assert(collection_clamped_found, "invalid collection filter was not clamped")
assert(species_filter_found, "species filter mirror did not reach the display-style bridge")
assert(element_mask_found, "panel element toggles did not produce the Fire-or-Water mask")
assert(#bridge_style_payloads >= 4, "display styles were not sent through the actor-free bridge event")
assert(bridge_style_payloads[#bridge_style_payloads].gender_filter_id == 2, "gender filter clamp did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].lucky_filter_id == 2, "Lucky filter clamp did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].boss_filter_id == 2, "Boss filter clamp did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].collection_filter_id == 2, "collection filter clamp did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].species_filter_text == "|SheepBall|Kitsunebi|", "species filter mirror did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].element_filter_mask == 6, "element mask did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].language_id == 0, "Chinese language id did not reach the bridge")
assert(bridge_actor.ESP_ElementFilterMask == 6, "element mask was not synchronized to the passive bridge actor")
assert(bridge_style_payloads[#bridge_style_payloads].show_name == true, "name style did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].show_iv == true, "IV style did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].show_passives == true, "passive-skill style did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].iv_hp_min == 75, "HP IV minimum did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].iv_attack_min == 76, "attack IV minimum did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].iv_defense_min == 77, "defense IV minimum did not reach the bridge")
assert(bridge_style_payloads[#bridge_style_payloads].passive_filter_revision == 1, "passive filter revision did not reach the bridge")
bridge_actor.ESP_LanguageId = 1
bridge_actor.ESP_ControlRevision = 67
reconcile_loop()
assert(bridge_style_payloads[#bridge_style_payloads].language_id == 1, "English language id did not reach the bridge")
bridge_actor.ESP_LanguageId = 0
bridge_actor.ESP_ControlRevision = 68
reconcile_loop()
assert(bridge_style_payloads[#bridge_style_payloads].language_id == 0, "Chinese language id was not restored through the bridge")
print("Panel display styles passed")

bridge_actor.ESP_DisplayTargetLimit = 60
bridge_actor.ESP_ControlRevision = 69
reconcile_loop()
local budget_payloads_before = #bridge_target_payloads
local budget_clears_before = bridge_clear_target_count
local budget_scans_before = monster_find_all_count
local budget_removals_before = #bridge_removed_targets
for index = 1, 22 do
    local budget_actor = {}
    local object_path = "/Game/Test.PersistentLevel.BudgetPal_" .. tostring(index)
    function budget_actor:GetFullName()
        return "PalMonsterCharacter " .. object_path
    end
    function budget_actor:GetAddress()
        return 0x4000 + index
    end
    function budget_actor:GetClass()
        return pal_runtime_class
    end
    function budget_actor:IsA(class_object)
        return class_object.path == "/Script/Pal.PalMonsterCharacter"
    end
    function budget_actor:GetCharacterParameterComponent()
        return event_component
    end
    function budget_actor:GetIndividualParameter()
        return event_parameter
    end
    function budget_actor:IsInitialized()
        return true
    end
    function budget_actor:K2_GetActorLocation()
        return { X = 800 + index * 100, Y = 0, Z = 0 }
    end
    static_objects[object_path] = budget_actor
    monsters[#monsters + 1] = budget_actor
    bridge_begin_play_hook(budget_actor)
end
assert(#bridge_target_payloads == budget_payloads_before
        and bridge_clear_target_count == budget_clears_before
        and monster_find_all_count == budget_scans_before
        and #delayed_callbacks == 1,
    "22-actor BeginPlay burst performed synchronous work")
drain_delayed_callbacks()
reconcile_loop()
assert(monster_find_all_count == budget_scans_before
        and bridge_clear_target_count == budget_clears_before + 1
        and #bridge_target_payloads == budget_payloads_before + 27,
    "22-actor path queue did not coalesce into one cached atomic 27-target submit")
assert(#bridge_removed_targets == budget_removals_before,
    "below-limit cached submit called single-target RemoveTarget")
print("Below-limit 27-of-60 cached atomic submit passed")

local reactivated_payloads_before = #bridge_target_payloads
local reactivated_clears_before = bridge_clear_target_count
local scans_before_reactivation = monster_find_all_count
monsters[#monsters + 1] = reactivated_actor
active_actor_hook(reactivated_actor)
assert(#bridge_target_payloads == reactivated_payloads_before
        and monster_find_all_count == scans_before_reactivation
        and #delayed_callbacks == 1,
    "active pooled actor hook performed synchronous work")
drain_delayed_callbacks()
reconcile_loop()
assert(#bridge_target_payloads == reactivated_payloads_before + 28
        and bridge_clear_target_count == reactivated_clears_before + 1
        and monster_find_all_count == scans_before_reactivation,
    "active pooled actor did not trigger one cached atomic submit")
reactivated_active = false
local reactivated_removals_before = #bridge_removed_targets
local reactivated_inactive_payloads_before = #bridge_target_payloads
local reactivated_inactive_clears_before = bridge_clear_target_count
assert(remove_monster(reactivated_actor), "reactivated actor cleanup failed")
active_actor_hook(reactivated_actor)
reconcile_loop()
assert(#bridge_target_payloads == reactivated_inactive_payloads_before + 27
        and bridge_clear_target_count == reactivated_inactive_clears_before + 1
        and #bridge_removed_targets == reactivated_removals_before,
    "inactive pooled actor did not use an atomic survivor rebuild")

local integrity_payloads_before = #bridge_target_payloads
local integrity_clears_before = bridge_clear_target_count
local integrity_scans_before = monster_find_all_count
local integrity_static_finds_before = static_actor_find_count
local integrity_removals_before = #bridge_removed_targets
monsters[#monsters + 1] = missed_stream_actor
camera_x = 50
reconcile_loop()
assert(monster_find_all_count == integrity_scans_before,
    "sub-threshold movement triggered a stream-integrity scan")
camera_x = 120
reconcile_loop()
assert(monster_find_all_count == integrity_scans_before + 1,
    "cumulative movement did not trigger exactly one identity census")
assert(bridge_clear_target_count == integrity_clears_before
        and #delayed_callbacks == 1,
    "movement census synchronously admitted or presented an unknown actor")
drain_delayed_callbacks()
reconcile_loop()
assert(bridge_clear_target_count == integrity_clears_before + 1
        and #bridge_target_payloads == integrity_payloads_before + 28,
    "movement census did not atomically submit the admitted cached targets")
assert(#delayed_callbacks == 0
        and static_actor_find_count - integrity_static_finds_before <= 33,
    "movement census exceeded one admission plus refresh-budget plus N path resolves")
assert(#bridge_removed_targets == integrity_removals_before,
    "movement discovery called single-target RemoveTarget")
local integrity_scans_after = monster_find_all_count
for _ = 1, 3 do
    reconcile_loop()
end
assert(monster_find_all_count == integrity_scans_after,
    "stationary runtime repeated the stream-integrity scan")
local integrity_scan_logged = false
for _, message in ipairs(runtime_logs) do
    integrity_scan_logged = integrity_scan_logged
        or message:match("STREAM_INTEGRITY_CENSUS_COMPLETED") ~= nil
end
assert(integrity_scan_logged, "movement-integrity census diagnostics were missing")
print("Pooled lifecycle and movement identity census passed")

assert(type(load_map_pre_hook) == "function", "load-map pre-hook was not captured")
bridge_actor.ESP_ProfileId = 2
bridge_actor.ESP_ControlRevision = 5
reconcile_loop()
assert(#delayed_callbacks == 0, "profile change retained actors in delayed callbacks")
fake_time = fake_time + 6
local scan_done_count_before = 0
for _, message in ipairs(runtime_logs) do
    if message:match("SCAN_DONE") then
        scan_done_count_before = scan_done_count_before + 1
    end
end
reconcile_loop()
assert(#runtime_settings_writes == 1, "stable settings changes were not coalesced into one append")
assert(runtime_settings_writes[1]:match("^v12 "), "saved settings did not use the current versioned format")
assert(runtime_settings_writes[1]:match("show_name=true"), "saved settings omitted the name toggle")
assert(runtime_settings_writes[1]:match("lucky=2"), "saved settings omitted the Lucky filter")
assert(runtime_settings_writes[1]:match("boss=2"), "saved settings omitted the Boss filter")
assert(runtime_settings_writes[1]:match("collection=2"), "saved settings omitted the collection filter")
assert(runtime_settings_writes[1]:match("species_filters=|SheepBall|Kitsunebi|"), "saved settings omitted selected species")
assert(runtime_settings_writes[1]:match("element_fire=true"), "saved settings omitted the Fire element")
assert(runtime_settings_writes[1]:match("element_water=true"), "saved settings omitted the Water element")
assert(runtime_settings_writes[1]:match("element_dragon=false"), "saved settings omitted an unselected element")
assert(runtime_settings_writes[1]:match("show_iv=true"), "saved settings omitted the IV toggle")
assert(runtime_settings_writes[1]:match("iv_hp_min=75"), "saved settings omitted the HP IV minimum")
assert(runtime_settings_writes[1]:match("iv_attack_min=76"), "saved settings omitted the attack IV minimum")
assert(runtime_settings_writes[1]:match("iv_defense_min=77"), "saved settings omitted the defense IV minimum")
assert(runtime_settings_writes[1]:match("show_passives=true"), "saved settings omitted the passive-skill toggle")
assert(runtime_settings_writes[1]:match("passive_includes=|Legend|Rare|"), "saved settings omitted included passives")
assert(runtime_settings_writes[1]:match("passive_excludes=|PAL_Coward|"), "saved settings omitted excluded passives")
assert(runtime_settings_writes[1]:match("expand_legend=true"), "saved settings omitted Legend expansion")
assert(runtime_settings_writes[1]:match("expand_negative1=true"), "saved settings omitted negative-I expansion")
local persisted_panel_restore_count_before_transition = persisted_panel_restore_count
load_map_pre_hook()
bridge_actor.ESP_ProfileId = 0
bridge_actor.ESP_ShowName = false
bridge_actor.ESP_ShowIV = false
bridge_actor.ESP_ShowPassiveSkills = false
bridge_actor.ESP_IvHpMin = 0
bridge_actor.ESP_IvAttackMin = 0
bridge_actor.ESP_IvDefenseMin = 0
bridge_actor.ESP_PassiveIncludeText = ""
bridge_actor.ESP_PassiveExcludeText = ""
bridge_actor.ESP_PassiveLegendExpanded = false
bridge_actor.ESP_PassiveNegative1Expanded = false
bridge_actor.ESP_CollectionFilterId = 0
bridge_actor.ESP_SpeciesFilterText = ""
bridge_actor.ESP_ControlRevision = 0
bridge_begin_play_hook(bridge_actor)
assert(bridge_actor.ESP_ProfileId == 2, "save transition restored the startup profile instead of the latest profile")
assert(bridge_actor.ESP_ShowName == true, "save transition did not restore the latest name toggle")
assert(bridge_actor.ESP_ShowIV == true, "save transition did not restore the latest IV toggle")
assert(bridge_actor.ESP_ShowPassiveSkills == true, "save transition did not restore the latest passive toggle")
assert(bridge_actor.ESP_IvHpMin == 75, "save transition did not restore the latest HP IV minimum")
assert(bridge_actor.ESP_IvAttackMin == 76, "save transition did not restore the latest attack IV minimum")
assert(bridge_actor.ESP_IvDefenseMin == 77, "save transition did not restore the latest defense IV minimum")
assert(bridge_actor.ESP_PassiveIncludeText == "|Legend|Rare|", "save transition did not restore included passives")
assert(bridge_actor.ESP_PassiveExcludeText == "|PAL_Coward|", "save transition did not restore excluded passives")
assert(bridge_actor.ESP_PassiveLegendExpanded == true, "save transition did not restore Legend expansion")
assert(bridge_actor.ESP_PassiveNegative1Expanded == true, "save transition did not restore negative-I expansion")
assert(bridge_actor.ESP_CollectionFilterId == 2, "save transition did not restore the collection filter")
assert(bridge_actor.ESP_SpeciesFilterText == "|SheepBall|Kitsunebi|", "save transition did not restore selected species")
assert(persisted_panel_restore_count == persisted_panel_restore_count_before_transition + 1, "save transition did not rebuild passive panel state")
print("Latest in-memory settings survive save transitions")
assert(#delayed_callbacks == 0, "event-driven teardown retained actors in delayed callbacks")
local scan_done_count_after = 0
for _, message in ipairs(runtime_logs) do
    if message:match("SCAN_DONE") then
        scan_done_count_after = scan_done_count_after + 1
    end
end
assert(scan_done_count_after == scan_done_count_before,
    "event-driven steady state scanned during save transition")
-- __DEPRECATED_20260720__ assert(scan_done_count_after == scan_done_count_before + 1,
--     "periodic safe reconcile did not complete inline")
assert(#delayed_callbacks == 0, "map teardown left a stale actor callback")
print("Wrapper-safe map teardown passed")
`;

const L = lauxlib.luaL_newstate();
lualib.luaL_openlibs(L);
let status = lauxlib.luaL_loadstring(L, to_luastring(source));
if (status === lua.LUA_OK) {
  status = lua.lua_pcall(L, 0, lua.LUA_MULTRET, 0);
}
if (status !== lua.LUA_OK) {
  const message = to_jsstring(lua.lua_tostring(L, -1));
  throw new Error(message);
}

console.log(`Parsed Lua files: ${files.length}`);
console.log("Pure core runtime-global check passed");
console.log("Blueprint slider/name/outline/Lucky/Boss/element/IV/passive source contract passed");

if (process.platform === "win32") {
  const performanceTests = path.join(root, "tests", "performance", "run-performance-tests.ps1");
  const result = spawnSync(
    "powershell.exe",
    ["-NoProfile", "-ExecutionPolicy", "Bypass", "-File", performanceTests],
    { stdio: "inherit" },
  );
  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    throw new Error(`PowerShell performance tests exited with code ${result.status}`);
  }
}

process.exitCode = 0;
