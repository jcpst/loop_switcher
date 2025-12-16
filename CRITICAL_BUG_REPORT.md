# 🔴 CRITICAL BUG REPORT

**Date:** December 16, 2025  
**Severity:** CRITICAL - Incorrect MIDI Output  
**Status:** UNPATCHED  

---

## Bug Summary

The MIDI Program Change calculation is **off by 4**, causing all Program Change messages to be 4 numbers higher than intended. This means Bank 1 sends PC 5-8 instead of PC 1-4, and Bank 32 attempts to send PC 129-132 (which overflow to PC 1-4 due to MIDI limits).

---

## Impact

### Current Behavior:
- **Bank 1** sends **PC 5, 6, 7, 8** (should be 1, 2, 3, 4)
- **Bank 2** sends **PC 9, 10, 11, 12** (should be 5, 6, 7, 8)
- **Bank 32** sends **PC 129, 130, 131, 132** → wraps to **PC 1, 2, 3, 4** (should be 125, 126, 127, 128)

### Consequences:
1. ❌ **MIDI devices receive wrong program numbers**
2. ❌ **Bank 32 wraps around due to MIDI 7-bit limit (0-127)**
3. ❌ **EEPROM presets are stored at wrong addresses**
4. ❌ **Users cannot access the first 4 presets via MIDI**
5. ❌ **Banks 1-31 never send correct PC numbers**

---

## Root Cause

**File:** `src/mode_controller.cpp`  
**Line:** 190  

```cpp
// INCORRECT - currentBank starts at 1, not 0
uint8_t pc = (state.currentBank * 4) + switchIndex + 1;
```

### Why This Is Wrong:

The code assumes `currentBank` is 0-based, but `StateManager` initializes it to 1:

```cpp
// From state_manager.cpp line 7
currentBank(1),  // Banks displayed as 1-32, not 0-31
```

### Calculation Example:

With **Bank 1, Switch 1** (index 0):
```
CURRENT (wrong):  pc = (1 × 4) + 0 + 1 = 5  ❌
EXPECTED:         pc = 1                    ✓
```

With **Bank 32, Switch 4** (index 3):
```
CURRENT (wrong):  pc = (32 × 4) + 3 + 1 = 132  ❌ (overflows to ~4)
EXPECTED:         pc = 128                      ✓
```

---

## The Fix

### Option 1: Fix the Formula (RECOMMENDED)

**File:** `src/mode_controller.cpp`  
**Line:** 190

```cpp
// BEFORE (incorrect):
uint8_t pc = (state.currentBank * 4) + switchIndex + 1;

// AFTER (correct):
uint8_t pc = ((state.currentBank - 1) * 4) + switchIndex + 1;
```

Also fix line 154 in the same file:
```cpp
// BEFORE (incorrect):
uint8_t presetNumber = (state.currentBank * 4) + state.activePreset + 1;

// AFTER (correct):
uint8_t presetNumber = ((state.currentBank - 1) * 4) + state.activePreset + 1;
```

### Option 2: Change Bank Numbering (NOT RECOMMENDED)

Change `currentBank` to be 0-based (0-31) internally and only display as 1-32. This would require changes in multiple places and affect bank up/down logic.

---

## Verification Test Cases

After applying the fix, verify these calculations:

| Bank | Switch | Expected PC | Current (Wrong) | Fixed |
|------|--------|-------------|-----------------|-------|
| 1    | 1      | 1           | 5               | 1 ✓   |
| 1    | 2      | 2           | 6               | 2 ✓   |
| 1    | 3      | 3           | 7               | 3 ✓   |
| 1    | 4      | 4           | 8               | 4 ✓   |
| 2    | 1      | 5           | 9               | 5 ✓   |
| 16   | 1      | 61          | 65              | 61 ✓  |
| 31   | 4      | 124         | 128             | 124 ✓ |
| 32   | 1      | 125         | 129 (→1)        | 125 ✓ |
| 32   | 4      | 128         | 132 (→4)        | 128 ✓ |

---

## Global Preset Bug

**File:** `src/mode_controller.cpp`  
**Line:** 177

```cpp
// Global preset always sends PC 128 - this is CORRECT
sendMIDIProgramChange(128, state.midiChannel);
```

This is hard-coded correctly, but it's inconsistent with the buggy formula. After fixing the main calculation, verify that double-pressing still sends PC 128.

---

## EEPROM Impact

The bug also affects EEPROM storage! 

