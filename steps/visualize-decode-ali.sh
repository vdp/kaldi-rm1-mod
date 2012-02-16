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

echo "--- Starting decoding visualization ($2) ..."  

picdir=$1
stage=$2
feats="ark:add-deltas scp:data_toy/test.scp ark:- |"
workdir=exp/decode_${stage}_toy

mkdir -p $picdir

# The key for the "toy" test utterance
utt=toy

scripts/mkgraph_toy.sh --$stage exp/$stage/tree exp/$stage/final.mdl $workdir $picdir

gmm-decode-faster --beam=20.0 --acoustic-scale=0.08333 --word-symbol-table=data_toy/words.txt exp/$stage/final.mdl $workdir/HCLG.fst "$feats" ark,t:$workdir/toy.tra ark,t:$workdir/toy.ali

# Print the result
echo
echo "Decoder output: " 
scripts/int2sym.pl --ignore-first-field data_toy/words.txt $workdir/toy.tra
echo

# Render the pictures
echo "$utt $workdir/HCLG.fst" | \
  draw-ali --key=$utt data_toy/phones_disambig.txt data_toy/words.txt exp/$stage/final.mdl ark,t:$workdir/toy.ali scp:- | dot -Tpdf > $picdir/decode_ali.pdf

echo "--- Done decoding visualization ($stage) !"
echo