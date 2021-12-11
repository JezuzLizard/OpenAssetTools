#include "MenuConverterIW4.h"

#include <cassert>
#include <cstring>

#include "Game/IW4/MenuConstantsIW4.h"
#include "Game/T5/T5_Assets.h"
#include "Utils/ClassUtils.h"
#include "Menu/AbstractMenuConverter.h"
#include "Parsing/Menu/Domain/EventHandler/CommonEventHandlerCondition.h"
#include "Parsing/Menu/Domain/EventHandler/CommonEventHandlerScript.h"
#include "Parsing/Menu/Domain/EventHandler/CommonEventHandlerSetLocalVar.h"
#include "Parsing/Menu/Domain/Expression/CommonExpressionFunctionCall.h"
#include "Parsing/Simple/Expression/SimpleExpressionBinaryOperation.h"
#include "Parsing/Simple/Expression/SimpleExpressionConditionalOperator.h"
#include "Parsing/Simple/Expression/SimpleExpressionUnaryOperation.h"

using namespace IW4;
using namespace menu;

namespace IW4
{
    class MenuConverterImpl : public AbstractMenuConverter
    {
        static void ApplyMenuDefaults(menuDef_t* menu)
        {
            memset(menu, 0, sizeof(menuDef_t));
            menu->window.foreColor[0] = 1.0f;
            menu->window.foreColor[1] = 1.0f;
            menu->window.foreColor[2] = 1.0f;
            menu->window.foreColor[3] = 1.0f;
        }

        static void ApplyItemDefaults(itemDef_s* item)
        {
            memset(item, 0, sizeof(itemDef_s));
            item->window.foreColor[0] = 1.0f;
            item->window.foreColor[1] = 1.0f;
            item->window.foreColor[2] = 1.0f;
            item->window.foreColor[3] = 1.0f;
        }

        _NODISCARD static rectDef_s ConvertRectDef(const CommonRect& rect)
        {
            return rectDef_s{
                static_cast<float>(rect.x),
                static_cast<float>(rect.y),
                static_cast<float>(rect.w),
                static_cast<float>(rect.h),
                static_cast<char>(rect.horizontalAlign),
                static_cast<char>(rect.verticalAlign)
            };
        }

        static void ConvertColor(float (&output)[4], const CommonColor& input)
        {
            output[0] = static_cast<float>(input.r);
            output[1] = static_cast<float>(input.g);
            output[2] = static_cast<float>(input.b);
            output[3] = static_cast<float>(input.a);
        }

        static void ApplyFlag(int& flags, const bool shouldApply, const int flagValue)
        {
            if (!shouldApply)
                return;

            flags |= flagValue;
        }

        static int ConvertItemType(const int input)
        {
            return input;
        }

        static int ConvertTextFont(const int input)
        {
            return input;
        }

        _NODISCARD Material* ConvertMaterial(const std::string& materialName, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (materialName.empty())
                return nullptr;

            auto* materialDependency = m_manager->LoadDependency(ASSET_TYPE_MATERIAL, materialName);
            if (!materialDependency)
                throw MenuConversionException("Failed to load material \"" + materialName + "\"", menu, item);

            return static_cast<Material*>(materialDependency->m_ptr);
        }

        static const std::map<std::string, int>& GetFunctionMap()
        {
            static std::map<std::string, int> mappings;
            static bool initialized = false;

            if(!initialized)
            {
                for(size_t i = EXP_FUNC_DYN_START; i < std::extent_v<decltype(g_expFunctionNames)>; i++)
                {
                    mappings[g_expFunctionNames[i]] = static_cast<int>(i);
                }
                initialized = true;
            }

            return mappings;
        }

