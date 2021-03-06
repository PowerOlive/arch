/*
 * ManagerProcess.cpp
 *
 *  Created on: 2011-1-28
 *      Author: wqy
 */
#include "framework/process/manager_process.hpp"
#include "framework/handler/ipc_event_decoder.hpp"
#include "framework/handler/ipc_event_encoder.hpp"
#include "exception/exception.hpp"
#include "util/file_helper.hpp"
#include "util/perftools_helper.h"
#include "util/string_helper.hpp"
#include "logging/logger_macros.hpp"
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <unistd.h>

using namespace std;
using namespace arch::framework;
using namespace arch::channel;
using namespace arch::channel::fifo;
using namespace arch::util;
using namespace arch::ipc;
using arch::channel::codec::CommonDelimiters;
using arch::exception::Exception;

arch::framework::ManagerProcess* g_manager_proc = NULL;
bool arch::framework::g_proc_running = true;

ManagerProcess::ManagerProcess(FrameworkOptions options, uint32 maxChannels) :
        m_admin_channel(NULL), m_channel_service(maxChannels), m_options(
                options), m_shm(NULL)
{
}

void ManagerProcess::TryStop()
{
    if (!g_proc_running)
    {
        bool serv_empty = true;
        ServiceProcessFactoryTable::iterator it =
                m_service_factory_table.begin();
        while (it != m_service_factory_table.end())
        {
            ServiceProcessFactory* factory = it->second;
            if (factory->GetActiveServiceProcessSize() > 0)
            {
                serv_empty = false;
                break;
            }
            it++;
        }
        if (serv_empty)
        {
            //All service processes have stopped
            INFO_LOG("Stop manager process.");
            if (!GetManagerHandler()->OnDestroy())
            {
                ERROR_LOG("Failed to destroy manager handler.");
            }
            if (!OnDestroy())
            {
                ERROR_LOG("Failed to destroy manager process. ");
            }
            m_channel_service.Stop();
        }
    }
}

void ManagerProcess::OnSignal(int signo, siginfo_t& info)
{
    char* sigstr = strsignal(signo);
    WARN_LOG(
            "Received signal(%s:%d) from process:%u", NULL != sigstr?sigstr:"UnknownSignal", signo, info.si_pid);
    if (signo == SIGCHLD)
    {
        while (true)
        {
            int status;
            pid_t child = waitpid(-1, &status, WNOHANG);
            if (0 == child || -1 == child)
            {
                break;
            }
            ServiceProcess* proc = GetServiceProcess(child);
            if (NULL != proc)
            {
                ServiceProcessFactory* factory =
                        proc->GetServiceProcessFactory();
                ServiceIndex idx = proc->GetProcessIndex();
                ServiceType type = proc->GetServiceType();
                factory->DeleteServiceProcess(proc);
                m_ipc_event_handler.BroadcastServiceProcessStoped(type, idx);
                //RemoveServiceProcess(child);
                if (g_proc_running)
                {
                    CreateServiceProcess(factory, idx);
                    LogPid4ProcessesInFile();
                }
                else
                {
                    TryStop();
                }
            }
        }
    }
    else if (signo == SIGPIPE)
    {
        //ignore
    }
    else if (signo == SIGQUIT)
    {
        g_proc_running = false;

        list<ServiceProcess*> proc_list;
        GetServiceProcessList(proc_list, 0);
        list<ServiceProcess*>::iterator it = proc_list.begin();
        while (it != proc_list.end())
        {
            ServiceProcess* proc = *it;
            proc->CloseIPCChannel();
            //proc->GetIPCChannel()->Close();
            kill(proc->GetPID(), SIGQUIT);
            it++;
        }
    }
    else if (signo == SIGUSR1)
    {

    }
    else if (signo == SIGUSR2)
    {

    }
}

