#include "ZoneWriteTemplate.h"


#include <cassert>
#include <iostream>
#include <sstream>


#include "Domain/Computations/StructureComputations.h"
#include "Internal/BaseTemplate.h"

class ZoneWriteTemplate::Internal final : BaseTemplate
{
    enum class MemberWriteType
    {
        ARRAY_POINTER,
        DYNAMIC_ARRAY,
        EMBEDDED,
        EMBEDDED_ARRAY,
        POINTER_ARRAY,
        SINGLE_POINTER
    };

    static std::string WriterClassName(StructureInformation* asset)
    {
        std::ostringstream str;
        str << "Writer_" << asset->m_definition->m_name;
        return str.str();
    }

    static std::string VariableDecl(const DataDefinition* def)
    {
        std::ostringstream str;
        str << def->GetFullName() << "* var" << MakeSafeTypeName(def) << ";";
        return str.str();
    }

    static std::string PointerVariableDecl(const DataDefinition* def)
    {
        std::ostringstream str;
        str << def->GetFullName() << "** var" << MakeSafeTypeName(def) << "Ptr;";
        return str.str();
    }

    void PrintHeaderPtrArrayWriteMethodDeclaration(const DataDefinition* def) const
    {
        LINE("void WritePtrArray_" << MakeSafeTypeName(def) << "(bool atStreamStart, size_t count);")
    }

    void PrintHeaderArrayWriteMethodDeclaration(const DataDefinition* def) const
    {
        LINE("void WriteArray_" << MakeSafeTypeName(def) << "(bool atStreamStart, size_t count);")
    }

    void PrintHeaderWriteMethodDeclaration(const StructureInformation* info) const
    {
        LINE("void Write_" << MakeSafeTypeName(info->m_definition) << "(bool atStreamStart);")
    }

    void PrintHeaderTempPtrWriteMethodDeclaration(const StructureInformation* info) const
    {
        LINE("void WritePtr_" << MakeSafeTypeName(info->m_definition) << "(bool atStreamStart);")
    }

    void PrintHeaderGetNameMethodDeclaration(const StructureInformation* info) const
    {
        LINE("static std::string GetAssetName(" << info->m_definition->GetFullName() << "* pAsset);")
    }

    void PrintHeaderMainWriteMethodDeclaration(const StructureInformation* info) const
    {
        LINE("void Write(" << info->m_definition->GetFullName() << "** pAsset);")
    }

    void PrintHeaderConstructor() const
    {
        LINE(WriterClassName(m_env.m_asset) << "("<<m_env.m_asset->m_definition->GetFullName()<<"* asset, Zone* zone, IZoneOutputStream* stream);")
    }

    void PrintVariableInitialization(const DataDefinition* def) const
    {
        LINE("var" << def->m_name << " = nullptr;")
    }

    void PrintPointerVariableInitialization(const DataDefinition* def) const
    {
        LINE("var" << def->m_name << "Ptr = nullptr;")
    }

    void PrintConstructorMethod()
    {
        LINE(WriterClassName(m_env.m_asset) << "::" << WriterClassName(m_env.m_asset) << "("<<m_env.m_asset->m_definition->GetFullName()<<"* asset, Zone* zone, IZoneOutputStream* stream)")

        m_intendation++;
        LINE_START(": AssetWriter(zone->m_pools->GetAsset("<<m_env.m_asset->m_asset_enum_entry->m_name<<", GetAssetName(asset))"<<", zone, stream)")
        LINE_END("")
        m_intendation--;

        LINE("{")
        m_intendation++;

        PrintVariableInitialization(m_env.m_asset->m_definition);
        PrintPointerVariableInitialization(m_env.m_asset->m_definition);
        LINE("")

        for (auto* type : m_env.m_used_types)
        {
            if (type->m_info && !type->m_info->m_definition->m_anonymous && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                PrintVariableInitialization(type->m_type);
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_info && type->m_pointer_array_reference_exists && !type->m_is_context_asset)
            {
                PrintPointerVariableInitialization(type->m_type);
            }
        }

        m_intendation--;
        LINE("}")
    }

