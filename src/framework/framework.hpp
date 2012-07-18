/*
 * Framework.hpp
 *
 *  Created on: 2011-2-18
 *      Author: wqy
 */

#ifndef FRAMEWORK_HPP_
#define FRAMEWORK_HPP_
#include "common/base.hpp"
#include "ipc/types.hpp"
#include <map>
#include <string>

#define PROC_NAME_ENV_VAR_NAME "__PROC_NAME__"
#define MANAGER_SERVICE_PROCESS_TYPE 0
#define DISPATCHER_SERVICE_PROCESS_TYPE 65535

#define IPC_DISPATCH_OK 1
#define IPC_DISPATCH_ERR -1
#define IPC_DISPATCH_OVERFLOW 0

using std::map;
using std::string;
using namespace arch::ipc;
namespace arch
{
    namespace framework
    {

        typedef uint16 ServiceType;
        typedef uint16 ServiceIndex;

        struct ProcessAddress
        {
                ServiceType proc_type;
                ServiceIndex proc_idx;
                inline ProcessAddress() :
                        proc_type(0), proc_idx(0)
                {
                }
                inline ProcessAddress(ServiceType type, ServiceIndex idx) :
                        proc_type(type), proc_idx(idx)
                {
                }
                inline bool operator<(const ProcessAddress& other) const
                {
                    if (proc_type > other.proc_type)
                    {
                        return false;
                    }
                    if (proc_type == other.proc_type)
                    {
                        return proc_idx < other.proc_idx;
                    }
                    return true;
                }

        };

        /**
         *
         */
        struct FrameworkOptions
        {
                string home_dir; //��ǰ�ķ���home·��
                string manager_process_name; //
                uint32 ipc_shm_size; //����IPC�Ĺ����ڴ��С�� Ĭ��1M
                uint32 max_fifo_length_limit; //
                uint32 global_shm_size; //ȫ�ֹ����ڴ��С�� Ĭ��0
                IPCType ipc_type; //�Ƿ�ʹ�ù����ڴ���ΪManager-�ӽ��̼���Ϣͨ�ţ� Ĭ��false
                uint32 manager_routine_period; //manager�������м����ms
                string admin_listen_addr; // ���ڹ���client�ļ�������Ĭ�ϵ�ַ0.0.0.0
                uint16 admin_listen_port; // ���ڹ���client�ļ�������˿ڣ�Ĭ��60000
                uint32 max_startup_time; //�������ʱ��,��λms
                uint32 max_ipc_buffer_size; //����ڲ�IPC��������С
                bool enable_ctrl_channel;
                FrameworkOptions() :
                        home_dir("."), manager_process_name("Manager"), ipc_shm_size(
                                1024 * 1024), max_fifo_length_limit(2000), global_shm_size(
                                0), ipc_type(IPC_FIFO), manager_routine_period(
                                29), admin_listen_addr("127.0.0.1"), admin_listen_port(
                                60000), max_startup_time(60000), max_ipc_buffer_size(
                                0), enable_ctrl_channel(false)
                {
                }
        };

        extern bool g_proc_running;

    }
}

#endif /* FRAMEWORK_HPP_ */
