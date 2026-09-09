# Computer Organisation & Architecture — what this code does

**Subject:** COA Laboratory 2310221L
**Folders:** `src/core/` (1,764 lines) and `src/asm/` (387 lines)

This is not a subject applied to the project — it is the thing the project simulates.
Every component a COA textbook draws as a box exists here as a class with the same
responsibility it has in real hardware.

---

## Course outcome CO1

> **2310221L.CO.1** — *Explicate the architecture and instruction set of the 80386
> microprocessor.* **[L2]**


---


![The machine we designed: register file, control unit and memory on one shared bus, with the ALU beneath it.](../diagrams/coa-datapath.png)

*The machine we designed: register file, control unit and memory on one shared bus, with the ALU beneath it.*

## `Word.h` — the machine word

> **CO1 — explicate the architecture.** How a value is represented is the first architectural decision a design makes.

A 16-bit value with all the operators a processor needs, so ALU code reads like
arithmetic instead of function calls.

**What this does.** Lets the 16-bit machine word be added like an ordinary number, so the ALU can be written as `a + b`. The cast is what makes the value wrap around at 16 bits instead of growing.

```cpp
Word operator+(const Word& o) const { return Word(static_cast<u16>(value_ + o.value_)); }
```
<sub>src/core/Word.h:44</sub>

It also knows how to print itself as binary, hexadecimal, unsigned decimal and signed
decimal — which is what lets the interface switch number bases.

---

## `RegisterFile.h` — the registers

> **CO1 — explicate the architecture.** Register organisation: what exists, how wide it is, and what each register is for.

Holds the eight general registers plus the special ones a processor needs:

| Register | Purpose |
|---|---|
| R0–R7 | General purpose |
| PC | Program counter — which instruction is next |
| IR | Instruction register — the one being executed |
| SP | Stack pointer |
| MAR / MDR | Memory address and data registers |

Each register also records whether it was **read or written during this cycle**:

**What this does.** Reads a register, and quietly notes that it was read during this cycle. That note is what lets the diagram highlight exactly which registers took part in the instruction.

```cpp
Word read()        { readThisCycle_ = true;  return value_; }
void write(Word v) { value_ = v; wroteThisCycle_ = true; }
```
<sub>src/core/RegisterFile.h:36</sub>

That is what lets the display highlight exactly which registers took part in the current
instruction. Without it the diagram could show values but not activity.

Flags live here too — Zero, Carry, Overflow and Negative, printed as `Z-V-` style text.

---


![The ALU is a pure function: two operands in, a result and four flags out. The flags are the only way a processor can decide anything.](../diagrams/coa-alu-flags.png)

*The ALU is a pure function: two operands in, a result and four flags out. The flags are the only way a processor can decide anything.*

## `ALU.h` / `ALU.cpp` — the calculator

> **CO1 — explicate the architecture.** The arithmetic unit and its status flags are part of the machine we designed. *(Also COA Practical 1 — design your ALU.)*

Performs ADD, SUB, AND, OR, XOR, NOT, SHL, SHR, CMP, INC and DEC, returning a result
**and** the four flags.

It holds no state at all — the same inputs always give the same outputs:

**What this does.** The ALU's entire public interface. Give it an operation and two values, and it returns the result together with the flags. It keeps no state between calls, which is why it can be tested completely on its own.

```cpp
AluResult execute(AluOp op, Word a, Word b);
```
<sub>src/core/ALU.h:82</sub>

That purity is deliberate: it means the ALU can be tested on its own, with no processor
around it.

**How the flags are worked out**

Carry is the bit that falls out of the top. We add in a 32-bit scratch value and test
bit 16:

**What this does.** How addition and the carry flag actually work. We add using a 32-bit scratch value and then test bit 16 — if that bit is set, the answer did not fit into 16 bits.

```cpp
case ALU_ADD:
    wide = static_cast<u32>(a.raw()) + static_cast<u32>(b.raw());
    res.value = Word(static_cast<u16>(wide));
    res.flags.carry    = (wide & 0x10000u) != 0;
    res.flags.overflow = computeOverflowAdd(a, b, res.value);
    break;
```
<sub>src/core/ALU.cpp:44</sub>

