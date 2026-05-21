#pragma once
#include "Board.h"
#include <limits>
#include <vector>

class Engine {
public:
    Engine();
    
    // Returns the best move for the current position
    Move getBestMove(Board& board, int depth);
    
    // Evaluation function
    int evaluate(const Board& board);

private:
    int nodesEvaluated;
    
    // Negamax with alpha-beta pruning
    int negamax(Board& board, int depth, int alpha, int beta, int colorMultiplier);
    
    // Helper to score a move for ordering
    int scoreMove(const Board& board, const Move& move);
    
    // Move ordering
    void orderMoves(const Board& board, std::vector<Move>& moves);
};
