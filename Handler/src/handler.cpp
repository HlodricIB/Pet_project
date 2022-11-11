#include <functional>
#include <algorithm>
//#include <boost/beast/core/string_type.hpp>
#include "handler.h"

bool Inotify_DB_handler::handle(std::vector<std::string>& arguments)
{
    std::string command;
    if (arguments[ARGS_EVENT] == "add")
    {
        size_t uid = std::hash<std::string>{}(arguments[ARGS_ENTITY_NAME]);
        command = "LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (DEFAULT, "
                + arguments[ARGS_ENTITY_NAME] + ", " + '\'' + std::to_string(uid) + '\'' + ", " + '\'' + arguments[ARGS_FILES_FOLDER] +
                '\'' + ")";
        exec_command(command);
        return true;
    }
    if (arguments[ARGS_EVENT] == "delete")
    {
        command = "LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                    "DELETE FROM song_table WHERE song_name = " + arguments[ARGS_ENTITY_NAME] + "; ALTER SEQUENCE song_seq RESTART;"
                    " UPDATE song_table SET ID = DEFAULT";
        exec_command(command);
        return true;
    }
    if (arguments[ARGS_EVENT] == "dir_del")
    {
        command = "LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY;"
                    " ALTER SEQUENCE song_seq RESTART; UPDATE song_table SET ID = DEFAULT";
        exec_command(command);
        return true;
    }
    if (arguments[ARGS_EVENT] == "refresh")
    {
        command = "LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY;"
                    " ALTER SEQUENCE song_seq RESTART";
        auto future_res = DB_ptr->exec_command(command, true);   //Clearing previous song list
        auto res = future_res.get();    //Have to block here to avoid possibility of truncating subsequent addings
        if (!res->res_succeed())
        {
            std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
            return false;
        }
        auto amount = arguments.size();
        std::string add("add");
        std::vector<std::string> temp{arguments[ARGS_FILES_FOLDER], "add", ""};
        for (std::vector<std::string>::size_type i = 2; i != amount; ++i)
        {
            temp[ARGS_ENTITY_NAME] = arguments[i];
            handle(temp);
        }
        return true;
    }
    return false;
}

void Inotify_DB_handler::exec_command(const std::string& command)
{
    auto future_res = DB_ptr->exec_command(command, true);
    auto lambda = [future_res = std::move(future_res), command] () mutable {
        auto res = future_res.get();
        if (res->res_succeed())
        {
            return;
        } else
        {
            std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
            return;
        }
    };
    t_pool_ptr->push_task_back(std::move(lambda));
    return;
}

Server_HTTP_handler::Server_HTTP_handler(std::shared_ptr<DB_module> DB_ptr_, std::shared_ptr<Logger> logger_, int days_limit_,
                                         int rows_limit_): DB_ptr(DB_ptr_), logger(logger_),
                                                            days_limit(days_limit_), rows_limit(rows_limit_)
{
    //Renumbering id columns in songs_table and log_table, for any case
    std::string command{"LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; ALTER SEQUENCE song_seq RESTART; UPDATE song_table SET ID = DEFAULT;"
        "LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE; ALTER SEQUENCE log_seq RESTART; UPDATE log_table SET ID = DEFAULT"};
    auto future_res_id_update = DB_ptr->exec_command(command, true);    //True tells, that command must be pushed at the front of
                                            //task_deque of appropriate thread pool that works with DB_module
    auto res_id_update = future_res_id_update.get();
    std::string message;    //For error messages
    if (!res_id_update->res_succeed())
    {
        message = "Couldn't update ids in tables of database: " + res_id_update->res_DB_name() + " error: " +
                                                        res_id_update->res_error() + ", executing: " + command + " query\n";
        std::cerr << message << std::flush;
        if (logger)
        {
            logger->make_record(message);
        }
    }
    //Сhecking if there are entries in the log_table that it is time to delete
    command = "LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE;"
                           "DELETE FROM log_table WHERE (DATE_TRUNC('days', (LOCALTIMESTAMP - req_date_time)) >= (INTERVAL '" +
                            std::to_string(days_limit) +
                            "' DAY)); ALTER SEQUENCE log_seq RESTART; UPDATE log_table SET ID = DEFAULT";
    //DELETE FROM log_table WHERE (DATE_TRUNC('days', (LOCALTIMESTAMP - req_date_time)) >= (INTERVAL '1' DAY));
    auto future_res = DB_ptr->exec_command(command, true);  //True tells, that command must be pushed at the front of
                                                            //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        message = "Database " + res->res_DB_name() + " error: " + res->res_error() + ", executing: " + command + " query\n";
        std::cerr << message << std::flush;
        if (logger)
        {
            logger->make_record(message);
        }
    }
    //Сounting number of rows in log_table
    command = "LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE; SELECT count(*) FROM log_table";
    auto res_rows = DB_ptr->exec_command(command, true).get();
    if (static_cast<bool>(res_rows->get_rows_number()))
    {
        log_table_nrows.store(std::atoi(res_rows->get_result_single()));
    }
}

