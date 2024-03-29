#include <thread>
#include <filesystem>
#include "tst_db_module_unit_tests.h"

namespace db_module_unit_tests
{
const char* tests_conninfo = "dbname = pet_project_tests_db";
const char* tests_DB_name = "pet_project_tests_db";

Pet_project_path::Pet_project_path(std::string path_)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    auto curr_path = fs::current_path(ec);
    if (!ec)
    {
        fs::path actual_path;
        auto iter_find = std::find(curr_path.begin(), curr_path.end(), "Pet_project");
        if (iter_find != curr_path.end())
        {
            ++iter_find;
            auto iter = curr_path.begin();
            while (iter != iter_find)
            {
                actual_path /= *iter++;
            }
            actual_path /= path_;
            path = actual_path.string();
        }
    } else {
        std::cerr << "Current path definition error: " << ec.message() << std::endl;
    }
}

MockParser::MockParser()
{
    ON_CALL(*this, parsed_info_ptr(_))
            .WillByDefault(Return(nullptr));

    ON_CALL(*this, validate_parsed())
            .WillByDefault(Return(std::make_pair(false, "")));
}

const char* const* MockParserHelper::returning_massives(char m) const
{
    switch (m)
    {
    case 'k':
        return k;
        break;
    case 'v':
        return v;
        break;
    default:
        return nullptr;
        break;
    }
}

const char* const* MockParserHelper::returning_massives_no_conns(char m) const
{
    switch (m)
    {
    case 'k':
        return k_not_valid;
        break;
    case 'v':
        return v_not_valid;
    default:
        return nullptr;
        break;
    }
}

//*******************************************connection_pool testing*******************************************

TEST_F(ConnectionPoolTesting, DefaultCtor)
{
    ASSERT_EQ(default_constructed.conns_amount(), 1);
    check_conns(default_constructed.conns_amount(), default_constructed);
}

TEST_F(ConnectionPoolTesting, ConninfoCtorOneArgument)
{
    ASSERT_EQ(zero_connections.conns_amount(), 0);
    EXPECT_FALSE(zero_connections.pull_connection(conn));

    const int num_conns = 5;
    connection_pool c_p{num_conns};
    ASSERT_EQ(c_p.conns_amount(), num_conns);
    check_conns(num_conns, c_p);
}

TEST_F(ConnectionPoolTesting, ConninfoCtorBothArguments)
{
    for (size_t i = 2; i < 5; i+=2)
    {
        connection_pool c_p{i, tests_conninfo};
        ASSERT_EQ(c_p.conns_amount(), i);
        check_conns(i, c_p);
    }

    //Test conninfo ctor with wrong second (conninfo) argument
    connection_pool c_p{9, "dbname = wrong_db_name"};
    ASSERT_EQ(c_p.conns_amount(), 0);
    EXPECT_FALSE(c_p.pull_connection(conn));
}

TEST_F(ConnectionPoolTesting, ParserCtor)
{
    MockParserHelper mock_parser_helper;
    std::shared_ptr<Parser> mock_parser_shared_ptr = std::make_shared<MockParser>();
    EXPECT_CALL(*(std::dynamic_pointer_cast<MockParser>(mock_parser_shared_ptr)), parsed_info_ptr(_))
            .Times(AtLeast(8))
            .WillRepeatedly(Invoke(&mock_parser_helper, &MockParserHelper::returning_massives));
    ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr->parsed_info_ptr('k')));
    ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr->parsed_info_ptr('v')));
    for (size_t i = 0; i < 7; i += 3)
    {
        connection_pool c_p{i, mock_parser_shared_ptr};
        EXPECT_EQ(c_p.conns_amount(), i);
        check_conns(i, c_p);
    }

    connection_pool c_p{1, nullptr};
    EXPECT_EQ(c_p.conns_amount(), 0);
}

