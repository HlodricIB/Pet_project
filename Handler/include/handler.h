#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <memory>
#include "DB_module.h"

class Handler
{
public:
    Handler() { }
    virtual bool handle(std::vector<std::string>&) = 0;
    virtual ~Handler() { };
};

class Inotify_DB_handler : public Handler
{
private:
    std::shared_ptr<DB_module> DB_ptr{nullptr};
    std::shared_ptr<thread_pool> t_pool_ptr{nullptr};
    std::string files_folder;
    void exec_command(const std::string&);
public:
    Inotify_DB_handler(std::shared_ptr<DB_module> DB_ptr_, std::shared_ptr<thread_pool> t_pool_ptr_): DB_ptr(DB_ptr_), t_pool_ptr(t_pool_ptr_) { }
    virtual bool handle(std::vector<std::string>&) override;
    void set_files_folder(std::string files_folder_) { files_folder = files_folder_; }
};

class Server_HTTP_handler : public Handler
{
public:
    using result_container = std::vector<std::vector<std::pair<const char*, size_t>>>;
    using inner_result_container = std::vector<std::pair<const char*, size_t>>;
private:
    std::shared_ptr<DB_module> DB_ptr{nullptr};
    void find(std::vector<std::string>&);
    void forming_files_table(std::vector<std::string>&);
    void forming_log_table(std::vector<std::string>&);
    void forming_tables_helper(std::vector<std::string>&, result_container&);
    void update_log_table(std::vector<std::string>&); //Returnig std::string is for passing possible error messages
    void get_file_URI(std::vector<std::string>&);
public:
    Server_HTTP_handler(std::shared_ptr<DB_module> DB_ptr_): DB_ptr(DB_ptr_) { }
    virtual bool handle(std::vector<std::string>&) override;    //Returning bool value shows if we have request for file (true) or
                                                                //request for something else (false)
                                                                //First element is requested target, second - host, third - port, fourth - ip
                                                                //fifth - method, sixth - is for storing result of handling by Handler,
                                                                //seventh - is for stroring possible error message
};

class Server_dir_handler : public Handler
{
private:
    std::filesystem::path files_path;
    bool check_res{false};
    void check_dir();
    void find(std::vector<std::string>&);
    std::string forming_files_table();
    std::vector<std::string>::size_type number_of_digits(std::vector<std::string>::size_type);
public:
    Server_dir_handler(const char* files_path_): files_path(files_path_) { check_dir(); }
    virtual bool handle(std::vector<std::string>&) override;    //Returning bool value shows if we have request for file (true) or
                                                                //request for something else (false)
                                                                //First element is requested target, second - host, third - port, fourth - ip
                                                                //fifth - method, sixth - is for storing result of handling by Handler,
                                                                //seventh - is for stroring possible error message
    bool set_path(const char*);
};

#endif // HANDLER_H
