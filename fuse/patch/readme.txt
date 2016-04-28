Let's say you want to patch distribution file X, which will be unpacked to
directory W[../W], relative to the top directory.

1) Copy W[../W]/X to patch/X and make requisite changes.

2) From top-level directory, do "diff -c W[../W]/X patch/X >| patch/X.patch"

3) From top-level directory, do a normal make. It should automatically perform
"patch -d W -p1 -N < patch/X.patch" 

4) Verify W[../W]/X was correctly patched, then you can delete the interim
copy of patch/X and check in patch/X.patch.
