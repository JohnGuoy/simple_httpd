#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define SMALL_BUF_SIZE 100

#define SERVER_NAME "simple httpd\r\n"

enum FILE_TYPE
{
    ENUM_HTML, ENUM_PYTHON, ENUM_PHP, ENUM_OTHER_FILE_TYPE
};

void *request_handler(void *arg);

void response(FILE *fp, const char *file_name);

enum FILE_TYPE get_file_type(const char *file_name);

long int get_content_length(const char *file_name);

void send_data_from_html(FILE *fp, const char *file_name);

void send_data_from_py(FILE *fp, const char *file_name);

void send_data_from_php(FILE *fp, const char *file_name);

void send_error_msg(FILE *fp, const int error_code);


int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size, opt_size;
    int option;

    pthread_t t_id;

    if (argc != 2)
    {
        fprintf(stderr, "Usage:%s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    opt_size = sizeof(option);
    option = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &option, opt_size);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (-1 == bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))
    {
        fprintf(stderr, "bind error.\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(serv_sock, 5))
    {
        fprintf(stderr, "listen error.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stdout, "Listening on 0.0.0.0:%s...\n", argv[1]);
    }

    clnt_addr_size = sizeof(clnt_addr);
    while (1)
    {
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);

        fprintf(stdout, "Connection request:%s:%d\n", inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

        pthread_create(&t_id, NULL, request_handler, &clnt_sock);
        pthread_detach(t_id);
    }

    close(serv_sock);

    return 0;
}

void *request_handler(void *arg)
{
    int clnt_sock = *(int *) arg;
    char request_msg[SMALL_BUF_SIZE] = {0};
    FILE *read_clnt_fp, *write_clnt_fp;

    char http_method[10] = {0};
    char file_name[SMALL_BUF_SIZE] = {0};

    read_clnt_fp = fdopen(clnt_sock, "r");
    write_clnt_fp = fdopen(dup(clnt_sock), "w");
    fgets(request_msg, SMALL_BUF_SIZE, read_clnt_fp);

    if (NULL == strstr(request_msg, "HTTP/"))
    {
        send_error_msg(write_clnt_fp, 400);
        // 记录日志
        fprintf(stderr, "客户端%d不是HTTP协议的请求!\n", clnt_sock);
        fclose(read_clnt_fp);
        fclose(write_clnt_fp);
        return NULL;
    }

    strcpy(http_method, strtok(request_msg, " /"));
    strcpy(file_name, strtok(NULL, " /"));

    // 暂时只支持处理HTTP GET方法的请求
    if (strcmp(http_method, "GET") != 0)
    {
        send_error_msg(write_clnt_fp, 400);
        // 记录日志
        fprintf(stderr, "客户端%d不是HTTP GET方法的请求!\n", clnt_sock);
        fclose(read_clnt_fp);
        fclose(write_clnt_fp);
        return NULL;
    }

    fclose(read_clnt_fp);
    response(write_clnt_fp, file_name);

    // 随便返回点什么，反正用不到
    return (void *) 1;
}

void response(FILE *fp, const char *file_name)
{
    /*
     根据文件的后缀名判断请求的文件是HTML文件，还是Python脚本文件，还是PHP脚本文件。如果请求动态脚本文件，那么可以使用popen函数开启一个进程
     执行该脚本，获得该进程返回的数据，响应给客户端。
     */
    enum FILE_TYPE file_type = get_file_type(file_name);

    switch (file_type)
    {
        case ENUM_HTML:
            send_data_from_html(fp, file_name);
            break;
        case ENUM_PYTHON:
            send_data_from_py(fp, file_name);
            break;
        case ENUM_PHP:
            send_data_from_php(fp, file_name);
            break;
        default:
        {
            send_error_msg(fp, 404);
            // 记录日志
            fprintf(stderr, "客户端%d请求的文件%s不存在!\n", fileno(fp), file_name);
            fclose(fp);
        }
    }
}

enum FILE_TYPE get_file_type(const char *file_name)
{
    char extension[SMALL_BUF_SIZE] = {0};
    char _file_name[SMALL_BUF_SIZE] = {0};

    strcpy(_file_name, file_name);
    strtok(_file_name, ".");
    strcpy(extension, strtok(NULL, "."));

    if (!strcmp(extension, "html") || !strcmp(extension, "htm") || !strcmp(extension, "shtml"))
        return ENUM_HTML;
    else if (!strcmp(extension, "py"))
        return ENUM_PYTHON;
    else if (!strcmp(extension, "php"))
        return ENUM_PHP;
    else
        return ENUM_OTHER_FILE_TYPE;
}

long int get_content_length(const char *file_name)
{
    struct stat stat_buf;
    stat(file_name, &stat_buf);

    return stat_buf.st_size;
}

void send_data_from_html(FILE *fp, const char *file_name)
{
    char protocol_and_status[] = "HTTP/1.0 200 ok\r\n";
    char server_name[] = SERVER_NAME;
    char content_length[SMALL_BUF_SIZE] = {0};
    char content_type[] = "Content-type:text/html\r\n\r\n";
    char content[BUF_SIZE] = {0};
    FILE *response_file_fp;

    sprintf(content_length, "Content-length:%ld\r\n", get_content_length(file_name));

    response_file_fp = fopen(file_name, "r");
    if (NULL == response_file_fp)
    {
        send_error_msg(fp, 404);
        // 记录日志
        fprintf(stderr, "客户端%d请求的文件%s不存在!\n", fileno(fp), file_name);
        fclose(fp);
        return;
    }

    // 传输响应消息的消息头，第1行是状态行
    fputs(protocol_and_status, fp);
    fputs(server_name, fp);
    fputs(content_length, fp);
    fputs(content_type, fp);

    // 传输响应消息的消息体
    while (fgets(content, BUF_SIZE, response_file_fp) != NULL)
    {
        fputs(content, fp);
        fflush(fp);
    }

    fclose(fp);
}

void send_data_from_py(FILE *fp, const char *file_name)
{
    char protocol_and_status[] = "HTTP/1.0 200 ok\r\n";
    char server_name[] = SERVER_NAME;
    char content_length[SMALL_BUF_SIZE] = {0};
    char content_type[] = "Content-type:text/html\r\n\r\n";

    char content[SMALL_BUF_SIZE] = {0};

    FILE *read_fp;
    char cmd[SMALL_BUF_SIZE] = {0};

    if (access(file_name, F_OK) != 0)
    {
        send_error_msg(fp, 404);
        // 记录日志
        fprintf(stderr, "客户端%d请求的文件%s不存在!\n", fileno(fp), file_name);
        fclose(fp);
        return;
    }

    // 使用popen函数开启一个进程执行该脚本，获得该进程返回的数据，响应给客户端。
    sprintf(cmd, "python %s", file_name);
    read_fp = popen(cmd, "r");
    if (NULL != read_fp)
    {
        if (fread(content, sizeof(char), SMALL_BUF_SIZE, read_fp) <= 0)
        {
            send_error_msg(fp, 400);
            // 记录日志
            fprintf(stderr, "客户端%d请求的命令%s执行出错!\n", fileno(fp), cmd);
            fclose(fp);
            return;
        }
        pclose(read_fp);
    }
    else
    {
        send_error_msg(fp, 404);
        // 记录日志
        fprintf(stderr, "客户端%d请求的命令%s执行出错!\n", fileno(fp), cmd);
        fclose(fp);
        return;
    }

    sprintf(content_length, "Content-length:%lu\r\n", strlen(content));

    fputs(protocol_and_status, fp);
    fputs(server_name, fp);
    fputs(content_length, fp);
    fputs(content_type, fp);

    fputs(content, fp);
    fflush(fp);
    fclose(fp);
}

void send_data_from_php(FILE *fp, const char *file_name)
{
    char protocol_and_status[] = "HTTP/1.0 200 ok\r\n";
    char server_name[] = SERVER_NAME;
    char content_length[SMALL_BUF_SIZE] = {0};
    char content_type[] = "Content-type:text/html\r\n\r\n";

    char content[SMALL_BUF_SIZE] = {0};

    FILE *read_fp;
    char cmd[SMALL_BUF_SIZE] = {0};

    if (access(file_name, F_OK) != 0)
    {
        send_error_msg(fp, 404);
        // 记录日志
        fprintf(stderr, "客户端%d请求的文件%s不存在!\n", fileno(fp), file_name);
        fclose(fp);
        return;
    }

    sprintf(cmd, "php %s", file_name);
    read_fp = popen(cmd, "r");
    if (NULL != read_fp)
    {
        if (fread(content, sizeof(char), SMALL_BUF_SIZE, read_fp) <= 0)
        {
            send_error_msg(fp, 400);
            // 记录日志
            fprintf(stderr, "客户端%d请求的命令%s执行出错!\n", fileno(fp), cmd);
            fclose(fp);
            return;
        }
        pclose(read_fp);
    }
    else
    {
        send_error_msg(fp, 404);
        // 记录日志
        fprintf(stderr, "客户端%d请求的命令%s执行出错!\n", fileno(fp), cmd);
        fclose(fp);
        return;
    }

    sprintf(content_length, "Content-length:%lu\r\n", strlen(content));

    fputs(protocol_and_status, fp);
    fputs(server_name, fp);
    fputs(content_length, fp);
    fputs(content_type, fp);

    fputs(content, fp);
    fflush(fp);
    fclose(fp);
}

void send_error_msg(FILE *fp, const int error_code)
{
    char *protocol_and_status;
    char server_name[] = SERVER_NAME;
    char content_length[SMALL_BUF_SIZE] = {0};
    char content_type[SMALL_BUF_SIZE] = "Content-type:text/html\r\n\r\n";

    char *content;

    if (400 == error_code)
    {
        protocol_and_status = "HTTP/1.0 400 Bad Request\r\n";
        content = "<html><head><title>network</title></head><body>400 Bad Request</body></html>";


    }
    else if (404 == error_code)
    {
        protocol_and_status = "HTTP/1.0 404 Not Found\r\n";
        content = "<html><head><title>network</title></head><body>404 Not Found</body></html>";
    }

    sprintf(content_length, "Content-length:%lu\r\n", strlen(content));

    fputs(protocol_and_status, fp);
    fputs(server_name, fp);
    fputs(content_length, fp);
    fputs(content_type, fp);

    fputs(content, fp);
    fflush(fp);
    // fclose(fp); // fp在上层函数里记录错误日志时还要用到，如果在这里被fclose掉，将引发Bad file descriptor错误
}
