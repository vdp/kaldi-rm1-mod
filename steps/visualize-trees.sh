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

echo "--- Starting tree visualization ($2) ..."  

picdir=$1
stage=$2
traceq=$3

draw-tree --query=$traceq data/phones.txt exp/$stage/tree | \
  dot -Tjpg > $picdir/tree.jpg

echo "--- Done tree visualization ($stage) !"