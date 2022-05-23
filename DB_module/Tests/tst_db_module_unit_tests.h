#ifndef TST_DB_MODULE_UNIT_TESTS_H
#define TST_DB_MODULE_UNIT_TESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include "DB_module.h"

using namespace testing;

class connection_pool_testing : public testing::Test
{
protected:
    connection_pool c_p_zero{0};
    static connection_pool c_p_dft_conninfo;
    PGconn* conn{nullptr};
    void pull_conns(int amount, connection_pool& c_p = c_p_dft_conninfo)
    {
        for (int i = 0; i != amount; ++i)
        {
            EXPECT_FALSE(static_cast<bool>(conn));
            EXPECT_TRUE(c_p.pull_connection(conn));
            EXPECT_TRUE(static_cast<bool>(conn));
            EXPECT_EQ(CONNECTION_OK, PQstatus(conn));
            PQfinish(conn);
            conn = nullptr;
        }
        EXPECT_FALSE(c_p.pull_connection(conn));
    }

    void n_e(int amount, connection_pool& c_p = c_p_dft_conninfo)
    {
        std::deque<PGconn*> temp(amount);
        for (int i = 0; c_p.pull_connection(temp[i]); ++i) { }
        while (!temp.empty())
        {
            PGconn* conn = temp.front();
            EXPECT_THAT(temp, Contains(conn));
            c_p.push_connection(conn);
            temp.pop_front();
        }
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
    const char* k[9] {"host", "hostaddr", "port", "dbname", "password", "connect_timeout", "client_encoding", "sslmode", nullptr};
    const char* v[9] {"localhost", "127.0.0.1", "5432", "pet_project_db", "pet_project_password", "", "auto", "prefer", nullptr};
public:
    constexpr MockParserHelper() = default;
    const char* const* returning_massives(char m) const;
};

class function_wrapper_testing : public testing::Test
{
protected:
    static void test_function1() { std::cout << "Passed!" << std::endl; }
public:
    bool test_function2() { return true; }
};

class thread_pool_testing : public testing::Test
{
protected:
    thread_pool t_p;
    static constexpr auto lambda = [] ()->bool { return true; };
public:
    class test_functor {
    public:
        void operator()() { std::cout << "Passed!" << std::endl; }
    };
    int test_function() { return 42; }
};

class PG_result_testing : public testing::Test
{
protected:
    PGconn* conn{nullptr};
    PGresult* res{nullptr};
    PG_result pg_res;
    void SetUp() override
    {
        conn = PQconnectdb("dbname = pet_project_db");
        ASSERT_EQ(CONNECTION_OK, PQstatus(conn));
        res = PQexec(conn, "SELECT * FROM song_table");
        ASSERT_EQ(PGRES_TUPLES_OK, PQresultStatus(res));
    }

    void TearDown() override
    {
       PQfinish(conn);
    }
};

class DB_module_testing : public testing::Test
{
protected:
    MockParserHelper mock_parser_helper;
    std::shared_ptr<Parser> mock_parser_shared_ptr{nullptr};
    void SetUp() override
    {
        mock_parser_shared_ptr = std::make_shared<MockParser>();
        ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr));
    }
    void check(const DB_module& db_module)
    {
        std::pair<int, int> conns_threads{db_module.conns_threads_amount()};
        EXPECT_LT(0, conns_threads.first);
        EXPECT_LT(0, conns_threads.second);
    }
};

#endif // TST_DB_MODULE_UNIT_TESTS_H
