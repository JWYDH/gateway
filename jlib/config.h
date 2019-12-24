#pragma once

/* #undef OS_WINDOWS */
/* #undef JW_ENABLE_LOG */

class noncopyable
{  
protected:  
    noncopyable() {}  
    ~noncopyable() {}  
private:
    noncopyable( const noncopyable& ) = delete;  
    const noncopyable& operator=(const noncopyable&) = delete;  
};
