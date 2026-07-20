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
if (!generatorSource.includes('TEXT("PalworldResourceESP_WidgetRemoveTarget")')
    || !generatorSource.includes('TEXT("PalworldResourceESP_RemoveTarget")')
    || !generatorSource.includes('TEXT("Array_Find")')
    || !generatorSource.includes('TEXT("Array_Remove")')
    || !mainSource.includes('BRIDGE_METHOD_REMOVE_TARGET = "PalworldResourceESP_RemoveTarget"')) {
  throw new Error("Event-driven single-target removal contract is incomplete");
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
if (!generatorSource.includes('TEXT("DisableInput")')
    || !generatorSource.includes('TEXT("EnableInput")')
    || !generatorSource.includes('TEXT("SetDisableInputFlag")')
    || !generatorSource.includes('TEXT("PalworldResourceESP_Panel")')
    || !generatorSource.includes('BuildHandledKeyOverride(Blueprint, TEXT("OnKeyDown"))')
    || !generatorSource.includes('BuildHandledKeyOverride(Blueprint, TEXT("OnKeyUp"))')
    || !generatorSource.includes('UWidgetBlueprintLibrary::StaticClass(), TEXT("Handled")')
    || !generatorSource.includes("bRefreshCatalogs")
    || !generatorSource.includes("RefreshPalCatalog")) {
  throw new Error("Panel input isolation or language-triggered catalog refresh contract is incomplete");
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
StaticFindObject = function(path)
    if static_objects[path] ~= nil then
        static_actor_find_count = static_actor_find_count + 1
        return static_objects[path]
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
RegisterHook = function(path, callback)
    registered_hooks[path] = callback
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

local camera_manager = {}
function camera_manager:GetCameraLocation()
    return { X = 0, Y = 0, Z = 0 }
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
    function actor:K2_GetActorLocation()
        return { X = index * 100, Y = 0, Z = 0 }
    end
    monsters[#monsters + 1] = actor
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
function event_parameter:GetLevel()
    return 5
end
local event_actor = {}
function event_actor:GetFullName()
    return "PalMonsterCharacter /Game/Test.PersistentLevel.EventPal_1"
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
function event_actor:K2_GetActorLocation()
    return { X = 500, Y = 0, Z = 0 }
end

local delayed_event_ready = false
local delayed_event_actor = {}
function delayed_event_actor:GetFullName()
    return "PalMonsterCharacter /Game/Test.PersistentLevel.DelayedEventPal_1"
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
    if delayed_event_ready then
        return event_parameter
    end
    return nil
end
function delayed_event_actor:K2_GetActorLocation()
    return { X = 600, Y = 0, Z = 0 }
end
static_objects["/Game/Test.PersistentLevel.DelayedEventPal_1"] = delayed_event_actor

local cancelled_event_ready = false
local cancelled_event_actor = {}
function cancelled_event_actor:GetFullName()
    return "PalMonsterCharacter /Game/Test.PersistentLevel.CancelledEventPal_1"
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
function cancelled_event_actor:K2_GetActorLocation()
    return { X = 700, Y = 0, Z = 0 }
end
static_objects["/Game/Test.PersistentLevel.CancelledEventPal_1"] = cancelled_event_actor

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
function bridge_actor:PalworldResourceESP_ClearTarget() end
function bridge_actor:PalworldResourceESP_RemoveTarget(actor, session_index)
    bridge_removed_targets[#bridge_removed_targets + 1] = {
        actor = actor,
        session_index = session_index,
    }
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
local payload_count_before_begin_play = #bridge_target_payloads
bridge_begin_play_hook(event_actor)
assert(#bridge_target_payloads == payload_count_before_begin_play + 1,
    "BeginPlay did not append exactly one event target")

local capture_hook = registered_hooks["/Script/Pal.PalUtility:PalCaptureSuccess"]
local dead_hook = registered_hooks["/Script/Pal.PalCharacter:OnDeadCharacter"]
local end_play_hook = registered_hooks["/Script/Engine.Actor:ReceiveEndPlay"]
assert(type(capture_hook) == "function" and type(dead_hook) == "function" and type(end_play_hook) == "function",
    "target lifecycle hooks were not registered")

local payload_count_before_delayed = #bridge_target_payloads
local scans_before_delayed = monster_find_all_count
bridge_begin_play_hook(delayed_event_actor)
assert(#bridge_target_payloads == payload_count_before_delayed,
    "not-ready BeginPlay target was admitted before its parameter existed")
assert(#delayed_callbacks == 1,
    "not-ready BeginPlay target did not schedule one path-based retry")
delayed_event_ready = true
local readiness_callbacks = delayed_callbacks
delayed_callbacks = {}
for _, callback in ipairs(readiness_callbacks) do
    callback()
end
assert(#bridge_target_payloads == payload_count_before_delayed + 1,
    "path-based readiness retry did not append the initialized target")
assert(static_actor_find_count == 1,
    "readiness retry did not resolve the target exactly once by object path")
assert(monster_find_all_count == scans_before_delayed,
    "readiness retry performed a global FindAllOf scan")
assert(#bridge_removed_targets == 1,
    "full event window did not evict exactly one older target")

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

local lifecycle_removals_before = #bridge_removed_targets
capture_hook(nil, nil, event_actor)
assert(#bridge_removed_targets == lifecycle_removals_before + 1, "capture did not remove the event target")
bridge_begin_play_hook(event_actor)
dead_hook(event_actor)
assert(#bridge_removed_targets == lifecycle_removals_before + 2, "death did not remove the event target")
bridge_begin_play_hook(event_actor)
end_play_hook(event_actor)
assert(#bridge_removed_targets == lifecycle_removals_before + 3, "EndPlay did not remove the event target")
bridge_begin_play_hook(event_actor)
monsters[#monsters + 1] = event_actor

local capture_start_found = false
local capture_mode_found = false
local capture_stop_found = false
local event_admission_found = false
for _, message in ipairs(runtime_logs) do
    capture_start_found = capture_start_found or message:match("PERF_SESSION_START") ~= nil
    capture_mode_found = capture_mode_found or message:match("PERF_MODE_CHANGED.*profile=off") ~= nil
    capture_stop_found = capture_stop_found or message:match("PERF_SESSION_STOP") ~= nil
    event_admission_found = event_admission_found or message:match("EVENT_TARGET_ADDED.*source=begin_play") ~= nil
end
assert(capture_start_found and capture_mode_found and capture_stop_found, "capture markers were incomplete")
assert(event_admission_found, "event-driven BeginPlay did not admit the new actor")
print("Panel controls and event-driven lifecycle passed")

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
