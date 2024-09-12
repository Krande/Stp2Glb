//
// Created by ofskrand on 12.09.2024.
//

#include "sc_parser.h"
#include <iostream>
#include "cllazyfile/lazyInstMgr.h"

// Function to parse a STEP file
void lazy_step_parser(const std::string& stp_file)
{
    auto mgr = new lazyInstMgr;
    mgr->openFile(stp_file);

    int instances = mgr->totalInstanceCount();
    std::cout << "Total instances: " << instances << "\n";
}
