# Smart Farm Physics Simulation Logic Backup

This document captures the physics simulation logic used in the initial demo. This logic creates a realistic response loop where actuators (pumps, fans, lights) affect sensor readings over time, simulating a real physical environment.

## 1. General Principles

- **Hybrid Sensing:**
  - **Large Delta (> Threshold):** Detected as Manual User Input -> Instant update (reset simulation offset).
  - **Small/No Delta:** Apply simulation physics (Sensor Value = Raw Pot Value + Simulation Offset).
- **Simulation Loop:** Runs every cycle (e.g., 200ms).
- **Hysteresis:** Prevents rapid on/off cycling near thresholds.

## 2. Zone A: Air Environment (AirGroup)

### Constants

```cpp
// Rates per update cycle
TEMP_COOLING_RATE = 0.15;   // Fan ON
TEMP_HEATING_RATE = 0.15;   // Heater ON
TEMP_RECOVERY_RATE = 0.05;  // Natural return to ambient

HUM_INCREASE_RATE = 0.3;    // Mist ON
HUM_DECREASE_RATE = 0.1;    // Natural drying

// Hysteresis
TEMP_HYSTERESIS = 2.0;      // °C
HUM_HYSTERESIS = 5.0;       // %
LIGHT_HYSTERESIS = 10;      // %
```

### Logic

- **Temperature:**
  - If `Fan ON`: `simTempOffset -= TEMP_COOLING_RATE`
  - If `Heater ON`: `simTempOffset += TEMP_HEATING_RATE`
  - If `Idle`: Gently adjust `simTempOffset` towards 0 using `TEMP_RECOVERY_RATE`.
- **Humidity:**
  - If `Mist ON`: `simHumOffset += HUM_INCREASE_RATE`
  - If `Idle`: `simHumOffset -= HUM_DECREASE_RATE` (towards 0 or ambient drying).

## 3. Zone B: Soil Environment (SoilGroup)

### Constants

```cpp
// Rates per update cycle
MOIST_INCREASE_RATE = 0.5;    // Pump ON
MOIST_DECREASE_RATE = 0.1;    // Natural drying

NUTRIENT_INCREASE_RATE = 0.8; // Dosing Pump ON
NUTRIENT_DECREASE_RATE = 0.2; // Plant consumption / Leaching

// Hysteresis
MOIST_HYSTERESIS = 10;        // %
NUTRIENT_HYSTERESIS = 30;     // mg/kg
```

### Logic

- **Moisture:**
  - If `Water Pump ON`: `simMoistOffset += MOIST_INCREASE_RATE` (Max 100%)
  - If `Idle`: `simMoistOffset -= MOIST_DECREASE_RATE` (Min 0%)
- **Nutrients (N, P, K independently):**
  - If `Pump N/P/K ON`: `sim[X]Offset += NUTRIENT_INCREASE_RATE`
  - If `Idle`: `sim[X]Offset -= NUTRIENT_DECREASE_RATE` (simulates consumption).
  - **Manual Override:** If Potentiometer is turned to ~0 (<30mg/kg) AND Pump is OFF -> Reset `sim[X]Offset` to 0 (simulates flushing/resetting soil).

### Safety Logic (pH Alarm)

- **Condition:** `soilPH < 5.0` OR `soilPH > 8.0`
- **Action:**
  - Trigger Alarm (Buzzer logic).
  - **LOCKOUT:** Force all Nutrient Pumps (N, P, K) to OFF. Prevents dosing in dangerous pH conditions.

## 4. Zone C: Hydroponics (HydroGroup)

### Constants

```cpp
EC_INCREASE_RATE = 0.05;      // Nutrient Pump ON
EC_DECREASE_RATE = 0.01;      // Natural depletion

EC_HYSTERESIS = 0.2;          // mS/cm
```

### Logic

- **EC (Electrical Conductivity):**
  - If `Nutrient Pump ON`: `simECOffset += EC_INCREASE_RATE`
  - If `Idle`: `simECOffset -= EC_DECREASE_RATE`
- **pH:**
  - Using color-coded display safety levels same as Zone B (Red/Yellow/Green zones).

## 5. Actuator Animations

- **Servos (Fan, Pumps):**
  - Sweep logic: 0° <-> 180°
  - Speed: 15° per cycle.
  - State tracked: `pos`, `direction`.
  - When OFF: Return to Center (90°) or Stop.

## 6. Sensor Mapping

- **Potentiometers (12-bit ADC 0-4095):**
  - Moisture: `map(0-4095) -> 0-100%`
  - pH: `map(0-4095) -> 0-140` (/10.0 for float 0.0-14.0)
  - NPK: `map(0-4095) -> 0-500 mg/kg`
  - EC: `map(0-4095) -> 0-300` (/100.0 for float 0.00-3.00 mS/cm)
  - LDR: `map(0-4095) -> 100-0%` (Inverted)
