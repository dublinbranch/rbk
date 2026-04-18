# rbk unit tests (Boost.Test)

Tests are built only when **`RBK_BUILD_TESTS=ON`**. From the `rbk` repository root that is the default when you configure this tree as the **top-level** CMake project; when `rbk` is pulled in with `add_subdirectory`, tests default to **off** unless you enable them.

## Configure

From the directory that contains `rbk` (often the repo root, where `CMakeLists.txt` lives):

```bash
cmake -B build -DRBK_BUILD_TESTS=ON
```

If `rbk` lives inside another project, add:

```bash
cmake -B build -DRBK_BUILD_TESTS=ON /path/to/parent
```

You need the same dependencies as the library (Qt6 Core, Boost including **unit_test_framework**, fmt, MariaDB client, etc.), matching your normal `rbk` configure.

## Build the test binary

```bash
cmake --build build --target rbk_tests
```

The executable is **`build/test/rbk_tests`** (path may vary if you use a different build directory name).

## Run tests

**Using CTest** (recommended; runs the registered test):

```bash
ctest --test-dir build --output-on-failure
```

Run only this project’s suite:

```bash
ctest --test-dir build -R rbk_tests --output-on-failure
```

**Run the binary directly** (Boost.Test CLI; useful for filters):

```bash
./build/test/rbk_tests
./build/test/rbk_tests --help
./build/test/rbk_tests --run_test=boostjson_parse
```

## Troubleshooting

- **`Could NOT find Boost (missing: unit_test_framework)`** — install Boost’s test library (on many distributions the package is split from the headers), or point CMake at your Boost prefix.
- **Tests not configured** — pass **`-DRBK_BUILD_TESTS=ON`** and re-run CMake from scratch if needed (`cmake -B build` again).
