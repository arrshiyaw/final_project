#include <stdio.h>
#include <postgresql/libpq-fe.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

void do_exit(PGconn *conn, PGresult *res)
{
    fprintf(stderr, "%s\n", PQerrorMessage(conn));
    PQclear(res);
    PQfinish(conn);
    FILE *f;
    f = fopen("../../../../../../../tmp/final_project_log/logg.log", "a+");
    if (f != NULL)
        fprintf(f, "Exit from c code ...\n");
    exit(1);
}

int main()
{
    FILE *f;
    f = fopen("../../../../../../../tmp/final_project_log/logg.log", "a+");
    PGconn *conn = PQconnectdb("user=postgres password=@Mahdi773155 dbname=fpdb");

    if (PQstatus(conn) == CONNECTION_BAD)
    {
        if (f != NULL)
        fprintf(f, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    PGresult *res = PQexec(conn, "DROP TABLE IF EXISTS fp_city_aggregation");
  	PQclear(res);
    res = PQexec(conn, "DROP TABLE IF EXISTS fp_store_aggregation");
    PQclear(res);
    
    
    res = PQexec(conn, "CREATE TABLE IF NOT EXISTS fp_stores_data("\
    "id SERIAL PRIMARY KEY,"\
    "time NUMERIC,"\
    "province VARCHAR,"\
    "city VARCHAR,"\
    "market_id INT,"\
    "product_id INT,"\
    "price INT,"\
    "quantity INT,"\
    "has_sold INT)");

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        do_exit(conn, res);
    }
    PQclear(res);

    res = PQexec(conn, "CREATE TEMP TABLE fp_stores_data_temp("\
    "id SERIAL PRIMARY KEY, time NUMERIC, province VARCHAR, city VARCHAR,"\
    "market_id INT, product_id INT, price INT, quantity INT, has_sold INT)");

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        do_exit(conn, res);
    }
    PQclear(res);
    DIR *d = opendir("/tmp/final_project");
    struct dirent *dir;
    
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            char store_query[200] = "COPY fp_stores_data(time, province, city, market_id, product_id, price, quantity, has_sold) FROM '/tmp/final_project/";
            if (!(strcmp(dir->d_name,".") && strcmp(dir->d_name,"..")))
                continue;
            strcat(store_query, dir->d_name);
            strcat(store_query,"' DELIMITER ',' CSV");
            res = PQexec(conn, store_query);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
                do_exit(conn, res);

            PQclear(res);
            char temp_store_query[200] = "COPY fp_stores_data_temp(time, province, city, market_id, product_id, price, quantity, has_sold) FROM '/tmp/final_project/";
            strcat(temp_store_query,dir->d_name);
            strcat(temp_store_query,"' DELIMITER ',' CSV");

            res = PQexec(conn, temp_store_query);

            if (PQresultStatus(res) != PGRES_COMMAND_OK)
                do_exit(conn, res);
            PQclear(res);

        }
        closedir(d);
    }
  	res = PQexec(conn, "SELECT time, city, SUM(quantity) whole_product, SUM(has_sold) total_sales, SUM(quantity * price) / SUM(quantity) average_price "\
  					   "INTO TABLE fp_city_aggregation "\
                       "FROM fp_stores_data_temp "\
  					   "GROUP BY time, city ");
    fprintf(f, "create fp_city_aggregation successfully ...\n");
    PQclear(res);

    res = PQexec(conn, "SELECT market_id, SUM(has_sold) total_sales, SUM(has_sold * price) total_sales_price "\
               "INTO TABLE fp_store_aggregation "\
               "FROM fp_stores_data_temp "\
               "GROUP BY market_id");
    fprintf(f, "create fp_store_aggregation successfully ...\n");
    PQclear(res);

    PQfinish(conn);
    return 0;
}
