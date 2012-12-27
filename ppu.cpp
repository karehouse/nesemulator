#include "ppu.h"
#include <stdio.h>
#include "main.h"
#include "assert.h"
#include <pthread.h>
pthread_mutex_t framebuffer_mutex;
void ppu::render()
{


}
void ppu::setVblank(bool value)
{
    vblank = value;
    if (vblank)
    {
        STATUS |= 0x80;
    }
    else {
        STATUS &= 0x7F;
    }
}

void ppu::writeData(uint8_t word)
{
    vram[ADDR] = word;
    ADDR += vram_address_inc ;

}
uint8_t ppu::readData()
{
    uint16_t saved_addy = ADDR;
    ADDR += vram_address_inc ; 
    return vram[saved_addy]; 

}
void ppu::writeScroll (uint8_t word)
{
    if(write_latch)
    {
        vram_latch &= 0x7FE0;
        vram_latch |= (word & 0xF8) >> 3;

    } else {
        vram_latch &= 0x0C1F;
        vram_latch |= (( (uint16_t)word & 0xF8) << 2) | (((uint16_t)word & 0x07) << 12); 
    }
    write_latch = !write_latch;
}
void ppu::writeAddr(uint8_t word)
{
/*
    //CPU mem address 0x2007
    assert(addr_write_count == 0 || addr_write_count ==1);
   if ( addr_write_count == 0 ) //write lower byte
   {
        ADDR = ADDR & 0x7F00;
        ADDR = ADDR | word;
        addr_write_count = 1;
    }
    else if ( addr_write_count == 1)
    {
        ADDR = ADDR & 0x00FF;
        word = word & 0x3F; //only use lowest 6 bits b/c highest valid vram addr is 0x3fff
        ADDR = ADDR | (uint16_t)word << 8 ;
        addr_write_count = 0;
    }
    */
    if(write_latch)
    {
        vram_latch = vram_latch & 0x00FF;
        word = word & 0x3F;
        vram_latch |= (uint16_t)word << 8;
    } else
    {
        vram_latch &= 0x7F00;
        vram_latch |= word;
        ADDR = vram_latch;
    }
    write_latch = !write_latch;
}



void ppu::writeCtrl(uint8_t word)
{
    ///// Control flag
        // 7654 3210
        // |||| ||||
        // |||| ||++- Base nametable address
        // |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
        // |||| |+--- VRAM address increment per CPU read/write of PPUDATA
        // |||| |     (0: increment by 1, going across; 1: increment by 32, going down)
        // |||| +---- Sprite pattern table address for 8x8 sprites
        // ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
        // |||+------ Background pattern table address (0: $0000; 1: $1000)
        // ||+------- Sprite size (0: 8x8; 1: 8x16)
        // |+-------- PPU master/slave select (has no effect on the NES)
        // +--------- Generate an NMI at the start of the
        //            vertical blanking interval (0: off; 1: on)
    CTRL = word;
    base_nametable_address  = word & 0x03;
    base_nametable_address = (base_nametable_address << 10);
    vram_latch = (vram_latch & 0xF3FF) | (uint16_t) base_nametable_address << 10;
    base_nametable_address = 0x2000 | base_nametable_address;
    base_attr_tbl_addy = base_nametable_address + 0x3C0;
    
    
    vram_address_inc = (word & 0x04) >> 2;
    vram_address_inc = vram_address_inc ? 32 : 1;
    
    
    sprite_pattern = ((word & 0x08) >> 3) ? 0x1000 : 0x0000;
   
    
    background_pattern = ((word & 0x10) >> 4) ? 0x0000 : 0x1000;
    sprite_size = (word & 0x20) >>5;
    //generate master/slave no effect
    generate_nmi = (word & 0x80) >>7 ;
}


void ppu::writeMask  (uint8_t word)
{

    // 76543210
    // ||||||||
    // |||||||+- Grayscale (0: normal color; 1: produce a monochrome display)
    // ||||||+-- 1: Show background in leftmost 8 pixels of screen; 0: Hide
    // |||||+--- 1: Show sprites in leftmost 8 pixels of screen; 0: Hide
    // ||||+---- 1: Show background
    // |||+----- 1: Show sprites
    // ||+------ Intensify reds (and darken other colors)
    // |+------- Intensify greens (and darken other colors)
    // +-------- Intensify blues (and darken other colors)
    
    MASK = word;

    greyscale = (word & 0x01);
    show_background_left = (word & 0x02) >> 1;
    show_sprites_left = (word & 0x04) >> 2;
    show_background = (word & 0x08) >> 3;
    show_sprites = (word & 0x10) >> 4;
    intensify_red = (word & 0x20) >> 5;
    intensify_green = (word & 0x40) >> 6;
    intensify_blue = (word & 0x80) >> 7;
}


uint8_t ppu::readStatus () 
{
    uint8_t old_STATUS = STATUS;

    write_latch = true;
    setVblank(false);

    
    //ADDR = 0x0;

   
    if( cycle ==1 && scanline ==240)
    {
        return STATUS;
    }
    return old_STATUS;
    
}

void ppu::writeOAMAddress( uint8_t word)
{
   sprite_ram_addy =word;
}

void ppu::writeOAMData( uint8_t word)
{
    sprite_ram[sprite_ram_addy] = word;
    sprite_ram_addy++;
    //sprite_ram_addy %= 0x100;
    //not needed because sprite_ram_addy overflows to correct value
    //type is uint8_t

}

uint8_t readOAMData()
{
    return sprite_ram[sprite_ram_addy];
}