void ManagerProcess::PrintPid4Processes(Buffer& buf)
{
    buf.Clear();
    buf.Printf("  PID   PPID  PROCESS\n");
    buf.Printf("%5u  %5u  %s\n", getpid(), getppid(),
            m_options.manager_process_name.c_str());
    ServiceProcessFactoryTable::iterator it = m_service_factory_table.begin();
    while (it != m_service_factory_table.end())
    {
        //uint8 type = it->first;
        ServiceProcessFactory* factory = it->second;
        it++;
        list<ServiceProcess*> proc_list;
        factory->GetActiveServiceProcessList(proc_list);
        list<ServiceProcess*>::iterator procit = proc_list.begin();
        while (procit != proc_list.end())
        {
            ServiceProcess* proc = *procit;
            buf.Printf("%5u  %5u  %s_%u_%u\n", proc->GetPID(), getpid(),
                    factory->GetName().c_str(), proc->GetServiceType(),
                    proc->GetProcessIndex());
            procit++;
        }
    }
}

void ManagerProcess::LogPid4ProcessesInFile()
{
    std::string pid_file = m_options.home_dir + "/.pids";
    FILE* fp = fopen(pid_file.c_str(), "w+");
    if (NULL == fp)
    {
        ERROR_LOG("Failed to open file:%s to log pids", pid_file.c_str());
        return;
    }
    Buffer buf;
    buf.EnsureWritableBytes(1024);
    PrintPid4Processes(buf);
    int fd = fileno(fp);
    int err;
    buf.WriteFD(fd, err);
    fclose(fp);
}

ServiceProcessFactory* ManagerProcess::GetServiceProcessFactory(
        ServiceType type)
{
    ServiceProcessFactoryTable::iterator it = m_service_factory_table.find(
            type);
    if (it != m_service_factory_table.end())
    {
        return it->second;
    }
    return NULL;
}

void ManagerProcess::GetAllServiceProcessFactory(
        std::list<ServiceProcessFactory*>& proc_factory_list)
{

    ServiceProcessFactoryTable::iterator it = m_service_factory_table.begin();
    while (it != m_service_factory_table.end())
    {
        proc_factory_list.push_back(it->second);
        it++;
    }
}

void ManagerProcess::GetServiceProcessList(
        std::list<ServiceProcess*>& proc_list, ServiceType type)
{
    if (type != 0)
    {
        ServiceProcessFactoryTable::iterator found =
                m_service_factory_table.find(type);
        if (found != m_service_factory_table.end())
        {
            ServiceProcessFactory* factory = found->second;
            factory->GetActiveServiceProcessList(proc_list);
        }
    }
    else
    {
        ServiceProcessFactoryTable::iterator it =
                m_service_factory_table.begin();
        for (; it != m_service_factory_table.end(); it++)
        {
            ServiceProcessFactory* factory = it->second;
            factory->GetActiveServiceProcessList(proc_list);
        }
    }
}

void ManagerProcess::RegisterServiceProcessFactory(
        ServiceProcessFactory* pfactory) throw ()
{
    if (NULL == pfactory || pfactory->GetName().empty())
    {
        ERROR_LOG("Register pfactory or it's name  is NULL.");
        throw Exception("Register pfactory or it's name  is NULL.");
    }
    if (0 == pfactory->GetType())
    {
        ERROR_LOG("0 is reserved for manager process.");
        throw Exception("0 is reserved for manager process.");
    }
    if (m_service_factory_table.count(pfactory->GetType()) > 0)
    {
        ERROR_LOG("Duplicate service factory type.");
        throw Exception("Duplicate service factory type.");
    }
    m_service_factory_table.insert(make_pair(pfactory->GetType(), pfactory));
}

void ManagerProcess::ClearAllServiceProcesses()
{
    ServiceProcessFactoryTable::iterator it = m_service_factory_table.begin();
    while (it != m_service_factory_table.end())
    {
        ServiceProcessFactory* facory = it->second;
        facory->DeleteAllServiceProcess();
        it++;
    }
    m_service_factory_table.clear();
}

