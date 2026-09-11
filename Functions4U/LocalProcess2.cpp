// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 - 2022, the Anboto author and contributors
#include <Core/Core.h>
#include "LocalProcess2.h"


namespace Upp {

#ifdef PLATFORM_WIN32
#include <tlhelp32.h>
#include <psapi.h>
#endif
#ifdef PLATFORM_POSIX
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif


void LocalProcess2::Init() {
#ifdef PLATFORM_WIN32
	hProcess = hOutputRead = hErrorRead = hInputWrite = NULL;
	dwProcessId = DWORD(NULL);
#endif
#ifdef PLATFORM_POSIX
	pid = 0;
	doublefork = false;
	rpipe[0] = rpipe[1] = wpipe[0] = wpipe[1] = epipe[0] = epipe[1] = -1;
#endif
	exit_code = Null;
	convertcharset = true;
}

void LocalProcess2::Free() {
#ifdef PLATFORM_WIN32
	if(hProcess) {
		CloseHandle(hProcess);
		hProcess = NULL;
	}
	if(hOutputRead) {
		CloseHandle(hOutputRead);
		hOutputRead = NULL;
	}
	if(hErrorRead) {
		CloseHandle(hErrorRead);
		hErrorRead = NULL;
	}
	if(hInputWrite) {
		CloseHandle(hInputWrite);
		hInputWrite = NULL;
	}
#endif
#ifdef PLATFORM_POSIX
	if (rpipe[0] >= 0) { close(rpipe[0]); rpipe[0] = -1; }
	if (rpipe[1] >= 0) { close(rpipe[1]); rpipe[1] = -1; }
	if (wpipe[0] >= 0) { close(wpipe[0]); wpipe[0] = -1; }
	if (wpipe[1] >= 0) { close(wpipe[1]); wpipe[1] = -1; }
	if (epipe[0] >= 0) { close(epipe[0]); epipe[0] = -1; }
	if (epipe[1] >= 0) { close(epipe[1]); epipe[1] = -1; }
	if (pid) 		   waitpid(pid, 0, WNOHANG | WUNTRACED);
	pid = 0;
#endif
}

#ifdef PLATFORM_POSIX
static void sNoBlock(int fd)
{
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
#endif

#ifdef PLATFORM_WIN32
bool Win32CreateProcess2(const char *command, const char *envptr, STARTUPINFOW& si, PROCESS_INFORMATION& pi, const char *cd)
{ // provides conversion of charset for cmdline and envptr
	Vector<WCHAR> cmd = ToSystemCharsetW(command);
	cmd.Add(0);
#if 0 // unicode environment not necessary for now
	wchar wenvptr = NULL;
	Buffer<WCHAR> env(n);
	if(envptr) {
		int len = 0;
		while(envptr[len] || envptr[len + 1])
			len++;
		WString wenv(envptr, len + 1);
		env.Alloc(len + 2);
		memcpy(env, wenv, (len + 2) * sizeof(wchar));
	}
#endif
	return CreateProcessW(NULL, cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, (void *)envptr,
	                      cd ? ToSystemCharsetW(cd).begin() : NULL, &si, &pi);
}
#endif

bool LocalProcess2::DoStart(const char *_command, const Vector<String> *arg, bool spliterr, const char *envptr, const char *cd)
{
	Kill();
	exit_code = Null;

	String command = TrimBoth(_command);

#ifdef PLATFORM_WIN32
	paused = false;
	HANDLE hOutputReadTmp, hOutputWrite;
	HANDLE hInputWriteTmp, hInputRead;
	HANDLE hErrorReadTmp, hErrorWrite;

	HANDLE hp = GetCurrentProcess();

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0);
	DuplicateHandle(hp, hInputWriteTmp, hp, &hInputWrite, 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hInputWriteTmp);

	CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0);
	DuplicateHandle(hp, hOutputReadTmp, hp, &hOutputRead, 0, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hOutputReadTmp);

	DWORD pipeMode = PIPE_NOWAIT;
    SetNamedPipeHandleState(hOutputRead, &pipeMode, NULL, NULL);

