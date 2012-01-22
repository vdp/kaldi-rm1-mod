// Copyright 2012 Vassil Panayotov <vd.panayotov@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <exception>

#include <util/common-utils.h>
#include <matrix/matrix-lib.h>

namespace kaldi {

/// As far as I understand from sphinx_fe's code it writes big endian float MFCCs
/// SphinxFeatHolder assumes that the floating point byte-order is the same
/// as the integer byte-order.
/// The template parameters are more about documenting assumptions, than anything else.
template <typename FeatType=float, int fvec_len=13, bool be_feats=true, bool be_machine=false>
class SphinxFeatHolder {
public:
    typedef Matrix<FeatType> T;

    SphinxFeatHolder(): feats_(0) {}

    /// Read a sphinx feature file
    bool Read(std::istream &is) {
        int nmfcc;
        try {
            if (feats_) {
                delete feats_;
                feats_ = 0;
            }
            is.read((char*) &nmfcc, sizeof(nmfcc));
            if (be_feats != be_machine)
                nmfcc = swap(nmfcc);
            KALDI_VLOG(2) << "#feats: " << nmfcc;

            int nfvec = nmfcc / fvec_len;
            KALDI_ASSERT((nmfcc % fvec_len) == 0);
            feats_ = new T(nfvec, fvec_len);
            for (int i = 0; i < nfvec; i++) {
                if (!is.read((char*) feats_->RowData(i), fvec_len * sizeof(FeatType))) {
                    KALDI_ERR << "Unexpected EOF" << std::endl;
                    return false;
                }

                if (be_feats != be_machine) {
                    FeatType *f = feats_->RowData(i);
                    for (int j=0; j < fvec_len; j++) {
                        f[j] = swap(f[j]);
                    }
                }
            }
        }
        catch(std::exception e) {
            KALDI_ERR << e.what() << std::endl;
            return false;
        }

        return true;
    }

    /// Write a Sphinx-format feature file
    static bool Write(std::ostream& os, bool binary, const T& m) {
        if (!binary) {
            KALDI_ERR << "Can't write Sphinx features in text" << std::endl;
            return false;
        }

        try {
            int rows = m.NumRows();
            int head = (be_feats == be_machine)? rows: swap(rows);
            os.write((char*) head, sizeof(head));
            for (int i = 0; i < rows; i++) {
                os.write((char*) m.RowData(i), fvec_len * sizeof(FeatType));
            }
        }
        catch(std::exception e) {
            KALDI_ERR << e.what() << std::endl;
            return false;
        }

        return true;
    }

    /// Get the features
    T& Value() { return *feats_; }

    /// The Sphinx's feature files are binary
    static bool IsReadInBinary() { return true; }

    /// Free the buffer if requested
    void Clear() {
        if (feats_)
            delete feats_;
        feats_ = 0;
    }

    ~SphinxFeatHolder() {
        if (feats_)
            delete feats_;
    }

private:
    /// A naive byte-swapping routine
    template<class N>
    inline N swap(N val) {
        char tmp[4];
        char *p = (char*) &val;
        tmp[0] = p[3];
        tmp[1] = p[2];
        tmp[2] = p[1];
        tmp[3] = p[0];

        return *((N*) tmp);
    }

    /// The feature matrix
    T *feats_;
};

} // namespace kaldi

int main(int argc, char **argv) {
    using namespace kaldi;

    ParseOptions po("Usage: pack-sphinx-feats [options] <rxspecifier> <wxspecifier>\n");
    po.Read(argc, argv);
    if (po.NumArgs() != 2) {
        po.PrintUsage();
        exit(1);
    }

    std::string rspec = po.GetArg(1);
    std::string wspec = po.GetArg(2);
    SequentialTableReader<SphinxFeatHolder<> > reader(rspec);
    BaseFloatMatrixWriter writer;
    if (!writer.Open(wspec)) {
        KALDI_ERR << "Error while trying to open \"" << wspec << '\"';
        return 1;
    }

    int count = 0;
    for (; !reader.Done(); reader.Next(), count++) {
        std::string key = reader.Key();
        const Matrix<float> &feats = reader.Value();
        writer.Write(key, feats);
        KALDI_VLOG(2) << "Packaged: " << key;
    }
    KALDI_LOG << "Done packaging " << count << " feature files";

    return 0;
}
