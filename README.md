# rbk

A modular C++26/Qt utility library collection -- the backbone for [digitalSpine](https://github.com/dublinbranch) projects.

Designed to be included as a submodule via **CMake** or **qmake**:

```cmake
# CMake (recommended)
add_subdirectory(rbk)
target_link_libraries(myapp PRIVATE rbk::rbk)
```

```qmake
# qmake (legacy, still supported)
include(rbk/rbk.pri)
```

## At a Glance

| Area | Modules | What it does |
|------|---------|--------------|
| **Database** | `minMysql`, `BoostMysql` | Thin MariaDB layer (SQL composition, result mapping, schema checks, TTL cache, TLS), optional ClickHouse and Boost.MySQL |
| **HTTP / Web** | `HTTP`, `minCurl`, `mustache` | FCGI gateway, URL/MIME utils, optional Boost.Beast server + router, libcurl wrapper, Mustache templates |
| **JSON** | `BoostJson`, `JSON`, `rapidjson` | Boost.JSON extensions (`tag_invoke`, subset checks, math), custom JSON reader, vendored RapidJSON |
| **Crypto & Auth** | `Sodium`, `totp`, `hash` | Optional libsodium wrapper, TOTP (OpenSSL), hashing (SHA, CRC, rapidhash, salted) |
| **Containers** | `mapExtensor` | Extended maps/vectors with thread-safe variants, dense hash (ankerl), indexed vector, multi-index, lock guards |
| **Strings** | `string`, `defines` | QString/QStringView utilities, comparators, converters |
| **Date & Time** | `dateTime` | Qt date/time extensions |
| **Serialization** | `serialization` | QDataStream helpers, string serialization |
| **Logging** | `log`, `QStacker` | Structured logging (JSON + SQL), stack traces via backward-cpp, HTTP exceptions |
| **Formatting** | `fmtExtra` | {fmt} extensions: dynamic formatters, enum formatting |
| **Filesystem** | `filesystem` | Path/suffix helpers, common file operations |
| **Locale / Geo** | `locale`, `GeoLite2PP` | Locale utilities, optional MaxMind GeoIP lookup |
| **Threading** | `thread` | Thread monitoring, thread-local status, thread vectors |
| **Numbers** | `number`, `rand` | Numeric sanitizers, double operators, converters, clamped normal distribution |
| **QR** | `QR` | QR code generation (nayuki) |
| **Caching** | `caching` | APCu-style in-memory cache |
| **Memory** | `jemalloc` | Optional jemalloc helpers |
| **Meta / Utility** | `magicEnum`, `concept`, `types`, `mixin`, `RAII`, `SpaceShipOP`, `fmtExtra`, `misc` | Vendored magic_enum, C++20 concepts, type traits, policy mixins, RAII guards, spaceship operators for Qt types, snowflake IDs, base64/32, CSV, etc. |
| **Build tricks** | `gitTrick` | Embeds `git describe` and submodule info into the binary via Qt resources |

## Requirements

| Dependency | Notes |
|------------|-------|
| **C++26** compiler | GCC with `-std=gnu++26` (set automatically) |
| **Qt 6** | Core at minimum; Network optional (`HAS_QT_NETWORK`) |
| **Boost >= 1.83** | json, describe, algorithm, multi_index; Beast/Asio when `WITH_BOOST_BEAST` |
| **{fmt}** | `zypper in fmt-devel` |
| **MariaDB client** | `zypper in libmariadb3 libmariadb-devel` |
| **libdw + libdl** (Linux) | `zypper in libdw-devel` -- for stack traces |

## Optional Features

All optional features are off by default. Enable with CMake flags or qmake variables:

| CMake option (`-D...=ON`) | qmake equivalent | Pulls in | Extra package |
|---------------------------|------------------|----------|---------------|
| `RBK_WITH_BOOST_BEAST` | `WITH_BOOST_BEAST` | Boost.Beast HTTP server + router | (Boost) |
| `RBK_WITH_BOOST_MYSQL` | `WITH_BoostMysql` | Boost.MySQL | (Boost) |
| `RBK_WITH_SODIUM` | `WITH_SODIUM` | libsodium crypto | `zypper in sodium-devel` |
| `RBK_WITH_SSL` | `WITH_SSL` | OpenSSL + TOTP | `zypper in libopenssl-devel` |
| `RBK_WITH_JEMALLOC` | `WITH_Jemalloc` | jemalloc allocator | `zypper in jemalloc-devel` |
| `RBK_WITH_ASAN` | `WITH_ASAN` | AddressSanitizer | (compiler) |
| `RBK_WITH_REPROC` | `WITH_REPROC` | reproc process launcher | see `rbk.pri` for repo |
| `RBK_WITH_ZIPPER` | `WITH_ZIPPER` | libzip | `zypper in libzip-devel` |
| `RBK_WITH_MAXMIND` | `WithMaxMind` | MaxMind GeoIP | `zypper in libmaxminddb-devel` |
| `RBK_WITH_MINCURL` | `withMinCurl` | libcurl wrapper | `zypper in libcurl-devel` |
| `RBK_WITH_CLICKHOUSE` | `withClickHouse` | ClickHouse client | |
| `RBK_WITH_QRCODE` | `withQrCode` | QR code generation | |
| `RBK_WITH_APCU` | `withAPCU` | In-memory caching | |
| `RBK_WITH_QT_NETWORK` | `HAS_QT_NETWORK` | Qt Network extras (SpaceShipOP) | `QT += network` |

## Quick Start

```bash
# from your project root
git submodule add https://github.com/dublinbranch/rbk.git rbk
```

### CMake

In your top-level `CMakeLists.txt`:

```cmake
add_subdirectory(rbk)
target_link_libraries(myapp PRIVATE rbk::rbk)
```

Enable features at configure time (from your project root, build tree in `build/`):

```bash
cmake -B build -S . -DRBK_WITH_BOOST_BEAST=ON -DRBK_WITH_SSL=ON
cmake --build build
```

#### Developing rbk itself (standalone clone)

All CMake output goes in **`build/`** at the repository root (gitignored). Use the preset or an explicit build directory:

```bash
cmake --preset default    # binary dir: ./build
cmake --build build
```

Same layout without presets:

```bash
cmake -B build -S .
cmake --build build
```

### qmake (legacy)

Create a `config.pri` from the template:

```bash
cp rbk/configTemplate.pri rbk/config.pri
```

In your `.pro` file:

```qmake
include(rbk/rbk.pri)
```

## Project Structure

```
rbk/
  build/               # CMake binary dir when developing rbk locally (gitignored)
  CMakeLists.txt       # CMake build (recommended)
  CMakePresets.json    # default preset → binary dir ./build
  rbk.pri              # qmake build (legacy, kept for compat)
  configTemplate.pri   # Template for local config.pri (qmake)
  BoostJson/           # Boost.JSON extensions
  BoostMysql/          # Optional Boost.MySQL
  caching/             # APCu-style caching
  concept/             # C++20 concepts
  dateTime/            # Date/time utilities
  defines/             # Shared string defines
  filesystem/          # File/path helpers
  fmtExtra/            # {fmt} extensions
  GeoLite2PP/          # MaxMind GeoIP wrapper
  gitTrick/            # Build-time git info embedding
  hash/                # Hashing (SHA, CRC, rapid, salt)
  HTTP/                # HTTP/FCGI, Beast server, router
  jemalloc/            # jemalloc helpers
  JSON/                # Custom JSON reader
  locale/              # Locale/country codes
  log/                 # Structured logging
  magicEnum/           # Vendored magic_enum
  mapExtensor/         # Container extensions
  minCurl/             # libcurl wrapper
  minMysql/            # MariaDB layer
  misc/                # Grab-bag utilities
  mixin/               # Policy classes (NoCopy, etc.)
  mustache/            # Mustache templates
  number/              # Numeric utilities
  QR/                  # QR code generation
  QStacker/            # Stack traces + exceptions
  RAII/                # RAII helpers
  rand/                # Random utilities
  rapidjson/           # Vendored RapidJSON
  serialization/       # Serialization helpers
  Sodium/              # libsodium wrapper
  SpaceShipOP/         # Spaceship operators for Qt
  string/              # String utilities
  thread/              # Thread monitoring
  totp/                # TOTP authentication
  types/               # Type traits
```

## License

[Unlicense](https://unlicense.org/) -- public domain. See [LICENSE](LICENSE).

Individual modules may carry their own licenses (e.g. `JSON/LICENSE` is MIT).