TEST_F(ConnectionPoolTesting, PullPushConnections)
{
    for (size_t i = 2; i < 7; i += 2)
    {
        connection_pool c_p{i, tests_conninfo};
        pull_push_connections_check(c_p.conns_amount(), c_p);
    }
}

TEST_F(ConnectionPoolTesting, ConnsAmount)
{
    EXPECT_EQ(zero_connections.conns_amount(), 0);

    EXPECT_EQ(default_constructed.conns_amount(), 1);

    connection_pool c_p{3, tests_conninfo};
    EXPECT_EQ(c_p.conns_amount(), 3);
}

void check_status(int status)
{
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
    ASSERT_FALSE(static_cast<bool>(WIFSIGNALED(status)));
}

//Due to the amount of time this test takes, it has been diasabled by adding the DISABLED_ prefix to its name.
//To include disabled tests in test execution, just invoke the test program with the --gtest_also_run_disabled_tests flag
//or remove DISABLED_ prefix
TEST_F(ConnectionPoolWarmingTesting, DISABLED_Warm)
{
    auto not_warmed_duration_1 = warm_test(false);
    auto warmed_duration_1 = warm_test(true);
    EXPECT_GT(not_warmed_duration_1, warmed_duration_1) << not_warmed_duration_1 << "<" << warmed_duration_1;
    //And now in reverse
    auto warmed_duration_2 = warm_test(true);
    auto not_warmed_duration_2 = warm_test(false);
    EXPECT_GT(not_warmed_duration_2, warmed_duration_2) << not_warmed_duration_2 << "<" << warmed_duration_2;
    std::cout << not_warmed_duration_1 << ">" << warmed_duration_1 << '\n'
              << not_warmed_duration_2 << ">" << warmed_duration_2 << std::endl;
}

//*******************************************function_wrapper testing*******************************************
TEST_F(FunctionWrapperTesting, DefaultCtor)
{
    //Test that we won't crash if no function passed to function wrapper
    EXPECT_EXIT(f_w_default(); exit(0), testing::ExitedWithCode(0), "");
}

TEST_F(FunctionWrapperTesting, FCtorAndCallOperator)
{
    f_w();
    EXPECT_TRUE(get_passed());
}

TEST_F(FunctionWrapperTesting, MoveCtorAndCallOperator)
{
    function_wrapper f_w_move_ctor(std::move(f_w));
    f_w_move_ctor();
    EXPECT_TRUE(get_passed());
}

TEST_F(FunctionWrapperTesting, AssignAndCallOperators)
{
    f_w_default = std::move(f_w);
    f_w_default();
    EXPECT_TRUE(get_passed());
}

TEST_F(FunctionWrapperTesting, ReturnValue)
{
    std::deque<function_wrapper> tasks;
    std::mutex m;
    std::packaged_task<bool()> task((std::bind(&FunctionWrapperTesting::test_function2, this)));
    std::future<bool> res(task.get_future());
    tasks.push_back(std::move(task));
    std::thread t([&tasks, &m] () {
        function_wrapper task;
        std::lock_guard<std::mutex> lk (m);
        task = std::move(tasks.front());
        tasks.pop_front();
        task();
    });
    t.join();
    EXPECT_TRUE(res.get());
}

//*******************************************thread_pool testing*******************************************
TEST(ThreadPoolNonFixtureTesting, DefaultCtor)
{
    thread_pool t_p;
    ASSERT_NE(0, t_p.threads_amount());
}

TEST(ThreadPoolNonFixtureTesting, SizeTCtorAndThreadsAmount)
{
    EXPECT_THROW(thread_pool(0), std::logic_error);

    EXPECT_THROW(thread_pool(-5), std::logic_error);

    thread_pool t_p_two(2);
    ASSERT_EQ(t_p_two.threads_amount(), 2);

    thread_pool t_p_four(4);
    ASSERT_EQ(t_p_four.threads_amount(), 4);
}

