import csv
import random

import matplotlib.pyplot as plt


def simulate_boost_converter(vin, duty, resistance):
    """
    Simplified averaged boost-converter model.
    This is a software model, not a physical circuit simulation.
    """
    inductance = 1e-3       # 1 mH
    capacitance = 470e-6    # 470 uF
    inductor_resistance = 0.25
    time_step = 1e-5
    steps = 15000           # 0.15 seconds
    average_last = 3000

    inductor_current = 0.0
    output_voltage = vin
    recent_voltages = []

    for step in range(steps):
        # Averaged converter equations
        current_change = (
            vin - (1 - duty) * output_voltage
            - inductor_resistance * inductor_current
        ) / inductance

        voltage_change = (
            (1 - duty) * inductor_current
            - output_voltage / resistance
        ) / capacitance

        inductor_current = max(
            0.0, inductor_current + current_change * time_step
        )
        output_voltage = max(
            0.0, output_voltage + voltage_change * time_step
        )

        if step >= steps - average_last:
            recent_voltages.append(output_voltage)

    return sum(recent_voltages) / len(recent_voltages)


# Create varied simulated examples for the later ML demonstration.
random.seed(42)
data = []

for _ in range(180):
    vin = random.uniform(9.0, 15.0)
    duty = random.uniform(0.20, 0.50)
    resistance = random.uniform(25.0, 60.0)
    vout = simulate_boost_converter(vin, duty, resistance)

    data.append({
        "input_voltage_V": vin,
        "duty_cycle": duty,
        "load_resistance_ohm": resistance,
        "output_voltage_V": vout,
    })

with open("boost_converter_data.csv", "w", newline="") as file:
    writer = csv.DictWriter(file, fieldnames=data[0].keys())
    writer.writeheader()
    writer.writerows(data)


# Create a chart at 12 V input, comparing three load values.
duty_values = [0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50]

for resistance in [25.0, 40.0, 60.0]:
    output_values = [
        simulate_boost_converter(12.0, duty, resistance)
        for duty in duty_values
    ]
    plt.plot(
        duty_values,
        output_values,
        marker="o",
        label=f"Load resistance = {resistance:.0f} ohm",
    )

plt.title("Simplified Boost Converter Model")
plt.xlabel("PWM duty cycle")
plt.ylabel("Average output voltage (V)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("boost_converter_duty_sweep.png", dpi=200)
plt.close()

print("Simulation finished.")
print("Created: boost_converter_data.csv")
print("Created: boost_converter_duty_sweep.png")
print(f"Simulated examples: {len(data)}")
