#pragma once

// Original path confirmed via x_assert() calls inside several TPictureIO
// methods (not yet reconstructed), e.g. TPictureIO::ReadPictureFile():
// src/graphicslib/picture.cpp - see manifest/source_layout.tsv.
//
// The body of main()'s onRetryLoad lambda resolves (via its
// _M_invoke thunk) to a plain `jmp` into this function, i.e. the lambda is
// `[]{ TPictureIO::RetryFailedPicturesLoad(); }`.
class TPictureIO {
public:
    static void RetryFailedPicturesLoad();
};
