SHELL := /usr/bin/bash
.SHELLFLAGS := -o pipefail -c

MINGW_ROOT   ?= C:/Qt/Tools/mingw1310_64/bin
C_COMPILER   ?= $(MINGW_ROOT)/gcc.exe
CXX_COMPILER ?= $(MINGW_ROOT)/g++.exe

BUILD_DIR      ?= build
BUILD_TYPE     ?= Release
INSTALL_PREFIX ?= $(abspath $(BUILD_DIR)/_install)

GEN         ?= MinGW Makefiles
CMAKE_FLAGS ?= -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64"

QT_PREFIX   ?= C:/Qt/6.9.1/mingw_64
QT_BIN      ?= $(QT_PREFIX)/bin
WINDEPLOYQT := $(QT_BIN)/windeployqt.exe

APP_EXE     = $(BUILD_DIR)/3DViewer.exe

CLANG_FORMAT ?= C:/Program Files/LLVM/bin/clang-format.exe
STYLE        ?= Google

.PHONY: all install uninstall clean dvi dist tests format

all:
	@cmake -S . -B "$(BUILD_DIR)" -G "$(GEN)" -DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" $(CMAKE_FLAGS) -DCMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)" -DCMAKE_C_COMPILER="$(C_COMPILER)" -DCMAKE_CXX_COMPILER="$(CXX_COMPILER)"
	@cmake --build "$(BUILD_DIR)" --config "$(BUILD_TYPE)"
	@if [ ! -f "$(WINDEPLOYQT)" ]; then \
	  echo "ERROR: windeployqt not found at: $(WINDEPLOYQT)"; \
	  echo "Set QT_PREFIX to your Qt MinGW path, e.g. C:/Qt/6.9.1/mingw_64"; \
	  exit 1; \
	fi
	@"$(WINDEPLOYQT)" --release --compiler-runtime --dir "$(BUILD_DIR)" "$(APP_EXE)"

tests: all
	@ctest --test-dir "$(BUILD_DIR)" -C "$(BUILD_TYPE)" --output-on-failure

install: all
	@cmake --install "$(BUILD_DIR)" --config "$(BUILD_TYPE)" --prefix "$(INSTALL_PREFIX)"

uninstall:
	@manifest="$(BUILD_DIR)/install_manifest.txt"; \
	if [ -f "$$manifest" ]; then \
	  echo "Uninstalling files listed in $$manifest"; \
	  while IFS= read -r f || [ -n "$$f" ]; do \
	    if [ -n "$$f" ]; then \
	      echo rm -vf "$$f"; \
	      rm -vf "$$f"; \
	    fi; \
	  done < "$$manifest"; \
	else \
	  echo "install_manifest.txt not found. Run 'make install' first."; exit 1; \
	fi

clean:
	@echo "Removing build directory: $(BUILD_DIR)"
	@cmake -E remove_directory "$(BUILD_DIR)"
	@echo "Removing docs directory"
	@cmake -E remove_directory "docs"
	@echo "Removing zip files"
	@cmake -E remove -f *.zip

dvi:
	@if command -v doxygen >/dev/null 2>&1; then \
	  if [ ! -f "Doxyfile" ]; then \
	    echo "Generating minimal Doxyfile..."; \
	    { \
	      echo "PROJECT_NAME = 3DViewer"; \
	      echo "OUTPUT_DIRECTORY = docs"; \
	      echo "GENERATE_HTML = YES"; \
	      echo "GENERATE_LATEX = NO"; \
	      echo "RECURSIVE = YES"; \
	      echo "INPUT = src tests"; \
	      echo "FILE_PATTERNS = *.h *.hpp *.cpp"; \
	      echo "QUIET = YES"; \
	    } > "Doxyfile"; \
	  fi; \
	  doxygen "Doxyfile"; \
	else \
	  echo "Doxygen not found. Install doxygen or add Doxyfile."; exit 1; \
	fi

dist:
	@if ! command -v git >/dev/null 2>&1; then \
	  echo "git not found. Install git to use dist."; exit 1; \
	fi
	@NAME="3DViewer"; \
	ZIP="$$NAME.zip"; \
	echo "Creating $$ZIP (sources only) ..."; \
	git archive --format=zip --output "$$ZIP" --prefix="$$NAME/" HEAD; \
	echo "=== Checking archive contents ==="; \
	TMPDIR="$(BUILD_DIR)/_tmp_dist"; \
	cmake -E remove_directory "$$TMPDIR"; \
	cmake -E make_directory "$$TMPDIR"; \
	unzip -q "$$ZIP" -d "$$TMPDIR"; \
	ROOT="$$TMPDIR/$$NAME"; \
	echo "--- First 50 files ---"; \
	find "$$ROOT" -type f | head -50; \
	echo "--- Total files ---"; \
	find "$$ROOT" -type f | wc -l; \
	if find "$$ROOT" -type d \( -name build -o -name docs \) | grep -q .; then \
	  echo "ERROR: build/ or docs/ directory found in archive!"; exit 1; \
	fi; \
	for d in src tests; do \
	  if [ ! -d "$$ROOT/$$d" ]; then echo "ERROR: required dir $$d not found!"; exit 1; fi; \
	done; \
	if [ ! -f "$$ROOT/CMakeLists.txt" ]; then echo "ERROR: CMakeLists.txt not found!"; exit 1; fi; \
	if ! ([ -f "$$ROOT/README.md" ] || [ -f "$$ROOT/README_RUS.md" ] || [ -f "$$ROOT/README_DEV.md" ]); then \
	  echo "ERROR: README file not found (README.md or README_RUS.md or README_DEV.md)!"; exit 1; \
	fi; \
	echo "Archive $$ZIP passed all checks."

format:
	@if [ ! -x "$(CLANG_FORMAT)" ] && ! command -v clang-format >/dev/null 2>&1; then \
	  echo "ERROR: clang-format not found. Set CLANG_FORMAT or add LLVM/bin to PATH."; exit 1; \
	fi; \
	CF="$(CLANG_FORMAT)"; \
	[ -x "$$CF" ] || CF="$$(command -v clang-format)"; \
	echo "Using $$CF"; \
	find src tests -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -print0 | \
	xargs -0 -I {} "$$CF" -style="$(STYLE)" -i "{}"
