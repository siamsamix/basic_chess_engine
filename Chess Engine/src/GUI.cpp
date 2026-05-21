#include "GUI.h"
#include <iostream>

const int SQUARE_SIZE = 80;

GUI::GUI() : window(sf::VideoMode(8 * SQUARE_SIZE, 8 * SQUARE_SIZE), "C++ Chess Engine"), selectedSquare(-1) {
    loadTextures();
}

void GUI::loadTextures() {
    std::string prefix = "assets/";
    
    auto load = [&](int pieceType, const std::string& name) {
        sf::Texture tex;
        if (!tex.loadFromFile(prefix + name + ".png")) {
            std::cerr << "Failed to load " << name << ".png" << std::endl;
        }
        tex.setSmooth(true);
        pieceTextures[pieceType] = tex;
    };
    
    load(Piece::Pawn | Piece::White, "wp");
    load(Piece::Knight | Piece::White, "wn");
    load(Piece::Bishop | Piece::White, "wb");
    load(Piece::Rook | Piece::White, "wr");
    load(Piece::Queen | Piece::White, "wq");
    load(Piece::King | Piece::White, "wk");
    
    load(Piece::Pawn | Piece::Black, "bp");
    load(Piece::Knight | Piece::Black, "bn");
    load(Piece::Bishop | Piece::Black, "bb");
    load(Piece::Rook | Piece::Black, "br");
    load(Piece::Queen | Piece::Black, "bq");
    load(Piece::King | Piece::Black, "bk");
}

void GUI::run() {
    // Initial draw to show board before engine starts thinking if it's black
    window.clear();
    drawBoard();
    drawPieces();
    window.display();

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            else
                handleInput(event);
        }
        
        window.clear();
        drawBoard();
        drawPieces();
        window.display();
        
        if (!board.whiteToMove && window.isOpen()) { // Let computer play black
            handleComputerMove();
            // Force redraw after computer move
            window.clear();
            drawBoard();
            drawPieces();
            window.display();
        }
    }
}

void GUI::handleComputerMove() {
    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) {
        if (board.isKingInCheck(false)) std::cout << "White wins by checkmate!\n";
        else std::cout << "Stalemate!\n";
        return;
    }
    
    Move best = engine.getBestMove(board, 4); // Depth 4
    if (best.fromSquare != -1) {
        board.makeMove(best);
    }
}

int GUI::getSquareFromCoords(int x, int y) {
    int file = x / SQUARE_SIZE;
    int rank = 7 - (y / SQUARE_SIZE); // SFML y=0 is top, our rank 7 is top
    return rank * 8 + file;
}

void GUI::handleInput(sf::Event event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        int sq = getSquareFromCoords(event.mouseButton.x, event.mouseButton.y);
        
        if (selectedSquare == -1) {
            int piece = board.squares[sq];
            if (piece != Piece::None && (piece & 24) == (board.whiteToMove ? Piece::White : Piece::Black)) {
                selectedSquare = sq;
                std::vector<Move> moves = board.generateLegalMoves();
                validMovesForSelected.clear();
                for (const auto& m : moves) {
                    if (m.fromSquare == sq) validMovesForSelected.push_back(m);
                }
            }
        } else {
            Move chosenMove{-1, -1, 0, false, false, false};
            for (const auto& m : validMovesForSelected) {
                if (m.toSquare == sq) {
                    chosenMove = m;
                    if (chosenMove.promotionResult != 0) {
                        chosenMove.promotionResult = Piece::Queen; // Auto queen for now
                    }
                    break;
                }
            }
            
            if (chosenMove.fromSquare != -1) {
                board.makeMove(chosenMove);
                selectedSquare = -1;
                validMovesForSelected.clear();
            } else {
                int piece = board.squares[sq];
                if (piece != Piece::None && (piece & 24) == (board.whiteToMove ? Piece::White : Piece::Black)) {
                    selectedSquare = sq;
                    std::vector<Move> moves = board.generateLegalMoves();
                    validMovesForSelected.clear();
                    for (const auto& m : moves) {
                        if (m.fromSquare == sq) validMovesForSelected.push_back(m);
                    }
                } else {
                    selectedSquare = -1;
                    validMovesForSelected.clear();
                }
            }
        }
    }
}

void GUI::drawBoard() {
    sf::RectangleShape square(sf::Vector2f(SQUARE_SIZE, SQUARE_SIZE));
    
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            bool isLight = (rank + file) % 2 != 0;
            if (isLight) square.setFillColor(sf::Color(240, 217, 181)); 
            else square.setFillColor(sf::Color(181, 136, 99)); 
            
            int sq = rank * 8 + file;
            if (sq == selectedSquare) {
                square.setFillColor(sf::Color(200, 200, 50, 200)); 
            }
            
            for (const auto& m : validMovesForSelected) {
                if (m.toSquare == sq) {
                    square.setFillColor(sf::Color(50, 200, 50, 150)); 
                }
            }
            
            square.setPosition(file * SQUARE_SIZE, (7 - rank) * SQUARE_SIZE);
            window.draw(square);
        }
    }
}

void GUI::drawPieces() {
    sf::Sprite sprite;
    float scale = (float)SQUARE_SIZE / 150.0f;
    sprite.setScale(scale, scale);
    
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            int piece = board.squares[sq];
            if (piece != Piece::None) {
                if (pieceTextures.find(piece) != pieceTextures.end()) {
                    sprite.setTexture(pieceTextures[piece]);
                    sprite.setPosition(file * SQUARE_SIZE, (7 - rank) * SQUARE_SIZE);
                    window.draw(sprite);
                }
            }
        }
    }
}