void ppu::writeDMA(uint8_t value)
{
    DMA_WAIT = 512;
    //pause cpu for 512 cycles

    uint16_t addy = value * 0x100;

    for(int i =0; i<0x100; i++)
    {
        sprite_ram[i] = RAM->read(addy+i);
    }

}
void ppu::renderBG()
{
    //for nametable
    // each byte represents 8x8 pixels
    // there are 30 rows of 32 tiles in the nametable
    // each tile is thus represented by one byte
    uint16_t name_tbl_addy = base_nametable_address;



    uint16_t attr_tbl_addy = base_attr_tbl_addy;

    unsigned int attr_tbl_offset = scanline/32; //each entry in attr table is for 32x32 pixel square
    attr_tbl_offset *= 8;
    attr_tbl_addy += attr_tbl_offset;





    uint16_t pattern_tbl_addy = background_pattern; //either 0x0000 or 0x1000
    uint8_t tmp_colors [8];
    for (int i=0; i< 32; i++) //for one scanline
    {
        //start at top left
        uint8_t name_tbl_entry = vram[name_tbl_addy++];
        if( i%4 == 0 && i != 0) attr_tbl_addy +=1;
        uint8_t attr_tbl_entry = vram[attr_tbl_addy];
        //ATTRIBUTE TABLE PICS WHICH PALETTE

        pattern_tbl_addy = background_pattern |  ( (0x0FF0 & ((unsigned int)name_tbl_entry <<4)) + (scanline%8));

        uint8_t low_byte = vram[pattern_tbl_addy];
        uint8_t high_byte = vram[pattern_tbl_addy + 8];


        // 
        //
        // Convert From palette to NES colour
        //
        //


        uint8_t colors[8];
        unsigned int attr_tbl_bits;
        if ( scanline % 32  < 16 )
        {
            //we are at top of attr table square


            if ( i% 32 < 16)
            {   // we are at top-left
                attr_tbl_bits = attr_tbl_entry & 0x03;

            }
            else
            {   //top-right
                attr_tbl_bits = attr_tbl_entry & 0x0C;

            }
        } else {
            //bottom
            if(i%32 < 16)
            {
                attr_tbl_bits = attr_tbl_entry & 0x30;

            } else
            {
                attr_tbl_bits = attr_tbl_entry & 0xC0;

            }



        }



        for( int j = 0 ; j < 8 ; j ++)
        {
            high_byte = ((high_byte >> (7-j)) & 0x01);
            low_byte = ((low_byte >> (7-j)) & 0x01);

            uint8_t palette_num = (high_byte << 1) + low_byte;

            uint16_t address = (bg_palette_addy & 0xff00) | (attr_tbl_bits << 2);
            address |= palette_num;
            colors[j] = vram[address];;
        }


        //
        //
        //
        //



        pthread_mutex_lock(&framebuffer_mutex);
        memcpy(nes_framebuffer + (scanline*256) + ( i*8) , tmp_colors, 8); //TODO
        // combine with sprite data???
        pthread_mutex_unlock(&framebuffer_mutex);

    }


}



void renderSprites()
{
}
void updateSprites()
{
}
void updateTiles()
{
}

void ppu::step()
{ 
   //262 scanlines per frame
  // 341 ppu clock cycles per scanline ( 1 CPU cycle = 3 PPu cycles)
    if ( scanline == -1) 
    {
        if ( cycle == 1)
        {
        }
        else if (cycle == 304)
        {
            if( show_background || show_sprites)
            {
                ADDR = vram_latch;
            }
        }
    } 
    else if (scanline >0 && scanline <= 239 )
    {
        if ( cycle ==255)
        {
//                if (show_background ) 
                {
                    renderBG();
                }

                if (show_sprites) 
                {
                    renderSprites();
                }
        }
        else if( cycle == 319)
        {
            updateSprites();
        }
        else if ( cycle == 335)
        {
            updateTiles();

        }

    }
    
    else if (scanline == 240)
    {
        if (cycle ==1)
        {
        //vblank!
        setVblank(true);
        if(ppu::generate_nmi)
        {
            
            CPU.request_nmi = true;
        }
        }
    }
    
    else if (scanline == 260 ) 
    {
        //endvblank
        if( cycle == 340 ) 
        {
            setVblank(false);
            scanline = -1;
            cycle = 0;
        }
    }
    
    
    if ( cycle == 341)
    {
        cycle = 0;
        scanline++;
    }

    cycle++;

}
const unsigned int color_palette [] = 
 {
        0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 
        0x940084, 0xA80020, 0xA81000, 0x881400, 
        0x503000, 0x007800, 0x006800, 0x005800, 
        0x004058, 0x000000, 0x000000, 0x000000, 
        0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 
        0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 
        0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 
        0x008888, 0x000000, 0x000000, 0x000000, 
        0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 
        0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 
        0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 
        0x00E8D8, 0x787878, 0x000000, 0x000000, 
        0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 
        0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 
        0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 
        0x00FCFC, 0xF8D8F8, 0x000000, 0x000000, 
    };
void ppu::convertFramebuffer()
{
    //convert from nes color space to rgb
    for ( unsigned int i = 0 ; i < resolution ;i++ )
    {
        pthread_mutex_lock(&framebuffer_mutex);
        uint32_t color = color_palette[nes_framebuffer[i]];
        pthread_mutex_unlock(&framebuffer_mutex);
        rgb_framebuffer[(3*i)] = ((color & 0xFF0000) >> 16) & 0xFF;
        rgb_framebuffer[(3*i)+1] = ((color & 0xFF00) >> 8) & 0xFF;
        rgb_framebuffer[(3*i)+2] = ((color & 0xFF));
    }
}










