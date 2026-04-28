#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
}

TEST(UrlEncodeTestSuite, SpaceIsEncodedAsPlus) {
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);
}

TEST(UrlEncodeTestSuite, ReservedCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("$"sv), "%24"s);
    EXPECT_EQ(UrlEncode("!"sv), "%21"s);
    EXPECT_EQ(UrlEncode("#"sv), "%23"s);
    EXPECT_EQ(UrlEncode("&"sv), "%26"s);
    EXPECT_EQ(UrlEncode("'"sv), "%27"s);
    EXPECT_EQ(UrlEncode("("sv), "%28"s);
    EXPECT_EQ(UrlEncode(")"sv), "%29"s);
    EXPECT_EQ(UrlEncode("*"sv), "%2A"s);
    EXPECT_EQ(UrlEncode(","sv), "%2C"s);
    EXPECT_EQ(UrlEncode("/"sv), "%2F"s);
    EXPECT_EQ(UrlEncode(":"sv), "%3A"s);
    EXPECT_EQ(UrlEncode(";"sv), "%3B"s);
    EXPECT_EQ(UrlEncode("="sv), "%3D"s);
    EXPECT_EQ(UrlEncode("?"sv), "%3F"s);
    EXPECT_EQ(UrlEncode("@"sv), "%40"s);
    EXPECT_EQ(UrlEncode("["sv), "%5B"s);
    EXPECT_EQ(UrlEncode("]"sv), "%5D"s);
}

TEST(UrlEncodeTestSuite, AsteriskIsEncoded) {
    EXPECT_EQ(UrlEncode("abc*"sv), "abc%2A"s);
}

TEST(UrlEncodeTestSuite, ControlCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("\x00"sv), "%00"s);
    EXPECT_EQ(UrlEncode("\x1F"sv), "%1F"s);
    EXPECT_EQ(UrlEncode("\x0A"sv), "%0A"s);
    EXPECT_EQ(UrlEncode("\x0D"sv), "%0D"s);
}

TEST(UrlEncodeTestSuite, HighCharsAreEncoded) {
    EXPECT_EQ(UrlEncode("\x80"sv), "%80"s);
    EXPECT_EQ(UrlEncode("\xFF"sv), "%FF"s);
}

TEST(UrlEncodeTestSuite, UnreservedCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("abcdefghijklmnopqrstuvwxyz"sv), "abcdefghijklmnopqrstuvwxyz"s);
    EXPECT_EQ(UrlEncode("ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv), "ABCDEFGHIJKLMNOPQRSTUVWXYZ"s);
    EXPECT_EQ(UrlEncode("0123456789"sv), "0123456789"s);
    EXPECT_EQ(UrlEncode("-._~"sv), "-._~"s);
}

TEST(UrlEncodeTestSuite, MixedString) {
    EXPECT_EQ(UrlEncode("Hello, World!"sv), "Hello%2C+World%21"s);
    EXPECT_EQ(UrlEncode("foo bar baz"sv), "foo+bar+baz"s);
}

TEST(UrlEncodeTestSuite, EmptyString) {
    EXPECT_EQ(UrlEncode(""sv), ""s);
}