	if(spliterr) {
		CreatePipe(&hErrorReadTmp, &hErrorWrite, &sa, 0);
		DuplicateHandle(hp, hErrorReadTmp, hp, &hErrorRead, 0, FALSE, DUPLICATE_SAME_ACCESS);
		CloseHandle(hErrorReadTmp);
	} else
		DuplicateHandle(hp, hOutputWrite, hp, &hErrorWrite, 0, TRUE, DUPLICATE_SAME_ACCESS);

	PROCESS_INFORMATION pi;
	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(STARTUPINFOW));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdInput  = hInputRead;
	si.hStdOutput = hOutputWrite;
	si.hStdError  = hErrorWrite;
	String cmdh;
	if(arg) {
		cmdh = command;
		for(int i = 0; i < arg->GetCount(); i++) {
			cmdh << ' ';
			String argument = (*arg)[i];
			if(argument.GetCount() && argument.FindFirstOf(" \t\n\v\"") < 0)
				cmdh << argument;
			else {
				cmdh << '\"';
				const char *s = argument;
				for(;;) {
					int num_backslashes = 0;
					while(*s == '\\') {
						s++;
						num_backslashes++;
					}
					if(*s == '\0') {
						cmdh.Cat('\\', 2 * num_backslashes);
						break;
					}
					else
					if(*s == '\"') {
						cmdh.Cat('\\', 2 * num_backslashes + 1);
						cmdh << '\"';
					}
					else {
						cmdh.Cat('\\', num_backslashes);
						cmdh.Cat(*s);
					}
					s++;
				}
				cmdh << '\"';
			}
	    }
		command = cmdh;
	}
	bool h = Win32CreateProcess2(command, envptr, si, pi, cd);
	
	CloseHandle(hErrorWrite);
	CloseHandle(hInputRead);
	CloseHandle(hOutputWrite);
	if(h) {
		hProcess = pi.hProcess;
		dwProcessId = pi.dwProcessId;
		CloseHandle(pi.hThread);
	} else {
		Free();
		return false;
	}
	timeFaultsPrev = start = GetSysTime();
	return true;
#endif
#ifdef PLATFORM_POSIX
	if(arg) {
		int n = strlen(command) + 1;
		for(int i = 0; i < arg->GetCount(); i++)
			n += (*arg)[i].GetCount() + 1;
		cmd_buf.Alloc(size_t(n + 1));
		char *p = cmd_buf;
		args.Add(p);
		int l = strlen(command) + 1;
		memcpy(p, command, size_t(l));
		p += l;
		for(int i = 0; i < arg->GetCount(); i++) {
			args.Add(p);
			l = (*arg)[i].GetCount() + 1;
			memcpy(p, ~(*arg)[i], size_t(l));
			p += l;
		}
	}
	else { // parse command line for execve
		cmd_buf.Alloc(strlen(command) + 1);
		char *cmd_out = cmd_buf;
		const char *p = command;
		while(*p)
			if((byte)*p <= ' ')
				p++;
			else {
				args.Add(cmd_out);
				while(*p && (byte)*p > ' ') {
					int c = *p;
					if(c == '\\') {
						if(*++p)
							*cmd_out++ = *p++;
					}
					else if(c == '\"' || c == '\'') {
						p++;
						while(*p && *p != c)
							if(*p == '\\') {
								if(*++p)
									*cmd_out++ = *p++;
							}
							else
								*cmd_out++ = *p++;
						if(*p == c)
							p++;
					}
					else
						*cmd_out++ = *p++;
				}
				*cmd_out++ = '\0';
			}
	}
	
	if(args.GetCount() == 0)
		return false;

	args.Add(NULL);

	String app_full = GetFileOnPath(args[0], getenv("PATH"), true);
	if(IsNull(app_full))
		return false;
	
	Buffer<char> arg0(size_t(app_full.GetCount() + 1));
	memcpy(~arg0, ~app_full, size_t(app_full.GetCount() + 1));
	args[0] = ~arg0;

	if(pipe(rpipe) || pipe(wpipe))
		return false;

	if(spliterr && pipe(epipe))
		return false;

