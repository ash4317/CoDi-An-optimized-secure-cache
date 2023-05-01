# import numpy as np
# import numpy as np
# from scipy.interpolate import make_interp_spline
# import matplotlib.pyplot as plt

# # Dataset
# x = np.array([
#     1,
#     2,
#     3,
#     4,
#     5,
#     6,
#     7
# ])

# # Miss rate
# y = np.array([59.067,59.587,61.301,61.564,63.546,63.36,62.511])

# # % displacement overflows
# z = np.array([
#     99.27,
#     98.45,
#     26.85,
#     6.2,
#     2.45,
#     0.42,
#     0.16
# ])

# plt.ylim(0, 100)

# # Plotting the Graph
# plt.plot(x, y, color="red")
# plt.scatter(x, y)
# plt.title("cactusADM: Miss rate vs % of invalid lines")
# plt.xlabel("% of invalid lines")
# plt.ylabel("Miss Rate")

# # fig, ax = plt.subplots()
# # fig.canvas.draw()
# # labels = [
# #     "1",
# #     "per skew",
# #     "1%",
# #     "2%",
# #     "5%",
# #     "8%",
# #     "10%",
# # ]
# # ax.set_xticklabels(labels)

# # plt.plot(x, z, color="blue")
# # plt.title("cactusADM: % displacement overflows vs % of invalid lines")
# # plt.xlabel("% of invalid lines")
# # plt.ylabel("% Displacement Overflow")

# txt = ["1", "per skew", "1%", "2%", "5%", "8%", "10%"]
# for i in range(len(txt)):
#     plt.annotate(txt[i], (x[i], y[i]),rotation=60)

# plt.show()

import matplotlib.pyplot as plt

# Data
x_axis = [1,2,3,4,5,6]
# y_axis = [25,189439,82370,705013,502702,367588]    # sse
# y_axis = [99.949,98.76,98.584,94.774,95.642,95.015] # miss rate
y_axis = [99.28,14.83,11.37,0.17,0.15,0.74] # % displacement overflow
x_labels = ["", "1", "1%", "2%", "5%", "8%", "10%"]

# Create figure and axes objects
fig, ax = plt.subplots()

# plt.ylim(0, 100)

plt.scatter(x_axis, y_axis) # sse

# Plot the line
ax.plot(x_axis, y_axis)

# Set the x-axis tick labels
ax.set_xticklabels(x_labels)

# Set the x-axis label
ax.set_xlabel("% of invalid lines")

# Set the y-axis label
# ax.set_ylabel("SSE")
# ax.set_ylabel("Miss Rate")
ax.set_ylabel("% displacement overflow")

# Set the title
# plt.title("bwaves: SSE vs % of invalid lines")
# plt.title("bwaves: Miss Rate vs % of invalid lines")
plt.title("bwaves: % displacement overflow vs % of invalid lines")

# Show the plot
plt.show()
