#pragma once
#include <vector>
#include <string>

namespace Piece {
    const int None = 0;
    const int King = 1;
    const int Pawn = 2;
    const int Knight = 3;
    const int Bishop = 4;
    const int Rook = 5;
    const int Queen = 6;
    
    const int White = 8;
    const int Black = 16;
}

struct Move {
    int fromSquare;
    int toSquare;
    int promotionResult; // e.g. Piece::Queen (0 if not promotion)
    bool isCapture;
    bool isEnPassant;
    bool isCastling;
    
    bool operator==(const Move& other) const {
        return fromSquare == other.fromSquare && toSquare == other.toSquare && promotionResult == other.promotionResult;
    }
};

class Board {
public:
    int squares[64];
    bool whiteToMove;
    int enPassantSquare; // -1 if none
    
    // Castling rights
    bool whiteCastleKingside;
    bool whiteCastleQueenside;
    bool blackCastleKingside;
    bool blackCastleQueenside;
    
    int halfMoveClock;
    int fullMoveNumber;

    Board();
    
    void loadFEN(const std::string& fen);
    
    std::vector<Move> generatePseudoLegalMoves() const;
    std::vector<Move> generateLegalMoves(); 
    
    void makeMove(const Move& move);
    void unmakeMove(const Move& move);
    
    bool isKingInCheck(bool white) const;
    bool isSquareAttacked(int square, bool attackedByWhite) const;
    
    int getKingSquare(bool white) const;

private:
    struct State {
        int capturedPiece;
        int enPassantSquare;
        bool whiteCastleKingside;
        bool whiteCastleQueenside;
        bool blackCastleKingside;
        bool blackCastleQueenside;
        int halfMoveClock;
    };
    std::vector<State> stateHistory;
    
    void generatePawnMoves(int square, std::vector<Move>& moves) const;
    void generateKnightMoves(int square, std::vector<Move>& moves) const;
    void generateSlidingMoves(int square, int pieceType, std::vector<Move>& moves) const;
    void generateKingMoves(int square, std::vector<Move>& moves) const;
};
