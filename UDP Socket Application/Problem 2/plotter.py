import matplotlib.pyplot as plt
import pandas as pd 

# Define paths, image paths, and TTL values
paths = ['./ttl_2.csv', './ttl_4.csv', './ttl_8.csv', './ttl_16.csv']
image_path = './combined_lines.png'
TTLs = [2, 4, 8, 16]

# Initialize figure
plt.figure(figsize=(10, 6))

# Plot each line
for path, ttl in zip(paths, TTLs):
    df = pd.read_csv(path)
    plt.plot(df['Value of payload(in bytes)'], df['Cumulative RTT(in milliseconds)'], label=f'TTL {ttl}')

# Set labels and title
plt.xlabel('Payload Size (bytes)')
plt.ylabel('Cumulative RTT (milliseconds)')
plt.title('Cumulative RTT vs. Payload Size for Different TTL Values')

# Adjust scale
plt.ylim(0, 10)  # Adjust as needed

# Add legend
plt.legend()

# Save image
plt.savefig(image_path)

# Show plot
plt.show()
