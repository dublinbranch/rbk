#ifndef MYMARIA_H
#define MYMARIA_H

#if __has_include(<mariadb/mysql.h>)
    #include <mariadb/mysql.h>
	#include <mariadb/mysqld_error.h>
#elif __has_include(<mysql/mysql.h>)
    #include <mysql/mysql.h>
	#include "mysql/mysqld_error.h"
#else
    #error "Neither MySQL nor MariaDB headers found"
#endif

#endif // MYMARIA_H
