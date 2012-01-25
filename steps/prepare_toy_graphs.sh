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


# The script was modified to work with a toy dictionary and grammar and produce
# the corresponding graphs (for didactic purpose).

# The output of this script is the symbol tables data/{words.txt,phones.txt},
# and the grammars and lexicons data/{L,G}{,_disambig}.fst

echo "--- Preparing the toy graphs ..."

mkdir -p data_toy
scripts/make_rm_lm.pl data_prep/wp_toy.txt > data_toy/G.txt
scripts/make_words_symtab.pl < data_toy/G.txt > data_toy/words.txt
egrep '^((NOT)|(KNOT )|(SHIP )|(FIVE)|(!SIL))' data_prep/lexicon.txt > data_toy/lexicon.txt

scripts/make_phones_symtab.pl < data_toy/lexicon.txt > data_toy/phones.txt

silphones="sil"; # This would in general be a space-separated list of all silence phones.  E.g. "sil vn"
# Generate colon-separated lists of silence and non-silence phones.
scripts/silphones.pl data_toy/phones.txt "$silphones" data_toy/silphones.csl data_toy/nonsilphones.csl

ndisambig=`scripts/add_lex_disambig.pl data_toy/lexicon.txt data_toy/lexicon_disambig.txt`
ndisambig=$[$ndisambig+1]; # add one disambig symbol for silence in lexicon FST.
scripts/add_disambig.pl data_toy/phones.txt $ndisambig > data_toy/phones_disambig.txt

# Create train transcripts in integer format: 
#(For now this is not needed for the toy example)
#cat data_prep/train_trans.txt | \
#  scripts/sym2int.pl --ignore-first-field data/words.txt  > data/train.tra

# Get lexicon in FST format.

# silprob = 0.5: same prob as word.
scripts/make_lexicon_fst.pl data_toy/lexicon.txt 0.5 sil  | fstcompile --isymbols=data_toy/phones.txt --osymbols=data_toy/words.txt --keep_isymbols=false --keep_osymbols=false | fstarcsort --sort_type=olabel > data_toy/L.fst

cat data_toy/lexicon.txt | awk '{printf("%s #1 ", $1); for (n=2; n <= NF; n++) { printf("%s ", $n); } print "#2"; }' | \
 scripts/make_lexicon_fst.pl - 0.5 sil | fstcompile --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --keep_isymbols=false --keep_osymbols=false | fstarcsort --sort_type=olabel > data_toy/L_align.fst

scripts/make_lexicon_fst.pl data_toy/lexicon_disambig.txt 0.5 sil '#'$ndisambig | fstcompile --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --keep_isymbols=false --keep_osymbols=false | fstarcsort --sort_type=olabel > data_toy/L_disambig.fst

fstcompile --isymbols=data_toy/words.txt --osymbols=data_toy/words.txt --keep_isymbols=false --keep_osymbols=false data_toy/G.txt > data_toy/G.fst

# Checking that G is stochastic [note, it wouldn't be for an Arpa]
fstisstochastic data_toy/G.fst || echo Error

# Checking that disambiguated lexicon times G is determinizable
fsttablecompose data_toy/L_disambig.fst data_toy/G.fst | fstdeterminize >/dev/null || echo Error

# Checking that LG is stochastic:
fsttablecompose data_toy/L.fst data_toy/G.fst | fstisstochastic || echo Error

## Check lexicon.
## just have a look and make sure it seems sane.
fstprint   --isymbols=data_toy/phones.txt --osymbols=data_toy/words.txt data_toy/L.fst  | head

# Make some pretty pictures
PICDIR=data_toy/pictures
mkdir $PICDIR

cat data_toy/G.fst | fstdraw --isymbols=data_toy/words.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/G.pdf

cat data_toy/L.fst | fstdraw --portrait=false --isymbols=data_toy/phones.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/L.pdf

cat data_toy/L_align.fst | fstdraw --portrait=false --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/L_align.pdf

cat data_toy/L_disambig.fst | fstdraw --portrait=false --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/L_disambig.pdf

# Underterminized LG compositions
fsttablecompose data_toy/L_disambig.fst data_toy/G.fst | fstdraw --portrait=false --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/LdG.pdf

fsttablecompose data_toy/L.fst data_toy/G.fst | fstdraw --portrait=false --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/LG.pdf

# Determinized LG compositions
fsttablecompose data_toy/L_disambig.fst data_toy/G.fst | fstdeterminize |  fstdraw --portrait=false --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/DET_LdG.pdf

fsttablecompose data_toy/L.fst data_toy/G.fst | fstdeterminize | fstdraw --portrait=false --isymbols=data_toy/phones_disambig.txt --osymbols=data_toy/words.txt --portrait=true | dot -Tpdf > $PICDIR/DET_LG.pdf

echo "--- Done toy graph preparation!"