TEST_F(ThreadPoolTesting, PushTaskFrontPushTaskBack)
{
    thread_pool tp{1};
    auto lambda_sleep = [] { std::this_thread::sleep_for(std::chrono::seconds(3)); };
    test_functor_back t_f;
    auto lambda_front = [] ()->time_point { return std::chrono::steady_clock::now(); };
    std::packaged_task<void()> task_sleep(lambda_sleep);
    std::future<void> res_sleep = task_sleep.get_future();
    std::packaged_task<time_point()> task_middle(std::bind(&ThreadPoolTesting::test_function_middle, this));
    std::future<time_point> res_middle = task_middle.get_future();
    std::packaged_task<time_point()> task_back(t_f);
    std::future<time_point> res_back = task_back.get_future();
    std::packaged_task<time_point()> task_front(lambda_front);
    std::future<time_point> res_front = task_front.get_future();
    tp.push_task_front(std::move(task_sleep));
    tp.push_task_back(std::move(task_middle));
    tp.push_task_back(std::move(task_back));
    tp.push_task_front(std::move(task_front));
    auto middle = res_middle.get();
    res_sleep.get();
    auto back = res_back.get();
    auto front = res_front.get();
    using namespace std::placeholders;
    EXPECT_PRED3(std::bind(&ThreadPoolTesting::pred, this, _1, _2, _3), front, middle, back);
}

//*******************************************PG_result testing*******************************************
TEST_F(PGResultTesting, DefaultCtor)
{
    EXPECT_FALSE(static_cast<bool>(pg_res_default.get_result_ptr()));
    EXPECT_THAT(pg_res_default.get_result_container(), SizeIs(1));
    EXPECT_THAT(pg_res_default.get_result_container()[0], SizeIs(0));
    EXPECT_EQ(pg_res_default.get_result_command_tag(), std::numeric_limits<int>::max());
    EXPECT_STREQ(pg_res_default.get_result_single(), nullptr);
    EXPECT_EQ(pg_res_default.get_columns_number(), 0);
    EXPECT_EQ(pg_res_default.get_rows_number(), 0);
    EXPECT_EQ(pg_res_default.res_error(), std::string{});
    EXPECT_FALSE(pg_res_default.res_succeed());
    EXPECT_EQ(pg_res_default.res_DB_name(), std::string{});
}

TEST_F(PGResultTestingQueried, PGresultStdStringCtor)
{
    ASSERT_EQ(pg_res_song->get_result_ptr(), res_song);
    ASSERT_EQ(pg_res_log->get_result_ptr(), res_log);
    EXPECT_STREQ(pg_res_song->res_DB_name().c_str(), tests_DB_name);
    EXPECT_STREQ(pg_res_log->res_DB_name().c_str(), tests_DB_name);
}

void PGResultTestingQueried::moving_check(PG_result& moved, std::shared_ptr<PG_result> original, PGresult* _pg_result)
{
    EXPECT_EQ(moved.get_result_ptr(), _pg_result);
    EXPECT_EQ(moved.res_DB_name(), tests_DB_name);
    EXPECT_NE(moved.get_columns_number(), 0);
    EXPECT_NE(moved.get_rows_number(), 0);
    EXPECT_TRUE(moved.res_succeed());
    EXPECT_EQ(original->get_result_ptr(), nullptr);
    EXPECT_EQ(original->res_DB_name().c_str(), std::string{});
    EXPECT_EQ(original->get_columns_number(), 0);
    EXPECT_EQ(original->get_rows_number(), 0);
    EXPECT_FALSE(original->res_succeed());
}

TEST_F(PGResultTestingQueried, MoveCtor)
{
    PG_result moved_song(std::move(*pg_res_song));
    moving_check(moved_song, pg_res_song, res_song);
    PG_result moved_log(std::move(*pg_res_log));
    moving_check(moved_log, pg_res_log, res_log);
}

