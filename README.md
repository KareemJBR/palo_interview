# Number Categorizer

A C++ program that generates random numbers, sorts each one into a category by a
fixed set of rules, and prints an action for it. It is built in three levels:

1. **Level 1** – generate and categorize the numbers, printing to the console.
2. **Level 2** – split the work across two processes connected by a pipe: one
   process produces the output and sends it as a message, the other receives the
   message and prints it.
3. **Level 3** – a hook that intercepts the message as it travels through the pipe
   and prints it, without changing either process.

---

## What the program does

It first picks a random count **X** (between 1 and 999), then generates **X**
random numbers (each also between 1 and 999). For every number it checks four
rules **in order** and uses the **first** one that matches:

| Rule | Matches | Action |
|------|---------|--------|
| 1 | Odd numbers | Print that the number is odd |
| 2 | Numbers whose first digit is `1` | Generate that many new random numbers (1–999) and print them as generated and then sorted |
| 3 | Numbers with exactly 3 digits | Print the product of the digits |
| 4 | Everything else | Print that the number belongs to "the others" |

Example output (one run; the numbers are random each time):

```
The selected random numbers are: 377 695 548 2 ...
377: this is an odd number
695: this is an odd number
548: the digit's multiplications are equal to 160
2: this number belongs to the others
```

---

## Requirements

- **Windows.** Levels 2 and 3 use Windows-specific features (pipes, DLL injection).
- **MinGW `g++`** with C++17 support. This project was built and tested with the
  MSYS2 MinGW build, `g++ 15.1.0`. Check yours with:
  ```bash
  g++ --version
  ```
- **Git Bash** (the terminal used in all commands below). It ships with Git for
  Windows. You can also use another terminal, but adjust the `./program` syntax.

---

## How to build

From the project folder, run the build script:

```bash
bash build.sh
```

This produces five files in the project folder:

| File | What it is |
|------|------------|
| `producer.exe` | Produces the output and sends it (Levels 1 & 2) |
| `consumer.exe` | Receives the message and prints it (Level 2) |
| `hook.dll` | The hook that intercepts the message (Level 3) |
| `injector.exe` | Starts the producer with the hook attached (Level 3) |
| `test.exe` | Automated tests |

> One harmless compiler **warning** about a `FARPROC` cast in `injector.cpp` is
> expected; it does not affect the build.

If you prefer to build by hand, these are the exact commands the script runs:

```bash
FLAGS="-std=c++17 -Wall -Wextra -O2 -Iheaders"
g++ $FLAGS source/rules.cpp source/process_message.cpp source/pipe.cpp source/producer.cpp -o producer
g++ $FLAGS source/process_message.cpp source/pipe.cpp source/consumer.cpp -o consumer
g++ $FLAGS -static -shared source/hook.cpp source/process_message.cpp -o hook.dll
g++ $FLAGS -static source/injector.cpp -o injector
g++ $FLAGS source/rules.cpp source/process_message.cpp source/test.cpp -o test
```

---

## How to run

All five files must be in the **same folder** (the build puts them there).

### Levels 1 & 2 — the main program

```bash
./producer
```

`producer` builds the output and sends it over a pipe to `consumer`, which prints
it. `producer` itself prints nothing except errors. You will see one header line
followed by one line per number.

### Level 3 — the hooked run

```bash
./injector
```

`injector` launches `producer` with `hook.dll` loaded into it. The program runs
exactly as before (you still see `consumer`'s output), but the hook also
**intercepts the message on the pipe** and writes it to:

- the console, as lines starting with `[hook]`, and
- a file named `hook.log` in the current folder.

Open `hook.log` to see the full intercepted message — it matches what `consumer`
printed, which is the proof that the hook saw the data in transit.

### Tests

```bash
./test
```

Runs two automated checks and prints `PASS` for each:
- the program with X fixed to 5 produces correctly categorized output, and
- a message survives being packaged and unpackaged (the pipe format).

---

## Project layout

```
headers/   # declarations (.h)
source/    # implementations (.cpp)
build.sh   # builds everything
```

| Source file | Role |
|-------------|------|
| `source/rules.cpp` | The four categorization rules and the program loop |
| `source/process_message.cpp` | The message format used on the pipe |
| `source/pipe.cpp` | Helpers for sending/receiving over a Windows pipe |
| `source/producer.cpp` | The producer (generates + sends) |
| `source/consumer.cpp` | The consumer (receives + prints) |
| `source/hook.cpp` | The hook (built into `hook.dll`) |
| `source/injector.cpp` | Loads the hook into the producer |
| `source/test.cpp` | Automated tests |

---

## Troubleshooting

- **`g++: command not found`** – MinGW is not installed or not on your `PATH`.
  Install MSYS2 and its MinGW toolchain, or Git for Windows' bundled toolchain.
- **`injector: failed to inject hook.dll`** – make sure `hook.dll`, `producer.exe`
  and `consumer.exe` are all in the same folder as `injector.exe`. Re-run
  `bash build.sh` to rebuild them together.
- **`producer: failed to start consumer`** – `consumer.exe` is missing from the
  folder; rebuild with `bash build.sh`.
