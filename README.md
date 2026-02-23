 # CodeRace2025 weCAN

 ## Overview
 CodeRace2025 weCAN is an embedded safety support system for vehicles, designed to assist new or inexperienced drivers during critical moments: engine start-up and parking. The system detects unsafe driving conditions using standard CAN bus signals and provides real-time feedback via an OLED display.

 ## Motivation
 Driving at the beginning and end of a trip—when starting the engine or parking—is often challenging for inexperienced drivers. Misjudgments during these moments can lead to:
 - Gear misuse
 - Engine strain
 - Risk of vehicle roll-away

 This project demonstrates how low-cost embedded systems can enhance driver safety by utilizing existing vehicle infrastructure.

 ## Key Features
 - **Gear Detection:** Via CAN ID 0x401, identifies gear state: P (Parking), R (Reverse), N (Neutral), D (Drive).
 - **Engine Status Monitoring:** Via CAN ID 0x380, distinguishes between Pre-Ignition, Ignition, and OFF.
 - **Real-Time Feedback:** Intuitive warnings displayed on a compact OLED screen.
 - **Minimal Hardware Setup:** Arduino-compatible MCU, MCP2515 CAN module, SH1106 OLED.

 ## Use Case Scenarios
 - **Start-Up Reminder:** When the driver turns on the ignition and prepares to shift to Drive (D), the system displays: "Press brake fully"—guiding new drivers to follow safe procedures.
 - **Parking Warning:** Engine is turned OFF but gear is not in P. The system displays: "Warning: Roll | Instruction: Shift to P or Hold Brake"—preventing potential rollback incidents.

 ## Hardware Modules
 - **Arduino Mega 2560:** Central node for receiving CAN messages, interpreting signals, and managing display logic.
 - **Arduino Nano:** Dedicated CAN message simulator, emulating gear-related signals via a 4x4 matrix keypad.
 - **MCP2515 CAN Bus Module (x2):** One for receiving, one for simulating CAN messages.
 - **4x4 Matrix Keypad:** User input for gear simulation.
 - **SH1106 OLED Display (128x64):** Displays real-time driving status and safety reminders.
 - **SPI and I2C Wiring:** Efficient integration of input/output devices.

 ## CAN Messages & Signal Mapping
 - **CVT_191 (CAN ID 0x401):** Byte 0, bits 0–3 represent gear state (P, R, N, D).
 - **ENG_17C (CAN ID 0x380):** Byte 5, bit 5 (Pre-Ignition), bit 7 (Ignition ON).
 - **Custom Simulation Message:** Arduino Nano generates messages matching real vehicle layout for testing.

 ## Algorithm & Logic
 1. **System Start:** Receives CAN data every 0.5s (engine status, gear position).
 2. **Engine State Check:** Determines ON/OFF state.
 3. **Startup Instructions:** If engine just turned on and gear is P, display instruction.
 4. **Stop & Gear Position Check:** If engine is OFF and gear is not P, display warning.
 5. **Warnings:**
	 - Startup: "Press the brake pedal fully before shifting gears."
	 - Parking: "The car roll-away risk."
	 - Instruction: "Shift to P mode or Hold Brake"

 ## Simulation Node (Arduino Nano)
 - Emulates CAN messages for gear and engine states using keypad input.
 - Sends messages conforming to CVT_191 and ENG_17C layout.
 - Allows controlled testing without a real vehicle ECU.

 ## Main MCU Logic (Arduino Mega)
 - Receives and decodes CAN messages.
 - Extracts gear and engine status bits.
 - Displays warnings and instructions on OLED.
 - Clears warnings after 3 seconds of stable state.

 ## Impact
 - Early driver confidence
 - Safer behavior during parking and start-up
 - Scalable, maintainable support solution for smart mobility

 ## Why It Matters
 - Beginner-friendly
 - Safety-oriented
 - Simple, reliable, embedded
 - Bosch-relevant: aligns with affordable vehicle intelligence based on ECU data

 ## Getting Started
 1. Connect hardware modules as described.
 2. Upload simulation code to Arduino Nano and receiver code to Arduino Mega.
 3. Use keypad to simulate gear and engine states.
 4. Observe real-time feedback on OLED display.

 ## License
This project is for educational and demonstration purposes.

---
Team weCAN | CodeRace2025

## System Testing and Validation
To ensure the system behaves as intended across various driving scenarios, a series of targeted test cases were conducted. These tests validate the receiver’s logic, confirming whether appropriate visual warnings are displayed on the OLED screen and ensuring safe driving prompts are triggered only when necessary. Each test simulates a realistic driving context using the simulation node and observes the system’s real-time response.

### Test Cases
**Test Case 1: Parking Without Shifting to P**
- Condition: engineProgress = 0, gearState = 1 (Reverse)
- Expected: OLED displays rollback warning: "Warning: Roll\nShift to P or\nHold Brake"
- Result: System correctly identifies unsafe gear state post-engine-off and provides visual cues.

**Test Case 2: Engine Start in Park**
- Condition: engineProgress = 1, enginePreProgress = 1, gearState = 0 (Park)
- Expected: OLED prompts "Press brake fully"
- Result: System reminds user to apply brake before gear transition.

**Test Case 3: Sudden Engine Shutdown While in N**
- Condition: engineProgress = 0, gearState = 2 (Neutral)
- Expected: OLED shows rollback warning
- Result: System alerts driver, preventing uncontrolled rolling.

**Test Case 4: Re-Start in Neutral After Stall**
- Condition: engineProgress = 1, enginePreProgress = 1, gearState = 2 (Neutral)
- Expected: No warning needed
- Result: System suppresses warnings, allowing uninterrupted operation during intentional restarts.

**Test Case 5: Normal Driving Scenario**
- Condition: engineProgress = 1, gearState = 3 (Drive), enginePreProgress = 0
- Expected: No warnings, only regular gear and engine status shown
- Result: System refrains from unnecessary alerts.

**Summary:**
The warning logic accurately distinguishes between critical and non-critical states, ensuring driver feedback is contextually relevant. These results affirm the reliability of the system in assisting drivers with safe startup and parking operations.

## Innovation
This project introduces a compact, low-cost driver assistance system designed specifically to support startup and parking procedures—critical phases where inexperienced drivers are most prone to errors.

### Simple Yet Effective Signal Usage
The system relies solely on standard CAN signals:
- gearState (from CAN ID 0x401): Identifies current gear (P, R, N, D).
- enginePreProcess and engineProcess (from CAN ID 0x380): Monitor engine states OFF/ON.

### Real-Time Behavior Monitoring
By analyzing the timing and sequence of gear and engine signals, the system can:
- Detect unsafe actions
- Identify incorrect parking steps
- Provide immediate feedback via OLED display, helping new drivers form safe habits

### Low-Cost and Easy Integration
The system uses Arduino-compatible microcontroller, MCP2515 CAN module, and SH1106 OLED screen, ensuring minimal cost and straightforward deployment in personal vehicles, training cars, or fleet applications.

### Potential Future Enhancements
- Personalized Behavior Analysis: Learn driver habits over time to provide tailored feedback and reduce false warnings.
- Training Support: Record incorrect startup/parking sequences for review by instructors or learners; useful for driving schools.
- Optional Sensor Add-ons: Integrate basic sensors like brake switch or door status for improved accuracy, while keeping the core system lightweight.


## Cách chạy thử
(Hướng dẫn chạy code hoặc build)


