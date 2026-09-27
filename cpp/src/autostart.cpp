#include "autostart.h"
#include "util.h"
#include <cstdio>
#include <windows.h>
#include <taskschd.h>

namespace {

// COM for this call only (the main thread doesn't keep it open)
struct Com {
    HRESULT hr;
    Com() : hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~Com() { if (SUCCEEDED(hr)) CoUninitialize(); }
};

template <class T> struct Ptr {
    T* p = nullptr;
    ~Ptr() { if (p) p->Release(); }
    T** operator&() { return &p; }
    T* operator->() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};

struct Bstr {
    BSTR b;
    explicit Bstr(const std::wstring& s) : b(SysAllocString(s.c_str())) {}
    ~Bstr() { SysFreeString(b); }
};

std::wstring UserId() { return util::EnvVar(L"USERDOMAIN") + L"\\" + util::EnvVar(L"USERNAME"); }

// one task per Windows user, so each person on the PC can choose for themselves
std::wstring TaskName() { return L"Project OptM (" + util::EnvVar(L"USERNAME") + L")"; }

std::wstring XmlEscape(const std::wstring& s) {
    std::wstring o;
    for (wchar_t c : s) {
        if (c == L'&') o += L"&amp;";
        else if (c == L'<') o += L"&lt;";
        else if (c == L'>') o += L"&gt;";
        else o += c;
    }
    return o;
}

std::wstring XmlUnescape(std::wstring s) {
    for (auto [from, to] : { std::pair<const wchar_t*, const wchar_t*>{ L"&lt;", L"<" }, { L"&gt;", L">" }, { L"&amp;", L"&" } })
        for (size_t p; (p = s.find(from)) != std::wstring::npos;) s.replace(p, wcslen(from), to);
    return s;
}

std::string Refused(HRESULT hr) {
    if (hr == E_ACCESSDENIED) return "needs admin rights";
    char b[80]; snprintf(b, sizeof(b), "Task Scheduler refused it (0x%08lX)", (unsigned long)hr);
    return b;
}

bool RootFolder(Ptr<ITaskService>& svc, Ptr<ITaskFolder>& folder) {
    if (FAILED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&svc))) return false;
    VARIANT none; VariantInit(&none);
    if (FAILED(svc->Connect(none, none, none, none))) return false;
    Bstr root(L"\\");
    return SUCCEEDED(svc->GetFolder(root.b, &folder));
}

}  // namespace

namespace autostart {

State Query() {
    State st;
    Com com;
    Ptr<ITaskService> svc; Ptr<ITaskFolder> folder; Ptr<IRegisteredTask> task;
    if (!RootFolder(svc, folder)) return st;
    Bstr name(TaskName());
    if (FAILED(folder->GetTask(name.b, &task))) return st;
    st.exists = true;
    BSTR xml = nullptr;
    if (SUCCEEDED(task->get_Xml(&xml)) && xml) {
        std::wstring x = xml;
        SysFreeString(xml);
        st.atSignIn = x.find(L"<LogonTrigger") != std::wstring::npos;
        size_t a = x.find(L"<Command>"), b = x.find(L"</Command>");
        if (a != std::wstring::npos && b != std::wstring::npos && b > a) st.exe = XmlUnescape(x.substr(a + 9, b - a - 9));
    }
    return st;
}

bool Set(const std::wstring& exe, bool atSignIn, std::string& error) {
    Com com;
    Ptr<ITaskService> svc; Ptr<ITaskFolder> folder; Ptr<IRegisteredTask> task;
    if (!RootFolder(svc, folder)) { error = "couldn't reach Task Scheduler"; return false; }
    std::wstring user = XmlEscape(UserId());
    std::wstring xml =
        L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>"
        L"<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">"
        L"<RegistrationInfo><Author>Project OptM</Author>"
        L"<Description>Lets Project OptM start as administrator without the Windows prompt"
        + std::wstring(atSignIn ? L", and starts it in the tray when you sign in" : L"") +
        L". Change it in Project OptM: Settings.</Description></RegistrationInfo>"
        + (atSignIn ? L"<Triggers><LogonTrigger><Enabled>true</Enabled><UserId>" + user + L"</UserId><Delay>PT10S</Delay></LogonTrigger></Triggers>"
                    : std::wstring(L"<Triggers/>")) +
        L"<Principals><Principal id=\"Author\"><UserId>" + user + L"</UserId>"
        L"<LogonType>InteractiveToken</LogonType><RunLevel>HighestAvailable</RunLevel></Principal></Principals>"
        L"<Settings>"
        L"<MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>"
        L"<DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>"
        L"<StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>"
        L"<AllowHardTerminate>true</AllowHardTerminate>"
        L"<StartWhenAvailable>false</StartWhenAvailable>"
        L"<RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>"
        L"<IdleSettings><StopOnIdleEnd>false</StopOnIdleEnd><RestartOnIdle>false</RestartOnIdle></IdleSettings>"
        L"<AllowStartOnDemand>true</AllowStartOnDemand>"
        L"<Enabled>true</Enabled><Hidden>false</Hidden><RunOnlyIfIdle>false</RunOnlyIfIdle>"
        L"<ExecutionTimeLimit>PT0S</ExecutionTimeLimit>"   // runs for as long as you're signed in
        L"<Priority>5</Priority>"                          // normal priority (the default, 7, is below normal)
        L"</Settings>"
        L"<Actions Context=\"Author\"><Exec><Command>" + XmlEscape(exe) + L"</Command><Arguments>--tray</Arguments></Exec></Actions>"
        L"</Task>";
    Bstr name(TaskName()), text(xml);
    VARIANT none; VariantInit(&none);
    HRESULT hr = folder->RegisterTask(name.b, text.b, TASK_CREATE_OR_UPDATE, none, none, TASK_LOGON_INTERACTIVE_TOKEN, none, &task);
    if (FAILED(hr)) { error = Refused(hr); return false; }
    return true;
}

bool Remove(std::string& error) {
    Com com;
    Ptr<ITaskService> svc; Ptr<ITaskFolder> folder;
    if (!RootFolder(svc, folder)) { error = "couldn't reach Task Scheduler"; return false; }
    Bstr name(TaskName());
    HRESULT hr = folder->DeleteTask(name.b, 0);
    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) { error = Refused(hr); return false; }
    return true;
}

bool Run() {
    Com com;
    Ptr<ITaskService> svc; Ptr<ITaskFolder> folder; Ptr<IRegisteredTask> task; Ptr<IRunningTask> running;
    if (!RootFolder(svc, folder)) return false;
    Bstr name(TaskName());
    if (FAILED(folder->GetTask(name.b, &task))) return false;
    VARIANT none; VariantInit(&none);
    return SUCCEEDED(task->Run(none, &running));
}

}  // namespace autostart
