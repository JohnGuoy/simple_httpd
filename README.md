# 一个简单的 HTTP 服务器

____________________________________________________________________________

## 描述

使用 C 语言（C89）编写，基于 Linux 多线程的一个简单的 HTTP 服务器，支持 Python 或者 PHP 动态脚本。

## 构建

软件环境需求：

* Linux 操作系统
* gcc 编译器
* cmake 构建器

如果要测试 Python 或者 PHP 动态脚本，那么还需要安装并配置好 Python 或者 PHP 的运行环境。

克隆代码到本地工作目录：
`$ git clone`

构建：

```bash
$ cd ./build/
$ cmake ..
-- The C compiler identification is GNU 4.8.5
-- Check for working C compiler: /usr/bin/cc
-- Check for working C compiler: /usr/bin/cc -- works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Configuring done
-- Generating done
-- Build files have been written to: /root/Projects/TCP-IP/24.2/build
$ make
Scanning dependencies of target simple_httpd
[100%] Building C object CMakeFiles/simple_httpd.dir/main.c.o
Linking C executable simple_httpd
[100%] Built target simple_httpd
```

得到可执行程序 simple_httpd。

## 运行

运行 simple_httpd 程序，绑定任意未被占用的端口，Web 根目录默认是当前工作目录：
`$ ./simple_httpd 6677`
打开浏览器，输入以下 URL 进行测试：

```url
http://< 运行 simple_httpd 程序的服务器的 IP>:6677/index.html
http://< 运行 simple_httpd 程序的服务器的 IP>:6677/index.py
http://< 运行 simple_httpd 程序的服务器的 IP>:6677/index.php
http://< 运行 simple_httpd 程序的服务器的 IP>:6677/index1.html
```
