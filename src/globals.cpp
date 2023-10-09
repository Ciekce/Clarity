#include "globals.h"
#include "bmi2.h"

int getType(int value) {
    return value & 7;
}
int getColor(int value) {
    return value >> 3;
}
uint64_t fileMask = 0b100000001000000010000000100000001000000010000000100000001;
uint64_t rankMask = 0b11111111;
uint64_t getFileMask(int file) {
    return fileMask << file;
}
uint64_t getRankMask(int rank) {
    return rankMask << (8 * rank);
}
// finds the least significant bit in the uint64_t, gets rid of it, and returns its index
int popLSB(uint64_t &bitboard) {
    int lsb = std::countr_zero(bitboard);
    bitboard &= bitboard - 1;
    return lsb;
}

std::array<std::array<uint8_t, 218>, 50> reductions;

void calculateReductions() {
    for(int i = 0; i < 50; i++) {
        for(int j = 0; j < 218; j++) {
            reductions[i][j] = uint8_t(0.77 + log(i) * log(j) * 0.42);
        }
    }
}

void initialize() {
    generateLookups();
    initializeZobrist();
    calculateReductions();
}

std::vector<std::string> split(const std::string string, const char seperator) {
    std::stringstream stream(string);
    std::string segment;
    std::vector<std::string> list;

    while (std::getline(stream, segment, seperator)) {
        list.push_back(segment);
    }

    return list;
}

int flipIndex(int index) {
    return index ^ 56;
}