    void WriteMember_ScriptString(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType) const
    {
        if (writeType == MemberWriteType::ARRAY_POINTER)
        {
            LINE("varScriptString = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("WriteScriptStringArray(true, " << MakeEvaluation(modifier.GetArrayPointerCountEvaluation()) << ");")
            LINE("m_stream->MarkFollowing("<<MakeMemberAccess("writtenData", info, member, modifier)<<");")
        }
        else if (writeType == MemberWriteType::EMBEDDED_ARRAY)
        {
            LINE("varScriptString = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("WriteScriptStringArray(false, " << MakeArrayCount(dynamic_cast<ArrayDeclarationModifier*>(modifier.GetDeclarationModifier())) << ");")
        }
        else if (writeType == MemberWriteType::EMBEDDED)
        {
            LINE(MakeMemberAccess(info, member, modifier) << " = UseScriptString(" << MakeMemberAccess(info, member, modifier) << ");")
        }
        else
        {
            assert(false);
            LINE("#error unsupported writeType " << static_cast<int>(writeType) << " for scripstring")
        }
    }

    void WriteMember_Asset(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType) const
    {
        if (writeType == MemberWriteType::SINGLE_POINTER)
        {
            LINE(WriterClassName(member->m_type) << " writer("<<MakeMemberAccess(info, member, modifier)<<", m_zone, m_stream);")
            LINE("writer.Write(&" << MakeMemberAccess(info, member, modifier) << ");")
        }
        else if (writeType == MemberWriteType::POINTER_ARRAY)
        {
            WriteMember_PointerArray(info, member, modifier);
        }
        else
        {
            assert(false);
            LINE("#error unsupported writeType " << static_cast<int>(writeType) << " for asset")
        }
    }

    void WriteMember_String(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType) const
    {
        if (writeType == MemberWriteType::SINGLE_POINTER)
        {
            if (member->m_member->m_type_declaration->m_is_const)
            {
                LINE("varXString = &" << MakeMemberAccess(info, member, modifier) << ";")
            }
            else
            {
                LINE("varXString = const_cast<const char**>(&" << MakeMemberAccess(info, member, modifier) << ");")
            }
            LINE("WriteXString(false);")
        }
        else if (writeType == MemberWriteType::POINTER_ARRAY)
        {
            LINE("varXString = " << MakeMemberAccess(info, member, modifier) << ";")
            if (modifier.IsArray())
            {
                LINE("WriteXStringArray(false, " << modifier.GetArraySize() << ");")
            }
            else
            {
                LINE("WriteXStringArray(true, " << MakeEvaluation(modifier.GetPointerArrayCountEvaluation()) << ");")
                LINE("m_stream->MarkFollowing(" << MakeMemberAccess("writtenData", info, member, modifier) << ");")
            }
        }
        else
        {
            assert(false);
            LINE("#error unsupported writeType " << static_cast<int>(writeType) << " for string")
        }
    }

    void WriteMember_ArrayPointer(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        if (member->m_type && !member->m_type->m_is_leaf && !computations.IsInRuntimeBlock())
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("WriteArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, " << MakeEvaluation(modifier.GetArrayPointerCountEvaluation()) << ");")
        }
        else
        {
            LINE("m_stream->Write<" << MakeTypeDecl(member->m_member->m_type_declaration.get()) << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers())
                << ">(" << MakeMemberAccess(info, member, modifier) << ", " << MakeEvaluation(modifier.GetArrayPointerCountEvaluation()) << ");")
        }
        LINE("m_stream->MarkFollowing(" << MakeMemberAccess("writtenData", info, member, modifier) << ");")
    }

    void WriteMember_PointerArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        LINE(MakeTypePtrVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
        if (modifier.IsArray())
        {
            LINE("WritePtrArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(false, " << modifier.GetArraySize() << ");")
        }
        else
        {
            LINE("WritePtrArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, " << MakeEvaluation(modifier.GetPointerArrayCountEvaluation()) << ");")
            LINE("m_stream->MarkFollowing("<<MakeMemberAccess("writtenData", info, member, modifier)<<");")
        }
    }

    void WriteMember_EmbeddedArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        std::string arraySizeStr;

        if (modifier.HasDynamicArrayCount())
            arraySizeStr = MakeEvaluation(modifier.GetDynamicArrayCountEvaluation());
        else
            arraySizeStr = std::to_string(modifier.GetArraySize());

        std::string memberAccess;
        if (!(info->m_definition->GetType() == DataDefinitionType::UNION && StructureComputations(info).GetDynamicMember()))
            memberAccess = MakeMemberAccess("originalData", info, member, modifier);
        else
            memberAccess = MakeMemberAccess(info, member, modifier);

