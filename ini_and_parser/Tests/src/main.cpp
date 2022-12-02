#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "tst_parser_unit_tests.h"
#include "parser.h"

namespace parser_unit_tests
{
const char* valid_path_to_ini_file = "/home/nikita/C++/Pet_project/ini_and_parser/Tests/pet_project_config.ini";
const char* wrong_path_to_ini_file = "/home/nikita/C++/Pet_project/ini_and_parser/Tests/no_file.ini";

//*******************************************Config_searching*******************************************

TEST_F(ConfigSearchingTesting, DefaultCtor)
{
    EXPECT_EQ(std::string{}, c_s_default.return_path());
}

TEST_F(ConfigSearchingTesting, CStringCtor)
{
    EXPECT_EQ(std::string(valid_path_to_ini_file), c_s_c_string.return_path());
}

TEST_F(ConfigSearchingTesting, STDStringCtor)
{
    EXPECT_EQ(std::string(valid_path_to_ini_file), c_s_std_string.return_path());
}

TEST(ConfigSearching, NoPathFound)
{
    EXPECT_THROW(Config_searching("Wrong ini filename"), c_s_exception);
}

TEST(ConfigSearching, CSExceptionMessage)
{
    try {
        Config_searching{wrong_path_to_ini_file};
    }  catch (c_s_exception e) {
        std::string what_message = wrong_path_to_ini_file + std::string{" not founded"};
        EXPECT_STREQ(what_message.c_str(), e.what());
    }
}

//*******************************************Parser*******************************************

using Implementations = ::testing::Types<Parser_DB, Parser_Inotify, Parser_Server_HTTP, Parser_Client_HTTP>;

TYPED_TEST_SUITE(ParserDefaultCtor, Implementations);
TYPED_TEST_SUITE(ParserCStringCtor, Implementations);
TYPED_TEST_SUITE(ParserStdStringCtor, Implementations);
TYPED_TEST_SUITE(ParserWrongIniFileCtor, Implementations);
TYPED_TEST_SUITE(ParserPropTreeCtor, Implementations);

TYPED_TEST(ParserDefaultCtor, DefaultCtor)
{
    EXPECT_EQ(nullptr, this->parser->parsed_info_ptr());
    EXPECT_EQ(nullptr, this->parser->parsed_info_ptr('k'));
    EXPECT_EQ(nullptr, this->parser->parsed_info_ptr('v'));
    EXPECT_EQ(nullptr, this->parser->parsed_info_ptr('K'));
    EXPECT_EQ(nullptr, this->parser->parsed_info_ptr('V'));
}

TYPED_TEST(ParserCStringCtor, CStringCtor)
{
    this->keys_values_checking();
}

TYPED_TEST(ParserStdStringCtor, STDStringCtor)
{
    this->keys_values_checking();
}

TYPED_TEST(ParserWrongIniFileCtor, WrongIniFileCtors)
{
    //Nothing to do here, everything is tested in ParserWrongIniFileCtor::SetUp()
}

TYPED_TEST(ParserPropTreeCtor, PropTreeCtor)
{
    this->keys_values_checking();
}

}   //namespace parser_unit_tests

int main(int argc, char *argv[])
{
    using namespace parser_unit_tests;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
