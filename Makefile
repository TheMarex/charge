DATA_DIR=data
CACHE_DIR ?=cache
RESULTS_DIR=results
DATASET ?=switzerland
GRAPH_PROVIDER ?=osm
CHARGING_PROVIDER ?=tesla
CHARGING_PENALTY ?=60
GRAPH_TYPE ?=unexpanded
CAPACITY ?=16000
HOST ?=$(shell hostname)
THREADS ?=1
PREFIX_CMD=$(if $(DEBUG), gdb --ex "r" --args)
SEED ?=1337
NUM_RUNS ?=1
NUM_QUERIES ?=100
X_EPS ?=0.1
Y_EPS ?=1
SAMPLE_RATE ?=10
MAX_TIME ?=infinity
POSTFIX ?=
BUILD_DIR ?=./build
POTENTIAL ?=lazy_omega

$(info **********************************)
$(info *** DATASET is [${DATASET}])
$(info *** GRAPH_PROVIDER is [${GRAPH_PROVIDER}])
$(info *** CHARGING_PROVIDER is [${CHARGING_PROVIDER}])
$(info *** GRAPH_TYPE is [${GRAPH_TYPE}])
$(info *** CAPACITY is [${CAPACITY}] Wh)
$(info *** CHARGING_PENALTY is [${CHARGING_PENALTY}] s)
$(info *** THREADS is [${THREADS}])
$(info *** NUM_RUNS is [${NUM_RUNS}])
$(info *** NUM_QUERIES is [${NUM_QUERIES}])
$(info *** X_EPS is [${X_EPS}])
$(info *** Y_EPS is [${Y_EPS}])
$(info *** SAMPLE_RATE is [${SAMPLE_RATE}])
$(info *** SEED is [${SEED}])
$(info *** POTENTIAL is [${POTENTIAL}])
$(info **********************************)

DATA_BASE=$(DATA_DIR)/graphs/$(DATASET)
PHEM_BASE=$(DATA_DIR)/phem/PHEMData
SRTM_BASE=$(DATA_DIR)/srtm
RESULTS_BASE=$(RESULTS_DIR)/$(HOST)/$(GRAPH_PROVIDER)_$(CHARGING_PROVIDER)/$(DATASET)/$(GRAPH_TYPE)
EV_NETWORK=$(DATA_BASE).rgr $(DATA_BASE).rct $(DATA_BASE).vmd $(DATA_BASE).emd
OSM_NETWORK=$(DATA_BASE).osm.pbf
CACHE_BASE=$(CACHE_DIR)/$(GRAPH_PROVIDER)_$(CHARGING_PROVIDER)/$(DATASET)/$(GRAPH_TYPE)
BASE_GRAPH=$(CACHE_BASE)/first_out $(CACHE_BASE)/head $(CACHE_BASE)/weight $(CACHE_BASE)/coordinates
CHARGERS=$(CACHE_BASE)/charger

UNEXPANDED_CACHE_BASE=$(CACHE_DIR)/$(GRAPH_PROVIDER)_$(CHARGING_PROVIDER)/$(DATASET)/unexpanded
UNEXPANDED_BASE_GRAPH=$(UNEXPANDED_CACHE_BASE)/first_out $(UNEXPANDED_CACHE_BASE)/head $(UNEXPANDED_CACHE_BASE)/weight $(UNEXPANDED_CACHE_BASE)/coordinates

UNEXPANDED_OSM_CACHE_BASE=$(CACHE_DIR)/osm_$(CHARGING_PROVIDER)/$(DATASET)/unexpanded
UNEXPANDED_OSM_BASE_GRAPH=$(UNEXPANDED_OSM_CACHE_BASE)/first_out $(UNEXPANDED_OSM_CACHE_BASE)/head $(UNEXPANDED_OSM_CACHE_BASE)/weight $(UNEXPANDED_OSM_CACHE_BASE)/coordinates

UNEXPANDED_TESLA_CACHE_BASE=$(CACHE_DIR)/$(GRAPH_PROVIDER)_tesla/$(DATASET)/unexpanded
UNEXPANDED_TESLA_BASE_GRAPH=$(UNEXPANDED_TESLA_CACHE_BASE)/first_out $(UNEXPANDED_TESLA_CACHE_BASE)/head $(UNEXPANDED_TESLA_CACHE_BASE)/weight $(UNEXPANDED_TESLA_CACHE_BASE)/coordinates