#ifdef CPU_BLACKFIN
	pid = vfork(); //we *can* use vfork here, since exec is done later or the parent will exit
#else
	pid = fork();
#endif
	if(pid < 0)
		return false;
//		throw Exc(NFormat(t_("fork() error; error code = %d"), errno));

	if(pid) {
		close(rpipe[0]); rpipe[0]=-1;
		close(wpipe[1]); wpipe[1]=-1;
		sNoBlock(rpipe[1]);
		sNoBlock(wpipe[0]);
		if (spliterr) {
			sNoBlock(epipe[0]);
			close(epipe[1]); epipe[1]=-1;
		}
		if (doublefork)
			pid = 0;
		return true;
	}

	if (doublefork) {
		pid_t pid2 = fork();
		
		if (pid2 < 0) {
			Exit(1);
		}
		if (pid2) {
			close(rpipe[0]); rpipe[0]=-1;
			close(wpipe[1]); wpipe[1]=-1;
			sNoBlock(rpipe[1]);
			sNoBlock(wpipe[0]);
			if (spliterr) {
				sNoBlock(epipe[0]);
				close(epipe[1]); epipe[1]=-1;
			}
			// we call exec instead of Exit, because exit doesn't behave nicelly with threads
			execl("/usr/bin/true", "[closing fork]", (char*)NULL);
			// only call Exit when execl fails
			Exit(0);
		}
	}

//	rpipe[1] = wpipe[0] = -1;
	dup2(rpipe[0], 0);
	dup2(wpipe[1], 1);
	dup2(spliterr ? epipe[1] : wpipe[1], 2);
	close(rpipe[0]);
	close(rpipe[1]);
	close(wpipe[0]);
	close(wpipe[1]);
	if (spliterr) {
		close(epipe[0]);
		close(epipe[1]);
	}
	rpipe[0] = rpipe[1] = wpipe[0] = wpipe[1] = epipe[0] = epipe[1] = -1;

	if(cd)
		(void)!chdir(cd); // that (void)! strange thing is to silence GCC warning

	if(envptr) {
		const char *from = envptr;
		Vector<const char *> env;
		while(*from) {
			env.Add(from);
			from += strlen(from) + 1;
		}
		env.Add(NULL);
		execve(app_full, args.Begin(), (char *const *)env.Begin());
	}
	else
		execv(app_full, args.Begin());
	
//	printf("Error running '%s', error code %d\n", command, errno);
	exit(-errno);
	return true;
#endif
}

#ifdef PLATFORM_POSIX
bool LocalProcess2::DecodeExitCode(int status)
{
	if(WIFEXITED(status)) {
		exit_code = (byte)WEXITSTATUS(status);
		return true;
	}
	else if(WIFSIGNALED(status) || WIFSTOPPED(status)) {
		static const struct {
			const char *name;
			int         code;
		}
		signal_map[] = {
#define SIGDEF(s) { #s, s },
SIGDEF(SIGHUP) SIGDEF(SIGINT) SIGDEF(SIGQUIT) SIGDEF(SIGILL) SIGDEF(SIGABRT)
SIGDEF(SIGFPE) SIGDEF(SIGKILL) SIGDEF(SIGSEGV) SIGDEF(SIGPIPE) SIGDEF(SIGALRM)
SIGDEF(SIGPIPE) SIGDEF(SIGTERM) SIGDEF(SIGUSR1) SIGDEF(SIGUSR2) SIGDEF(SIGTRAP)
SIGDEF(SIGURG) SIGDEF(SIGVTALRM) SIGDEF(SIGXCPU) SIGDEF(SIGXFSZ) SIGDEF(SIGIOT)
SIGDEF(SIGIO) SIGDEF(SIGWINCH)
#ifndef PLATFORM_BSD
//SIGDEF(SIGCLD) SIGDEF(SIGPWR)
#endif
//SIGDEF(SIGSTKFLT) SIGDEF(SIGUNUSED) // not in Solaris, make conditional if needed
#undef SIGDEF
		};

		int sig = (WIFSIGNALED(status) ? WTERMSIG(status) : WSTOPSIG(status));
		exit_code = (WIFSIGNALED(status) ? 1000 : 2000) + sig;
		exit_string << "\nProcess " << (WIFSIGNALED(status) ? "terminated" : "stopped") << " on signal " << sig;
		for(int i = 0; i < __countof(signal_map); i++)
			if(signal_map[i].code == sig)
			{
				exit_string << " (" << signal_map[i].name << ")";
				break;
			}
		exit_string << "\n";
		return true;
	}
	return false;
}
#endif//PLATFORM_POSIX

