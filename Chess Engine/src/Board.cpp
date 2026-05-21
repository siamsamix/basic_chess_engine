#include "Board.h"
#include <sstream>
#include <cctype>
#include <cmath>

Board::Board() {
    loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::loadFEN(const std::string& fen) {
    for (int i = 0; i < 64; ++i) squares[i] = Piece::None;
    
    std::istringstream iss(fen);
    std::string boardPart, colorPart, castlePart, epPart;
    iss >> boardPart >> colorPart >> castlePart >> epPart;
    
    int rank = 7, file = 0;
    for (char c : boardPart) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += c - '0';
        } else {
            int piece = 0;
            switch (tolower(c)) {
                case 'p': piece = Piece::Pawn; break;
                case 'n': piece = Piece::Knight; break;
                case 'b': piece = Piece::Bishop; break;
                case 'r': piece = Piece::Rook; break;
                case 'q': piece = Piece::Queen; break;
                case 'k': piece = Piece::King; break;
            }
            piece |= (isupper(c) ? Piece::White : Piece::Black);
            squares[rank * 8 + file] = piece;
            file++;
        }
    }
    
    whiteToMove = (colorPart == "w");
    
    whiteCastleKingside = castlePart.find('K') != std::string::npos;
    whiteCastleQueenside = castlePart.find('Q') != std::string::npos;
    blackCastleKingside = castlePart.find('k') != std::string::npos;
    blackCastleQueenside = castlePart.find('q') != std::string::npos;
    
    if (epPart != "-") {
        int f = epPart[0] - 'a';
        int r = epPart[1] - '1';
        enPassantSquare = r * 8 + f;
    } else {
        enPassantSquare = -1;
    }
    
    if (!(iss >> halfMoveClock >> fullMoveNumber)) {
        halfMoveClock = 0;
        fullMoveNumber = 1;
    }
}

std::vector<Move> Board::generateLegalMoves() {
    std::vector<Move> pseudoMoves = generatePseudoLegalMoves();
    std::vector<Move> legalMoves;
    
    for (const Move& move : pseudoMoves) {
        makeMove(move);
        // After making move, it's the other player's turn. We check if the player who just moved is in check.
        if (!isKingInCheck(!whiteToMove)) {
            legalMoves.push_back(move);
        }
        unmakeMove(move);
    }
    return legalMoves;
}

int Board::getKingSquare(bool white) const {
    int target = Piece::King | (white ? Piece::White : Piece::Black);
    for (int i = 0; i < 64; ++i) {
        if (squares[i] == target) return i;
    }
    return -1;
}

bool Board::isKingInCheck(bool white) const {
    int kingSquare = getKingSquare(white);
    if (kingSquare == -1) return false; // Should not happen in a valid game
    return isSquareAttacked(kingSquare, !white);
}

bool Board::isSquareAttacked(int square, bool attackedByWhite) const {
    int attackerColor = attackedByWhite ? Piece::White : Piece::Black;
    int rank = square / 8;
    int file = square % 8;

    // Check pawn attacks
    int pawnDir = attackedByWhite ? -1 : 1; // From the perspective of the attacked square, where would the pawn come from?
    for (int fDir : {-1, 1}) {
        int r = rank + pawnDir;
        int f = file + fDir;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int p = squares[r * 8 + f];
            if (p == (Piece::Pawn | attackerColor)) return true;
        }
    }

    // Check knight attacks
    int knightMoves[8][2] = {{-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}};
    for (auto& m : knightMoves) {
        int r = rank + m[0];
        int f = file + m[1];
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int p = squares[r * 8 + f];
            if (p == (Piece::Knight | attackerColor)) return true;
        }
    }

    // Check sliding attacks (Bishop, Rook, Queen)
    int dirs[8][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {-1,1}, {1,-1}, {1,1}};
    for (int i = 0; i < 8; ++i) {
        bool isDiagonal = i >= 4;
        for (int step = 1; step < 8; ++step) {
            int r = rank + dirs[i][0] * step;
            int f = file + dirs[i][1] * step;
            if (r < 0 || r >= 8 || f < 0 || f >= 8) break;
            
            int p = squares[r * 8 + f];
            if (p != Piece::None) {
                if ((p & 24) == attackerColor) {
                    int type = p & 7;
                    if (type == Piece::Queen) return true;
                    if (isDiagonal && type == Piece::Bishop) return true;
                    if (!isDiagonal && type == Piece::Rook) return true;
                }
                break; // Blocked by piece
            }
        }
    }

    // Check king attacks (adjacent)
    for (int r = -1; r <= 1; ++r) {
        for (int f = -1; f <= 1; ++f) {
            if (r == 0 && f == 0) continue;
            int rr = rank + r;
            int ff = file + f;
            if (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) {
                int p = squares[rr * 8 + ff];
                if (p == (Piece::King | attackerColor)) return true;
            }
        }
    }

    return false;
}

