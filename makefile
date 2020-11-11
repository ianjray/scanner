CXX      = clang++
SANITIZE = -g -O0 -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
COVERAGE = -fprofile-instr-generate -fcoverage-mapping
OPTS     = $(SANITIZE) $(COVERAGE) -Weverything -Wno-c++98-compat -Wno-padded -Wno-weak-vtables -Wno-poison-system-directories -DUNITTEST_SCANNER

scanner : scanner.cpp
	$(CXX) -std=c++17 $(OPTS) $^ -o $@

.PHONY : clean
clean :
	rm -rf scanner scanner.dSYM scanner.profdata scanner.profraw

.PHONY : coverage
coverage : scanner.profdata
	xcrun llvm-cov show ./scanner -instr-profile=$<

%.profdata : %.profraw
	xcrun llvm-profdata merge -sparse $< -o $@

%.profraw : %
	LLVM_PROFILE_FILE=$@ ./$<