TEST_F(PGResultTestingQueried, MoveAssignmentOperator)
{
    pg_res_default = std::move(*pg_res_song);
    moving_check(pg_res_default, pg_res_song, res_song);
    //Self assignment checking
    *pg_res_log = std::move(*pg_res_log);
    EXPECT_EQ(pg_res_log->get_result_ptr(), res_log);
    EXPECT_EQ(pg_res_log->res_DB_name(), tests_DB_name);
    EXPECT_NE(pg_res_log->get_columns_number(), 0);
    EXPECT_NE(pg_res_log->get_rows_number(), 0);
    EXPECT_TRUE(pg_res_log->res_succeed());
}

void PGResultTestingResultContainer::result_container_check(const std::vector<std::vector<const char*>>& container_to_match,
                                                                const ::db_module::PG_result::result_container& res)
{
    ASSERT_THAT(res, SizeIs(container_to_match.size()));
    for (size_t i = 0; i != container_to_match.size(); ++i)
    {
        ASSERT_THAT(res[i], SizeIs(container_to_match[i].size()));
        for (size_t j = 0; j != container_to_match[i].size(); ++j)
        {
            EXPECT_STREQ(res[i][j].first, container_to_match[i][j]);
            EXPECT_EQ(res[i][j].second, std::strlen(container_to_match[i][j]));
        }
    }
}

TEST_F(PGResultTestingResultContainer, GetResultContainer)
{
    auto song_container = pg_res_song->get_result_container();
    result_container_check(song_table, song_container);
    auto log_container = pg_res_log->get_result_container();
    result_container_check(log_table, log_container);
}

//*******************************************DB_module testing*******************************************
/*TEST_F(DB_module_testing, Parser_ctor)
{
    EXPECT_CALL(*(std::dynamic_pointer_cast<MockParser>(mock_parser_shared_ptr)), parsed_info_ptr(_))
            .Times(AtLeast(2))
            .WillRepeatedly(Invoke(&mock_parser_helper, &MockParserHelper::returning_massives));
    DB_module db_module{mock_parser_shared_ptr};
    check(db_module);
}

TEST_F(DB_module_testing, Default_ctor)
{
    DB_module db_module;
    check(db_module);
}

TEST_F(DB_module_testing, Conninfo_ctor)
{
    DB_module db_module{"dbname = pet_project_db"};
    check(db_module);
}

TEST_F(DB_module_testing, Shared_pointers_ctor_and_conns_threads_amount)
{
    EXPECT_CALL(*(std::dynamic_pointer_cast<MockParser>(mock_parser_shared_ptr)), parsed_info_ptr(_))
            .Times(AtLeast(2))
            .WillRepeatedly(Invoke(&mock_parser_helper, &MockParserHelper::returning_massives));
    auto c_p_shared_ptr = std::make_shared<connection_pool>(3, mock_parser_shared_ptr);
    auto t_p_shared_ptr = std::make_shared<thread_pool>(5);
    DB_module db_module{c_p_shared_ptr, t_p_shared_ptr};
    std::pair<int, int> conns_threads{db_module.conns_threads_amount()};
    EXPECT_EQ(3, conns_threads.first);
    EXPECT_EQ(5, conns_threads.second);
}

TEST_F(DB_module_testing, Exec_command)
{
    DB_module db_module;
    future_result res = db_module.exec_command("SELECT * FROM song_table");
    shared_PG_result pg_result = res.get();
    ASSERT_TRUE(static_cast<bool>(pg_result->get_result()));
    EXPECT_EQ(PGRES_TUPLES_OK, PQresultStatus(pg_result->get_result()));

    res = db_module.exec_command("SELECT * FROM log_table");
    pg_result = res.get();
    ASSERT_TRUE(static_cast<bool>(pg_result->get_result()));
    EXPECT_EQ(PGRES_TUPLES_OK, PQresultStatus(pg_result->get_result()));
}*/
}   //namespace db_module_unit_tests



