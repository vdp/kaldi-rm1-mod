#!/bin/bash

# Copyright 2010-2011 Microsoft Corporation

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

#exit 1 # Don't run this... it's to be run line by line from the shell.

# This script file cannot be run as-is; some paths in it need to be changed
# before you can run it.
# Search for /path/to.
# It is recommended that you do not invoke this file from the shell, but
# run the paths one by one, by hand.

# the step in data_prep/ will need to be modified for your system.

# First step is to do data preparation:
# This just creates some text files, it is fast.
# If not on the BUT system, you would have to change run.sh to reflect
# your own paths.
#

#Example arguments to run.sh: /mnt/matylda2/data/RM, /ais/gobi2/speech/RM, /cygdrive/e/data/RM
# RM is a directory with subdirectories rm1_audio1, rm1_audio2, rm2_audio
cd data_prep
#*** You have to change the pathname below.***
RM1_ROOT=/home/vassil/devel/speech/datasets/rm1
./run.sh $RM1_ROOT
cd ..

mkdir -p data
# orig
#( cd data; cp ../data_prep/{train,test*}.{spk2utt,utt2spk} . ; cp ../data_prep/spk2gender.map . )
# /orig
( cd data; cp ../data_prep/{train,test}.{spk2utt,utt2spk} . )

# This next step converts the lexicon, grammar, etc., into FST format.
steps/prepare_graphs.sh

# Next, make sure that "exp/" is someplace you can write a significant amount of
# data to (e.g. make it a link to a file on some reasonably large file system).
# If it doesn't exist, the scripts below will make the directory "exp".

# mfcc should be set to some place to put training mfcc's
# where you have space.  Make sure you create the directory.
#e.g.: mfccdir=/mnt/matylda6/jhu09/qpovey/kaldi_rm_mfccb
# Note: mfccdir should be an absolute pathname
mfccdir=./mfcc
steps/make_mfcc.sh $mfccdir train
steps/make_mfcc.sh $mfccdir test

steps/train_mono.sh
steps/decode_mono.sh 

steps/train_tri1.sh
steps/decode_tri1.sh
steps/decode_tri1_latgen.sh
steps/decode_tri1_latoracle.sh

# putting here in case anyone needs ctm output.
scripts/make_ctms.sh exp/tri1 exp/decode_tri1

steps/train_tri2a.sh
steps/decode_tri2a.sh

exit 0
