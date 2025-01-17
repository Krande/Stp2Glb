#include "custom_progress.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

CustomProgressIndicator::CustomProgressIndicator() : shouldCancel(false), progress(0.0), lastProgress(-1.0) {
    // Start the progress update thread
    progressThread = std::thread(&CustomProgressIndicator::UpdateProgress, this);
}

CustomProgressIndicator::~CustomProgressIndicator() {
    // Stop the progress update thread safely
    if (progressThread.joinable()) {
        shouldCancel = true;
        progressThread.join();
    }
}

Standard_Boolean CustomProgressIndicator::UserBreak() {
    return shouldCancel; // Return true to stop the operation
}

void CustomProgressIndicator::Show(const Message_ProgressScope &theScope, const Standard_Boolean isForce) {
    // This function will be used to set progress from your existing logic
    const double newProgress = GetPosition(); // Get current progress [0.0, 1.0]
    SetProgress(newProgress);  // Update progress asynchronously
}

void CustomProgressIndicator::Cancel() {
    shouldCancel = true;
}

void CustomProgressIndicator::Reset() {
    shouldCancel = false;
    SetProgress(0.0);  // Reset progress to 0
}

void CustomProgressIndicator::SetProgress(double newProgress) {
    std::lock_guard<std::mutex> lock(mtx);
    progress = newProgress;
}

void CustomProgressIndicator::UpdateProgress() {
    while (!shouldCancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Update progress every 100ms

        // Lock the mutex to safely access the progress value
        std::lock_guard<std::mutex> lock(mtx);

        // Only update the progress if it has changed by at least 5%
        if (static_cast<int>(progress * 100) / 5 > static_cast<int>(lastProgress * 100) / 5) {
            lastProgress = progress;

            const int barWidth = 50; // Width of the progress bar
            const int pos = static_cast<int>(progress * barWidth);

            // Lock std::cout for thread safety to avoid race conditions
            std::lock_guard<std::mutex> coutLock(coutMutex);

            // Clear the line before printing the new progress
            std::cout << "\r" << std::string(80, ' ');  // Clear the current line (adjust the size if necessary)
            std::cout << "\r[";

            // Build the progress bar
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "="; // Completed part
                else if (i == pos) std::cout << ">"; // Current progress
                else std::cout << " "; // Remaining part
            }

            std::cout << "] " << static_cast<int>(progress * 100.0) << "%   " << std::flush;
        }
    }

    // Finalize the progress bar when the task is done
    std::lock_guard<std::mutex> coutLock(coutMutex);
    std::cout << "\r[==================================================] 100%" << std::endl;
}
