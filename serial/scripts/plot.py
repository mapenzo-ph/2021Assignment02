import numpy as np
import math as mat
import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv("results/runs.csv")
data.sort_values(by=['size'], inplace=True, ignore_index=True)

f, ax2 = plt.subplots(figsize=(7,5))
ax2.set_title("execution times serial")
ax2.set_xlabel("points")
ax2.set_ylabel("time")
# log-log plot
ax2.set_xscale("log")
ax2.set_yscale("log")

# get data
x = data["size"].tolist()
y = data["time"].tolist()

# plot points
ax2.plot(x,y,'or')

# perform fit of logs
xlog = [mat.log(el) for el in x]
ylog = [mat.log(el) for el in y]
exp,cost = np.polyfit(xlog, ylog, 1)
fxlog = [el*exp+cost for el in xlog]
fx = [mat.exp(el) for el in fxlog]

ax2.plot(x,fx,'--k')

f.tight_layout()
plt.show()

print(exp, mat.exp(cost))