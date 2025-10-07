#pragma once

class UEngineStatics
{
public:
	static uint32 GenUUID()
	{
		return NextUUID++;
	}
	static void ResetUUID()
	{
		NextUUID = 0;
	}
private:
	static uint32 NextUUID;
};