static void unixSockPipelineInit(ChannelPipeline* pipeline, void* data)
{
    ManagerIPCEventHandler* ipchandler = (ManagerIPCEventHandler*) data;
    IPCEventEncoder* encoder = NULL;
    NEW(encoder, IPCEventEncoder);
    IPCEventDecoder* decoder = NULL;
    NEW(decoder, IPCEventDecoder);
    pipeline->AddLast("decoder", decoder);
    pipeline->AddLast("encoder", encoder);
    pipeline->AddLast("handler", ipchandler);
}

static void unixSockPipelineFinalize(ChannelPipeline* pipeline, void* data)
{
    ChannelHandler* decoder = pipeline->Get("decoder");
    DELETE(decoder);
    ChannelHandler* encoder = pipeline->Get("encoder");
    DELETE(encoder);
}

void ManagerProcess::CreateServiceProcess(ServiceProcessFactory* pfactory,
        ServiceIndex index)
{
    string ipcdir = m_options.home_dir + "/ipc";
    //make_dir(ipcdir);
    ServiceProcess* proc = pfactory->NewServiceProcess(index);
    proc->m_options = pfactory->GetOptions();
    proc->SetProcessIndex(index);
    proc->SetServiceProcessFactory(pfactory);
    proc->BaseInit(m_options);
    switch (m_options.ipc_type)
    {
        case IPC_FIFO:
        {
            string name = pfactory->GetName();
            char temp[ipcdir.size() + name.size() + 100];
            sprintf(temp, "%s/%s_%s%u", ipcdir.c_str(),
                    m_options.manager_process_name.c_str(), name.c_str(),
                    index);
            FIFOChannel* fifo = m_channel_service.NewFIFOChannel();
            proc->SetFIFO(fifo);
            fifo->SetReadPath(temp);
            fifo->Open();
            ChannelOptions ops;
            ops.user_write_buffer_water_mark = 8192;
            ops.user_write_buffer_flush_timeout_mills = 10;
            ops.max_write_buffer_size = m_options.max_ipc_buffer_size;
            fifo->Configure(ops);
            Process& currentProc = Process::CurrentProcess();
            Process child = currentProc.Fork(proc, false);
            sprintf(temp, "%s/%s_%s%u", ipcdir.c_str(), name.c_str(),
                    m_options.manager_process_name.c_str(), index);
            fifo->SetWritePath(temp);
            fifo->Open();
            fifo->GetPipeline().AddLast("decoder",
                    &(proc->GetIPCEventDecoder()));
            fifo->GetPipeline().AddLast("handler", &m_ipc_event_handler);
            fifo->GetPipeline().AddLast("encoder",
                    &(proc->GetIPCEventEncoder()));
            proc->m_pid = child.GetPID();
            break;
        }
        case IPC_SharedMemory:
        {
            SharedMemoryFIFOChannel* fifo =
                    m_channel_service.NewSharedMemoryFIFOChannel();
            proc->SetShmFIFO(fifo);
            fifo->Open();
            fifo->SetFIFOLengthLimit(m_options.max_fifo_length_limit);
            Process& currentProc = Process::CurrentProcess();
            Process child = currentProc.Fork(proc, false);
            fifo->SetProcessRole(true);
            fifo->GetPipeline().AddLast("decoder",
                    &(proc->GetIPCEventDecoder()));
            fifo->GetPipeline().AddLast("handler", &m_ipc_event_handler);
            fifo->GetPipeline().AddLast("encoder",
                    &(proc->GetIPCEventEncoder()));
            proc->m_pid = child.GetPID();
            break;
        }
        case IPC_UnixDomainSocket:
        {
            Process& currentProc = Process::CurrentProcess();
            Process child = currentProc.Fork(proc, false);
            proc->m_pid = child.GetPID();
            break;
        }
    }
}

void ManagerProcess::OnServiceRoutineTimeout(ServiceProcess* serv)
{
    WARN_LOG(
            "%s_%u(%u) is timeout for a long while.", serv->GetServiceName().c_str(), serv->GetProcessIndex(), serv->GetPID());
    if (serv->GetServiceProcessFactory()->GetOptions().restart_when_timeout)
    {
        KillServiceProcess(serv);
    }
}

