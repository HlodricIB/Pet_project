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
    //An order of std::string in arguments vector for handle() function
    enum : size_t
    {
        ARGS_FILES_FOLDER,   //Must not end with '/'
        ARGS_EVENT,
        ARGS_ENTITY_NAME
    };
    void exec_command(const std::string&);
public:
    Inotify_DB_handler(std::shared_ptr<DB_module> DB_ptr_, std::shared_ptr<thread_pool> t_pool_ptr_): DB_ptr(DB_ptr_), t_pool_ptr(t_pool_ptr_) { }
    virtual bool handle(std::vector<std::string>&) override;
};

class Server_HTTP_handler : public Handler
{
public:
    using result_container = std::vector<std::vector<std::pair<const char*, size_t>>>;
    using inner_result_container = std::vector<std::pair<const char*, size_t>>;
private:
    std::shared_ptr<DB_module> DB_ptr{nullptr};
    int rows_limit{0};  //Number of rows to store in log_table, after exceeding this value, first rows_limit rows clearing
    int days_limit{0};  //Number of days to store records in log_table, after the expiration of this period, the records are deleted
    void find(std::vector<std::string>&);
    void forming_files_table(std::vector<std::string>&);
    void forming_log_table(std::vector<std::string>&);
    void forming_tables_helper(std::vector<std::string>&, result_container&);
    void update_log_table(std::vector<std::string>&);
    void get_file_URI(std::vector<std::string>&);
    //An order of parameters for res_handle function
    enum : int
    {
        DB_ERROR,
        RESULT_IS_OK,
        SEVERAL_FILES,
        NOT_FOUND,
    };
    int res_handle(std::vector<std::string>&, shared_PG_result, std::string&);  //Returning int value shows result of handling
public:
    Server_HTTP_handler(std::shared_ptr<DB_module>, int days_limit_ = 5, int rows_limit_ = 10);
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
