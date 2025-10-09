# Prolix

**Prolix plays _shatranj_, not chess**.

## Compiling
The same command as used by OpenBench:

```make EXE=(executable name) EVALFILE=(path to NNUE file)```

Prolix compiles using `clang++`, though in theory `g++` should also be supported.

## NNUE
On compilation, Prolix embeds a NNUE file into the executable using [incbin](https://github.com/graphitemaster/incbin), which becomes the default NNUE file used for evaluation. You can download NNUE files from [Google Drive](https://drive.google.com/drive/folders/1d4HROM-7nbSpkQGt4e4TMEcmcVa00XoE?usp=sharing).

Prolix NNUE is trained on [bullet-legacy](https://github.com/jw1912/bullet/tree/legacy) using original data (the Google Drive contains more information regarding data). The default (and usually strongest) NNUE file's name is indicated in the Makefile. If you wish to compile with a different file, please ensure that the constants in `src/eval/arch.h` match the architecture of the net, and that the weights are binary encoded in the same way bullet-legacy would encode them.

## Syzygy Tablebases
Prolix optionally includes support for (Shatranj) Syzygy tablebases (downloadable at ftp://chessdb:chessdb@ftp.chessdb.cn/pub/syzygy/shatranj/), using probing code of [Ronald De Man](https://github.com/syzygy1/probetool). Currently only WDL probing is supported, and I have only tested 5-men and below. Using tablebases also reduces mate finding ability.

## How to read evaluations
A score of `+270.00` or `mate x` indicates a forced win. The exact mate distance may not be accurate.

A score of `+259.xy` or `+260.00` indicates a tablebase win. This does not take 70 move rule into account.

All other evaluations are **normalized**. A score of `+1.00` corresponds to an estimated win probability of 50%.
The UCI_ShowWDL option, which is on by default (though not all GUIs display it), provides additional information.