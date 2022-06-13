#include <functional>
#include "handler.h"

void Inotify_DB_handler::handle(const std::vector<std::string> &arguments)
{
    std::string command;
    if (arguments[0] == "add")
    {
        size_t uid = std::hash<std::string>{}(arguments[1]);
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "INSERT INTO song_table (id, song_name, song_uid) VALUES (DEFAULT, "
                + arguments[1] + ", " + std::to_string(uid) + "); COMMIT";
        auto future_res = DB_ptr->exec_command({command});
        auto lambda = [future_res = std::move(future_res)] () mutable {
            auto res = future_res.get();
        };
        if (t_pool_ptr)
        {
            t_pool_ptr->push_task(lambda);
        } else
        {
            lambda();
        }
        return;
    }
    if (arguments[0] == "delete")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; "
                "DELETE FROM song_table WHERE song_name = '" + arguments[1] + "'; COMMIT";
        auto i = DB_ptr->exec_command({command});
        auto future_res = DB_ptr->exec_command({command});
        auto lambda = [future_res = std::move(future_res)] () mutable {
            auto res = future_res.get();
        };
        if (t_pool_ptr)
        {
            t_pool_ptr->push_task(lambda);
        } else
        {
            lambda();
        }
        return;
    }
    if (arguments[0] == "dir_del")
    {
        command = "BEGIN; LOCK TABLE song_table IN ACCESS EXCLUSIVE MODE; TRUNCATE song_table RESTART IDENTITY; COMMIT";
        auto i = DB_ptr->exec_command({command});
        auto future_res = DB_ptr->exec_command({command});
        auto lambda = [future_res = std::move(future_res)] () mutable {
            auto res = future_res.get();
        };
        if (t_pool_ptr)
        {
            t_pool_ptr->push_task(lambda);
        } else
        {
            lambda();
        }
        return;
    }
    if (arguments[0] == "refresh")
    {
        auto amount = arguments.size() - 1;
        std::string add("add");
        for (std::vector<std::string>::size_type i = 0; i != amount; ++i)
        {
            handle({add, arguments[i]});
        }
        return;
    }
}