Signed overflow is a different question — it happens when both operands share a sign but
the result does not:

**What this does.** Signed overflow is a different question from carry. It happens when both inputs have the same sign but the answer comes out with the opposite sign — which means the true result was too large to represent.

```cpp
bool ALU::computeOverflowAdd(Word a, Word b, Word r) {
    return (a.msb() == b.msb()) && (r.msb() != a.msb());
}
```
<sub>src/core/ALU.cpp:27</sub>

**Multiplication** is also here, done two ways so they can be compared — add-and-shift,
and successive addition. Both return a printable trace:

**What this does.** Multiplication done two ways, each writing out a step-by-step trace. Running both on 13 × 7 and getting 91 from each is also a useful correctness check.

```cpp
static Word multiplyAddShift(Word a, Word b, std::string* trace);
static Word multiplySuccessiveAdd(Word a, Word b, std::string* trace);
```
<sub>src/core/ALU.h:88</sub>

Running both on 13 × 7 and getting 91 from each is also a correctness check.

---

## `Instruction.h` / `.cpp` — the instruction set and control signals

> **CO1 — explicate the instruction set.** Every instruction the processor understands is defined here.

Fifteen instruction classes, all deriving from one abstract base. Each knows two things:
how to execute itself, and which control signals it needs.

**What this does.** Every instruction must be able to do two things: execute itself, and say which control signals it needs. These two lines are what the entire instruction set is built on.

```cpp
class Instruction {
public:
    virtual void           execute(CPU& cpu) = 0;
    virtual ControlSignals signals() const   = 0;
};
```
<sub>src/core/Instruction.h:65</sub>

**The control signals** are the control unit's entire output — fourteen lines,
regenerated every cycle:

```
PCout  PCload  PCinc  MARload  MemRead  MemWrite  IRload
RegRead  RegWrite  ALUen  FlagWr  StackOp  BusAct  Halt
```

Each instruction returns its own vector. For example an ADD asserts:

**What this does.** What an ADD instruction asks the control unit to switch on. This *is* the control unit's output for that instruction — nothing else in the system produces signals.

```cpp
ControlSignals AluBinaryInstruction::signals() const {
    ControlSignals s;
    s.regRead   = true;
    s.regWrite  = (op_ != ALU_CMP);   // CMP computes flags but writes nothing
    s.aluEnable = true;
    s.aluOp     = op_;
    s.flagWrite = true;
    s.busActive = true;
    s.pcInc     = true;
    return s;
}
```
<sub>src/core/Instruction.cpp:188</sub>

Because those signals already exist, plotting them as a timing chart later requires no
new machinery — only a view.

---

## `Memory.h` / `.cpp` — storage

> **CO1 — explicate the architecture.** The memory side: how it is addressed and how it is held.

64K addressable words, but stored sparsely: only cells that were actually written take
up space, and anything unwritten reads as zero.

**What this does.** Memory is a hash table rather than an array. All 64K addresses exist in principle, but only the cells actually written take up any space.

```cpp
class Memory {
    ds::HashMap<unsigned int, u16> cells_;   // address -> value
};
```
<sub>src/core/Memory.h:22</sub>

It also tracks the last address touched and whether it was a read or a write, so the
diagram can show memory activity.

---


![One call to step() advances exactly one stage, and each stage asserts its own control signals — fourteen lines in total, regenerated every cycle.](../diagrams/coa-instruction-cycle.png)

*One call to step() advances exactly one stage, and each stage asserts its own control signals — fourteen lines in total, regenerated every cycle.*

## `CPU.h` / `.cpp` — the datapath

> **CO1 — explicate the architecture.** The fetch-decode-execute cycle, made steppable one stage at a time.

The part that ties everything together. One call to `step()` advances **exactly one
micro-stage**:

**What this does.** One press of Step runs exactly one of these four stages. This is the fetch-decode-execute cycle from the textbook, made steppable so it can be watched.

