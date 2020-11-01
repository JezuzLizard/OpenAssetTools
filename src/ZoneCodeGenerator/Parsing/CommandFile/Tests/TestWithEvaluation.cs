using System;
using System.Collections.Generic;
using System.Linq;
using ZoneCodeGenerator.Domain;
using ZoneCodeGenerator.Domain.Evaluation;
using ZoneCodeGenerator.Domain.Information;
using ZoneCodeGenerator.Parsing.Matching;
using ZoneCodeGenerator.Parsing.Matching.Matchers;
using ZoneCodeGenerator.Parsing.Testing;

namespace ZoneCodeGenerator.Parsing.CommandFile.Tests
{
    abstract class TestWithEvaluation : AbstractTokenTest<ICommandParserState>
    {
        private const string TagOperand = "operand";
        private const string TagOperationType = "operationType";
        private const string TokenOperationType = "operationTypeToken";

        // Evaluation Sub-Tags
        private const string TagEvaluationParenthesis = "evaluationParenthesis";
        private const string TagEvaluationParenthesisEnd = "evaluationParenthesisEnd";
        private const string TagEvaluationNot = "evaluationNot";
        private const string TagEvaluationOperation = "evaluationOperation";

        // Operand Sub-Tags
        private const string TagOperandNumber = "operandNumber";
        private const string TagOperandTypename = "operandTypename";
        private const string TokenOperandNumber = "operandNumberToken";
        private const string TokenOperandTypename = "operandTypenameToken";
        private const string TokenOperandArray = "operandArrayToken";

        // OperandArray Sub-Tags
        private const string TagOperandArray = "operandArray";
        private const string TagOperandArrayEnd = "operandArrayEnd";

        // Visible to children
        protected const string TagEvaluation = "evaluation";

        private static readonly TokenMatcher operandArray = new MatcherGroupAnd(
            new MatcherLiteral("["),
            new MatcherWithTag(TagEvaluation),
            new MatcherLiteral("]").WithTag(TagOperandArrayEnd)
            ).WithTag(TagOperandArray);

        // operand ::= <typename> <array>* | <number>
        private static readonly TokenMatcher operand = new MatcherGroupOr(
            new MatcherGroupAnd(
                new MatcherTypename().WithName(TokenOperandTypename),
                new MatcherGroupLoop(MatcherGroupLoop.LoopMode.ZeroOneMultiple, new MatcherWithTag(TagOperandArray))
            ).WithTag(TagOperandTypename),
            new MatcherNumber().WithName(TokenOperandNumber).WithTag(TagOperandNumber)
        ).WithTag(TagOperand);

        // operationType ::= + | - | * | / | << | >> | ...
        private static readonly TokenMatcher operationType = new MatcherGroupOr(
            OperationType.Types
                .OrderByDescending(type => type.Syntax.Length)
                .Select(type => new MatcherLiteral(type.Syntax.ToCharArray().Select(c => c.ToString()).ToArray()).WithName(TokenOperationType))
                .Cast<TokenMatcher>().ToArray()
        ).WithTag(TagOperationType);

        // evaluation ::= ( <evaluation> ) | <operand> [<operationType> <evaluation>]
        private static readonly TokenMatcher evaluation = new MatcherGroupAnd(
            new MatcherGroupOr(
                new MatcherGroupAnd(
                    new MatcherGroupOptional(new MatcherLiteral("!").WithTag(TagEvaluationNot)),
                    new MatcherLiteral("("),
                    new MatcherWithTag(TagEvaluation),
                    new MatcherLiteral(")").WithTag(TagEvaluationParenthesisEnd)
                ).WithTag(TagEvaluationParenthesis),
                new MatcherWithTag(TagOperand)
            ),
            new MatcherGroupOptional(new MatcherGroupAnd(
                new MatcherWithTag(TagOperationType),
                new MatcherWithTag(TagEvaluation)
            ).WithTag(TagEvaluationOperation))

        ).WithTag(TagEvaluation);

        protected TestWithEvaluation(TokenMatcher[] matchers) : base(matchers)
        {
            AddTaggedMatcher(operandArray);
            AddTaggedMatcher(operand);
            AddTaggedMatcher(operationType);
            AddTaggedMatcher(evaluation);
        }

        private IEvaluation ProcessOperandNumber(ICommandParserState state)
        {
            var numberString = NextMatch(TokenOperandNumber);
            return new OperandStatic(int.Parse(numberString));
        }

        protected abstract IEnumerable<StructureInformation> GetUsedTypes(ICommandParserState state);

