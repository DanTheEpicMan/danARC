#include <filesystem>
#include <iostream>
#include "memory/memory.h"
#include "utils/localUtil.h"
#include <fstream>
#include <sstream>

#define DBGPrint true

#ifdef DBGPrint
#define DBG(x) std::cout << x << std::endl;
#else
#define DBG(x)
#endif

int main() {
    // Set these from your process finder
    ProcessId = FindGamePID();  // Your game PID
    BaseAddress = 0x140000000;  // Base address

    DBG("[+] PID:" << ProcessId)

    int count{};
    while (true) { count++;
        //small changes, infrequently (every coupel sec)
        if (count % 100 == 0) {
            uintptr_t uworld = GetUWorld(BaseAddress);
            DBG("[+] UWorld 0x" << std::hex << uworld << std::dec)
            if (!isValidPtr(uworld)) {
                DBG("[-] UWorld is Invalid!");
                return 1;
            }

            ptr levelsPtr = ReadMemory<uintptr_t>(uworld + off::LEVELS_PTR);
            int levelsCount = ReadMemory<int>(uworld + off::LEVELS_PTR + 0x8);

            if (levelsCount > 0 && levelsCount < 50) {
                DBG("[+] UWorld is VALID! Contains " << levelsCount << " level(s)");
            } else {
                DBG("[-] UWorld might be invalid (bad level count)");
            }//if levels
        }//if count

        //get cam info

        //get actor loop and count, then loop

            //get individual actor

            //filter for players vs other stuff

            //get position

            //translate to screen coords

    } //while true

    //end func
    return 0;
}