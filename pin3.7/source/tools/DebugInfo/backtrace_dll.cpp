/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2018 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#define __ISO_C_VISIBLE 1999
#ifdef NDEBUG
# undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <execinfo.h>
#include <iostream>

using namespace std;

extern "C" void qux()
{
    cout << "qux" << endl;
}

extern "C" void baz()
{
    void* buf[128];
    int nptrs = backtrace(buf, sizeof(buf)/sizeof(buf[0]));
    assert(nptrs > 0);
    char** bt = backtrace_symbols(buf, nptrs);
    assert(NULL != bt);
    for (int i = 0; i < nptrs; i++)
    {
        cout << bt[i] << endl;
    }
    free(bt);
}

extern "C" void bar()
{
    baz();
    qux();
}

extern "C" __attribute__((visibility("default"))) void foo();

extern "C" void foo()
{
    bar();
    qux();
}
