#define BOOST_TEST_MODULE TV tests
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sstream>

#include "../src/tv.h"
#include "boost_test_helpers.h"

struct TVFixture {
    TV tv;
};
BOOST_FIXTURE_TEST_SUITE(TV_, TVFixture)
BOOST_AUTO_TEST_CASE(is_off_by_default) {
    // Внутри теста поля структуры TVFixture доступны по их имени
    BOOST_TEST(!tv.IsTurnedOn());
}
BOOST_AUTO_TEST_CASE(doesnt_show_any_channel_by_default) {
    BOOST_TEST(!tv.GetChannel().has_value());
}
// Включите этот тест и доработайте класс TV, чтобы тест выполнился успешно
BOOST_AUTO_TEST_CASE(cant_select_any_channel_when_it_is_off) {
    BOOST_CHECK_THROW(tv.SelectChannel(10), std::logic_error);
    BOOST_TEST(tv.GetChannel() == std::nullopt);
    tv.TurnOn();
    BOOST_TEST(tv.GetChannel() == 1);
}

// Тестовый стенд "Включенный телевизор" унаследован от TVFixture.
struct TurnedOnTVFixture : TVFixture {
    // В конструкторе выполняем донастройку унаследованного поля tv
    TurnedOnTVFixture() {
        tv.TurnOn();
    }
};
// (Телевизор) после включения
BOOST_FIXTURE_TEST_SUITE(After_turning_on_, TurnedOnTVFixture)
// показывает канал #1
BOOST_AUTO_TEST_CASE(shows_channel_1) {
    BOOST_TEST(tv.IsTurnedOn());
    BOOST_TEST(tv.GetChannel() == 1);
}
// Может быть выключен
BOOST_AUTO_TEST_CASE(can_be_turned_off) {
    tv.TurnOff();
    BOOST_TEST(!tv.IsTurnedOn());
    BOOST_TEST(tv.GetChannel() == std::nullopt);
}
// Может выбирать каналы с 1 по 99
BOOST_AUTO_TEST_CASE(can_select_channel_from_1_to_99) {
    BOOST_TEST(tv.GetChannel() == 1);
    
    tv.SelectChannel(42);
    BOOST_TEST(tv.GetChannel() == 42);
    
    tv.SelectChannel(99);
    BOOST_TEST(tv.GetChannel() == 99);
    
    tv.SelectChannel(1);
    BOOST_TEST(tv.GetChannel() == 1);
}

// Отклоняет каналы за пределами диапазона
BOOST_AUTO_TEST_CASE(rejects_channels_out_of_range) {
    BOOST_CHECK_THROW(tv.SelectChannel(0), std::out_of_range);
    BOOST_CHECK_THROW(tv.SelectChannel(100), std::out_of_range);
    BOOST_CHECK_THROW(tv.SelectChannel(-5), std::out_of_range);
    BOOST_CHECK_THROW(tv.SelectChannel(200), std::out_of_range);
}

// Запоминает предыдущий канал при переключении
BOOST_AUTO_TEST_CASE(remembers_previous_channel) {
    tv.SelectChannel(10);
    BOOST_TEST(tv.GetChannel() == 10);
    
    tv.SelectChannel(20);
    BOOST_TEST(tv.GetChannel() == 20);
    
    tv.SelectLastViewedChannel();
    BOOST_TEST(tv.GetChannel() == 10);
}

// Не меняет канал при повторном выборе того же канала
BOOST_AUTO_TEST_CASE(ignores_selecting_same_channel) {
    int current_channel = tv.GetChannel().value();
    tv.SelectChannel(current_channel);
    BOOST_TEST(tv.GetChannel() == current_channel);
}

// SelectLastViewedChannel не меняет канал, если никогда не переключали
BOOST_AUTO_TEST_CASE(last_viewed_unchanged_when_no_switch) {
    BOOST_CHECK_THROW(tv.SelectLastViewedChannel(), std::logic_error);
    tv.TurnOn();
    BOOST_TEST(tv.GetChannel() == 1);
    BOOST_CHECK_THROW(tv.SelectLastViewedChannel(), std::logic_error);
}

// SelectLastViewedChannel переключает между двумя каналами
BOOST_AUTO_TEST_CASE(toggles_between_two_channels) {
    tv.SelectChannel(5);
    tv.SelectChannel(10);
    BOOST_TEST(tv.GetChannel() == 10);
    
    tv.SelectLastViewedChannel();
    BOOST_TEST(tv.GetChannel() == 5);
    
    tv.SelectLastViewedChannel();
    BOOST_TEST(tv.GetChannel() == 10);
    
    tv.SelectLastViewedChannel();
    BOOST_TEST(tv.GetChannel() == 5);
}

// После выключения и включения последний канал сбрасывается
BOOST_AUTO_TEST_CASE(resets_last_channel_after_turn_off_on) {
    tv.SelectChannel(10);
    tv.SelectChannel(20);
    tv.TurnOff();
    tv.TurnOn();
    
    BOOST_TEST(tv.GetChannel() == 20);
    BOOST_CHECK_THROW(tv.SelectLastViewedChannel(), std::logic_error);
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
