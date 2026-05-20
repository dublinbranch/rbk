# rbk unit tests (Boost.Test)

- **`rbk/BoostJson/test/`** — Boost.JSON helpers (`boostjson_*`), `boostjson_intrusive.cpp` (`try_value_to` + `intrusivedebug.h` include order like application config), shared Boost.Test module (`boost_test_main.cpp`), and **`CMakeLists.txt`** for that object library — next to `rbk/BoostJson` sources.
- **`rbk/test/`** — top-level test target (`rbk_tests`) and non–Boost.JSON suites (e.g. `filefunction.cpp`).

Tests are built only when **`RBK_BUILD_TESTS=ON`**. From the `rbk` repository root that is the default when you configure this tree as the **top-level** CMake project; when `rbk` is pulled in with `add_subdirectory`, tests default to **off** unless you enable them.

Use **one** out-of-tree build directory for the static library, **`rbk_tests`**, and every test translation unit — no separate CMake trees per suite or per test file.

## Configure

You need the same dependencies as the library (Qt6 Core, Boost including **unit_test_framework**, fmt, MariaDB client, etc.), matching your normal `rbk` configure.

**`rbk` as CMake top-level** (shell cwd is the `rbk` repository root):

```bash
cmake -B build -DRBK_BUILD_TESTS=ON
```

**`rbk` pulled in by a parent project** (shell cwd is the parent that contains the `rbk/` folder):

```bash
cmake -B build -DRBK_BUILD_TESTS=ON
```

(Use the same `-B build` you use for the parent; artifact paths follow that project’s layout.)

## Build the test binary

From the same tree you configured (examples below assume `-B build` and `rbk` as top-level):

```bash
cmake --build build --target rbk_tests
```

When `rbk` is top-level, the executable is **`build/test/rbk_tests`**.

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
