## Tested Environment
- Ubuntu 20.04
- Clang++ 10.0.0

## Compile
- make

## Build Graph
Based on the random arrival model, generate the initial graph and the edge update.
```sh
# convert the input graph from a text format to a binary format
format <data_path> --directed|undirected

# split the binary graph: basic graphs and the remaining edges for insertion
divide <data_path> --base_ratio <size ratio of basic graph>

# generate workloads
# workload format:
#   i<num insert>d<num delete>q<num query>k<topk>
#   <topk> = 0 to generate full queries
process <data_path> [workloads]
```
Example: 
```sh
# The dataset dblp is an undirected graph
./format datasets/dblp --undirected

# we randomly select 90% edges from the oringal graph as the inital graph 
./divide datasets/dblp --base_ratio 0.9

# generate a work with 100 insertions and 50 queries
./process datasets/dblp i100d0q50k0
```

# Run  a workload
Run a certain algorithm to process a given workload.
```sh
firm <algo_name> <data_path> [options]
```
Descriptions:
- algo_name: firm, agenda, fora, fora+, agenda*, exact
- options:
  - alpha: the decay factor of the random walk. It is 0.2 by default.
  - epsilon: the error bound of the approximation guarantee
  - index_ratio: the value of $r_{max} \cdot \omega$ in the paper, which is used to control the index size
  - round: the number of rounds when runing the power method 
  - workloads: the workload list
  - output: whether to save the computing result.

Example:
```sh
# use the algorithm firm to handle the workload consisting of 100 insertions and 50 queries on dataset dblp
./firm firm datasets/dblp --workloads i100d0q50k0 --epsilon 0.5

# use the algorithm agenda
./firm agenda datasets/dblp --workloads i100d0q50k0 --epsilon 0.5
```
