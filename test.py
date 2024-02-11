import matplotlib.pyplot as plt

# Function to read data from file
def read_data_from_file(filename):
    time_values = []
    values = []
    colors = []
    with open(filename, 'r') as file:
        for _ in range(30):
            next(file)
        for line in file:
            # Assuming each line in the file is formatted as "TIME VALUE COLOR"
            time, value, color = line.split()
            time_values.append(float(time))
            values.append(float(value))
            colors.append(float(color))
    return time_values, values, colors

# Replace 'data.txt' with your file name
filename = 'data.txt'
time_values, values, colors = read_data_from_file(filename)

# Plotting
plt.scatter(time_values, values, c=colors, cmap='viridis', marker='o')

# Adding labels and title
plt.xlabel('TIME')
plt.ylabel('VALUE')
plt.title('Plot of TIME vs VALUE')

# Displaying the plot
plt.grid(True)
plt.show()
