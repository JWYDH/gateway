#ifndef JLIB_CONFIG_H_
#define JLIB_CONFIG_H_

/* #undef OS_LINUX */

class noncopyable
{  
protected:  
    noncopyable() {}  
    ~noncopyable() {}  
private:
    noncopyable( const noncopyable& ) = delete;  
    const noncopyable& operator=(const noncopyable&) = delete;  
};

#endif