        void ConvertExpressionEntryFunctionCall(std::vector<expressionEntry>& entries, const CommonExpressionFunctionCall* functionCall, const CommonMenuDef* menu,
            const CommonItemDef* item) const
        {
            const auto& functionMappings = GetFunctionMap();

            const auto foundMapping = functionMappings.find(functionCall->m_function_name);
            if (foundMapping == functionMappings.end())
                throw MenuConversionException("Could not find function \"" + functionCall->m_function_name + "\"", menu, item);

            expressionEntry functionEntry{};
            functionEntry.type = EET_OPERATOR;
            functionEntry.data.op = foundMapping->second;
            entries.emplace_back(functionEntry);

            auto firstArg = true;
            for(const auto& arg : functionCall->m_args)
            {
                if (!firstArg)
                {
                    expressionEntry argSeparator{};
                    argSeparator.type = EET_OPERATOR;
                    argSeparator.data.op = OP_COMMA;
                    entries.emplace_back(argSeparator);
                }
                else
                    firstArg = false;

                ConvertExpressionEntry(entries, arg.get(), menu, item);
            }

            expressionEntry parenRight{};
            parenRight.type = EET_OPERATOR;
            parenRight.data.op = OP_RIGHTPAREN;
            entries.emplace_back(parenRight);
        }

        constexpr static expressionOperatorType_e UNARY_OPERATION_MAPPING[static_cast<unsigned>(SimpleUnaryOperationId::COUNT)]
        {
            OP_NOT,
            OP_BITWISENOT,
            OP_SUBTRACT
        };

        void ConvertExpressionEntryUnaryOperation(std::vector<expressionEntry>& entries, const SimpleExpressionUnaryOperation* unaryOperation, const CommonMenuDef* menu,
                                                  const CommonItemDef* item) const
        {
            assert(static_cast<unsigned>(unaryOperation->m_operation_type->m_id) < static_cast<unsigned>(SimpleUnaryOperationId::COUNT));
            expressionEntry operation{};
            operation.type = EET_OPERATOR;
            operation.data.op = UNARY_OPERATION_MAPPING[static_cast<unsigned>(unaryOperation->m_operation_type->m_id)];
            entries.emplace_back(operation);

            if (unaryOperation->OperandNeedsParenthesis())
            {
                expressionEntry parenLeft{};
                parenLeft.type = EET_OPERATOR;
                parenLeft.data.op = OP_LEFTPAREN;
                entries.emplace_back(parenLeft);

                ConvertExpressionEntry(entries, unaryOperation->m_operand.get(), menu, item);

                expressionEntry parenRight{};
                parenRight.type = EET_OPERATOR;
                parenRight.data.op = OP_RIGHTPAREN;
                entries.emplace_back(parenRight);
            }
            else
                ConvertExpressionEntry(entries, unaryOperation->m_operand.get(), menu, item);
        }

        constexpr static expressionOperatorType_e BINARY_OPERATION_MAPPING[static_cast<unsigned>(SimpleBinaryOperationId::COUNT)]
        {
            OP_ADD,
            OP_SUBTRACT,
            OP_MULTIPLY,
            OP_DIVIDE,
            OP_MODULUS,
            OP_BITWISEAND,
            OP_BITWISEOR,
            OP_BITSHIFTLEFT,
            OP_BITSHIFTRIGHT,
            OP_GREATERTHAN,
            OP_GREATERTHANEQUALTO,
            OP_LESSTHAN,
            OP_LESSTHANEQUALTO,
            OP_EQUALS,
            OP_NOTEQUAL,
            OP_AND,
            OP_OR
        };

        void ConvertExpressionEntryBinaryOperation(std::vector<expressionEntry>& entries, const SimpleExpressionBinaryOperation* binaryOperation, const CommonMenuDef* menu,
                                                   const CommonItemDef* item) const
        {
            if (binaryOperation->Operand1NeedsParenthesis())
            {
                expressionEntry parenLeft{};
                parenLeft.type = EET_OPERATOR;
                parenLeft.data.op = OP_LEFTPAREN;
                entries.emplace_back(parenLeft);

                ConvertExpressionEntry(entries, binaryOperation->m_operand1.get(), menu, item);

                expressionEntry parenRight{};
                parenRight.type = EET_OPERATOR;
                parenRight.data.op = OP_RIGHTPAREN;
                entries.emplace_back(parenRight);
            }
            else
                ConvertExpressionEntry(entries, binaryOperation->m_operand1.get(), menu, item);

            assert(static_cast<unsigned>(binaryOperation->m_operation_type->m_id) < static_cast<unsigned>(SimpleBinaryOperationId::COUNT));
            expressionEntry operation{};
            operation.type = EET_OPERATOR;
            operation.data.op = BINARY_OPERATION_MAPPING[static_cast<unsigned>(binaryOperation->m_operation_type->m_id)];
            entries.emplace_back(operation);

            if (binaryOperation->Operand2NeedsParenthesis())
            {
                expressionEntry parenLeft{};
                parenLeft.type = EET_OPERATOR;
                parenLeft.data.op = OP_LEFTPAREN;
                entries.emplace_back(parenLeft);

                ConvertExpressionEntry(entries, binaryOperation->m_operand2.get(), menu, item);

                expressionEntry parenRight{};
                parenRight.type = EET_OPERATOR;
                parenRight.data.op = OP_RIGHTPAREN;
                entries.emplace_back(parenRight);
            }
            else
                ConvertExpressionEntry(entries, binaryOperation->m_operand2.get(), menu, item);
        }

