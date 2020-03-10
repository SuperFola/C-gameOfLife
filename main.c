#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define WIDTH 32
#define ARRAY_SIZE (WIDTH * WIDTH)
#define LOWER_BITS 0b00001111
#define UPPER_BITS 0b11110000
#define ALL_ALIVE  0b00010001
#define SWAP_ALIVE 0b00010000

// sleep for specified number of milliseconds
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

typedef struct Data Data;
struct Data
{
    char content[ARRAY_SIZE];
    unsigned turns;
};

// load a model from a file
Data* init_from_file(const char *filename)
{
    FILE *file;
    if (file = fopen(filename, "r"))
    {
        Data *data = (Data*) calloc(1, sizeof(Data));
        unsigned long y = 0;
        unsigned long x = 0;
        for (char c = fgetc(file); !feof(file); c = fgetc(file))
        {
            if (x > 0 && x < WIDTH - 1 && y > 0 && y < WIDTH - 1)
                data->content[y * WIDTH + x] = (c != '*') ? 0 : ALL_ALIVE;
            
            if (c == '\n')
            {
                y++;
                x = 0;
            }
            else
                x++;
        }
        fclose(file);
        return data;
    }
    else
    {
        printf("Couldn't open file %s\n", filename);
        exit(1);
    }
}

// init with a random grid
Data* init()
{
    srand((unsigned) time(NULL));

    Data *data = (Data*) calloc(1, sizeof(Data));
    for (unsigned long i = 0; i < ARRAY_SIZE; ++i)
    {
        unsigned long y = i / WIDTH;
        unsigned long x = i % WIDTH;
        char val = (x > 0 && x < WIDTH - 1 && y > 0 && y < WIDTH - 1) ? (rand() % 16) <= 1 : 0;
        data->content[i] = val | (val << 4);
    }

    return data;
}

// count neighbours of a given cell
char get_neigh(Data *data, unsigned long x, unsigned long y)
{
    return  (data->content[y       * WIDTH + (x + 1)] & LOWER_BITS) +
            (data->content[y       * WIDTH + (x - 1)] & LOWER_BITS) +
            (data->content[(y + 1) * WIDTH +  x     ] & LOWER_BITS) +
            (data->content[(y - 1) * WIDTH +  x     ] & LOWER_BITS) +

            (data->content[(y - 1) * WIDTH + (x + 1)] & LOWER_BITS) +
            (data->content[(y - 1) * WIDTH + (x - 1)] & LOWER_BITS) +
            (data->content[(y + 1) * WIDTH + (x + 1)] & LOWER_BITS) +
            (data->content[(y + 1) * WIDTH + (x - 1)] & LOWER_BITS) ;
}

char get_val(Data *data, unsigned long x, unsigned long y)
{
    return data->content[y * WIDTH + x] & LOWER_BITS;
}

void set_val(Data *data, unsigned long x, unsigned long y, char val)
{
    // only set values if we can
    if (x > 0 && x < WIDTH - 1 && y > 0 && y < WIDTH - 1)
    {
        unsigned long pos = y * WIDTH + x;
        if (val)
            data->content[pos] |= SWAP_ALIVE;
        else  // keep only the lowest 4 seven bits to zero out the swap
            data->content[pos] &= LOWER_BITS;
    }
}

// swap the temp buffer and the current buffer
void swap_vals(Data *data)
{
    for (unsigned long i = 0; i < ARRAY_SIZE; ++i)
        data->content[i] = (data->content[i] & UPPER_BITS) >> 4;
}

void display(Data *data)
{
    printf("\033[H\033[J");

    for (unsigned long i = 0; i < ARRAY_SIZE; ++i)
    {
        unsigned long y = i / WIDTH;
        unsigned long x = i % WIDTH;

        if (i != 0)
            printf((i % WIDTH) == 0 ? "\n" : " ");

        if ((x == 0 || x == WIDTH - 1 || y == 0 || y == WIDTH - 1) && (data->content[i] & LOWER_BITS) == 0)
            printf("#");
        else
            printf("%c", (data->content[i] & LOWER_BITS) > 0 ? '*' : ' ');
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    Data *data;

    if (argc == 2)
        data = init_from_file(argv[1]);
    else
        data = init();

    printf("Number of turns: ");
    scanf("%u", &data->turns);

    unsigned current = 0;
    while (current <= data->turns)
    {
        display(data);
        printf("Turn: %u/%u\n", current, data->turns);

        for (unsigned long x = 0; x < WIDTH; ++x)
        {
            for (unsigned long y = 0; y < WIDTH; ++y)
            {
                char n = get_neigh(data, x, y);
                char v = get_val(data, x, y);

                // printf("x: %lu y: %lu v: %u n: %u\n", x, y, v, n);

                if ((n == 2 || n == 3) && v == 1)
                    set_val(data, x, y, 1);
                else if (n == 3 && v == 0)
                    set_val(data, x, y, 1);
                else
                    set_val(data, x, y, 0);
            }
        }

        swap_vals(data);

        msleep(125);
        current++;
    }

    free(data);
    data = NULL;

    return EXIT_SUCCESS;
}