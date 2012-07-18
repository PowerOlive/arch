/*
 * file_helper.cpp
 *
 *  Created on: 2011-3-2
 *      Author: wqy
 */

#include "util/file_helper.hpp"
#include "logging/logger_macros.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

namespace arch
{
    namespace util
    {
        bool is_file_exist(const std::string& path)
        {
            struct stat buf;
            int ret = stat(path.c_str(), &buf);
            if (0 == ret)
            {
                return S_ISREG(buf.st_mode);
            }
            return false;
        }

        bool is_dir_exist(const std::string& path)
        {
            struct stat buf;
            int ret = stat(path.c_str(), &buf);
            if (0 == ret)
            {
                return S_ISDIR(buf.st_mode);
            }
            return false;
        }

        bool make_dir(const std::string& para_path)
        {
            if (is_dir_exist(para_path))
            {
                return true;
            }
            if (is_file_exist(para_path))
            {
                ERROR_LOG("Exist file '%s' is not a dir.", para_path.c_str());
                return false;
            }
            std::string path = para_path;
            size_t found = path.rfind("/");
            if (found == path.size() - 1)
            {
                path = path.substr(0, path.size() - 1);
                found = path.rfind("/");
            }
            if (found != std::string::npos)
            {
                std::string base_dir = path.substr(0, found);
                if (make_dir(base_dir))
                {
                    //mode is 0755
                    return mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP
                            | S_IROTH | S_IXOTH) == 0;
                }
            }
            else
            {
                return mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP
                        | S_IROTH | S_IXOTH) == 0;
            }
            return false;
        }

        bool make_file(const std::string& para_path)
        {
            if (is_file_exist(para_path))
            {
                return true;
            }
            if (is_dir_exist(para_path))
            {
                ERROR_LOG("Exist file '%s' is not a regular file.", para_path.c_str());
                return false;
            }
            std::string path = para_path;
            size_t found = path.rfind("/");
            if (found != std::string::npos)
            {
                std::string base_dir = path.substr(0, found);
                if (make_dir(base_dir))
                {
                    //mode is 0755
                    return open(path.c_str(), O_CREAT, S_IRWXU | S_IRGRP
                            | S_IXGRP | S_IROTH | S_IXOTH) == 0;
                }
            }
            else
            {
                return open(path.c_str(), O_CREAT, S_IRWXU | S_IRGRP | S_IXGRP
                        | S_IROTH | S_IXOTH) == 0;
            }
            return false;
        }

        int make_fd_blocking(int fd)
        {
            int flags;
            if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
            {
                return -1;
            }
            if (fcntl(fd, F_SETFL, flags | ~O_NONBLOCK) == -1)
            {
                return -1;
            }
            return 0;
        }

        int make_fd_nonblocking(int fd)
        {
            int flags;
            if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
            {
                return -1;
            }
            if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                return -1;
            }
            return 0;
        }

        int make_tcp_nodelay(int fd)
        {
            int yes = 1;
            //return 1;
            return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        }

        int get_n_bytes_readable_on_socket(int fd)
        {
            int n = 4096;
            if (ioctl(fd, FIONREAD, &n) < 0)
                return -1;
            return n;
        }
    }
}