#ifdef PLATFORM_WIN32
static Vector<DWORD> GetChildProcessList(DWORD processId) {
	Vector<DWORD> child, all, parents;
	
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE) 
		return child;
	
	PROCESSENTRY32 proc;
	proc.dwSize = sizeof(proc);
	
	if (!Process32First(hSnap, &proc)) {
		CloseHandle(hSnap);	
		return child;
	}
	
	do {
		all << proc.th32ProcessID;
		parents << proc.th32ParentProcessID;
    } while(Process32Next(hSnap, &proc));
	
	CloseHandle(hSnap);
	
	child << processId;
	int init = 0;
	while (true) {
		int count = child.GetCount();
		if (init >= count)
			break;
		for (int cid = init; cid < count; ++cid) {
			for (int i = 0; i < all.GetCount(); ++i) {
				if (parents[i] == child[cid])
					child << all[i];
			}
		}
		init = count;
	}
	child.Remove(0);
	return child;	
}

 void TerminateChildProcesses(DWORD dwProcessId, UINT uExitCode) {
	Vector<DWORD> children = GetChildProcessList(dwProcessId);
	for (int i = 0; i < children.GetCount(); ++i) {
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, children[i]);
		TerminateProcess(hProcess, uExitCode);
		CloseHandle(hProcess);
	}
}

#endif

#ifdef PLATFORM_POSIX

static void GetDirectChildren(pid_t pid, Vector<pid_t>& out) {
    String path = Format("/proc/%d/task/%d/children", (int)pid, (int)pid);
    String content = LoadFile(path);
    if (content.IsEmpty())
        return;

    for (const String& tok : Split(content, ' ')) {
        String t = TrimBoth(tok);
        if(!t.IsEmpty())
            out << (pid_t)StrInt(t);
    }
}

 Vector<pid_t> GetChildProcessList(pid_t processId) {
    Vector<pid_t> result;
    Vector<pid_t> to_visit;
    to_visit << processId;

    int init = 0;
    while (init < to_visit.size()) {
        int count = to_visit.size();
        for (int i = init; i < count; i++) {
            pid_t current = to_visit[i];
            if(current != processId)
                result << current;
            GetDirectChildren(current, to_visit);
        }
        init = count;
    }
    return result;
}

#endif

void LocalProcess2::Kill() {
#ifdef PLATFORM_WIN32
	if(hProcess && IsRunning()) {
		TerminateChildProcesses(dwProcessId, (DWORD)-1);
		TerminateProcess(hProcess, (DWORD)-1);
		exit_code = 255;
	}
#endif
#ifdef PLATFORM_POSIX
	if(IsRunning()) {
		exit_code = 255;
		kill(pid, SIGTERM);
		GetExitCode();
		int status;
		if(pid && waitpid(pid, &status, 0) == pid)
			DecodeExitCode(status);
		exit_string = "Child process has been killed.\n";
	}
#endif
	Free();
}

void LocalProcess2::Detach()
{
#ifdef PLATFORM_POSIX
	if (doublefork)
		waitpid(pid, 0, WUNTRACED);
#endif
	Free();
}

