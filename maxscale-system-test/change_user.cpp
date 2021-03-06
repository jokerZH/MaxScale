/**
 * @file change_user.cpp mysql_change_user test
 *
 * - using RWSplit and user 'skysql': GRANT SELECT ON test.* TO user@'%'  identified by 'pass2';  FLUSH PRIVILEGES;
 * - create a new connection to RSplit as 'user'
 * - try INSERT expecting 'access denied'
 * - call mysql_change_user() to change user to 'skysql'
 * - try INSERT again expecting success
 * - try to execute mysql_change_user() to switch to user 'user' but use rong password (expecting access denied)
 * - try INSERT again expecting success (user should not be changed)
 */


#include <iostream>
#include "testconnections.h"

int main(int argc, char *argv[])
{
    TestConnections * Test = new TestConnections(argc, argv);
    Test->set_timeout(60);

    Test->repl->connect();
    Test->maxscales->connect_maxscale(0);

    Test->tprintf("Creating user 'user' \n");

    execute_query(Test->maxscales->conn_rwsplit[0], "DROP USER 'user'@'%%'");
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "CREATE USER user@'%%' identified by 'pass2'");
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "GRANT SELECT ON test.* TO user@'%%'");
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "FLUSH PRIVILEGES;");
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "DROP TABLE IF EXISTS t1");
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "CREATE TABLE t1 (x1 int, fl int)");

    Test->maxscales->restart_maxscale();
    sleep(2);
    Test->maxscales->connect_maxscale(0);

    Test->tprintf("Changing user... \n");
    Test->add_result(mysql_change_user(Test->maxscales->conn_rwsplit[0], (char *) "user", (char *) "pass2", (char *) "test") ,
                     "changing user failed \n");
    Test->tprintf("mysql_error is %s\n", mysql_error(Test->maxscales->conn_rwsplit[0]));

    Test->tprintf("Trying INSERT (expecting access denied)... \n");
    if ( execute_query(Test->maxscales->conn_rwsplit[0], (char *) "INSERT INTO t1 VALUES (77, 11);") == 0)
    {
        Test->add_result(1, "INSERT query succedded to user which does not have INSERT PRIVILEGES\n");
    }

    Test->tprintf("Changing user back... \n");
    Test->add_result(mysql_change_user(Test->maxscales->conn_rwsplit[0], Test->repl->user_name, Test->repl->password,
                                       (char *) "test"), "changing user failed \n");

    Test->tprintf("Trying INSERT (expecting success)... \n");
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "INSERT INTO t1 VALUES (77, 12);");

    Test->tprintf("Changing user with wrong password... \n");
    if (mysql_change_user(Test->maxscales->conn_rwsplit[0], (char *) "user", (char *) "wrong_pass2", (char *) "test") == 0)
    {
        Test->add_result(1, "changing user with wrong password successed! \n");
    }
    Test->tprintf("%s\n", mysql_error(Test->maxscales->conn_rwsplit[0]));
    if ((strstr(mysql_error(Test->maxscales->conn_rwsplit[0]), "Access denied for user")) == NULL)
    {
        Test->add_result(1, "There is no proper error message\n");
    }

    Test->tprintf("Trying INSERT again (expecting failure - change user should have failed)...");
    Test->add_result(!execute_query(Test->maxscales->conn_rwsplit[0], (char *) "INSERT INTO t1 VALUES (77, 13);"),
                     "Query should fail, MaxScale should disconnect on auth failure");

    Test->tprintf("Changing user with wrong password using ReadConn \n");
    if (mysql_change_user(Test->maxscales->conn_slave[0], (char *) "user", (char *) "wrong_pass2", (char *) "test") == 0)
    {
        Test->add_result(1, "FAILED: changing user with wrong password successed! \n");
    }
    Test->tprintf("%s\n", mysql_error(Test->maxscales->conn_slave[0]));
    if ((strstr(mysql_error(Test->maxscales->conn_slave[0]), "Access denied for user")) == NULL)
    {
        Test->add_result(1, "There is no proper error message\n");
    }

    Test->tprintf("Changing user for ReadConn \n");
    Test->add_result(mysql_change_user(Test->maxscales->conn_slave[0], (char *) "user", (char *) "pass2", (char *) "test") ,
                     "changing user failed \n");

    Test->maxscales->connect_maxscale(0);
    Test->try_query(Test->maxscales->conn_rwsplit[0], (char *) "DROP USER user@'%%';");
    execute_query_silent(Test->maxscales->conn_rwsplit[0], "DROP TABLE test.t1");

    Test->maxscales->close_maxscale_connections(0);
    int rval = Test->global_result;
    delete Test;
    return rval;
}

