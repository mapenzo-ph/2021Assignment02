from turtle import clear
import matplotlib.pyplot as plt

mpi = [1.00, 0.82, 0.61, 0.42, 0.27]
omp = [1.00, 0.80, 0.64, 0.46, 0.27]
tasks = [1, 2, 4, 8, 16]

f,ax = plt.subplots(figsize=(6,5))
ax.set_xlabel("tasks")
ax.set_ylabel("weak efficiency")
ax.plot(tasks, mpi, 'o', label='MPI')
ax.plot(tasks, mpi, '--k')
ax.plot(tasks, omp, 'o', label='OpenMP')
ax.plot(tasks, omp, ':k')
ax.legend(loc="upper right")
f.tight_layout()
plt.show()