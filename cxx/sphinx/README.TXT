This directory contains a source file than can be used to "package"
a set of Sphinx feature files, given as a Kaldi script(.scp), into
Kaldi arhive.

Example usage:
./pack-sphinx-feats --verbose=2 scp:train.scp ark,scp:test/train.ark,test/train.scp

, where train.scp entries look like:
trn_adg04_sr089 /home/vassil/devel/speech/datasets/rm1/rm1_feats/feat/adg0_4/sr089.mfc
trn_adg04_sr249 /home/vassil/devel/speech/datasets/rm1/rm1_feats/feat/adg0_4/sr249.mfc
trn_adg04_sr289 /home/vassil/devel/speech/datasets/rm1/rm1_feats/feat/adg0_4/sr289.mfc
...


In order to compile, place 'pack-sphinx-feat.cc' in kaldi/src/featbin and make the
following change in the Makefile found in that directory:

---------------------------------------------------------
diff --git a/src/featbin/Makefile b/src/featbin/Makefile
index cf0ac8a..81204b4 100644
--- a/src/featbin/Makefile
+++ b/src/featbin/Makefile
@@ -5,7 +5,7 @@ include ../kaldi.mk
 
 BINFILES = compute-mfcc-feats compute-plp-feats compute-cmvn-stats add-deltas \
        remove-mean apply-cmvn transform-feats copy-feats compose-transforms \
-    splice-feats extract-segments subset-feats feat-to-len feat-to-dim
+    splice-feats extract-segments subset-feats feat-to-len feat-to-dim pack-sphinx-feats
 
 
 OBJFILES =
---------------------------------------------------------

i.e. just add "pack-sphinx-feats" to "$BINFILES".
The simply run 'make'.