EXPANDED_CACHE_BASE=$(CACHE_DIR)/$(GRAPH_PROVIDER)_$(CHARGING_PROVIDER)/$(DATASET)/expanded

.PHONY: clean experiments run verify

all: graph zero_turn_graph static_turn_graph max_turn_graph avg_consumption_turn_graph avg_consumption_turn_graph

# Data

graph: $(UNEXPANDED_BASE_GRAPH) $(UNEXPANDED_CACHE_BASE)/charger
zero_turn_graph: $(EXPANDED_CACHE_BASE)/zero/first_out $(EXPANDED_CACHE_BASE)/zero/charger
static_turn_graph: $(EXPANDED_CACHE_BASE)/static/first_out $(EXPANDED_CACHE_BASE)/static/charger
max_turn_graph: $(EXPANDED_CACHE_BASE)/max/first_out $(EXPANDED_CACHE_BASE)/max/charger
avg_consumption_turn_graph: $(EXPANDED_CACHE_BASE)/avg_consumption/first_out $(EXPANDED_CACHE_BASE)/avg_consumption/charger
avg_consumption_static_turn_graph: $(EXPANDED_CACHE_BASE)/avg_consumption_static/first_out $(EXPANDED_CACHE_BASE)/avg_consumption_static/charger

$(UNEXPANDED_OSM_BASE_GRAPH): $(BUILD_DIR)/osm2graph
	mkdir -p $(UNEXPANDED_OSM_CACHE_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/osm2graph $(DATA_BASE) $(PHEM_BASE) $(SRTM_BASE) $(UNEXPANDED_OSM_CACHE_BASE)

$(UNEXPANDED_TESLA_CACHE_BASE)/charger: $(DATA_DIR)/tesla_charger.geojson $(UNEXPANDED_BASE_GRAPH)
	mkdir -p $(UNEXPANDED_TESLA_CACHE_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/geojson2charger $< $(UNEXPANDED_TESLA_CACHE_BASE) $(UNEXPANDED_TESLA_CACHE_BASE)

$(EXPANDED_TESLA_CACHE_BASE)/%/charger: $(DATA_DIR)/tesla_charger.geojson $(EXPANDED_CACHE_BASE)/%/first_out
	mkdir -p $(EXPANDED_TESLA_CACHE_BASE)/$*
	$(PREFIX_CMD) $(BUILD_DIR)/geojson2charger $< $(EXPANDED_TESLA_CACHE_BASE)/$* $(EXPANDED_TESLA_CACHE_BASE)/$*

$(EXPANDED_CACHE_BASE)/%/first_out\
$(EXPANDED_CACHE_BASE)/%/head\
$(EXPANDED_CACHE_BASE)/%/weight\
$(EXPANDED_CACHE_BASE)/%/heights\
$(EXPANDED_CACHE_BASE)/%/coordinates: $(UNEXPANDED_BASE_GRAPH) $(BUILD_DIR)/graph2turngraph
	mkdir -p $(EXPANDED_CACHE_BASE)/$*
	$(PREFIX_CMD) $(BUILD_DIR)/graph2turngraph $* $(UNEXPANDED_CACHE_BASE) $(EXPANDED_CACHE_BASE)/$*

# Queries

$(CACHE_BASE)/random_$(NUM_QUERIES).csv: $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/generate_queries
	$(PREFIX_CMD) $(BUILD_DIR)/generate_queries random $(SEED) $(NUM_QUERIES) $(THREADS) $(CACHE_BASE) $(CAPACITY) $@

$(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv: $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/generate_queries
	$(PREFIX_CMD) $(BUILD_DIR)/generate_queries random_in_range $(SEED) $(NUM_QUERIES) $(THREADS) $(CACHE_BASE) $* $@

$(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv: $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/generate_queries
	$(PREFIX_CMD) $(BUILD_DIR)/generate_queries random_in_charger_range $(SEED) $(NUM_QUERIES) $(THREADS) $(CACHE_BASE) $* $@

$(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv: $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/generate_queries
	$(PREFIX_CMD) $(BUILD_DIR)/generate_queries rank_in_charger_range $(SEED) $(NUM_QUERIES) $(THREADS) $(CACHE_BASE) $* $@

$(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES)_ranks.csv: $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BUILD_DIR)/generate_ranks
	$(PREFIX_CMD) $(BUILD_DIR)/generate_ranks $(THREADS) $(CACHE_BASE) $< $@

ranks: $(CACHE_BASE)/random_in_charger_range_$(CAPACITY)wh_$(NUM_QUERIES)_ranks.csv

.PRECIOUS: $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv \
		$(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES)_ranks.csv \
		$(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv \
		$(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv \
		$(CACHE_BASE)/random_$(NUM_QUERIES).csv

# Experiments

$(RESULTS_BASE)/random_in_range_%wh_dijkstra$(POSTFIX): $(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(BUILD_DIR)/dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/dijkstra_experiment $(CACHE_BASE)/random_in_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE)

# Battery Range

$(RESULTS_BASE)/random_in_range_%wh_mc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(BUILD_DIR)/mc_dijkstra_experiment $(X_EPS) $(Y_EPS)
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/mc_dijkstra_experiment $(CACHE_BASE)/random_in_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* 10 $(X_EPS) $(Y_EPS)

$(RESULTS_BASE)/random_in_range_%wh_mcc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/mcc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/mcc_dijkstra_experiment $(CACHE_BASE)/random_in_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest 10 $(X_EPS) $(Y_EPS) $(SAMPLE_RATE) $(MAX_TIME)

$(RESULTS_BASE)/random_in_range_%wh_fp_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(BUILD_DIR)/fp_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fp_dijkstra_experiment $(CACHE_BASE)/random_in_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest $(X_EPS) $(Y_EPS)

$(RESULTS_BASE)/random_in_range_%wh_fpc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/random_in_range_%wh_fpc_dijkstra_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* omega $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

# Charger Range

$(RESULTS_BASE)/random_in_charger_range_%wh_dijkstra$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(BUILD_DIR)/dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE)

$(RESULTS_BASE)/random_in_charger_range_%wh_mcc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/mcc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/mcc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest 10 $(X_EPS) $(Y_EPS) $(SAMPLE_RATE) $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_mcc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/mcc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/mcc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* lazy_fastest 10 $(X_EPS) $(Y_EPS) $(SAMPLE_RATE) $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* omega $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_lazy_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* lazy_omega $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* lazy_fastest $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_none_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* none $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

# Rank

$(RESULTS_BASE)/rank_in_charger_range_%wh_mcc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/mcc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/mcc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest 10 $(X_EPS) $(Y_EPS) $(SAMPLE_RATE) $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_mcc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/mcc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/mcc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* lazy_fastest 10 $(X_EPS) $(Y_EPS) $(SAMPLE_RATE) $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* fastest $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* omega $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_lazy_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* lazy_omega $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* lazy_fastest $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_none_$(X_EPS)_$(Y_EPS)$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* none $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) none $(MAX_TIME)

# Heuristic Experiments

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_linear$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) linear $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_min_rate$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) min_rate $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_fast$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) only_fast $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_super_charger$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) no_super_charger $(MAX_TIME)

