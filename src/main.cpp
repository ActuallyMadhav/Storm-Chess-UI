#include "../inc/raylib/raylib.h"
#include "../inc/board.h"
#include <iostream>

// window params
int width = 640;
int height = 640;

double boardX = 0.5 * (width - height);
double boardY = 0;
double boardSize = height;

double squareSize = boardSize * 0.9 * 0.125;
double squareX = boardX + boardSize * 0.05;
double squareY = boardY + boardSize * 0.05;

// board color
Color boardBackground = CLITERAL(Color){90, 63, 47, 255};

void drawBoard();
void drawPiece(int x, int y, int size, int piece);
bool flipped = false;
std::pair<int, int> getScreenCoords(int x, int y);

void mouse();
void keyboard();

int selectX = -1;
int selectY = -1;

// promotion state
int promoteX = -1;
int promoteY = -1;
bool pendingPromotion = false;
Move pendingMove;   // promotion move

Board game;

Move moveTracker[1000];
int moveIndex = 0;

int main(){
    InitWindow(width, height, "Chess Engine");
    game.loadSprites();
    SetTargetFPS(60);

    while(!WindowShouldClose()){
        mouse();
        keyboard();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawBoard();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

std::pair<int, int> getScreenCoords(int x, int y){
    int screenX, screenY;
    if(flipped){
        screenX = squareX + squareSize * (7 - x);
        screenY = squareY + squareSize * (7 - y);
    }
    else{
        screenX = squareX + squareSize * x;
        screenY = squareY + squareSize * y;
    }
    return {screenX, screenY};
}

void mouse(){
    Vector2 mousePos = GetMousePosition();
    if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    // promotion gui
    if(pendingPromotion){
        std::pair<int, int> promotionCoords = getScreenCoords(promoteX, promoteY);
        uint8_t chosenPiece = empty;

        if(promoteY == 0){  // white promote so GUI draws downward from rank 0
            float guiTop = promotionCoords.second;
            if(mousePos.x >= promotionCoords.first && mousePos.x <= promotionCoords.first + squareSize){
                int row = (int)floor((mousePos.y - guiTop) / squareSize);
                switch(row){
                    case 0: chosenPiece = wqueen;  break;
                    case 1: chosenPiece = wrook;   break;
                    case 2: chosenPiece = wbishop; break;
                    case 3: chosenPiece = wknight; break;
                }
            }
        }
        else if(promoteY == 7){  // black promot so GUI draws upward from rank 7
            float guiTop = promotionCoords.second - squareSize * 4;
            if(mousePos.x >= promotionCoords.first && mousePos.x <= promotionCoords.first + squareSize){
                int row = (int)floor((mousePos.y - guiTop) / squareSize);
                switch(row){
                    case 0: chosenPiece = bqueen;  break;
                    case 1: chosenPiece = brook;   break;
                    case 2: chosenPiece = bbishop; break;
                    case 3: chosenPiece = bknight; break;
                }
            }
        }

        if(chosenPiece != empty){
            // undo pawn move and redo with promotion set
            game.undo();
            moveIndex--;

            pendingMove.promote = chosenPiece;
            moveTracker[moveIndex] = pendingMove;
            game.move(moveTracker[moveIndex++]);

            pendingPromotion = false;
            promoteX = -1;
            promoteY = -1;
        }
        return; // don't allow normal move while GUI is open
    }

    // normal moves handling
    int clickX = (int)floor((mousePos.x - squareX) / squareSize);
    int clickY = (int)floor((mousePos.y - squareY) / squareSize);

    if(flipped){
        clickX = 7 - clickX;
        clickY = 7 - clickY;
    }

    if(selectX >= 0 && selectX < 8 && selectY >= 0 && selectY < 8
       && game.getPiece(selectX + 8 * selectY) != empty){

        if(clickX >= 0 && clickX < 8 && clickY >= 0 && clickY < 8){
            uint8_t fromSquare = selectX + selectY * 8;
            uint8_t toSquare   = clickX  + clickY  * 8;

            moveTracker[moveIndex].fromSquare = fromSquare;
            moveTracker[moveIndex].toSquare   = toSquare;
            moveTracker[moveIndex].promote    = empty;

            game.move(moveTracker[moveIndex++]);

            // check for pawn reaching back rank
            uint8_t movedPiece = game.getPiece(toSquare);
            if((clickY == 0 && movedPiece == wpawn) ||
               (clickY == 7 && movedPiece == bpawn)){
                promoteX = clickX;
                promoteY = clickY;
                pendingPromotion = true;
                pendingMove = moveTracker[moveIndex - 1];
            }
            else{
                promoteX = -1;
                promoteY = -1;
            }
        }

        selectX = -1;
        selectY = -1;
    }
    else{
        selectX = clickX;
        selectY = clickY;
    }
}

void keyboard(){
    if(IsKeyPressed(KEY_ESCAPE)){
        CloseWindow();
    }
    if(IsKeyPressed(KEY_SPACE)){
        flipped = !flipped;
    }
    if(IsKeyPressed(KEY_BACKSPACE)){
        // don't allow undo while promotion is pending
        if(!pendingPromotion){
            game.undo();
            if(moveIndex > 0) moveIndex--;
        }
    }
}

void drawBoard(){
    DrawRectangle(boardX, boardY, boardX + boardSize, boardY + boardSize, boardBackground);

    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            std::pair<int, int> screenCoords = getScreenCoords(i, j);
            Color squareColor = ((i + j) % 2) ? BROWN : LIGHTGRAY;
            DrawRectangle(screenCoords.first, screenCoords.second, squareSize, squareSize, squareColor);
        }
    }

    // highlight selected square
    if(selectX >= 0 && selectX < 8 && selectY >= 0 && selectY < 8){
        std::pair<int, int> highlightCoords = getScreenCoords(selectX, selectY);
        DrawRectangle(highlightCoords.first, highlightCoords.second, squareSize, squareSize, GREEN);
    }

    // show legal moves
    if(selectX >= 0 && selectX < 8 && selectY >= 0 && selectY < 8){
        for(int x = 0; x < 8; x++){
            for(int y = 0; y < 8; y++){
                Move move;
                move.fromSquare = selectX + 8 * selectY;
                move.toSquare = x + 8 * y;
                move.promote = empty;

                Move promotion = move;
                if(game.turn() == white){
                    promotion.promote = wqueen;
                }
                else{
                    promotion.promote = bqueen;
                }

                if(game.isLegal(move) || game.isLegal(promotion)){
                    
                }
            }
        }
    }

    // show previous move arrow
    if(moveIndex > 0){
        Vector2 fromCoords = {
            (float)(squareX + squareSize * (moveTracker[moveIndex-1].fromSquare % 8 + 0.5)),
            (float)(squareY + squareSize * (moveTracker[moveIndex-1].fromSquare / 8 + 0.5)),
        };
        Vector2 toCoords = {
            (float)(squareX + squareSize * (moveTracker[moveIndex-1].toSquare % 8 + 0.5)),
            (float)(squareY + squareSize * (moveTracker[moveIndex-1].toSquare / 8 + 0.5)),
        };
        DrawLineEx(fromCoords, toCoords, 5.0f, GREEN);
    }

    // draw pieces
    for(int x = 0; x < 8; x++){
        for(int y = 0; y < 8; y++){
            std::pair<int, int> screenCoords = getScreenCoords(x, y);
            uint8_t square = y * 8 + x;
            drawPiece(screenCoords.first + 0.5 * squareSize, screenCoords.second + 0.5 * squareSize, squareSize, game.getPiece(square));
        }
    }

    // draw promotion GUI
    if(pendingPromotion){
        std::pair<int, int> promotionCoords = getScreenCoords(promoteX, promoteY);

        if(promoteY == 0){
            // rank 0 is top of screen so draw downward
            DrawRectangle(promotionCoords.first, promotionCoords.second, squareSize, squareSize*4, WHITE);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second + squareSize*0.5, squareSize, wqueen);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second + squareSize*1.5, squareSize, wrook);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second + squareSize*2.5, squareSize, wbishop);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second + squareSize*3.5, squareSize, wknight);
        }
        if(promoteY == 7){
            // rank 7 is bottom of screen so draw upward
            DrawRectangle(promotionCoords.first, promotionCoords.second - squareSize*4, squareSize, squareSize*4, WHITE);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second - squareSize*3.5, squareSize, bqueen);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second - squareSize*2.5, squareSize, brook);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second - squareSize*1.5, squareSize, bbishop);
            drawPiece(promotionCoords.first + squareSize*0.5, promotionCoords.second - squareSize*0.5, squareSize, bknight);
        }
    }
}

void drawPiece(int x, int y, int size, int piece){
    if(piece != empty){
        Texture2D sprite = game.pieceSprites[piece];
        float scaleX = (float)squareSize / sprite.width;
        float scaleY = (float)squareSize / sprite.height;
        float scale = fmin(scaleX, scaleY);
        Vector2 pos = {
            x - (sprite.width * scale) / 2,
            y - (sprite.height * scale) / 2
        };
        DrawTextureEx(sprite, pos, 0.0f, scale, WHITE);
    }
}