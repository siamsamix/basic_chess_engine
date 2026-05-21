#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include "Board.h"
#include "Engine.h"

class GUI {
public:
    GUI();
    void run();

private:
    sf::RenderWindow window;
    Board board;
    Engine engine;
    
    std::map<int, sf::Texture> pieceTextures;
    
    int selectedSquare;
    std::vector<Move> validMovesForSelected;
    
    void loadTextures();
    void drawBoard();
    void drawPieces();
    void handleInput(sf::Event event);
    
    int getSquareFromCoords(int x, int y);
    void handleComputerMove();
};