$(RESULTS_BASE)/random_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_slow_charger$(POSTFIX): $(CACHE_BASE)/random_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/random_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) no_slow_charger $(MAX_TIME)


$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_linear$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) linear $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_min_rate$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) min_rate $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_fast$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) only_fast $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_super_charger$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) no_super_charger $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_slow_charger$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) no_slow_charger $(MAX_TIME)

$(RESULTS_BASE)/rank_in_charger_range_%wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_slow_charger_min_rate$(POSTFIX): $(CACHE_BASE)/rank_in_charger_range_%wh_$(NUM_QUERIES).csv $(BASE_GRAPH) $(CHARGERS) $(BUILD_DIR)/fpc_dijkstra_experiment
	mkdir -p $(RESULTS_BASE)
	$(PREFIX_CMD) $(BUILD_DIR)/fpc_dijkstra_experiment $(CACHE_BASE)/rank_in_charger_range_$*wh_$(NUM_QUERIES).csv $(NUM_RUNS) $(THREADS) $@ $(CACHE_BASE) $* $(POTENTIAL) $(X_EPS) $(Y_EPS) $(CHARGING_PENALTY) no_slow_charger_min_rate $(MAX_TIME)

# Experiment aliases

range_dijkstra_experiments: $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_dijkstra$(POSTFIX)

range_mc_experiments: $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_mc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)

