#ifndef TST_DB_MODULE_UNIT_TESTS_H
#define TST_DB_MODULE_UNIT_TESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "DB_module.h"

using namespace testing;

class connection_pool_testing : public testing::Test{
protected:
    connection_pool c_p_dft_conninfo{4};
    bool b{false};
    PGconn* conn{0};
    void pull_conns(int amount)
    {
        for (int i = 0; i != amount; ++i)
        {
            EXPECT_TRUE(c_p_dft_conninfo.pull_connection(conn));
            EXPECT_TRUE(static_cast<bool>(conn));
        }
        EXPECT_FALSE(static_cast<bool>(conn));
    }
};



#endif // TST_DB_MODULE_UNIT_TESTS_H
