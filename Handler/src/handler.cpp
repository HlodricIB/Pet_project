#include <functional>
#include <boost/beast/core/string_type.hpp>
#include "handler.h"

bool Inotify_DB_handler::handle(std::vector<std::string>& arguments)
{
    std::string command;
    if (arguments[0] == "add")
    {
        size_t uid = std::hash<std::string>{}(arguments[1]);
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO song_table (id, song_name, song_uid, song_URL) VALUES (DEFAULT, "
                + arguments[1] + ", " + std::to_string(uid) + ", " + '\'' + files_folder + '\'' + "); COMMIT";
        exec_command(command);
        return true;
    }
    if (arguments[0] == "delete")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "DELETE FROM song_table WHERE song_name = '" + arguments[1] + "'; COMMIT";
        exec_command(command);
        return true;
    }
    if (arguments[0] == "dir_del")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY; COMMIT";
        exec_command(command);
        return true;
    }
    if (arguments[0] == "refresh")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY; COMMIT";
        auto future_res = DB_ptr->exec_command(command, true);   //Clearing previous song list
        auto res = future_res.get();    //Have to block here to avoid possibility of truncating subsequent addings
        if (!res->res_succeed())
        {
            std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_error() << ", executing: " << command << " query" << std::endl;
            return false;
        }
        auto amount = arguments.size();
        std::string add("add");
        for (std::vector<std::string>::size_type i = 1; i != amount; ++i)
        {
            std::vector<std::string> temp{add, arguments[i]};
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

std::string Server_HTTP_handler::update_log_table(std::vector<std::string>& target_body_info)
{
    auto& target = target_body_info[0];
    auto& host =  target_body_info[1];
    auto& port =  target_body_info[2];
    auto& ip_address =  target_body_info[3];
    auto& method =  target_body_info[4];
}

bool Server_HTTP_handler::handle(std::vector<std::string>& target_body_info)    //First element is requested target, second - host, third - port, fourth - ip
                                                                                //fifth - method, sixth - is for storing result of handling by Handler,
                                                                                //seventh - is for stroring possible error message
{
    auto& target = target_body_info[0];
    auto error_msg = update_log_table(target_body_info);
    //Check if we have a request for both, files and log tables
    if (target.empty() || (target.size() == 1 && target[0] == '/'))
    {

    }
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
        files_path.clear();
        check_res = false;
        return;
    }
    check_res = true;
}

void Server_dir_handler::find(std::vector<std::string>& target_body_info)
{
    auto& target = target_body_info[0];
    if (target.back() == '/' || target.find("..") != std::string::npos)
    {
        target_body_info.emplace_back();    //Fill target_body_info[5] element to fill target_body_info[6] element
        target_body_info.emplace_back("Illegal request-target\n");  //Fill target_body_info[6] element (element for error)
        return;
    }
    static std::function<bool(std::string_view)> compare;
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
                target_body_info.emplace(target_body_info.begin() + 1, (temp_files_path /= temp_filename).string());
                return;
            }
        }
    }
    target_body_info.emplace_back();    //Fill target_body_info[5] element to fill target_body_info[6] element
    target_body_info.emplace_back("File " + target + " not found\n");  //Fill target_body_info[6] element (element for error)
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

std::string Server_dir_handler::forming_files_list()
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

bool Server_dir_handler::handle(std::vector<std::string>& target_body_info) //First element is requested target, second - host, third - port, fourth - ip
                                                                            //fifth - method, sixth - is for storing result of handling by Handler,
                                                                            //seventh - is for stroring possible error message
{
    auto& target = target_body_info[0];
    //Check if we have a request for files list
    if (target.empty() || (target.size() == 1 && target[0] == '/') || target == "files_list" || target == "Files_list")
    {
        target_body_info.emplace(target_body_info.begin() + 1, forming_files_list());
        return false;   //Shows that we have request for something else, but not the file
    } else {
        find(target_body_info);
        return true;    //Shows that we have request for file
    }
}
