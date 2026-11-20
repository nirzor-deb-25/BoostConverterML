import csv
import math

import matplotlib.pyplot as plt
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error, mean_squared_error
from sklearn.model_selection import train_test_split

with open("boost_converter_data.csv", newline="") as file:
    rows = list(csv.DictReader(file))

feature_names = [
    "input_voltage_V",
    "duty_cycle",
    "load_resistance_ohm",
]

X = [[float(row[name]) for name in feature_names] for row in rows]
y = [float(row["output_voltage_V"]) for row in rows]

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.25, random_state=42
)

model = RandomForestRegressor(
    n_estimators=200,
    min_samples_leaf=2,
    random_state=42,
)
model.fit(X_train, y_train)

predictions = model.predict(X_test)

mae = mean_absolute_error(y_test, predictions)
rmse = math.sqrt(mean_squared_error(y_test, predictions))

print(f"Training examples: {len(X_train)}")
print(f"Test examples: {len(X_test)}")
print(f"MAE: {mae:.3f} V")
print(f"RMSE: {rmse:.3f} V")

example = model.predict([[12.0, 0.40, 40.0]])[0]
print(f"Example ML estimate: {example:.2f} V")

low = min(min(y_test), min(predictions))
high = max(max(y_test), max(predictions))

plt.figure(figsize=(7, 6))
plt.scatter(y_test, predictions, alpha=0.8)
plt.plot([low, high], [low, high], "r--", label="Perfect estimate")
plt.xlabel("Simulated output voltage (V)")
plt.ylabel("ML-estimated output voltage (V)")
plt.title(f"ML Estimates vs Simulation Results (MAE = {mae:.2f} V)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.savefig("ml_prediction_comparison.png", dpi=200)
plt.close()

print("Created: ml_prediction_comparison.png")