#include <stdlib.h>
#include <Windows.h>
#include <ShellApi.h>

//
// Execute a command and get the results. (Only standard output)
//
int shellExec(const char *cmd) {
		SHELLEXECUTEINFO ShExecInfo = {0};
		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo.hwnd = NULL;
		ShExecInfo.lpVerb = NULL;
		ShExecInfo.lpFile = "cmd.exe";
		
		ShExecInfo.lpParameters = cmd;
		ShExecInfo.nShow = SW_HIDE;
		ShExecInfo.hInstApp = NULL;
		ShellExecuteEx(&ShExecInfo);
		return 0;
}

int main(int argc, char **argv)
{
	shellExec("/C bin\\chipmachine.exe -f");
	return 0;
}
