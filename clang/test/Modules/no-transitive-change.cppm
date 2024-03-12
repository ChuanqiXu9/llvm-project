// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: cd %t
//
// RUN: %clang_cc1 -std=c++20 %t/A.cppm -emit-reduced-module-interface -o %t/A.pcm
// RUN: %clang_cc1 -std=c++20 %t/B.cppm -emit-reduced-module-interface -fmodule-file=A=%t/A.pcm \
// RUN:     -o %t/B.pcm
//
// RUN: %clang_cc1 -std=c++20 %t/A.v1.cppm -emit-reduced-module-interface -o %t/A.v1.pcm
// RUNX: not diff %t/A.pcm %t/A.new.pcm &> /dev/null
// RUN: %clang_cc1 -std=c++20 %t/B.cppm -emit-reduced-module-interface -fmodule-file=A=%t/A.v1.pcm \
// RUN:     -o %t/B.new.pcm
// RUN: diff %t/B.new.pcm %t/B.pcm  &> /dev/null

//--- A.cppm
export module A;
export int funcA() {
    return 43;
}

//--- A.v1.cppm
export module A;
export int funcA() {
    return 43;
}

//--- B.cppm
export module B;
import A;
export int funcB() {
    return 45;
}
