#include <catch2/catch.hpp>

#include "Parsing/Impl/DefinesStreamProxy.h"
#include "Parsing/Mock/MockParserLineStream.h"

namespace test::parsing::impl::defines_stream_proxy
{
    void ExpectLine(IParserLineStream* stream, const int lineNumber, const std::string& value)
    {
        auto line = stream->NextLine();
        REQUIRE(line.m_line_number == lineNumber);
        REQUIRE(line.m_line == value);
    }

    TEST_CASE("DefinesStreamProxy: Ensure simple define and positive ifdef is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifdef ASDF",
            "Hello World",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "Hello World");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure simple define and negative ifdef is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifdef NONO",
            "Hello World",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure simple define and positive ifndef is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifndef NONO",
            "Hello World",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "Hello World");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure simple define and negative ifndef is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifndef ASDF",
            "Hello World",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure else is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifdef NONO",
            "Hello World1",
            "#else",
            "Hello World2",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "Hello World2");
        ExpectLine(&proxy, 6, "");
        ExpectLine(&proxy, 7, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure nested ifdef is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifdef ASDF",
            "#ifdef NONO",
            "Hello World1",
            "#else",
            "Hello World2",
            "#endif",
            "#else",
            "#ifdef ASDF",
            "Hello World3",
            "#else",
            "Hello World4",
            "#endif",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "");
        ExpectLine(&proxy, 6, "Hello World2");
        ExpectLine(&proxy, 7, "");
        ExpectLine(&proxy, 8, "");
        ExpectLine(&proxy, 9, "");
        ExpectLine(&proxy, 10, "");
        ExpectLine(&proxy, 11, "");
        ExpectLine(&proxy, 12, "");
        ExpectLine(&proxy, 13, "");
        ExpectLine(&proxy, 14, "");
        ExpectLine(&proxy, 15, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure undef is working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifdef ASDF",
            "Hello World",
            "#endif",
            "#undef ASDF",
            "#ifdef ASDF",
            "Hello World",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "Hello World");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "");
        ExpectLine(&proxy, 6, "");
        ExpectLine(&proxy, 7, "");
        ExpectLine(&proxy, 8, "");
        ExpectLine(&proxy, 9, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure undef does not undefine everything", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF",
            "#ifdef ASDF",
            "Hello World",
            "#endif",
            "#undef NONO",
            "#ifdef ASDF",
            "Hello World",
            "#endif",
            "Hello Galaxy"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "Hello World");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "");
        ExpectLine(&proxy, 6, "");
        ExpectLine(&proxy, 7, "Hello World");
        ExpectLine(&proxy, 8, "");
        ExpectLine(&proxy, 9, "Hello Galaxy");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure simple defines are working", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF LOL",
            "ASDF",
            "A ASDF B",
            "ASDF B",
            "A ASDF"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "LOL");
        ExpectLine(&proxy, 3, "A LOL B");
        ExpectLine(&proxy, 4, "LOL B");
        ExpectLine(&proxy, 5, "A LOL");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure defines can be surrounded by symbols", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define ASDF LOL",
            "!ASDF%"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "!LOL%");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure can use multiple defines in one line", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define A Hello",
            "#define B world",
            "A my dear B!"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "Hello my dear world!");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure defines in disabled block are ignored", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#ifdef LOLO",
            "#define hello world",
            "#endif",
            "hello"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "");
        ExpectLine(&proxy, 4, "hello");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure undefs in disabled block are ignored", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define hello world",
            "#ifdef LOLO",
            "#undef hello",
            "#endif",
            "hello"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "");
        ExpectLine(&proxy, 3, "");
        ExpectLine(&proxy, 4, "");
        ExpectLine(&proxy, 5, "world");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure can define name with underscores and digits", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define __int16 short",
            "unsigned __int16 value;"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "unsigned short value;");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure define can render parameters", "[parsing][parsingstream]")
    {
        DefinesStreamProxy::Define define("helloworld", "hello universe");

        std::vector<std::string> parameterNames({
            "universe"
        });
        define.IdentifyParameters(parameterNames);

        std::vector<std::string> parameterValues({
            "mr moneyman"
        });
        REQUIRE(define.Render(parameterValues) == "hello mr moneyman");
    }

    TEST_CASE("DefinesStreamProxy: Ensure define can render parameters in middle of symbols", "[parsing][parsingstream]")
    {
        DefinesStreamProxy::Define define("helloworld", "alignas(x)");

        std::vector<std::string> parameterNames({
            "x"
        });
        define.IdentifyParameters(parameterNames);

        std::vector<std::string> parameterValues({
            "1337"
        });
        REQUIRE(define.Render(parameterValues) == "alignas(1337)");
    }

    TEST_CASE("DefinesStreamProxy: Ensure can add define with parameters", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define test(x) alignas(x)",
            "struct test(1337) test_struct"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "struct alignas(1337) test_struct");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure can use parameter multiple times", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define test(x) x|x|x|x",
            "struct test(1337) test_struct"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "struct 1337|1337|1337|1337 test_struct");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure can use parameterized define in between symbols", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define test(x) x|x|x|x",
            "%test(5)%"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "%5|5|5|5%");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure can define multiple parameters", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define test(a1, a2, a3) a1 + a2 = a3",
            "make calc test(1, 2, 3);"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "make calc 1 + 2 = 3;");

        REQUIRE(proxy.Eof());
    }

    TEST_CASE("DefinesStreamProxy: Ensure can define multiple parameters without spacing", "[parsing][parsingstream]")
    {
        const std::vector<std::string> lines
        {
            "#define test(a1,a2,a3) a1+a2=a3",
            "make calc test(1,2,3);"
        };

        MockParserLineStream mockStream(lines);
        DefinesStreamProxy proxy(&mockStream);

        ExpectLine(&proxy, 1, "");
        ExpectLine(&proxy, 2, "make calc 1+2=3;");

        REQUIRE(proxy.Eof());
    }
}
