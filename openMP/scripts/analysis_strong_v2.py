import pandas as pd
import matplotlib.pyplot as plt

# get strong scaling times
data = pd.read_csv("results/strong_v2.csv")
data.sort_values(by=['nprocs'], inplace=True, ignore_index=True)
n = data["nprocs"].tolist()
t = data["time"].tolist()

# compute speedups
su = [round(t[0]/el,2) for el in t]
data["speedup"] = su

# compute strong scaling efficiency
eff = [round(su[i]/n[i],2) for i in range(len(n))]
data["eff"] = eff

# compute parallel fraction using Amdahl's Law
# S(n) = 1/((pf/n)+(1-pf)) => pf*S/n + S -pf*S = 1 => pf*S(1/n - 1) = (1-S) => pf = (S-1)*n/(S*(n-1)) 
pf = [round((su[i]-1)/su[i] * n[i]/(n[i]-1),2) if i != 0 else 0.00 for i in range(len(n))]
sf = [1-el for el in pf]

data["s"] = sf
data["p"] = pf

# print table
print(data.to_latex(columns=["nprocs", "time", "speedup", "eff", "s", "p"], index=False))

# plot speedup vs expected from Amdahl's law
max_pf = max(pf[1:])
exp_su = [1/((max_pf/np)+(1-max_pf)) if np != 1 else 1.00 for np in range(1,max(n)+1)]

f, ax = plt.subplots(figsize=(7,5))
ax.set_xlabel("OpenMP threads")
ax.set_ylabel("speedup")
ax.plot(n, su, 'ro')
ax.plot(range(1,max(n)+1), exp_su, '--k')
f.tight_layout()
plt.show()