        private IEvaluation ProcessOperandTypename(ICommandParserState state)
        {
            var typenameString = NextMatch(TokenOperandTypename);
            var arrayIndexEvaluations = new List<IEvaluation>();

            while (PeekTag().Equals(TagOperandArray))
            {
                NextTag();

                if (NextTag() != TagEvaluation)
                    throw new Exception("Expected evaluation tag @ Operand");

                arrayIndexEvaluations.Add(ProcessEvaluation(state));

                if (NextTag() != TagOperandArrayEnd)
                    throw new Exception("Expected operand array end tag @ Operand");
            }

            var nameParts = typenameString.Split(new[] { "::" }, StringSplitOptions.None);

            if (nameParts.Length == 1 && arrayIndexEvaluations.Count == 0)
            {
                var enumMember = state.Repository.GetAllEnums().SelectMany(_enum => _enum.Members)
                    .FirstOrDefault(member => member.Name.Equals(nameParts[0]));

                if(enumMember != null)
                {
                    return new OperandStatic(enumMember);
                }
            }

            List<MemberInformation> referencedMemberChain = null;

            var referencedType = GetUsedTypes(state)
                .FirstOrDefault(usedType => state.GetMembersFromParts(nameParts, usedType, out referencedMemberChain));

            if (referencedType == null)
            {
                if (!state.GetTypenameAndMembersFromParts(nameParts, out referencedType,
                    out referencedMemberChain))
                {
                    throw new TestFailedException($"Could not evaluate '{typenameString}'.");
                }
            }

            var operandDynamic = new OperandDynamic(referencedType, referencedMemberChain);
            foreach (var indexEvaluation in arrayIndexEvaluations)
            {
                operandDynamic.ArrayIndices.Add(indexEvaluation);
            }

            return operandDynamic;
        }

        private IEvaluation ProcessOperand(ICommandParserState state)
        {
            var operandTypeTag = NextTag();

            if (operandTypeTag.Equals(TagOperandNumber))
            {
                return ProcessOperandNumber(state);
            }
            
            if(operandTypeTag.Equals(TagOperandTypename))
            {
                return ProcessOperandTypename(state);
            }

            throw new Exception("Unknown Operand Type");
        }

        private OperationType ProcessOperationType(ICommandParserState state)
        {
            var operationMatch = NextMatch(TokenOperationType);

            return OperationType.Types.First(type => type.Syntax.Equals(operationMatch));
        }

        private IEvaluation ProcessEvaluationInParenthesis(ICommandParserState state)
        {
            var isNegated = false;
            if (NextTag().Equals(TagEvaluationNot))
            {
                NextTag();
                isNegated = true;
            }

            var processedEvaluation = ProcessEvaluation(state);

            if (NextTag() != TagEvaluationParenthesisEnd)
                throw new Exception("Expected parenthesis end tag @ EvaluationInParenthesis");

            return !isNegated 
                ? processedEvaluation 
                : new Operation(processedEvaluation, new OperandStatic(0), OperationType.OperationEquals);
        }

        protected IEvaluation ProcessEvaluation(ICommandParserState state)
        {
            var operands = new List<IEvaluation>();
            var operators = new List<OperationType>();

            while (true)
            {
                IEvaluation firstStatementPart;
                var tag = NextTag();
                switch (tag)
                {
                    case TagEvaluationParenthesis:
                        firstStatementPart = ProcessEvaluationInParenthesis(state);
                        break;

                    case TagOperand:
                        firstStatementPart = ProcessOperand(state);
                        break;

                    default:
                        throw new Exception($"Invalid followup tag '{tag}' @ Evaluation");
                }
                operands.Add(firstStatementPart);

                if (PeekTag() == TagEvaluationOperation)
                {
                    NextTag();

                    if (NextTag() != TagOperationType)
                        throw new Exception("Expected operationType tag @ Evaluation");

                    operators.Add(ProcessOperationType(state));

                    if (NextTag() != TagEvaluation)
                        throw new Exception("Expected EvaluationTag @ Evaluation");
                }
                else
                {
                    break;
                }
            }

            while (operators.Any())
            {
                var nextOperator = operators
                    .OrderBy(type => type.Precedence)
                    .First();
                var operatorIndex = operators.IndexOf(nextOperator);

                var operation  = new Operation(operands[operatorIndex], operands[operatorIndex + 1], nextOperator);
                operands.RemoveRange(operatorIndex, 2);
                operands.Insert(operatorIndex, operation);
                operators.RemoveAt(operatorIndex);
            }

            return operands[0];
        }
    }
}