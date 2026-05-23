#include "ComSerial.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include "Windows/HideWindowsPlatformTypes.h"

#define BOOL2bool(B) B == 0 ? false : true

UComSerial* UComSerial::ms_pComSerial = nullptr;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

UComSerial::UComSerial()
	: WriteLineEnd(EComLineEnd::n)
	, m_hIDComDev(nullptr)
	, m_OverlappedRead(nullptr)
	, m_OverlappedWrite(nullptr)
	, m_Port(-1)
	, m_Baud(-1)
{
	// Allocate the OVERLAPPED structs
	m_OverlappedRead = new OVERLAPPED();
	m_OverlappedWrite = new OVERLAPPED();

	FMemory::Memset(m_OverlappedRead, 0, sizeof(OVERLAPPED));
	FMemory::Memset(m_OverlappedWrite, 0, sizeof(OVERLAPPED));

	ms_pComSerial = this;
}

// ----------------------------------------------------------------------------

UComSerial::~UComSerial()
{
	//Close();

	if (m_hIDComDev != nullptr)
	{
		Close();
	}

	ms_pComSerial = nullptr;

	// Delete allocated OVERLAPPED structs
	delete m_OverlappedRead;
	delete m_OverlappedWrite;
}

// ----------------------------------------------------------------------------

UComSerial* UComSerial::OpenComPort(bool& bOpened, int32 Port, int32 BaudRate)
{
	UComSerial* Serial = NewObject<UComSerial>();
	bOpened = Serial->OpenWFC(Port, BaudRate);
	return Serial;
}

// ----------------------------------------------------------------------------

void UComSerial::Close()
{
	if (!m_hIDComDev) return;

	if (m_OverlappedRead->hEvent != NULL) CloseHandle(m_OverlappedRead->hEvent);
	if (m_OverlappedWrite->hEvent != NULL) CloseHandle(m_OverlappedWrite->hEvent);
	CloseHandle(m_hIDComDev);
	m_hIDComDev = NULL;

	RemoveFromRoot();
}

// ----------------------------------------------------------------------------

FString UComSerial::Readln(bool& bSuccess)
{
	return ReadStringUntil(bSuccess, '\n');
}

// ----------------------------------------------------------------------------

FString UComSerial::ReadStringUntil(bool& bSuccess, uint8 Terminator)
{
	bSuccess = false;
	if (!m_hIDComDev) return TEXT("");

	TArray<uint8> Chars;
	uint8 Byte = 0x0;
	bool bReadStatus;
	unsigned long dwBytesRead, dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError(m_hIDComDev, &dwErrorFlags, &ComStat);
	if (!ComStat.cbInQue) return TEXT("");

	do {
		bReadStatus = BOOL2bool(ReadFile(
			m_hIDComDev,
			&Byte,
			1,
			&dwBytesRead,
			nullptr)); // m_OverlappedRead));

		if (!bReadStatus)
		{
			int lastError = GetLastError();

			if (lastError == ERROR_IO_PENDING)
			{
// 				WaitForSingleObject(m_OverlappedRead->hEvent, 2000);
// 
// 				DWORD dwBytesTransferred;
// 				GetOverlappedResult(m_hIDComDev, &m_OverlappedRead, &dwBytesRead, FALSE); // Don't wait again

				continue;
			}
			else
			{
				Chars.Add(0x0);
				break;
			}
		}

		if (Byte == Terminator || dwBytesRead == 0)
		{
			// when Terminator is \n, we know we're expecting lines from Arduino. But those
			// are ended in \r\n. That means that if we found the line Terminator (\n), our previous
			// character could be \r. If it is, we remove that from the array.
			if (Chars.Num() > 0 && Terminator == '\n' && Chars.Top() == '\r') Chars.Pop(EAllowShrinking::Yes);

			Chars.Add(0x0);
			break;
		}
		else Chars.Add(Byte);

	} while (Byte != 0x0 && Byte != Terminator);

	bSuccess = true;
	FString result = FString(reinterpret_cast<const char*>(Chars.GetData()), Chars.Num());

	return result;
}

// ----------------------------------------------------------------------------

