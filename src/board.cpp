#include "../inc/board.h"
#include <cmath>

uint8_t Board::getPiece(uint8_t square){
    uint64_t mask = uint64_t(1) << square;
    for(int i = 0; i < 12; i++){
        if(stateTracker[trackerIndex].bitboards[i] & mask){
            return i;
        }
    }
    return empty;
}

void Board::setPiece(uint8_t square, uint8_t piece){
    uint64_t mask = uint64_t(1) << square;
    for(int i = 0; i < 12; i++){
        if(i == piece){
            stateTracker[trackerIndex].bitboards[i] |= mask;   // set bit
        }
        else{
            stateTracker[trackerIndex].bitboards[i] &= ~mask;  // clear bit
        }
    }
}

void Board::loadSprites(){
    pieceSprites[wpawn] = LoadTexture("../sprites/white/whitePawn.png");
    SetTextureFilter(pieceSprites[wpawn], TEXTURE_FILTER_POINT);
    
    pieceSprites[wknight] = LoadTexture("../sprites/white/whiteKnight.png");
    SetTextureFilter(pieceSprites[wknight], TEXTURE_FILTER_POINT);    
    
    pieceSprites[wbishop] = LoadTexture("../sprites/white/whiteBishop.png");
    SetTextureFilter(pieceSprites[wbishop], TEXTURE_FILTER_POINT);
    
    pieceSprites[wrook] = LoadTexture("../sprites/white/whiteRook.png");
    SetTextureFilter(pieceSprites[wrook], TEXTURE_FILTER_POINT);
    
    pieceSprites[wqueen] = LoadTexture("../sprites/white/whiteQueen.png");
    SetTextureFilter(pieceSprites[wqueen], TEXTURE_FILTER_POINT);
    
    pieceSprites[wking] = LoadTexture("../sprites/white/whiteKing.png");
    SetTextureFilter(pieceSprites[wking], TEXTURE_FILTER_POINT);    
    
    pieceSprites[bpawn] = LoadTexture("../sprites/black/blackPawn.png");
    SetTextureFilter(pieceSprites[bpawn], TEXTURE_FILTER_POINT);    
    
    pieceSprites[bknight] = LoadTexture("../sprites/black/blackKnight.png");
    SetTextureFilter(pieceSprites[bknight], TEXTURE_FILTER_POINT);
    
    pieceSprites[bbishop] = LoadTexture("../sprites/black/blackBishop.png");
    SetTextureFilter(pieceSprites[bbishop], TEXTURE_FILTER_POINT);
    
    pieceSprites[brook] = LoadTexture("../sprites/black/blackRook.png");
    SetTextureFilter(pieceSprites[brook], TEXTURE_FILTER_POINT);
    
    pieceSprites[bqueen] = LoadTexture("../sprites/black/blackQueen.png");
    SetTextureFilter(pieceSprites[bqueen], TEXTURE_FILTER_POINT);
    
    pieceSprites[bking] = LoadTexture("../sprites/black/blackKing.png");
    SetTextureFilter(pieceSprites[bking], TEXTURE_FILTER_POINT);
}

