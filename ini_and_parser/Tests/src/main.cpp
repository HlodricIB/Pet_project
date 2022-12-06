#include <type_traits>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "tst_parser_unit_tests.h"
#include "parser.h"

namespace parser_unit_tests
{
const char* valid_path_to_ini_file = "/home/nikita/C++/Pet_project/ini_and_parser/Tests/pet_project_config.ini";
const char* wrong_path_to_ini_file = "/home/nikita/C++/Pet_project/ini_and_parser/Tests/no_file.ini";
const char* path_to_ini_file_less = "/home/nikita/C++/Pet_project/ini_and_parser/Tests/pet_project_config_less.ini";
const char* path_to_ini_file_wrong_key_name = "/home/nikita/C++/Pet_project/ini_and_parser/Tests/pet_project_config_wrong_key_name.ini";

//*******************************************Config_searching*******************************************

TEST_F(ConfigSearchingTesting, Ctors)
{
    EXPECT_EQ(std::string{}, c_s_default.return_path());
    EXPECT_EQ(std::string(valid_path_to_ini_file), c_s_c_string.return_path());
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

using ImplementationsAll = ::testing::Types<Parser_DB, Parser_Inotify, Parser_Server_HTTP, Parser_Client_HTTP>;
using ImplementationsPartial = ::testing::Types<Parser_Inotify, Parser_Server_HTTP, Parser_Client_HTTP>;

TYPED_TEST_SUITE(ParserDefaultCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserCStringCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserStdStringCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserWrongIniFileCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserPropTreeCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserConfigSearchingCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserCopyCtor, ImplementationsAll);
TYPED_TEST_SUITE(ParserValidateLess, ImplementationsAll);
TYPED_TEST_SUITE(ParserValidateWrongKeyName, ImplementationsPartial);

TYPED_TEST(ParserDefaultCtor, DefaultCtor)
{
    this->default_initialized();
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

TYPED_TEST(ParserConfigSearchingCtor, PropTreeCtor)
{
    this->keys_values_checking();
}

TYPED_TEST(ParserCopyCtor, CopyCtor)
{
    //Nothing to do here, everything is tested in ParserCopyCtor::SetUp()
}

TYPED_TEST(ParserCStringCtor, ValidateAllOk)
{
    auto temp_ok = std::make_pair<bool, std::string_view>(true, std::string_view{});
    EXPECT_EQ(temp_ok, this->parser->validate_parsed());
}

TYPED_TEST(ParserValidateLess, ValidateLess)
{
    std::pair<bool, std::string_view> temp_less;
    if (std::is_same_v<TypeParam, Parser_DB>)
    {
        temp_less = std::make_pair<bool, std::string_view>(false, std::string_view{"No data was parsed, may be because "
                                                                            "there is no appropriate information to parse in ini-file"});
    } else {
        temp_less = std::make_pair<bool, std::string_view>(false, std::string_view{"Parsed count of keywords is less than it needed"});
    }
    EXPECT_EQ(temp_less, this->parser->validate_parsed());
}

TYPED_TEST(ParserValidateWrongKeyName, ValidateWrongKeyName)
{
    std::pair<bool, std::string_view> temp_wrong_key_name;
    if (std::is_same_v<TypeParam, Parser_Inotify>)
    {
        temp_wrong_key_name = std::make_pair<bool, std::string_view>(false, std::string_view{"max_log_file_size"});
    } else {
        if (std::is_same_v<TypeParam, Parser_Server_HTTP>)
        {
            temp_wrong_key_name = std::make_pair<bool, std::string_view>(false, std::string_view{"files_folder"});
        } else {
            if (std::is_same_v<TypeParam, Parser_Client_HTTP>)
            {
                temp_wrong_key_name = std::make_pair<bool, std::string_view>(false, std::string_view{"version"});
            }
        }
    }
    EXPECT_EQ(temp_wrong_key_name, this->parser->validate_parsed());
}


}   //namespace parser_unit_tests

int main(int argc, char *argv[])
{
    using namespace parser_unit_tests;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