        void ConvertExpressionEntryExpressionValue(std::vector<expressionEntry>& entries, const SimpleExpressionValue* expressionValue) const
        {
            expressionEntry entry{};
            entry.type = EET_OPERAND;

            if (expressionValue->m_type == SimpleExpressionValue::Type::INT)
            {
                entry.data.operand.dataType = VAL_INT;
                entry.data.operand.internals.intVal = expressionValue->m_int_value;
            }
            else if (expressionValue->m_type == SimpleExpressionValue::Type::DOUBLE)
            {
                entry.data.operand.dataType = VAL_FLOAT;
                entry.data.operand.internals.floatVal = static_cast<float>(expressionValue->m_double_value);
            }
            else if (expressionValue->m_type == SimpleExpressionValue::Type::STRING)
            {
                entry.data.operand.dataType = VAL_STRING;
                entry.data.operand.internals.stringVal.string = m_memory->Dup(expressionValue->m_string_value->c_str());
            }

            entries.emplace_back(entry);
        }

        void ConvertExpressionEntry(std::vector<expressionEntry>& entries, const ISimpleExpression* expression, const CommonMenuDef* menu, const CommonItemDef* item) const
        {
            if (!m_legacy_mode && expression->IsStatic())
            {
                const auto expressionStaticValue = expression->Evaluate();
                ConvertExpressionEntryExpressionValue(entries, &expressionStaticValue);
            }
            else if (const auto* expressionValue = dynamic_cast<const SimpleExpressionValue*>(expression))
            {
                ConvertExpressionEntryExpressionValue(entries, expressionValue);
            }
            else if (const auto* binaryOperation = dynamic_cast<const SimpleExpressionBinaryOperation*>(expression))
            {
                ConvertExpressionEntryBinaryOperation(entries, binaryOperation, menu, item);
            }
            else if (const auto* unaryOperation = dynamic_cast<const SimpleExpressionUnaryOperation*>(expression))
            {
                ConvertExpressionEntryUnaryOperation(entries, unaryOperation, menu, item);
            }
            else if (const auto* functionCall = dynamic_cast<const CommonExpressionFunctionCall*>(expression))
            {
                ConvertExpressionEntryFunctionCall(entries, functionCall, menu, item);
            }
            else if (dynamic_cast<const SimpleExpressionConditionalOperator*>(expression))
            {
                throw MenuConversionException("Cannot use conditional expression in menu expressions", menu, item);
            }
            else
            {
                assert(false);
                throw MenuConversionException("Unknown expression entry type in menu expressions", menu, item);
            }
        }

        _NODISCARD Statement_s* ConvertExpression(const ISimpleExpression* expression, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (!expression)
                return nullptr;

            auto* statement = m_memory->Create<Statement_s>();
            statement->lastResult = Operand{};
            statement->lastExecuteTime = 0;

            std::vector<expressionEntry> expressionEntries;

            ConvertExpressionEntry(expressionEntries, expression, menu, item);

            auto* outputExpressionEntries = static_cast<expressionEntry*>(m_memory->Alloc(sizeof(expressionEntry) * expressionEntries.size()));
            memcpy(outputExpressionEntries, expressionEntries.data(), sizeof(expressionEntry) * expressionEntries.size());

            statement->entries = outputExpressionEntries;
            statement->numEntries = static_cast<int>(expressionEntries.size());

            // TODO: Add supporting data
            statement->supportingData = nullptr;

            return statement;
        }

