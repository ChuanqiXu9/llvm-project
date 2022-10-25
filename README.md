Draft for implementing P1689R5 based on clang-trunk.

# Current status:

We're able to generate the P1689 format with clang-scan-deps if we write the compilation database manually. But the cmake generated compilation database don't contain the output field. So we can't run the codes actually in practice.
