# Prolix

**Prolix plays _shatranj_, not chess**.

## Compiling
The same command as used by OpenBench:

```make EXE=(executable name) EVALFILE=(path to NNUE file)```

Prolix compiles using `clang++`, though in theory `g++` should also be supported.

## NNUE
You can download NNUE files from [Google Drive](https://drive.google.com/drive/folders/1d4HROM-7nbSpkQGt4e4TMEcmcVa00XoE?usp=sharing)

The default NNUE file is indicated in the Makefile. If you wish to compile with a different file (provided it is in standard Bullet format), please ensure that the constants in `nnue.cpp` match the architecture of the net.

## Syzygy Tablebases
Prolix optionally includes support for (Shatranj) Syzygy tablebases (downloadable at ftp://chessdb:chessdb@ftp.chessdb.cn/pub/syzygy/shatranj/). Currently only WDL probing is supported. Using tablebases currently heavily reduces mate finding ability.

## How to read evaluations
A score of `+270.00` or `mate x` indicates a forced win. The exact mate distance may not be accurate.

A score of `+259.xy` or `+260.00` indicates a tablebase win. This does not take 70 move rule into account.

All other evaluations are **normalized**. A score of `+1.00` corresponds to an estimated win probability of 50%.
