# rbk_shutdown_demo

Test bed for `rbk::Shutdown` (`thread/shutdown.h`). Design: PPPLC `todo/graceful-shutdown.md`.

The demo runs five kinds of thread, one for each shape in the design:

| Thread | Shape | What it shows |
|---|---|---|
| `ticker` | `sleepFor(30s)` loop | shutdown cuts a long sleep short |
| `worker` | `Wakeup` + job queue | the job in hand finishes, queued jobs are dropped |
| `timer` | own `io_context` + `stopOnShutdown` | `ioc.run()` returns |
| `http` | Beast, 2 workers | `listen()` returns normally |
| `stuck` (`--stuck`) | ignores shutdown | forced exit at the deadline, the log names it |

HTTP on 127.0.0.1 (port 8097 by default): `/` (status), `/job?ms=N` (queue a job), `/exit`
(`rbk::Shutdown::request()`).

## Build

```bash
cmake -B build -DRBK_BUILD_TESTS=ON -DRBK_WITH_BOOST_BEAST=ON -DRBK_WITH_APCU=ON
cmake --build build
```

Beast needs APCU to link, so the target exists only with both options on.

## Run

```bash
demo/shutdown/demo.sh build/demo/shutdown/rbk_shutdown_demo
```

The script runs scenarios S1–S12 (PPPLC `todo/graceful-shutdown-demo.md` and design §15) and prints one line for each.
Each scenario runs in its own temp directory. The directories of failed scenarios are kept.
Exit status 0 means all passed.

By hand:

```bash
build/demo/shutdown/rbk_shutdown_demo --help
build/demo/shutdown/rbk_shutdown_demo &   # then: curl 'localhost:8097/job?ms=1500'; kill %1
```