void Board::makeMove(const Move& move) {
    State s;
    s.capturedPiece = squares[move.toSquare];
    s.enPassantSquare = enPassantSquare;
    s.whiteCastleKingside = whiteCastleKingside;
    s.whiteCastleQueenside = whiteCastleQueenside;
    s.blackCastleKingside = blackCastleKingside;
    s.blackCastleQueenside = blackCastleQueenside;
    s.halfMoveClock = halfMoveClock;
    stateHistory.push_back(s);

    int piece = squares[move.fromSquare];
    int pieceType = piece & 7;
    int pieceColor = piece & 24;

    squares[move.toSquare] = piece;
    squares[move.fromSquare] = Piece::None;

    if (move.promotionResult != 0) {
        squares[move.toSquare] = move.promotionResult | pieceColor;
    }

    if (move.isEnPassant) {
        int capturedPawnSquare = move.toSquare + (whiteToMove ? -8 : 8);
        squares[capturedPawnSquare] = Piece::None;
    }

    if (move.isCastling) {
        int rank = move.toSquare / 8;
        if (move.toSquare > move.fromSquare) { // Kingside
            squares[rank * 8 + 5] = squares[rank * 8 + 7];
            squares[rank * 8 + 7] = Piece::None;
        } else { // Queenside
            squares[rank * 8 + 3] = squares[rank * 8 + 0];
            squares[rank * 8 + 0] = Piece::None;
        }
    }

    // Update castling rights
    if (pieceType == Piece::King) {
        if (whiteToMove) {
            whiteCastleKingside = false;
            whiteCastleQueenside = false;
        } else {
            blackCastleKingside = false;
            blackCastleQueenside = false;
        }
    }
    if (pieceType == Piece::Rook) {
        if (move.fromSquare == 0) whiteCastleQueenside = false;
        if (move.fromSquare == 7) whiteCastleKingside = false;
        if (move.fromSquare == 56) blackCastleQueenside = false;
        if (move.fromSquare == 63) blackCastleKingside = false;
    }
    // If a rook is captured, remove castling rights for that side
    if (s.capturedPiece == (Piece::Rook | Piece::White)) {
        if (move.toSquare == 0) whiteCastleQueenside = false;
        if (move.toSquare == 7) whiteCastleKingside = false;
    }
    if (s.capturedPiece == (Piece::Rook | Piece::Black)) {
        if (move.toSquare == 56) blackCastleQueenside = false;
        if (move.toSquare == 63) blackCastleKingside = false;
    }

    // Update en passant square
    enPassantSquare = -1;
    if (pieceType == Piece::Pawn) {
        if (std::abs(move.toSquare - move.fromSquare) == 16) {
            enPassantSquare = move.toSquare + (whiteToMove ? -8 : 8);
        }
    }

    halfMoveClock++;
    if (pieceType == Piece::Pawn || s.capturedPiece != Piece::None) {
        halfMoveClock = 0;
    }

    if (!whiteToMove) {
        fullMoveNumber++;
    }
    whiteToMove = !whiteToMove;
}

