# minMysql
A thin layer for mysql, to be usefull and not a the millionth kitchen sink project wiht 100K useless features

This lib will require the support of timezone, and enforce a UTC connection for now. So be ready to

zypper in mariadb-tools

mysql_tzinfo_to_sql /usr/share/zoneinfo | mysql -u root mysql -p

zypper in libmariadb-devel

It also now requires git@github.com:dublinbranch/mapExtensor.git

Just add in .pro

include(minMysql/minMysql.pri)


Or 

Link with

LIBS += -lmariadb
