cachesimulator: cachesimulator.cpp
	@echo "g++ $^ -o $@"; g++ $^ -o $@

.PHONY: clean
clean:
	@echo "rm cachesimulator"; rm cachesimulator
