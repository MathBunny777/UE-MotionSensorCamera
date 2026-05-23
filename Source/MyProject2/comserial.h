#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "comserial.generated.h"

// Forward declaration
typedef struct _OVERLAPPED OVERLAPPED;
							
UENUM(BlueprintType, Category = "Communication Serial")
enum class EComLineEnd : uint8
{
	rn	UMETA(DisplayName = "\r\n"),
	n	UMETA(DisplayName = "\n"),
	r	UMETA(DisplayName = "\r"),
	nr	UMETA(DisplayName = "\n\r")
};


/**
 * 
 */
UCLASS(BlueprintType, Category = "Communication Serial", meta = (Keywords = "com arduino serial arduino duino"))
class MYPROJECT2_API UComSerial : public UObject
{
	GENERATED_BODY()

public:
	/** Determines the line ending used when writing lines to serial port with PrintLine. */
	UPROPERTY(BlueprintReadWrite, Category = "Communication Serial | String")
		EComLineEnd WriteLineEnd;

public:
	UComSerial();
	~UComSerial();

	static UComSerial* GetInstance();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Open Serial Port"), Category = "Communication Serial", meta = (Keywords = "communication com COMSERIAL duino arduino serial port start open serial"))
		static UComSerial* OpenComPort(bool &bOpened, int32 Port = 1, int32 BaudRate = 9600);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Close Serial Port"), Category = "Communication Serial", meta = (Keywords = "communication com COMSERIAL duino arduino serial end finish release close port"))
		void Close();

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Open Serial Port With Target and Flow Control"), Category = "Communication Serial", meta = (Keywords = "communication com COMSERIAL duino arduino serial start init open port"))
		bool OpenWFC(int32 Port = 2, int32 BaudRate = 9600, bool DTR = true, bool RTS = true);

	/** Clean the serial port by reading everything left to be read. */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Flush Serial Port"), Category = "Communication Serial")
		void Flush();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "COMSERIAL Is Serial Port Open?"), Category = "Communication Serial")
		bool IsOpened() { return m_hIDComDev != NULL; }

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Serial Print", keywords = "communication com COMSERIAL duino arduino serial print send write string words text characters"), Category = "Communication Serial")
		bool Print(FString String);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Serial Write Bytes", keywords = "communication com COMSERIAL duino arduino serial write bytes send"), Category = "Communication Serial")
		bool WriteBytes(TArray<uint8> Buffer);

	/**
	 * Writes a string with newline (\n) appended at the end to the serial port.
	 * @param String The string to be sent to the serial port.
	 * @return True if the string was sent.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Serial Print Line", keywords = "communication com COMSERIAL duino arduino serial print line send write string words text characters"), Category = "Communication Serial")
		bool Println(FString String);

	FString Readln(bool& bSuccess);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Serial Read Bytes", keywords = "communication com COMSERIAL duino arduino serial read bytes get read receive"), Category = "Communication Serial")
		TArray<uint8> ReadBytes(int32 Limit = 256);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "COMSERIAL Serial Line End to String", keywords = "communication com COMSERIAL duino arduino serial cast convert line end to string"), Category = "Communication Serial")
		FString LineEndToStr(EComLineEnd LineEnd);

protected:
	FString ReadStringUntil(bool& bSuccess, uint8 Terminator);

	void* m_hIDComDev;

	// These are pointers to be able to use OVERLAPPED with forward declaration
	OVERLAPPED* m_OverlappedRead;
	OVERLAPPED* m_OverlappedWrite;

	int32 m_Port;
	int32 m_Baud;

	static UComSerial* ms_pComSerial;
};