void ManagerProcess::BaseRoutine()
{
    list<ServiceProcess*> proc_list;
    GetServiceProcessList(proc_list, 0);
    uint64 now = get_current_monotonic_millis();
    list<ServiceProcess*>::iterator it = proc_list.begin();
    while (it != proc_list.end())
    {
        ServiceProcess* proc = *it;
        if (!proc->GetStatus()->IsAlive(proc->m_options.timeout_count_limit,
                now))
        {
            OnServiceRoutineTimeout(proc);
        }
        if (!proc->GetStatus()->HaveBroadcasted())
        {
            m_ipc_event_handler.BroadcastServiceProcessStarted(
                    proc->GetServiceType(), proc->GetProcessIndex());
            list<ServiceProcess*>::iterator iter = proc_list.begin();
            while (iter != proc_list.end())
            {
                ServiceProcess* cur_proc = *iter;
                if (cur_proc != proc)
                {
                    m_ipc_event_handler.NotifyerviceProcessStarted(
                            cur_proc->GetServiceType(),
                            cur_proc->GetProcessIndex(), proc->GetServiceType(),
                            proc->GetProcessIndex());
                }
                iter++;
            }
            proc->GetStatus()->SetBroadcastedFlag(true);
        }
        it++;
    }
    m_channel_service.Routine();
}

void ManagerProcess::OnRoutine()
{
}

//routine method
void ManagerProcess::Run()
{
    if (!g_proc_running)
    {
        TryStop();
    }
    else
    {
        BaseRoutine();
        OnRoutine();
        if (NULL != GetManagerHandler())
        {
            GetManagerHandler()->OnRoutine();
        }
    }
}

int ManagerProcess::Wait4AllServiceProcStarted()
{
    INFO_LOG("Waiting for all service process started...");
    list<ServiceProcess*> proc_list;
    GetServiceProcessList(proc_list, 0);
    list<ServiceProcess*>::iterator it = proc_list.begin();
    uint32 count = 0;
    while (it != proc_list.end())
    {
        ServiceProcess* proc = *it;
        while (!proc->GetStatus()->IsStarted())
        {
            usleep(1000);
            count++;
            int status;
            pid_t child = waitpid(-1, &status, WNOHANG);
            if (child > 0)
            {
                ERROR_LOG("Child process:%u exit at startup.", child);
                return -1;
            }
            if (m_options.max_startup_time > 0
                    && count > m_options.max_startup_time)
            {
                ERROR_LOG(
                        "Process(%s_%u) startup exceed max time %ums", proc->GetServiceName().c_str(), proc->GetProcessIndex(), m_options.max_startup_time);
                return -1;
            }
        }
        it++;
    }INFO_LOG("All service process started successfully!");
    it = proc_list.begin();
    while (it != proc_list.end())
    {
        ServiceProcess* proc = *it;
        m_ipc_event_handler.BroadcastServiceProcessStarted(
                proc->GetServiceType(), proc->GetProcessIndex());
        proc->GetStatus()->SetBroadcastedFlag(true);
        it++;
    }

    return 0;
}

static void adminChannelPipelineInit(ChannelPipeline* pipeline, void* data)
{
    AdminMessageHandler* handler = (AdminMessageHandler*) data;
    DelimiterBasedFrameDecoder* decoder = NULL;
    NEW(
            decoder,
            DelimiterBasedFrameDecoder(8192, CommonDelimiters::lineDelimiter()));
    pipeline->AddLast("codec", decoder);
    pipeline->AddLast("handler", handler);
}

static void adminChannelPipelineFinalize(ChannelPipeline* pipeline, void* data)
{
    ChannelHandler* decoder = pipeline->Get("codec");
    DELETE(decoder);
}

