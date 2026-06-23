#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "API.h"

#define MAZE_LENGTH 16
#define MAZE_WIDTH 16
#define CENTER_X 7
#define CENTER_Y 7

void log(char* text) {
    fprintf(stderr, "%s\n", text);
    fflush(stderr);
}

#define WALL_N (1 << 0)
#define WALL_E (1 << 1)
#define WALL_S (1 << 2)
#define WALL_W (1 << 3)



typedef struct {
    // x and y location of cell, any value from 0 - 15
    uint8_t R : 4, C : 4;

    // uses bit padding to store all direction in 1st 4 bits
    uint8_t walls;

    // distance to center of maze
    uint8_t d;
} maze_cell_t;

void updateMMSFromCell(maze_cell_t* cell);

// maze flood function

// maze[y][x]
// maze[row][column], starting from bottom left of maze which is maze[0][0]
maze_cell_t maze[MAZE_LENGTH][MAZE_WIDTH];
maze_cell_t* queue[MAZE_LENGTH * MAZE_WIDTH];
uint16_t front = 0, back = 0;

void enqueue(maze_cell_t* cell){
    queue[back++] = cell;
}

maze_cell_t* dequeue(){
    return queue[front++];
}

bool empty(){
    return front == back;
}
void clear_maze(maze_cell_t maze[MAZE_LENGTH][MAZE_WIDTH]){
    maze_cell_t* cell;
    for(int r = 0; r < MAZE_LENGTH; r++){
        for(int c = 0; c < MAZE_WIDTH; c++){
            cell = &maze[r][c];
            // if(!(r == CENTER_Y && c == CENTER_X)){
            //     maze[r][c].d = 255;
            // }
            if(cell->d != 0) cell->d = 255;
        }
    }
}

// "floods" the maze with d values
// column # needs to be known at compile time to pass maze into function
void flood_maze(maze_cell_t maze[MAZE_LENGTH][MAZE_WIDTH]){
    front = 0;
    back = 0;
    clear_maze(maze);
    enqueue(&maze[CENTER_Y][CENTER_X]);

        while(!empty()){
            maze_cell_t* cell = dequeue();
            updateMMSFromCell(cell);

            if(cell->R + 1 <= MAZE_LENGTH - 1){
                maze_cell_t* neighbor = &maze[cell->R + 1][cell->C];
                if(!(neighbor->walls & WALL_S) && !(cell->walls & WALL_N)) {
                    if(neighbor->d > cell->d + 1){
                        neighbor->d = cell->d + 1;
                        enqueue(neighbor);
                        updateMMSFromCell(neighbor);
                    }
                }
            }
            if(cell->R - 1 >= 0){
                maze_cell_t* neighbor = &maze[cell->R - 1][cell->C];
                if(!(neighbor->walls & WALL_N) && !(cell->walls & WALL_S)) {
                    if(neighbor->d > cell->d + 1){
                        neighbor->d = cell->d + 1;
                        enqueue(neighbor);
                        updateMMSFromCell(neighbor);
                    }
                }
            }
            if(cell->C + 1 <= MAZE_WIDTH - 1){
            maze_cell_t* neighbor = &maze[cell->R][cell->C + 1];
                if(!(neighbor->walls & WALL_W) && !(cell->walls & WALL_E)) {
                    if(neighbor->d > cell->d + 1){
                        neighbor->d = cell->d + 1;
                        enqueue(neighbor);
                        updateMMSFromCell(neighbor);
                    }
                }
            }
            if(cell->C - 1 >= 0){
                maze_cell_t* neighbor = &maze[cell->R][cell->C - 1];
                if(!(neighbor->walls & WALL_E) && !(cell->walls & WALL_W)) {
                    if(neighbor->d > cell->d + 1){
                        neighbor->d = cell->d + 1;
                        enqueue(neighbor);
                        updateMMSFromCell(neighbor);
                    }
                }
            }

        }

}

// prints cell walls to mms
void updateMMSFromCell(maze_cell_t* cell){
        if(cell->walls & WALL_N){
            API_setWall(cell->C, cell->R, 'n');
        }
        if(cell->walls & WALL_E){
            API_setWall(cell->C, cell->R, 'e');
        }
        if(cell->walls & WALL_S){
            API_setWall(cell->C, cell->R, 's');
        }
        if(cell->walls & WALL_W){
            API_setWall(cell->C, cell->R, 'w');
        }
        char buffer[16] = "";
        sprintf(buffer, "%d", cell->d);
        API_setText(cell->C, cell->R, buffer);
}



