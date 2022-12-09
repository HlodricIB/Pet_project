#ifndef TST_PARSER_UNIT_TESTS_H
#define TST_PARSER_UNIT_TESTS_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "parser.h"

namespace parser_unit_tests
{
extern const char* valid_path_to_ini_file;
extern const char* wrong_path_to_ini_file;
extern const char* path_to_ini_file_less;
extern const char* path_to_ini_file_wrong_key_name;

using namespace ::parser;

//*******************************************Config_searching*******************************************

class ConfigSearchingTesting : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_NO_THROW(c_s_c_string = Config_searching{"pet_project_config.ini"});
        ASSERT_NO_THROW(c_s_std_string = Config_searching{std::string("pet_project_config.ini")});
    }
    Config_searching c_s_default;
    Config_searching c_s_c_string;
    Config_searching c_s_std_string;
};

//*******************************************Parser*******************************************

template<class T>
std::shared_ptr<Parser> create_parser(const char* path_c_string = nullptr, std::string path_std_string = std::string{})
{
    std::shared_ptr<Parser> parser{nullptr};
    if (path_c_string != nullptr && path_std_string.empty())
    {
        return std::make_shared<T>(path_c_string);
    }
    if (path_c_string == nullptr && !path_std_string.empty())
    {
        return std::make_shared<T>(path_std_string);
    }
    return std::make_shared<T>();
}

//An order of char* returned by Parser_DB
enum p_DB : size_t
{
    HOST,
    HOSTADDR,
    PORT_DB,
    DBNAME,
    PASSWORD,
    CONNECT_TIMEOUT,
    CLIENT_ENCODING,
    SSLMODE
};

//An order of char* returned by Parser_Inotify
enum p_Inotify : size_t
{
    FILES_FOLDER_Inotify,
    LOGS_FOLDER_Inotify,
    MAX_LOG_FILE_SIZE_Inotify
};

//An order of char* returned by Parser_Server_HTTP
enum p_Server_HTTP : size_t
{
    ADDRESS,
    PORT_Server_HTTP,
    SERVER_NAME,
    FILES_FOLDER_Server_HTTP,
    LOGS_FOLDER_Server_HTTP,
    MAX_LOG_FILE_SIZE_Server_HTTP,
    NUM_THREADS_Server_HTTP,
    DAYS_LIMIT,
    ROWS_LIMIT
};

//An order of char* returned by Parser_Client_HTTP
enum p_Client_HTTP : size_t
{
    HOST_ADDRESS,
    PORT_SERVICE,
    CLIENT_NAME,
    VERSION,
    NUM_THREADS_Client_HTTP,
    DOWNLOAD_DIR,
    TARGETS
};

template<class T>
void values_checking(const char* const*);

template<class T>
void keys_checking(const char* const* ptr_k)
{
    values_checking<T>(ptr_k);
}

template<>
void keys_checking<Parser_DB>(const char* const* ptr_k)
{
    EXPECT_STREQ(ptr_k[HOST], "host");
    EXPECT_STREQ(ptr_k[HOSTADDR], "hostaddr");
    EXPECT_STREQ(ptr_k[PORT_DB], "port");
    EXPECT_STREQ(ptr_k[DBNAME], "dbname");
    EXPECT_STREQ(ptr_k[PASSWORD],"password");
    EXPECT_STREQ(ptr_k[CONNECT_TIMEOUT], "connect_timeout");
    EXPECT_STREQ(ptr_k[CLIENT_ENCODING], "client_encoding");
    EXPECT_STREQ(ptr_k[SSLMODE], "sslmode");
}

