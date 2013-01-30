#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <string.h>


struct ppu 
{
    ppu() {
    name_tbl_addy = 0;
    nm_tbl_offset = 0;
    attr_tbl_addy =0;
    attr_tbl_offset = 0;
        cycle = 0;
    //    scanline = 0;
      //  scrollUpdate();
        scanline = -1;
        //STATUS |= 0x80;
        frame_count = 0;
        vblanktime = 20 * 341 * 5; // NTSC
//        bg_palette_addy = 0x3F01;
 //       sprite_palette_addy = 0x3F11;
  //      resolution = 260*240;
        write_latch = true;
        fine_horiz_offset = 0;
    coarse_horiz_offset = 0;
    fine_vert_offset = 0;
    coarse_vert_offset = 0;

        writeCtrl(0x00);




    }
    void scrollUpdate();
    void incrementAddresses(unsigned int deltaX);
    unsigned int  name_tbl_addy ;
    unsigned int nm_tbl_offset ;
    unsigned int attr_tbl_addy ;
    unsigned int attr_tbl_offset ;
    unsigned int fine_horiz_offset;
    unsigned int coarse_horiz_offset;
    unsigned int fine_vert_offset;
    unsigned int coarse_vert_offset;

// 8 ppu memory mapped registers

    uint8_t CTRL;    //$2000
    uint8_t MASK;    //$2001
    uint8_t STATUS;  //$2002
    uint8_t OAMADDR; //  .
    uint8_t OAMDATA; //  .
    uint8_t SCROLL;  //  .
    uint16_t ADDR;    //  .
    uint8_t DATA;    //$2007
    
    int addr_write_count;

    //CTRL
    uint16_t base_nametable_address;
    uint16_t base_attr_tbl_addy;
    unsigned int vram_address_inc;
    uint16_t sprite_pattern;
    uint16_t background_pattern;
    bool sprite_size;
    bool generate_nmi;


    //MASK
    bool greyscale;
    bool show_background_left;
    bool show_sprites_left;
    bool show_background;
    bool show_sprites;
    bool intensify_red;
    bool intensify_green;
    bool intensify_blue;

    //STATUS
    bool sprite_overflow;
    bool sprite_0_hit;
    bool vblank;


    // SPRITE RAM
    /*

        stores 4 bytes for 64 sprites so 256 total bytes
        OAM == sprite_ram ???its very possible
        Byte 0: y pos of top of sprite
        byte 1: tile index number
                8x8 sprites : tile number within pattern table selected by CTRL register
                8x16 sprites :    76543210
                                  ||||||||
                                  |||||||+- Bank ($0000 or $1000) of tiles
                                  +++++++-- Tile number of top of sprite (0 to 254; bottom half gets the next tile)
        Byte 2:
                Attribute
                    76543210
                    ||||||||
                    ||||||++- Palette (4 to 7) of sprite
                    |||+++--- Unimplemented
                    ||+------ Priority (0: in front of background; 1: behind background)
                    |+------- Flip sprite horizontally
                    +-------- Flip sprite vertically
        Byte 3:
                X pos of left side , not wrapped around
    */

    uint8_t sprite_ram[0x100];
    uint8_t secondary_sprite_ram[64];
    uint8_t shift_regs[16];
    uint8_t latches[8];
    uint8_t counters[8];
    uint8_t sprite_ram_addy;




    //all tiles are 8x8 pixel 
    //usually 2 nametables though space for 4
    // other two are usually mirrored
    // each nametable is 960 bytes
    //th bg is 30 X 32 tiles or 256 X240 pixels
    //each nametable has a paired attribute table
    uint8_t * chr_rom[2]; // pionters to char_ram 
                          //$0000 - $1000 in chr_ram[0]
                          //$1000 - $2000 in chr_ram[1]

    uint8_t nametable_rom[0x1000]; //$2000 - $3000

    uint8_t palettes[0x20];

                          //$0000 - $0FFF sprite tiles
                          //$1000 - $1FFF Background tiles
                          //$2000 - $2FFF Name & attribute tables
                          //$3000 - $3FFF Palettes
    //PALETTES
    // each color is one byte
    // each palette has three colors
    // each color's byte is such:
    //   76543210
    //   ||||||||
    //   ||||++++- Hue (phase)
    //   ||++----- Value (voltage)
    //   ++------- Unimplemented, reads back as 0
    //
    //
    // $3F00 is the universal bg colour
                                            // 1st palette is $3F01 - $3F03
                                            // 2nd is         $3F05 - $3F07
                                            // 4 total palettes
    static const unsigned int bg_palette_addy = 0x3F00; // until $3F0F

    static const unsigned int sprite_palette_addy = 0x3F10;
                                            // 1st palette is $3F11 - $3F13 , 2nd is $3F15 - $3F17 etc., 4 total , until $3F1F
    static const unsigned int universal_bg_color = 0x3F00;

    //uint8_t *bg_ram = &vram[0x1000];
    //uint8_t *nametable_ram = &vram[0x2000];
    //uint8_t *palette_ram = &vram[0x3000];
    uint16_t bg_shift_16[2];
    uint8_t  bg_shift_8[2];
    int cycle;
    int scanline;
    int vblanktime;
    int frame_count;
    int frame_cycles;
    static const unsigned int resolution = 256 * 240;
    uint8_t nes_framebuffer [resolution]; //resolution correct?
    uint8_t rgb_framebuffer [resolution * 3];

    void renderBG();

    void convertFramebuffer();
    void step();
    void render();
    void writeStatus (uint8_t word);
    void writeMask   (uint8_t word);
    void writeCtrl   (uint8_t word);
    void writeOAMAddress    (uint8_t word);
    void writeOAMData(uint8_t word);
    uint8_t readOAMData();
    void writeDMA    (uint8_t word);
    void writeScroll (uint8_t word);
    uint8_t readStatus();

    uint8_t readVram(uint16_t address);
    void writeVram(uint8_t value, uint16_t address);
    
    uint16_t vram_latch;
    bool write_latch;
    void writeData(uint8_t word);
    uint8_t readData();
    void writeAddr(uint8_t word);
    void renderSprites();

    void nameTableViewer(unsigned int);
private:
    void setVblank(bool);
    void setSpriteOverflow(bool status);
    void setSprite0Hit(bool status);
    void updateEndScanLine();
    void getColors(uint8_t * colors , uint16_t pattern_table_addy, unsigned int palette_choice_bits, bool sprite);

};


#endif
