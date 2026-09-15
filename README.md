# Distributed Grep

Run one `grep` across the log files of 10 machines in parallel and merge the
results on the client. One thread per machine; a slow or dead machine costs
its own result, not the query.

## Build

```sh
make
make clean
```

## Run (For a computer having access to remote machine.)

Machines are listed in `config/machines-remote.txt` as `id ip port`.

```sh
./remote.sh deploy      # build on all 10 hosts, then generate & push logs
./remote.sh start [id]  # start servers (all, or one)
./remote.sh stop  [id]  # stop servers
```

Query from any machine that can reach the cluster — the client reads one
command line from stdin:

```sh
echo "grep -c 'ERROR'" | ./bins/client
```

## Metrics

Prefix a query with `metrics|` to enable timing on both ends:

```sh
echo "metrics|grep 'ERROR'" | ./bins/client
```

CSV rows (`timestamp_us,role,id,name,duration_us,extra`) land in
`metrics/<role>.<id>.metrics.log`. `./profiling.sh` runs three patterns ten
times each to collect a latency sample.

## Tests (For a remote computer)

`test/UnitTest.cpp`

The test should be executed on one of the machines.

```sh
make test
```
