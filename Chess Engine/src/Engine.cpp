#include "Engine.h"
#include <algorithm>
#include <iostream>

const int pieceValues[7] = {0, 20000, 100, 320, 330, 500, 900}; // None, King, Pawn, Knight, Bishop, Rook, Queen

// Simple Piece Square Tables for Pawns
const int pawnPst[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

// Knight PST
const int knightPst[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

Engine::Engine() : nodesEvaluated(0) {}

int Engine::evaluate(const Board& board) {
    int score = 0;
    for (int i = 0; i < 64; ++i) {
        int piece = board.squares[i];
        if (piece != Piece::None) {
            int type = piece & 7;
            bool isWhite = (piece & 24) == Piece::White;
            int value = pieceValues[type];
            
            // For White, rank 0 (i=0..7) is rank 8 of the board? No, in our FEN parsing:
            // "rank = 7, file = 0; rank--; " 
            // The FEN starts at rank 8. So rank 7 in our array is the 8th rank.
            // Wait, my FEN parser sets squares[rank * 8 + file]. 
            // For standard starting FEN, rank 7 is black pieces.
            // So rank 7 is 8th rank. Rank 0 is 1st rank.
            // In the PST table (which typically has rank 8 at the top, rank 1 at the bottom):
            // The table's index 0 is rank 8 (a8).
            // So to map board square `i` (where rank = i/8, file = i%8) to PST:
            // We want rank 0 in our board (1st rank) to map to rank 7 in PST (indices 56..63).
            // So pstIndex = (7 - (i / 8)) * 8 + (i % 8);
            // This is for White.
            // For Black, the pawns move the other way, so rank 7 in our board (8th rank) should map to rank 7 in PST.
            // So pstIndex = (i / 8) * 8 + (i % 8) = i;
            
            int pstIndex = isWhite ? ((7 - (i / 8)) * 8 + (i % 8)) : i; 
            
            if (type == Piece::Pawn) value += pawnPst[pstIndex];
            else if (type == Piece::Knight) value += knightPst[pstIndex];
            
            if (isWhite) score += value;
            else score -= value;
        }
    }
    return score;
}

int Engine::scoreMove(const Board& board, const Move& move) {
    int score = 0;
    if (move.isCapture) {
        int targetPiece = board.squares[move.toSquare] & 7;
        int attackerPiece = board.squares[move.fromSquare] & 7;
        score = 10 * pieceValues[targetPiece] - pieceValues[attackerPiece];
    }
    if (move.promotionResult != 0) {
        score += pieceValues[move.promotionResult & 7];
    }
    return score;
}

void Engine::orderMoves(const Board& board, std::vector<Move>& moves) {
    std::vector<std::pair<int, Move>> scoredMoves;
    scoredMoves.reserve(moves.size());
    for (const auto& move : moves) {
        scoredMoves.push_back({scoreMove(board, move), move});
    }
    std::sort(scoredMoves.begin(), scoredMoves.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });
    for (size_t i = 0; i < moves.size(); ++i) {
        moves[i] = scoredMoves[i].second;
    }
}

int Engine::negamax(Board& board, int depth, int alpha, int beta, int colorMultiplier) {
    if (depth == 0) {
        return colorMultiplier * evaluate(board);
    }
    
    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) {
        if (board.isKingInCheck(board.whiteToMove)) {
            return -100000 + (10 - depth); // Checkmate
        }
        return 0; // Stalemate
    }
    
    orderMoves(board, moves);
    
    int maxScore = -1000000;
    for (const Move& move : moves) {
        board.makeMove(move);
        nodesEvaluated++;
        int score = -negamax(board, depth - 1, -beta, -alpha, -colorMultiplier);
        board.unmakeMove(move);
        
        if (score > maxScore) maxScore = score;
        if (maxScore > alpha) alpha = maxScore;
        if (alpha >= beta) break; // Beta cutoff
    }
    return maxScore;
}

Move Engine::getBestMove(Board& board, int depth) {
    nodesEvaluated = 0;
    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) return Move{-1, -1, 0, false, false, false}; // No moves
    
    orderMoves(board, moves);
    
    Move bestMove = moves[0];
    int maxScore = -1000000;
    int alpha = -1000000;
    int beta = 1000000;
    int colorMultiplier = board.whiteToMove ? 1 : -1;
    
    for (const Move& move : moves) {
        board.makeMove(move);
        nodesEvaluated++;
        int score = -negamax(board, depth - 1, -beta, -alpha, -colorMultiplier);
        board.unmakeMove(move);
        
        if (score > maxScore) {
            maxScore = score;
            bestMove = move;
        }
        if (maxScore > alpha) {
            alpha = maxScore;
        }
    }
    
    std::cout << "Engine evaluated " << nodesEvaluated << " nodes. Best score: " << (maxScore * colorMultiplier) << "\n";
    return bestMove;
}