//An order of parameters in req_info
enum : size_t
{
    REQ_TARGET,
    REQ_HOST,
    REQ_PORT,
    REQ_IP_ADDRESS,
    REQ_USER_AGENT,
    REQ_METHOD,
    REQ_RESULT,
    REQ_ERROR
};

void Server_HTTP_handler::update_log_table(std::vector<std::string>& req_info)
{
    static thread_local std::string command;
    //Checking if  number of rows in the log_table exceeds the limit
    if (auto log_tables_nrows_temp = log_table_nrows.load(); log_tables_nrows_temp > rows_limit)
    {
        static const auto rows_delete = rows_limit / 2;
        command = "LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE; "
                    "DELETE FROM log_table WHERE id <= " + std::to_string(rows_delete) + "; ALTER SEQUENCE log_seq RESTART;"
                    " UPDATE log_table SET ID = DEFAULT; SELECT last_value FROM log_seq";
        auto res = DB_ptr->exec_command(command, true).get();
        if (!res->res_succeed())
        {
            //If we can't delete rows with id number exceeding the limit we don't bother to inform client about that, but we make
                //appropriate record in log file
            std::string message{"Database " + res->res_DB_name() + " error: " + res->res_error() + ", executing: " + command + " query\n"};
            if (logger)
            {
                logger->make_record(message);
            }
        }
        int temp_nrows = std::atoi(res->get_result_single());
        log_table_nrows.compare_exchange_strong(log_tables_nrows_temp, temp_nrows);
    }
    command = "LOCK TABLE log_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO log_table (id, requested_host, port, ip, user_agent, rest_method, target, req_date_time) VALUES"
                "(DEFAULT, \'" +
                req_info[REQ_HOST] + "\', \'" +
                req_info[REQ_PORT] + "\', \'" +
                req_info[REQ_IP_ADDRESS] + "\'::inet, \'" +
                req_info[REQ_USER_AGENT] + "\', \'" +
                req_info[REQ_METHOD] + "\', \'" +
                req_info[REQ_TARGET] + "\', " +
                "LOCALTIMESTAMP" +
                ")";
    //INSERT INTO log_table (id, requested_host, port, ip, user_agent, rest_method, target, req_date_time) VALUES(DEFAULT, 'test_host', 'test_req_port', '168.0.0.1'::inet, 'test_req_user_agent', 'test_req_method', 'test_req_target', LOCALTIMESTAMP);
    auto future_res = DB_ptr->exec_command(command, false);  //Second argument (false) tells, that command must be queued in a
                                                             //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        //std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR] += std::string("Database " + res->res_DB_name() + " error while updating log table: " + res->res_error());
        return;
    }
    ++log_table_nrows;
}

