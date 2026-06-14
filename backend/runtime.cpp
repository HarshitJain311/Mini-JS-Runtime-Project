#include "runtime.h"

#include <array>
#include <cstdio>
#include <string>

#if defined(_WIN32)
  #include <windows.h>
  #include <vector>

  static std::string readAllFromPipeWin(HANDLE handle) {
      std::string out;
      DWORD bytesRead = 0;
      char buffer[4096];
      while (true) {
          BOOL ok = ReadFile(handle, buffer, sizeof(buffer), &bytesRead, nullptr);
          if (!ok || bytesRead == 0) break;
          out.append(buffer, buffer + bytesRead);
      }
      return out;
  }
#endif

RuntimeResult executeNodeFile(const std::string& filePath, int timeoutMs) {
    RuntimeResult result;

#if defined(_WIN32)
    // CreateProcess requires a mutable command line buffer (we build it into cmdBuf below).
    // IMPORTANT: do not pass untrusted strings to a shell; we execute Node directly with a fixed argv format.
    std::string command = "node \"" + filePath + "\"";

    SECURITY_ATTRIBUTES saAttr{};
    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = TRUE;


    HANDLE outRead = nullptr, outWrite = nullptr;
    HANDLE errRead = nullptr, errWrite = nullptr;

    if (!CreatePipe(&outRead, &outWrite, &saAttr, 0)) {
        result.success = false;
        result.error = "Failed to create stdout pipe";
        return result;
    }
    if (!CreatePipe(&errRead, &errWrite, &saAttr, 0)) {
        CloseHandle(outRead);
        CloseHandle(outWrite);
        result.success = false;
        result.error = "Failed to create stderr pipe";
        return result;
    }

    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = outWrite;
    si.hStdError = errWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};

    std::vector<char> cmdBuf(command.begin(), command.end());
    cmdBuf.push_back('\0');

    BOOL ok = CreateProcessA(
        nullptr,
        cmdBuf.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    CloseHandle(outWrite);
    CloseHandle(errWrite);

    if (!ok) {
        CloseHandle(outRead);
        CloseHandle(errRead);
        result.success = false;
        result.error = "Failed to start Node.js process";
        return result;
    }

    DWORD wait = WaitForSingleObject(pi.hProcess, static_cast<DWORD>(timeoutMs));
    bool timedOut = (wait == WAIT_TIMEOUT);

    if (timedOut) {
        TerminateProcess(pi.hProcess, 1);
        result.success = false;
        result.error = "Execution timed out";
    }

    std::string stdoutStr = readAllFromPipeWin(outRead);
    std::string stderrStr = readAllFromPipeWin(errRead);

    CloseHandle(outRead);
    CloseHandle(errRead);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    auto merge = [&](const std::string& a, const std::string& b) {
        if (b.empty()) return a;
        if (a.empty()) return b;
        return a + "\n" + b;
    };

    if (timedOut) {
        if (!stdoutStr.empty() || !stderrStr.empty()) {
            result.error = merge(stdoutStr, stderrStr);
        }
        return result;
    }

    if (exitCode == 0) {
        result.success = true;
        result.output = merge(stdoutStr, stderrStr);
    } else {
        result.success = false;
        result.error = !stderrStr.empty() ? stderrStr : stdoutStr;
    }

    return result;

#else
    // Non-Windows fallback: timeout not enforced.
    std::string command = "node \"" + filePath + "\" 2>&1";

    std::array<char, 256> buffer;
    std::string output;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        result.success = false;
        result.error = "Failed to start Node.js process.";
        return result;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    int returnCode = pclose(pipe);

    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }

    if (returnCode == 0) {
        result.success = true;
        result.output = output;
    } else {
        result.success = false;
        result.error = output;
    }

    (void)timeoutMs;
    return result;
#endif
}