int main(int argc, char* argv[]) {
    log("Running...");
    API_setColor(0, 0, 'G');
    API_setText(0, 0, "abc");
    maze_cell_t blank_cell;
    blank_cell.d = 255; // "blank" is 255, these values will go away after great flood
    blank_cell.walls = 0; // sets all bits in walls to 0

    // initalize maze with blank cells and assign location
    for(int i = 0; i < MAZE_LENGTH; i++){
        for(int j = 0; j < MAZE_WIDTH; j++){
            maze[i][j] = blank_cell;
            maze[i][j].R = i;
            maze[i][j].C = j;
        }
    }

    // set the center of the maze
    // divide by 2 subtract 1 for center, if % 2 = 0, then also set top/right to 0
    // TODO change to automatically set center at some point
    // TODO only 1 center cell for now, it shouldnt matter for now but we will want to change that later
    maze[7][7].d = 0;
    // maze[7][8].d = 0;
    // maze[8][7].d = 0;
    // maze[8][8].d = 0;
    maze_cell_t* center_cell = &maze[7][7];
    maze_cell_t* starting_pos = &maze[0][0];
    maze_cell_t* current_pos = starting_pos;
    flood_maze(maze);
    

    //starting cell is always in bottom-left with walls to north, east, and south
    // starting direction is north
    maze[0][0].walls |= (WALL_S | WALL_W);
    if(API_wallRight()) maze[0][0].walls |= WALL_E;
    if(API_wallFront()) maze[0][0].walls |= WALL_N;

    updateMMSFromCell(&maze[0][0]);

    // N=0, E=1, S=2, W=3
    int8_t dr[] = {1, 0, -1, 0};
    int8_t dc[] = {0, 1, 0, -1};
    uint8_t wall_mask[] = {WALL_N, WALL_E, WALL_S, WALL_W};
    char dir_char[] = {'n', 'e', 's', 'w'};
    uint8_t op_map[] = {2, 3, 0, 1};

    uint8_t current_dir = 0;
    uint8_t left_dir, right_dir, nr, nc;

    while (1) {

    
        
        left_dir = (current_dir + 3) % 4;
        right_dir = (current_dir + 1) % 4;
        
        // check walls and update maze map if needed
        if(API_wallFront()){
            nr = current_pos->R + dr[current_dir];
            nc = current_pos->C + dc[current_dir];
            // set current cell wall if wall in front
            API_setWall(current_pos->C, current_pos->R, dir_char[current_dir]);
            current_pos->walls |= wall_mask[current_dir];

            // set next cell wall if there is one
            if(nr >= 0 && nr < MAZE_LENGTH && nc >= 0 && nc < MAZE_WIDTH){
                maze_cell_t* neighborCell = &maze[nr][nc];
                API_setWall(neighborCell->C, neighborCell->R, dir_char[op_map[current_dir]]);
                neighborCell->walls |= wall_mask[op_map[current_dir]];
            }
            flood_maze(maze);
        }
        if(API_wallRight()){
            nr = current_pos->R + dr[right_dir];
            nc = current_pos->C + dc[right_dir];
            // set current cell wall if wall in front
            API_setWall(current_pos->C, current_pos->R, dir_char[right_dir]);
            current_pos->walls |= wall_mask[right_dir];

            // set next cell wall if there is one
            if(nr >= 0 && nr < MAZE_LENGTH && nc >= 0 && nc < MAZE_WIDTH){
                maze_cell_t* neighborCell = &maze[nr][nc];
                API_setWall(neighborCell->C, neighborCell->R, dir_char[op_map[right_dir]]);
                neighborCell->walls |= wall_mask[op_map[right_dir]];
            }
            flood_maze(maze);
        }
        if(API_wallLeft()){
            nr = current_pos->R + dr[left_dir];
            nc = current_pos->C + dc[left_dir];
            // set current cell wall if wall in front
            API_setWall(current_pos->C, current_pos->R, dir_char[left_dir]);
            current_pos->walls |= wall_mask[left_dir];

            // set next cell wall if there is one
            if(nr >= 0 && nr < MAZE_LENGTH && nc >= 0 && nc < MAZE_WIDTH){
                maze_cell_t* neighborCell = &maze[nr][nc];
                API_setWall(neighborCell->C, neighborCell->R, dir_char[op_map[left_dir]]);
                neighborCell->walls |= wall_mask[op_map[left_dir]];
            }
            flood_maze(maze);
        }


        // reflood the maze after checking walls
        uint8_t high_d = 255, best_dir = 255;
        uint8_t dir;
        bool wayForward = false;
        uint8_t back_dir = (current_dir + 2) % 4;
        uint8_t dir_order[4] = {
            (current_dir + 1) % 4, // right
            (current_dir + 3) % 4, // left
            current_dir,           // forward
            (current_dir + 2) % 4  // backward
        };

        // find best direction
        for(int i = 0; i < 4; i++){

            dir = dir_order[i];
            // skip direction if there is wall
            if(current_pos->walls & wall_mask[dir]) continue;
            nr = current_pos->R + dr[dir];
            nc = current_pos->C + dc[dir];

            maze_cell_t* neighbor = &maze[nr][nc];
            if(neighbor->d < high_d) {
                if(dir != back_dir){
                    high_d = neighbor->d;
                    best_dir = dir;
                    wayForward = true;
                } else if (!wayForward){
                    high_d = neighbor->d;
                    best_dir = dir;
                }
            }
        }

        // if need to change direction, do that
        if(current_dir != best_dir){
            // turn right
            if((current_dir + 1) % 4 == best_dir){
                API_turnRight();
            } else if((current_dir + 3) % 4 == best_dir){
                API_turnLeft();
            } else if((current_dir + 2) % 4 == best_dir){
                log("BONK!");
                API_turnRight();
                API_turnRight();
            }
            current_dir = best_dir;
        }
        // move forward
        API_moveForward();
        current_pos = &maze[current_pos->R + dr[current_dir]][current_pos->C + dc[current_dir]];
        // restart maze if reach the end



    }
}
