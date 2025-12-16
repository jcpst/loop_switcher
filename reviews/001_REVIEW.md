# Code Review - Start Here

**Review Date:** December 16, 2025  
**Updated:** December 16, 2025 (Bugs Fixed in Main)  
**Project:** 4-Loop MIDI Switcher (jcpst/loop_switcher)  
**Status:** Production Ready

---

## ✅ Issues Fixed in Main Branch

### Issue #1: ✅ MIDI PC Calculation Bug - **FIXED**
**Location:** `mode_controller.cpp` lines 154, 190  
**Status:** Merged via PR #4  
**Fix:** Changed formula to `((state.currentBank - 1) * 4) + switchIndex + 1`

### Issue #2: ✅ EEPROM Wear Leveling - **FIXED**
**Location:** `state_manager.cpp` `savePreset()` function  
**Status:** Merged via PR #6  
**Fix:** Added dirty-check before EEPROM writes

### Issue #3: ✅ Display Pin Conflict - **Review Needed**
**Location:** `main.cpp` line 39  
**Status:** Merged via PR #8
**Fix:** Removed call during setup

---

## 🚀 Recommended Path Forward

### Phase 1: Critical Fixes (TODAY - 15 minutes)
1. Apply PC calculation fix
2. Remove LED pin conflict  
3. Add EEPROM dirty-check
4. Test with MIDI monitor

### Phase 2: Improvements (NEXT WEEK - 2 hours)
5. Define constants for magic numbers
6. Add debug output flag
7. Implement display buffering
8. Fix mode transition memory
9. Add loop timing control

### Phase 3: Enhancement (FUTURE - 1 day)
10. Add unit test infrastructure
11. Create state diagrams
12. Add hardware schematic
13. Write troubleshooting guide
14. Implement staggered relay switching

---

## 🎓 Key Learnings

This review revealed:

1. **Off-by-one errors:** Common with 0-based vs 1-based numbering
2. **EEPROM bounds:** Validate before writing
3. **Pin conflicts:** Document all pin usage
4. **Testing:** Hardware projects need test procedures
5. **Architecture:** Clear structure aids debugging