template<>
void values_checking<Parser_DB>(const char* const* ptr_v)
{
    EXPECT_STREQ(ptr_v[HOST], "localhost");
    EXPECT_STREQ(ptr_v[HOSTADDR], "127.0.0.1");
    EXPECT_STREQ(ptr_v[PORT_DB], "5432");
    EXPECT_STREQ(ptr_v[DBNAME], "pet_project_db");
    EXPECT_STREQ(ptr_v[PASSWORD], "pet_project_password");
    EXPECT_STREQ(ptr_v[CONNECT_TIMEOUT], "");
    EXPECT_STREQ(ptr_v[CLIENT_ENCODING], "auto");
    EXPECT_STREQ(ptr_v[SSLMODE], "prefer");
}

template<>
void values_checking<Parser_Inotify>(const char* const* ptr_v)
{
    EXPECT_STREQ(ptr_v[FILES_FOLDER_Inotify], "/home/nikita/C++/Pet_project/songs_folder");
    EXPECT_STREQ(ptr_v[LOGS_FOLDER_Inotify], "/home/nikita/C++/Pet_project/logs_folder/inotify_logs");
    EXPECT_STREQ(ptr_v[MAX_LOG_FILE_SIZE_Inotify], "1500");
}

template<>
void values_checking<Parser_Server_HTTP>(const char* const* ptr_v)
{
    EXPECT_STREQ(ptr_v[ADDRESS], "0.0.0.0");
    EXPECT_STREQ(ptr_v[PORT_Server_HTTP], "8080");
    EXPECT_STREQ(ptr_v[SERVER_NAME], "pet_project_server");
    EXPECT_STREQ(ptr_v[FILES_FOLDER_Server_HTTP], "/home/nikita/C++/Pet_project/songs_folder");
    EXPECT_STREQ(ptr_v[LOGS_FOLDER_Server_HTTP], "/home/nikita/C++/Pet_project/logs_folder/server_http_logs");
    EXPECT_STREQ(ptr_v[MAX_LOG_FILE_SIZE_Server_HTTP], "1500");
    EXPECT_STREQ(ptr_v[NUM_THREADS_Server_HTTP], "2");
    EXPECT_STREQ(ptr_v[DAYS_LIMIT], "1");
    EXPECT_STREQ(ptr_v[ROWS_LIMIT], "10");
}

template<>
void values_checking<Parser_Client_HTTP>(const char* const* ptr_v)
{
    EXPECT_STREQ(ptr_v[HOST_ADDRESS], "127.0.0.41, 127.0.0.42, 127.0.0.43, 127.0.0.44, 127.0.0.45, 127.0.0.46, 127.0.0.47, 127.0.0.48, "
                                                                                                                           "127.0.0.49");
    EXPECT_STREQ(ptr_v[PORT_SERVICE], "8080, 8080, 8080, 8080, 8080, 8080, 8080, 8080, 8080");
    EXPECT_STREQ(ptr_v[CLIENT_NAME], "pet_project_client");
    EXPECT_STREQ(ptr_v[VERSION], "");
    EXPECT_STREQ(ptr_v[NUM_THREADS_Client_HTTP], "2");
    EXPECT_STREQ(ptr_v[DOWNLOAD_DIR], "/home/nikita/C++/Pet_project/Download/1, /home/nikita/C++/Pet_project/Download/2");
    EXPECT_STREQ(ptr_v[TARGETS], "files_table, log_table, 11, 11.wav, 14.wav, 15, 25.mp3, 35.wav, 3.mp3, 53.mp3, 63.mp3, added2.mp3, song1.mp3, "
                                                                                                        "song2.wav");
}

template<class T>
class ParserTesting : public ::testing::Test
{
protected:
    std::shared_ptr<Parser> parser{nullptr};
    ParserTesting(const char* path_c_string = nullptr, std::string path_std_string = std::string{}):
                                                                        parser(create_parser<T>(path_c_string, path_std_string)) { }

