// Fill out your copyright notice in the Description page of Project Settings.


#include "ReadSerialMotSensorLibrary.h"
#include "comserial.h"

//#pragma optimize("", off)

#define _CRT_SECURE_NO_WARNINGS

// ----------------------------------------------------------------------------

class SCameraAngle
{
	float prevx, prevy, prevz;
	bool init;

public:
	SCameraAngle()
	{
		init = false;
		prevx = prevy = prevz = 0;
	}

	void Reset()
	{
		init = false;
	}

	bool IsInitialized() const
	{
		return init;
	}

	void MarkedAsInitialized()
	{
		init = true;
	}

	float getX() const
	{
		return prevx;
	}

	float getY() const
	{
		return prevy;
	}

	float getZ() const
	{
		return prevz;
	}

	void SetCurrentAngles(float x, float y, float z)
	{
		prevx = x;
		prevy = y;
		prevz = z;
	}
};

static SCameraAngle gs_cameraAngle;

// ----------------------------------------------------------------------------

float delta_angle(float current, float previous) {
	float delta = current - previous;
	delta = fmodf(delta + 180.0f, 360.0f);
	if (delta < 0.f)
		delta += 360.0f;
	delta -= 180.0f;
	return delta;
}

// ----------------------------------------------------------------------------

void UReadSerialMotSensorLibrary::ZeroSerialData()
{
	gs_cameraAngle.Reset();
}

// ----------------------------------------------------------------------------

FString UReadSerialMotSensorLibrary::ProcessSerialData(bool &success, float& dx, float& dy, float& dz)
{
	UComSerial* pComSerial = UComSerial::GetInstance();

	FString serialString = pComSerial->Readln(success);

	if (!success)
	{
		return "";
	}

	if ((serialString.Len() < 2) || (serialString[0] != 'R' && serialString[1] != ':'))
	{
		success = false;
		return "";
	}

	// extract angles from string
	// string comes in this format -> R:xxx,yyy,zzz
	serialString.RemoveAt(0, 2, EAllowShrinking::Yes);	 // removes R:
	char const * pStr = TCHAR_TO_ANSI(*serialString);	
	char * pDupStr = _strdup(pStr);
	void * pDupStrOrg = pDupStr;
	char const * pDelim = ",\0";
	char * next_token = NULL;							// Context pointer for strtok_s

	float x, y, z;
	char * pAngle = strtok_s(pDupStr, pDelim, &next_token);
	x = atof(pAngle);

	pAngle = strtok_s(nullptr, pDelim, &next_token);
	y = atof(pAngle);

	pAngle = strtok_s(nullptr, pDelim, &next_token);
	z = atof(pAngle);
	free(pDupStrOrg);

	// compute delta from previous reading
	if (!gs_cameraAngle.IsInitialized())
	{
		dx = dy = dz = 0.0f;
		gs_cameraAngle.MarkedAsInitialized();
	}
	else
	{
		dx = delta_angle(x, gs_cameraAngle.getX());
		dy = delta_angle(y, gs_cameraAngle.getY());
		dz = delta_angle(z, gs_cameraAngle.getZ());
	}

	gs_cameraAngle.SetCurrentAngles(x, y, z);
	
	return serialString;
}
