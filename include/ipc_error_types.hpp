#pragma once

namespace zero_copy_ipc {

enum class IpcErrorType {
    NoError = 0,
    LoanNoSubscriber = 1,
    LoanBufferFull = 2,
    UnknownError = 100
};

}