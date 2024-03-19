import matplotlib.pyplot as plt
import pandas as pd

csv_paths = ['./ttl_2.csv', './ttl_4.csv', './ttl_8.csv', './ttl_16.csv']
ttl_values = [2, 4, 8, 16] 

plt.figure(figsize=(10, 6))  

colors = ['blue', 'green', 'red', 'purple']

for path, ttl, color in zip(csv_paths, ttl_values, colors):
    df = pd.read_csv(path)
    plt.plot(df['Value of payload(in bytes)'], df['Cumulative RTT(in milliseconds)'], label=f'TTL={ttl}', color=color)

plt.title('Cumulative RTT vs. Payload Size for Different TTL Values')
plt.xlabel('Payload Size (bytes)')
plt.ylabel('Cumulative RTT (milliseconds)')
plt.legend()
plt.grid(True)

plt.show()
