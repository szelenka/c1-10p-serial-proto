NANOPB_REPO=https://github.com/nanopb/nanopb.git
NANOPB_DIR=build/nanopb
VENV_DIR=build/venv
PROTO_SRC=c110p_serial.proto
PROTO_OUT=lib/C110PSerial

.PHONY: all nanopb venv deps gen clean

all: gen

venv:
	@if [ ! -d "$(VENV_DIR)" ]; then \
		python3 -m venv $(VENV_DIR); \
	fi

deps: venv
	@source $(VENV_DIR)/bin/activate; \
	$(VENV_DIR)/bin/pip install --upgrade pip; \
	$(VENV_DIR)/bin/pip install \
		protobuf \
		grpcio-tools \
		platformio \
		pytest \
		pytest-cov

nanopb:
	@if [ -d "$(NANOPB_DIR)" ]; then \
		cd $(NANOPB_DIR) && git pull; \
	else \
		git clone --depth 1 $(NANOPB_REPO) $(NANOPB_DIR); \
	fi

gen-cpp: nanopb deps
	mkdir -p $(PROTO_OUT)
	source $(VENV_DIR)/bin/activate; \
	$(VENV_DIR)/bin/python -m grpc_tools.protoc \
		--proto_path=. \
		--proto_path=$(NANOPB_DIR)/generator/proto \
		--plugin=protoc-gen-nanopb=$(NANOPB_DIR)/generator/protoc-gen-nanopb \
		--nanopb_out=$(PROTO_OUT) \
		$(PROTO_SRC)

gen-py: deps nanopb
	mkdir -p python/$(PROTO_OUT)
	cp $(PROTO_SRC) python/
	sed -i '' '/import "nanopb.proto"/d' python/$(PROTO_SRC)
	sed -i '' 's/\s*\[(nanopb).max_size = [^]]*\]//g' python/$(PROTO_SRC)

gen: gen-cpp gen-py
	@echo "Generated files in $(PROTO_OUT)"

test-cpp:
	pio test \
		-e native \
		-vvv 

test-py:
	@source $(VENV_DIR)/bin/activate; \
	PYTHONPATH=python/lib pytest \
		--cov=python/lib \
		--cov-report=term-missing \
		--log-cli-level=DEBUG \
		-s python/test

test: test-cpp test-py
	@echo "Ran C++ and Python tests"

clean:
	rm -rf build
	pio run --target clean
	@pio run || true
	@echo "Hotfix for ArduinoFake"; \
	sed -i '' '7972s/template //' .pio/libdeps/native/ArduinoFake/src/fakeit.hpp;

