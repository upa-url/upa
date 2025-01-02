# Upa URL demo source code

The Upa URL demo is available online at https://upa-url.github.io/demo/

How to prepare for the Web:
1. Install and activate Emscripten, more information: https://emscripten.org/docs/getting_started/downloads.html
2. Build `url-api.cpp` with `build-url-api.sh` (`build-url-api.bat` on Windows) script. Output files `url-api.js` and `url-api.wasm` will be written to the `www` directory.
3. Optionally, the Public Suffix List file `public_suffix_list.dat` can be downloaded from https://publicsuffix.org/list/ into the `www` directory. Then the [registrable domain](https://url.spec.whatwg.org/#host-registrable-domain) of the parsed URL host will be displayed.
4. Publish contents of `www` directory to web server.
