// Test that instrumentation based profiling feeds branch prediction
// correctly. This tests both generation of profile data and use of the same,
// and the input file for the -fprofile-instr-use case is expected to be result
// of running the program generated by the -fprofile-instr-generate case. As
// such, main() should call every function in this test.

// FIXME: Don't seek bb labels, like "if.else"
// REQUIRES: asserts

// RUN: %clangxx %s -o - -emit-llvm -S -fprofile-instr-generate -target %itanium_abi_triple | FileCheck -check-prefix=PGOGEN %s
// RUN: %clangxx %s -o - -emit-llvm -S -fprofile-instr-generate -target %itanium_abi_triple | FileCheck -check-prefix=PGOGEN-EXC %s

// RUN: %clang %s -o - -emit-llvm -S -fprofile-instr-use=%S/Inputs/instr-profile-throws.pgodata -target %itanium_abi_triple | FileCheck -check-prefix=PGOUSE %s
// RUN: %clang %s -o - -emit-llvm -S -fprofile-instr-use=%S/Inputs/instr-profile-throws.pgodata -target %itanium_abi_triple | FileCheck -check-prefix=PGOUSE-EXC %s

// PGOGEN: @[[THC:__llvm_pgo_ctr[0-9]*]] = private global [11 x i64] zeroinitializer
// PGOGEN-EXC: @[[THC:__llvm_pgo_ctr[0-9]*]] = private global [11 x i64] zeroinitializer

// PGOGEN-LABEL: @_Z6throwsv()
// PGOUSE-LABEL: @_Z6throwsv()
// PGOGEN: store {{.*}} @[[THC]], i64 0, i64 0
void throws() {
  // PGOGEN: store {{.*}} @[[THC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[TH1:[0-9]+]]
  for (int i = 0; i < 100; ++i) {
    try {
      // PGOGEN: store {{.*}} @[[THC]], i64 0, i64 5
      // PGOUSE: br {{.*}} !prof ![[TH2:[0-9]+]]
      if (i % 3) {
        // PGOGEN: store {{.*}} @[[THC]], i64 0, i64 6
        // PGOUSE: br {{.*}} !prof ![[TH3:[0-9]+]]
        if (i < 50)
          throw 1;
      } else {
        // The catch block may be emitted after the throw above, we can skip it
        // by looking for an else block, but this will break if anyone puts an
        // else in the catch
        // PGOUSE: if.else{{.*}}:
        // PGOGEN: if.else{{.*}}:

        // PGOGEN: store {{.*}} @[[THC]], i64 0, i64 7
        // PGOUSE: br {{.*}} !prof ![[TH4:[0-9]+]]
        if (i >= 50)
          throw 0;
      }
    } catch (int e) {
      // PGOUSE-EXC: catch{{.*}}:
      // PGOGEN-EXC: catch{{.*}}:

      // PGOGEN-EXC: store {{.*}} @[[THC]], i64 0, i64 8
      // PGOGEN-EXC: store {{.*}} @[[THC]], i64 0, i64 9
      // PGOUSE-EXC: br {{.*}} !prof ![[TH5:[0-9]+]]
      if (e) {}
    }
    // PGOGEN: store {{.*}} @[[THC]], i64 0, i64 4

    // PGOGEN: store {{.*}} @[[THC]], i64 0, i64 10
    // PGOUSE: br {{.*}} !prof ![[TH6:[0-9]+]]
    if (i < 100) {}
  }

  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
  // PGOUSE: ret void
}

// PGOUSE-DAG: ![[TH1]] = metadata !{metadata !"branch_weights", i32 101, i32 2}
// PGOUSE-DAG: ![[TH2]] = metadata !{metadata !"branch_weights", i32 67, i32 35}
// PGOUSE-DAG: ![[TH3]] = metadata !{metadata !"branch_weights", i32 34, i32 34}
// PGOUSE-DAG: ![[TH4]] = metadata !{metadata !"branch_weights", i32 18, i32 18}
// PGOUSE-EXC: ![[TH5]] = metadata !{metadata !"branch_weights", i32 34, i32 18}
// PGOUSE-DAG: ![[TH6]] = metadata !{metadata !"branch_weights", i32 101, i32 1}

int main(int argc, const char *argv[]) {
  throws();
  return 0;
}