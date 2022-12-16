#include "tst_db_module_unit_tests.h"

namespace db_module_unit_tests
{
const char* tests_conninfo = "dbname = pet_project_db";
const char* tests_DB_name = "pet_project_db";

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

TEST_F(ConnectionPoolWarmingTesting, Warm)
{
    double not_warmed_duration{0}, warmed_duration{0};
    warm_test(false, not_warmed_duration);
    warm_test(true, warmed_duration);
    //EXPECT_GT(not_warmed_duration, warmed_duration);
    EXPECT_GT(not_warmed_duration, warmed_duration) << not_warmed_duration << "<" << warmed_duration;
    std::cerr << not_warmed_duration << ">" << warmed_duration << std::endl;
    //And now it's better to do warmed at first and unwarmed afterwards;
}

//*******************************************function_wrapper testing*******************************************
/*TEST_F(function_wrapper_testing, Default_ctor_and_move_assign_operator)
{
    function_wrapper f_w_default;
    function_wrapper f_w(test_function1);
    f_w_default = std::move(f_w);
    f_w_default();
}

TEST_F(function_wrapper_testing, Ctor_and_call_operator)
{
    function_wrapper f_w{test_function1};
    f_w();
}

TEST_F(function_wrapper_testing, Move_ctor)
{
    function_wrapper f_w(test_function1);
    function_wrapper f_w_move_ctor(std::move(f_w));
    f_w_move_ctor();
}

TEST_F(function_wrapper_testing, Return_value)
{
    std::deque<function_wrapper> tasks;
    std::mutex m;
    std::packaged_task<bool()> task((std::bind(&function_wrapper_testing::test_function2, this)));
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
TEST_F(thread_pool_testing, Default_ctor)
{
    ASSERT_NE(0, t_p.threads_amount());
}

TEST_F(thread_pool_testing, Size_t_ctor_and_threads_amount)
{
    thread_pool t_p_zero(0);
    ASSERT_EQ(0, t_p_zero.threads_amount());

    thread_pool t_p_two(2);
    ASSERT_EQ(2, t_p_two.threads_amount());

    thread_pool t_p_four(4);
    ASSERT_EQ(4, t_p_four.threads_amount());
}

TEST_F(thread_pool_testing, Push_task)
{
    thread_pool tp;
    test_functor t_f{};
    std::packaged_task<bool()> task_bool(lambda);
    std::future<bool> res_bool = task_bool.get_future();
    std::packaged_task<void()> task_functor(t_f);
    std::packaged_task<int()> task_int(std::bind(&thread_pool_testing::test_function, this));
    std::future<int> res_int = task_int.get_future();
    tp.push_task(std::move(task_bool));
    tp.push_task(std::move(task_functor));
    tp.push_task(std::move(task_int));
    EXPECT_TRUE(res_bool.get());
    EXPECT_EQ(42, res_int.get());
}

TEST_F(PG_result_testing, Default_ctor_and_get_result)
{
    EXPECT_FALSE(static_cast<bool>(pg_res.get_result()));
}

TEST_F(PG_result_testing, PGresult_pointer_ctor)
{
    PG_result p_r{res};
    EXPECT_EQ(res, p_r.get_result());
}

TEST_F(PG_result_testing, Move_ctor)
{
    PG_result p_r{res};
    PG_result p_r_moved(std::move(p_r));
    EXPECT_FALSE(static_cast<bool>(p_r.get_result()));
    EXPECT_TRUE(static_cast<bool>(p_r_moved.get_result()));
}

TEST_F(PG_result_testing, Move_assign_operator)
{
    PG_result p_r{res};
    pg_res = std::move(p_r);
    EXPECT_FALSE(static_cast<bool>(p_r.get_result()));
    EXPECT_TRUE(static_cast<bool>(pg_res.get_result()));
}

TEST_F(PG_result_testing, Display_exec_result)
{
    //NOT TESTED YET DUE TO HUGE POSSIBILITY OF CHANGING!
}

//*******************************************DB_module testing*******************************************
TEST_F(DB_module_testing, Parser_ctor)
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



