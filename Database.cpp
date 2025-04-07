#include "Database.h"

bool Database::connect() {
    SQLRETURN retcode;

    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate environment handle\n";
        return false;
    }

    retcode = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to set ODBC version attribute\n";
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }

    retcode = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        std::cerr << "Failed to allocate connection handle\n";
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }

    // Set login timeout to 60 seconds
    SQLSetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)60, SQL_IS_INTEGER);

    // Use a connection string instead of a DSN
    SQLWCHAR connString[] = L"DRIVER={SQL Server};SERVER=ZARKUHP;DATABASE=UsersFileExplorer;Trusted_Connection=Yes;";
    // For SQL Server Authentication, use:
    // SQLWCHAR connString[] = L"DRIVER={SQL Server};SERVER=your_server_name;DATABASE=your_database;UID=your_username;PWD=your_password;";

    SQLWCHAR outConnString[1024];
    SQLSMALLINT outConnStringLen;
    retcode = SQLDriverConnect(hDbc, NULL, connString, SQL_NTS, outConnString, 1024, &outConnStringLen, SQL_DRIVER_NOPROMPT);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) {
        SQLWCHAR sqlState[6], messageText[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLSMALLINT recNum = 1;

        while (SQLGetDiagRec(SQL_HANDLE_DBC, hDbc, recNum++, sqlState, &nativeError, messageText, SQL_MAX_MESSAGE_LENGTH - 1, &textLength) == SQL_SUCCESS) {
            std::wcerr << L"Failed to connect. SQLState: " << sqlState << L", Message: " << messageText << std::endl;
        }

        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }

    std::cout << "Database connected successfully\n";
    return true;
}


void Database::disconnect() {
    if (hStmt) {
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        hStmt = NULL;
    }
    if (hDbc) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        hDbc = NULL;
    }
    if (hEnv) {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        hEnv = NULL;
    }
}