void Board::unmakeMove(const Move& move) {
    whiteToMove = !whiteToMove;
    if (!whiteToMove) fullMoveNumber--;

    State s = stateHistory.back();
    stateHistory.pop_back();

    int pieceColor = whiteToMove ? Piece::White : Piece::Black;
    int movedPiece = squares[move.toSquare];

    if (move.promotionResult != 0) {
        squares[move.fromSquare] = Piece::Pawn | pieceColor;
    } else {
        squares[move.fromSquare] = movedPiece;
    }

    squares[move.toSquare] = s.capturedPiece;

    if (move.isEnPassant) {
        int capturedPawnSquare = move.toSquare + (whiteToMove ? -8 : 8);
        squares[capturedPawnSquare] = Piece::Pawn | (whiteToMove ? Piece::Black : Piece::White);
        squares[move.toSquare] = Piece::None;
    }

    if (move.isCastling) {
        int rank = move.toSquare / 8;
        if (move.toSquare > move.fromSquare) { // Kingside
            squares[rank * 8 + 7] = squares[rank * 8 + 5];
            squares[rank * 8 + 5] = Piece::None;
        } else { // Queenside
            squares[rank * 8 + 0] = squares[rank * 8 + 3];
            squares[rank * 8 + 3] = Piece::None;
        }
    }

    enPassantSquare = s.enPassantSquare;
    whiteCastleKingside = s.whiteCastleKingside;
    whiteCastleQueenside = s.whiteCastleQueenside;
    blackCastleKingside = s.blackCastleKingside;
    blackCastleQueenside = s.blackCastleQueenside;
    halfMoveClock = s.halfMoveClock;
}

std::vector<Move> Board::generatePseudoLegalMoves() const {
    std::vector<Move> moves;
    moves.reserve(40);
    int color = whiteToMove ? Piece::White : Piece::Black;
    
    for (int i = 0; i < 64; ++i) {
        int piece = squares[i];
        if (piece != Piece::None && (piece & 24) == color) {
            int type = piece & 7;
            if (type == Piece::Pawn) generatePawnMoves(i, moves);
            else if (type == Piece::Knight) generateKnightMoves(i, moves);
            else if (type == Piece::King) generateKingMoves(i, moves);
            else generateSlidingMoves(i, type, moves);
        }
    }
    return moves;
}

void Board::generatePawnMoves(int square, std::vector<Move>& moves) const {
    int rank = square / 8;
    int file = square % 8;
    int dir = whiteToMove ? 1 : -1;
    int startRank = whiteToMove ? 1 : 6;
    int promoRank = whiteToMove ? 7 : 0;
    
    // Push one square
    int push1 = square + 8 * dir;
    if (squares[push1] == Piece::None) {
        if (rank + dir == promoRank) {
            moves.push_back({square, push1, Piece::Queen, false, false, false});
            moves.push_back({square, push1, Piece::Rook, false, false, false});
            moves.push_back({square, push1, Piece::Bishop, false, false, false});
            moves.push_back({square, push1, Piece::Knight, false, false, false});
        } else {
            moves.push_back({square, push1, 0, false, false, false});
            // Push two squares
            if (rank == startRank) {
                int push2 = square + 16 * dir;
                if (squares[push2] == Piece::None) {
                    moves.push_back({square, push2, 0, false, false, false});
                }
            }
        }
    }
    
    // Captures
    int opponentColor = whiteToMove ? Piece::Black : Piece::White;
    for (int fDir : {-1, 1}) {
        if (file + fDir >= 0 && file + fDir < 8) {
            int capSq = square + 8 * dir + fDir;
            if (capSq >= 0 && capSq < 64) {
                bool isEp = (capSq == enPassantSquare);
                int targetPiece = squares[capSq];
                if ((targetPiece != Piece::None && (targetPiece & 24) == opponentColor) || isEp) {
                    if (rank + dir == promoRank) {
                        moves.push_back({square, capSq, Piece::Queen, true, false, false});
                        moves.push_back({square, capSq, Piece::Rook, true, false, false});
                        moves.push_back({square, capSq, Piece::Bishop, true, false, false});
                        moves.push_back({square, capSq, Piece::Knight, true, false, false});
                    } else {
                        moves.push_back({square, capSq, 0, true, isEp, false});
                    }
                }
            }
        }
    }
}

