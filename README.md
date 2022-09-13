Draft for implementing P1689R5 based on clang-trunk.

# Current status:

For each TU, now we can generate the direct dependency information for named modules as the P1689R5 described. We can found the result from test/Modules/P1689.cpp.

Also for the implementation (non partition) unit, currently it will provide nothing and require the corresponding primary module interface correctly.

# Possible improvement?
- In the section of `Example scanning output`, it shows

```json
{
  "version": 1,
  "revision": 0,
  "rules": [
    {
      "primary-output": "duplicate.mpp.o",
      "provides": [
        {
          "logical-name": "duplicate"
        }
      ]
    },
    {
      "primary-output": "another.mpp.o",
      "provides": [
        {
          "logical-name": "another"
        }
      ],
      "requires": [
        {
          "logical-name": "duplicate"
        }
      ]
    },
    {
      "primary-output": "use.mpp.o",
      "requires": [
        {
          "logical-name": "duplicate"
        },
        {
          "logical-name": "another"
        }
      ]
    }
  ]
}
```

So it looks like the output should be for the project instead of the each TU.
But I am wondering if it will be better to merge them outside the compiler.
