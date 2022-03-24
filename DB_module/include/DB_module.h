#ifndef DB_MODULE_H
#define DB_MODULE_H

#include <iostream>
#include "libpq-fe.h"
#include "parser.h"

class DB_module
{
private:
    PGconn* conn;
    void exit_nicely() { PQfinish(conn); exit(1); };
public:
    DB_module(const Parser_DB&);
    ~DB_module() { PQfinish(conn); }
    void exec_command(const char*);
};

#endif // DB_MODULE_H
