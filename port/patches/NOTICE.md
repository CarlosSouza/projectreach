# SRW patch provenance

`srw-macos-llasm.patch` applies to M-HT/SR revision `ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3`, SRW directory. It changes build selection, standard allocation includes and Darwin compiler/runtime selection, and (2026-09-26) adds the standard OLEAUT32 ordinals 8 (`VariantInit`) and 9 (`VariantClear`) to the loader's import-by-ordinal table, which Halo's executable uses. It contains context from upstream MIT-licensed files. Ignored source/build copies retain all upstream notices, including the separately licensed bundled udis86 files.

Copyright (C) 2016-2025 Roman Pauer

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This notice does not license Halo input, translated game code, UTP, or the overall HaloPad project for distribution. Public release remains gated separately.
