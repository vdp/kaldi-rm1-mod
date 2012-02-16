// bin/draw-ali.cc

// Copyright 2012  Vassil Panayotov <vd.panayotov@gmail.com>

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

#include "base/kaldi-common.h"
#include "hmm/transition-model.h"
#include "hmm/hmm-utils.h"
#include "util/common-utils.h"
#include "fst/fstlib.h"

#include <tr1/unordered_set>
#include <tr1/unordered_map>

namespace kaldi {

template<class F> class AlignmentDrawer
{
public:
    typedef F Fst;
    typedef typename Fst::Arc Arc;
    typedef typename Arc::StateId StateId;
    typedef typename Arc::Label Label;
    typedef typename Arc::Weight Weight;
    typedef typename fst::ArcIterator<Fst> ArcIterator;
    typedef std::vector<kaldi::int32> Alignment;
    typedef std::pair<StateId, size_t> FstTracePoint;
    typedef std::vector<FstTracePoint> FstTrace;
    typedef std::tr1::unordered_set<size_t> TraceArcs;
    typedef std::tr1::unordered_map<StateId, TraceArcs> TraceMap;

    static const std::string kAliColor;
    static const std::string kNonAliColor;
    static const int kEpsLabel = 0;

    AlignmentDrawer(const Fst &fst, TransitionModel &tmodel,
                    const std::vector<kaldi::int32> &ali,
                    fst::SymbolTable &phone_syms,
                    fst::SymbolTable &word_syms,
                    const char *sep, bool show_tids, bool ali_only):
        fst_(fst), tmodel_(tmodel), ali_(ali), phone_syms_(phone_syms),
        word_syms_(word_syms), sep_(sep),
        show_tids_(show_tids), ali_only_(ali_only) {}


    void Draw()
    {
        using namespace std;
        bool found = FindTrace();
        if (!found) {
            KALDI_WARN << "No alignment has been found!";
            return;
        }

        // DOT header
        cout << "digraph FST {\n"
                "rankdir = LR;\n"
                "size = \"8.5,11\";\n"
                "label = \"\";\n"
                "center = 1;\n"
                "orientation = Portrait;\n"
                "ranksep = \"0.4\";\n"
                "nodesep = \"0.25\";\n";

        DrawTrace();
        if (!ali_only_)
            DrawRest();

        // DOT footer
        cout << "}\n";
    }

private:

    void DrawRest() {
        fst::StateIterator<Fst> sti(fst_);
        for (; !sti.Done(); sti.Next()) {
            StateId state = sti.Value();
            typename TraceMap::iterator tmi = trace_map_.find(state);
            bool state_traced = (tmi != trace_map_.end());
            if (!state_traced)
                DrawState(state, kNonAliColor);
            ArcIterator ai(fst_, state);
            for (; !ai.Done(); ai.Next()) {
                if (!state_traced)
                    DrawArc(state, ai.Value(), 1, kNonAliColor);
                else if (tmi->second.find(ai.Position()) == tmi->second.end())
                    DrawArc(state, ai.Value(), 1, kNonAliColor);
            }
        }
    }

    /// Creates a map: state -> all out arcs which belong to the alignment trace
    void UpdateTraceMap(StateId &state, size_t arc) {
        typename TraceMap::iterator tmi = trace_map_.find(state);
        if (tmi == trace_map_.end()) {
            // This is the first time we visit this state - init its arc set
            TraceArcs arcs;
            arcs.insert(arc);
            trace_map_.insert(std::make_pair(state, arcs));
        }
        else {
            tmi->second.insert(arc);
        }
    }

    void DrawState(StateId state, const std::string &color) {
        using namespace std;

        string node_style = "solid";
        string node_shape = "circle";
        if (state == fst_.Start())
            node_style = "bold";
        if (fst_.Final(state) != Weight::Zero())
            node_shape = "doublecircle";
        ostringstream label;
        label << state;
        if (fst_.Final(state) != Weight::Zero())
            label << " / " << fst_.Final(state);

        cout << state << " [label = \"" << label.str() << "\", shape = " << node_shape;
        cout << ", style = " << node_style << ", color = " << color << "];\n";
    }

    std::string MakeLabel(const Arc &arc, int count)
    {
        std::ostringstream oss;

        if(count > 1)
            oss << '(' << count << "x)";

        kaldi::int32 tid = arc.ilabel;
        kaldi::int32 phnid = 0;
        if (tid != 0) {
            phnid = tmodel_.TransitionIdToPhone(tid);
            oss << phone_syms_.Find(static_cast<kaldi::int64>(phnid));
            oss << sep_ << tmodel_.TransitionIdToHmmState(tid);
            oss << sep_ << tmodel_.TransitionIdToPdf(tid);
            oss << sep_ << tmodel_.TransitionIdToTransitionIndex(tid);
        }
        else {
            // the input symbol is <eps>
            oss << phone_syms_.Find(static_cast<kaldi::int64>(phnid));
        }
        if (show_tids_)
            oss << '[' << tid << ']';
        oss << ':' << word_syms_.Find(static_cast<kaldi::int64>(arc.olabel));
        if (arc.weight != Weight::One())
            oss << '/' << arc.weight;

        return oss.str();
    }

    void DrawArc(const StateId &state, const Arc &arc,
                 const int count, const std::string &color) {
        using namespace std;

        cout << "\t" << state << " -> " << arc.nextstate;
        cout << " [ label = \"" << MakeLabel(arc, count) << "\", ";
        cout << "color = " << color << ", fontcolor = " << color;
        cout << "];\n";
    }

    void DrawTrace() {
        int t;
        for (t = 0; t < fst_trace_.size();) {
            StateId state = fst_trace_[t].first;
            size_t arc_pos = fst_trace_[t].second;
            ArcIterator ait(fst_, state);
            ait.Seek(arc_pos);
            const Arc &arc = ait.Value();
            int count = 1;
            while (++t < fst_trace_.size() &&
                   fst_trace_[t].first == state &&
                   fst_trace_[t].second == arc_pos)
                ++ count;

            typename TraceMap::const_iterator tmi = trace_map_.find(state);
            if (tmi == trace_map_.end())
                // This is the first time we reach this state - draw it
                DrawState(state, kAliColor);

            DrawArc(state, arc, count, kAliColor);
            UpdateTraceMap(state, arc_pos);
        }
    }

    // Describes a particular state of the alignment-matching process
    struct AliHypothesys {
        AliHypothesys(StateId state, size_t arc,
                      size_t ali_idx, size_t fst_ali_len):
            state(state), arc(arc),
            ali_idx(ali_idx), fst_ali_len(fst_ali_len) {}

        StateId state; // state ID
        size_t arc;  // the index of an outgoing arc of "state"
        size_t ali_idx; // points to the trans-id to be matched by state/arc
        size_t fst_ali_len; // the length of the fst state/arc sequence that agrees with
                            // state-id subsequence from 0 to ali_idx inclusive
    };

    template <typename S>
    struct VisitedHash {
        size_t operator() (const std::pair<S, size_t> &entry) const {
            return (entry.first << 16) + entry.second;
        }
    };

    template <typename S>
    struct VisitedEqual {
        bool operator() (const std::pair<S, size_t> &a,
                         const std::pair<S, size_t> &b) const {
            return (a.first == b.first && a.second == b.second);
        }
    };

    bool FindTrace()
    {
        fst_trace_.clear();
        std::vector<AliHypothesys> hypotheses;

        // <state, alignment_prefix> to avoid e.g. <eps> loops
        std::tr1::unordered_set<
                std::pair<StateId, size_t>,
                VisitedHash<StateId>, VisitedEqual<StateId> > visited;

        StateId start = fst_.Start();
        if (start == fst::kNoStateId)
            return false;

        // Init the hypotheses queue
        size_t ali_idx = 0;
        size_t fst_ali_len = 0;
        for (ArcIterator aiter(fst_, start); !aiter.Done(); aiter.Next()) {
            const Arc &arc = aiter.Value();
            if (arc.ilabel == kEpsLabel || arc.ilabel == ali_[ali_idx]) {
                hypotheses.push_back(
                            AliHypothesys(start, aiter.Position(),
                                          ali_idx, fst_ali_len));
            }
        }
        visited.insert(std::make_pair(start, ali_idx));

        // Test and extend/backtrack alignments as needed
        while (!hypotheses.empty()) {
            AliHypothesys hyp = hypotheses.back();
            hypotheses.pop_back();

            ali_idx = hyp.ali_idx;
            ArcIterator ait(fst_, hyp.state);
            ait.Seek(hyp.arc);
            const Arc &arc = ait.Value();
            KALDI_ASSERT(arc.ilabel == kEpsLabel ||
                         arc.ilabel == ali_[ali_idx]);
            if (arc.ilabel != kEpsLabel)
                ++ ali_idx;

            if (hyp.fst_ali_len < fst_trace_.size()) {
                //backtrack
                typename FstTrace::iterator ftit = fst_trace_.begin() + hyp.fst_ali_len;
                fst_trace_.erase(ftit, fst_trace_.end());
            }
            fst_trace_.push_back(std::make_pair(hyp.state, hyp.arc));

            StateId nextstate = arc.nextstate;
            if (ali_idx == ali_.size() && fst_.Final(nextstate) != Weight::Zero())
                return true;

            if (visited.find(std::make_pair(nextstate, ali_idx))
                    != visited.end())
                continue; // this state/alignment pair was already considered

            // Extend the current hypothesis
            visited.insert(std::make_pair(nextstate, ali_idx));
            ArcIterator nait(fst_, nextstate);
            for (; !nait.Done(); nait.Next()) {
                const Arc &narc = nait.Value();
                if (narc.ilabel == kEpsLabel || narc.ilabel == ali_[ali_idx])
                    hypotheses.push_back(AliHypothesys(nextstate, nait.Position(),
                                                       ali_idx, fst_trace_.size()));
            }
        }

        return false; // no alignment has been found
    }

    FstTrace fst_trace_;

    // A map from a state that belongs to the alignment trace
    // to its output arcs that belong to the trace
    TraceMap trace_map_;

    const Fst &fst_;
    const TransitionModel &tmodel_;
    const Alignment &ali_;
    const fst::SymbolTable &phone_syms_;
    const fst::SymbolTable &word_syms_;
    const std::string sep_;
    const bool show_tids_;
    const bool ali_only_;
};

template<typename F> const std::string AlignmentDrawer<F>::kAliColor = "red";
template<typename F> const std::string AlignmentDrawer<F>::kNonAliColor = "black";

} // namespace kaldi

