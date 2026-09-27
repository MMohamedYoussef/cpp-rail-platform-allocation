# Rail platform allocation

Assign trains to the fewest platforms needed for a one-day timetable. Trains are processed by arrival time; a platform becomes available exactly at its current train's departure. A min-heap of departures and a min-heap of free platform IDs keep the allocation deterministic.

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o platforms
./platforms fixtures/trains.csv example-out.csv
python3 tests/test_cli.py ./platforms
```

Input is unquoted CSV with the exact header `train,arrival,departure`. Train IDs must be unique. Times use 24-hour `HH:MM`, and each departure must be later than its arrival on the same day. The output is `train,platform,arrival,departure`, sorted by arrival and then ID. The demo produces [examples/allocation.csv](examples/allocation.csv) and prints `platforms=2 trains=3`.

At most 100,000 trains are accepted. Train IDs beginning with `=`, `+`, `-`, or `@` after leading whitespace are rejected to keep exported CSV safe to open in a spreadsheet. Overnight service, dwell buffers, platform compatibility, and quoted CSV fields are outside this model. Output is staged and published only if the path is absent; this requires hard-link support in the output directory. Bad input exits nonzero with a diagnostic.
