/*
 * Copyright (c) 2014 LastPass. All Rights Reserved.
 *
 * 
 */

#include "process.h"
#include "util.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/resource.h>

#if defined(__linux__)
#include <sys/prctl.h>
#elif defined(__APPLE__) && defined(__MACH__)
#include <libproc.h>
#include <sys/ptrace.h>
#endif

void process_set_name(const char *name)
{
#if defined(__linux__)
	prctl(PR_SET_NAME, name);
#endif

	if (!ARGC || !ARGV)
		return;

	for (int i = 0; i < ARGC; ++i) {
		for (char *p = ARGV[i]; *p; ++p)
			*p = '\0';
	}

	strcpy(ARGV[0], name);
}

#if defined(__linux__) || defined(__CYGWIN__)
bool process_is_same_executable(pid_t pid)
{
	_cleanup_free_ char *proc = NULL;
	char resolved_them[PATH_MAX + 1] = { 0 }, resolved_me[PATH_MAX + 1] = { 0 };

	xasprintf(&proc, "/proc/%lu/exe", (unsigned long)pid);
	if (readlink(proc, resolved_them, PATH_MAX) < 0 || readlink("/proc/self/exe", resolved_me, PATH_MAX) < 0)
		return false;
	if (strcmp(resolved_them, resolved_me))
		return false;
	return true;

}

void process_disable_ptrace(void)
{
#if defined(__linux__)
	prctl(PR_SET_DUMPABLE, 0);
#endif
	struct rlimit limit = { 0, 0 };
	setrlimit(RLIMIT_CORE, &limit);
}
#elif defined(__APPLE__) && defined(__MACH__)
bool process_is_same_executable(pid_t pid)
{
	char resolved_them[PROC_PIDPATHINFO_MAXSIZE], resolved_me[PROC_PIDPATHINFO_MAXSIZE];

	if (proc_pidpath(pid, resolved_them, sizeof(resolved_them)) <= 0 || proc_pidpath(getpid(), resolved_me, sizeof(resolved_me)) <= 0)
		return false;
	if (strcmp(resolved_them, resolved_me))
		return false;
	return true;
}

void process_disable_ptrace(void)
{
	ptrace(PT_DENY_ATTACH, 0, 0, 0);
	struct rlimit limit = { 0, 0 };
	setrlimit(RLIMIT_CORE, &limit);
}
#endif
