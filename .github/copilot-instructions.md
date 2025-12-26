# GitHub Copilot Instructions for Loop Switcher

## Project Context

- **Project**: loop_switcher
- **Domain**: Embedded Firmware
- **Target Hardware**: ATmega328 (Arduino Uno/Nano)
- **Languages**: C, C++

## Project Goals

- Maintain deterministic behavior for hardware control
- Favor explicit state machines over implicit logic
- Separate hardware access from application logic
- Keep firmware testable without physical hardware

## Coding Conventions

### Naming

- **Constants**: `UPPER_SNAKE_CASE`
- **Functions**: `lowerCamelCase`
- **Types**: `PascalCase`
- **Globals**: `g_` prefix

### File Organization

- **Hardware Interfaces**: `include/`
- **Application Logic**: `src/`
- **Tests**: `test/`

## General Guidelines

### Memory and Performance

- Avoid dynamic memory allocation
- Avoid recursion
- Prefer compile-time configuration
- Assume limited RAM and flash
- Do not introduce blocking delays in main control paths

### Hardware Abstractions

#### Pins

- Define all pins in config headers
- Never hardcode pin numbers in logic

#### Relays

- Access relays only through wrapper functions
- Ensure relay state changes are explicit

#### Switches

- Always debounce footswitch input
- Treat switches as edge-triggered events

### State Machine Design

#### Structure

- Use explicit enums for modes and states
- Handle transitions with `switch`/`case`
- Provide entry and exit handling when applicable

#### Modes

The system supports three operating modes:

- **Manual**: Direct loop control
- **Bank**: Bank selection with presets
- **Edit**: Preset editing

#### Rules

- No hidden state transitions
- No fallthrough without comments

### MIDI

#### Output

- Encapsulate MIDI messages in helper functions
- Validate channel range (1–16)
- Validate program range (1–128)

#### Behavior

- Send MIDI only on state change
- Avoid duplicate program change messages

### Persistence

#### Storage

- Abstract EEPROM access
- Include validity flags or checksums
- Fail safely on corrupted data

#### Presets

- Load presets explicitly
- Never assume EEPROM is initialized

## Testing Guidelines

### Focus Areas

- State transitions
- Preset save/load behavior
- Relay state correctness
- MIDI message formatting

### Style

- Avoid hardware dependencies
- Mock hardware interactions
- Keep tests deterministic

## Documentation Standards

### Function Documentation

All functions should include:

- **Description**: What the function does
- **Parameters**: Explanation of each parameter
- **Return Value**: What the function returns
- **Side Effects**: Any state changes or external effects

### Architecture Documentation

- Document rationale for mode logic
- Explain hardware constraints when relevant
- Summarize design trade-offs in comments

## Copilot Suggestions

### When Editing Headers

- Propose clear hardware abstraction layers
- Suggest grouping related constants

### When Editing Source Files

- Suggest helper functions over inline logic
- Suggest refactoring duplicated control logic

### When Editing Tests

- Suggest missing coverage for new behavior
- Suggest boundary condition tests

## Review Awareness

When reviewing or modifying code:

- Prefer clarity over cleverness
- Preserve decisions made in prior reviews
- Surface trade-offs explicitly in comments
