// bin/draw-tree.cc

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
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "fst/fstlib.h"

namespace kaldi {

/// Traverses the event map tree depth-first and spits out its GraphViz description
class TreeRenderer: public EventMapVisitor
{
public:
    TreeRenderer(EventMap &root, const fst::SymbolTable *phone_syms,
                 kaldi::int32 N, kaldi::int32 P) :
        kColor_("black"), kTraceColor_("red"), kPen_(1), kTracePen_(3),
        root_(root), N(N), P(P), phone_syms_(phone_syms),
        next_id_(0), parent_id_(0)
    {
        KALDI_ASSERT(((N == 3 && P == 1) || (N == 1 && P == 0)) &&
                     "Unsupported context window!");
    }

    void Render(const EventType *event = 0) {
        event_ = event;
        if (event != 0)
            path_active_ = true;
        else
            path_active_ = false;

        std::cout << "digraph EventMap {" << std::endl;
        root_.Accept(*this);
        std::cout << '}' << std::endl;
    }

    virtual void VisitSplit(EventKeyType &key,
                            ConstIntegerSet<EventValueType> &yes_set,
                            EventMap *yes_map,
                            EventMap *no_map)
    {
        kaldi::int32 my_id = next_id_ ++;

        // Draw this node and the input edge from its parent
        DrawThisNode(my_id, key);

        // Descend into this node's children
        std::string yes_color(kColor_), no_color(kColor_);
        kaldi::int32 yes_pen = kPen_, no_pen = kPen_;
        bool yes_active = false;
        bool active = path_active_;
        if (event_ != 0 && path_active_) {
            EventValueType value;
            EventMap::Lookup(*event_, key, &value);
            if (yes_set.count(value)) {
                yes_color = kTraceColor_;
                yes_active = true;
                yes_pen = kTracePen_;
            } else {
                no_color = kTraceColor_;
                no_pen = kTracePen_;
            }
        }

        // "Yes" child
        std::string yes_tooltip = MakeYesTooltip(key, yes_set);
        path_active_ = (active && yes_active);
        parent_id_ = my_id;
        std::ostringstream oss_yes;
        oss_yes << "[color=" << yes_color
                << ", label=\"" << yes_tooltip << '\"'
                << ", penwidth=" << yes_pen
                << "];";
        edge_attr_ = oss_yes.str();
        yes_map->Accept(*this);

        // "No" child
        path_active_ = (active && !yes_active);
        parent_id_ = my_id;
        std::ostringstream oss_no;
        oss_no << "[color=" << no_color
               << ", penwidth=" << no_pen << "];";
        edge_attr_ = oss_no.str();
        no_map->Accept(*this);
    }

    virtual void VisitConst(const EventAnswerType &answer)
    {
        std::ostringstream oss;

        kaldi::int32 id = next_id_++;

        // Draw the edge from parent
        if (parent_id_ >= 0 && id > 0)
            oss << '\t' << parent_id_ << " -> " << id <<
                   edge_attr_ << std::endl;

        std::string color = kColor_;
        kaldi::int32 pen = kPen_;
        if (path_active_) {
            pen = kTracePen_;
            color = kTraceColor_;
        }

        // Draw a leaf node
        oss << id << "[shape=\"doublecircle\", label=" << answer
            << ",color=" << color << ", penwidth=" << pen << "];";
        std::cout << oss.str() << std::endl;
    }

    virtual void VisitTable(const EventKeyType &key, std::vector<EventMap*> &table)
    {
        kaldi::int32 my_id = next_id_ ++;

        // Draw the node and the input edge from its parent
        DrawThisNode(my_id, key);

        // Descend into the event maps stored in table's entries
        bool active = path_active_;
        EventValueType value = -1;
        if (event_)
            EventMap::Lookup(*event_, key, &value);
        for (int i = 0; i < table.size(); i++) {
            if (table[i] == NULL)
                continue;

            std::ostringstream label;
            if (key == kPdfClass) {
                label << i;
            }
            else if (key < N) {
                std::string phone = phone_syms_->Find(
                            static_cast<kaldi::int64>(i));
                if (phone.empty()) {
                    KALDI_ERR << "Invalid phone key!";
                }
                label << phone;
            }
            else {
                KALDI_ERR << "Invalid event key!";
            }

            path_active_ = false;
            parent_id_ = my_id;
            std::string color(kColor_);
            kaldi::int32 pen = kPen_;
            if (i == value && active) {
                color = kTraceColor_;
                path_active_ = true;
                pen = kTracePen_;
            }
            std::ostringstream oss;
            oss << "[color=" << color
                << ", label=" << label.str()
                << ", penwidth=" << pen << "];";
            edge_attr_ = oss.str();
            table[i]->Accept(*this);
        }
    }

private:

    void DrawThisNode(kaldi::int32 my_id, const EventKeyType &key)
    {
        std::ostringstream out;
        std::string color = kColor_;
        kaldi::int32 pen = kPen_;
        if (path_active_ && event_) {
            color = kTraceColor_;
            pen = kTracePen_;
        }

        // Draw the incomming edge from this node's parent
        if (my_id > 0) // don't draw self-loop at the root
            out << '\t' <<  parent_id_ << " -> " << my_id << edge_attr_ << std::endl;

        // Draw the node itself
        std::string label;
        if (key == kPdfClass) {
            label = "\"HMM state = ?\"";
        }
        else if (key == 0) {
            if (N == 1)
                label = "\"Phone = ?\"";
            else
                label = "\"LContext = ?\"";
        }
        else if (key == 1 && key < N) {
            label = "\"Center = ?\"";
        }
        else if (key == 2 && key < N) {
            label = "\"RContext = ?\"";
        }
        else {
            KALDI_ERR << "Unexpected key: " << key;
        }
        out << my_id << " [label=" << label
            << ", color=" << color
            << ", penwidth=" << pen << "];";
        std::cout << out.str() << std::endl;
    }