range_mcc_experiments: $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_mcc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)

range_fp_experiments: $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_fp_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)

range_fpc_experiments: $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_fpc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)


charger_range_dijkstra_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_dijkstra

charger_range_mcc_fastest_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_mcc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)

charger_range_mcc_lazy_fastest_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_mcc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)

charger_range_fpc_fastest_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)

charger_range_fpc_omega_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX)

charger_range_fpc_lazy_fastest_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)

charger_range_fpc_lazy_omega_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_lazy_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX)

charger_range_fpc_none_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_none_$(X_EPS)_$(Y_EPS)$(POSTFIX)


charger_range_fpc_linear_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_linear$(POSTFIX)

charger_range_fpc_min_rate_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_min_rate$(POSTFIX)

charger_range_fpc_fast_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_fast$(POSTFIX)

charger_range_fpc_no_super_charger_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_super_charger$(POSTFIX)

charger_range_fpc_no_slow_charger_experiments: $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_slow_charger$(POSTFIX)

rank_fpc_linear_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_linear$(POSTFIX)

rank_fpc_min_rate_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_min_rate$(POSTFIX)

rank_fpc_fast_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_fast$(POSTFIX)

rank_fpc_no_slow_charger_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_slow_charger$(POSTFIX)

rank_fpc_no_slow_charger_min_rate_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_slow_charger_min_rate$(POSTFIX)

rank_fpc_no_super_charger_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_$(POTENTIAL)_$(X_EPS)_$(Y_EPS)_no_super_charger$(POSTFIX)


rank_mcc_fastest_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_mcc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)$(POSTFIX)

rank_mcc_lazy_fastest_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_mcc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)_$(SAMPLE_RATE)$(POSTFIX)

rank_fpc_fastest_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)

rank_fpc_none_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_none_$(X_EPS)_$(Y_EPS)$(POSTFIX)

rank_fpc_omega_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX)

rank_fpc_lazy_fastest_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_lazy_fastest_$(X_EPS)_$(Y_EPS)$(POSTFIX)

rank_fpc_lazy_omega_experiments: $(RESULTS_BASE)/rank_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_lazy_omega_$(X_EPS)_$(Y_EPS)$(POSTFIX)



all_dijkstra_experiments: range_dijkstra_experiments

all_mc_experiments: range_mc_experiments

all_mcc_experiments: range_mcc_experiments charger_range_mcc_fastest_experiments rank_mcc_fastest_experiments

all_fp_experiments: range_fp_experiments

all_fpc_experiments: range_fpc_experiments \
	charger_range_fpc_fastest_experiments \
	charger_range_fpc_omega_experiments \
	charger_range_fpc_linear_experiments \
	charger_range_fpc_fastest_experiments \
	charger_range_fpc_fast_experiments \
	rank_fpc_fastest_experiments \
	rank_fpc_omega_experiments \
	rank_fpc_linear_experiments \
	rank_fpc_fastest_experiments \
	rank_fpc_fast_experiments

experiments: all_dijkstra_experiments all_fp_experiments all_mcc_experiments all_mc_experiments all_fpc_experiments

# Emperical verification by comparing the results of different algorithms

verify_fp:
	./test/is_better.py $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_fp_dijkstra_fastest_0.1_1.json $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_mc_dijkstra_0.1_1.json

verify_fpc:
	./test/is_better.py $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_omega_0.1_1.json   $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_fastest_0.1_1.json
	./test/is_better.py $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_fastest_0.1_1.json $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_mcc_dijkstra_fastest_0.1_1.json
	./test/is_better.py $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_fpc_dijkstra_omega_0.1_1.json   $(RESULTS_BASE)/random_in_charger_range_$(CAPACITY)wh_mcc_dijkstra_fastest_0.1_1.json

verify_mcc:
	./test/is_better.py $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_mcc_dijkstra_fastest_0.1_1.json $(RESULTS_BASE)/random_in_range_$(CAPACITY)wh_mc_dijkstra_fastest_0.1_1.json

verify: verify_fp verify_mcc verify_fpc

run: $(BASE_GRAPH) $(CHARGERS)
	$(PREFIX_CMD) $(BUILD_DIR)/routed $(CACHE_BASE) $(CAPACITY)

clean:
	rm -f $(BASE_GRAPH)
	rm -f $(CHARGERS)