bool UComSerial::OpenWFC(int32 nPort, int32 nBaud, bool bDTR, bool bRTS)
{
	if (nPort < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid port number: %d"), nPort);
		return false;
	}
	if (m_hIDComDev)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trying to use opened Serial instance to open a new one. "
			"Current open instance port: %d | Port tried: %d"), m_Port, nPort);
		return false;
	}

	FString szPort;
	if (nPort < 10)
		szPort = FString::Printf(TEXT("COM%d"), nPort);
	else
		szPort = FString::Printf(TEXT("\\\\.\\COM%d"), nPort);
	DCB dcb;

	//m_hIDComDev = CreateFile(*szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	m_hIDComDev = CreateFile(*szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hIDComDev == NULL)
	{
		unsigned long dwError = GetLastError();
		UE_LOG(LogTemp, Error, TEXT("Failed to open port COM%d (%s). Error: %08X"), nPort, *szPort, dwError);
		return false;
	}

	FMemory::Memset(m_OverlappedRead, 0, sizeof(OVERLAPPED));
	FMemory::Memset(m_OverlappedWrite, 0, sizeof(OVERLAPPED));

	COMMTIMEOUTS CommTimeOuts;
	//CommTimeOuts.ReadIntervalTimeout = 10;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 10;
	SetCommTimeouts(m_hIDComDev, &CommTimeOuts);

	m_OverlappedRead->hEvent = CreateEvent(NULL, true, false, NULL);
	m_OverlappedWrite->hEvent = CreateEvent(NULL, true, false, NULL);

	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hIDComDev, &dcb);
	dcb.BaudRate = nBaud;
	dcb.ByteSize = 8;
	if (bDTR == true)
	{
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
	}
	else
	{
		dcb.fDtrControl = DTR_CONTROL_DISABLE;
	}

	if (bRTS == true)
	{
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	}
	else
	{
		dcb.fRtsControl = RTS_CONTROL_DISABLE;
	}

	if (!SetCommState(m_hIDComDev, &dcb) ||
		!SetupComm(m_hIDComDev, 10000, 10000) ||
		m_OverlappedRead->hEvent == NULL ||
		m_OverlappedWrite->hEvent == NULL)
	{
		unsigned long dwError = GetLastError();
		if (m_OverlappedRead->hEvent != NULL) CloseHandle(m_OverlappedRead->hEvent);
		if (m_OverlappedWrite->hEvent != NULL) CloseHandle(m_OverlappedWrite->hEvent);
		CloseHandle(m_hIDComDev);
		m_hIDComDev = NULL;
		UE_LOG(LogTemp, Error, TEXT("Failed to setup port COM%d. Error: %08X"), nPort, dwError);
		return false;
	}

	//FPlatformProcess::Sleep(0.05f);
	AddToRoot();
	m_Port = nPort;
	m_Baud = nBaud;
	return true;
}

// ----------------------------------------------------------------------------

bool UComSerial::WriteBytes(TArray<uint8> Buffer)
{
	if (!m_hIDComDev) false;

	bool bWriteStat;
	unsigned long dwBytesWritten;

	bWriteStat = BOOL2bool(WriteFile(m_hIDComDev, Buffer.GetData(), Buffer.Num(), &dwBytesWritten, m_OverlappedWrite));
	if (!bWriteStat && (GetLastError() == ERROR_IO_PENDING))
	{
		if (WaitForSingleObject(m_OverlappedWrite->hEvent, 1000))
		{
			dwBytesWritten = 0;
			return false;
		}
		else
		{
			GetOverlappedResult(m_hIDComDev, m_OverlappedWrite, &dwBytesWritten, false);
			m_OverlappedWrite->Offset += dwBytesWritten;
			return true;
		}
	}

	return true;
}

// ----------------------------------------------------------------------------

FString UComSerial::LineEndToStr(EComLineEnd LineEnd)
{
	switch (LineEnd)
	{
	case EComLineEnd::rn:
		return TEXT("\r\n");
	case EComLineEnd::n:
		return TEXT("\n");
	case EComLineEnd::r:
		return TEXT("\r");
	case EComLineEnd::nr:
		return TEXT("\n\r");
	default:
		return TEXT("null");
	}
}

// ----------------------------------------------------------------------------

bool UComSerial::Print(FString String)
{
	auto Convert = FTCHARToUTF8(*String);
	TArray<uint8> Data;
	Data.Append((uint8*)Convert.Get(), Convert.Length());

	return WriteBytes(Data);
}

// ----------------------------------------------------------------------------

bool UComSerial::Println(FString String)
{
	return Print(String + LineEndToStr(WriteLineEnd));
}

// ----------------------------------------------------------------------------

TArray<uint8> UComSerial::ReadBytes(int32 Limit)
{
	TArray<uint8> Data;

	if (!m_hIDComDev) return Data;

	Data.Empty(Limit);

	uint8* Buffer = new uint8[Limit];
	bool bReadStatus;
	unsigned long dwBytesRead, dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError(m_hIDComDev, &dwErrorFlags, &ComStat);
	if (!ComStat.cbInQue) return Data;

	bReadStatus = BOOL2bool(ReadFile(
		m_hIDComDev,
		Buffer,
		Limit,
		&dwBytesRead,
		m_OverlappedRead));

	if (!bReadStatus)
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			WaitForSingleObject(m_OverlappedRead->hEvent, 2000);
		}
		else
		{
			return Data;
		}
	}

	Data.Append(Buffer, dwBytesRead);
	return Data;
}

// ----------------------------------------------------------------------------

void UComSerial::Flush()
{
	if (!m_hIDComDev) return;

	TArray<uint8> Data;

	do {
		Data = ReadBytes(8192);
	} while (Data.Num() > 0);
}

// ----------------------------------------------------------------------------

UComSerial* UComSerial::GetInstance()
{
	return ms_pComSerial;
}