int ManagerProcess::Execute()
{
    adjust_google_profiler();
    g_proc_running = true;
    chdir(m_options.home_dir.c_str());
    char* homedir = get_current_dir_name();
    if (NULL != homedir)
    {
        m_options.home_dir = homedir;
        free(homedir);
    }

    setenv(PROC_NAME_ENV_VAR_NAME, m_options.manager_process_name.c_str(), 1);
    logging::LoggerFactory::Destroy();
    logging::LoggerFactory::SetLoggerTimer(&(GetChannelService().GetTimer()));
    logging::LoggerFactory::Configure(
            m_options.home_dir + "/conf/logging.conf");
    INFO_LOG("%s process start.", m_options.manager_process_name.c_str());

    //开辟一块共享内存作为进程通信，管理
    NEW(
            m_shm,
            FastSharedMemoryPool(new SystemVSharedMemory(m_options.ipc_shm_size), true));
    //NEW(m_shm, FastSharedMemoryPool(new SystemVSharedMemory(m_options.ipc_shm_size), true));
    if (NULL == m_shm)
    {
        ERROR_LOG("Failed to create IPC shm pool in manager.");
        return -1;
    }
    SharedMemoryPoolHolder<IPC_FIFO_SHM_POOL>::SetSharedMemoryPool(m_shm);

    g_manager_proc = this;
    GetChannelService().GetSignalChannel().Register(SIGPIPE, g_manager_proc);
    GetChannelService().GetSignalChannel().Register(SIGCHLD, g_manager_proc);
    GetChannelService().GetSignalChannel().Register(SIGQUIT, g_manager_proc);
    GetChannelService().GetSignalChannel().Register(SIGUSR1, g_manager_proc);
    GetChannelService().GetSignalChannel().Register(SIGUSR2, g_manager_proc);

    //创建进程admin监听服务， 默认绑定在0.0.0.0:60000上
    m_admin_channel = GetChannelService().NewServerSocketChannel();
    m_admin_channel->SetChannelPipelineInitializor(adminChannelPipelineInit,
            &m_admin_msg_handler);
    m_admin_channel->SetChannelPipelineFinalizer(adminChannelPipelineFinalize,
            NULL);
    SocketHostAddress listen_addr(m_options.admin_listen_addr.c_str(),
            m_options.admin_listen_port);
    m_admin_channel->Bind(&listen_addr);

    if (!OnInit()) //回调子类初始化方法
    {
        ERROR_LOG("Failed to init manager process when invoke OnInit().");
        return -1;
    }
    ManagerHandler* man_handler = GetManagerHandler();
    m_ipc_event_handler.m_handler = man_handler;
    m_ipc_event_handler.m_manager = this;
    m_ipc_event_handler.Init();
    man_handler->m_proc = this;
    if (!man_handler->OnInit()) //回调子类handler初始化方法
    {
        ERROR_LOG("Failed to init manager handler ");
        return -1;
    }
    m_ipc_event_handler.SetIOProcessHandler(man_handler);
    string ipcdir = m_options.home_dir + "/ipc";
    make_dir(ipcdir);
    if (m_options.enable_ctrl_channel)
    {
        ServerSocketChannel* unixSockServer =
                m_channel_service.NewServerSocketChannel();
        char temp[ipcdir.size() + 100];
        sprintf(temp, "%s/MAN_CTRL.SOCK", ipcdir.c_str());
        SocketUnixAddress address(temp);
        if (!unixSockServer->Bind(&address))
        {
            ERROR_LOG("Failed to bind unix socket on %s", temp);
            return -1;
        }
        ChannelOptions ops;
        ops.user_write_buffer_water_mark = 8192;
        ops.user_write_buffer_flush_timeout_mills = 1;
        ops.max_write_buffer_size = m_options.max_ipc_buffer_size;
        unixSockServer->Configure(ops);
        unixSockServer->SetChannelPipelineInitializor(unixSockPipelineInit,
                &m_ipc_event_handler);
        unixSockServer->SetChannelPipelineFinalizer(unixSockPipelineFinalize,
                NULL);
    }

    if (m_options.ipc_type == IPC_UnixDomainSocket)
    {
        ServerSocketChannel* unixSockServer =
                m_channel_service.NewServerSocketChannel();
        char temp[ipcdir.size() + 100];
        sprintf(temp, "%s/MAN.SOCK", ipcdir.c_str());
        SocketUnixAddress address(temp);
        if (!unixSockServer->Bind(&address))
        {
            ERROR_LOG("Failed to bind unix socket on %s", temp);
            return -1;
        }
        ChannelOptions ops;
        ops.user_write_buffer_water_mark = 8192;
        ops.user_write_buffer_flush_timeout_mills = 1;
        ops.max_write_buffer_size = m_options.max_ipc_buffer_size;
        unixSockServer->Configure(ops);
        unixSockServer->SetChannelPipelineInitializor(unixSockPipelineInit,
                &m_ipc_event_handler);
        unixSockServer->SetChannelPipelineFinalizer(unixSockPipelineFinalize,
                NULL);
    }

    if (GetDispatcherCount() > 0)
    {
        if (1 == GetDispatcherCount())
        {
            WARN_LOG(
                    "Dispatcher process would not start when  worker number is not greater than 1.");
        }
        else
        {
            ServiceProcessFactory* pfactory = GetServiceProcessFactory(
                    DISPATCHER_SERVICE_PROCESS_TYPE);
            if (NULL != pfactory)
            {
                ServiceIndex count = pfactory->GetProcessCount();
                for (ServiceIndex i = 0; i < count; i++)
                {
                    CreateServiceProcess(pfactory, i);
                }
            }
            INFO_LOG("Create dispatcher processes finished.");
        }
    }
    ServiceProcessFactoryTable::iterator it = m_service_factory_table.begin();
    while (it != m_service_factory_table.end())
    {
        ServiceProcessFactory* pfactory = it->second;
        if (pfactory->GetType() != DISPATCHER_SERVICE_PROCESS_TYPE)
        {
            ServiceIndex count = pfactory->GetProcessCount();
            for (ServiceIndex i = 0; i < count; i++)
            {
                CreateServiceProcess(pfactory, i);
            }
        }

        it++;
    }

    LogPid4ProcessesInFile();
    if (-1 == Wait4AllServiceProcStarted())
    {
        return -1;
    }

    GetChannelService().GetTimer().Schedule(this,
            m_options.manager_routine_period, m_options.manager_routine_period);
    INFO_LOG("Manager process(%u) started successfully.", getpid());
    m_channel_service.Start(false);

    ClearAllServiceProcesses();
    DELETE(m_shm);
    INFO_LOG("Manager process exit.");
    logging::LoggerFactory::Destroy();
    return 0;
}

