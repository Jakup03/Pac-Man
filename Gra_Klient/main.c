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
#define MAP_WIDTH 63
#define MAP_HEIGHT 33

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
};

struct server_info_t{
    int connected_users;
    int max_users;
    int PID;
};
struct data_block_t{
    struct server_info_t server_info;
    struct user_t user_data;
    unsigned char map_fragment[5][5];
};
struct point_t{
    int x;
    int y;
}camp_base = {0};

unsigned char downloaded_map[MAP_HEIGHT][MAP_WIDTH] = {0};

void display_map(const struct user_t user_data){
    for(int l = 0;l < MAP_HEIGHT; l++){
        for(int k = 0; k < MAP_WIDTH; k++){
            if(downloaded_map[l][k] == '='){
                if(abs(user_data.pos_x - k) <= 2 && abs(user_data.pos_y - l) <= 2){
                    attron(COLOR_PAIR(WALL_PAIR));
                    mvprintw(l, k, "%c", downloaded_map[l][k]);
                    attroff(COLOR_PAIR(WALL_PAIR));
                }
                else{
                    attron(COLOR_PAIR(WALL_PAIR_OUT_OF_RANGE));
                    mvprintw(l, k, "%c", downloaded_map[l][k]);
                    attroff(COLOR_PAIR(WALL_PAIR_OUT_OF_RANGE));
                }
            }
            else if(downloaded_map[l][k] == user_data.game_ID + '0'){
                if(l != user_data.pos_y && k != user_data.pos_x){
                    downloaded_map[l][k] = 'X';
                    attron(COLOR_PAIR(BEAST_PAIR));
                    mvprintw(l, k, "%c", downloaded_map[l][k]);
                    attroff(COLOR_PAIR(BEAST_PAIR));
                }
                else{
                    attron(COLOR_PAIR(PLAYER_PAIR));
                    mvprintw(l, k, "%c", downloaded_map[l][k]);
                    attroff(COLOR_PAIR(PLAYER_PAIR));
                }
            }
            else if(downloaded_map[l][k] == '1' || downloaded_map[l][k] == '2' || downloaded_map[l][k] == '3' || downloaded_map[l][k] == '4'){
                if(abs(user_data.pos_x - k) <= 2 && abs(user_data.pos_y - l) <= 2){
                    mvprintw(l, k, "%c", downloaded_map[l][k]);
                }
                else{
                    downloaded_map[l][k] = 32;
                    mvprintw(l, k, "%c", downloaded_map[l][k]);
                }
            }
            else if(downloaded_map[l][k] == 'c' || downloaded_map[l][k] == 't' || downloaded_map[l][k] == 'T' || downloaded_map[l][k] == 'D'){
                attron(COLOR_PAIR(COIN_PAIR));
                mvprintw(l, k, "%c", downloaded_map[l][k]);
                attroff(COLOR_PAIR(COIN_PAIR));
            }
            else if(downloaded_map[l][k] == 'A'){
                attron(COLOR_PAIR(CAMP_PAIR));
                mvprintw(l, k, "%c", downloaded_map[l][k]);
                attroff(COLOR_PAIR(CAMP_PAIR));
            }
            else if(downloaded_map[l][k] == '*' || downloaded_map[l][k] == 'X'){
                attron(COLOR_PAIR(BEAST_PAIR));
                mvprintw(l, k, "%c", downloaded_map[l][k]);
                attroff(COLOR_PAIR(BEAST_PAIR));
            }
            else if(downloaded_map[l][k]){
                mvprintw(l, k, "%c", downloaded_map[l][k]);
            }
        }
    }
}

