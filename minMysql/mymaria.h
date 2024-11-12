#ifndef MYMARIA_H
#define MYMARIA_H

#if __has_include(<mariadb/mysql.h>)
    #include <mariadb/mysql.h>
#elif __has_include(<mysql/mysql.h>)
    #include <mysql/mysql.h>
#else
    #error "Neither MySQL nor MariaDB headers found"
#endif

#endif // MYMARIA_H
