#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define PORT 8888
#define MAX_USERS 4
#define MAP_WIDTH 63
#define MAP_HEIGHT 33
#define MAX_BEASTS 5

#define WALL_PAIR 1
#define WALL_PAIR_OUT_OF_RANGE 2
#define PLAYER_PAIR 3
#define COIN_PAIR 4
#define CAMP_PAIR 5
#define BEAST_PAIR 6

pthread_mutex_t lock;

enum type_e {CPU, BEAST, HUMAN};

struct user_t{
    int PID;
    int socket_ID;
    int game_ID;
    enum type_e type;
    int pos_x;
    int pos_y;
    int spawn_pos_x;
    int spawn_pos_y;
    int coins;
    int points_at_base;
    int deaths;
    int krzak;
}user_table[MAX_USERS];

struct point_t{
    int x;
    int y;
};

struct server_t{
    int connected_users;
    struct point_t campsite_xy;
    char* address;
    int beasts;
}server = {0};

struct server_info_t{
    int connected_users;
    int max_users;
    int PID;
};

struct beast_t{
    int x;
    int y;
    int chasing_player_id;
}server_beasts[MAX_BEASTS];

struct death_coins_t{
    int x;
    int y;
    int value;
}death_coins[200] = {0};

struct data_block_t{
    struct server_info_t server_info;
    struct user_t user_data;
    unsigned char map_fragment[5][5];
};
int server_action = 0;

//const unsigned char map[MAP_HEIGHT][MAP_WIDTH]={
//        " ============================================================= ",
//        "                                                               ",
//        "          ==                                    ==             ",
//        "                                                               ",
//        "                               ==                              ",
//        "                                                               ",
//        "                                                               ",
//        "                     ==                                        ",
//        "                                                               ",
//        "                            ==                                 ",
//        "                                                ==             ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        "                                                               ",
//};

//const unsigned char map[MAP_HEIGHT][MAP_WIDTH]={
//        "                                                               ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ==============================  ============================= ",
//        " ==============================  ============================= ",
//        " ===========================     ============================= ",
//        " =============================== ===== ======================= ",
//        " =============================== ===== ======================= ",
//        " ===========================     ===== ======================= ",
//        " ===============================       ======================= ",
//        " =============================== ============================= ",
//        " =============================== ============================= ",
//        " ============================       ========================== ",
//        " =============================== ============================= ",
//        " =============================== ============================= ",
//        " ==========================      ============================= ",
//        " ========================== ================================== ",
//        " ========================== ================================== ",
//        " ========================== ================================== ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        " ============================================================= ",
//        "                                                               ",
//};

const unsigned char map[MAP_HEIGHT][MAP_WIDTH]={
        "                                                               ",
        " ============================================================= ",
        " = =     =   = = =     = = = = = =     =   ########= =     = = ",
        " = = ===== === = === = = = = = = === = = ========= = = === = = ",
        " =   =   =   =     = =       =     = =       = =   =     =   = ",
        " = ===== = ======= === ===== = = === === = = = =========== === ",
        " =     = =     =   =     =   = = =   =   = = = =       = =   = ",
        " === = = ===== = === === ===== = = === = === = ===== = = === = ",
        " = = = = = =   =       =   = = =     = = =   =   =   = = =   = ",
        " = = = = = = ===== = ======= = ===== =========== = ===== = === ",
        " = = =    ####     = = =   =     =         =     =   =       = ",
        " = = ======= = === = =#=== === === ======= === ===== ======= = ",
        " =   =     = =   = = ####  =   =   = = = =   =       = =     = ",
        " === = =========== === ========= === = = === === ===== === === ",
        " = =       =     = =           =   =       = =   =   =     = = ",
        " = = = === ===== === ============= === = = === ===== = === = = ",
        " = = = = =   = =  #  =  ####     =     = =   =   =     = =   = ",
        " = = = = ===== ======= ======= ===== = = = ===== ===== = = === ",
        " =   =   =   =  ####     =     =     = = = = =       = =   = = ",
        " ===== === = === === = === === === ========= = ===== === = = = ",
        " = =       =     =#= =   = =             = = =   =     = =   = ",
        " = === ===== === =#= ======= ========= === = = =========== = = ",
        " = =   =   = = =  #=   =   = =   =     = =   =     = = =   = = ",
        " = === = = === = =#===== = === = === = = = === = = = = = ===== ",
        " = =     =     = = = =   =   = =   = =     =   = = =         = ",
        " = === ========= === ===== === = ========= = = ===== ===== = = ",
        " =         =   =   = =       = =   =   ###   = = =     =   = = ",
        " = = === === = = = = === ======= === === === = = = = === = = = ",
        " = = =       = = = = =     =     =     = = = =   = =   = = = = ",
        " =========== === === = === === ========= = ===== = ======= === ",
        " =           =     =   =          ######       =     =       = ",
        " ============================================================= ",
        "                                                               ",
};
void end();
unsigned char misc_map[MAP_HEIGHT][MAP_WIDTH] = {0};

