Instructions to compile the code:
    - choose version
    - `cd` into directory
    - (MPI ONLY: `module load openmpi-4.1.1+gnu-9.3.0`)
    - `make`

then run the code with the desired number of processes. 

By default the code takes the data from the `test_data.csv` file, and reads 50 data points. To test on a bigger dataset, first generate it using the provided python script (run it with no args to print usage) or anything else, the data need to be formatted as follows: each point on a different line, coordinates separated by a comma (actually this can be modified changing the `#define SEP ,` line in the code). After generating the dataset, change the `#define NPTS` in the file (or remove it and provide a suitable definition in the makefile CFLAGS, like `-DNPTS=...`).

