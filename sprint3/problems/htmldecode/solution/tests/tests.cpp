#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

#include "../src/htmldecode.h"

using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") {
    CHECK(HtmlDecode(""sv) == ""sv);
    CHECK(HtmlDecode("hello"sv) == "hello"sv);
    CHECK(HtmlDecode("Hello World!"sv) == "Hello World!"sv);
    CHECK(HtmlDecode("12345"sv) == "12345"sv);
}

TEST_CASE("Decode lt entity", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;"sv) == "<"sv);
    CHECK(HtmlDecode("&lt"sv) == "<"sv);
    CHECK(HtmlDecode("&LT;"sv) == "<"sv);
    CHECK(HtmlDecode("&LT"sv) == "<"sv);
    // &ltb - не мнемоника, не декодируется
    CHECK(HtmlDecode("a&ltb"sv) == "a&ltb"sv);
}

TEST_CASE("Decode gt entity", "[HtmlDecode]") {
    CHECK(HtmlDecode("&gt;"sv) == ">"sv);
    CHECK(HtmlDecode("&gt"sv) == ">"sv);
    CHECK(HtmlDecode("&GT;"sv) == ">"sv);
    CHECK(HtmlDecode("&GT"sv) == ">"sv);
    // &gtb - не мнемоника, не декодируется
    CHECK(HtmlDecode("a&gtb"sv) == "a&gtb"sv);
}

TEST_CASE("Decode amp entity", "[HtmlDecode]") {
    CHECK(HtmlDecode("&amp;"sv) == "&"sv);
    CHECK(HtmlDecode("&amp"sv) == "&"sv);
    CHECK(HtmlDecode("&AMP;"sv) == "&"sv);
    CHECK(HtmlDecode("&AMP"sv) == "&"sv);
    CHECK(HtmlDecode("Johnson&amp;Johnson"sv) == "Johnson&Johnson"sv);
    CHECK(HtmlDecode("M&amp;M"sv) == "M&M"sv);
}

TEST_CASE("Decode apos entity", "[HtmlDecode]") {
    CHECK(HtmlDecode("&apos;"sv) == "'"sv);
    CHECK(HtmlDecode("&apos"sv) == "'"sv);
    CHECK(HtmlDecode("&APOS;"sv) == "'"sv);
    CHECK(HtmlDecode("&APOS"sv) == "'"sv);
    CHECK(HtmlDecode("M&amp;M&APOS;s"sv) == "M&M's"sv);
    // &APOSs - не мнемоника (слишком длинная), не декодируется
    CHECK(HtmlDecode("M&amp;M&APOSs"sv) == "M&M&APOSs"sv);
}

TEST_CASE("Decode quot entity", "[HtmlDecode]") {
    CHECK(HtmlDecode("&quot;"sv) == "\""sv);
    CHECK(HtmlDecode("&quot"sv) == "\""sv);
    CHECK(HtmlDecode("&QUOT;"sv) == "\""sv);
    CHECK(HtmlDecode("&QUOT"sv) == "\""sv);
    CHECK(HtmlDecode("He said &quot;Hello&quot;"sv) == "He said \"Hello\""sv);
}

TEST_CASE("Mixed case is not a mnemonic", "[HtmlDecode]") {
    CHECK(HtmlDecode("&aPos;"sv) == "&aPos;"sv);
    CHECK(HtmlDecode("&Lt;"sv) == "&Lt;"sv);
    CHECK(HtmlDecode("&lT;"sv) == "&lT;"sv);
    CHECK(HtmlDecode("&AmP;"sv) == "&AmP;"sv);
    CHECK(HtmlDecode("&QuOt;"sv) == "&QuOt;"sv);
}

TEST_CASE("Unknown entities remain unchanged", "[HtmlDecode]") {
    CHECK(HtmlDecode("&abracadabra"sv) == "&abracadabra"sv);
    CHECK(HtmlDecode("&unknown;"sv) == "&unknown;"sv);
    CHECK(HtmlDecode("&ABC;"sv) == "&ABC;"sv);
    CHECK(HtmlDecode("text&ampnbspmore"sv) == "text&ampnbspmore"sv);
}

TEST_CASE("One pass decoding - no recursive decoding", "[HtmlDecode]") {
    CHECK(HtmlDecode("&amp;lt;"sv) == "&lt;"sv);
    CHECK(HtmlDecode("&amp;lt"sv) == "&lt"sv);
    CHECK(HtmlDecode("&AMP;LT;"sv) == "&LT;"sv);
    CHECK(HtmlDecode("&amp;gt;"sv) == "&gt;"sv);
    CHECK(HtmlDecode("&amp;amp;"sv) == "&amp;"sv);
}

TEST_CASE("Multiple entities in sequence", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;&gt;&amp;"sv) == "<>&"sv);
    CHECK(HtmlDecode("&lt;&gt;&amp;&apos;&quot;"sv) == "<>&'\""sv);
    CHECK(HtmlDecode("a&lt;b&gt;c&amp;d"sv) == "a<b>c&d"sv);
}

TEST_CASE("Entities at boundaries", "[HtmlDecode]") {
    CHECK(HtmlDecode("&lt;div&gt;"sv) == "<div>"sv);
    CHECK(HtmlDecode("start&lt;end"sv) == "start<end"sv);
    // &gtend - не мнемоника, не декодируется
    CHECK(HtmlDecode("start&gtend"sv) == "start&gtend"sv);
}

TEST_CASE("Partial mnemonic at end", "[HtmlDecode]") {
    CHECK(HtmlDecode("test&amp"sv) == "test&"sv);
    CHECK(HtmlDecode("test&amp;"sv) == "test&"sv);
    CHECK(HtmlDecode("test&ampnbsp"sv) == "test&ampnbsp"sv);
}

TEST_CASE("Multiple & characters", "[HtmlDecode]") {
    CHECK(HtmlDecode("&&&"sv) == "&&&"sv);
    CHECK(HtmlDecode("&amp;&amp;&amp;"sv) == "&&&"sv);
    CHECK(HtmlDecode("a&amp;b&amp;c"sv) == "a&b&c"sv);
}