void Server_HTTP_handler::forming_files_table(std::vector<std::string>& req_info)
{
    static const std::string command{"LOCK TABLE song_table IN SHARE MODE; SELECT id, song_name, song_uid FROM song_table ORDER BY id"};
    auto future_res = DB_ptr->exec_command(command, false);  //Second argument (false) tells, that command must be queued in a
                                                             //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        //std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " error while forming files table: " + res->res_error()));
        return;
    }
    if (res->get_rows_number() == 0)
    {
        req_info[REQ_RESULT] = "No files available at that moment";
        return;
    }
    result_container res_cntnr(res->get_result_container());
    forming_tables_helper(req_info, res_cntnr);
}

void Server_HTTP_handler::forming_log_table(std::vector<std::string>& req_info)
{
    static const std::string command{"LOCK TABLE log_table IN SHARE MODE; SELECT * FROM log_table ORDER BY id"};
    auto future_res = DB_ptr->exec_command(command, false);  //Second argument (false) tells, that command must be queued in a
                                                             //task_deque of appropriate thread pool that works with DB_module
    auto res = future_res.get();
    if (!res->res_succeed())
    {
        //std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
        req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " error while forming log table: " + res->res_error()));
        return;
    }
    if (res->get_rows_number() == 0)
    {
        req_info[REQ_RESULT] += "Log table have no records at that moment";
        return;
    }
    result_container res_cntnr(res->get_result_container());
    forming_tables_helper(req_info, res_cntnr);
}

void Server_HTTP_handler::forming_tables_helper(std::vector<std::string>& req_info, result_container& res_cntnr)
{
    //Determining maximum length of strings in every column exclude last column (for formatted table sending)
    auto n_columns = res_cntnr[0].size() - 1;  //-1 here is because of we don't need to take into account
                                               //lenght of rows in the last column that would be sent in response
    std::vector<size_t> vec_max_len(n_columns, 0); //
    for (inner_result_container::size_type i = 0; i != n_columns; ++i)
    {
        for (result_container::size_type j = 0; j != res_cntnr.size(); ++j)
        {
            vec_max_len[i] = std::max<size_t>(vec_max_len[i], res_cntnr[j][i].second);
        }
    }
    //Forming files table in std::string
    inner_result_container::size_type j;
    for (result_container::size_type i = 0; i != res_cntnr.size(); ++i)
    {
        for (j = 0; j != n_columns; ++j)
        {
            req_info[REQ_RESULT] += res_cntnr[i][j].first + std::string(vec_max_len[j] - res_cntnr[i][j].second + 2, ' ');
        }
        //Adding last column in the requested table, this column doesn't need adding spaces for output
        req_info[REQ_RESULT] += (res_cntnr[i][j].first);
        req_info[REQ_RESULT].append(1, '\n');
    }
}