**File:** `src/mode_controller.cpp`  
**Line:** 154

```cpp
uint8_t presetNumber = (state.currentBank * 4) + state.activePreset + 1;
state.savePreset(presetNumber);
```

### Current Behavior:
- Bank 1 presets are saved at EEPROM addresses **6-9** instead of **2-5**
- Bank 32 presets are saved at addresses **130-133** instead of **129-132**
- Addresses **132-133 are OUTSIDE the allocated EEPROM space** (131 is max)

### Consequences:
1. ❌ **EEPROM addresses 2-5 are never used** (first 4 presets unreachable)
2. ❌ **Bank 32 writes corrupt data outside bounds**
3. ❌ **Potential memory corruption on ATmega328**
4. ❌ **Users' saved presets are at wrong locations**

---

## Migration Note

⚠️ **IMPORTANT:** After fixing this bug, **all existing saved presets will be in wrong locations**.

Users who have already programmed presets will need to:
1. **Re-program all their presets** after the fix
2. Alternatively, provide a migration script to move EEPROM data:

```cpp
// Migration function (one-time run)
void migratePresets() {
    for (uint8_t bank = 1; bank <= 32; bank++) {
        for (uint8_t preset = 0; preset < 4; preset++) {
            // Old (wrong) address calculation
            uint8_t oldAddr = ((bank * 4) + preset + 1) - 1 + EEPROM_PRESETS_START_ADDR;
            // New (correct) address calculation  
            uint8_t newAddr = (((bank - 1) * 4) + preset) + EEPROM_PRESETS_START_ADDR;
            
            // Copy data if different
            if (oldAddr != newAddr && oldAddr < 132) {
                uint8_t data = EEPROM.read(oldAddr);
                EEPROM.write(newAddr, data);
            }
        }
    }
}
```

---

## Testing Plan

1. **Unit Test** (requires hardware or simulator):
   ```
   Test: Bank 1, Switch 1 → Expect MIDI PC 1
   Test: Bank 1, Switch 4 → Expect MIDI PC 4
   Test: Bank 32, Switch 4 → Expect MIDI PC 128
   ```

2. **MIDI Monitor**:
   - Connect MIDI output to computer
   - Use MIDI monitor software
   - Press switches and verify PC numbers

3. **EEPROM Verification**:
   - Save preset in Bank 1, Switch 1
   - Check EEPROM address 0x02 (should contain data)
   - Previously would incorrectly write to 0x05

---

## Related Issues

This bug also affects the README documentation. From README.md line 55:

```markdown
- Press individual switch to send MIDI PC: `PC = (bank x 4) + switch + 1`
```

This formula is **incorrect** when banks are numbered 1-32. It should be:

```markdown
- Press individual switch to send MIDI PC: `PC = ((bank - 1) × 4) + switch + 1`
```

Or more clearly:
```markdown
- Bank 1 sends PC 1-4
- Bank 2 sends PC 5-8
- ...
- Bank 32 sends PC 125-128
```

---

## Priority

**🔴 CRITICAL - FIX IMMEDIATELY**

This bug makes the device send wrong MIDI messages and corrupts EEPROM. It should be fixed before any production use or sharing with other builders.

---

## Patch File

Create this patch and apply with `git apply`:

```diff
diff --git a/src/mode_controller.cpp b/src/mode_controller.cpp
index XXXXXXX..YYYYYYY 100644
--- a/src/mode_controller.cpp
+++ b/src/mode_controller.cpp
@@ -151,7 +151,7 @@ void ModeController::exitEditMode() {
   relays.update(state.loopStates);
 
   // Calculate preset number and save to EEPROM
-  uint8_t presetNumber = (state.currentBank * 4) + state.activePreset + 1;
+  uint8_t presetNumber = ((state.currentBank - 1) * 4) + state.activePreset + 1;
   state.savePreset(presetNumber);
 
   // Show "SAVED" message
@@ -187,7 +187,7 @@ void ModeController::handleSingleSwitchPress(uint8_t switchIndex) {
       state.activePreset = switchIndex;
 
       // Send MIDI Program Change
-      uint8_t pc = (state.currentBank * 4) + switchIndex + 1;
+      uint8_t pc = ((state.currentBank - 1) * 4) + switchIndex + 1;
       sendMIDIProgramChange(pc, state.midiChannel);
 
       // Load preset from EEPROM and apply to relays
```

---

## Discovered By

Code review process while creating architecture documentation

**Review Date:** December 16, 2025