        if (!member->m_is_leaf)
        {
            if (computations.IsAfterPartialLoad())
            {
                LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type)<<" = "<<memberAccess<<";")
                LINE("WriteArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, " << arraySizeStr << ");")
            }
            else
            {
                LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
                LINE("WriteArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(false, " << arraySizeStr << ");")
            }
        }
        else if (computations.IsAfterPartialLoad())
        {
            LINE("m_stream->Write<" << MakeTypeDecl(member->m_member->m_type_declaration.get()) << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers())
                << ">(" << memberAccess << ", " << arraySizeStr << ");")
        }
    }

    void WriteMember_DynamicArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        std::string memberAccess;
        if(!(info->m_definition->GetType() == DataDefinitionType::UNION && StructureComputations(info).GetDynamicMember()))
            memberAccess = MakeMemberAccess("originalData", info, member, modifier);
        else
            memberAccess = MakeMemberAccess(info, member, modifier);

        if (member->m_type && !member->m_type->m_is_leaf)
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << memberAccess << ";")
            LINE("WriteArray_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true, " << MakeEvaluation(modifier.GetDynamicArraySizeEvaluation()) << ");")
        }
        else
        {
            LINE("m_stream->Write<" << MakeTypeDecl(member->m_member->m_type_declaration.get()) << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers())
                << ">(" << memberAccess << ", " << MakeEvaluation(modifier.GetDynamicArraySizeEvaluation()) << ");")
        }
    }

    void WriteMember_Embedded(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);

        std::string memberAccess;
        if (!(info->m_definition->GetType() == DataDefinitionType::UNION && StructureComputations(info).GetDynamicMember()))
            memberAccess = MakeMemberAccess("originalData", info, member, modifier);
        else
            memberAccess = MakeMemberAccess(info, member, modifier);

        if (!member->m_is_leaf)
        {
            if (computations.IsAfterPartialLoad())
            {
                LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = &" << memberAccess << ";")
                LINE("Write_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true);")
            }
            else
            {
                LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = &" << MakeMemberAccess(info, member, modifier) << ";")
                LINE("Write_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(false);")
            }
        }
        else if (computations.IsAfterPartialLoad())
        {
            LINE("m_stream->Write<" << MakeTypeDecl(member->m_member->m_type_declaration.get()) << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers())
                << ">(&" << memberAccess << ");")
        }
    }

    void WriteMember_SinglePointer(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier) const
    {
        const MemberComputations computations(member);
        if (member->m_type && !member->m_type->m_is_leaf && !computations.IsInRuntimeBlock())
        {
            LINE(MakeTypeVarName(member->m_member->m_type_declaration->m_type) << " = " << MakeMemberAccess(info, member, modifier) << ";")
            LINE("Write_" << MakeSafeTypeName(member->m_member->m_type_declaration->m_type) << "(true);")
        }
        else
        {
            LINE("m_stream->Write<" << MakeTypeDecl(member->m_member->m_type_declaration.get()) << MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers())
                << ">(" << MakeMemberAccess(info, member, modifier) << ");")
        }
        LINE("m_stream->MarkFollowing(" << MakeMemberAccess("writtenData", info, member, modifier) << ");")
    }

    void WriteMember_TypeCheck(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType) const
    {
        if (member->m_is_string)
        {
            WriteMember_String(info, member, modifier, writeType);
        }
        else if (member->m_is_script_string)
        {
            WriteMember_ScriptString(info, member, modifier, writeType);
        }
        else if (member->m_type && StructureComputations(member->m_type).IsAsset())
        {
            WriteMember_Asset(info, member, modifier, writeType);
        }
        else
        {
            switch (writeType)
            {
            case MemberWriteType::ARRAY_POINTER:
                WriteMember_ArrayPointer(info, member, modifier);
                break;

            case MemberWriteType::SINGLE_POINTER:
                WriteMember_SinglePointer(info, member, modifier);
                break;

            case MemberWriteType::EMBEDDED:
                WriteMember_Embedded(info, member, modifier);
                break;

            case MemberWriteType::POINTER_ARRAY:
                WriteMember_PointerArray(info, member, modifier);
                break;

            case MemberWriteType::DYNAMIC_ARRAY:
                WriteMember_DynamicArray(info, member, modifier);
                break;

            case MemberWriteType::EMBEDDED_ARRAY:
                WriteMember_EmbeddedArray(info, member, modifier);
                break;

            default:
                LINE("// t=" << static_cast<int>(writeType))
                break;
            }
        }
    }

    static bool WriteMember_ShouldMakeInsertReuse(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (writeType != MemberWriteType::ARRAY_POINTER
            && writeType != MemberWriteType::SINGLE_POINTER
            && writeType != MemberWriteType::POINTER_ARRAY)
        {
            return false;
        }

        if (writeType == MemberWriteType::POINTER_ARRAY
            && modifier.IsArray())
        {
            return false;
        }

        return true;
    }

    void WriteMember_InsertReuse(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (!WriteMember_ShouldMakeInsertReuse(info, member, modifier, writeType)
            || !member->m_is_reusable)
        {
            WriteMember_TypeCheck(info, member, modifier, writeType);
            return;
        }

        if(writeType == MemberWriteType::ARRAY_POINTER)
        {
            LINE("m_stream->ReusableAddOffset("<<MakeMemberAccess(info, member, modifier)<<", "<<MakeEvaluation(modifier.GetArrayPointerCountEvaluation())<<");")
        }
        else if(writeType == MemberWriteType::POINTER_ARRAY)
        {
            LINE("m_stream->ReusableAddOffset("<<MakeMemberAccess(info, member, modifier)<<", "<<MakeEvaluation(modifier.GetPointerArrayCountEvaluation())<<");")
        }
        else
        {
            LINE("m_stream->ReusableAddOffset("<<MakeMemberAccess(info, member, modifier)<<");")
        }

        WriteMember_TypeCheck(info, member, modifier, writeType);
    }

    static bool WriteMember_ShouldMakeAlign(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (writeType != MemberWriteType::ARRAY_POINTER
            && writeType != MemberWriteType::POINTER_ARRAY
            && writeType != MemberWriteType::SINGLE_POINTER)
        {
            return false;
        }

        if (writeType == MemberWriteType::POINTER_ARRAY)
        {
            return !modifier.IsArray();
        }

        if (member->m_is_string)
        {
            return false;
        }

        if (member->m_type && StructureComputations(member->m_type).IsAsset())
        {
            return false;
        }

        return true;
    }

    void WriteMember_Align(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (!WriteMember_ShouldMakeAlign(info, member, modifier, writeType))
        {
            WriteMember_InsertReuse(info, member, modifier, writeType);
            return;
        }

        const auto typeDecl = MakeTypeDecl(member->m_member->m_type_declaration.get());
        const auto followingReferences = MakeFollowingReferences(modifier.GetFollowingDeclarationModifiers());

        if (member->m_alloc_alignment)
        {
            LINE("m_stream->Align("<<MakeEvaluation(member->m_alloc_alignment.get())<<");")
        }
        else
        {
            LINE("m_stream->Align("<<modifier.GetAlignment()<<");")
        }

        WriteMember_InsertReuse(info, member, modifier, writeType);
    }

    static bool WriteMember_ShouldMakeReuse(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (writeType != MemberWriteType::ARRAY_POINTER
            && writeType != MemberWriteType::SINGLE_POINTER
            && writeType != MemberWriteType::POINTER_ARRAY)
        {
            return false;
        }

        if (writeType == MemberWriteType::POINTER_ARRAY
            && modifier.IsArray())
        {
            return false;
        }

        return true;
    }

    void WriteMember_Reuse(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (!WriteMember_ShouldMakeReuse(info, member, modifier, writeType)
            || !member->m_is_reusable)
        {
            WriteMember_Align(info, member, modifier, writeType);
            return;
        }

        LINE("if(m_stream->ReusableShouldWrite(&" << MakeMemberAccess(info, member, modifier) << "))")
        LINE("{")
        m_intendation++;

        WriteMember_Align(info, member, modifier, writeType);

        m_intendation--;
        LINE("}")
    }

    static bool WriteMember_ShouldMakePointerCheck(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (writeType != MemberWriteType::ARRAY_POINTER
            && writeType != MemberWriteType::POINTER_ARRAY
            && writeType != MemberWriteType::SINGLE_POINTER)
        {
            return false;
        }

        if (writeType == MemberWriteType::POINTER_ARRAY)
        {
            return !modifier.IsArray();
        }

        if (member->m_is_string)
        {
            return false;
        }

        return true;
    }

    void WriteMember_PointerCheck(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, const MemberWriteType writeType)
    {
        if (WriteMember_ShouldMakePointerCheck(info, member, modifier, writeType))
        {
            LINE("if (" << MakeMemberAccess(info, member, modifier) << ")")
            LINE("{")
            m_intendation++;

            WriteMember_Reuse(info, member, modifier, writeType);

            m_intendation--;
            LINE("}")
        }
        else
        {
            WriteMember_Reuse(info, member, modifier, writeType);
        }
    }

    void WriteMember_Block(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier, MemberWriteType writeType)
    {
        const MemberComputations computations(member);

        const auto notInDefaultNormalBlock = computations.IsNotInDefaultNormalBlock();
        if (notInDefaultNormalBlock)
        {
            LINE("m_stream->PushBlock(" << member->m_fast_file_block->m_name << ");")
        }

        WriteMember_PointerCheck(info, member, modifier, writeType);

        if (notInDefaultNormalBlock)
        {
            LINE("m_stream->PopBlock();")
        }
    }

    void WriteMember_ReferenceArray(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier)
    {
        auto first = true;
        for (const auto& entry : modifier.GetArrayEntries())
        {
            if (first)
            {
                first = false;
            }
            else
            {
                LINE("")
            }

            WriteMember_Reference(info, member, entry);
        }
    }

    void WriteMember_Reference(StructureInformation* info, MemberInformation* member, const DeclarationModifierComputations& modifier)
    {
        if (modifier.IsDynamicArray())
        {
            WriteMember_Block(info, member, modifier, MemberWriteType::DYNAMIC_ARRAY);
        }
        else if (modifier.IsSinglePointer())
        {
            WriteMember_Block(info, member, modifier, MemberWriteType::SINGLE_POINTER);
        }
        else if (modifier.IsArrayPointer())
        {
            WriteMember_Block(info, member, modifier, MemberWriteType::ARRAY_POINTER);
        }
        else if (modifier.IsPointerArray())
        {
            WriteMember_Block(info, member, modifier, MemberWriteType::POINTER_ARRAY);
        }
        else if (modifier.IsArray() && modifier.GetNextDeclarationModifier() == nullptr)
        {
            WriteMember_Block(info, member, modifier, MemberWriteType::EMBEDDED_ARRAY);
        }
        else if (modifier.GetDeclarationModifier() == nullptr)
        {
            WriteMember_Block(info, member, modifier, MemberWriteType::EMBEDDED);
        }
        else if (modifier.IsArray())
        {
            WriteMember_ReferenceArray(info, member, modifier);
        }
        else
        {
            assert(false);
            LINE("#error WriteMemberReference failed @ " << member->m_member->m_name)
        }
    }

    void WriteMember_Condition_Struct(StructureInformation* info, MemberInformation* member)
    {
        LINE("")
        if (member->m_condition)
        {
            LINE("if(" << MakeEvaluation(member->m_condition.get()) << ")")
            LINE("{")
            m_intendation++;

            WriteMember_Reference(info, member, DeclarationModifierComputations(member));

            m_intendation--;
            LINE("}")
        }
        else
        {
            WriteMember_Reference(info, member, DeclarationModifierComputations(member));
        }
    }

    void WriteMember_Condition_Union(StructureInformation* info, MemberInformation* member)
    {
        const MemberComputations computations(member);

        if (computations.IsFirstMember())
        {
            LINE("")
            if (member->m_condition)
            {
                LINE("if(" << MakeEvaluation(member->m_condition.get()) << ")")
                LINE("{")
                m_intendation++;

                WriteMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
            else
            {
                WriteMember_Reference(info, member, DeclarationModifierComputations(member));
            }
        }
        else if (computations.IsLastMember())
        {
            if (member->m_condition)
            {
                LINE("else if(" << MakeEvaluation(member->m_condition.get()) << ")")
                LINE("{")
                m_intendation++;

                WriteMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
            else
            {
                LINE("else")
                LINE("{")
                m_intendation++;

                WriteMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
        }
        else
        {
            if (member->m_condition)
            {
                LINE("else if(" << MakeEvaluation(member->m_condition.get()) << ")")
                LINE("{")
                m_intendation++;

                WriteMember_Reference(info, member, DeclarationModifierComputations(member));

                m_intendation--;
                LINE("}")
            }
            else
            {
                LINE("#error Middle member of union must have condition (" << member->m_member->m_name << ")")
            }
        }
    }

    void PrintWriteMemberIfNeedsTreatment(StructureInformation* info, MemberInformation* member)
    {
        const MemberComputations computations(member);
        if (computations.ShouldIgnore())
            return;

        if (member->m_is_string
            || member->m_is_script_string
            || computations.ContainsNonEmbeddedReference()
            || member->m_type && !member->m_type->m_is_leaf
            || computations.IsAfterPartialLoad())
        {
            if (info->m_definition->GetType() == DataDefinitionType::UNION)
                WriteMember_Condition_Union(info, member);
            else
                WriteMember_Condition_Struct(info, member);
        }
    }

    void PrintWriteMethod(StructureInformation* info)
    {
        const StructureComputations computations(info);
        LINE("void " << WriterClassName(m_env.m_asset) << "::Write_" << info->m_definition->m_name << "(const bool atStreamStart)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypeVarName(info->m_definition) << " != nullptr);")

        auto* dynamicMember = computations.GetDynamicMember();
        if (!(info->m_definition->GetType() == DataDefinitionType::UNION && dynamicMember))
        {
            LINE("")

            if(dynamicMember)
            {
                LINE("auto* originalData = "<<MakeTypeVarName(info->m_definition)<<";")
            }

            LINE("if(atStreamStart)")
            m_intendation++;

            if (dynamicMember == nullptr)
            {
                LINE(MakeTypeVarName(info->m_definition)<<" = m_stream->Write<" << info->m_definition->GetFullName() << ">(" << MakeTypeVarName(info->m_definition) << "); // Size: " << info->
                    m_definition->GetSize())
            }
            else
            {
                LINE(MakeTypeVarName(info->m_definition) << " = m_stream->WritePartial<" << info->m_definition->GetFullName() << ">(" << MakeTypeVarName(info->m_definition) << ", offsetof(" << info->
                    m_definition->GetFullName()
                    << ", " << dynamicMember->m_member->m_name << "));")
            }

            m_intendation--;

            LINE("")
            LINE("auto* writtenData = "<<MakeTypeVarName(info->m_definition)<<";")
        }
        else
        {
            LINE("assert(atStreamStart);")
        }

        if (info->m_block)
        {
            LINE("")
            LINE("m_stream->PushBlock(" << info->m_block->m_name << ");")
        }
        else if (computations.IsAsset())
        {
            LINE("")
            LINE("m_stream->PushBlock(" << m_env.m_default_normal_block->m_name << ");")
        }

        for (const auto& member : info->m_ordered_members)
        {
            PrintWriteMemberIfNeedsTreatment(info, member.get());
        }

        if (info->m_block || computations.IsAsset())
        {
            LINE("")
            LINE("m_stream->PopBlock();")
        }

        m_intendation--;
        LINE("}")
    }

    void PrintWriteTempPtrMethod(StructureInformation* info)
    {
        LINE("void " << WriterClassName(m_env.m_asset) << "::WritePtr_" << MakeSafeTypeName(info->m_definition) << "(const bool atStreamStart)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypePtrVarName(info->m_definition) << " != nullptr);")
        LINE("")

        LINE("if(atStreamStart)")
        m_intendation++;
        LINE(MakeTypePtrVarName(info->m_definition)<<" = m_stream->Write<" << info->m_definition->GetFullName() << "*>(" << MakeTypePtrVarName(info->m_definition) << ");")
        m_intendation--;

        LINE("")
        LINE("m_stream->PushBlock(" << m_env.m_default_temp_block->m_name << ");")
        LINE("")
        LINE("if(m_stream->ReusableShouldWrite(" << MakeTypePtrVarName(info->m_definition) << "))")
        LINE("{")
        m_intendation++;

        LINE("m_stream->Align(" << info->m_definition->GetAlignment() << ");")
        LINE("m_stream->ReusableAddOffset(*"<<MakeTypePtrVarName(info->m_definition)<<");")
        LINE("")
        if (!info->m_is_leaf)
        {
            LINE(MakeTypeVarName(info->m_definition) << " = *" << MakeTypePtrVarName(info->m_definition) << ";")
            LINE("Write_" << MakeSafeTypeName(info->m_definition) << "(true);")
        }
        else
        {
            LINE("#error Temp method cannot have leaf type")
        }

        LINE("")
        LINE("m_stream->MarkFollowing(*"<<MakeTypePtrVarName(info->m_definition)<<");")

        m_intendation--;
        LINE("}")

        LINE("")
        LINE("m_stream->PopBlock();")

        m_intendation--;
        LINE("}")
    }

    void PrintMainWriteMethod()
    {
        LINE("void " << WriterClassName(m_env.m_asset) << "::Write(" << m_env.m_asset->m_definition->GetFullName() << "** pAsset)")
        LINE("{")
        m_intendation++;

        LINE("assert(pAsset != nullptr);")
        LINE("")
        LINE(MakeTypePtrVarName(m_env.m_asset->m_definition) << " = pAsset;")
        LINE("WritePtr_" << MakeSafeTypeName(m_env.m_asset->m_definition) << "(false);")

        m_intendation--;
        LINE("}")
    }

    void PrintGetNameMethod()
    {
        LINE("std::string " << WriterClassName(m_env.m_asset) << "::GetAssetName(" << m_env.m_asset->m_definition->GetFullName() << "* pAsset)")
        LINE("{")
        m_intendation++;

        if (!m_env.m_asset->m_name_chain.empty())
        {
            LINE_START("return pAsset")

            auto first = true;
            for (auto* member : m_env.m_asset->m_name_chain)
            {
                if (first)
                {
                    first = false;
                    LINE_MIDDLE("->" << member->m_member->m_name)
                }
                else
                {
                    LINE_MIDDLE("." << member->m_member->m_name)
                }
            }
            LINE_END(";")
        }
        else
        {
            LINE("return \"" << m_env.m_asset->m_definition->m_name << "\";")
        }

        m_intendation--;
        LINE("}")
    }

    void PrintWritePtrArrayMethod_Loading(const DataDefinition* def, StructureInformation* info, const bool reusable) const
    {
        LINE("m_stream->Align("<<def->GetAlignment()<<");")

        if(reusable)
        {
            LINE("m_stream->ReusableAddOffset(*"<<MakeTypePtrVarName(def)<< ");")
        }

        if (info && !info->m_is_leaf)
        {
            LINE(MakeTypeVarName(info->m_definition) << " = *" << MakeTypePtrVarName(def) << ";")
            LINE("Write_" << MakeSafeTypeName(def) << "(true);")
        }
        else
        {
            LINE("m_stream->Write<" << def->GetFullName() << ">(*" << MakeTypePtrVarName(def) << ");")
        }
        LINE("m_stream->MarkFollowing(*"<< MakeTypePtrVarName(def)<<");")
    }

    void PrintWritePtrArrayMethod_PointerCheck(const DataDefinition* def, StructureInformation* info, const bool reusable)
    {
        LINE("if (*" << MakeTypePtrVarName(def) << ")")
        LINE("{")
        m_intendation++;

        if (info && StructureComputations(info).IsAsset())
        {
            LINE(WriterClassName(info) << " writer(*"<<MakeTypePtrVarName(def)<<", m_zone, m_stream);")
            LINE("writer.Write(" << MakeTypePtrVarName(def) << ");")
        }
        else
        {
            if (reusable)
            {
                LINE("if(m_stream->ReusableShouldWrite(" << MakeTypePtrVarName(def) << "))")
                LINE("{")
                m_intendation++;

                PrintWritePtrArrayMethod_Loading(def, info, reusable);

                m_intendation--;
                LINE("}")
            }
            else
            {
                PrintWritePtrArrayMethod_Loading(def, info, reusable);
            }
        }

        m_intendation--;
        LINE("}")
    }

    void PrintWritePtrArrayMethod(const DataDefinition* def, StructureInformation* info, const bool reusable)
    {
        LINE("void " << WriterClassName(m_env.m_asset) << "::WritePtrArray_" << MakeSafeTypeName(def) << "(const bool atStreamStart, const size_t count)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypePtrVarName(def) << " != nullptr);")
        LINE("")

        LINE("if(atStreamStart)")
        m_intendation++;
        LINE(MakeTypePtrVarName(def)<<" = m_stream->Write<" << def->GetFullName() << "*>(" << MakeTypePtrVarName(def) << ", count);")
        m_intendation--;

        LINE("")
        LINE(def->GetFullName() << "** var = " << MakeTypePtrVarName(def) << ";")
        LINE("for(size_t index = 0; index < count; index++)")
        LINE("{")
        m_intendation++;

        LINE(MakeTypePtrVarName(def) << " = var;")
        PrintWritePtrArrayMethod_PointerCheck(def, info, reusable);
        LINE("")
        LINE("var++;")

        m_intendation--;
        LINE("}")
        m_intendation--;
        LINE("}")
    }

    void PrintWriteArrayMethod(const DataDefinition* def, StructureInformation* info)
    {
        LINE("void " << WriterClassName(m_env.m_asset) << "::WriteArray_" << MakeSafeTypeName(def) << "(const bool atStreamStart, const size_t count)")
        LINE("{")
        m_intendation++;

        LINE("assert(" << MakeTypeVarName(def) << " != nullptr);")
        LINE("")
        LINE("if(atStreamStart)")
        m_intendation++;
        LINE(MakeTypeVarName(def)<<" = m_stream->Write<" << def->GetFullName() << ">(" << MakeTypeVarName(def) << ", count);")
        m_intendation--;

        LINE("")
        LINE(def->GetFullName() << "* var = " << MakeTypeVarName(def) << ";")
        LINE("for(size_t index = 0; index < count; index++)")
        LINE("{")
        m_intendation++;

        LINE(MakeTypeVarName(info->m_definition) << " = var;")
        LINE("Write_" << info->m_definition->m_name << "(false);")
        LINE("var++;")

        m_intendation--;
        LINE("}")

        m_intendation--;
        LINE("}")
    }

public:
    Internal(std::ostream& stream, RenderingContext* context)
        : BaseTemplate(stream, context)
    {
    }

    void Header()
    {
        LINE("// ====================================================================")
        LINE("// This file has been generated by ZoneCodeGenerator.")
        LINE("// Do not modify. ")
        LINE("// Any changes will be discarded when regenerating.")
        LINE("// ====================================================================")
        LINE("")
        LINE("#pragma once")
        LINE("")
        LINE("#include \"Writing/AssetWriter.h\"")
        LINE("#include \"Game/" << m_env.m_game << "/" << m_env.m_game << ".h\"")
        LINE("#include <string>")
        LINE("")
        LINE("namespace " << m_env.m_game)
        LINE("{")
        m_intendation++;
        LINE("class " << WriterClassName(m_env.m_asset) << " final : public AssetWriter")
        LINE("{")
        m_intendation++;

        LINE(VariableDecl(m_env.m_asset->m_definition))
        LINE(PointerVariableDecl(m_env.m_asset->m_definition))
        LINE("")

        // Variable Declarations: type varType;
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_info && !type->m_info->m_definition->m_anonymous && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                LINE(VariableDecl(type->m_type))
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_pointer_array_reference_exists && !type->m_is_context_asset)
            {
                LINE(PointerVariableDecl(type->m_type))
            }
        }

        LINE("")

        // Method Declarations
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_pointer_array_reference_exists)
            {
                PrintHeaderPtrArrayWriteMethodDeclaration(type->m_type);
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_array_reference_exists && type->m_info && !type->m_info->m_is_leaf && type->m_non_runtime_reference_exists)
            {
                PrintHeaderArrayWriteMethodDeclaration(type->m_type);
            }
        }
        for (auto* type : m_env.m_used_structures)
        {
            if (type->m_non_runtime_reference_exists && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                PrintHeaderWriteMethodDeclaration(type->m_info);
            }
        }
        PrintHeaderWriteMethodDeclaration(m_env.m_asset);
        PrintHeaderTempPtrWriteMethodDeclaration(m_env.m_asset);
        LINE("")
        m_intendation--;
        LINE("public:")
        m_intendation++;
        PrintHeaderConstructor();
        PrintHeaderMainWriteMethodDeclaration(m_env.m_asset);
        PrintHeaderGetNameMethodDeclaration(m_env.m_asset);

        m_intendation--;
        LINE("};")
        m_intendation--;
        LINE("}")
    }

    void Source()
    {
        LINE("// ====================================================================")
        LINE("// This file has been generated by ZoneCodeGenerator.")
        LINE("// Do not modify. ")
        LINE("// Any changes will be discarded when regenerating.")
        LINE("// ====================================================================")
        LINE("")
        LINE("#include \"" << Lower(m_env.m_asset->m_definition->m_name) << "_write_db.h\"")
        LINE("#include <cassert>")
        LINE("")

        if (!m_env.m_referenced_assets.empty())
        {
            LINE("// Referenced Assets:")
            for (auto* type : m_env.m_referenced_assets)
            {
                LINE("#include \"../" << Lower(type->m_type->m_name) << "/" << Lower(type->m_type->m_name) << "_write_db.h\"")
            }
            LINE("")
        }
        LINE("using namespace " << m_env.m_game << ";")
        LINE("")
        PrintConstructorMethod();

        for (auto* type : m_env.m_used_types)
        {
            if (type->m_pointer_array_reference_exists)
            {
                LINE("")
                PrintWritePtrArrayMethod(type->m_type, type->m_info, type->m_pointer_array_reference_is_reusable);
            }
        }
        for (auto* type : m_env.m_used_types)
        {
            if (type->m_array_reference_exists && type->m_info && !type->m_info->m_is_leaf && type->m_non_runtime_reference_exists)
            {
                LINE("")
                PrintWriteArrayMethod(type->m_type, type->m_info);
            }
        }
        for (auto* type : m_env.m_used_structures)
        {
            if (type->m_non_runtime_reference_exists && !type->m_info->m_is_leaf && !StructureComputations(type->m_info).IsAsset())
            {
                LINE("")
                PrintWriteMethod(type->m_info);
            }
        }
        LINE("")
        PrintWriteMethod(m_env.m_asset);
        LINE("")
        PrintWriteTempPtrMethod(m_env.m_asset);
        LINE("")
        PrintMainWriteMethod();
        LINE("")
        PrintGetNameMethod();
    }
};

std::vector<CodeTemplateFile> ZoneWriteTemplate::GetFilesToRender(RenderingContext* context)
{
    std::vector<CodeTemplateFile> files;

    auto assetName = context->m_asset->m_definition->m_name;
    for (auto& c : assetName)
        c = static_cast<char>(tolower(c));

    {
        std::ostringstream str;
        str << assetName << '/' << assetName << "_write_db.h";
        files.emplace_back(str.str(), TAG_HEADER);
    }

    {
        std::ostringstream str;
        str << assetName << '/' << assetName << "_write_db.cpp";
        files.emplace_back(str.str(), TAG_SOURCE);
    }

    return files;
}

void ZoneWriteTemplate::RenderFile(std::ostream& stream, const int fileTag, RenderingContext* context)
{
    Internal internal(stream, context);

    if (fileTag == TAG_HEADER)
    {
        internal.Header();
    }
    else if (fileTag == TAG_SOURCE)
    {
        internal.Source();
    }
    else
    {
        std::cout << "Unknown tag for ZoneWriteTemplate: " << fileTag << "\n";
    }
}