void* handle_client(void* arg){
    struct user_t* user_record = (struct user_t *)arg;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Server full";
    int action = 0;

    valread = read(user_record->socket_ID, &action, sizeof(int));
    if(valread == -1){
        //wprintw(Debug,"No response from %d\n, user_record->socket_ID);
    }
    else{
        struct data_block_t data;
        data.server_info.connected_users = server.connected_users;
        data.server_info.max_users = MAX_USERS;
        data.user_data = *user_record;
        data.map_fragment[2][2] = '0' + data.user_data.game_ID;

        for(int l = 0, r = 0; l> -3 && r < 3; l--, r++){
            for(int u = 0, d = 0; u> -3 && d < 3; u--, d++){
                data.map_fragment[2 + d][2 + r] = map[user_record->pos_y + d][user_record->pos_x + r]; //BOTTOM-RIGHT BOX
                data.map_fragment[2 + d][2 + l] = map[user_record->pos_y + d][user_record->pos_x + l]; //BOTTOM-LEFT BOX
                data.map_fragment[2 + u][2 + r] = map[user_record->pos_y + u][user_record->pos_x + r]; //TOP-RIGHT BOX
                data.map_fragment[2 + u][2 + l] = map[user_record->pos_y + u][user_record->pos_x + l]; //TOP-LEFT BOX
            }
        }
        for(int l = 0, r = 0; l> -3 && r < 3; l--, r++){
            for(int u = 0, d = 0; u> -3 && d < 3; u--, d++){
                unsigned char x = misc_map[user_record->pos_y + d][user_record->pos_x + r];
                if(x == 'A' || x == 'c' || x == 't' || x == 'T' || x == 'D'){
                    data.map_fragment[2 + d][2 + r] = misc_map[user_record->pos_y + d][user_record->pos_x + r]; //BOTTOM-RIGHT BOX
                }
                x = misc_map[user_record->pos_y + d][user_record->pos_x + l];
                if(x == 'A' || x == 'c' || x == 't' || x == 'T' || x == 'D'){
                    data.map_fragment[2 + d][2 + l] = misc_map[user_record->pos_y + d][user_record->pos_x + l]; //BOTTOM-LEFT BOX
                }
                x = misc_map[user_record->pos_y + u][user_record->pos_x + r];
                if(x == 'A' || x == 'c' || x == 't' || x == 'T' || x == 'D'){
                    data.map_fragment[2 + u][2 + r] = misc_map[user_record->pos_y + u][user_record->pos_x + r]; //TOP-RIGHT BOX
                }
                x = misc_map[user_record->pos_y + u][user_record->pos_x + l];
                if(x == 'A' || x == 'c' || x == 't' || x == 'T' || x == 'D'){
                    data.map_fragment[2 + u][2 + l] = misc_map[user_record->pos_y + u][user_record->pos_x + l]; //TOP-LEFT BOX
                }
            }
        }
        for(int i = 0; i < server.connected_users; i++){
            if(user_table[i].game_ID == user_record->game_ID)
                continue;
            int x_diff = user_table[i].pos_x - user_record->pos_x;
            int y_diff = user_table[i].pos_y - user_record->pos_y;
            if((x_diff < 3 && x_diff > -3) && (y_diff < 3 && y_diff > -3)){
                data.map_fragment[2 + y_diff][2 + x_diff] = '0' + user_table[i].game_ID;
            }
        }
        for(int i = 0; i < server.beasts; i++){
            int x_diff = server_beasts[i].x - user_record->pos_x;
            int y_diff = server_beasts[i].y - user_record->pos_y;
            if((x_diff < 3 && x_diff > -3) && (y_diff < 3 && y_diff > -3)){
                data.map_fragment[2 + y_diff][2 + x_diff] = '*';
            }
        }

        send(user_record->socket_ID, &data, sizeof(struct data_block_t), MSG_NOSIGNAL);
        switch(action){
            case 261:{
                if(map[user_record->pos_y][user_record->pos_x + 1] != '='){
                    if(map[user_record->pos_y][user_record->pos_x] == '#' && !user_record->krzak){
                        user_record->krzak = 1;
                        break;
                    }
                    if(user_record->krzak){
                        user_record->krzak = 0;
                    }
                    user_record->pos_x++;
                }
                break;
            }
            case 260:{
                if(map[user_record->pos_y][user_record->pos_x - 1] != '='){
                    if(map[user_record->pos_y][user_record->pos_x] == '#' && !user_record->krzak){
                        user_record->krzak = 1;
                        break;
                    }
                    if(user_record->krzak){
                        user_record->krzak = 0;
                    }
                    user_record->pos_x--;
                }
                break;
            }
            case 259:{
                if(map[user_record->pos_y - 1][user_record->pos_x] != '='){
                    if(map[user_record->pos_y][user_record->pos_x] == '#' && !user_record->krzak){
                        user_record->krzak = 1;
                        break;
                    }
                    if(user_record->krzak){
                        user_record->krzak = 0;
                    }
                    user_record->pos_y--;
                }
                break;
            }
            case 258:{
                if(map[user_record->pos_y + 1][user_record->pos_x] != '='){
                    if(map[user_record->pos_y][user_record->pos_x] == '#' && !user_record->krzak){
                        user_record->krzak = 1;
                        break;
                    }
                    if(user_record->krzak){
                        user_record->krzak = 0;
                    }
                    user_record->pos_y++;
                }
                break;
            }
            case -1:{
                break;
            }
            case 113:{
                errno = 32;
                break;
            }
        }
        if(errno == 32){ //client disconnected
            close(user_record->socket_ID);
            user_record->socket_ID = 0;
        }
        //end();
    }
    return NULL;
}

