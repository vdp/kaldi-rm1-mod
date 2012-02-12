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

echo "--- Starting training alignment visualization ($2) ..."  

picdir=$1
stage=$2
dir=exp/$2
niters=30 # should match the #of iteration in the training script
show_tids=false # make it true if you want to also see the trans-ids on input labels
draw_iters=$3 # should correspond to a subset of the realignment passes

mkdir -p $picdir

# The of the chosen training utterance
utt=trn_adg04_st1350

# Visualize the alignment at some passes
# (add --show-tids=true if you want transition id to be shown too)
x=0
while [ $x -lt $niters ]; do
    if echo $draw_iters | egrep -w $x > /dev/null; then
	draw-ali --key=$utt --show-tids=$show_tids data/phones_disambig.txt\
           data/words.txt $dir/$x.mdl ark:$dir/$x.ali \
           "ark:gunzip -c $dir/graphs.fsts.gz |" | \
           dot -Tpdf > $picdir/${utt}_ali_${x}.pdf
    fi
    x=$[$x + 1]
done


echo "--- Done training alignment visualization ($stage) !"