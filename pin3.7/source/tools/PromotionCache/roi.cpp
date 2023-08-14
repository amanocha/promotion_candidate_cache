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
/*! @file
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>

#include "huge_page_reuse.h"
#include "hawkeye.h"

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

//std::ofstream TraceFile;
bool isROI = false;
const CHAR * ROI_BEGIN = "pin_start";
const CHAR * ROI_END = "pin_end";
int mode = 0; // 0 = PCC, 1 = HawkEye

std::map<std::string, unsigned long> access_intervals;

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "pinatrace.out", "specify trace file name");
KNOB<BOOL> KnobValues(KNOB_MODE_WRITEONCE, "pintool",
    "values", "1", "Output memory values reads and written");

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

static INT32 Usage()
{
    cerr <<
        "This tool produces a memory address trace.\n"
        "For each (dynamic) instruction reading or writing to memory the the ip and ea are recorded\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}


static VOID EmitMem(VOID * ea, INT32 size)
{
    std::ofstream TraceFile;
    
    if (!KnobValues)
        return;
    
    switch(size)
    {
      case 0:
        TraceFile << setw(1);
        break;
        
      case 1:
        TraceFile << static_cast<UINT32>(*static_cast<UINT8*>(ea));
        break;
        
      case 2:
        TraceFile << *static_cast<UINT16*>(ea);
        break;
        
      case 4:
        TraceFile << *static_cast<UINT32*>(ea);
        break;
        
      case 8:
        TraceFile << *static_cast<UINT64*>(ea);
        break;
        
      default:
        TraceFile.unsetf(ios::showbase);
        TraceFile << setw(1) << "0x";
        for (INT32 i = 0; i < size; i++)
        {
            TraceFile << static_cast<UINT32>(static_cast<UINT8*>(ea)[i]);
        }
        TraceFile.setf(ios::showbase);
        break;
    }
    TraceFile << "#eof" << endl;
    TraceFile.close();
}

// Set ROI flag
static VOID StartROI()
{
    isROI = true;
}

// Set ROI flag
static VOID StopROI()
{
    isROI = false;
}

// From https://stackoverflow.com/questions/32026456/how-can-i-specify-an-area-of-code-to-instrument-it-by-pintool
// Pin calls this function every time a new rtn is executed
VOID Routine(RTN rtn, VOID *v)
{
    // Get routine name
    string name = RTN_Name(rtn);

    if(name.find(ROI_BEGIN) != std::string::npos) {
        cout << "Routine = " << name << endl;
        // Start tracing after ROI begin exec
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)StartROI, IARG_END);
        RTN_Close(rtn);
    } else if (name.find(ROI_END) != std::string::npos) {
        cout << "Routine = " << name << endl;
        // Stop tracing before ROI end exec
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)StopROI, IARG_END);
        RTN_Close(rtn);
    }
}

static VOID RecordMem(VOID * ip, CHAR r, VOID * addr, INT32 size, BOOL isPrefetch)
{
    if (!isROI) return;
    /*cout << "RecordMem: " << r << " " << setw(2+2*sizeof(ADDRINT)) << addr << " "
              << dec << setw(2) << size << " "
              << hex << setw(2+2*sizeof(ADDRINT)) << endl;
    */

    if (mode) hawkeye_track_access((uint64_t) addr);
    else pcc_track_access((uint64_t) addr);
}

static VOID * WriteAddr;
static INT32 WriteSize;

static VOID RecordWriteAddrSize(VOID * addr, INT32 size)
{
    WriteAddr = addr;
    WriteSize = size;
}

static VOID RecordMemWrite(VOID * ip)
{
    RecordMem(ip, 'W', WriteAddr, WriteSize, false);
}

VOID Instruction(INS ins, VOID *v)
{

    // instruments loads using a predicated call, i.e.
    // the call happens iff the load will be actually executed
        
    if (INS_IsMemoryRead(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
            IARG_INST_PTR,
            IARG_UINT32, 'R',
            IARG_MEMORYREAD_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_BOOL, INS_IsPrefetch(ins),
            IARG_END);
    }

    if (INS_HasMemoryRead2(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordMem,
            IARG_INST_PTR,
            IARG_UINT32, 'R',
            IARG_MEMORYREAD2_EA,
            IARG_MEMORYREAD_SIZE,
            IARG_BOOL, INS_IsPrefetch(ins),
            IARG_END);
    }

    // instruments stores using a predicated call, i.e.
    // the call happens iff the store will be actually executed
    if (INS_IsMemoryWrite(ins) && INS_IsStandardMemop(ins))
    {
        INS_InsertPredicatedCall(
            ins, IPOINT_BEFORE, (AFUNPTR)RecordWriteAddrSize,
            IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE,
            IARG_END);
        
        if (INS_HasFallThrough(ins))
        {
            INS_InsertCall(
                ins, IPOINT_AFTER, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_END);
        }
        if (INS_IsBranchOrCall(ins))
        {
            INS_InsertCall(
                ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_END);
        }
        
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    cout << "Total Accesses = " << total_num_accesses << endl;
    //TraceFile << "#eof" << endl;
    //TraceFile.close();
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    string path = argv[6];
    string exec = path.substr(path.find_last_of("/\\") + 1);
    cout << argc << " arguments, path = " << path << ", exec = " << exec << endl;

    if (argc > 9) {
        mode = atoi(argv[9]);
    }
    if (argc > 12) {
        ACCESS_INTERVAL = atoi(argv[10]);
        PROMOTION_CACHE_SIZE = atoi(argv[11]);
        FACTOR = atoi(argv[12]);
    } else {
        ACCESS_INTERVAL = 10000000;
        PROMOTION_CACHE_SIZE = 128;
        FACTOR = 30;
    }

    if !(mode) cout << "ACCESS INTERVAL = " << ACCESS_INTERVAL << ", PROMOTION CACHE SIZE = " << PROMOTION_CACHE_SIZE << ", FACTOR = " << FACTOR << endl;
    
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    PIN_InitSymbols();
    RTN_AddInstrumentFunction(Routine, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    PIN_AddFiniFunction(Fini, 0);

    if (!mode) promotion_cache_init();

    // Never returns
    PIN_StartProgram();
    
    RecordMemWrite(0);
    RecordWriteAddrSize(0, 0);

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
