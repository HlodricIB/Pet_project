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

//Helper class to determine absolute path to "Pet_project/the_directory_specified_in_constructor" dir
class Pet_project_path
{
private:
    std::string path;
public:
    Pet_project_path(std::string);  //The argument is a relative (with respect to the absolute path to Pet_project) path required
    std::string return_full_path() const { return path; }
};

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

//Function for checking status that std::system returns
void check_status(int);

//For cache clear we need to stop postgresql service, so, just in case, it is better to implement another fixture class for wram function
//testing purposes, because in the ConnectionPoolTesting class we create two connection pools which keeps opened connections to
//postgresql database
class ConnectionPoolWarmingTesting : public testing::Test
{
protected:
    double warm_test(bool if_warm)
    {
        //Clearing cache
        std::cout.flush();
        auto status = std::system("service postgresql stop");
        db_module_unit_tests::check_status(status);
        status = std::system("sync && gnome-terminal --wait -- sh -c \"sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches\'\"");
        db_module_unit_tests::check_status(status);
        status = std::system("service postgresql start");
        db_module_unit_tests::check_status(status);

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
        return std::chrono::duration<double, std::micro>(stop - start).count();
    }
};

class FunctionWrapperTesting : public testing::Test
{
    //To have access to protected function test_function2 we have to declare appropriate test as a friend
    FRIEND_TEST(FunctionWrapperTesting, ReturnValue);
private:
    bool passed{false};
protected:
    function_wrapper f_w_default;
    function_wrapper f_w{std::bind(&FunctionWrapperTesting::test_function1, this)};
    void test_function1() { passed = true; }
    bool test_function2() const { return true; }
    bool get_passed() const { return passed; }
};

class ThreadPoolTesting : public testing::Test
{
    //To have access to protected function test_function_middle we have to declare appropriate test as a friend
    FRIEND_TEST(ThreadPoolTesting, PushTaskFrontPushTaskBack);
protected:
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    struct test_functor_back
    {
        time_point operator()() { return std::chrono::steady_clock::now(); }
    };
    time_point test_function_middle() { return std::chrono::steady_clock::now(); }
    bool pred(time_point t_front, time_point t_middle, time_point t_back) { return t_front < t_middle && t_middle  < t_back; }
};

class PGResultTesting : public testing::Test
{
protected:
    PGconn* conn{nullptr};

    Pet_project_path path{"scripts_for_tests_DB"};
    PG_result pg_res_default;
    void SetUp() override
    {
        ASSERT_NE(path.return_full_path(), std::string{});
        //gnome-terminal --wait -- sh -c 'cd /home/nikita/C++/Pet_project/scripts_for_tests_DB; sh create_tests_db.sh'
        std::string run_script = "gnome-terminal --wait -- sh -c 'cd " + path.return_full_path() + "; sh create_tests_db.sh'";
        auto status = std::system(run_script.c_str());
        db_module_unit_tests::check_status(status);
        conn = PQconnectdb(tests_conninfo);
        ASSERT_EQ(PQstatus(conn), CONNECTION_OK);
    }

    void TearDown() override
    {
       PQfinish(conn);
    }
};

class PGResultTestingQueried : public PGResultTesting
{
protected:
    PGresult* res_song{nullptr};
    PGresult* res_log{nullptr};
    std::shared_ptr<PG_result> pg_res_song{nullptr};
    std::shared_ptr<PG_result> pg_res_log{nullptr};
    void moving_check(PG_result&, std::shared_ptr<PG_result>, PGresult*);
    void SetUp() override
    {
        PGResultTesting::SetUp();
        res_song = PQexec(conn, "SELECT * FROM song_table");
        ASSERT_EQ(PQresultStatus(res_song), PGRES_TUPLES_OK);
        res_log = PQexec(conn, "SELECT * FROM log_table");
        ASSERT_EQ(PQresultStatus(res_log), PGRES_TUPLES_OK);
        pg_res_song = std::make_shared<PG_result>(res_song, tests_DB_name);
        pg_res_log = std::make_shared<PG_result>(res_log, tests_DB_name);
    }
};

class PGResultTestingResultContainer : public PGResultTestingQueried
{
protected:
    const std::vector<std::vector<const char*>> song_table{
        { "id", "song_name", "song_uid", "song_url" },
        { "1", "song1.wav", "11111", "/some_dir/1" },
        { "2", "song2.mp3", "11112", "/some_dir/2" },
        { "3", "song3.aac", "11113", "/some_dir/3" },
        { "4", "song4", "11114", "/some_dir/4" }
    };
    const std::vector<std::vector<const char*>> log_table{
        { "id", "requested_host", "port", "ip", "user_agent", "rest_method", "target", "req_date_time" },
        { "1", "127.0.0.41:8080", "11111", "127.0.0.1", "tests_client_1", "METHOD_1", "target_1", "2022-12-21 10:15:20" },
        { "2", "127.0.0.42:8080", "11112", "127.0.0.2", "tests_client_2", "METHOD_2", "target_2", "2022-12-21 12:15:20" },
        { "3", "127.0.0.43:8080", "11113", "127.0.0.3", "tests_client_3", "METHOD_3", "target_3", "2022-12-21 13:15:20" },
        { "4", "127.0.0.44:8080", "11114", "127.0.0.4", "tests_client_4", "METHOD_4", "target_4", "2022-12-23 14:15:20" }
    };
    void result_container_check(const std::vector<std::vector<const char*>>&, const ::db_module::PG_result::result_container&);
};

/*class DB_module_testing : public testing::Test
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