bool LocalProcess2::IsRunning() {
#ifdef PLATFORM_WIN32
	dword exitcode;
	if(!hProcess)
		return false;
	if(GetExitCodeProcess(hProcess, &exitcode) && exitcode == STILL_ACTIVE)
		return true;
	dword n;
	if(PeekNamedPipe(hOutputRead, NULL, 0, NULL, &n, NULL) && n)
		return true;
	exit_code = (int)exitcode;
	return false;
#endif
#ifdef PLATFORM_POSIX
	if(!pid || !IsNull(exit_code))
		return false;
	
	int status = 0;
	if(!(waitpid(pid, &status, WNOHANG | WUNTRACED) == pid && DecodeExitCode(status)))
		return true;
	
	return false;
#endif
}

int  LocalProcess2::GetExitCode() {
#ifdef PLATFORM_WIN32
	return IsRunning() ? (int)Null : exit_code;
#endif
#ifdef PLATFORM_POSIX
	if(!IsRunning())
		return Nvl(exit_code, -1);
	int status;
	if(!( waitpid(pid, &status, WNOHANG | WUNTRACED) == pid && 
	      DecodeExitCode(status) ))
		return -1;
	
	return exit_code;
#endif
}

String LocalProcess2::GetExitMessage() {
#ifdef PLATFORM_POSIX
	if (!IsRunning() && GetExitCode() == -1)
		return exit_string;
	else
#endif
		return String();
}

bool LocalProcess2::Read(String& res) {
	String dummy;
	return Read2(res, dummy);
}

bool LocalProcess2::Read2(String& reso, String& rese) {
	reso = wreso;
	rese = wrese;
	wreso.Clear();
	wrese.Clear();

#ifdef PLATFORM_WIN32
	bool was_running = IsRunning();
	char buffer[1024];
	dword n;
	if(hOutputRead && PeekNamedPipe(hOutputRead, NULL, 0, NULL, &n, NULL) && n &&
	   ReadFile(hOutputRead, buffer, sizeof(buffer), &n, NULL) && n)
		reso.Cat(buffer, (int)n);

	if(hErrorRead && PeekNamedPipe(hErrorRead, NULL, 0, NULL, &n, NULL) && n &&
	   ReadFile(hErrorRead, buffer, sizeof(buffer), &n, NULL) && n)
		rese.Cat(buffer, (int)n);

	if(convertcharset) {
		reso = FromOEMCharset(reso);
		rese = FromOEMCharset(rese);
	}
	
	return reso.GetCount() || rese.GetCount() || was_running;
#endif
#ifdef PLATFORM_POSIX
	String res[2];
	bool was_running = IsRunning() || wpipe[0] >= 0 || epipe[0] >= 0;
	for (int wp=0; wp<2;wp++) {
		int *pipe = wp ? epipe : wpipe;
		if (pipe[0] < 0)
			continue;
		
		fd_set set[1];
		FD_ZERO(set);
		FD_SET(pipe[0], set);
		timeval tval = { 0, 0 };
		int sv;
		while((sv = select(pipe[0]+1, set, NULL, NULL, &tval)) > 0) {
			char buffer[1024];
			int done = read(pipe[0], buffer, sizeof(buffer));
			
			if(done > 0)
				res[wp].Cat(buffer, done);
			else if (done == 0) {
				close(pipe[0]);
				pipe[0] = -1;
			}
		}
	}
	if(convertcharset) {
		reso << FromSystemCharset(res[0]);
		rese << FromSystemCharset(res[1]);
	} else {
		reso << res[0];
		rese << res[1];
	}
	return !IsNull(res[0]) || !IsNull(res[1]) || was_running;
#endif
}

void LocalProcess2::Write(String s) {
	if(convertcharset)
		s = ToSystemCharset(s);
#ifdef PLATFORM_WIN32
	if (hInputWrite) {
		bool ret = true;
		dword n;
		for(int wn = 0; ret && wn < s.GetLength(); wn += n) {
			ret = WriteFile(hInputWrite, ~s + wn, (DWORD)s.GetLength(), &n, NULL);
			String ho = wreso;
			String he = wrese;
			wreso = wrese = Null;
			Read2(wreso, wrese);
			wreso = ho + wreso;
			wrese = he + wrese;
		}
	}
#endif
#ifdef PLATFORM_POSIX
	if (rpipe[1] >= 0) {
		int ret=1;
		for(int wn = 0; (ret > 0 || errno == EINTR) && wn < s.GetLength(); wn += ret) {
			String ho = wreso;
			String he = wrese;
			wreso = wrese = Null;
			Read2(wreso, wrese);
			wreso = ho + wreso;
			wrese = he + wrese;
			ret = write(rpipe[1], ~s + wn, size_t(s.GetLength() - wn));
		}
	}
#endif
}

