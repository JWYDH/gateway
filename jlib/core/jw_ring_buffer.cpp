#include "jw_ring_buffer.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <algorithm>

namespace jw
{

//-------------------------------------------------------------------------------------
RingBuf::RingBuf(size_t capacity)
{
	/* One byte is used for detecting the full condition. */
	m_buf = (uint8_t *)malloc(capacity);
	m_end = capacity;
	reset();
}

//-------------------------------------------------------------------------------------
RingBuf::~RingBuf()
{
	free(this->m_buf);
}

//-------------------------------------------------------------------------------------
void RingBuf::_auto_resize(size_t need_size)
{
	//auto inc size
	size_t new_size = 2;
	while (new_size < need_size)
	{
		new_size *= 2;
	}

	//copy old data
	size_t old_size = size();
	uint8_t *buf = (uint8_t *)malloc(new_size);
	this->write(buf, old_size);

	//free old buf
	free(m_buf);

	//reset
	m_buf = buf;
	m_end = new_size;
	m_read = 0;
	m_write = old_size;
}

void RingBuf::write(const void *src, size_t count)
{
	if (get_free_size() < count)
	{
		_auto_resize(size() + count);
	}

	char *csrc = (char *)src;

	//write data
	size_t nwritten = 0;
	while (nwritten != count)
	{
		size_t n = (size_t)std::min((size_t)(m_end - m_write), count - nwritten);
		memcpy(m_buf + m_write, csrc + nwritten, n);
		m_write += n;
		nwritten += n;

		// wrap
		assert(m_write <= m_end);
		if (m_write == m_end)
		{
			m_write = 0;
		}
	}
}

size_t RingBuf::read(void *dst, size_t count)
{
	size_t bytes_used = size();
	if (count > bytes_used)
	{
		count = bytes_used;
	}

	char *cdst = (char *)dst;
	size_t nread = 0;
	while (nread != count)
	{
		size_t n = std::min((size_t)(m_end - m_read), count - nread);
		memcpy(cdst + nread, m_buf + m_read, n);
		m_read += n;
		nread += n;

		assert(m_read <= m_end);
		// wrap
		if (m_read == m_end)
		{
			m_read = 0;
		}
	}

	//reset read and write index to zero
	if (empty())
		reset();
	return count;
}

size_t RingBuf::copyto(RingBuf *dst, size_t count)
{
	size_t bytes_used = size();
	if (count > bytes_used)
		count = bytes_used;

	size_t nread = 0;
	while (nread != count)
	{
		size_t n = std::min((size_t)(m_end - m_read), count - nread);
		dst->read(m_buf + m_read, n);
		m_read += n;
		nread += n;

		assert(m_read <= m_end);
		// wrap
		if (m_read == m_end)
		{
			m_read = 0;
		}
	}

	//reset read and write index to zero
	if (empty())
	{
		reset();
	}

	return count;
}

} // namespace jw