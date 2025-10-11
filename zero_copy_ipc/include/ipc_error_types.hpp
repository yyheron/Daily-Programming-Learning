#pragma once

namespace zero_copy_ipc {

enum class IpcErrorType {
    NoError = 0,
    LoanNoSubscriber = 1,
    LoanBufferFull = 2,
    LoanQueueNotCreated = 3,
    // publish error
    PublishNoSubscriber = 4,
    PublishSlotNotCreated = 5,
    PublishSemaphoreNotCreated = 6,
    PublishQueueNotCreated = 7,

    UnknownError = 100
};

}