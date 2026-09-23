#pragma once

// The body of main()'s onRetryLoad lambda resolves (via its
// _M_invoke thunk) to a plain `jmp` into this function, i.e. the lambda is
// `[]{ TPictureIO::RetryFailedPicturesLoad(); }`.
class TPictureIO {
public:
    static void RetryFailedPicturesLoad();
};
