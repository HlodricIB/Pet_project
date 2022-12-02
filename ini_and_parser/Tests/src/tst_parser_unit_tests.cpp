//#include <gtest/gtest.h>


/*TEST_F(Parser_DB_testing, Default_ctor)
{
    Parser_DB default_constructed;
    EXPECT_EQ(nullptr, default_constructed.parsed_info_ptr());
    EXPECT_EQ(nullptr, default_constructed.parsed_info_ptr('k'));
    EXPECT_EQ(nullptr, default_constructed.parsed_info_ptr('v'));
    EXPECT_EQ(nullptr, default_constructed.parsed_info_ptr('K'));
    EXPECT_EQ(nullptr, default_constructed.parsed_info_ptr('V'));
}

TEST_F(Parser_DB_testing, C_string_ctor)
{
    p_DB_checking(parser_constructed);
}

TEST_F(Parser_DB_testing, Std_string_ctor)
{
    std::string path("/home/nikita/C++/Pet_project/ini_and_parser/pet_project_config.ini");
    Parser_DB parser_std_string_constructed(path);
    p_DB_checking(parser_std_string_constructed);
}

TEST_F(Parser_DB_testing, Property_tree_ctor)
{
    boost::property_tree::ptree config;
    try {
        boost::property_tree::ini_parser::read_ini("/home/nikita/C++/Pet_project/ini_and_parser/pet_project_config.ini", config);
    }  catch (const prop_tree::ini_parser_error& error) {
        std::cerr << error.what() << std::endl;
    }
    Parser_DB parser_property_tree_constructed(config);
    p_DB_checking(parser_property_tree_constructed);
}

TEST_F(Parser_DB_testing, Copy_ctor)
{
    Parser_DB copy_constructed{parser_constructed};
    p_DB_checking(copy_constructed);
}

TEST_F(Parser_DB_testing, Assignment_operator)
{
    Parser_DB to_assign;
    to_assign = parser_constructed;
    p_DB_checking(to_assign);
}

TEST_F(Parser_DB_testing, Parsed_info_ptr)
{
    EXPECT_EQ(nullptr, parser_constructed.parsed_info_ptr('?'));
}*/

