#include <ncurses.h>



int main(){
    initscr();
    int nrows, nclos;
    getmaxyx(stdscr,nrows,nclos);
    int row, col;
    row = nrows/2;
    col = nclos/2;
    getch();
    cbreak();
    noecho();
    //nrows/=2;
    //nclos/=2;


    keypad(stdscr, 1);
    while (1) {

        int ch = getch();
        clear();

        switch (ch) {
            case 'q':
                goto end;
                break;
            case KEY_UP:
                if (row > 0) --row;
                break;
            case KEY_DOWN:
                if (row < nrows) ++row;
                break;
            case KEY_LEFT:
                if (col > 0) --col;
                break;
            case KEY_RIGHT:
                if (col < nclos) ++col;
        }
        mvprintw(row,col,"Z");


        refresh();

    }
    end:
    mvprintw(nrows/2,nclos/2, "ZZZVVV");


    endwin();
    return 0;

}