void Board::move(const Move& move){
    // add new state to state tracker
    stateTracker[trackerIndex + 1] = stateTracker[trackerIndex];
    trackerIndex++;

    // to and from bitboards
    uint64_t fromBitboard = uint64_t(1) << move.fromSquare;
    uint64_t toBitboard = uint64_t(1) << move.toSquare;
    uint64_t moveBitboard = fromBitboard | toBitboard;

    // to move a piece
    uint8_t movedPiece = empty;
    int i, limit;
    if(stateTracker[trackerIndex].turn == white){
        i = 0;
        limit = 6;
    }
    else{
        i = 6;
        limit = 12;
    }

    for(; i < limit; i++){
        if(stateTracker[trackerIndex].bitboards[i] & fromBitboard){
            stateTracker[trackerIndex].bitboards[i] &= ~fromBitboard;
            stateTracker[trackerIndex].bitboards[i] |= toBitboard;
            movedPiece = i;
            break;
        }
    }

    // to capture piece
    if(stateTracker[trackerIndex].turn == white){
        i = 6;
        limit = 11;
    }
    else{
        i = 0;
        limit = 5;
    }
    
    for(; i < limit; i++){
        if(stateTracker[trackerIndex].bitboards[i] & toBitboard){
            stateTracker[trackerIndex].bitboards[i] &= ~toBitboard;
            break;
        }
    }

    // en passant implementation
    if(toBitboard == stateTracker[trackerIndex].enPassantTarget){
        if(movedPiece == wpawn){
            stateTracker[trackerIndex].bitboards[bpawn] ^= stateTracker[trackerIndex].enPassantTarget << 8;
        }
        else if(movedPiece == bpawn){
            stateTracker[trackerIndex].bitboards[wpawn] ^= stateTracker[trackerIndex].enPassantTarget >> 8;
        }
    }

    // set en passant target
    if((movedPiece == wpawn || movedPiece == bpawn) && abs(move.toSquare - move.fromSquare) == 16){
        stateTracker[trackerIndex].enPassantTarget = uint64_t(1) << int((move.fromSquare + move.toSquare) * 0.5);
    }
    else{
        stateTracker[trackerIndex].enPassantTarget = 0;
    }

    // castling
    if(movedPiece == wking){
        if(moveBitboard == 0x5000000000000000){
            stateTracker[trackerIndex].bitboards[wrook] ^= 0xa000000000000000;
        }
        else if(moveBitboard == 0x1400000000000000){
            stateTracker[trackerIndex].bitboards[wrook] ^= 0x0900000000000000;
        }
    }
    else if(movedPiece == bking){
        if(moveBitboard == 0x0000000000000050){
            stateTracker[trackerIndex].bitboards[brook] ^= 0x00000000000000a0;
        }
        else if(moveBitboard == 0x0000000000000014){
            stateTracker[trackerIndex].bitboards[brook] ^= 0x0000000000000009;
        }
    }

    // update castling rights
    if(moveBitboard & 0x5000000000000000) stateTracker[trackerIndex].whiteKingCastle = false;    
    if(moveBitboard & 0x1100000000000000) stateTracker[trackerIndex].whiteQueenCastle = false;
    if(moveBitboard & 0x0000000000000050) stateTracker[trackerIndex].blackKingCastle = false;
    if(moveBitboard & 0x0000000000000011) stateTracker[trackerIndex].blackQueenCastle = false;

    // promoting pawns
    if(move.promote != empty){
        if(stateTracker[trackerIndex].turn == white){
            stateTracker[trackerIndex].bitboards[wpawn] ^= toBitboard;
        }
        else{
            stateTracker[trackerIndex].bitboards[bpawn] ^= toBitboard;
        }
        
        stateTracker[trackerIndex].bitboards[move.promote] ^= toBitboard;
    }

    // change turn
    if(stateTracker[trackerIndex].turn == white){
        stateTracker[trackerIndex].turn = black;
    }
    else{
        stateTracker[trackerIndex].turn = white;
    }
}

void Board::undo(){
    if(trackerIndex){
        trackerIndex--;
    }
}

void Board::psuedoLegalMoves(Move* move, int& numMoves){

}

bool Board::isLegal(const Move& move){
    Move moves[256];
    int numMoves;
    psuedoLegalMoves(moves, numMoves);

    for(int i = 0; i < numMoves; i++){
        if(move == moves[i]) return true;
    }

    return false;
}

uint8_t Board::turn(){
    return stateTracker[trackerIndex].turn;
}
Board::Board(){
    // allocating memory for entire game
    stateTracker = new  boardState[1000];
    trackerIndex = 0;
    // starting positions
    stateTracker[0].bitboards[bpawn] = 0x000000000000ff00;
    stateTracker[0].bitboards[bknight] = 0x0000000000000042;
    stateTracker[0].bitboards[bbishop] = 0x0000000000000024;
    stateTracker[0].bitboards[brook] = 0x0000000000000081;
    stateTracker[0].bitboards[bqueen] = 0x0000000000000008;
    stateTracker[0].bitboards[bking] = 0x0000000000000010;
    stateTracker[0].bitboards[wpawn] = 0x00ff000000000000;
    stateTracker[0].bitboards[wknight] = 0x4200000000000000;
    stateTracker[0].bitboards[wbishop] = 0x2400000000000000;
    stateTracker[0].bitboards[wrook] = 0x8100000000000000;
    stateTracker[0].bitboards[wqueen] = 0x0800000000000000;
    stateTracker[0].bitboards[wking] = 0x1000000000000000;

    // castling rights
    stateTracker[0].whiteKingCastle = true;
    stateTracker[0].whiteQueenCastle = true;
    stateTracker[0].blackKingCastle = true;
    stateTracker[0].blackQueenCastle = true;

    // en passant bitboard
    stateTracker[0].enPassantTarget = 0;

    // white moves first
    stateTracker[0].turn = white;
}

Board::~Board(){
    for(int i = 0; i < 12; i++){
        UnloadTexture(pieceSprites[i]);
    }

    delete[] stateTracker;
}