void update_global_map(const struct data_block_t data){
    if(camp_base.x == 0 && camp_base.y == 0){
        for(int i = 0; i < MAP_HEIGHT; i++){
            for(int l = 0; l < MAP_WIDTH; l++){
                if(downloaded_map[i][l] == 'A'){
                    camp_base.x = l;
                    camp_base.y = i;
                    break;
                }
            }
        }
    }
    for(int i = 0; i < 3; i++){
        for(int z = 0; z < 3; z++){
            int posy = 2;
            int posx = 2;
            downloaded_map[data.user_data.pos_y][data.user_data.pos_x] = data.user_data.game_ID + '0';

            downloaded_map[data.user_data.pos_y - (posy - 2) + i][data.user_data.pos_x + (posx - 2) + z] = data.map_fragment[posy + i][posx + z];   //BOTTOM_RIGHT BOX
            downloaded_map[data.user_data.pos_y - (posy - 2) + i][data.user_data.pos_x - (posx - 2) - z] = data.map_fragment[posy + i][posx - z];   //BOTTOM_LEFT BOX
            downloaded_map[data.user_data.pos_y + (posy - 2) - i][data.user_data.pos_x + (posx - 2) + z] = data.map_fragment[posy - i][posx + z];   //TOP_RIGHT BOX
            downloaded_map[data.user_data.pos_y + (posy - 2) - i][data.user_data.pos_x - (posx - 2) - z] = data.map_fragment[posy - i][posx - z];   //TOP_LEFT BOX
        }
    }
}

void* keyboard_input_thread(void* arg){
    int* input = (int*)arg;
    while(*input != 'q' || *input != 'Q'){
        timeout(-1);
        flushinp();
        *input = getch();
        usleep(200000);
    }
    return NULL;
}
void end();

int main() {
    pthread_t th1, th2, th3;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] = {0};
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
    char* IP = "127.0.0.1";
    if(inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0){
        printf("\n Invalid address/Address not supported \n");
        return -1;
    }
    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\n Connection failed \n");
        return -1;
    }

    printf("Connected to server\n");
    valread = read(sock, &buffer, sizeof(buffer));
    if(errno == EPIPE){
        if(strcmp(buffer, "Server is full") == 0){
            printf("Server is full, exiting...\n");
            close(sock);
            return 0;
        }
    }
    if(strcmp(buffer, "Server is full") == 0){
        printf("Server is full, exiting...\n");
        close(sock);
        return 0;
    }
    int pid = getpid();
    send(sock, &pid, sizeof(int), 0);
    printf("%s", buffer);

    WINDOW* win = initscr();
    noecho();
    raw();
    keypad(stdscr, TRUE);
    struct data_block_t buff;
    int input = 0;

    start_color();
    init_pair(WALL_PAIR, COLOR_BLACK, COLOR_WHITE);
    init_pair(WALL_PAIR_OUT_OF_RANGE, COLOR_WHITE, COLOR_BLACK);
    init_pair(PLAYER_PAIR, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(COIN_PAIR, COLOR_BLACK, COLOR_YELLOW);
    init_pair(CAMP_PAIR, COLOR_BLACK, COLOR_GREEN);
    init_pair(BEAST_PAIR, COLOR_WHITE, COLOR_RED);

    pthread_create(&th1, NULL, keyboard_input_thread, &input);
    int tick = 0;

    do{
        send(sock, &input, sizeof(int), MSG_NOSIGNAL);
        if(errno == EPIPE){
            break;
        }
        if(input == 'q'){
            break;
        }

        input = 0;
        refresh();
        valread = read(sock, &buff, sizeof(struct data_block_t));
        if(errno == EPIPE){
            break;
        }
        if(valread > 0)
            update_global_map(buff);

        display_map(buff.user_data);
        mvprintw(1, 101, "Server");
        mvprintw(2, 103, "IP: %s", IP);
        mvprintw(3, 103, "Tick: %d", tick);
        if(camp_base.y && camp_base.x){
            mvprintw(4, 103, "Campsite X/Y: %2d/%2d", camp_base.x, camp_base.y);
        }
        else{
            mvprintw(4, 103, "Campsite X/Y: ?/?");
        }
        mvprintw(6, 101, "PLAYER:");
        mvprintw(7, 103, "Number: %d", buff.user_data.game_ID);
        mvprintw(8, 103, "Type: %d", buff.user_data.type);
        mvprintw(9, 103, "Curr X/Y: %2d/%2d", buff.user_data.pos_x - 1, buff.user_data.pos_y - 1);
        mvprintw(10, 103, "Deaths: %d", buff.user_data.deaths);
        mvprintw(12, 103, "COINS: ");
        mvprintw(13, 103, "Found: %4d", buff.user_data.coins);
        mvprintw(14, 103, "At base: %4d", buff.user_data.points_at_base);
        refresh();
        tick++;
        usleep(200000);
    }while(1);

    endwin();
    if(errno == EPIPE){
        printf("Connection lost, please reconnect\n");
    }
    else{
        printf("Closing connection\n");
    }
    close(sock);
    return 0;
}
















