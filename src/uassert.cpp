// uassert.cpp -- SUPPORT 0x004acc90..0x004acdb8.  The library's own assert.
//
// `_uassert` is the one every VASSERT in the DRDP tree lands in, and it is
// where `eqcalls.py` reads the original source tree's name out of the relocated
// dump: the second argument is `__FILE__` and the third is `__LINE__`, so every
// file in this library is padded to the line layout it shipped with.
//
// Neither of these RETURNS.  The report goes to stderr and then the function
// spins -- `for (;;) ;`, which the assembler pads out to the R5900's short-loop
// minimum -- so a tripped assert on a console is a hang, not an exit.
#include "drdp.h"

// newlib's reentrancy structure, as far as `stderr` needs it: the macro is
// `_impure_ptr->_stderr` and the pointer is at +12, which is what the `lw
// a0,12(v1)` in front of every fprintf here is.
struct _reent
{
    int   _errno;
    void *_stdin;
    void *_stdout;
    void *_stderr;
};

extern "C" struct _reent *_impure_ptr;
extern "C" int fprintf(void *stream, const char *format, ...);
extern "C" int vfprintf(void *stream, const char *format, char *ap);

#define stderr (_impure_ptr->_stderr)

// Phase 10's va_start: the EABI's register save block sits BELOW
// virtual_incoming_args, so bare __builtin_next_arg lands past the whole area.
typedef char *va_list;
#define va_start(ap, last) \
    ((ap) = (char *)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)

// The location `_uassertf` quotes.  It has no expression string of its own --
// its first argument is the CONDITION, already evaluated -- so the file and the
// line have to have been parked by a macro at the call site.
static const char *g_pAssertFile;
static int g_iAssertLine;

extern "C" void _uassert_set_location(const char *pFile, int iLine)
{
    g_pAssertFile = pFile;
    g_iAssertLine = iLine;
}

extern "C" void _uassert(const char *pExpr, const char *pFile, int iLine)
{
    fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", pExpr, pFile, iLine);
    for (;;)
        ;
}

extern "C" void _uassertf(int iExpr, const char *pFormat, ...)
{
    va_list ap;

    if (!iExpr)
    {
        fprintf(stderr, "Assertion failed: ");
        va_start(ap, pFormat);
        vfprintf(stderr, pFormat, ap);
        fprintf(stderr, ", file %s, line %d\n", g_pAssertFile, g_iAssertLine);
        for (;;)
            ;
    }
}