        _NODISCARD Statement_s* ConvertOrApplyStatement(float& staticValue, const ISimpleExpression* expression, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (m_legacy_mode)
                return ConvertExpression(expression, menu, item);

            if (!expression)
                return nullptr;

            if (expression->IsStatic())
            {
                const auto value = expression->Evaluate();
                switch (value.m_type)
                {
                case SimpleExpressionValue::Type::DOUBLE:
                    staticValue = static_cast<float>(value.m_double_value);
                    break;
                case SimpleExpressionValue::Type::INT:
                    staticValue = static_cast<float>(value.m_int_value);
                    break;
                case SimpleExpressionValue::Type::STRING:
                    throw MenuConversionException("Cannot convert string expression value to floating point", menu, item);
                }
                return nullptr;
            }

            return ConvertExpression(expression, menu, item);
        }

        _NODISCARD Statement_s* ConvertOrApplyStatement(const char*& staticValue, const ISimpleExpression* expression, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (m_legacy_mode)
                return ConvertExpression(expression, menu, item);

            if (!expression)
                return nullptr;

            if (expression->IsStatic())
            {
                const auto value = expression->Evaluate();
                switch (value.m_type)
                {
                case SimpleExpressionValue::Type::STRING:
                    staticValue = m_memory->Dup(value.m_string_value->c_str());
                    break;

                case SimpleExpressionValue::Type::DOUBLE:
                case SimpleExpressionValue::Type::INT:
                    throw MenuConversionException("Cannot convert numeric expression value to string", menu, item);
                }
                return nullptr;
            }

            return ConvertExpression(expression, menu, item);
        }

        _NODISCARD Statement_s* ConvertOrApplyStatement(Material*& staticValue, const ISimpleExpression* expression, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (m_legacy_mode)
                return ConvertExpression(expression, menu, item);

            if (!expression)
                return nullptr;

            if (expression->IsStatic())
            {
                const auto value = expression->Evaluate();
                switch (value.m_type)
                {
                case SimpleExpressionValue::Type::STRING:
                    staticValue = ConvertMaterial(*value.m_string_value, menu, item);
                    break;

                case SimpleExpressionValue::Type::DOUBLE:
                case SimpleExpressionValue::Type::INT:
                    throw MenuConversionException("Cannot convert numeric expression value to string", menu, item);
                }
                return nullptr;
            }

            return ConvertExpression(expression, menu, item);
        }

        _NODISCARD static EventType SetLocalVarTypeToEventType(const SetLocalVarType setLocalVarType)
        {
            switch (setLocalVarType)
            {
            case SetLocalVarType::BOOL:
                return EVENT_SET_LOCAL_VAR_BOOL;
            case SetLocalVarType::STRING:
                return EVENT_SET_LOCAL_VAR_STRING;
            case SetLocalVarType::FLOAT:
                return EVENT_SET_LOCAL_VAR_FLOAT;
            case SetLocalVarType::INT:
                return EVENT_SET_LOCAL_VAR_INT;
            default:
            case SetLocalVarType::UNKNOWN:
                assert(false);
                return EVENT_SET_LOCAL_VAR_INT;
            }
        }

        MenuEventHandler* ConvertEventHandlerSetLocalVar(const CommonEventHandlerSetLocalVar* setLocalVar, const CommonMenuDef* menu, const CommonItemDef* item) const
        {
            assert(setLocalVar);
            if (!setLocalVar)
                return nullptr;

            auto* outputHandler = static_cast<MenuEventHandler*>(m_memory->Alloc(sizeof(MenuEventHandler) + sizeof(SetLocalVarData)));
            auto* outputSetLocalVar = reinterpret_cast<SetLocalVarData*>(reinterpret_cast<int8_t*>(outputHandler) + sizeof(MenuEventHandler));

            outputHandler->eventType = SetLocalVarTypeToEventType(setLocalVar->m_type);
            outputHandler->eventData.setLocalVarData = outputSetLocalVar;

            outputSetLocalVar->localVarName = m_memory->Dup(setLocalVar->m_var_name.c_str());
            outputSetLocalVar->expression = ConvertExpression(setLocalVar->m_value.get(), menu, item);

            return outputHandler;
        }