void ManagerProcess::Start()
{
    Process& currentProc = Process::CurrentProcess();
    Process child = currentProc.Fork(this);
}

ServiceProcess* ManagerProcess::GetServiceProcess(pid_t pid)
{
    ServiceProcessFactoryTable::iterator it = m_service_factory_table.begin();
    while (it != m_service_factory_table.end())
    {
        ServiceProcessFactory* factory = it->second;
        ServiceProcess* proc = factory->GetServiceProcessByPID(pid);
        if (NULL != proc)
        {
            return proc;
        }
        it++;
    }WARN_LOG("Service process not found for pid:%u", pid);
    return NULL;
}

ServiceProcess* ManagerProcess::GetServiceProcess(ServiceType type,
        ServiceIndex index)
{
    ServiceProcessFactoryTable::iterator found = m_service_factory_table.find(
            type);
    if (found != m_service_factory_table.end())
    {
        ServiceProcessFactory* facory = found->second;
        return facory->GetServiceProcessByIndex(index);
    }
    return NULL;
}

void ManagerProcess::KillServiceProcess(ServiceProcess* proc)
{
    WARN_LOG(
            "Send signal 11 to service process:%u to force process exit!", proc->GetPID());
    kill(proc->GetPID(), SIGSEGV);
}

uint32 ManagerProcess::GetDispatcherCount()
{
    ServiceProcessFactory* factory = GetServiceProcessFactory(
            DISPATCHER_SERVICE_PROCESS_TYPE);
    if (NULL == factory)
    {
        return 0;
    }
    return factory->GetProcessCount() > 1 ? factory->GetProcessCount() : 0;
}

