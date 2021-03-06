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
#define RESOLUTION_WIDTH 64
#define RESOLUTION_HEIGHT 32

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
#define MAX_SPRITE_SIZE_BYTES 15

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
        0b00100000,
        0b01100000,
        0b00100000,
        0b00100000,
        0b01110000,
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

bool logical_pixels[RESOLUTION_HEIGHT][RESOLUTION_WIDTH];

typedef enum {
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_UNKNOWN
} Key;

#define NUM_KEYS 17

bool keys[NUM_KEYS];

Key ReadInput();


void InitCHIP8() {
    memset(registers, 0, NUM_REG);
    reg_i = 0;
    delay_reg = 0;
    sound_reg = 0;
    pc = 0x200;  // End of reserved mem.
    sp = 0; 
    memset(ram, 0, NUM_RAM);
    memset(stack, 0, NUM_STACK * sizeof(uint16_t));
    memcpy(ram, fonts, NUM_FONTS * FONT_SIZE * sizeof(uint8_t));
    return ;
}

void EmulateCycle() {
    // CHIP-8 instructions are big endian encoded.
    uint16_t instruction = ram[pc + 1] << 0 | ram[pc] << 8;
    printf("pc: %d; instruction: 0x%04" PRIx16 "\n", pc, instruction);
    switch (instruction & 0xF000) {
        case 0x0000: {
            switch (instruction) {
                case 0x00E0: {
                    memset(logical_pixels, false, RESOLUTION_WIDTH * RESOLUTION_HEIGHT * sizeof(bool));
                    pc += 2;
                    DPRINT("CLS\n");
                    break;
                }
                case 0x00EE: {
                    // TODO: Is this the right order of operations?
                    assert(sp > 0);
                    --sp;
                    pc = stack[sp] + 2;  // Need to increment so I'm not stuck in an infinite loop?
                    DPRINT("RET\n");
                    break;
                }
                default: {
                    uint16_t addr = instruction & 0x0FFF;
                    DPRINT("SYS %d\n", addr);
                    assert(false);
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
            // TODO: Is this the right order of operations?
            uint16_t addr = instruction & 0x0FFF;
            assert(sp < NUM_STACK);
            stack[sp] = pc;
            ++sp;
            pc = addr;
            DPRINT("CALL %d\n", addr);
            break;
        }
        case 0x3000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            assert(reg < NUM_REG);
            uint8_t val = instruction & 0x00FF;
            pc += registers[reg] == val ? 4 : 2;
            DPRINT("SE V%d, %d\n", reg, val);
            break;
        }
        case 0x4000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            assert(reg < NUM_REG);
            uint8_t val = instruction & 0x00FF;
            pc += registers[reg] != val ? 4 : 2;
            DPRINT("SNE V%d, %d\n", reg, val);
            break;
        }
        case 0x5000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            assert(lreg < NUM_REG);
            assert(rreg < NUM_REG);
            pc += registers[lreg] == registers[rreg] ? 4 : 2;
            DPRINT("SE V%d, V%d\n", lreg, rreg);
            break;
        }
        case 0x6000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            assert(reg < NUM_REG);
            uint8_t val = instruction & 0x00FF;
            registers[reg] = val;
            pc += 2;
            DPRINT("LD V%d, %d\n", reg, val);
            break;
        }
        case 0x7000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            assert(reg < NUM_REG);
            uint8_t val = instruction & 0x00FF;
            registers[reg] += val;
            pc += 2;
            DPRINT("ADD V%d, %d\n", reg, val);
            break;
        }
        case 0x8000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            assert(lreg < NUM_REG);
            assert(rreg < NUM_REG);
            switch (instruction & 0x000F) {
                case 0x0000: {
                    registers[lreg] = registers[rreg];
                    pc += 2;
                    DPRINT("LD V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0001: {
                    registers[lreg] |= registers[rreg];
                    pc += 2;
                    DPRINT("OR V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0002: {
                    registers[lreg] &= registers[rreg];
                    pc += 2;
                    DPRINT("AND V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0003: {
                    registers[lreg] ^= registers[rreg];
                    pc += 2;
                    DPRINT("XOR V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0004: {
                    registers[VF] = 255 - registers[lreg] < registers[rreg] ? 1 : 0;
                    registers[lreg] += registers[rreg];
                    pc += 2;
                    DPRINT("ADD V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0005: {
                    registers[VF] = registers[lreg] > registers[rreg] ? 1 : 0;
                    registers[lreg] -= registers[rreg];
                    pc += 2;
                    DPRINT("SUB V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x0006: {
                    registers[VF] = registers[lreg] & 0x01;
                    registers[lreg] >>= 1;
                    pc += 2;
                    DPRINT("SHR V%d {, V%d}\n", lreg, rreg);
                    break;
                }
                case 0x0007: {
                    registers[VF] = registers[rreg] > registers[lreg] ? 1 : 0;
                    registers[lreg] = registers[rreg] - registers[lreg];
                    pc += 2;
                    DPRINT("SUBN V%d, V%d\n", lreg, rreg);
                    break;
                }
                case 0x000E: {
                    registers[VF] = (registers[lreg] & 0x80) >> 7;
                    registers[lreg] <<= 1;
                    pc += 2;
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
            assert(lreg < NUM_REG);
            assert(rreg < NUM_REG);
            pc += registers[lreg] != registers[rreg] ? 4 : 2;
            DPRINT("SNE V%d, V%d\n", lreg, rreg);
            break;
        }
        case 0xA000: {
            uint16_t addr = instruction & 0x0FFF;
            reg_i = addr;
            pc += 2;
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
            assert(reg < NUM_REG);
            uint8_t val = instruction & 0x00FF;
            registers[reg] = (rand() % 255) & val;
            pc += 2;
            DPRINT("RND V%d, %d\n", reg, val);
            break;
        }
        case 0xD000: {
            uint8_t lreg = (instruction & 0x0F00) >> 8;
            uint8_t rreg = (instruction & 0x00F0) >> 4;
            assert(lreg < NUM_REG);
            assert(rreg < NUM_REG);
            uint8_t nbytes = instruction & 0x000F;
            assert (nbytes <= MAX_SPRITE_SIZE_BYTES);
            registers[VF] = 0;
            for (size_t i = reg_i, y = registers[rreg] % RESOLUTION_HEIGHT; i < reg_i + nbytes; ++i, y = (y + 1) % RESOLUTION_HEIGHT) {
                uint8_t sprite_byte = ram[i];
                for (uint8_t pos = 8, x = registers[lreg] % RESOLUTION_WIDTH; pos > 0; --pos, x = (x + 1) % RESOLUTION_WIDTH) {
                    bool signal = (sprite_byte >> (pos - 1)) & 0x01;
                    bool result = logical_pixels[y][x] ^ signal;
                    registers[VF] = registers[VF] || (logical_pixels[y][x] && !result) ? 1 : 0;
                    logical_pixels[y][x] = result; 
                }
            }
            pc += 2;
            DPRINT("DRW V%d, V%d, %d\n", lreg, rreg, nbytes);
            break;
        }
        case 0xE000: {
            uint8_t reg = (instruction & 0x0F00) >> 8;
            assert(reg < NUM_REG);
            switch (instruction & 0x00FF) {
                case 0x009E: {
                    pc += keys[registers[reg]] ? 4 : 2;
                    DPRINT("SKP V%d\n", reg);
                    break;
                }
                case 0x00A1: {
                    pc += !keys[registers[reg]] ? 4 : 2;
                    DPRINT("SKNP V%d\n", reg);
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
                    pc += 2;
                    DPRINT("LD V%d, DT\n", reg);
                    break;
                }
                case 0x000A: {
                    Key key = ReadInput();
                    while (key == KEY_UNKNOWN) {
                        key = ReadInput();
                    }
                    registers[reg] = key;
                    DPRINT("LD V%d, K\n", reg);
                    break;
                }
                case 0x0015: {
                    delay_reg = registers[reg];
                    pc += 2;
                    DPRINT("LD DT, V%d\n", reg);
                    break;
                }
                case 0x0018: {
                    sound_reg = registers[reg];
                    pc += 2;
                    DPRINT("LD ST, V%d\n", reg);
                    break;
                }
                case 0x001E: {
                    reg_i += registers[reg];
                    pc += 2;
                    DPRINT("ADD I, V%d\n", reg);
                    break;
                }
                case 0x0029: {
                    uint8_t font_idx = registers[reg];
                    assert(font_idx < NUM_FONTS);
                    reg_i = font_idx * FONT_SIZE;
                    pc += 2;
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
                    pc += 2;
                    DPRINT("LD B, V%d\n", reg);
                    break;
                }
                case 0x0055: {
                    uint16_t addr = reg_i;
                    // TODO: implement as memcpy.
                    for (uint8_t i = 0; i <= reg; ++i, ++addr) {
                        ram[addr] = registers[i];
                    }
                    pc += 2;
                    DPRINT("LD [I], V%d\n", reg);
                    break;
                }
                case 0x0065: {
                    uint16_t addr = reg_i;
                    for (uint8_t i = 0; i <= reg; ++i, ++addr) {
                        registers[i] = ram[addr];
                    }
                    pc += 2;
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
    // This is hacky for the moment. I should tie this to the framterate/clock somehow.
    if (delay_reg > 0) {
        --delay_reg;
    }
    return ;
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

    return ;
}

typedef uint32_t Pixel;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
Pixel pixels[RESOLUTION_WIDTH * RESOLUTION_HEIGHT];


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

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, RESOLUTION_WIDTH, RESOLUTION_HEIGHT);
    if (texture == NULL) {
        fprintf(stderr, "Texture could not be created! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    DPRINT("type: %d\n", SDL_PIXELTYPE(SDL_PIXELFORMAT_ABGR8888));
    DPRINT("order: %d\n", SDL_PIXELORDER(SDL_PIXELFORMAT_ABGR8888));
    DPRINT("layout: %d\n", SDL_PIXELLAYOUT(SDL_PIXELFORMAT_ABGR8888));
    DPRINT("bytes/pixel: %d\n", SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_ABGR8888));
    return ;
}

void Render() {
    // Translate logical pixels to actual pixels displayed on screen.
    for (size_t y = 0; y < RESOLUTION_HEIGHT; ++y) {
        for (size_t x = 0; x < RESOLUTION_WIDTH; ++x) {
            pixels[(y * RESOLUTION_WIDTH) + x] = logical_pixels[y][x] ? 0x00FFFFFF : 0x00000000;
        }
    }
    if (SDL_UpdateTexture(texture, NULL, pixels, RESOLUTION_WIDTH * sizeof(Pixel)) < 0) {
        fprintf(stderr, "Failed to update texture! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (SDL_RenderClear(renderer) < 0) {
        fprintf(stderr, "Failed to clear renderer! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    if (SDL_RenderCopy(renderer, texture, NULL, NULL) < 0) {
        fprintf(stderr, "Failed to render copy! SDL Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_RenderPresent(renderer);
    return ;
}

Key MapKeycode(SDL_Keycode code) {
    Key key = KEY_UNKNOWN;
    switch (code) {
        case SDLK_1: {
            key = KEY_1;
            break;
        }
        case SDLK_2: {
            key = KEY_2;
            break;    
        }
        case SDLK_3: {
            key = KEY_3;
            break;
        }
        case SDLK_4: {
            key = KEY_C;
            break;
        }
        case SDLK_q: {
            key = KEY_4;
            break;
        }
        case SDLK_w: {
            key = KEY_5;
            break;
        }
        case SDLK_e: {
            key = KEY_6;
            break;
        }
        case SDLK_r: {
            key = KEY_D;
            break;
        }
        case SDLK_a: {
            key = KEY_7;
            break;
        }
        case SDLK_s: {
            key = KEY_8;
            break;
        }
        case SDLK_d: {
            key = KEY_9;
            break;
        }
        case SDLK_f: {
            key = KEY_E;
            break;
        }
        case SDLK_z: {
            key = KEY_A;
            break;
        }
        case SDLK_x: {
            key = KEY_0;
            break;
        }
        case SDLK_c: {
            key = KEY_B;
            break;
        }
        case SDLK_v: {
            key = KEY_F;
            break;
        }
        default: {
            fprintf(stderr, "Uknown keycode: %d\n", code);
            fflush(stderr);
            key = KEY_UNKNOWN;
            // exit(EXIT_FAILURE);
        }
    }

    return key;
}

Key ReadInput() {
    SDL_Event e;
    Key last_key = KEY_UNKNOWN;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            exit(EXIT_SUCCESS);
        }
        Key key = MapKeycode(e.key.keysym.sym);
        keys[key] = e.type == SDL_KEYDOWN;
        last_key = e.type == SDL_KEYDOWN ? key : last_key;
    }

    return last_key;
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
    InitGraphics();

    for (;;) {
        ReadInput();
        EmulateCycle();
        Render();
        usleep(1200);
    }

    SDL_Quit();

    exit(EXIT_SUCCESS);
}
