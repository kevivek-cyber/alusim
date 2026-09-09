# Datapath Bench

**A computer with the lid off.** A working 16-bit processor you can run one instruction at
a time — watching values travel the bus, registers change, and every step explained in
plain English.

An integrated Second Year B.Tech project spanning four subjects, built as one codebase
rather than four separate assignments.

---

## The problem

A Computer Organisation course teaches that a processor fetches an instruction, decodes
it, executes it, and that an arithmetic unit sets status flags along the way. Students can
recite that sequence, but they never *see* it — it happens billions of times a second
inside a sealed chip.

Practising engineers solve this with a **logic analyzer**, an instrument clipped onto real
hardware that draws each signal as it changes. This project is a virtual one.

---

## Try it

**Browser version** — nothing to build:

```bash
cd web && python -m http.server 8080
```

Then open <http://localhost:8080>. Press **Play** and watch it run itself.

**C++ version** — needs `g++` and `make`:

```bash
make                        # builds vsim.exe
./vsim.exe programs/sum.asm
```

Inside the simulator: `step`, `instr`, `run`, `wave`, `data`, `regs`, `mem`, `gfx`, `help`.

---

## What it does

Type a program, assemble it, then run it one instruction at a time. On each step the
register that changed is highlighted, the value physically travels along the bus to its
destination, and a line of plain English says what happened:

> *Added R1 (1) to R0 (0). R0 is now 1.*

Execution can be played automatically, stepped forward, or **stepped backwards** through
512 recorded states.

```asm
        LOAD  R0, #0        ; running total
        LOAD  R1, #1        ; counter
        LOAD  R2, #6        ; stop when the counter reaches 6
loop:   ADD   R0, R1        ; total = total + counter
        INC   R1
        MOV   R3, R1
        CMP   R3, R2        ; reached six?
        JNZ   loop          ; not yet — go round again
        STORE R0, [0x40]
        OUT   R0            ; prints 15
        HLT
```

---

## The machine

| Property | Value |
|---|---|
| Word size | 16 bits |
| Registers | R0–R7, plus PC, IR, SP, MAR, MDR |
| Flags | Zero, Carry, Overflow, Sign |
| Memory | 64K words, stored sparsely |
| Instructions | 14 |
| Control signals | 14 lines, regenerated every cycle |
| Engine | C++, 4,102 lines across 28 files |
| Ready-made containers used | **none** |

---

## How four subjects fit together

Each subject supplies something the machine cannot run without.

| The project needs | Supplied by | Without it |
|---|---|---|
| Knowledge of what is inside a processor | **Computer Organisation** | We would not know what to build |
| Containers to hold state while it runs | **Data Structures** | No call stack, no fetch queue, no symbol table |
| A way to draw the machine on screen | **Computer Graphics** | Nothing is observable — the whole point is lost |
| A language and a design that scales | **Object-Oriented C++** | Fifteen instructions become an unmaintainable branch chain |

Each subject has its own notebook, with diagrams:

| Notebook | Subject |
|---|---|
| [`docs/01_Data_Structures.ipynb`](docs/01_Data_Structures.ipynb) | Programming Lab — 2310214L |
| [`docs/02_Computer_Organisation.ipynb`](docs/02_Computer_Organisation.ipynb) | COA Lab — 2310221L |
| [`docs/03_Computer_Graphics.ipynb`](docs/03_Computer_Graphics.ipynb) | Computer Graphics Lab — 2310218L |
| [`docs/04_Object_Oriented_Cpp.ipynb`](docs/04_Object_Oriented_Cpp.ipynb) | Problem Solving using OOP — 2310261L |

Longer written notes are in [`docs/subjects/`](docs/subjects/).

---

## Layout

```
src/ds/      Data Structures    Stack, CircularQueue, Deque, LinkedList, HashMap
src/core/    Computer Org.      Word, RegisterFile, ALU, Instruction, Memory, CPU
src/ui/      Computer Graphics  Canvas (Bresenham, scan-line, clipping), ConsoleView
src/asm/     Assembly           Two-pass assembler with a hash-backed symbol table
programs/    Sample .asm programs
web/         Browser prototype — one self-contained HTML file
docs/        Subject notebooks and notes
```

Every container in `src/ds/` is written by hand. The project uses no `std::stack`,
`std::queue`, `std::list` or `std::map` anywhere, because writing them *is* the practical
— and each one does a real job inside the processor.

---

## Verified

| Test | Expected | Result |
|---|---|---|
| `sum.asm` — add 1 to 10 | 55 | ✅ in 135 cycles |
| `multiply.asm` — add-and-shift | 13 × 7 = 91 | ✅ |
| `multiply.asm` — repeated addition | 13 × 7 = 91 | ✅ both methods agree |
| `subroutine.asm` — CALL / RET / PUSH / POP | 10, 42, 42 | ✅ |
| Graphics self-test (`gfx`) | all shapes render | ✅ |

---

## Status

**Working** — the simulation engine, the assembler, all five data structures, the graphics
layer, the console interface, and the browser prototype.

**Planned next** — a control-signal timing chart, micro-stepping inside a single
instruction, pipelining with visible hazard stalls, breakpoints, and a real OpenGL
rendering path.

---

## Team

Vivek Dhamale · Pranjal Bhandari · Sanika Patil · Mrudula Bongulwar

**Guide:** Mrs. Jaya S. Mane, Assistant Professor, Software Engineering Department
School of Computer Engineering, MIT Academy of Engineering, Alandi, Pune
