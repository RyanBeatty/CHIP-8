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
#include <SDL.h>
#include <stdbool.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#ifndef DEBUG
    #define DEBUG 0
#endif

// Debug printing macro function.
// https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
#define DPRINT(...) \
    do { if (DEBUG) printf(__VA_ARGS__); } while (0)

#define NUM_RAM 4096
#define NUM_REG 16
#define NUM_STACK 16
#define VF 15
#define NUM_FONTS 16
#define FONT_SIZE 5

uint8_t ram[NUM_RAM];
uint16_t stack[NUM_STACK];


uint8_t registers[NUM_REG];
uint16_t reg_i;
uint8_t delay_reg;
uint8_t sound_reg;

uint16_t pc;
uint8_t sp;

uint8_t fonts[NUM_FONTS][FONT_SIZE] = {
    {
        0b11110000,
        0b10010000,
        0b10010000,
        0b10010000,
        0b11110000,
    },
    {
        0b01000000,
        0b11000000,
        0b01000000,
        0b01000000,
        0b11110000,
    },
    {
        0b11110000,
        0b00010000,
        0b11110000,
        0b10000000,
        0b11110000,
    },
    {
        0b11110000,
        0b00010000,
        0b11110000,
        0b00010000,
        0b11110000,
    },
    {
        0b10010000,
        0b10010000,
        0b11110000,
        0b00010000,
        0b00010000,
    },
    {
        0b11110000,
        0b10000000,
        0b11110000,
        0b00010000,
        0b11110000,
    },
    {
        0b11110000,
        0b10000000,
        0b11110000,
        0b10010000,
        0b11110000,
    },
    {
        0b11110000,
        0b00010000,
        0b00100000,
        0b01000000,
        0b01000000,
    },
    {
        0b11110000,
        0b10010000,
        0b11110000,
        0b10010000,
        0b11110000,
    },
    {
        0b11110000,
        0b10010000,
        0b11110000,
        0b00010000,
        0b11110000,
    },
    {
        0b11110000,
        0b10010000,
        0b11110000,
        0b10010000,
        0b10010000,
    },
    {
        0b11100000,
        0b10010000,
        0b11100000,
        0b10010000,
        0b11100000,
    },
    {
        0b11110000,
        0b10000000,
        0b10000000,
        0b10000000,
        0b11110000,
    },
    {
        0b11100000,
        0b10010000,
        0b10010000,
        0b10010000,
        0b11100000,
    },
    {
        0b11110000,
        0b10000000,
        0b11110000,
        0b10000000,
        0b11110000,
    },
    {
        0b11110000,
        0b10000000,
        0b11110000,
        0b10000000,
        0b10000000,
    }
};