int generate_coin(char);
int generate_beast();

void* beast_logic(void* arg){
    int beast_id = *(int*)arg;
    int player_near = 0;
    int y = server_beasts[beast_id].y;
    int x = server_beasts[beast_id].x;
    unsigned char player_id = 0;

    for(int i = 0; i < server.connected_users; i++){
        if(abs(user_table[i].pos_x - x) < 2 && abs(user_table[i].pos_y - y) < 2){
            player_near = 1;
            player_id = user_table[i].game_ID;
            break;
        }
        else if(abs(user_table[i].pos_x - x) < 3 && abs(user_table[i].pos_y - y) < 3){
            player_near = 1;
            player_id = user_table[i].game_ID;
            break;
        }
    }

    if(player_near){ //check player visibility;
        struct point_t player_pos;
        for(int i = 0; i <server.connected_users ; i++){ //Copy user values and calculate visibility
            if(user_table[i].game_ID == player_id){
                player_pos.x = user_table[i].pos_x;
                player_pos.y = user_table[i].pos_y;
            }
        }
        int visible = 1;
        int y_diff = player_pos.y - y;  //while y_diff > 0 - player below beast, y < 0 player above beast
        int x_diff = player_pos.x - x;  //while x_diff > 0 - player is east from beast, y < 0 player is west from beast
        if(y == player_pos.y){  //check for obstacles in x-axis
            for(int i = 0; i < abs(x_diff); i++){
                if(x_diff > 0) {
                    if (map[y][x + i] == '=') {
                        visible = 0;
                    }
                }
                else{
                    if(map[y][x - i] == '='){
                        visible = 0;
                    }
                }
            }
        }
        if(x == player_pos.x) {  //check for obstacles in y-axis
            for(int i = 0; i < abs(y_diff); i++){
                if(y_diff > 0) {
                    if (map[y + i][x] == '=') {
                        visible = 0;
                    }
                }
                else{
                    if(map[y - i][x] == '='){
                        visible = 0;
                    }
                }
            }
        }

        if(abs(y_diff) == 1 && abs(x_diff) == 1){   //visible by diagonal
            visible = 1;
        }
        else if(abs(y_diff) == 2 && abs(x_diff) == 2){  //visible by longer diagonal
            if(map[y + y_diff/2][x + x_diff/2] == '=')
                visible = 0;
            else
                visible = 1;
        }
        else if(abs(x_diff) == 1 && abs(y_diff) == 2){
            if(map[y + x_diff][x] == '=' || map[y + x_diff][x + y_diff/2] == '=')
                visible = 0;
        }
        else if(abs(x_diff) == 2 && abs(y_diff) == 1){
            if(map[y][x + y_diff] == '=' || map[y + x_diff/2][x + y_diff] == '=')
                visible = 0;
        }

        if(visible)
            server_beasts[beast_id].chasing_player_id = player_id;
        else
            server_beasts[beast_id].chasing_player_id = -1;
        if(server_beasts[beast_id].chasing_player_id > 0){  //start chasing
            if(x_diff > 0){
                if(map[server_beasts[beast_id].y][server_beasts[beast_id].x + 1] != '=') {
                    server_beasts[beast_id].x++;
                }
                else if(y_diff > 0){
                    if(map[server_beasts[beast_id].y + 1][server_beasts[beast_id].x] != '=') {
                        server_beasts[beast_id].y++;
                    }
                }
                else{
                    if(map[server_beasts[beast_id].y - 1][server_beasts[beast_id].x] != '=') {
                        server_beasts[beast_id].y--;
                    }
                }
            }
            else if(x_diff < 0){
                if(map[server_beasts[beast_id].y][server_beasts[beast_id].x - 1] != '=') {
                    server_beasts[beast_id].x--;
                }
                else if(y_diff < 0){
                    if(map[server_beasts[beast_id].y - 1][server_beasts[beast_id].x] != '=') {
                        server_beasts[beast_id].y--;
                    }
                }
                else{
                    if(map[server_beasts[beast_id].y + 1][server_beasts[beast_id].x] != '=') {
                        server_beasts[beast_id].y++;
                    }
                }
            }
            else if(y_diff > 0){
                if(map[server_beasts[beast_id].y + 1][server_beasts[beast_id].x] != '=') {
                    server_beasts[beast_id].y++;
                }
                else if(x_diff > 0){
                    if(map[server_beasts[beast_id].y][server_beasts[beast_id].x + 1] != '=') {
                        server_beasts[beast_id].x++;
                    }
                }
                else{
                    if(map[server_beasts[beast_id].y][server_beasts[beast_id].x + 1] != '=') {
                        server_beasts[beast_id].x++;
                    }
                }
            }
            else if(y_diff < 0){
                if(map[server_beasts[beast_id].y - 1][server_beasts[beast_id].x] != '=') {
                    server_beasts[beast_id].y--;
                }
                else if(x_diff < 0){
                    if(map[server_beasts[beast_id].y][server_beasts[beast_id].x - 1] != '=') {
                        server_beasts[beast_id].x--;
                    }
                }
                else{
                    if(map[server_beasts[beast_id].y][server_beasts[beast_id].x - 1] != '=') {
                        server_beasts[beast_id].x--;
                    }
                }
            }
        }
        else{   //random moves
            int dir = rand() % 4 + 1;
            switch(dir){
                case 1:{
                    if(map[y][x + 1] != '=')
                        server_beasts[beast_id].x++;
                    break;
                }
                case 2:{
                    if(map[y + 1][x] != '=')
                        server_beasts[beast_id].y++;
                    break;
                }
                case 3:{
                    if(map[y][x - 1] != '=')
                        server_beasts[beast_id].x--;
                    break;
                }
                case 4:{
                    if(map[y - 1][x] != '=')
                        server_beasts[beast_id].y--;
                    break;
                }
                default:{
                    break;
                }
            }
            server_beasts[beast_id].chasing_player_id = -1;
        }
    }
    else{   //random moves
        int dir = rand() % 4 + 1;
        switch(dir){
            case 1:{
                if(map[y][x + 1] != '=')
                    server_beasts[beast_id].x++;
                break;
            }
            case 2:{
                if(map[y + 1][x] != '=')
                    server_beasts[beast_id].y++;
                break;
            }
            case 3:{
                if(map[y][x - 1] != '=')
                    server_beasts[beast_id].x--;
                break;
            }
            case 4:{
                if(map[y - 1][x] != '=')
                    server_beasts[beast_id].y--;
                break;
            }
            default:{
                break;
            }
        }
        server_beasts[beast_id].chasing_player_id = -1;
    }
    return NULL;
}

