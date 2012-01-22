Some trivial modifications of compile-train-graphs and training-graph-compiler
to visualize the intermediate steps in training graphs composition.

In order to incorporate the modified compile-train-graphs-vis tool into Kaldi's
build infrastructure the following changes are needed in Makefiles:
diff --git a/src/bin/Makefile b/src/bin/Makefile
index 28a3af1..b021da6 100644
--- a/src/bin/Makefile
+++ b/src/bin/Makefile
@@ -12,7 +12,7 @@ BINFILES = align-equal align-equal-compiled acc-tree-stats \
         ali-to-pdf est-mllt build-tree build-tree-two-level decode-faster \
         decode-faster-mapped scale-vecs copy-transition-model rand-prune-post \
         phones-to-prons prons-to-wordali copy-gselect copy-tree scale-post \
-        compute-mce-scale
+        compute-mce-scale compile-train-graphs-vis
 
 OBJFILES = 
 
diff --git a/src/decoder/Makefile b/src/decoder/Makefile
index 83000da..794b9ef 100644
--- a/src/decoder/Makefile
+++ b/src/decoder/Makefile
@@ -8,7 +8,7 @@ include ../kaldi.mk
 #TESTFILES =  kaldi-decoder-test
 TESTFILES = 
 
-OBJFILES = decodable-am-diag-gmm.o training-graph-compiler.o decodable-am-sgmm.o decodable-am-tied-diag-gmm.o decodable-am-tied-full-gmm.o
+OBJFILES = decodable-am-diag-gmm.o training-graph-compiler.o decodable-am-sgmm.o decodable-am-tied-diag-gmm.o decodable-am-tied-full-gmm.o training-graph-compiler-vis.o
 
 LIBFILE = kaldi-decoder.a
 
Then copy the training-graph-compiler-vis.* to src/decoder and compile-train-graphs-vis.cc to
src/bin. Finally run 'make' first in 'src/decoder', and then in 'src/bin' directory to compile.
 