void Server_HTTP_handler::get_file_URI(std::vector<std::string>& req_info)
{
    auto& target = req_info[REQ_TARGET];
    if (target.back() == '/' || target.find("..") != std::string::npos)
    {
        req_info[REQ_ERROR] = "Illegal request-target";  //Fill target_body_info[REQ_ERROR] element (element for error)
        return;
    }

    if (target.size() > 1 && target.back() == '*')
    {
        target.back() = '%';
    }
    static thread_local std::string command_name{"SELECT song_name, song_url FROM song_table WHERE song_name SIMILAR TO \'"};
    static thread_local std::string command_uid{"SELECT song_name, song_url FROM song_table WHERE song_uid SIMILAR TO \'"};
    static const auto command_name_start_size = command_name.size();
    static const auto command_uid_start_size = command_uid.size();
    command_name += target + "\';";
    command_uid += target + "\';";
    auto future_res_name = DB_ptr->exec_command(command_name, false);  //Second argument (false) tells, that command must be queued in a
                                                                       //task_deque of appropriate thread pool that works with DB_module
    //"SELECT song_name, song_url FROM song_table WHERE song_name SIMILAR TO 'rt%?' LIMIT 1;"
    command_name.resize(command_name_start_size);
    auto future_res_uid = DB_ptr->exec_command(command_uid, false);
    command_uid.resize(command_uid_start_size);
    auto res_name = future_res_name.get();
    auto res_uid = future_res_uid.get();
    shared_PG_result res{nullptr};
    auto handled_name = res_handle(req_info, res_name);
    if (handled_name == DB_ERROR || handled_name == SEVERAL_FILES)
    {
        return;
    }
    auto handled_uid = res_handle(req_info, res_uid);
    if (handled_uid == DB_ERROR || handled_uid == SEVERAL_FILES)
    {
        return;
    }
    if (handled_name == NOT_FOUND && handled_uid == NOT_FOUND)
    {
        req_info[REQ_ERROR] = "File " + target + " not found";
        return;
    }
    if (handled_name == RESULT_IS_OK && handled_uid != RESULT_IS_OK)
    {
       res = res_name;
    } else {

        if (handled_name != RESULT_IS_OK && handled_uid == RESULT_IS_OK)
        {
           res = res_uid;
        } else {
            req_info[REQ_ERROR] = "Several files match the requested target(" + target + ") at once";
            return;
        }
    }
    result_container res_cntnr(res->get_result_container());
    target = res_cntnr[1][0].first;
    req_info[REQ_RESULT] = res_cntnr[1][1].first + std::string('/' + target);
}

int Server_HTTP_handler::res_handle(std::vector<std::string>& req_info, shared_PG_result res)
{
    auto& target = req_info[REQ_TARGET];
    if (!res->res_succeed())
    {
        //std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command_name
        //          << " query" << std::endl;
        req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " error while searching for target "
                                    + target + ": "+ res->res_error()));
        return DB_ERROR;
    }
    auto rows_number = res->get_rows_number();
    if (rows_number == 1)
    {
        return RESULT_IS_OK;
    }
    if (rows_number > 1)
    {
        req_info[REQ_ERROR] = "Several files match the requested target(" + target + ") at once";
        return SEVERAL_FILES;
    }
    if (rows_number == 0)
    {
        return NOT_FOUND;
    }
    //Actually we shouldn't be here, but just in case
    req_info[REQ_ERROR].append(std::string("Database " + res->res_DB_name() + " returned unusual result while searching for target "
                                + target));
    return DB_ERROR;
}

bool Server_HTTP_handler::handle(std::vector<std::string>& req_info)    //Returning bool value shows if we have request for file (true) or
                                                                        //request for something else (false)
                                                                        //First element is requested target, second - host, third - port, fourth - ip
                                                                        //fifth - user_agent, sixth - method, seventh - is for storing result of handling by Handler,
                                                                        //eighth - is for stroring possible error message
{
    auto& target = req_info[REQ_TARGET];
    update_log_table(req_info);
    //Check if we have a request for both, files and log tables
    if (target.empty() || (target.size() == 1 && target[0] == '/'))
    {
        forming_files_table(req_info);
        forming_log_table(req_info);
        return false;
    }
    //Check if we have a request for files table
    if (target == "files_table" || target == "Files_table")
    {
        forming_files_table(req_info);
        return false;
    }
    //Check if we have a request for logs table
    if (target == "log_table" || target == "Log_table")
    {
        forming_log_table(req_info);
        return false;
    }
    //If we are here, we have a request for file
    get_file_URI(req_info);
    return true;
}

void Server_dir_handler::check_dir()
{
    std::error_code ec;
    std::string_view msg;
    bool _is_directory = std::filesystem::is_directory(files_path, ec);
    if (ec || !_is_directory)
    {
        if (ec)
        {
            msg = "Unable to check if given path to files dir is directory: " + ec.message();
        } else {
            msg = "Given path to files dir is not a directory";
        }
        std::cerr << msg << std::endl;
        if (logger)
        {
            logger->make_record(std::string{msg.data(), msg.size()});
        }
        files_path.clear();
        check_res = false;
        return;
    }
    check_res = true;
}

