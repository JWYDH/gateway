#pragma once
#include "../stdafx.h"


struct Buffer{
	
public:
	Buffer(int default_size = 1024);
	virtual ~Buffer();

	char *MemPtr();
	char *EndPtr();
	char *DataEndPtr();
	char *OffsetPtr();
	
	int32_t Capacity();
	int32_t AvaliableCapacity();
	int32_t Length();
	int32_t AvaliableLength();

	void SetLength(int32_t length);
	void Seek(int32_t n);
	void AdjustOffset(int32_t adjust_offset);

	int32_t ResetSize(int32_t size);
	int32_t ReadBuf(char* buf, int32_t len);
	int32_t WriteBuff(const char* buf, int32_t len);
private:
	char *mem_start_ptr_;
	char *mem_end_ptr_;
	char *data_end_ptr_;
	char *offset_ptr_;
};