    std::string MakeYesTooltip(EventKeyType key,
                               const ConstIntegerSet<EventValueType> &yes_set)
    {
        std::ostringstream oss;
        //oss << '\"';
        ConstIntegerSet<EventValueType>::iterator child = yes_set.begin();
        for (; child != yes_set.end(); child ++) {
            if (child != yes_set.begin())
                oss << ", ";
            if (key != kPdfClass) {
                std::string phone =
                        phone_syms_->Find(static_cast<kaldi::int64>(*child));
                if (phone.empty())
                    KALDI_ERR << "No phone found for Phone ID " << *child;
                oss << phone;
            }
            else {
                oss << *child;
            }
        }
        //oss << '\"';
        return oss.str();
    }

    const std::string kColor_;
    const std::string kTraceColor_;
    const kaldi::int32 kPen_;
    const kaldi::int32 kTracePen_; // GraphViz pen width for the "traced" path

    EventMap &root_; // the root of the tree
    const kaldi::int32 N; // context length
    const kaldi::int32 P; // central phone
    const EventType *event_; // the 'event' to be traced (0 means "don't trace")
    const fst::SymbolTable *phone_syms_;

    kaldi::int32 next_id_; // The next node id to be assigned
    kaldi::int32 parent_id_; // The id of the current node's parent
    std::string edge_attr_; // The attributes of the edge to current node from its parent
    bool path_active_; // True if the current node is traversed when tracing an event through the tree
}; // TreeRenderer

} // namespace kaldi

kaldi::EventType* MakeEvent(std::string &query,
                            kaldi::int32 N,
                            fst::SymbolTable *phone_syms,
                            kaldi::ParseOptions &po)
{
    using namespace kaldi;

    EventType *query_event = new EventType();
    size_t found, old_found = 0;
    EventKeyType key = -1;
    while ((found = query.find('/', old_found)) != std::string::npos) {
        std::string valstr = query.substr(old_found, found - old_found);
        EventValueType value;
        if (key == -1) {
            value = static_cast<EventValueType>(atoi(valstr.c_str()));
            if (value < 0 || value > 2) { // not valid 3-phone state
                std::cerr << "Bad query: invalid HMM state index ("
                          << valstr << ')' << std::endl << std::endl;
                return 0;
            }
        }
        else {
            value = static_cast<EventValueType>(phone_syms->Find(valstr.c_str()));
            if (value == fst::SymbolTable::kNoSymbol) {
                std::cerr << "Bad query: invalid symbol ("
                          << valstr << ')' << std::endl << std::endl;
                return 0;
            }
        }
        query_event->push_back(std::make_pair(key++, value));
        old_found = found + 1;
    }
    if ((N == 3 && query_event->size() != 3) ||
        (N == 1 && query_event->size() != 1)) {
        std::cerr << "Invalid query: " << query << std::endl;
        return 0;
    }
    std::string valstr = query.substr(old_found);
    EventValueType value = static_cast<EventValueType>(phone_syms->Find(valstr.c_str()));
    if (value == fst::SymbolTable::kNoSymbol) {
        std::cerr << "Bad query: invalid symbol ("
                  << valstr << ')' << std::endl << std::endl;
        return 0;
    }
    query_event->push_back(std::make_pair(key, value));

    return query_event;
}

int main(int argc, char **argv)
{
    using namespace kaldi;
    try {
        const char *usage =
                "Draws a phonetic states-tying tree using GraphViz\n"
                "The output is meant to be rendered in SVG (to see the tooltips)\n"
                "Usage: draw-tree [options] <phones-syms> <tree>\n\n";

        std::string query;
        ParseOptions po(usage);
        po.Register("query", &query, "Traces a mono/tri phone state through the tree(format: state/lc/c/rc)");
        po.Read(argc, argv);

        if (po.NumArgs() != 2) {
            po.PrintUsage();
            return 1;
        }

        std::string phnfile = po.GetArg(1);
        std::string treefile = po.GetArg(2);

        fst::SymbolTable *phones_symtab = NULL;
        {
            std::ifstream is(phnfile.c_str());
            phones_symtab = ::fst::SymbolTable::ReadText(is, phnfile);
            if (!phones_symtab)
                KALDI_ERR << "Could not read phones symbol table file "<< phnfile;
        }

        ContextDependency ctx_dep;  // the tree.
        {
          bool binary;
          Input ki(treefile, &binary);
          ctx_dep.Read(ki.Stream(), binary);
        }
        EventMap &root = const_cast<EventMap&>(ctx_dep.ToPdfMap());
        const kaldi::int32 P = ctx_dep.CentralPosition();
        const kaldi::int32 N = ctx_dep.ContextWidth();

        if (!((N == 3 && P == 1) || (N == 1 && P == 0))) {
            std::cerr << "Only monophone and triphone trees are supported\n";
            po.PrintUsage();
            return 1;
        }

        EventType *query_event = 0;
        if (!query.empty()) {
            query_event = MakeEvent(query, N, phones_symtab, po);
            if (query_event == 0) {
                po.PrintUsage();
                return 2;
            }
        }

        TreeRenderer renderer(root, phones_symtab, N, P);
        renderer.Render(query_event);

        return 0;
    }
    catch (std::exception& e) {
        KALDI_ERR << e.what();
    }

    return 0;
}
