#pragma once

#include <string>
#include <array>
#include <vector>
#include <tuple>

bool check_registry();
bool check_registry_keys();
bool check_fs_artifacts();
bool check_running_procs();
bool check_vm_vendor();
bool has_hypervisor();

// WMI QUERY
std::tuple<IWbemServices*, IWbemLocator*> init_wql();
bool query_msvm(IWbemServices* pSvc, IWbemLocator* pLoc);
void cleanup_wql(IWbemServices* pSvc, IWbemLocator* pLoc);

std::vector<std::tuple<HKEY, std::string, std::string, std::string>> registry_keys_value_artifacts = {
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\Scsi\Scsi Port 0\Scsi Bus 0\Target Id 0\Logical Unit Id 0)", "Identifier", "VMWARE"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\Scsi\Scsi Port 0\Scsi Bus 0\Target Id 0\Logical Unit Id 0)", "Identifier", "VBOX"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\Scsi\Scsi Port 0\Scsi Bus 0\Target Id 0\Logical Unit Id 0)", "Identifier", "QEMU"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\Scsi\Scsi Port 1\Scsi Bus 0\Target Id 0\Logical Unit Id 0)", "Identifier", "VMWARE"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\DEVICEMAP\Scsi\Scsi Port 2\Scsi Bus 0\Target Id 0\Logical Unit Id 0)", "Identifier", "VMWARE"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\Description\System)", "SystemBiosVersion", "VMWARE"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\Description\System)", "SystemBiosVersion", "VBOX"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\Description\System)", "SystemBiosVersion", "QEMU"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\Description\System)", "VideoBiosVersion", "VIRTUALBOX"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\Description\System)", "SystemBiosDate", "06/23/99"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Control\SystemInformation)", "SystemManufacturer", "VMWARE"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Control\SystemInformation)", "SystemProductName", "VMWARE"},
};

std::vector<std::tuple<HKEY, std::string>> registry_keys_artifacts = {
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\ACPI\DSDT\VBOX__)"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\ACPI\FADT\VBOX__)"},
    {HKEY_LOCAL_MACHINE, R"(HARDWARE\ACPI\RSDT\VBOX__)"},
    {HKEY_LOCAL_MACHINE, R"(SOFTWARE\Oracle\VirtualBox Guest Additions)"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Services\VBoxGuest)"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Services\VBoxMouse)"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Services\VBoxService)"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Services\VBoxSF)"},
    {HKEY_LOCAL_MACHINE, R"(SYSTEM\ControlSet001\Services\VBoxVideo)"},
    {HKEY_LOCAL_MACHINE, R"(SOFTWARE\VMware, Inc.\VMware Tools)"},
    {HKEY_LOCAL_MACHINE, R"(SOFTWARE\Wine)"},
    {HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Virtual Machine\Guest\Parameters)"}
};

std::vector<std::string> file_system_artifacts = {
   R"(C:\Windows\system32\drivers\VBoxMouse.sys)",
   R"(C:\Windows\system32\drivers\VBoxGuest.sys)",
   R"(C:\Windows\system32\drivers\VBoxSF.sys)",
   R"(C:\Windows\system32\drivers\VBoxVideo.sys)",
   R"(C:\Windows\system32\vboxdisp.dll)",
   R"(C:\Windows\system32\vboxhook.dll)",
   R"(C:\Windows\system32\vboxmrxnp.dll)",
   R"(C:\Windows\system32\vboxogl.dll)",
   R"(C:\Windows\system32\vboxoglarrayspu.dll)",
   R"(C:\Windows\system32\vboxoglcrutil.dll)",
   R"(C:\Windows\system32\vboxoglerrorspu.dll)",
   R"(C:\Windows\system32\vboxoglfeedbackspu.dll)",
   R"(C:\Windows\system32\vboxoglpackspu.dll)",
   R"(C:\Windows\system32\vboxoglpassthroughspu.dll)",
   R"(C:\Windows\system32\vboxservice.exe)",
   R"(C:\Windows\system32\vboxtray.exe)",
   R"(C:\Windows\system32\VBoxControl.exe)",
   R"(C:\Windows\system32\drivers\vmmouse.sys)",
   R"(C:\Windows\system32\drivers\vmhgfs.sys)",
   R"(C:\Windows\system32\drivers\vm3dmp.sys)",
   R"(C:\Windows\system32\drivers\vmhgfs.sys)",
   R"(C:\Windows\system32\drivers\vmmemctl.sys)",
   R"(C:\Windows\system32\drivers\vmmouse.sys)",
   R"(C:\Windows\system32\drivers\vmrawdsk.sys)",
   R"(C:\Windows\system32\drivers\vmusbmouse.sys)",
};

std::vector<std::string> target_processes = {
    "vboxservice.exe",
    "vboxtray.exe",
    "vmtoolsd.exe",
    "vmwaretray.exe",
    "vmwareuser.exe",
    "VGAuthService.exe",
    "vmacthlp.exe",
    "vmsrvc.exe",
    "vmusrvc.exe",
    "prl_cc.exe",
    "prl_tools.exe",
    "xenservice.exe",
    "qemu-ga.exe",
};

std::vector<std::string> vendor_artifacts = {
    "VMwareVMware",
    "Microsoft Hv",
    "KVMKVMKVM",
    "XenVMMXenVMM",
    "VBoxVBoxVBox"
};