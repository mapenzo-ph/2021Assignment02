import pandas as pd
import matplotlib.pyplot as plt

# get strong scaling times
data = pd.read_csv("results/weak.csv")
data.sort_values(by=['nprocs'], inplace=True, ignore_index=True)

# separate single proc from multiproc and merge back in single df
multiple = data[data.nprocs != 1].sort_values(by=["size"], ignore_index=True)
multiple.rename(columns={"time":"tp"}, inplace=True)

single = data[data.nprocs == 1].sort_values(by=["size"], ignore_index=True)
single.drop(labels=["nprocs"], inplace=True, axis=1)
single.rename(columns={"time":"ts"}, inplace=True)


new_data = single.merge(multiple, on=["size"], how="outer")
new_data.at[0, "nprocs"] = 1
new_data.at[0, "tp"] = new_data.at[0, "ts"]
cols = ["nprocs", "size", "ts", "tp"]
new_data = new_data[cols]
new_data["nprocs"] = [int(el) for el in new_data.nprocs.tolist()]

# get scaled speedup
ts = new_data.ts.tolist()
tp = new_data.tp.tolist()
ssu = [round(ts[i]/tp[i],2) for i in range(len(ts))]
new_data["speedup"] = ssu

# compute weak scaling efficiency
eff = [round(tp[0]/tp[i], 2) for i in range(len(tp))]
new_data["eff"] = eff

# compute parallel fraction from scaled speedup using Gustafson's law 
# s(n) = sf+n*pf = 1-pf+n*pf = 1 + (n-1)*pf => pf = (s(n)-1)/n-1
n = new_data.nprocs.tolist()
pf = [round((ssu[i]-1)/(n[i]-1), 2) if i != 0 else 0.00 for i in range(len(n))]
sf = [1-el for el in pf]
new_data["sf"] = sf
new_data["pf"] = pf

# print table
print(new_data.to_latex(columns=["nprocs", "size", "ts", "tp", "speedup", "eff", "sf", "pf"], index=False))

# plot parallel efficiency from Gustafson's law
f, ax = plt.subplots(figsize=(7,5))
ax.set_xlabel("MPI processes")
ax.set_ylabel("efficiency")
ax.plot(n, eff, 'ro')
#ax.plot(range(1,max(n)+1), exp_su, '--k')
f.tight_layout()
plt.show()