        MenuEventHandler* ConvertEventHandlerScript(const CommonEventHandlerScript* script) const
        {
            assert(script);
            if (!script)
                return nullptr;

            auto* outputHandler = m_memory->Create<MenuEventHandler>();
            outputHandler->eventType = EVENT_UNCONDITIONAL;
            outputHandler->eventData.unconditionalScript = m_memory->Dup(script->m_script.c_str());

            return outputHandler;
        }

        void ConvertEventHandlerCondition(MenuEventHandler** eventHandlerArray, size_t& eventHandlerArrayIndex, const CommonEventHandlerCondition* condition, const CommonMenuDef* menu,
                                          const CommonItemDef* item) const
        {
            assert(condition);
            if (!condition)
                return;

            auto* outputHandler = static_cast<MenuEventHandler*>(m_memory->Alloc(sizeof(MenuEventHandler) + sizeof(ConditionalScript)));
            auto* outputCondition = reinterpret_cast<ConditionalScript*>(reinterpret_cast<int8_t*>(outputHandler) + sizeof(MenuEventHandler));

            outputHandler->eventType = EVENT_IF;
            outputHandler->eventData.conditionalScript = outputCondition;

            outputCondition->eventExpression = ConvertExpression(condition->m_condition.get(), menu, item);
            outputCondition->eventHandlerSet = ConvertEventHandlerSet(condition->m_condition_elements.get(), menu, item);

            eventHandlerArray[eventHandlerArrayIndex] = outputHandler;

            if (condition->m_else_elements)
            {
                eventHandlerArrayIndex++;

                auto* outputElseHandler = m_memory->Create<MenuEventHandler>();
                outputElseHandler->eventType = EVENT_ELSE;
                outputElseHandler->eventData.elseScript = ConvertEventHandlerSet(condition->m_else_elements.get(), menu, item);

                eventHandlerArray[eventHandlerArrayIndex] = outputElseHandler;
            }
        }

        void ConvertEventHandler(MenuEventHandler** eventHandlerArray, size_t& eventHandlerArrayIndex, const ICommonEventHandlerElement* eventHandler, const CommonMenuDef* menu,
                                 const CommonItemDef* item) const
        {
            assert(eventHandler);
            if (!eventHandler)
                return;

            switch (eventHandler->GetType())
            {
            case CommonEventHandlerElementType::CONDITION:
                ConvertEventHandlerCondition(eventHandlerArray, eventHandlerArrayIndex, dynamic_cast<const CommonEventHandlerCondition*>(eventHandler), menu, item);
                break;

            case CommonEventHandlerElementType::SCRIPT:
                eventHandlerArray[eventHandlerArrayIndex] = ConvertEventHandlerScript(dynamic_cast<const CommonEventHandlerScript*>(eventHandler));
                break;

            case CommonEventHandlerElementType::SET_LOCAL_VAR:
                eventHandlerArray[eventHandlerArrayIndex] = ConvertEventHandlerSetLocalVar(dynamic_cast<const CommonEventHandlerSetLocalVar*>(eventHandler), menu, item);
                break;
            }
        }

        _NODISCARD static size_t EventHandlerSetElementCount(const CommonEventHandlerSet* eventHandlerSet)
        {
            auto elementCount = 0u;
            for (const auto& element : eventHandlerSet->m_elements)
            {
                auto* condition = dynamic_cast<const CommonEventHandlerCondition*>(element.get());
                if (condition && condition->m_else_elements)
                    elementCount += 2;
                else
                    elementCount++;
            }
            return elementCount;
        }

