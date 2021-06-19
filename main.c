#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <inttypes.h>


// // Debug 
// #ifdef DEBUG
//     #define DPRINT(fmt, args...) printf(fmt, ##args)
// #else
//     #define DPRINT(fmt, args...)

#define NUM_RAM 4096
#define NUM_REG 16
#define NUM_STACK 16

uint8_t ram[NUM_RAM];
uint16_t stack[NUM_STACK];


uint8_t reg[NUM_REG];
uint16_t reg_i;
uint8_t delay_reg;
uint8_t sound_reg;

uint16_t pc;
uint8_t sp;

void InitCHIP8() {
    memset(reg, 0, NUM_REG);
    reg_i = 0;
    delay_reg = 0;
    sound_reg = 0;
    pc = 0x200;  // End of reserved mem.
    sp = 0; 
    memset(ram, 0, NUM_RAM);
    memset(stack, 0, NUM_STACK * sizeof(uint16_t));
}

void EmulateCycle() {
    uint16_t instruction = ram[pc] | (ram[pc + 1] << 8);
    printf("0x%04" PRIx16 ": ", instruction);
    switch (instruction & 0xF000) {
        case 0x0000: {
            switch (instruction) {
                case 0x00E0: {
                    printf("CLS\n");
                    break;
                }
                case 0x00EE: {
                    printf("RET\n");
                    break;
                }
                default: {
                    uint16_t addr = instruction & 0x0FFF;
                    printf("SYS %d\n", addr);
                }
            }
            break;
        }
        case 0x1000: {
            uint16_t addr = instruction & 0x0FFF;
            printf("JP %d\n", addr);
            break;
        }
        case 0x2000: {
            uint16_t addr = instruction & 0x0FFF;
            printf("CAL %d\n", addr);
            break;
        }
        case 0x3000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            printf("SE V%d, %d\n", reg, val);
            break;
        }
        case 0x4000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            printf("SNE V%d, %d\n", reg, val);
            break;
        }
        case 0x5000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            printf("SE V%d, V%d\n", lreg, rreg);
            break;
        }
        case 0x6000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            printf("LD V%d, %d\n", reg, val);
            break;
        }
        case 0x7000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            printf("ADD V%d, %d\n", reg, val);
            break;
        }
        case 0x8000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            switch (instruction & 0x000F) {
                case 0x0000: {
                    printf("LD V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0001: {
                    printf("OR V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0002: {
                    printf("AND V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0003: {
                    printf("XOR V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0004: {
                    printf("ADD V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0005: {
                    printf("SUB V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0006: {
                    printf("SHR V%d {, V%d}\n", lreg, rreg);
                    break;
                }
                case 0x0007: {
                    printf("SUBN V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x000E: {
                    printf("SHL V%d {, V%d}\n", lreg, rreg);
                    break;
                }
                default: {
                    fprintf(stderr, "unknown 0x80 instruction: %04" PRIx16 "\n", instruction);
                    fflush(stderr);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }
        case 0x9000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            printf("SNE V%d, V%d\n", lreg, rreg);
            break;
        }
        case 0xA000: {
            uint16_t addr = instruction & 0x0FFF;
            printf("LD I, %d\n", addr);
            break;
        }
        case 0xB000: {
            uint16_t addr = instruction & 0x0FFF;
            printf("JP V0, %d\n", addr);
            break;
        }
        case 0xC000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            printf("RND V%d, %d\n", reg, val);
            break;
        }
        case 0xD000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            uint8_t nbytes = instruction & 0x000F;
            printf("DRW V%d, V%d, %d\n", lreg, rreg, nbytes);
            break;
        }
        case 0xE000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            switch (instruction & 0x00FF) {
                case 0x009E: {
                    printf("SKP V%d\n", reg);
                    break;
                }
                case 0x00A1: {
                    printf("SKNP V%d\n", reg);
                    break;
                }
                default: {
                    fprintf(stderr, "unknown 0xE0 instruction: %04" PRIx16 "\n", instruction);
                    fflush(stderr);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }
        case 0xF000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            switch (instruction & 0x00FF) {
                case 0x0007: {
                    printf("LD V%d, DT\n", reg);
                    break;
                }
                case 0x000A: {
                    printf("LD V%d, K\n", reg);
                    break;
                }
                case 0x0015: {
                    printf("LD DT, V%d\n", reg);
                    break;
                }
                case 0x0018: {
                    printf("LD ST, V%d\n", reg);
                    break;
                }
                case 0x001E: {
                    printf("ADD I, V%d\n", reg);
                    break;
                }
                case 0x0029: {
                    printf("LD F, V%d\n", reg);
                    break;
                }
                case 0x0033: {
                    printf("LD B, V%d\n", reg);
                    break;
                }
                case 0x0055: {
                    printf("LD [I], V%d\n", reg);
                    break;
                }
                case 0x0065: {
                    printf("LD V%d, [I]\n", reg);
                    break;
                }
                default: {
                    fprintf(stderr, "unknown 0xF0 instruction: %04" PRIx16 "\n", instruction);
                    fflush(stderr);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }
        default: {
            fprintf(stderr, "unknown instruction: %04" PRIx16 "\n", instruction);
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
    }
}

void Draw() {}



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

    // CHIP-8 instructions are stored big endian, so we convert the ROM to host byte order.
    for (size_t i = 0; i < rom->size; i += 2) {
        uint16_t instruction = (rom->data[i] << 0) | (rom->data[i+1] << 8);
        instruction = ntohs(instruction);
        memcpy(&rom->data[i], &instruction, 2); 
    }
    return ;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: main <rom-filename>\n");
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    
    const char* rom_filename = argv[1];
    Rom rom;
    RomInit(&rom, rom_filename);

    InitCHIP8();
    memcpy(&ram[0x200], rom.data, rom.size);

    for (;;) {
        EmulateCycle();
        Draw();
    }

    exit(EXIT_SUCCESS);
}