void Board::generateKnightMoves(int square, std::vector<Move>& moves) const {
    int rank = square / 8;
    int file = square % 8;
    int knightDirs[8][2] = {{-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}};
    int opponentColor = whiteToMove ? Piece::Black : Piece::White;
    
    for (auto& d : knightDirs) {
        int r = rank + d[0];
        int f = file + d[1];
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int target = r * 8 + f;
            int piece = squares[target];
            if (piece == Piece::None) {
                moves.push_back({square, target, 0, false, false, false});
            } else if ((piece & 24) == opponentColor) {
                moves.push_back({square, target, 0, true, false, false});
            }
        }
    }
}

void Board::generateSlidingMoves(int square, int pieceType, std::vector<Move>& moves) const {
    int rank = square / 8;
    int file = square % 8;
    int opponentColor = whiteToMove ? Piece::Black : Piece::White;
    
    int dirs[8][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}, {-1,-1}, {-1,1}, {1,-1}, {1,1}};
    int startIdx = (pieceType == Piece::Bishop) ? 4 : 0;
    int endIdx = (pieceType == Piece::Rook) ? 4 : 8;
    
    for (int i = startIdx; i < endIdx; ++i) {
        for (int step = 1; step < 8; ++step) {
            int r = rank + dirs[i][0] * step;
            int f = file + dirs[i][1] * step;
            if (r < 0 || r >= 8 || f < 0 || f >= 8) break;
            
            int target = r * 8 + f;
            int piece = squares[target];
            if (piece == Piece::None) {
                moves.push_back({square, target, 0, false, false, false});
            } else {
                if ((piece & 24) == opponentColor) {
                    moves.push_back({square, target, 0, true, false, false});
                }
                break;
            }
        }
    }
}

void Board::generateKingMoves(int square, std::vector<Move>& moves) const {
    int rank = square / 8;
    int file = square % 8;
    int opponentColor = whiteToMove ? Piece::Black : Piece::White;
    
    for (int r = -1; r <= 1; ++r) {
        for (int f = -1; f <= 1; ++f) {
            if (r == 0 && f == 0) continue;
            int rr = rank + r;
            int ff = file + f;
            if (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) {
                int target = rr * 8 + ff;
                int piece = squares[target];
                if (piece == Piece::None) {
                    moves.push_back({square, target, 0, false, false, false});
                } else if ((piece & 24) == opponentColor) {
                    moves.push_back({square, target, 0, true, false, false});
                }
            }
        }
    }
    
    // Castling
    if (whiteToMove && square == 4) {
        if (whiteCastleKingside && squares[5] == Piece::None && squares[6] == Piece::None) {
            if (!isSquareAttacked(4, false) && !isSquareAttacked(5, false) && !isSquareAttacked(6, false)) {
                moves.push_back({4, 6, 0, false, false, true});
            }
        }
        if (whiteCastleQueenside && squares[3] == Piece::None && squares[2] == Piece::None && squares[1] == Piece::None) {
            if (!isSquareAttacked(4, false) && !isSquareAttacked(3, false) && !isSquareAttacked(2, false)) {
                moves.push_back({4, 2, 0, false, false, true});
            }
        }
    } else if (!whiteToMove && square == 60) {
        if (blackCastleKingside && squares[61] == Piece::None && squares[62] == Piece::None) {
            if (!isSquareAttacked(60, true) && !isSquareAttacked(61, true) && !isSquareAttacked(62, true)) {
                moves.push_back({60, 62, 0, false, false, true});
            }
        }
        if (blackCastleQueenside && squares[59] == Piece::None && squares[58] == Piece::None && squares[57] == Piece::None) {
            if (!isSquareAttacked(60, true) && !isSquareAttacked(59, true) && !isSquareAttacked(58, true)) {
                moves.push_back({60, 58, 0, false, false, true});
            }
        }
    }
}
