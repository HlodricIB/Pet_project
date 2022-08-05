#include <functional>
#include "handler.h"

bool Inotify_DB_handler::handle(const std::vector<std::string>& arguments)
{
    std::string command;
    if (arguments[0] == "add")
    {
        size_t uid = std::hash<std::string>{}(arguments[1]);
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, "
                + arguments[1] + ", " + std::to_string(uid) + "); COMMIT";
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
        handle({std::string("dir_del")});   //Clearing previous song list
        auto amount = arguments.size();
        std::string add("add");
        for (std::vector<std::string>::size_type i = 1; i != amount; ++i)
        {
            handle({add, arguments[i]});
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
            std::cerr << "Database " << res->res_DB_name() << " error: " << res->res_status() << ", executing: " << command << " query" << std::endl;
            return;
        }
    };
    t_pool_ptr->push_task_back(std::move(lambda));
    return;
}