void InitCHIP8() {
    memset(registers, 0, NUM_REG);
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
    DPRINT("0x%04" PRIx16 ": ", instruction);
    switch (instruction & 0xF000) {
        case 0x0000: {
            switch (instruction) {
                case 0x00E0: {
                    DPRINT("CLS\n");
                    assert(false);
                    break;
                }
                case 0x00EE: {
                    DPRINT("RET\n");
                    assert(false);
                    break;
                }
                default: {
                    uint16_t addr = instruction & 0x0FFF;
                    DPRINT("SYS %d\n", addr);
                }
            }
            break;
        }
        case 0x1000: {
            uint16_t addr = instruction & 0x0FFF;
            pc = addr;
            DPRINT("JP %d\n", addr);
            break;
        }
        case 0x2000: {
            uint16_t addr = instruction & 0x0FFF;
            // Is this the right order of operations?
            stack[sp] = pc;
            ++sp;
            pc = addr;
            DPRINT("CALL %d\n", addr);
            break;
        }
        case 0x3000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            pc += registers[reg] == val ? 2 : 1;
            DPRINT("SE V%d, %d\n", reg, val);
            break;
        }
        case 0x4000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            pc += registers[reg] != val ? 2 : 1;
            DPRINT("SNE V%d, %d\n", reg, val);
            break;
        }
        case 0x5000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            pc += registers[lreg] == registers[rreg] ? 2 : 1;
            DPRINT("SE V%d, V%d\n", lreg, rreg);
            break;
        }
        case 0x6000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            registers[reg] = val;
            ++pc;
            DPRINT("LD V%d, %d\n", reg, val);
            break;
        }
        case 0x7000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            registers[reg] += val;
            ++pc;
            DPRINT("ADD V%d, %d\n", reg, val);
            break;
        }
        case 0x8000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            switch (instruction & 0x000F) {
                case 0x0000: {
                    registers[lreg] = registers[rreg];
                    ++pc;
                    DPRINT("LD V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0001: {
                    registers[lreg] |= registers[rreg];
                    DPRINT("OR V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0002: {
                    registers[lreg] &= registers[rreg];
                    DPRINT("AND V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0003: {
                    registers[lreg] ^= registers[rreg];
                    DPRINT("XOR V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0004: {
                    registers[VF] = 255 - registers[lreg] < registers[rreg] ? 1 : 0;
                    registers[lreg] += registers[rreg];
                    ++pc;
                    DPRINT("ADD V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0005: {
                    registers[VF] = registers[lreg] > registers[rreg] ? 1 : 0;
                    registers[lreg] -= registers[rreg];
                    ++pc;
                    DPRINT("SUB V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0006: {
                    registers[VF] = registers[lreg] & 0x01;
                    registers[lreg] /= 2;
                    ++pc;
                    DPRINT("SHR V%d {, V%d}\n", lreg, rreg);
                    break;
                }
                case 0x0007: {
                    registers[VF] = registers[rreg] > registers[lreg] ? 1 : 0;
                    registers[lreg] = registers[rreg] - registers[lreg];
                    ++pc;
                    DPRINT("SUBN V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x000E: {
                    registers[VF] = registers[lreg] & 0x80;
                    registers[lreg] *= 2;
                    ++pc;
                    DPRINT("SHL V%d {, V%d}\n", lreg, rreg);
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
            pc += registers[lreg] != registers[rreg] ? 2 : 1;
            DPRINT("SNE V%d, V%d\n", lreg, rreg);
            break;
        }
        case 0xA000: {
            uint16_t addr = instruction & 0x0FFF;
            reg_i = addr;
            ++pc;
            DPRINT("LD I, %d\n", addr);
            break;
        }
        case 0xB000: {
            uint16_t addr = instruction & 0x0FFF;
            pc = addr + registers[0];
            DPRINT("JP V0, %d\n", addr);
            break;
        }
        case 0xC000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            uint8_t val = instruction & 0x00FF;
            registers[reg] = (rand() % 255) & val;
            ++pc;
            DPRINT("RND V%d, %d\n", reg, val);
            break;
        }
        case 0xD000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            uint8_t nbytes = instruction & 0x000F;
            DPRINT("DRW V%d, V%d, %d\n", lreg, rreg, nbytes);
            assert(false);
            break;
        }
        case 0xE000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            switch (instruction & 0x00FF) {
                case 0x009E: {
                    DPRINT("SKP V%d\n", reg);
                    assert(false);
                    break;
                }
                case 0x00A1: {
                    DPRINT("SKNP V%d\n", reg);
                    assert(false);
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
            assert(reg < NUM_REG);
            switch (instruction & 0x00FF) {
                case 0x0007: {
                    registers[reg] = delay_reg;
                    ++pc;
                    DPRINT("LD V%d, DT\n", reg);
                    break;
                }
                case 0x000A: {
                    DPRINT("LD V%d, K\n", reg);
                    assert(false);
                    break;
                }
                case 0x0015: {
                    delay_reg = registers[reg];
                    ++pc;
                    DPRINT("LD DT, V%d\n", reg);
                    break;
                }
                case 0x0018: {
                    sound_reg = registers[reg];
                    ++pc;
                    DPRINT("LD ST, V%d\n", reg);
                    break;
                }
                case 0x001E: {
                    reg_i += registers[reg];
                    ++pc;
                    DPRINT("ADD I, V%d\n", reg);
                    break;
                }
                case 0x0029: {
                    uint8_t font_idx = registers[reg];
                    assert(font_idx < NUM_FONTS);
                    reg_i = fonts[font_idx];
                    ++pc;
                    DPRINT("LD F, V%d\n", reg);
                    break;
                }
                case 0x0033: {
                    uint8_t val = registers[reg];
                    ram[reg_i + 2] = val % 10;
                    val /= 10;
                    ram[reg_i + 1] = val % 10;
                    val /= 10;
                    ram[reg_i] = val;
                    ++pc;
                    DPRINT("LD B, V%d\n", reg);
                    break;
                }
                case 0x0055: {
                    uint16_t addr = reg_i;
                    // TODO: implement as memcpy.
                    for (uint8_t i = 0; i <= reg; ++i, ++addr) {
                        ram[addr] = registers[i];
                    }
                    ++pc;
                    DPRINT("LD [I], V%d\n", reg);
                    break;
                }
                case 0x0065: {
                    uint16_t addr = reg_i;
                    for (uint8_t i = 0; i <= reg; ++i, ++addr) {
                        registers[i] = ram[addr];
                    }
                    ++pc;
                    DPRINT("LD V%d, [I]\n", reg);
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

typedef uint32_t Pixel;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
Pixel pixels[SCREEN_WIDTH * SCREEN_HEIGHT];


void InitGraphics() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if( window == NULL ){
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if (SDL_RenderSetLogicalSize(renderer, 64, 32) < 0) {
        fprintf(stderr, "Failed to set renderer logical size! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (texture == NULL) {
        fprintf(stderr, "Texture could not be created! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    DPRINT("type: %d\n", SDL_PIXELTYPE(SDL_PIXELFORMAT_ABGR8888));
    DPRINT("order: %d\n", SDL_PIXELORDER(SDL_PIXELFORMAT_ABGR8888));
    DPRINT("layout: %d\n", SDL_PIXELLAYOUT(SDL_PIXELFORMAT_ABGR8888));
    DPRINT("bytes/pixel: %d\n", SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ABGR8888));
}

void Draw() {
    if (SDL_RenderClear(renderer) < 0) {
        fprintf(stderr, "Failed to clear renderer! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
        fprintf(stderr, "Failed to render copy! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {

    InitGraphics();

    while (true) {

        memset(pixels, 0x00FFFFFF, (SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)) / 2);
        if (SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Pixel)) < 0) {
            fprintf(stderr, "Failed to update texture! SDL Error: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        Draw();
        sleep(1);
    }

    // if (argc != 2) {
    //     fprintf(stderr, "usage: main <rom-filename>\n");
    //     fflush(stderr);
    //     exit(EXIT_FAILURE);
    // }
    
    // const char* rom_filename = argv[1];
    // Rom rom;
    // RomInit(&rom, rom_filename);

    // InitCHIP8();
    // memcpy(&ram[0x200], rom.data, rom.size);

    // for (;;) {
    //     EmulateCycle();
    //     Draw();
    // }

    SDL_Quit();

    exit(EXIT_SUCCESS);
}
