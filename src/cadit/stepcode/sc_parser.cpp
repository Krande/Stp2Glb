//
// Created by ofskrand on 12.09.2024.
//
#include <windows.h>
#include "sc_parser.h"

#include <filesystem>
#include <iostream>
#include "cllazyfile/lazyInstMgr.h"
#include <psapi.h>

#include "../../config_structs.h"

// Function to print memory usage
void printMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        std::cout << "Memory usage: " << pmc.WorkingSetSize / 1024 << " KB\n";
    }
}

// Function to parse a STEP file using lazy loading
void lazy_step_parser(const GlobalConfig& config)
{
    auto mgr = std::make_unique<lazyInstMgr>();

    printMemoryUsage(); // Memory before opening the file

    mgr->openFile(config.stpFile.string());

    printMemoryUsage(); // Memory after opening the file

    int instances = mgr->totalInstanceCount();
    std::cout << "Total instances: " << instances << "\n";

    printMemoryUsage(); // Memory after counting instances
}
