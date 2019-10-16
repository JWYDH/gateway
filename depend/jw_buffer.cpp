#include "jw_buffer.h"
#include <string.h>

Buffer::Buffer(int default_size)
: mem_start_ptr_(nullptr)
, mem_end_ptr_(nullptr)
, data_end_ptr_(nullptr)
, offset_ptr_(nullptr) {

	mem_start_ptr_ = new char[default_size];
	data_end_ptr_ = mem_start_ptr_;
	offset_ptr_ = mem_start_ptr_;
	mem_end_ptr_ = mem_start_ptr_ + default_size;
}

Buffer::~Buffer(){
	if (mem_start_ptr_)
	{
		delete[] mem_start_ptr_;
	}
}

char* Buffer::MemPtr() {
	return mem_start_ptr_;
}

char* Buffer::EndPtr() {
	return mem_end_ptr_;
}

char* Buffer::DataEndPtr() {
	return data_end_ptr_;
}

char* Buffer::OffsetPtr() {
	return offset_ptr_;
}

int32_t Buffer::Capacity() {
	return mem_end_ptr_ - mem_start_ptr_;
}

int32_t Buffer::AvaliableCapacity() {
	return mem_end_ptr_ - data_end_ptr_;
}

int32_t Buffer::Length() {
	return data_end_ptr_ - mem_start_ptr_;
}

int32_t Buffer::AvaliableLength() {
	return data_end_ptr_ - offset_ptr_;
}


void Buffer::SetLength(int32_t length)
{
	if (length > (mem_end_ptr_ - mem_start_ptr_)) {
		ResetSize(length);
	}
	data_end_ptr_ = mem_start_ptr_ + length;
	if (offset_ptr_ > data_end_ptr_)
	{
		offset_ptr_ = data_end_ptr_;
	}
}

void Buffer::Seek(int32_t n) {
	if (n > (mem_end_ptr_ - mem_start_ptr_)){
		ResetSize(n);
	}
	offset_ptr_ = mem_start_ptr_ + n;
	if (offset_ptr_ > data_end_ptr_)
	{
		data_end_ptr_ = offset_ptr_;
	}
}


void Buffer::AdjustOffset(int32_t adjust_offset)
{
	offset_ptr_ = mem_end_ptr_ + adjust_offset;
	if (offset_ptr_ > data_end_ptr_)
	{
		data_end_ptr_ = offset_ptr_;
	}else if (offset_ptr_ < mem_start_ptr_)
	{
		offset_ptr_ = mem_start_ptr_;
	}
}

int32_t Buffer::ResetSize(int32_t size) {
	char *tmp_mem_ptr = new char[size];
	int32_t data_len = data_end_ptr_ - mem_start_ptr_;
	int32_t offset_len = offset_ptr_ - mem_start_ptr_;
	memcpy(tmp_mem_ptr, mem_start_ptr_, data_len);
	delete[] mem_start_ptr_;
	mem_start_ptr_ = tmp_mem_ptr;
	mem_end_ptr_ = mem_start_ptr_ + size;
	data_end_ptr_ = mem_start_ptr_ + data_len;
	offset_ptr_ = mem_start_ptr_ + offset_len;
	return size;
}

int32_t Buffer::WriteBuff(const char* buf, int32_t len) {
	int32_t avaliable_length = mem_end_ptr_ - data_end_ptr_;
	if (avaliable_length < len) {
		int32_t need_size = len - avaliable_length;
		int32_t size = mem_end_ptr_ - mem_start_ptr_ + need_size;
		ResetSize(size);
	}
	memcpy(data_end_ptr_, buf, len);
	data_end_ptr_ += len;
	return len;
}

int32_t Buffer::ReadBuf(char* buf, int32_t len) {
	if (!mem_start_ptr_){
		return 0;
	}
	int32_t avaliable_len = data_end_ptr_ - offset_ptr_;
	if (len > avaliable_len){
		len = avaliable_len;
	}
	if (len > 0){
		memcpy(buf, offset_ptr_, len);
		offset_ptr_ += len;
		if ( (offset_ptr_ - mem_start_ptr_) > (mem_end_ptr_ - mem_start_ptr_)/2 ){
			int data_len = data_end_ptr_ - offset_ptr_;
			memcpy(mem_start_ptr_, offset_ptr_, data_len);
			offset_ptr_ = 0;
			data_end_ptr_ = mem_start_ptr_ + data_len;
		}
	}
	return len;
}





