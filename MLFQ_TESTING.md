# MLFQ Testing Guide

## Quick Test Procedure

### 1. Basic Build & Run (2-3 minutes)
```bash
make clean
make
make qemu
```

Inside QEMU:
```bash
$ mlfq_demo
```

### 2. Demo Output Expectations (30 seconds each test)

**Expected Output:**
```
=== MLFQ Scheduler Demo ===

Test 1: CPU-bound process (should be demoted)
Child PID: 3
Child PID 3: Queue Level = 0, Ticks In Queue = 0, Total Ticks = 100
Child PID 3: Queue Level = 1, Ticks In Queue = 2, Total Ticks = 250
Child PID 3: Queue Level = 2, Ticks In Queue = 0, Total Ticks = 300
Child PID 3: Queue Level = 3, Ticks In Queue = 8, Total Ticks = 500
CPU-bound test complete

Test 2: I/O-bound process (should stay high priority)
Child PID: 4
Child PID 4: Queue Level = 0, Ticks In Queue = 0, Total Ticks = 50
Child PID 4: Queue Level = 0, Ticks In Queue = 1, Total Ticks = 100
Child PID 4: Queue Level = 0, Ticks In Queue = 2, Total Ticks = 150
Child PID 4: Queue Level = 0, Ticks In Queue = 3, Total Ticks = 200
I/O-bound test complete

=== Demo Complete ===
```

---

## Variable Tuning & Effects

### 1. TIME_QUANTA_0 (Default: 4)

**Location:** `kernel/param.h`

| Value | Effect | Test Time |
|-------|--------|-----------|
| 2 | CPU-bound demotes faster (queue 0→1 at 2 ticks) | 15 sec |
| 4 | Normal demotion (baseline) | 30 sec |
| 8 | CPU-bound stays longer at queue 0 (uses 8 ticks) | 45 sec |

**How to Test:**
```bash
# Edit kernel/param.h
#define TIME_QUANTA_0 2

make clean
make
make qemu
$ mlfq_demo
# Observe: CPU-bound process demotes faster
```

**Expected Change:**
- Lower value → Process demotes sooner → Demotion chain faster
- Higher value → Process stays at queue 0 longer → More responsive initially

---

### 2. TIME_QUANTA_1, 2, 3 (Default: 8, 16, 32)

**Location:** `kernel/param.h`

| Parameter | Value | Effect | Test Time |
|-----------|-------|--------|-----------|
| TIME_QUANTA_1 | 4 | Fast demotion through queue 1 | 20 sec |
| TIME_QUANTA_1 | 8 | Normal (baseline) | 30 sec |
| TIME_QUANTA_1 | 16 | Slow progression to queue 2 | 45 sec |
| TIME_QUANTA_2 | 8 | Fast reach to queue 3 | 25 sec |
| TIME_QUANTA_2 | 16 | Normal (baseline) | 30 sec |
| TIME_QUANTA_2 | 32 | Slow reach to queue 3 | 40 sec |
| TIME_QUANTA_3 | 16 | Low-priority processes run less often | 45 sec |
| TIME_QUANTA_3 | 32 | Normal (baseline) | 30 sec |
| TIME_QUANTA_3 | 64 | Low-priority processes run much less | 60 sec |

**How to Test:**
```bash
#define TIME_QUANTA_1 4    # Instead of 8
make clean && make && make qemu
$ mlfq_demo
# Watch how fast CPU-bound moves through queues
```

**Expected Observation:**
- Lower values in higher queues → Faster classification
- Lower values in lower queues → Less starvation but more overhead
- Higher values in lower queues → More CPU for batch jobs, potential starvation

---

### 3. BOOST_INTERVAL (Default: 256)

**Location:** `kernel/param.h`

| Value | Effect | Test Time | Observation |
|-------|--------|-----------|-------------|
| 50 | Very frequent boosts | 30 sec | Low-priority always gets boosted quickly; less differentiation |
| 128 | Frequent boosts | 40 sec | Moderate starvation prevention |
| 256 | Normal (baseline) | 30 sec | Good balance |
| 512 | Rare boosts | 60 sec | Possible starvation; low-priority starves longer |
| 1024 | Very rare boosts | 120 sec | High risk of starvation |

**How to Test Starvation Prevention:**
```bash
#define BOOST_INTERVAL 512

make clean && make && make qemu
$ mlfq_test
# Run full test suite
# Observe: Low-priority process waits ~500 ticks before boost
```

**Expected Change:**
- Lower BOOST_INTERVAL → All processes reset to queue 0 often → Fair but less differentiation
- Higher BOOST_INTERVAL → Better I/O vs CPU separation → Risk of starvation

---

## Testing Scenarios

### Scenario 1: Test CPU-Bound Demotion (5 minutes total)