void* handle_beasts_thread(void* arg){
    pthread_t beasts[MAX_BEASTS];
    do{
        if(server_action == 'q' || server_action == 'Q'){
            break;
        }
        for(int i = 0; i < server.beasts; i++) {
            pthread_create(&beasts[i], NULL, beast_logic, &i);
            pthread_join(beasts[i], NULL);
        }
        sleep(1);
    }while(1);
    return NULL;
}

void* keyboard_input_thread(void* arg){
    int* input = (int*)arg;
    while(*input != 'q' || *input != 'Q'){
        timeout(-1);
        flushinp();
        *input = getch();
        if(*input == 'c' || *input == 't' || *input == 'T'){
            pthread_mutex_lock(&lock);
            int k = generate_coin(*(char*)input);
            pthread_mutex_unlock(&lock);
        }
        if(*input == 'b' || *input == 'B'){
            pthread_mutex_lock(&lock);
            int k = generate_beast();
            pthread_mutex_unlock(&lock);
        }
        usleep(200000);
    }
    return NULL;
}

struct point_t generate_spawn_point(){
    struct point_t result;
    srand(time(NULL));
    int valid = 0;
    int max_tries = MAP_WIDTH * MAP_HEIGHT;
    int k = 0;
    while(!valid){
        if(k >= max_tries){ //map full
            result.x = -1;
            result.y = -1;
            return result;
        }
        int x = rand() % (MAP_WIDTH - 2) + 1;
        int y = rand() % (MAP_HEIGHT - 2) + 1;
        for(int i = 0; i < server.connected_users; i++){
            if(user_table[i].pos_x == x && user_table[i].pos_y == y){   //prevent spawning in the same position with other player
                continue;
            }
            if(server.campsite_xy.x == x && server.campsite_xy.y == y){ //prevent spawning on base
                continue;
            }
        }
        for(int i = 0; i < server.beasts; i++){ //prevent spawning on beast
            if(server_beasts[i].x == x && server_beasts[i].y == y)
                continue;
        }
        if(misc_map[y][x] == 'c' || misc_map[y][x] == 't' || misc_map[y][x] == 'T' || misc_map[y][x] == 'D' || misc_map[y][x] == 'A'){
            //prevent spawning on any other entity
            continue;
        }
        for(int i = 0; i < server.connected_users; i++){    //copy map to miscellaneous map
            for(int a = 0; a < MAP_HEIGHT; a++){
                for(int b = 0; b < MAP_WIDTH; b++){
                    if(map[a][b] == '1' || map[a][b] == '2' || map[a][b] == '3' || map[a][b] == '4')
                        misc_map[a][b] = map[a][b];
                }
            }
            misc_map[user_table[i].pos_y][user_table[i].pos_x] = '0' + user_table[i].game_ID;
        }
        if(map[y][x] == ' '){
            int player_near = 0;
            for(int i = 0;i < 3; i++){
                for(int l = 0; l < 3 ; l++){
                    if(misc_map[y + i][x + l] == '1' || misc_map[y + i][x + l] == '2' || misc_map[y + i][x + l] == '3' || misc_map[y + i][x + l] == '4')
                        player_near = 1;
                    if(misc_map[y + i][x - l] == '1' || misc_map[y + i][x - l] == '2' || misc_map[y + i][x - l] == '3' || misc_map[y + i][x - l] == '4')
                        player_near = 1;
                    if(misc_map[y - i][x + l] == '1' || misc_map[y - i][x + l] == '2' || misc_map[y - i][x + l] == '3' || misc_map[y - i][x + l] == '4')
                        player_near = 1;
                    if(misc_map[y - i][x - l] == '1' || misc_map[y - i][x - l] == '2' || misc_map[y - i][x - l] == '3' || misc_map[y - i][x - l] == '4')
                        player_near = 1;
                }
            }
            if(!player_near)
                valid = 1;
        }
        if(valid){
            result.x = x;
            result.y = y;
        }
        k++;
    }
    return result;
}

