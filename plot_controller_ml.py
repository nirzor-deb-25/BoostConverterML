import csv
from pathlib import Path

import matplotlib.pyplot as plt

project_dir = Path(__file__).resolve().parent
input_path = project_dir / "controller_ml_comparison.csv"
output_path = project_dir / "controller_ml_comparison.png"

with input_path.open("r", newline="", encoding="utf-8-sig") as file:
    rows = list(csv.DictReader(file))

cases = [row["Case"] for row in rows]
injected = [float(row["MeasuredOutputV"]) for row in rows]
estimates = [float(row["ML_EstimatedOutputV"]) for row in rows]

x_positions = list(range(len(rows)))
bar_width = 0.36
mae = sum(abs(estimate - value) for estimate, value in zip(estimates, injected)) / len(rows)

fig, ax = plt.subplots(figsize=(9, 5.5))
ax.bar(
    [x - bar_width / 2 for x in x_positions],
    injected,
    bar_width,
    label="Injected synthetic test value",
)
ax.bar(
    [x + bar_width / 2 for x in x_positions],
    estimates,
    bar_width,
    label="ML estimate",
)

ax.set_title(f"STM32 Test Cases and ML Estimates (3-case MAE = {mae:.3f} V)")
ax.set_ylabel("Output voltage (V)")
ax.set_xticks(x_positions)
ax.set_xticklabels(cases)
ax.grid(axis="y", alpha=0.3)
ax.legend()
fig.tight_layout()
fig.savefig(output_path, dpi=180)
plt.close(fig)

print(f"Created: {output_path}")
