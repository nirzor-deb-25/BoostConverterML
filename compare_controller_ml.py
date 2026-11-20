import csv
from pathlib import Path

from sklearn.ensemble import RandomForestRegressor

project_dir = Path(__file__).resolve().parent
data_path = project_dir / "boost_converter_data.csv"
controller_path = project_dir / "firmware" / "controller_test_results.csv"
output_path = project_dir / "controller_ml_comparison.csv"

# Fit the model using the simulated converter dataset.
with data_path.open("r", newline="", encoding="utf-8-sig") as file:
    simulation_rows = list(csv.DictReader(file))

features = [
    [
        float(row["input_voltage_V"]),
        float(row["duty_cycle"]),
        float(row["load_resistance_ohm"]),
    ]
    for row in simulation_rows
]
targets = [float(row["output_voltage_V"]) for row in simulation_rows]

model = RandomForestRegressor(n_estimators=200, random_state=42)
model.fit(features, targets)

# Estimate output using each STM32 test's calculated duty command.
with controller_path.open("r", newline="", encoding="utf-8-sig") as file:
    controller_rows = list(csv.DictReader(file))

output_rows = []

for row in controller_rows:
    input_voltage = 12.0
    load_resistance = 40.0
    duty_cycle = float(row["DutyCommandPct"]) / 100.0

    estimate = float(
        model.predict([[input_voltage, duty_cycle, load_resistance]])[0]
    )
    injected_value = float(row["MeasuredOutputV"])

    output_rows.append({
        **row,
        "InputVoltageV": input_voltage,
        "LoadResistanceOhm": load_resistance,
        "ML_EstimatedOutputV": round(estimate, 3),
        "EstimateMinusInjectedV": round(estimate - injected_value, 3),
    })

fieldnames = list(output_rows[0].keys())

with output_path.open("w", newline="", encoding="utf-8") as file:
    writer = csv.DictWriter(file, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(output_rows)

print(f"Model training rows: {len(simulation_rows)}")
print(f"Controller cases compared: {len(output_rows)}")
print(f"Created: {output_path}")
for row in output_rows:
    print(
        f'{row["Case"]}: duty={row["DutyCommandPct"]}% | '
        f'ML estimate={row["ML_EstimatedOutputV"]} V | '
        f'injected test value={row["MeasuredOutputV"]} V'
    )