int generate_coin(char type){
    struct point_t point = generate_spawn_point();
    if(point.x == -1 && point.y == -1){
        return -1;
    }
    misc_map[point.y][point.x] = type;
    return 1;
}

int generate_beast(){
    struct point_t p = generate_spawn_point();
    if(p.y == -1 && p.x == -1){
        return 0;   //map is full
    }
    if(server.beasts >= MAX_BEASTS){
        return -1; //No more beasts allowed
    }
    server_beasts[server.beasts].x = p.x;
    server_beasts[server.beasts].y = p.y;
    server_beasts[server.beasts].chasing_player_id = -1;
    server.beasts++;
}

void check_for_collisions(){
    for(int i = 0; i < server.connected_users; i++){
        if(misc_map[user_table[i].pos_y][user_table[i].pos_x] == 'c'){
            misc_map[user_table[i].pos_y][user_table[i].pos_x] = ' ';
            user_table[i].coins++;
        }
        if(misc_map[user_table[i].pos_y][user_table[i].pos_x] == 't'){
            misc_map[user_table[i].pos_y][user_table[i].pos_x] = ' ';
            user_table[i].coins += 10;
        }
        if(misc_map[user_table[i].pos_y][user_table[i].pos_x] == 'T'){
            misc_map[user_table[i].pos_y][user_table[i].pos_x] = ' ';
            user_table[i].coins += 50;
        }
        if(user_table[i].pos_y == server.campsite_xy.y && user_table[i].pos_x == server.campsite_xy.x){
            user_table[i].points_at_base += user_table[i].coins;
            user_table[i].coins = 0;
        }

        if(misc_map[user_table[i].pos_y][user_table[i].pos_x] == 'D'){
            for(int a = 0; a < 200; a++){
                if(death_coins[a].x == user_table[i].pos_x && death_coins[a].y == user_table[i].pos_y){
                    user_table[i].coins += death_coins[a].value;
                    misc_map[death_coins[a].y][death_coins[a].x] = ' ';
                    death_coins[a].y = 0;
                    death_coins[a].x = 0;
                    death_coins[a].value = 0;
                    break;
                }
            }
            for(int a = 0; a < 199; a++){
                if(death_coins[a].x == 0 && death_coins[a].y == 0){
                    for(int l = 0; l < 199; l++){
                        death_coins[l] = death_coins[l+1];
                    }
                }
            }
        }

        for(int l = i+1; l < server.connected_users; l++){
            if(user_table[l].pos_x == user_table[i].pos_x && user_table[l].pos_y == user_table[i].pos_y){
                int k = 0;
                while(death_coins[k].y != 0){
                    k++;
                }
                if(user_table[l].coins == 0 && user_table[i].coins == 0){
                    user_table[i].pos_x = user_table[i].spawn_pos_x;
                    user_table[i].pos_y = user_table[i].spawn_pos_y;
                    user_table[l].pos_x = user_table[l].spawn_pos_x;
                    user_table[l].pos_y = user_table[l].spawn_pos_y;
                    break;
                }
                death_coins[k].x = user_table[l].pos_x;
                death_coins[k].y = user_table[l].pos_y;
                death_coins[k].value = user_table[i].coins + user_table[l].coins;
                user_table[i].coins = 0;
                user_table[l].coins = 0;
                misc_map[user_table[i].pos_y][user_table[i].pos_x] = 'D';
                user_table[i].pos_x = user_table[i].spawn_pos_x;
                user_table[i].pos_y = user_table[i].spawn_pos_y;
                user_table[l].pos_x = user_table[l].spawn_pos_x;
                user_table[l].pos_y = user_table[l].spawn_pos_y;
            }
        }

        for(int l = 0; l < server.beasts; l++){
            if(user_table[i].pos_x == server_beasts[l].x && user_table[i].pos_y == server_beasts[l].y){
                int k = 0;
                while(death_coins[k].y != 0){
                    k++;
                }
                if(user_table[i].coins == 0){
                    user_table[i].pos_x = user_table[i].spawn_pos_x;
                    user_table[i].pos_y = user_table[i].spawn_pos_y;
                }
                death_coins[k].x = user_table[i].pos_x;
                death_coins[k].y = user_table[i].pos_y;
                death_coins[k].value = user_table[i].coins;
                user_table[i].coins = 0;
                misc_map[user_table[i].pos_y][user_table[i].pos_x] = 'D';
                user_table[i].pos_x = user_table[i].spawn_pos_x;
                user_table[i].pos_y = user_table[i].spawn_pos_y;
            }
        }
    }
}

