//
// Created by ofskrand on 15.01.2025.
//

#include "custom_progress.h"

#include <Message_ProgressScope.hxx>

CustomProgressIndicator::CustomProgressIndicator() : shouldCancel(false) {
}

Standard_Boolean CustomProgressIndicator::UserBreak() {
    return shouldCancel; // Return true to stop the operation
}

void CustomProgressIndicator::Show(const Message_ProgressScope &theScope, const Standard_Boolean isForce) {
    const int barWidth = 50; // Width of the progress bar
    double progress = GetPosition(); // Current progress [0.0, 1.0]

    // Build the progress bar
    std::cout << "\r[";
    int pos = static_cast<int>(progress * barWidth);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "="; // Completed part
        else if (i == pos) std::cout << ">"; // Current progress
        else std::cout << " "; // Remaining part
    }
    std::cout << "] " << static_cast<int>(progress * 100.0) << "%   " << std::flush;
}

void CustomProgressIndicator::Cancel() {
    shouldCancel = true;
}

void CustomProgressIndicator::Reset() {
    shouldCancel = false;
}
