// A smoke test that checks we didn't replace the generated thin BMI if only the non-inline
// function bodies changes.
//
// RUN: rm -rf %t
// RUN: mkdir %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++20 %t/a.cppm -emit-thin-module-interface -o %t/a.pcm
// RUN: md5sum %t/a.pcm > %t/a.pcm.md5sum
// RUN: %clang_cc1 -std=c++20 %t/a.v1.cppm -emit-thin-module-interface -o %t/a.pcm
// RUN: md5sum %t/a.pcm > %t/a.v1.pcm.md5sum
//
// RUN: diff %t/a.pcm.md5sum %t/a.v1.pcm.md5sum

//--- a.cppm
export module a;
export int v = 43;
export int a() {
    return 43;
}

unsigned int non_exported() {
    return v;
}

//--- a.v1.cppm
export module a;
export int v = 45;
export int a() {
    return 44;
}

unsigned int non_exported() {
    return v + 43;
}
