#ifndef TST_DB_MODULE_UNIT_TESTS_H
#define TST_DB_MODULE_UNIT_TESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include "DB_module.h"

using namespace testing;

class connection_pool_testing : public testing::Test{
protected:
    connection_pool c_p_zero{0};
    connection_pool c_p_dft_conninfo{4};
    PGconn* conn{nullptr};
    void pull_conns(int amount)
    {
        for (int i = 0; i != amount; ++i)
        {
            EXPECT_FALSE(static_cast<bool>(conn));
            EXPECT_TRUE(c_p_dft_conninfo.pull_connection(conn));
            EXPECT_TRUE(static_cast<bool>(conn));
            conn = nullptr;
        }
        EXPECT_FALSE(c_p_dft_conninfo.pull_connection(conn));
    }
};

class MockParser : public Parser
{
public:
    MockParser();
    MOCK_METHOD(const char* const*, parsed_info_ptr, (char m), (const, override));
    MOCK_METHOD(void, display, (), (const, override));
};

class MockParserHelper
{
private:
    const char* k[9] {"host", "port", "dbname", "password", "connect_timeout", "client_encoding", "sslmode", nullptr};
    const char* v[9] {"localhost", "5432", "pet_project_db", "pet_project_password", "", "auto", "prefer", nullptr};
public:
    constexpr MockParserHelper() = default;
    const char* const* returning_massives(char m) const;
};

#endif // TST_DB_MODULE_UNIT_TESTS_H
