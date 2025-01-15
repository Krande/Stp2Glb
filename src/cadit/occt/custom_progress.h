#ifndef CUSTOM_PROGRESS_H
#define CUSTOM_PROGRESS_H

#include <Message_ProgressIndicator.hxx>
#include <atomic>

class CustomProgressIndicator : public Message_ProgressIndicator {
public:
    CustomProgressIndicator();

    //! Called by OpenCASCADE to check if the operation should stop
    Standard_Boolean UserBreak() override;

    //! Updates progress
    void Show(const Message_ProgressScope &theScope, const Standard_Boolean isForce) override;

    //! Cancels the progress
    void Cancel();

    //! Resets the progress indicator (optional implementation)
    void Reset() override;

private:
    std::atomic<bool> shouldCancel; // Thread-safe cancellation flag
};

#endif //CUSTOM_PROGRESS_H
