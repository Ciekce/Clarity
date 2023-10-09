#pragma once

#include "globals.h"
#include "nnue.h"

// structs and stuff
struct Board;

struct Move {
public:
    int getStartSquare();
    int getEndSquare();
    int getFlag();
    int getValue();
    Move(int startSquare, int endSquare, int flag);
    Move();
    Move(std::string longAlgebraic, Board &board);
private:
    uint16_t value;
};

struct BoardState {
    std::array<uint64_t, 2> coloredBitboards;
    std::array<uint64_t, 6> pieceBitboards;
    uint8_t enPassantIndex;
    std::array<uint8_t, 2> kingSquares;
    uint8_t fiftyMoveCounter;
    uint8_t hundredPlyCounter;
    uint8_t castlingRights;
    uint64_t zobristHash;
    bool isRepeated;
};

struct Board {
public:
    uint64_t zobristHash;
    bool isRepeated;
    Board(std::string fen);
    bool makeMove(Move move);
    void undoMove();
    int getMoves(std::array<Move, 256> &moves);
    int getMovesQSearch(std::array<Move, 256> &moves);
    std::string getFenString();
    bool isInCheck();
    bool squareIsUnderAttack(int square);
    void toString();
    uint8_t getColorToMove();
    uint64_t getCurrentPlayerBitboard() const;
    uint64_t getOccupiedBitboard() const;
    uint64_t getColoredPieceBitboard(int color, int piece) const;
    int pieceAtIndex(int index) const;
    int colorAtIndex(int index) const;
    void changeColor();
    void undoChangeColor();
    int getEvaluation();
    int getCastlingRights();
    int getEnPassantIndex();
    uint64_t fullZobristRegen();
    bool isRepeatedPosition();
private:
    std::array<uint64_t, 2> coloredBitboards;
    std::array<uint64_t, 6> pieceBitboards;
    uint8_t enPassantIndex;
    std::array<uint8_t, 2> kingSquares;
    int plyCount;
    uint8_t hundredPlyCounter;
    uint8_t fiftyMoveCounter;
    uint8_t castlingRights;
    uint8_t colorToMove;
    std::vector<BoardState> stateHistory;
    std::vector<uint64_t> zobristHistory;
    stormphrax::eval::NnueState nnueState;
    template <bool UpdateNnue> void addPiece(int square, int type);
    template <bool UpdateNnue> void removePiece(int square, int type);
    template <bool UpdateNnue> void movePiece(int square1, int type1, int square2, int type2);
    void loadBoardState(BoardState state);
    BoardState generateBoardState();
    void resetNnue();
};

[[nodiscard]]std::string toLongAlgebraic(Move move);

void sortMoves(std::array<int, 256> &values, std::array<Move, 256> &moves, int numMoves);
void incrementalSort(std::array<int, 256> &values, std::array<Move, 256> &moves, int numMoves, int i);
