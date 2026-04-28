#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/included/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(decode_simple_percent_encoding) {
    BOOST_CHECK_EQUAL(UrlDecode("Hello%20World"), "Hello World");
    BOOST_CHECK_EQUAL(UrlDecode("%20"), " ");
    BOOST_CHECK_EQUAL(UrlDecode("%21"), "!");
    BOOST_CHECK_EQUAL(UrlDecode("%2B"), "+");
    BOOST_CHECK_EQUAL(UrlDecode("%3F"), "?");
}

BOOST_AUTO_TEST_CASE(decode_plus_as_space) {
    BOOST_CHECK_EQUAL(UrlDecode("Hello+World"), "Hello World");
    BOOST_CHECK_EQUAL(UrlDecode("a+b+c"), "a b c");
}

BOOST_AUTO_TEST_CASE(decode_mixed_encoding) {
    BOOST_CHECK_EQUAL(UrlDecode("Hello%20World+Test"), "Hello World Test");
    BOOST_CHECK_EQUAL(UrlDecode("%21%22%23"), "!\"#");
}

BOOST_AUTO_TEST_CASE(decode_lowercase_and_uppercase_hex) {
    BOOST_CHECK_EQUAL(UrlDecode("%2f"), "/");
    BOOST_CHECK_EQUAL(UrlDecode("%2F"), "/");
    BOOST_CHECK_EQUAL(UrlDecode("%2a%42%3c"), "*B<");
    BOOST_CHECK_EQUAL(UrlDecode("%2A%42%3C"), "*B<");
}

BOOST_AUTO_TEST_CASE(decode_unreserved_characters) {
    BOOST_CHECK_EQUAL(UrlDecode("abc-123_def.test"), "abc-123_def.test");
    BOOST_CHECK_EQUAL(UrlDecode("Hello~World"), "Hello~World");
}

BOOST_AUTO_TEST_CASE(decode_reserved_characters_unencoded) {
    // Незакодированные зарезервированные символы остаются как есть
    BOOST_CHECK_EQUAL(UrlDecode("Hello!World"), "Hello!World");
    BOOST_CHECK_EQUAL(UrlDecode("test@example.com"), "test@example.com");
    BOOST_CHECK_EQUAL(UrlDecode("a?b=c"), "a?b=c");
    BOOST_CHECK_EQUAL(UrlDecode("a[b]c"), "a[b]c");
    BOOST_CHECK_EQUAL(UrlDecode("a;bc"), "a;bc");
    BOOST_CHECK_EQUAL(UrlDecode("a@bc"), "a@bc");
}

BOOST_AUTO_TEST_CASE(incomplete_percent_encoding_throws) {
    BOOST_CHECK_THROW(UrlDecode("%"), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%2"), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("Hello%2"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(invalid_percent_encoding_throws) {
    BOOST_CHECK_THROW(UrlDecode("%GG"), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%2G"), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%G2"), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("Hello%ZZWorld"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(empty_string) {
    BOOST_CHECK_EQUAL(UrlDecode(""), "");
}

BOOST_AUTO_TEST_CASE(no_encoding_needed) {
    BOOST_CHECK_EQUAL(UrlDecode("Hello World !"), "Hello World !");
    BOOST_CHECK_EQUAL(UrlDecode("abc123"), "abc123");
}