To compile with Emscripten tools:
em++ -O2 --bind FaceWASM.cpp face.cpp -o FaceWASM.js --preload-file models/ -I include libopencv_world.bc libzlib.bc -s ALLOW_MEMORY_GROWTH=1 -s WASM=1