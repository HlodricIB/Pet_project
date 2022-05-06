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

TEST_F(connection_pool_testing, Default_conninfo_ctor)
{
    ASSERT_TRUE(static_cast<bool>(c_p_dft_conninfo.conns_amount()));
    EXPECT_EQ(4, c_p_dft_conninfo.conns_amount());
}

TEST_F(connection_pool_testing, Conninfo_ctor_and_conns_amount)
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
}

TEST_F(connection_pool_testing, Pull_connection)
{
    pull_conns(c_p_dft_conninfo.conns_amount());
}

TEST_F(connection_pool_testing, Push_connection)
{
    for (int i = 0; i != c_p_dft_conninfo.conns_amount(); ++i)
    {
        c_p_dft_conninfo.push_connection(conn);
    }
    pull_conns(c_p_dft_conninfo.conns_amount());
}