int main(int argc, char *argv[])
{
    using namespace kaldi;

    try {
        std::string key = "";
        bool show_tids = false;
        bool ali_only = false;

        const char *usage = "Visualizes an alignment using GraphViz DOT language\n"
                "Usage: draw-ali [options] <phone-syms> <word-syms> <model> <ali-rspec> <fst-rspec>\n\n";
        ParseOptions po(usage);
        po.Register("key", &key, "The key of the alignment/fst we want to render(mandatory!)");
        po.Register("show-tids", &show_tids, "Also shows the transition-ids");
        po.Register("ali-only", &ali_only, "Draw only the states/arcs in the alignment");
        po.Read(argc, argv);
        if (po.NumArgs() != 5 || key == "") {
            po.PrintUsage();
            exit(1);
        }

        std::string phn_file = po.GetArg(1);
        std::string wrd_file = po.GetArg(2);
        std::string mdl_file = po.GetArg(3);
        std::string ali_rspec = po.GetArg(4);
        std::string fst_rspec = po.GetArg(5);

        RandomAccessTableReader<BasicVectorHolder<kaldi::int32> > ali_reader(ali_rspec);

        fst::SymbolTable *phones_symtab = NULL;
        {
            std::ifstream is(phn_file.c_str());
            phones_symtab = ::fst::SymbolTable::ReadText(is, phn_file);
            if (!phones_symtab)
                KALDI_ERR << "Could not read phones symbol-table file "<< phn_file;
        }

        fst::SymbolTable *words_symtab = NULL;
        {
            std::ifstream is(wrd_file.c_str());
            words_symtab = ::fst::SymbolTable::ReadText(is, wrd_file);
            if (!words_symtab)
                KALDI_ERR << "Could not read words symbol-table file "<< wrd_file;
        }

        TransitionModel trans_model;
        {
            bool binary;
            Input ki(mdl_file, &binary);
            trans_model.Read(ki.Stream(), binary);
        }

        if (!ali_reader.HasKey(key)) {
            KALDI_ERR << "No alignment with key '" << key
                      << "' has been found in '" << ali_rspec << "'";
            exit(1);
        }

        const std::vector<kaldi::int32> &ali = ali_reader.Value(key);

        const fst::VectorFst<fst::StdArc> *graph;
        RandomAccessTableReader<fst::VectorFstHolder> fst_reader;
        if (fst_rspec.compare(0, 4, "ark:") &&
            fst_rspec.compare(0, 4, "scp:")) {

            graph = fst::VectorFst<fst::StdArc>::Read(fst_rspec);
        }
        else {
            fst_reader.Open(fst_rspec);
            if (!fst_reader.HasKey(key))
                KALDI_ERR << "No FST with key '" << key
                          << "' has been found in '" << fst_rspec << "'";
            graph = &(fst_reader.Value(key));
        }

        typedef AlignmentDrawer<fst::VectorFst<fst::StdArc> > Drawer;
        Drawer drawer(*graph, trans_model,
                      ali, *phones_symtab, *words_symtab,
                      (const char *) "_", show_tids, ali_only);

        drawer.Draw();

        delete phones_symtab;
        delete words_symtab;
    }
    catch (std::exception& e) {
        KALDI_ERR << e.what();
        return -1;
    }

    return 0;
}