        _NODISCARD MenuEventHandlerSet* ConvertEventHandlerSet(const CommonEventHandlerSet* eventHandlerSet, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (!eventHandlerSet)
                return nullptr;

            const auto elementCount = EventHandlerSetElementCount(eventHandlerSet);
            auto* outputSet = static_cast<MenuEventHandlerSet*>(m_memory->Alloc(sizeof(MenuEventHandlerSet) + sizeof(void*) * elementCount));
            auto* outputElements = reinterpret_cast<MenuEventHandler**>(reinterpret_cast<int8_t*>(outputSet) + sizeof(MenuEventHandlerSet));

            outputSet->eventHandlerCount = static_cast<int>(elementCount);
            outputSet->eventHandlers = outputElements;

            auto eventHandlerIndex = 0u;
            for (const auto& element : eventHandlerSet->m_elements)
            {
                ConvertEventHandler(outputElements, eventHandlerIndex, element.get(), menu, item);
                eventHandlerIndex++;
            }

            return outputSet;
        }

        _NODISCARD ItemKeyHandler* ConvertKeyHandler(const std::map<int, std::unique_ptr<CommonEventHandlerSet>>& keyHandlers, const CommonMenuDef* menu, const CommonItemDef* item = nullptr) const
        {
            if (keyHandlers.empty())
                return nullptr;

            const auto keyHandlerCount = keyHandlers.size();
            auto* output = static_cast<ItemKeyHandler*>(m_memory->Alloc(sizeof(ItemKeyHandler) * keyHandlerCount));
            auto currentKeyHandler = keyHandlers.cbegin();
            for (auto i = 0u; i < keyHandlerCount; i++)
            {
                output[i].key = currentKeyHandler->first;
                output[i].action = ConvertEventHandlerSet(currentKeyHandler->second.get(), menu, item);

                if (i + 1 < keyHandlerCount)
                    output[i].next = &output[i + 1];
                else
                    output[i].next = nullptr;
                ++currentKeyHandler;
            }

            return output;
        }

        _NODISCARD itemDef_s* ConvertItem(const CommonMenuDef& parentMenu, const CommonItemDef& commonItem) const
        {
            auto* item = m_memory->Create<itemDef_s>();
            ApplyItemDefaults(item);

            item->window.name = ConvertString(commonItem.m_name);
            item->text = ConvertString(commonItem.m_text);
            ApplyFlag(item->itemFlags, commonItem.m_text_save_game, ITEM_FLAG_SAVE_GAME_INFO);
            ApplyFlag(item->itemFlags, commonItem.m_text_cinematic_subtitle, ITEM_FLAG_CINEMATIC_SUBTITLE);
            item->window.group = ConvertString(commonItem.m_group);
            item->window.rect = ConvertRectDef(commonItem.m_rect);
            item->window.style = commonItem.m_style;
            ApplyFlag(item->window.staticFlags, commonItem.m_decoration, WINDOW_FLAG_DECORATION);
            ApplyFlag(item->window.staticFlags, commonItem.m_auto_wrapped, WINDOW_FLAG_AUTO_WRAPPED);
            ApplyFlag(item->window.staticFlags, commonItem.m_horizontal_scroll, WINDOW_FLAG_HORIZONTAL_SCROLL);
            item->type = ConvertItemType(commonItem.m_type);
            item->window.border = commonItem.m_border;
            item->window.borderSize = static_cast<float>(commonItem.m_border_size);
            item->visibleExp = ConvertExpression(commonItem.m_visible_expression.get(), &parentMenu, &commonItem);
            item->disabledExp = ConvertExpression(commonItem.m_disabled_expression.get(), &parentMenu, &commonItem);
            item->window.ownerDraw = commonItem.m_owner_draw;
            item->window.ownerDrawFlags = commonItem.m_owner_draw_flags;
            item->alignment = commonItem.m_align;
            item->textAlignMode = commonItem.m_text_align;
            item->textalignx = static_cast<float>(commonItem.m_text_align_x);
            item->textaligny = static_cast<float>(commonItem.m_text_align_y);
            item->textscale = static_cast<float>(commonItem.m_text_scale);
            item->textStyle = commonItem.m_text_style;
            item->fontEnum = ConvertTextFont(commonItem.m_text_font);
            ConvertColor(item->window.backColor, commonItem.m_back_color);
            ConvertColor(item->window.foreColor, commonItem.m_fore_color);
            ConvertColor(item->window.borderColor, commonItem.m_border_color);
            ConvertColor(item->window.outlineColor, commonItem.m_outline_color);
            ConvertColor(item->window.disableColor, commonItem.m_disable_color);
            ConvertColor(item->glowColor, commonItem.m_glow_color);
            item->window.background = ConvertMaterial(commonItem.m_background, &parentMenu, &commonItem);
            item->onFocus = ConvertEventHandlerSet(commonItem.m_on_focus.get(), &parentMenu, &commonItem);
            item->leaveFocus = ConvertEventHandlerSet(commonItem.m_on_leave_focus.get(), &parentMenu, &commonItem);
            item->mouseEnter = ConvertEventHandlerSet(commonItem.m_on_mouse_enter.get(), &parentMenu, &commonItem);
            item->mouseExit = ConvertEventHandlerSet(commonItem.m_on_mouse_exit.get(), &parentMenu, &commonItem);
            item->mouseEnterText = ConvertEventHandlerSet(commonItem.m_on_mouse_enter_text.get(), &parentMenu, &commonItem);
            item->mouseExitText = ConvertEventHandlerSet(commonItem.m_on_mouse_exit_text.get(), &parentMenu, &commonItem);
            item->action = ConvertEventHandlerSet(commonItem.m_on_action.get(), &parentMenu, &commonItem);
            item->accept = ConvertEventHandlerSet(commonItem.m_on_accept.get(), &parentMenu, &commonItem);
            // item->focusSound
            item->dvarTest = ConvertString(commonItem.m_dvar_test);
            // enableDvar
            item->onKey = ConvertKeyHandler(commonItem.m_key_handlers, &parentMenu, &commonItem);
            item->textExp = ConvertOrApplyStatement(item->text, commonItem.m_text_expression.get(), &parentMenu, &commonItem);
            item->materialExp = ConvertOrApplyStatement(item->window.background, commonItem.m_material_expression.get(), &parentMenu, &commonItem);
            item->disabledExp = ConvertExpression(commonItem.m_disabled_expression.get(), &parentMenu, &commonItem);
            // FloatExpressions
            item->gameMsgWindowIndex = commonItem.m_game_message_window_index;
            item->gameMsgWindowMode = commonItem.m_game_message_window_mode;
            item->fxLetterTime = commonItem.m_fx_letter_time;
            item->fxDecayStartTime = commonItem.m_fx_decay_start_time;
            item->fxDecayDuration = commonItem.m_fx_decay_duration;

            return item;
        }