```cpp
case STAGE_FETCH:      // PC drives the address, instruction loads into IR
case STAGE_DECODE:     // ask the instruction for its control signals
case STAGE_EXECUTE:    // current_->execute(*this)   <- polymorphic
case STAGE_WRITEBACK:  // PC advances, unless a branch already moved it
```
<sub>the four cases of CPU::step() — src/core/CPU.cpp:128-176, bodies omitted</sub>

That is what turns a memorised four-word sequence into something you can watch happen.

The CPU also owns the call stack (`ds::Stack<Word>`), the cycle history
(`ds::Deque<CycleRecord>`) and the output log — recording a full snapshot every cycle so
execution can be rewound.

---


![Two passes, because a jump can point forwards to a label that has not been seen yet.](../diagrams/coa-assembler.png)

*Two passes, because a jump can point forwards to a label that has not been seen yet.*

## `asm/Assembler.h` / `.cpp` — text into instructions

A two-pass assembler.

**Pass one** walks the source and records where every label is:

**What this does.** Pass one of the assembler: whenever a line carries a colon, remember the name and the address it sits at. Pass two then turns `JNZ loop` into a jump to line 3.

```cpp
if (colon != std::string::npos) {
    std::string label = trim(t.substr(0, colon));
    if (!label.empty()) symbols_.put(label, address);
}
```
<sub>src/asm/Assembler.cpp:253</sub>

**Pass two** builds the instruction objects and resolves label references, so `JNZ loop`
becomes a jump to line 3.

It accepts comments, labels, flexible spacing, and decimal, hexadecimal or binary
literals. Errors name the exact line:

```
line 4: 'R9' is not a register (use R0 to R7)
```

---

## The instruction set

> **CO1 — the instruction set of the processor**, designed by us rather than adopted from an existing machine.

Fourteen instructions, enough for real programs with loops and decisions:

```
LOAD  Rd, #n       put a literal into a register
LOADM Rd, [addr]   read from memory
STORE Rs, [addr]   write to memory
MOV   Rd, Rs       copy register to register
ADD / SUB / AND / OR / XOR  Rd, Rs
CMP   Rd, Rs       compare — sets flags, writes nothing
NOT / SHL / SHR / INC / DEC  Rd
MUL   Rd, Rs       add-and-shift multiply
JMP / JZ / JNZ / JC / JNC / JN  addr
CALL addr  /  RET  subroutines, using the stack
PUSH Rs  /  POP Rd
OUT   Rs           print a register
HLT                stop
```

---

## Sample programs

`programs/` holds three working programs, all verified:

| File | What it does | Result |
|---|---|---|
| `sum.asm` | Adds 1 to 10 with a counted loop | 55 ✓ |
| `multiply.asm` | 13 × 7 both by add-and-shift and by repeated addition | 91 from both ✓ |
| `subroutine.asm` | CALL / RET and PUSH / POP | 10, 42, 42 ✓ |

---

## Outcomes beyond CO1

| Outcome | Wording | Where it is |
|---|---|---|
| **CO2** | *Develop assembly language programs using 80386 instruction set.* [L3] | `asm/Assembler.cpp` — the two-pass assembler — and the six programs in `programs/` |
| **CO3** | *Illustrate processor level details about arithmetic operations using computer arithmetic algorithms.* [L3] | `ALU.cpp` — carry taken from a 32-bit scratch value, signed overflow from the sign rule, and multiplication by both add-and-shift and successive addition, each printing its steps |
| **CO4** | *Demonstrate the different ways to generate control signals and their organization techniques and instruction pipelining of computer system.* [L3] | Half met. `Instruction.cpp` generates the fourteen control lines for every instruction, and the console build already plots them as a timing chart. **Pipelining is not built** and is listed as planned work |

CO2 and CO3 are met on our own machine. CO4 is met for control signals and open for
pipelining.

## Practicals this covers

P1 (ALU) and P2 (control unit) are the core of the project.

Still to do: P3–P5 need 64-bit wide arithmetic, P4 needs hex↔BCD conversion, P6 needs
string operations, P7 needs floating point for quadratic roots. Pipelining with visible
hazard stalls is the other large remaining piece.
