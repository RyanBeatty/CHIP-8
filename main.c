#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>


typedef struct {
    uint8_t* data;
    size_t size;
} Rom;

void RomInit(Rom* rom, const char* filename) {
    assert(rom != NULL);
    int rom_fd = open(filename, O_RDONLY);
    if (rom_fd == -1) {
        char* err_msg = (char*)calloc(strlen("open ") + strlen(filename) + 1, sizeof(char));
        strcat(err_msg, "open ");
        strcat(err_msg, filename);
        perror(err_msg);
        exit(EXIT_FAILURE);
    }
    
    off_t nseek = lseek(rom_fd, 0L, SEEK_END);
    if (nseek == -1) {
        perror("lseek end");
        exit(EXIT_FAILURE);
    }

    if (lseek(rom_fd, 0L, SEEK_SET) == -1) {
        perror("lseek start");
        exit(EXIT_FAILURE);
    }

    rom->data = calloc(nseek, sizeof(uint8_t));
    rom->size = nseek;
    ssize_t nread = read(rom_fd, rom->data, nseek);
    if (nread == -1) {
        perror("read rom");
        exit(EXIT_FAILURE);
    }

    if (close(rom_fd) == -1) {
        perror("close rom");
        exit(EXIT_FAILURE);
    }
    return ;
}

#define NUM_RAM 4096
uint8_t ram[NUM_RAM];

#define NUM_REG 16
uint8_t reg[NUM_REG];
uint16_t reg_i;
uint8_t delay_reg;
uint8_t sound_reg;

uint16_t pc;
uint8_t sp;
#define NUM_STACK 16
uint16_t stack[NUM_STACK];

void InitRegisters() {
    memset(reg, 0, NUM_REG);
    reg_i = 0;
    delay_reg = 0;
    sound_reg = 0;
    pc = 0x200;
    sp = 0;
    memset(stack, 0, NUM_STACK);
    return;
}

void EmulateCycle() {}

void Draw() {}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: main <rom-filename>\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    
    const char* rom_filename = argv[1];
    Rom rom;
    RomInit(&rom, rom_filename);

    memset(ram, 0, NUM_RAM);
    memcpy(&ram[0x200], rom.data, rom.size);
    InitRegisters();

    for (;;) {
        EmulateCycle();
        Draw();
    }

    exit(EXIT_SUCCESS);
}
