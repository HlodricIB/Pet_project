#include "tst_db_module_unit_tests.h"

MockParser::MockParser()
{
    ON_CALL(*this, parsed_info_ptr(_))
            .WillByDefault(Return(nullptr));
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

//*******************************************connection_pool testing*******************************************

connection_pool connection_pool_testing::c_p_dft_conninfo{4};

TEST_F(connection_pool_testing, Parser_ctor)
{
    MockParserHelper mock_parser_helper;
    std::shared_ptr<Parser> mock_parser_shared_ptr = std::make_shared<MockParser>();
    EXPECT_CALL(*(std::dynamic_pointer_cast<MockParser>(mock_parser_shared_ptr)), parsed_info_ptr(_))
            .Times(AtLeast(6))
            .WillRepeatedly(Invoke(&mock_parser_helper, &MockParserHelper::returning_massives));
    ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr->parsed_info_ptr('k')));
    ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr->parsed_info_ptr('v')));
    connection_pool c_p_parser(4, mock_parser_shared_ptr);
    EXPECT_EQ(4, c_p_parser.conns_amount());
    pull_conns(c_p_parser.conns_amount(), c_p_parser);
}

TEST_F(connection_pool_testing, Conns_amount)
{
    EXPECT_EQ(0, c_p_zero.conns_amount());

    EXPECT_EQ(4, c_p_dft_conninfo.conns_amount());
}

TEST_F(connection_pool_testing, Conninfo_ctor_defaulted_args)
{
    connection_pool c_p;
    EXPECT_EQ(1, c_p.conns_amount());
    pull_conns(c_p.conns_amount(), c_p);

    //Conninfo ctor with one specified argument, which is number of connections (equal to 0)
    EXPECT_FALSE(static_cast<bool>(c_p_zero.conns_amount()));
    EXPECT_FALSE(c_p_zero.pull_connection(conn));

    ASSERT_TRUE(static_cast<bool>(c_p_dft_conninfo.conns_amount()));
    EXPECT_EQ(4, c_p_dft_conninfo.conns_amount());
    pull_conns(c_p_dft_conninfo.conns_amount());
}

TEST_F(connection_pool_testing, Conninfo_ctor)
{
    {
        connection_pool c_p_conninfo{4, "dbname = pet_project_db"};
        ASSERT_TRUE(static_cast<bool>(c_p_conninfo.conns_amount()));
        EXPECT_EQ(4, c_p_conninfo.conns_amount());
        pull_conns(c_p_conninfo.conns_amount(), c_p_conninfo);
    }
    {
        connection_pool c_p_conninfo{2, "dbname = pet_project_db"};
        ASSERT_TRUE(static_cast<bool>(c_p_conninfo.conns_amount()));
        EXPECT_EQ(2, c_p_conninfo.conns_amount());
        pull_conns(c_p_conninfo.conns_amount(), c_p_conninfo);
    }
}

TEST_F(connection_pool_testing, Pull_connection)
{
    connection_pool c_p_conninfo{9};
    pull_conns(c_p_conninfo.conns_amount(), c_p_conninfo);
}

TEST_F(connection_pool_testing, Push_connection)
{
    PGconn* conn = PQconnectdb("dbname = pet_project_db");
    c_p_zero.push_connection(conn);
    EXPECT_TRUE(c_p_zero.pull_connection(conn));
}

//*******************************************function_wrapper testing*******************************************
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

TEST_F(function_wrapper_testing, Move_assign_operator)
{
    function_wrapper f_w(test_function1);
    f_w_default = std::move(f_w);
    f_w_default();
}

TEST_F(function_wrapper_testing, Return_value)
{
    std::deque<function_wrapper> tasks;
    std::mutex m;
    std::packaged_task<bool()> task(std::move(test_function2));
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