void LocalProcess2::CloseRead() {
#ifdef PLATFORM_WIN32
	if(hOutputRead) {
		CloseHandle(hOutputRead);
		hOutputRead = NULL;
	}
#endif
#ifdef PLATFORM_POSIX
	if (wpipe[0] >= 0) {
		close(wpipe[0]);
		wpipe[0]=-1;
	}
#endif
}

void LocalProcess2::CloseWrite() {
#ifdef PLATFORM_WIN32
	if(hInputWrite) {
		CloseHandle(hInputWrite);
		hInputWrite = NULL;
	}
#endif
#ifdef PLATFORM_POSIX
	if (rpipe[1] >= 0) {
		close(rpipe[1]);
		rpipe[1]=-1;
	}
#endif
}

int LocalProcess2::Finish(String& out) {
	out.Clear();
	while(IsRunning()) {
		String h = Get();
		if(IsNull(h))
			Sleep(1); // p.Wait would be much better here!
		else
			out.Cat(h);
	}
	for(;;) {
		String h = Get();
		if(h.IsVoid())
			break;
		out.Cat(h);
	}
	return GetExitCode();
}

#ifdef PLATFORM_WIN32
#include <tlhelp32.h>


void PauseChildThreads(DWORD dwProcessId, bool paused) {
	HANDLE hThSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if(hThSnap == INVALID_HANDLE_VALUE)
		return; 
		
    THREADENTRY32 thEntry; 
    thEntry.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hThSnap, &thEntry)) {
        CloseHandle(hThSnap);
        return;
    }
	
    do {
        if (thEntry.th32OwnerProcessID == dwProcessId || thEntry.th32ThreadID == dwProcessId) {
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, thEntry.th32ThreadID);
           	if (paused)
            	SuspendThread(hThread);
           	else
           		ResumeThread(hThread);
            CloseHandle(hThread);
        }
    } while(Thread32Next(hThSnap, &thEntry));

    CloseHandle(hThSnap);
}

void LocalProcess2::Pause() {
	if (!IsRunning())
		return;
	
	paused = !paused;
	
	Vector<DWORD> children = GetChildProcessList(dwProcessId);
	for (int i = 0; i < children.GetCount(); ++i)
		PauseChildThreads(children[i], paused);
    PauseChildThreads(dwProcessId, paused);
}

static bool GetProcessMemoryUsage(DWORD dwProcessId, uint64 &total, uint64 &ram, uint64 &faults) {
	HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
	if (!handle)
		return false;
	
    PROCESS_MEMORY_COUNTERS_EX pmc;
    bool ret;
    
    if ((ret = GetProcessMemoryInfo(handle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))) {
        total  = pmc.PrivateUsage; 
        ram    = pmc.WorkingSetSize; 
        faults = pmc.PageFaultCount;
        
        total = max(total, ram);
    }   
    CloseHandle(handle);
    return ret;
}

bool LocalProcess2::GetMemory(uint64 &total, uint64 &ram, int &paging_s) {
	if (!IsRunning())
		return false;	
	
	total = ram = 0;
	uint64 faults = 0;
	Vector<DWORD> children = GetChildProcessList(dwProcessId);
	for (const DWORD &child : children) {
		uint64 t, r, f;
		if (GetProcessMemoryUsage(child, t, r, f)) {
			total += t;
			ram += r;
			faults += f;
		}
	}
    Time actual = GetSysTime();
    int delta = actual - timeFaultsPrev;
	if (IsNull(timeFaultsPrev) || delta == 0)
		paging_s = 0;
	else if (faults >= faultsPrev)
        paging_s = int((faults - faultsPrev)/delta);
	else
		paging_s = int((faults + (0xFFFFFFFF - faultsPrev) + 1)/delta);
    timeFaultsPrev = actual;
	faultsPrev = faults;
	
	return true;
}