void Server_dir_handler::find(std::vector<std::string>& req_info)
{
    auto& target = req_info[REQ_TARGET];
    if (target.back() == '/' || target.find("..") != std::string::npos)
    {
        req_info[REQ_ERROR] = "Illegal request-target";  //Fill target_body_info[REQ_ERROR] element (element for error)
        return;
    }
    static thread_local std::function<bool(std::string_view)> compare;
    if (target.size() > 1 && target.back() == '*')
    {
        target.resize(target.size() - 1);
        compare = [&target] (std::string_view curr_file) mutable {
            if (curr_file.starts_with(target))
            {
                target = curr_file;
                return true;
            }
            return false; };
    } else {
        compare = [target] (std::string_view curr_file)->bool { return curr_file == target; };
    }
    std::string_view curr_file;
    for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
    {
        if (dir_entry.is_regular_file())
        {
            curr_file = dir_entry.path().filename().c_str();
            if (compare(curr_file))
            {
                std::filesystem::path temp_filename{curr_file};
                auto temp_files_path = files_path;
                req_info[REQ_RESULT] = (temp_files_path /= temp_filename).string();
                return;
            }
        }
    }
    req_info[REQ_ERROR] = "File " + target + " not found";  //Fill target_body_info[REQ_ERROR] element (element for error)
    return;
}

std::vector<std::string>::size_type Server_dir_handler::number_of_digits(std::vector<std::string>::size_type n)
{
    std::vector<std::string>::size_type number_of_digits = 0;
    do
    {
        ++number_of_digits;
        n /= 10;
    } while (n != 0);
    return number_of_digits;
}

std::string Server_dir_handler::forming_files_table()
{
    std::vector<std::string> file_names;    //Temporary container to store all file names in specified dir;
    std::string::size_type max_file_length = 0;
    std::vector<std::string>::size_type i = 0;
    for (auto const& dir_entry : std::filesystem::directory_iterator{files_path})
    {
        if (dir_entry.is_regular_file())
        {
            file_names.push_back(dir_entry.path().filename().string());
            max_file_length = std::max(max_file_length, file_names[i].size());
            ++i;
        }
    }
    //Determine maximum number of digits in maximum number of files integer (for formatted string_body)
    auto max_digits = number_of_digits(i);
    //Max length of record in first column
    auto max_length_first_column = std::max<std::vector<std::string>::size_type>(max_digits, 3);  //3 is the length of first column
                                                                                                    //name (Pos)
    auto num_spaces = max_length_first_column - 3 + 1;
    std::string files_table = "Pos" + std::string(num_spaces, ' ') + "File name\n";
    for (std::vector<std::string>::size_type c = 0, pos = 1; c != i; ++c, ++pos)
    {
        num_spaces = max_length_first_column - number_of_digits(pos) + 1;
        files_table += std::to_string(pos) + std::string(num_spaces, ' ') + file_names[c] + '\n';
    }
    return files_table;
}

bool Server_dir_handler::set_path(const char* files_path_)
{
    files_path = files_path_;
    check_dir();
    return check_res;
}

bool Server_dir_handler::handle(std::vector<std::string>& req_info) //First element is requested target, second - host, third - port, fourth - ip
                                                                    //fifth - user_agent, sixth - method,
                                                                    //seventh - is for storing result of handling by Handler,
                                                                    //eighth - is for stroring possible error message
{
    auto& target = req_info[REQ_TARGET];
    //Check if we have a request for files table
    if (target.empty() || (target.size() == 1 && target[0] == '/') || target == "files_table" || target == "Files_table")
    {
        req_info[REQ_RESULT] = forming_files_table();
        return false;   //Shows that we have request for something else, but not the file
    } else {
        find(req_info);
        return true;    //Shows that we have request for file
    }
}
