import numpy as np
import matplotlib.pyplot as plt
 
# set width of bar
barWidth = 0.3
fig = plt.subplots(figsize =(10, 7.5))
 
# set height of bar
one_victim = [19, 19, 22, 17]
multiple_victims = [526, 600, 1434, 834]
# IT = [12, 30, 1, 8, 22]
# ECE = [28, 6, 16, 5, 10]
# CSE = [29, 3, 24, 25, 17]
 
# Set position of bar on X axis
br1 = np.arange(len(one_victim))
br2 = [x + barWidth for x in br1]
 
# Make the plot
plt.bar(br1, one_victim, color ='r', width = barWidth,
        edgecolor ='grey', label ='One Victim')
plt.bar(br2, multiple_victims, color ='g', width = barWidth,
        edgecolor ='grey', label ='Multiple Victims')
# plt.bar(br3, CSE, color ='b', width = barWidth,
#         edgecolor ='grey', label ='CSE')
 
# Adding Xticks
plt.xlabel('SPEC06 benchmark', fontweight ='bold', fontsize = 15)
plt.ylabel('SSE', fontweight ='bold', fontsize = 15)
plt.xticks([r + barWidth for r in range(len(one_victim))],
        ['cactusADM', 'bzip2', 'GemsFDTD', 'zeusmp'])
 
plt.legend()
plt.show()