    ~ParserTesting() override { }
    void default_initialized()
    {
        EXPECT_EQ(parser->parsed_info_ptr(), nullptr);
        EXPECT_EQ(parser->parsed_info_ptr('k'), nullptr);
        EXPECT_EQ(parser->parsed_info_ptr('v'), nullptr);
        EXPECT_EQ(parser->parsed_info_ptr('K'), nullptr);
        EXPECT_EQ(parser->parsed_info_ptr('V'), nullptr);
    }
    void keys_values_checking()
    {
        //Keys checking
        const char* const* ptr = parser->parsed_info_ptr();
        keys_checking<T>(ptr);
        const char* const* ptr_k = parser->parsed_info_ptr('k');
        keys_checking<T>(ptr_k);
        const char* const* ptr_K = parser->parsed_info_ptr('K');
        keys_checking<T>(ptr_K);
        //Values checking
        const char* const* ptr_v = parser->parsed_info_ptr('v');
        values_checking<T>(ptr_v);
        const char* const* ptr_V = parser->parsed_info_ptr('v');
        values_checking<T>(ptr_V);
    }
};

template<class T>
class ParserDefaultCtor : public ParserTesting<T>
{
protected:
    ParserDefaultCtor(): ParserTesting<T>() { }
};

template<class T>
class ParserCStringCtor : public ParserTesting<T>
{
protected:
    ParserCStringCtor(): ParserTesting<T>(valid_path_to_ini_file) { }
};

template<class T>
class ParserStdStringCtor : public ParserTesting<T>
{
protected:
    ParserStdStringCtor(): ParserTesting<T>(nullptr, std::string{valid_path_to_ini_file}) { }
};

template<class T>
class ParserWrongIniFileCtor : public ::testing::Test
{
protected:
    void SetUp() override
    {
        EXPECT_THROW(T{wrong_path_to_ini_file}, prop_tree::ptree_error);
    }
};

//To test const prop_tree::ptree& ctors we need one more function to get prop_tree::ptree
const prop_tree::ptree parse_ini(const char* path_to_ini_file)
{
    boost::property_tree::ptree config;
    try {
        boost::property_tree::ini_parser::read_ini(path_to_ini_file, config);
    }  catch (const prop_tree::ini_parser_error& error) {
        std::cerr << error.what() << std::endl;
    }
    return config;
}

template<class T>
class ParserPropTreeCtor : public ParserTesting<T>
{
private:
    prop_tree::ptree config;
protected:
    void SetUp() override
    {
        //Testing prop_tree::ptree with no information
        std::shared_ptr<Parser> parser = std::make_shared<T>(config);
        this->default_initialized();
        //Filling config with appropriate information
        ASSERT_NO_THROW(config = parse_ini(valid_path_to_ini_file));
        parser = std::make_shared<T>(config);
        ParserTesting<T>::parser = parser;
    }
};

template<class T>
class ParserConfigSearchingCtor : public ParserTesting<T>
{
private:
    Config_searching c_s;
protected:
    void SetUp() override
    {
        fs::current_path("/home/nikita/C++/Pet_project/ini_and_parser/Tests/");
        ASSERT_NO_THROW(c_s = Config_searching{"pet_project_config.ini"});
        std::shared_ptr<Parser> parser = std::make_shared<T>(c_s);
        ParserTesting<T>::parser = parser;
    }
};

template<class T>
class ParserCopyCtor : public ::testing::Test
{
protected:
    void SetUp() override
    {
        T origin{valid_path_to_ini_file};
        T copied(origin);
        const char* const* ptr_k = copied.parsed_info_ptr('k');
        keys_checking<T>(ptr_k);
        const char* const* ptr_v = copied.parsed_info_ptr('v');
        values_checking<T>(ptr_v);
    }
};

template<class T>
class ParserValidateLess : public ParserTesting<T>
{
protected:
    ParserValidateLess(): ParserTesting<T>(path_to_ini_file_less) { }
};

template<class T>
class ParserValidateWrongKeyName : public ParserTesting<T>
{
protected:
    ParserValidateWrongKeyName(): ParserTesting<T>(path_to_ini_file_wrong_key_name) { }
};
}   //namespace parser_unit_tests

#endif // TST_PARSER_UNIT_TESTS_H
