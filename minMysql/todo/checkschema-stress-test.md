# CheckSchema: stress test the whitespace hardening

Written 2026-10-07, after 3 review rounds of `checkschema.cpp`. Every check so far was
static (code reading and compile). **Nothing was run against a DB.** Do the tests below
before you trust it on machines.

## What changed (short)

- `checkDbSchema()` returns `false` on a difference. It does not `abort()` any more.
  Every caller (PPPLC, googleAdsListener, digitalSpine, clubttheK, diter, swapTronic)
  checks the return value and calls `exit(1)`.
- `checkTableData()` runs after a schema failure too, so one run shows both reports.
- `reMap()` matches rows on the **trimmed** key.
- `checkWhitespace()` (private) scans the data. The `Origin` enum decides what it scans:

  | Origin | Keys | Other columns | Fix SQL |
  |---|---|---|---|
  | `MachineDb` (check, DB of the machine) | yes | no | key only |
  | `Disk` (check, reference file) | yes | yes | none |
  | `RefreshDb` (refresh, source DB) | yes | yes | key and values |

- A key with whitespace at the start or end, or two keys that are equal after trim,
  **fails** the check.
- A space or tab at the end of another column is **only a warning**. Newline is out for
  now. The value `" - "` is a placeholder and is not reported (`trailingBlanks()`).
- `saveTableData()` is all or nothing: if one table has a key problem, it saves no file
  and returns `false`. PPPLC calls it **before** `saveSchema()`.
- A missing reference file is **not fatal** (see "Open points").
- A missing key column or a failed query skips the table and fails the check. No crash.

## The trap to remember

MariaDB collations without `_nopad` are PAD SPACE: `=` and `<>` ignore trailing spaces.
`WHERE x = 'abc'` finds `'abc '`, and `WHERE x <> TRIM(x)` finds nothing. C++ compares
bytes, so the same row looks missing there. To verify a value, always use `HEX(x)` and
`LENGTH(x)`, never `=`.

## Test setup

- Use a **copy** of a PPPLC DB, never the DB of a machine.
- The reference files are `<PPPLC>/db/machineState`, `db/pages` and `db/schema`. Back
  them up before the refresh tests, and put them back after.
- After each fix SQL that the program prints, run it and run the check again. The
  printed SQL must really fix the row.

## Tests: check (`./PPPLC -C <config> --forceDbCheck`)

| # | Setup on the test DB | Expected |
|---|---|---|
| 1 | key `'carriageMaxDrift '` (trailing space) | key warning "in DB" with UPDATE, rest of the data report is printed, exit 1 |
| 2 | key `' carriageMaxDrift'` and key `'boardLoaded\t'` | same as 1. Run the printed UPDATE: it must find the row (exact `WHERE`) |
| 3 | keys `'foo'` and `'foo\t'` (the unique index allows this, tab is not padded) | "more than one row after trim", exit 1 |
| 4 | reference has `name = 'encoderMaxCarriage '`, DB is clean | "in Disk ends with space or tab", mismatch block says "reference data is wrong", **no** UPDATE |
| 5 | DB has `info = 'abc '`, reference is clean | **no** scan warning, mismatch with UPDATE to `'abc'`. Run it: next check passes |
| 6 | `okRange = ' - '` | no warning |
| 7 | `okRange = ' -'`, `'- '`, `' -\t'` | warning for each |
| 8 | drop column `info` (in the machineState SELECT) | schema diff, "query failed … skipped", "Schema check failed", exit 1, **no crash** |
| 9 | rename `pages.id` (`SELECT *` still works) | "key column id is not in the DB data", table skipped, exit 1, **no crash** |
| 10 | move `db/pages` away (binary without the file in QRC) | `qCritical` "impossible to load table data", other tables checked, **not** fatal |
| 11 | truncate `db/pages` | "reference data … is corrupt", exit 1 |
| 12 | clean DB that matches the reference | passes, program starts |

## Tests: refresh (`./PPPLC -C <config> --refreshDBSchema`)

| # | Setup on the source DB | Expected |
|---|---|---|
| 13 | one key with a trailing space | key warning with UPDATE, "NO table data is saved", "schema and table data NOT refreshed". `db/schema`, `db/machineState` and `db/pages` are **unchanged** (check mtime) |
| 14 | `info = 'abc '` | warning with UPDATE, all files are saved |
| 15 | a `pages` body that ends with a space | warning "too long to print the UPDATE" |
| 16 | clean DB | files saved, then `--forceDbCheck` on the same DB passes (round trip) |

## Tests: other projects

| # | Project | Expected |
|---|---|---|
| 17 | googleAdsListener, digitalSpine, clubttheK with a schema difference | stop with exit code 1 (before: SIGABRT and core dump). Check that no service file or script needs the abort signal |

## Open points

- **The PPPLC data check probably does nothing on deployed machines.** The PPPLC CMake
  build does not include `db.qrc` (only `gitTrick/resources.qrc`, `cmake/RbkQt5.cmake`).
  The table data has no `#embed` fallback, but `db/schema` has one. A binary that runs
  without its source folder at `BasePath` skips the data check. Fix: add `db.qrc` to the
  CMake build. First find out how the washers get the binary.
- `reMap()` is public and now trims the key. I did not check the other projects for
  callers.
- The fix SQL uses double quotes. It breaks with the `ANSI_QUOTES` sql_mode. This was so
  before, too.
- A DB `CHECK` constraint would stop bad keys at write time. It must be
  `CHECK (LENGTH(internalCode) = LENGTH(TRIM(internalCode)))`.
  `CHECK (internalCode = TRIM(internalCode))` does **not** work (PAD SPACE).