void* socket_server_daemon(void* arg){
    pthread_t users_t[MAX_USERS];
    pthread_t kbd_in;

    int users[MAX_USERS] = {0};
    int server_fd, new_socket, valread;
    struct sockaddr_in address;

    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Server is full";
    char *hello_2 = "Welcome";
    int action = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    int rc = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if(rc < 0){
        perror("setsockopt() failed");
        close(server_fd);
        exit(-1);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        return NULL;
    }

    pthread_create(&kbd_in, NULL, keyboard_input_thread, &server_action);
    listen(server_fd, MAX_USERS);
    while(1){
        new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
        if(new_socket > 0 && server.connected_users < MAX_USERS){
            int PID = 0;
            send(new_socket, hello_2, strlen(hello_2), MSG_NOSIGNAL);
            valread = read(new_socket, &PID, sizeof(int));
            user_table[server.connected_users].PID = PID;
            user_table[server.connected_users].socket_ID = new_socket;
            user_table[server.connected_users].game_ID = server.connected_users + 1;
            user_table[server.connected_users].type = HUMAN;
            user_table[server.connected_users].coins = 0;
            user_table[server.connected_users].points_at_base = 0;
            user_table[server.connected_users].krzak = 0;
            struct point_t spawn_point = generate_spawn_point();
            user_table[server.connected_users].spawn_pos_x = spawn_point.x;
            user_table[server.connected_users].spawn_pos_y = spawn_point.y;
            user_table[server.connected_users].pos_x = user_table[server.connected_users].spawn_pos_x;
            user_table[server.connected_users].pos_y = user_table[server.connected_users].spawn_pos_y;
            server.connected_users++;
        }
        else if(new_socket == -1){
            //Do nothing because no new connection
        }
        else{
            send(new_socket, hello, strlen(hello), 0);
            close(new_socket);
        }

        for(int i = 0; i < server.connected_users; i++){
            if(user_table[i].socket_ID == 0){   //If connection lost/close
                for(int a = i; a < server.connected_users -1; a++){
                    user_table[a] = user_table[a + 1];
                    user_table[a].game_ID--;
                }
                server.connected_users--;
            }
        }

        for(int i = 0; i <server.connected_users; i++){
            pthread_create(&users_t[i], NULL, handle_client, &user_table[i]);
        }
        for(int i = 0; i <server.connected_users; i++){
            pthread_join(users_t[i], NULL);
        }
        if(server_action == 'q'){
            for(int i = 0; i <server.connected_users; i++){
                close(user_table[i].socket_ID);
            }
            break;
        }
    }
    close(server_fd);
    return NULL;
}

