#!/bin/bash

# Copyright 2012 Vassil Panayotov <vd.panayotov@gmail.com>

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
# WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
# MERCHANTABLITY OR NON-INFRINGEMENT.
# See the Apache 2 License for the specific language governing permissions and
# limitations under the License.

echo "--- Starting training graph visualization ..."  

scriptdir=$1
picdir=$2
stage=$3

mkdir -p $scriptdir
mkdir -p $picdir

# Prepare the input/output Kaldi scripts

# The ID and transcription for the chosen training utterance
utt_id="trn_adg04_st1350"
utt_transcript="314 425 17"

# The transcription for "trn_adg04_st1350" is "FIND INDEPENDENCE+S ALERTS"
echo "$utt_id $utt_transcript" > $scriptdir/in.ark

# Final graph
echo "$utt_id $scriptdir/train.fst" > $scriptdir/out.scp

# LG graph
echo "$utt_id $scriptdir/lg_train.fst" > $scriptdir/lg_out.scp

# CLG graph
echo "$utt_id $scriptdir/clg_train.fst" > $scriptdir/clg_out.scp

# HCLG before optimization(determinization, eps removal, minimization)
# and before the self-loops are added
echo "$utt_id $scriptdir/hclg_noopt_train.fst" > $scriptdir/hclg_noopt_out.scp

model="final.mdl"
#if [ $stage == "mono" ]; then
#    model="0.mdl"
#fi

# Compile the train graphs and output the intermediate WFSTs
compile-train-graphs-vis --batch-size=1 exp/$stage/tree exp/$stage/$model  data/L.fst ark:$scriptdir/in.ark scp:$scriptdir/out.scp  scp:$scriptdir/lg_out.scp scp:$scriptdir/clg_out.scp scp:$scriptdir/hclg_noopt_out.scp

# Prepare the symbol tables
phnsymtab="data/phones_disambig.txt"

fstmakecontextsyms $phnsymtab exp/graph_$stage/ilabels > $scriptdir/context_syms.txt

fstmaketidsyms $phnsymtab exp/$stage/$model > $scriptdir/transid_syms.txt

# Render the graphs

# LG
fstdraw --portrait=true --isymbols=$phnsymtab --osymbols=data/words.txt $scriptdir/lg_train.fst | dot -Tpdf > $picdir/LG_train.pdf

# CLG
fstdraw --portrait=true --isymbols=$scriptdir/context_syms.txt --osymbols=data/words.txt $scriptdir/clg_train.fst | dot -Tpdf > $picdir/CLG_train.pdf

# HCLG non-optimized and no self-loops 
fstdraw --portrait=true --isymbols=$scriptdir/transid_syms.txt --osymbols=data/words.txt $scriptdir/hclg_noopt_train.fst | dot -Tpdf > $picdir/HCLG_noopt_train.pdf

# The final HCLG graph
fstdraw --portrait=true --isymbols=$scriptdir/transid_syms.txt --osymbols=data/words.txt $scriptdir/train.fst | dot -Tpdf > $picdir/train.pdf

echo "--- Done training graph visualization!"