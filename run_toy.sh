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

# A top-level script to guide the process of building the
# toy WFST cascade. Run _after_ the training is completed,
# (i.e. start after './run.sh' is done)  

if [ -f path.sh ]; then . path.sh; fi

mkdir -p data_toy
steps/prepare_toy_graphs.sh

# Visualize the stages in a training graph's composition (monophone and triphone)
steps/visualize-train-graphs.sh data_toy/train/mono data_toy/pictures/train/mono mono 
steps/visualize-train-graphs.sh data_toy/train/tri1 data_toy/pictures/train/tri1 tri1

# Visualization of the alignments for some utterance during some training passes
steps/visualize-train-ali.sh data_toy/pictures/train/mono mono "0 1 5 10 20" 
steps/visualize-train-ali.sh data_toy/pictures/train/tri1 tri1 "5 10 20"

# Draw some phonetic trees
steps/visualize-trees.sh data_toy/pictures/train/mono mono "1/k"
steps/visualize-trees.sh data_toy/pictures/train/tri1 tri1 "0/ax/k/ch"

# Visualize the decoding graphs/alignments
# (the features were extracted using the default parameters of
#  "wave2feat" tool from SphinxTrain)