int ManagerProcess::GetIPCClientUnixSocketFileName(const ProcessAddress& addr,
        std::string& result)
{
    char temp[100];
    ServiceProcessFactory* fac = GetServiceProcessFactory(addr.proc_type);
    if (NULL == fac)
    {
        return -1;
    }
    sprintf(temp, "%s_%u_%u.SOCK", fac->GetName().c_str(), addr.proc_type,
            addr.proc_idx);
    result = temp;
    return 0;
}

bool ManagerProcess::IsManagerIPCUnixSocketServer(const std::string& path)
{
    size_t found = path.rfind("/MAN.SOCK");
    if (found != std::string::npos)
    {
        return true;
    }
    return false;

}

bool ManagerProcess::IsManagerCtrlUnixSocketServer(const std::string& path)
{
    size_t found = path.rfind("/MAN_CTRL.SOCK");
    if (found != std::string::npos)
    {
        return true;
    }
    return false;
}

int ManagerProcess::GetDispatcherClientUnixSocketFileName(
        const ProcessAddress& addr, uint32 index, std::string& result)
{
    char temp[100];
    ServiceProcessFactory* fac = GetServiceProcessFactory(addr.proc_type);
    if (NULL == fac)
    {
        return -1;
    }
    sprintf(temp, "%s_%u_%u_%u.socket", fac->GetName().c_str(), addr.proc_type,
            addr.proc_idx, index);
    result = temp;
    return 0;
}

int ManagerProcess::ParseIPCClientUnixSocketAddress(
        const std::string& unixSocketFileName, ProcessAddress& addr)
{
    size_t found = unixSocketFileName.rfind(".SOCK");
    if (found != std::string::npos)
    {
        size_t next = unixSocketFileName.rfind('_', found);
        if (next == std::string::npos)
        {
            return -1;
        }
        std::string procindexstr = unixSocketFileName.substr(next + 1,
                found - next - 1);
        found = next - 1;
        next = unixSocketFileName.rfind('_', found);
        if (next == std::string::npos)
        {
            return -1;
        }
        std::string proctypestr = unixSocketFileName.substr(next + 1,
                found - next);

        uint32 proctype, procindex;
        if (string_touint32(proctypestr, proctype)
                && string_touint32(procindexstr, procindex))
        {
            addr.proc_type = proctype;
            addr.proc_idx = procindex;
            return 0;
        }
    }
    return -1;
}

int ManagerProcess::ParseDispatcherClientUnixSocketAddress(
        const std::string& unixSocketFileName, ProcessAddress& addr)
{
    size_t found = unixSocketFileName.rfind(".socket");
    if (found != std::string::npos)
    {
        size_t next = unixSocketFileName.rfind('_', found);
        if (next == std::string::npos)
        {
            return -1;
        }
        std::string indexstr = unixSocketFileName.substr(next + 1,
                found - next - 1);
        found = next - 1;
        next = unixSocketFileName.rfind('_', found);
        if (next == std::string::npos)
        {
            return -1;
        }
        std::string procindexstr = unixSocketFileName.substr(next + 1,
                found - next);
        found = next - 1;
        next = unixSocketFileName.rfind('_', found);
        if (next == std::string::npos)
        {
            return -1;
        }
        std::string proctypestr = unixSocketFileName.substr(next + 1,
                found - next);

        uint32 proctype, procindex;
        if (string_touint32(proctypestr, proctype)
                && string_touint32(procindexstr, procindex))
        {
            addr.proc_type = proctype;
            addr.proc_idx = procindex;
            return 0;
        }
    }

    return -1;
}

ManagerProcess::~ManagerProcess()
{
    ClearAllServiceProcesses();
    DELETE(m_shm);
}