**Test Command:**
```bash
#define TIME_QUANTA_0 4
#define TIME_QUANTA_1 8
#define TIME_QUANTA_2 16
#define TIME_QUANTA_3 32

make clean && make && make qemu
$ mlfq_demo
```

**Check in Output:**
- ✓ CPU-bound starts at queue_level=0
- ✓ After ~4 ticks: demotes to queue_level=1
- ✓ After ~12 ticks (4+8): demotes to queue_level=2
- ✓ After ~28 ticks (4+8+16): demotes to queue_level=3
- ✓ Stays at queue_level=3 until boost

---

### Scenario 2: Test I/O-Bound Stays High Priority (5 minutes total)

**Test Command:**
```bash
$ mlfq_demo
# Watch Test 2 output
```

**Check in Output:**
- ✓ I/O-bound stays at queue_level=0 throughout
- ✓ ticks_in_queue never exceeds TIME_QUANTA_0
- ✓ Quick response times maintained

---

### Scenario 3: Test Priority Boost (10 minutes total)

**Test Command:**
```bash
#define BOOST_INTERVAL 100  # Shorter for testing

make clean && make && make qemu
$ mlfq_test
```

**Observe:**
- ✓ Low-priority process gets CPU after ~100 ticks
- ✓ All processes reset to queue_level=0 periodically
- ✓ No starvation visible

---

### Scenario 4: Stress Test (15 minutes total)

**Test Command:**
```bash
$ mlfq_test
# Full test suite with multiple processes
```

**Metrics to Check:**
- All 3 test suites complete successfully
- CPU-bound processes demoted correctly
- I/O-bound processes maintain high priority
- No deadlocks or crashes

---

## Quick Reference: Change & Impact Matrix

| Parameter | Change | Time to Test | Impact | Risk |
|-----------|--------|--------------|--------|------|
| TIME_QUANTA_0 | 2→4→8 | 15-45 sec | Demotion speed | Low |
| BOOST_INTERVAL | 100→256→512 | 30-120 sec | Starvation prevention | Medium |
| TIME_QUANTA_3 | 16→32→64 | 30-60 sec | Low-priority CPU share | Medium |
| MLFQ_LEVELS | 4→3→5 | 5-10 min | Queue structure | High |

---

## Fastest Tests (Use These First)

### 1. Verify Installation (1 minute)
```bash
make clean
make
# Check: No compilation errors
```

### 2. Quick Functionality (2 minutes)
```bash
make qemu
$ mlfq_demo
# Check: Demo runs without crashing
```

### 3. CPU-Bound Demotion (30 seconds)
```bash
# Observe output: Queue level changes 0→1→2→3
```

### 4. I/O-Bound Stability (30 seconds)
```bash
# Observe output: Queue level stays at 0
```

---

## Performance Impact of Changes

| Change | Performance | Responsiveness | Fairness | Test Duration |
|--------|-------------|-----------------|----------|----------------|
| Decrease TIME_QUANTA_0 | Slower (more switches) | Better | Same | 20 sec |
| Increase TIME_QUANTA_0 | Faster (fewer switches) | Worse | Same | 40 sec |
| Decrease BOOST_INTERVAL | Slower (more boosts) | Same | Better | 20 sec |
| Increase BOOST_INTERVAL | Faster (fewer boosts) | Same | Worse | 60 sec |
| Add queue level | Slower (more search) | Better | Same | 10 min |
| Remove queue level | Faster (less search) | Worse | Same | 5 min |

---

## Variable Change Checklist

### Before Each Test:
- [ ] Edit `kernel/param.h`
- [ ] Save file
- [ ] Run `make clean`
- [ ] Run `make`
- [ ] Verify no compilation errors
- [ ] Run `make qemu`
- [ ] Execute test program
- [ ] Document output
- [ ] Restore original values for next test

### Example Test Session:
```bash
# Test 1: Default values
make clean && make && make qemu
$ mlfq_demo
# Record output

# Test 2: Faster demotion
# Edit: #define TIME_QUANTA_0 2
make clean && make && make qemu
$ mlfq_demo
# Record output

# Test 3: Starvation prevention
# Edit: #define BOOST_INTERVAL 100
make clean && make && make qemu
$ mlfq_test
# Record output
```

---

## Summary

| Task | Time | Value |
|------|------|-------|
| Initial setup & test | 5 min | Verify working system |
| Test one parameter | 3-5 min | Understand single effect |
| Test all 4 quanta values | 15 min | Understand queue dynamics |
| Test 3 boost intervals | 10 min | Understand starvation prevention |
| Full parameter sweep | 45 min | Complete understanding |

**Recommendation:** Start with 5-minute basic test, then systematically change ONE parameter at a time and observe effects.