        _NODISCARD itemDef_s** ConvertMenuItems(const CommonMenuDef& commonMenu) const
        {
            if (commonMenu.m_items.empty())
                return nullptr;

            auto* items = static_cast<itemDef_s**>(m_memory->Alloc(sizeof(void*) * commonMenu.m_items.size()));
            memset(items, 0, sizeof(void*) * commonMenu.m_items.size());

            for (auto i = 0u; i < commonMenu.m_items.size(); i++)
                items[i] = ConvertItem(commonMenu, *commonMenu.m_items[i]);

            return items;
        }

    public:
        MenuConverterImpl(const bool legacyMode, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager)
            : AbstractMenuConverter(legacyMode, searchPath, memory, manager)
        {
        }

        _NODISCARD menuDef_t* ConvertMenu(const CommonMenuDef& commonMenu) const
        {
            auto* menu = m_memory->Create<menuDef_t>();
            ApplyMenuDefaults(menu);

            menu->window.name = m_memory->Dup(commonMenu.m_name.c_str());
            menu->fullScreen = commonMenu.m_full_screen;
            ApplyFlag(menu->window.staticFlags, commonMenu.m_screen_space, WINDOW_FLAG_SCREEN_SPACE);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_decoration, WINDOW_FLAG_DECORATION);
            menu->window.rect = ConvertRectDef(commonMenu.m_rect);
            menu->window.style = commonMenu.m_style;
            menu->window.border = commonMenu.m_border;
            menu->window.borderSize = static_cast<float>(commonMenu.m_border_size);
            ConvertColor(menu->window.backColor, commonMenu.m_back_color);
            ConvertColor(menu->window.foreColor, commonMenu.m_fore_color);
            ConvertColor(menu->window.borderColor, commonMenu.m_border_color);
            ConvertColor(menu->focusColor, commonMenu.m_focus_color);
            menu->window.background = ConvertMaterial(commonMenu.m_background, &commonMenu);
            menu->window.ownerDraw = commonMenu.m_owner_draw;
            menu->window.ownerDrawFlags = commonMenu.m_owner_draw_flags;
            ApplyFlag(menu->window.staticFlags, commonMenu.m_out_of_bounds_click, WINDOW_FLAG_OUT_OF_BOUNDS_CLICK);
            menu->soundName = ConvertString(commonMenu.m_sound_loop);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_popup, WINDOW_FLAG_POPUP);
            menu->fadeClamp = static_cast<float>(commonMenu.m_fade_clamp);
            menu->fadeCycle = commonMenu.m_fade_cycle;
            menu->fadeAmount = static_cast<float>(commonMenu.m_fade_amount);
            menu->fadeInAmount = static_cast<float>(commonMenu.m_fade_in_amount);
            menu->blurRadius = static_cast<float>(commonMenu.m_blur_radius);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_legacy_split_screen_scale, WINDOW_FLAG_LEGACY_SPLIT_SCREEN_SCALE);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_hidden_during_scope, WINDOW_FLAG_HIDDEN_DURING_SCOPE);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_hidden_during_flashbang, WINDOW_FLAG_HIDDEN_DURING_FLASH_BANG);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_hidden_during_ui, WINDOW_FLAG_HIDDEN_DURING_UI);
            menu->allowedBinding = ConvertString(commonMenu.m_allowed_binding);
            ApplyFlag(menu->window.staticFlags, commonMenu.m_text_only_focus, WINDOW_FLAG_TEXT_ONLY_FOCUS);
            menu->visibleExp = ConvertExpression(commonMenu.m_visible_expression.get(), &commonMenu);
            menu->rectXExp = ConvertOrApplyStatement(menu->window.rect.x, commonMenu.m_rect_x_exp.get(), &commonMenu);
            menu->rectYExp = ConvertOrApplyStatement(menu->window.rect.y, commonMenu.m_rect_y_exp.get(), &commonMenu);
            menu->rectWExp = ConvertOrApplyStatement(menu->window.rect.w, commonMenu.m_rect_w_exp.get(), &commonMenu);
            menu->rectHExp = ConvertOrApplyStatement(menu->window.rect.h, commonMenu.m_rect_h_exp.get(), &commonMenu);
            menu->openSoundExp = ConvertExpression(commonMenu.m_open_sound_exp.get(), &commonMenu);
            menu->closeSoundExp = ConvertExpression(commonMenu.m_close_sound_exp.get(), &commonMenu);
            menu->onOpen = ConvertEventHandlerSet(commonMenu.m_on_open.get(), &commonMenu);
            menu->onClose = ConvertEventHandlerSet(commonMenu.m_on_close.get(), &commonMenu);
            menu->onCloseRequest = ConvertEventHandlerSet(commonMenu.m_on_request_close.get(), &commonMenu);
            menu->onESC = ConvertEventHandlerSet(commonMenu.m_on_esc.get(), &commonMenu);
            menu->onKey = ConvertKeyHandler(commonMenu.m_key_handlers, &commonMenu);
            menu->items = ConvertMenuItems(commonMenu);

            return menu;
        }

        std::vector<XAssetInfoGeneric*> m_dependencies;
    };
}

MenuConverter::MenuConverter(const bool legacyMode, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager)
    : m_legacy_mode(legacyMode),
      m_search_path(searchPath),
      m_memory(memory),
      m_manager(manager)
{
}

std::vector<XAssetInfoGeneric*>& MenuConverter::GetDependencies()
{
    return m_dependencies;
}

menuDef_t* MenuConverter::ConvertMenu(const CommonMenuDef& commonMenu)
{
    MenuConverterImpl impl(m_legacy_mode, m_search_path, m_memory, m_manager);

    try
    {
        auto* result = impl.ConvertMenu(commonMenu);
        m_dependencies = std::move(impl.m_dependencies);
        return result;
    }
    catch (const MenuConversionException& e)
    {
        MenuConverterImpl::PrintConversionExceptionDetails(e);
    }

    return nullptr;
}