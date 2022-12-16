#ifndef TST_DB_MODULE_UNIT_TESTS_H
#define TST_DB_MODULE_UNIT_TESTS_H

#include <cstdlib>
#include <chrono>
#include <sys/wait.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include "DB_module.h"

namespace db_module_unit_tests
{
extern const char* tests_conninfo;
extern const char* tests_DB_name;

using namespace testing;
using namespace parser;
using namespace db_module;

class MockParserHelper
{
private:
    const char* k[9] {"host", "hostaddr", "port", "dbname", "password", "connect_timeout", "client_encoding", "sslmode", nullptr};
    const char* v[9] {"localhost", "127.0.0.1", "5432", "pet_project_db", "pet_project_password", "", "auto", "prefer", nullptr};
    const char* k_not_valid[2] {"dbname", nullptr};
    const char* v_not_valid[2] {"no_db", nullptr};
public:
    constexpr MockParserHelper() = default;
    const char* const* returning_massives(char m) const;
    const char* const* returning_massives_no_conns(char m) const;
};

class MockParser : public Parser
{
public:
    MockParser();
    MOCK_METHOD(const char* const*, parsed_info_ptr, (char m), (const, override));
    MOCK_METHOD((std::pair<bool, std::string_view>), validate_parsed, (), (override));
};

class ConnectionPoolTesting : public testing::Test
{
protected:
    connection_pool default_constructed{};
    connection_pool zero_connections{0};
    PGconn* conn{nullptr};
    void check_conns(int amount, connection_pool& c_p)
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

    void pull_push_connections_check(int amount, connection_pool& c_p)
    {
        //Since container, that contains the PGconns (conns_deque) is private we need to do at least two cycles of pull_connection() and
        //at least one cycle of push_connection() calls to test this methods
        std::deque<PGconn*> temp1(amount);
        std::deque<PGconn*> temp2(amount);
        for (int i = 0; c_p.pull_connection(temp1[i]); ++i);
        for (int i = 0; i != temp1.size(); ++i)
        {
            c_p.push_connection(temp1[i]);
        }
        for (int i = 0; c_p.pull_connection(temp2[i]); ++i);
        EXPECT_THAT(temp1, UnorderedElementsAreArray(temp2));
    }
};

//For cache clear we need to stop postgresql service, so, just in case, it is better to implement another fixture class for wram function
//testing purposes, because in the ConnectionPoolTesting class we create two connection pools which keep opened connections to
//postgresql database
class ConnectionPoolWarmingTesting : public testing::Test
{
private:
    void check_status(int status)
    {
        ASSERT_TRUE(WIFEXITED(status));
        ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
        ASSERT_FALSE(static_cast<bool>(WIFSIGNALED(status)));
    }
protected:
    //In order to use ASSERTION macros the return value of this function have to be void
    void warm_test(bool if_warm, double& _duration)
    {
        //Clearing cache (just in case)
        std::cout.flush();
        //auto status = std::system("sudo service postgresql stop; sync; sudo sh -c \"echo 3 > /proc/sys/vm/drop_caches\"; sudo service postgresql start");
        /*auto status = std::system("service postgresql stop");
        check_status(status);
        status = std::system("sync");
        check_status(status);
        status = std::system("gnome-terminal -- sh -c \"sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'\"; "
                                                "echo \"Enter password in newely opened terminal, then press any key\"; read -s -n 1");
        check_status(status);
        status = std::system("service postgresql start");
        check_status(status);*/
        //auto status = std::system("gnome-terminal -- /home/nikita/C++/Pet_project/DB_module/Tests/warm.sh");
        //auto status = std::system("sudo -S sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
        //status = std::system("echo 3 | sudo tee /proc/sys/vm/drop_caches");
        //ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
        //ASSERT_FALSE(static_cast<bool>(WIFSIGNALED(status)));
        /*status = std::system("sync");
        ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
        ASSERT_FALSE(static_cast<bool>(WIFSIGNALED(status)));
        status = std::system("sudo -S sh -c \"echo 3 > /proc/sys/vm/drop_caches\"");
        ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
        ASSERT_FALSE(static_cast<bool>(WIFSIGNALED(status)));
        status = std::system("sudo -S service postgresql start");*/
        //auto status = std::system("sudo -S service postgresql stop; sync; sudo sh -c \"echo 3 > /proc/sys/vm/drop_caches\"; sudo service postgresql start");
        /*FILE  *cmd;
            int    status;

            //cmd = popen("gnome-terminal -- sh -c '/usr/bin/sudo id -un 2>/dev/null; sudo ls'", "w");
            cmd = popen("sudo service postgresql stop; sudo service postgresql start", "w");
            if (!cmd) {
                fprintf(stderr, "Cannot run sudo: %s.\n", strerror(errno));
                return;
            }

            //fprintf(cmd, "Password\n");
            //fflush(cmd);
            status = pclose(cmd);*/
            //if (WIFEXITED(status)
            auto s = WEXITSTATUS(status);
            int t;
            if (s == EXIT_SUCCESS)
            {
                t = 10;
            }
        connection_pool c_p{2, tests_conninfo};
        if (if_warm)
        {
            c_p.warm(tests_DB_name, {"song_table", "log_table"});
        }
        PGconn* conn1{nullptr};
        PGconn* conn2{nullptr};
        c_p.pull_connection(conn1);
        c_p.pull_connection(conn2);
        auto start = std::chrono::high_resolution_clock::now();
        auto res1 = PQexec(conn1, "SELECT * FROM song_table; SELECT * FROM log_table");
        auto res2 = PQexec(conn2, "SELECT * FROM song_table; SELECT * FROM log_table");
        auto stop = std::chrono::high_resolution_clock::now();
        PQclear(res1);
        PQclear(res2);
        _duration = std::chrono::duration<double, std::micro>(stop - start).count();
    }
};
/*
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
};*/
}   //db_module_unit_tests

#endif // TST_DB_MODULE_UNIT_TESTS_H
