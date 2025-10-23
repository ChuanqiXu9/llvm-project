// RUN: %clang_cc1 -std=c++20 %s -triple %itanium_abi_triple -O3 -emit-llvm -disable-llvm-passes  -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++20 %s -triple %itanium_abi_triple -O3 -emit-llvm -o - | FileCheck %s --check-prefix=CHECK-LLVM

using size_t = unsigned long;
using int64_t = long;

class Base {
public:
    virtual int64_t get(size_t i) const = 0;
    virtual void getBatch(size_t offset, size_t len, int64_t arr[]) const {
        for (size_t i = 0; i < len; ++i) {
            arr[i] = get(offset + i);
        }
    }
};

class Derived1 final : public Base {
public:
    int64_t get(size_t i) const override {
        return 0;
    }
    
    void getBatch(size_t offset, size_t len, int64_t arr[]) const override;
};

void Derived1::getBatch(size_t offset, size_t len, int64_t arr[]) const {
    Base::getBatch(offset, len, arr);
}

// CHECK: define{{.*}} void @_ZNK8Derived18getBatchEmmPl(ptr {{[^,]*}} %[[This:[a-zA-Z0-9_.]*]],
// CHECK: %[[LoadedVtable:[a-zA-Z0-9_.]*]] = load ptr, ptr %[[This]]{{.*}} !invariant.load
// CHECK: %[[CMP:.*]] = icmp eq ptr %vtable, {{.*}}@_ZTV8Derived1
// CHECK: call {{.*}}void @llvm.assume(i1 %[[CMP]])

// CHECK-LLVM: define{{.*}} void @_ZNK8Derived18getBatchEmmPl(
// CHECK-LLVM: call {{.*}}void @llvm.memset.p0.i64

