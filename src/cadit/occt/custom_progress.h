#ifndef CUSTOM_PROGRESS_H
#define CUSTOM_PROGRESS_H

#include <Message_ProgressIndicator.hxx>
#include <atomic>
#include <thread>
#include <mutex>

class CustomProgressIndicator : public Message_ProgressIndicator {
public:
    CustomProgressIndicator();  // Constructor
    ~CustomProgressIndicator(); // Destructor

    //! Called by OpenCASCADE to check if the operation should stop
    Standard_Boolean UserBreak() override;

    //! Updates progress (called by OpenCASCADE)
    void Show(const Message_ProgressScope &theScope, const Standard_Boolean isForce) override;

    //! Cancels the progress update (stops background thread)
    void Cancel();

    //! Resets the progress indicator
    void Reset() override;

private:
    // Thread-safe method to set progress
    void SetProgress(double newProgress);

    // Background thread function to update progress asynchronously
    void UpdateProgress();

    // Flag to indicate whether the operation should be canceled
    std::atomic<bool> shouldCancel;

    // Current progress (from 0.0 to 1.0)
    double progress;

    // Last progress to track the threshold for updating
    double lastProgress;

    // Mutex to protect shared progress data between threads
    std::mutex mtx;

    // Mutex for locking std::cout to prevent race conditions in printing
    std::mutex coutMutex;

    // Background thread for asynchronous progress updates
    std::thread progressThread;
};

#endif // CUSTOM_PROGRESS_H