void* display_map(){
    pthread_mutex_lock(&lock);
    for(int l = 0; l < MAP_HEIGHT; l++){
        for(int k = 0; k < MAP_WIDTH; k++){
            mvprintw(4, 100, "map %d", l+k);
            if(map[l][k] == '='){
                attron(COLOR_PAIR(WALL_PAIR));
                mvprintw(l, k, "%c", map[l][k]);
                attroff(COLOR_PAIR(WALL_PAIR));
            }
            else if(map[l][k] == '1' || map[l][k] == '2' || map[l][k] == '3' || map[l][k] == '4'){
                attron(COLOR_PAIR(PLAYER_PAIR));
                mvprintw(l, k, "%c", map[l][k]);
                attroff(COLOR_PAIR(PLAYER_PAIR));
            }
            else if(map[l][k]){
                mvprintw(l, k, "%c", map[l][k]);
            }
        }
    }
    for(int i = 0; i < server.connected_users; i++){
        attron(COLOR_PAIR(PLAYER_PAIR));
        mvprintw(user_table[i].pos_y, user_table[i].pos_x, "%c", '0' + user_table[i].game_ID);
        attroff(COLOR_PAIR(PLAYER_PAIR));
    }

    for(int l = 0; l < MAP_HEIGHT; l++){
        for(int k = 0; k < MAP_WIDTH; k++){
            mvprintw(4, 100, "map %d", l+k);
            if(misc_map[l][k] == 'c' || misc_map[l][k] == 't' || misc_map[l][k] == 'T' || misc_map[l][k] == 'D'){
                attron(COLOR_PAIR(COIN_PAIR));
                mvprintw(l, k, "%c", misc_map[l][k]);
                attroff(COLOR_PAIR(COIN_PAIR));
            }
            if(misc_map[l][k] == 'A'){
                attron(COLOR_PAIR(CAMP_PAIR));
                mvprintw(l, k, "%c", misc_map[l][k]);
                attroff(COLOR_PAIR(CAMP_PAIR));
            }
        }
    }
    for(int i = 0; i < server.beasts; i++){
        attron(COLOR_PAIR(BEAST_PAIR));
        mvprintw(server_beasts[i].y, server_beasts[i].x, "%c", '*');
        attroff(COLOR_PAIR(BEAST_PAIR));
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}
int main() {
    if(pthread_mutex_init(&lock, NULL) != 0){
        printf("\n mutex init failed");
        return 1;
    }
    pthread_t th1, th2, th3;
    initscr();
    pthread_create(&th1, NULL, socket_server_daemon, NULL);
    pthread_create(&th2, NULL, handle_beasts_thread, NULL);
    noecho();
    raw();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(WALL_PAIR, COLOR_BLACK, COLOR_WHITE);
    init_pair(WALL_PAIR_OUT_OF_RANGE, COLOR_WHITE, COLOR_BLACK);
    init_pair(PLAYER_PAIR, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(COIN_PAIR, COLOR_BLACK, COLOR_YELLOW);
    init_pair(CAMP_PAIR, COLOR_BLACK, COLOR_GREEN);
    init_pair(BEAST_PAIR, COLOR_WHITE, COLOR_RED);
    int i = 0;
    int pid = getpid();
    struct point_t camp = generate_spawn_point();
    server.campsite_xy.x = camp.x;
    server.campsite_xy.y = camp.y;
    misc_map[camp.y][camp.x] = 'A';
    do{
        display_map();
        check_for_collisions();
        if(server_action == 'q')
            break;
        pthread_mutex_lock(&lock);
        mvprintw(3, 98, "Server's PID: %d", pid);
        mvprintw(4, 100, "Campsite X/Y: %2d/%2d", server.campsite_xy.x-1, server.campsite_xy.y-1);
        mvprintw(5, 100, "Server tick: %d", i);
        mvprintw(7, 98, "Parameters:    Player1     Player2     Player3     Player4");
        mvprintw(8, 100, "PID:");
        for(int a = 0; a < MAX_USERS; a++){
            if(a < server.connected_users)
                mvprintw(8, 113 + a * 11, "%d", user_table[a].PID);
            else
                mvprintw(8, 113 + a * 11, "   ---   ");
        }
        mvprintw(9, 100, "Type");
        for(int a = 0; a < MAX_USERS; a++){
            if(a < server.connected_users){
                mvprintw(9, 113 + a * 11, "HUMAN");
            }
            else
                mvprintw(9, 113 + a * 11, "    -   ");
        }
        mvprintw(10, 100, "Curr X/Y");
        for(int a = 0; a < MAX_USERS; a++){
            if(a < server.connected_users){
                mvprintw(10, 113 + a * 11, "%2d/%2d", user_table[a].pos_x - 1, user_table[a].pos_y - 1);
            }
            else
                mvprintw(10, 113 + a * 11, "   -/-  ");
        }
        mvprintw(11, 100, "Deaths");
        for(int a = 0; a < MAX_USERS; a++){
            if(a < server.connected_users){
                mvprintw(11, 115 + a * 11, "%d", user_table[a].deaths);
            }
            else
                mvprintw(11, 113 + a * 11, "    -   ");
        }
        mvprintw(13, 98, "Coins");
        mvprintw(14, 100, "Carried");
        for(int a = 0; a < MAX_USERS; a++){
            if(a < server.connected_users){
                mvprintw(14, 115 + a * 11, "%5d", user_table[a].coins);
            }
            else
                mvprintw(14, 113 + a * 11, "    -   ");
        }
        mvprintw(15, 100, "At base");
        for(int a = 0; a < MAX_USERS; a++){
            if(a < server.connected_users){
                mvprintw(15, 115 + a * 11, "%5d", user_table[a].points_at_base);
            }
            else
                mvprintw(15, 113 + a * 11, "    -   ");
        }
        mvprintw(17, 98, "Beasts");
        mvprintw(18, 100, "Alive: %d", server.beasts);
        mvprintw(19, 100, "Total available: %d", MAX_BEASTS);

        for(int a = 0; a < server.beasts; a++){
            mvprintw(21, 100 + a * 8, "(%d/%d) ",server_beasts[a].x - 1, server_beasts[a].y - 1);
        }
        for(int a = 0; a < server.beasts; a++){
            mvprintw(22, 100 + a * 3, "%d ", server_beasts[a].chasing_player_id);
        }

        mvprintw(30, 98, "Available commands: ");
        mvprintw(31, 100, "q,Q - Stop server and quit");
        mvprintw(32, 100, "c/t/T - Spawn coin/treasure/big treasure");
        mvprintw(33, 100, "b/B - spawn beast");
        i++;
        pthread_mutex_unlock(&lock);
        refresh();
        usleep(200000);
    }while(1);

    pthread_join(th1, NULL);
    endwin();
    return 0;
}























