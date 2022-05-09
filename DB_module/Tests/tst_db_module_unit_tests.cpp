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

TEST(connection_pool_testing, Parser_ctor)
{
    MockParserHelper mock_parser_helper;
    std::shared_ptr<Parser> mock_parser_shared_ptr = std::make_shared<MockParser>();
    EXPECT_CALL(*(std::dynamic_pointer_cast<MockParser>(mock_parser_shared_ptr)), parsed_info_ptr(_))
            .Times(AtLeast(1))
            .WillRepeatedly(Invoke(&mock_parser_helper, &MockParserHelper::returning_massives));
    ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr->parsed_info_ptr('k')));
    ASSERT_TRUE(static_cast<bool>(mock_parser_shared_ptr->parsed_info_ptr('v')));

    //PGconn* conn{0};
    //PGconn* conn = PQconnectdbParams(mock_parser_shared_ptr->parsed_info_ptr('k'), mock_parser_shared_ptr->parsed_info_ptr('v'), 0);
    PGconn* conn = PQconnectdbParams(mock_parser_helper.returning_massives('k'), mock_parser_helper.returning_massives('v'), 0);
    EXPECT_EQ(CONNECTION_OK, PQstatus(conn));

    //connection_pool c_p_parser(1, mock_parser_shared_ptr);
    //EXPECT_EQ(4, c_p_parser.conns_amount());
    //pull_conns(c_p_parser.conns_amount());
}

/*TEST_F(connection_pool_testing, Conns_amount)
{
    EXPECT_EQ(0, c_p_zero.conns_amount());

    EXPECT_EQ(4, c_p_dft_conninfo.conns_amount());
}

TEST_F(connection_pool_testing, Default_conninfo_ctor)
{
    connection_pool c_p;
    EXPECT_EQ(1, c_p.conns_amount());

    EXPECT_FALSE(static_cast<bool>(c_p_zero.conns_amount()));
    EXPECT_FALSE(c_p_zero.pull_connection(conn));

    ASSERT_TRUE(static_cast<bool>(c_p_dft_conninfo.conns_amount()));
    EXPECT_EQ(4, c_p_dft_conninfo.conns_amount());
}*/

/*TEST(connection_pool_testing, Conninfo_ctor)
{
    {
        connection_pool c_p_conninfo{4, "dbname = pet_project_db"};
        ASSERT_TRUE(static_cast<bool>(c_p_conninfo.conns_amount()));
        EXPECT_EQ(4, c_p_conninfo.conns_amount());
    }
    {
        connection_pool c_p_conninfo{2, "dbname = pet_project_db"};
        ASSERT_TRUE(static_cast<bool>(c_p_conninfo.conns_amount()));
        EXPECT_EQ(2, c_p_conninfo.conns_amount());
    }
}*/

/*TEST_F(connection_pool_testing, Pull_connection)
{
    pull_conns(c_p_dft_conninfo.conns_amount());
}

TEST_F(connection_pool_testing, Push_connection)
{
    c_p_zero.push_connection(conn);
    EXPECT_TRUE(c_p_zero.pull_connection(conn));
}*/
