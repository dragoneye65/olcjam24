#!/bin/bash

emsdk="~/emsdk"
cmd="em++ -std=c++20 -O2 -s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s USE_LIBPNG=1 -sUSE_SDL_MIXER=2 hover.cpp -o index.html --preload-file ./res"

echo "source $emsdk/emsdk_env.sh and run the compile command for ya..."
ln -s ../src/hover.cpp .
ln -s ../inc/miniaudio.h
ln -s ../inc/olcPGEX_SplashScreen.h .
ln -s ../inc/olcPGEX_MiniAudio.h .
ln -s ../inc/olcPixelGameEngine.h .
bash -c "source $emsdk/emsdk_env.sh && $cmd"
cp ../Doc/javids_index.html index.html
rm olcjam24.zip
zip olcjam24.zip index.data index.html index.js index.wasm


