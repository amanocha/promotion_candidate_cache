#include "common.h"
#include "huge_page_reuse.h"
#include "hawkeye.h"

static VOID RecordMem(VOID * ip, CHAR r, VOID * addr, INT32 size, BOOL isPrefetch)
{
    if (!isROI) return;
    /*cout << "RecordMem: " << r << " " << setw(2+2*sizeof(ADDRINT)) << addr << " "
              << dec << setw(2) << size << " "
              << hex << setw(2+2*sizeof(ADDRINT)) << endl;
    */

    if (!mode) pcc_track_access((uint64_t) addr);
    else hawkeye_track_access((uint64_t) addr);
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
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    string path = argv[6];
    string exec = path.substr(path.find_last_of("/\\") + 1);

    if (std::find(other_benchmarks, other_benchmarks + 5, exec) != other_benchmarks + 5) {
        isROI = true;
    }
 
    cout << argc << " arguments, path = " << path << ", exec = " << exec << endl;

    if (argc > NUM_ARGS) {
        mode = pcc_str.compare(argv[NUM_ARGS]);
    }
    if (argc > NUM_ARGS+3) {
        ACCESS_INTERVAL = atol(argv[NUM_ARGS+1]);
        PROMOTION_CACHE_SIZE = atoi(argv[NUM_ARGS+2]);
        FACTOR = atoi(argv[NUM_ARGS+3]);
    } else if (argc > NUM_ARGS+1) {
        ACCESS_INTERVAL = atol(argv[NUM_ARGS-1]);
        PROMOTION_CACHE_SIZE = 128;
        FACTOR = 30;
    }

    cout << "MODE = " << mode << ", ACCESS INTERVAL = " << ACCESS_INTERVAL << ", PROMOTION CACHE SIZE = " << PROMOTION_CACHE_SIZE << ", FACTOR = " << FACTOR << endl;
    
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
