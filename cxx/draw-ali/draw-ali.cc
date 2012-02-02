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

template<class A> class AlignmentDrawer
{
public:
    typedef A Arc;
    typedef typename fst::Fst<Arc> Fst;
    typedef typename Arc::StateId StateId;
    typedef typename Arc::Label Label;
    typedef typename Arc::Weight Weight;
    typedef kaldi::int32 AliIdx;
    typedef std::vector<AliIdx> Ali;
    typedef typename std::tr1::unordered_map<Label, const Arc* > ISymArcMap;
    typedef typename std::tr1::unordered_map<StateId, ISymArcMap> StateMap;

    const std::string kAliColor;
    const std::string kNonAliColor;

    AlignmentDrawer(const Fst &fst, TransitionModel &tmodel,
                    const std::vector<kaldi::int32> &ali,
                    fst::SymbolTable &phone_syms,
                    fst::SymbolTable &word_syms,
                    const char *sep, bool show_tids):
        kAliColor("red"), kNonAliColor("black"), fst_(fst),
        tmodel_(tmodel), ali_(ali), phone_syms_(phone_syms),
        word_syms_(word_syms), sep_(sep), show_tids_(show_tids) {}

    void Draw()
    {
        DrawHeader();
        DrawFst();
        DrawFooter();
    }

private:
    void DrawHeader()
    {
        using namespace std;
        cout << "digraph FST {\n"
                "rankdir = LR;\n"
                "size = \"8.5,11\";\n"
                "label = \"\";\n"
                "center = 1;\n"
                "orientation = Portrait;\n"
                "ranksep = \"0.4\";\n"
                "nodesep = \"0.25\";\n";
    }

    void DrawFooter()
    {
        std::cout << "}\n";
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
                 const std::string color, int count)
    {
        using namespace std;

        cout << "\t" << state << " -> " << arc.nextstate;
        cout << " [ label = \"" << MakeLabel(arc, count) << "\", ";
        cout << "color = " << color << ", fontcolor = " << color;
        cout << ", fontsize = 14 ];\n";
    }

    /// Draws the nodes which are not in the alignment
    void DrawState(const StateId &state)
    {
        using namespace std;

        string node_style = "solid";
        string node_shape = "circle";
        KALDI_ASSERT(state != fst_.Start());
        if (fst_.Final(state) != Weight::Zero())
            node_shape = "doublecircle";
        ostringstream label;
        label << state;
        if (fst_.Final(state) != Weight::Zero())
            label << " / " << fst_.Final(state);

        // first draw the node itself
        cout << state << " [label = \"" << label.str() << "\", shape = " << node_shape;
        cout << ", style = " << node_style << ", color = " << kNonAliColor;
        cout << ", fontsize = 14]\n";

        // ... and then the outgoing arcs
        for (fst::ArcIterator<Fst> aiter(fst_, state); !aiter.Done(); aiter.Next())
            DrawArc(state, aiter.Value(), kNonAliColor, 1);

    }

    void DrawNonAliArcs()
    {
        typename StateMap::const_iterator stit;
        for (stit = ali_states_.begin(); stit != ali_states_.end(); stit++) {
            StateId state = stit->first;
            const ISymArcMap &isym_arc = stit->second;
            typename ISymArcMap::const_iterator arcit;
            for (arcit = isym_arc.begin(); arcit != isym_arc.end(); arcit++)
                DrawArc(state, *(arcit->second), kNonAliColor, 1);
        }
    }

    /// Update the map stateId -> non_alignment arcs
    void UpdateAliState(const StateId &state,
                        ISymArcMap &isym_to_arc,
                        const Label &ali_label)
    {
        typename StateMap::iterator it = ali_states_.find(state);
        if (it != ali_states_.end()) {
            // If the input label of an arc belonging to the alignment is
            // still in the list of the non-ali arcs - remove it
            typename ISymArcMap::iterator itlab = it->second.find(ali_label);
            if (itlab != it->second.end()) {
                it->second.erase(itlab);
            }
        }
        else {
            // If the state is not in the list of the nodes in the alignemnt - add it
            ali_states_[state] = isym_to_arc;
            it = ali_states_.find(state);
            typename ISymArcMap::iterator itlab = it->second.find(ali_label);
            KALDI_ASSERT(itlab != it->second.end());
            it->second.erase(itlab);
        }
    }

    bool DrawAliState(const StateId& state)
    {
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

        // Draw the state itself
        cout << state << " [label = \"" << label.str() << "\", shape = " << node_shape;
        cout << ", style = " << node_style << ", color = " << kAliColor;
        cout << ", fontsize = 14]\n";

        // Maybe this is a final state? - If so end the reccursion
        bool is_final = (fst_.Final(state) != Weight::Zero());
        if (is_final && ali_idx_ == ali_.size())
            return true;


        ISymArcMap isym_to_arc;
        for (fst::ArcIterator<Fst> aiter(fst_, state); !aiter.Done(); aiter.Next())
            isym_to_arc[aiter.Value().ilabel] = &(aiter.Value());

        bool arc_found;
        do {
            arc_found = false;
            Label isym = ali_[ali_idx_];
            typename ISymArcMap::iterator it =
                    isym_to_arc.find(static_cast<Label>(isym));
            if (it != isym_to_arc.end()) {
                arc_found = true;
                int count = 0;
                do {
                    ++ ali_idx_;
                    ++ count;
                } while (ali_[ali_idx_] == isym);
                DrawArc(state, *(it->second), kAliColor, count);
                StateId nextstate = it->second->nextstate;
                UpdateAliState(state, isym_to_arc, it->first);
                if (nextstate != state) {
                    // An alignment arc out of this state is found - hit the road
                    return DrawAliState(nextstate);
                }
            }
        } while (arc_found);

        if (is_final && ali_idx_ == ali_.size())
            return true;

        // if not final, try making an eps transition
        typename unordered_map<Label, const Arc*>::iterator it =
                isym_to_arc.find(static_cast<Label>(0));
        if (it != isym_to_arc.end()) {
            DrawArc(state, *(it->second), kAliColor, 1);
            StateId nextstate = it->second->nextstate;
            UpdateAliState(state, isym_to_arc, it->first);
            return DrawAliState(nextstate);
        }

        return false; // the alignment couldn't be matched with the graph
    }

    void DrawFst()
    {
        using namespace std::tr1;

        ali_idx_ = 0;
        ali_states_.clear();
        if (!DrawAliState(fst_.Start())) {
            KALDI_ERR << "Couldn't match the alignment and the graph!";
            return;
        }
        DrawNonAliArcs();
        for (fst::StateIterator<Fst> si(fst_); !si.Done(); si.Next()) {
            typename StateMap::iterator it =
                    ali_states_.find(si.Value());
            if (it == ali_states_.end())
                DrawState(si.Value()); // only draw states not in the alignment
        }
    }

    kaldi::int32 ali_idx_;
    StateMap ali_states_;

    const fst::Fst<A> &fst_;
    const kaldi::TransitionModel &tmodel_;
    const std::vector<kaldi::int32> &ali_;
    const fst::SymbolTable &phone_syms_;
    const fst::SymbolTable &word_syms_;
    const std::string sep_;
    const bool show_tids_;
}; // class AliDrawer
} // namespace kaldi

int main(int argc, char *argv[])
{
    using namespace kaldi;

    try {
        std::string key = "";
        bool show_tids = false;

        const char *usage = "Visualizes an alignment using GraphViz DOT language\n"
                "Usage: draw-alignments --ali-key=some_key <phone-syms> <word-syms> <model> <ali-rspec> <fst-rspec>\n\n";
        ParseOptions po(usage);
        po.Register("key", &key, "The key of the alignment/fst we want to render(mandatory!)");
        po.Register("show-tids", &show_tids, "Also shows the transition-ids");
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
        RandomAccessTableReader<fst::VectorFstHolder> fst_reader(fst_rspec);

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
        if (!fst_reader.HasKey(key)) {
            KALDI_ERR << "No FST with key '" << key
                      << "' has been found in '" << fst_rspec << "'";
            exit(1);
        }

        const std::vector<kaldi::int32> &ali = ali_reader.Value(key);
        const fst::Fst<fst::StdArc> &graph = fst_reader.Value(key);

        AlignmentDrawer<fst::StdArc> drawer(graph, trans_model,
                                        ali, *phones_symtab, *words_symtab,
                                        (const char *) "_", show_tids);
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