#endif

#ifdef PLATFORM_POSIX

static bool ParseKbField(const String& content, const char *key, int64& out) {
    int pos = content.Find(key);
    if(pos < 0)
        return false;
    pos += (int)strlen(key);
    while(pos < content.GetLength() && (content[pos] == ' ' || content[pos] == '\t'))
        pos++;
    out = StrInt64(content.Mid(pos));
    return true;
}

static bool GetProcessMemoryUsage(pid_t pid, uint64& total, uint64& ram, uint64& faults) {
    String path = Format("/proc/%d/smaps_rollup", (int)pid);
    String content = LoadFile(path);

    int64 rss_kb = 0, priv_clean_kb = 0, priv_dirty_kb = 0;
    bool have_smaps = !content.IsEmpty();

    if (have_smaps) {
        ParseKbField(content, "Rss:", rss_kb);
        ParseKbField(content, "Private_Clean:", priv_clean_kb);
        ParseKbField(content, "Private_Dirty:", priv_dirty_kb);
    } else {
        path = Format("/proc/%d/status", (int)pid);
        content = LoadFile(path);
        if(content.IsEmpty())
            return false;
        ParseKbField(content, "VmRSS:", rss_kb);
    }

    ram   = (uint64)rss_kb * 1024ULL;
    total = (uint64)(priv_clean_kb + priv_dirty_kb) * 1024ULL;
	total = max(total, ram);
	
    path = Format("/proc/%d/stat", (int)pid);
    content = LoadFile(path);
    faults = 0;
    if(content.IsEmpty())
        return have_smaps;

    int rparen = content.ReverseFind(')');
    if (rparen < 0)
        return have_smaps;

    String rest = content.Mid(rparen + 2);
    Vector<String> f = Split(rest, ' ', false);
    
    if (f.GetCount() >= 10) {
        uint64 minflt = (uint64)StrInt64(f[7]);
        uint64 majflt = (uint64)StrInt64(f[9]);
        faults = minflt + majflt;
    }
    return true;
}

bool LocalProcess2::GetMemory(uint64& total, uint64& ram, int& paging_s) {
    if(!IsRunning())
        return false;

    total = ram = 0;
    uint64 faultsC = 0;

    Vector<pid_t> children = GetChildProcessList(pid);
    for (pid_t cid : children) {
        uint64 t, r, f;
        if (GetProcessMemoryUsage(cid, t, r, f)) {
            total += t;
            ram   += r;
            faultsC += f;
        }
    }

    Time actual = GetSysTime();
	if (IsNull(timeFaultsPrev))
		paging_s = 0;
	else {
		int delta = actual - timeFaultsPrev;
        paging_s = delta > 0 ? max(0, int((faultsC - faultsPrev)/delta)) : 0;
	}	
    timeFaultsPrev = actual;
    faultsPrev = faultsC;

    return true;
}

static bool SignalProcessTree(pid_t pid, int sig) {
    bool ok = true;
    if (kill(pid, sig) != 0)
        ok = false;

    Vector<pid_t> to_visit;
    to_visit << pid;

    int init = 0;
    while(init < to_visit.size()) {
        int count = to_visit.size();
        for (int i = init; i < count; i++) {
            Vector<pid_t> kids;
            GetDirectChildren(to_visit[i], kids);
            for (pid_t k : kids) {
                if (kill(k, sig) != 0) 
                    ok = false;
                to_visit << k;
            }
        }
        init = count;
    }
    return ok;
}

void LocalProcess2::Pause() {
	if (!IsRunning())
		return;
	
	paused = !paused;	
	
	if (paused)
		SignalProcessTree(pid, SIGCONT);
	else
		SignalProcessTree(pid, SIGSTOP);
}

#endif

}
