#ifndef TST_PARSER_UNIT_TESTS_H
#define TST_PARSER_UNIT_TESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "parser.h"

using namespace testing;

class Config_searching_testing : public testing::Test
{
protected:
    Config_searching c_s_default;
    Config_searching c_s_c_string{"pet_project_config.ini"};
    Config_searching c_s_std_string{std::string("pet_project_config.ini")};
};

class Parser_DB_testing : public testing::Test
{
protected:
    Parser_DB parser_constructed{"/home/nikita/C++/Pet_project/ini_and_parser/pet_project_config.ini"};
    void keys_checking(const char* const* ptr_k)
    {
        EXPECT_STREQ("host", ptr_k[0]);
        EXPECT_STREQ("hostaddr", ptr_k[1]);
        EXPECT_STREQ("port", ptr_k[2]);
        EXPECT_STREQ("dbname", ptr_k[3]);
        EXPECT_STREQ("password", ptr_k[4]);
        EXPECT_STREQ("connect_timeout", ptr_k[5]);
        EXPECT_STREQ("client_encoding", ptr_k[6]);
        EXPECT_STREQ("sslmode", ptr_k[7]);
    }
    void values_checking(const char* const* ptr_v)
    {
        EXPECT_STREQ("localhost", ptr_v[0]);
        EXPECT_STREQ("127.0.0.1", ptr_v[1]);
        EXPECT_STREQ("5432", ptr_v[2]);
        EXPECT_STREQ("pet_project_db", ptr_v[3]);
        EXPECT_STREQ("pet_project_password", ptr_v[4]);
        EXPECT_STREQ("", ptr_v[5]);
        EXPECT_STREQ("auto", ptr_v[6]);
        EXPECT_STREQ("prefer", ptr_v[7]);
    }
    void p_DB_checking(const Parser_DB& p_DB)
    {
        //Keys checking
        const char* const* ptr_ = p_DB.parsed_info_ptr();
        keys_checking(ptr_);
        const char* const* ptr_k = p_DB.parsed_info_ptr('k');
        keys_checking(ptr_k);
        const char* const* ptr_K = p_DB.parsed_info_ptr('K');
        keys_checking(ptr_K);
        //Values checking
        const char* const* ptr_v = p_DB.parsed_info_ptr('v');
        values_checking(ptr_v);
        const char* const* ptr_V = p_DB.parsed_info_ptr('v');
        values_checking(ptr_V);
    }
};


#endif // TST_PARSER_UNIT_TESTS_H
