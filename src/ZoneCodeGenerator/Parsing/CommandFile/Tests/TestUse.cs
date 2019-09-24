﻿using ZoneCodeGenerator.Domain;
using ZoneCodeGenerator.Parsing.Matching;
using ZoneCodeGenerator.Parsing.Matching.Matchers;
using ZoneCodeGenerator.Parsing.Testing;

namespace ZoneCodeGenerator.Parsing.CommandFile.Tests
{
    class TestUse : AbstractTokenTest<ICommandParserState>
    {
        private const string TypeNameToken = "typeName";

        private static readonly TokenMatcher[] matchers = {
            new MatcherLiteral("use"),
            new MatcherTypename().WithName(TypeNameToken),
            new MatcherLiteral(";")
        };

        public TestUse() : base(matchers)
        {

        }

        protected override void ProcessMatch(ICommandParserState state)
        {
            var typeName = GetMatcherTokens(TypeNameToken)[0];
            var dataTypeToUse = state.Repository.GetDataTypeByName(typeName);

            if (dataTypeToUse == null)
            {
                throw new LoadingException($"Could not find data type '{typeName}'");
            }

            if (!(dataTypeToUse is DataTypeWithMembers dataTypeWithMembersToUse))
            {
                throw new LoadingException($"To use data type '{typeName}' it must either be a struct or a union.");
            }

            state.DataTypeInUse = dataTypeWithMembersToUse;
        }
    }
}
