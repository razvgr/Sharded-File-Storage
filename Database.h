#pragma once
#include <atomic>
#include <iostream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <thread>


#pragma comment(lib, "odbc32.lib")

class Database
{
public:
    SQLHENV hEnv;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;


    Database() : hEnv(NULL), hDbc(NULL), hStmt(NULL) {}

    ~Database() {
        disconnect();
    }

    bool connect();

    void disconnect();
};

