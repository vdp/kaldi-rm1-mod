This program produces symbolic names for the transition IDs that are used as
input labels in the final(HCLG) cascade.
Sample usage:
fstmaketidsyms data/phones.txt exp/mono/final.mdl

Use --verbose-output=true to make it write somewhat more explicit info
to stderr.

To compile copy fstmaketidsyms to kaldi/src/fstbin and make the following
changes to the Makefile:

---

diff --git a/src/fstbin/Makefile b/src/fstbin/Makefile
index 7bcb7d2..fb1ea54 100644
--- a/src/fstbin/Makefile
+++ b/src/fstbin/Makefile
@@ -19,7 +19,7 @@ BINFILES = fstpredeterminize fstdeterminizestar  \
            fstmakecontextsyms fstaddsubsequentialloop fstaddselfloops  \
            fstrmepslocal fstcomposecontext fsttablecompose fstrand fstfactor \
            fstdeterminizelog fstreorder fstremoveuselessarcs \
-           fstphicompose fstpropfinal
+           fstphicompose fstpropfinal fstmaketidsyms
 
 # actually, this library is currently empty.  Everything is a header.
 LIBFILE = 
@@ -27,7 +27,7 @@ LIBFILE =
 all: $(BINFILES)
 
 
-$(BINFILES): ../base/kaldi-base.a ../util/kaldi-util.a
+$(BINFILES): ../hmm/kaldi-hmm.a ../tree/kaldi-tree.a ../matrix/kaldi-matrix.a ../base/kaldi-base.
 
 %.a:
        $(MAKE) -C ${@D} ${@F}

---

Note that the additional libs will be linked also to the other programs in
src/fstbin.