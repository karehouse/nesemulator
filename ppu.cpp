#include "ppu.h"
#include <stdio.h>
#include "main.h"
#include "assert.h"
#include <pthread.h>
pthread_mutex_t framebuffer_mutex;
//64 colors
     unsigned int color_palette [] = 
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
        0x00FCFC, 0xF8D8F8, 0x000000, 0x000000, 0xFFFFFF
    };
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


void ppu::setSprite0Hit(bool status)
{

    if( status)
    {
        STATUS |= 0x40;
    } else
    {
        STATUS &= 0xBF;
    }

}
void ppu::setSpriteOverflow(bool status)
{
    if(status)
    {
        STATUS |= 0x20;
    }
    else
    {
        STATUS &= 0xDF;
    }
}

uint16_t translateAddress(uint16_t address)
{
    //nametable mirroring
    if ( address > 0x2000 && address < 0x3000)
    {
        if ( ROM->mirroring ) ///vertical
        {
            if ( address > 0x2800 ) 
            {
                address -= 0x800;
            }
        } 
        else
        { //horizontal
            if ((address >= 0x2400 && address < 0x2800) || ( address > 0x2C00))
            {
                address -= 0x400;
            }


        }

    }
    return address;
}
uint8_t ppu::readVram(uint16_t address)
{

        return vram[translateAddress(address)];

    }
void ppu::writeVram(uint8_t value, uint16_t address)
{
    vram[translateAddress(address)] = value;

}

void ppu::writeData(uint8_t word)
{
    writeVram(word, ADDR);
    ADDR += vram_address_inc ;

}
uint8_t ppu::readData()
{
    uint16_t saved_addy = ADDR;
    ADDR += vram_address_inc ; 
    return readVram(saved_addy);

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

uint8_t ppu::readOAMData()
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
bool print = false;
void ppu::renderBG()
{
    if (print)
    {
        for (int i =0; i< 256; i ++)
        {
            if ( i%64 == 0) printf("\n");
            printf("%X ", vram[base_nametable_address + i]);
        }
        print = false;
    }
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
    for (int i=0; i< 32; i++) //for one scanline
    {
        //start at top left
        uint8_t name_tbl_entry = readVram(name_tbl_addy++);
        if( i%4 == 0 && i != 0) attr_tbl_addy +=1;
        uint8_t attr_tbl_entry = readVram(attr_tbl_addy);
        //ATTRIBUTE TABLE PICS WHICH PALETTE
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
                attr_tbl_bits = (attr_tbl_entry & 0x0C) >> 2;

            }
        } else {
            //bottom
            if(i%32 < 16)
            {
                attr_tbl_bits = (attr_tbl_entry & 0x30) >> 4;

            } else
            {
                attr_tbl_bits = (attr_tbl_entry & 0xC0) >> 6;

            }



        }

        pattern_tbl_addy = background_pattern |  ( (0x0FF0 & ((unsigned int)name_tbl_entry <<4)) + (scanline%8));



        // 
        //
        // Convert From palette to NES colour
        //
        //


        uint8_t colors[8];
        getColors(colors ,pattern_tbl_addy,attr_tbl_bits , false);
        if ( scanline ==2 && i == 0)
        {   
            for (int k = 0; k< 8; k++)
            {

                //printf("colors %d  = %X \n", k, color_palette[colors[k]]);
            }
        }

        //
        //
        //
        //



          //  printf("color[0] = %X\n", colors[0]);
        //pthread_mutex_lock(&framebuffer_mutex);
        //cause bg gets rendered first we can just copy it!
        memcpy(nes_framebuffer + (scanline*256) + ( i*8) , colors, 8); //
        //printf("nes_frame = %X\n", (nes_framebuffer + (scanline * 256) + (i*8))[0]);
        // combine with sprite data???
        //pthread_mutex_unlock(&framebuffer_mutex);

    }


}

void ppu::getColors(uint8_t * colors , uint16_t pattern_table_addy, unsigned int palette_choice_bits, bool sprite)
{

        uint8_t low_byte = readVram(pattern_table_addy);
        uint8_t high_byte = readVram(pattern_table_addy + 8);
        uint16_t palette_addy = sprite ? sprite_palette_addy: bg_palette_addy;

        for( int j = 0 ; j < 8 ; j ++)
        {
            uint8_t high_bit = ((high_byte >> (7-j)) & 0x01);
            uint8_t low_bit = ((low_byte >> (7-j)) & 0x01);

            uint8_t palette_num = (high_bit << 1) + low_bit;
            uint16_t address = (palette_addy & 0xff00) | (palette_choice_bits << 2);
            address |= ( 0x0003& palette_num);
            if ( palette_num == 0) // No color / transparent
            {
                colors[j] = 64; //ony 64 colors so this means no color

            } else {
                colors[j] = readVram(address);
                assert(colors[j] < 64);
            }
        }
}

void ppu::renderSprites()
{
    uint8_t  curr_sprite_data[8][4];

    int num_sprites = 0;
    int  sprite_height = sprite_size ? 16 : 8; //I assume this will not change in the middle of rendering
    int sprite_offset = 0;
    while ( num_sprites < 8 ) 
    {

        uint8_t y_coord = sprite_ram[ 4 * sprite_offset] + 1; //sprite data is delayed by one scanline
        if ( y_coord <= (scanline - sprite_height) && y_coord >= scanline)
        {
            //y_coord is valid

            for ( int i = 0; i< 4 ; i++)
            {
                curr_sprite_data[num_sprites][i] = sprite_ram[(4 * sprite_offset) + i];
            }


            sprite_offset++;
            num_sprites++;
            if (sprite_offset == 64)
            {
                break;
            }

        }
        else 
        {
            //y_coord is not valid
            sprite_offset++;
            if (sprite_offset == 64)
            {
                break;
            }
        }
    }
    // exactly 8 sprites found
    if( num_sprites == 7) setSpriteOverflow(true);


    //copied all applicable sprite data to curr_sprite_data
    //now get the proper colour and add it to the framebuffer with the bg pixels
    
    uint8_t scanline_buf[256][2];// for each pixel on line:  0 is data 1 is priority
    unsigned int pattern_tbl_addy;
    for (int i =0; i < num_sprites; i++)
    {
        //RENDER SPRITESS!!

        uint8_t y_pos = curr_sprite_data[i][0]; //top of sprite
        uint8_t x_pos = curr_sprite_data[i][3]; //left of sprite
        uint8_t sprite_tile_num = curr_sprite_data[i][1];
        unsigned int palette = (curr_sprite_data[i][2] & (0x3));
        bool behind_background = curr_sprite_data[i][2] & (0x20);
        bool hflip = curr_sprite_data[i][2] & (0x40);
        bool vflip = curr_sprite_data[i][2] & (0x80);

        //scanline is our current y coordinate
        int y_coord; // between 0 and sprite_height-1  y coord within sprite
        y_coord = scanline - y_pos;
        assert(y_coord >= 0 && y_coord <=sprite_height-1 );

        //
        // Calculate PAttern Table Addresss
        //
        /////////////////////////////////////////////////////////////////////////////////////////////////////
        if ( sprite_height == 8)
        {

            pattern_tbl_addy = (sprite_pattern | ((uint16_t) sprite_tile_num << 4))  | y_coord;    
                
        
        } else //height is 16
        {
            pattern_tbl_addy = ( sprite_tile_num & 0x01) << 7; 
            pattern_tbl_addy |= (sprite_tile_num & 0xFE); 

            if ( y_coord >= 8) { //bottom half
                pattern_tbl_addy += 0x10;
            }

        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////////

        uint8_t colors[8];
        getColors(colors, pattern_tbl_addy, palette, true); 
        for ( int j = 0; j < 8; j++)
        {
            if ( scanline_buf[x_pos + j][0] == 0 ) //use first pixel found only
            {
                scanline_buf[x_pos + j][0] = colors[j];
                scanline_buf[x_pos + j][1] = behind_background;
            }
        }
    } 

    //Done adding sprite colors to  scanline_buf .... now to combine it with the background in nes_framebuffer....

    int base = scanline * 256;
    assert(base <= 256 * 240);
    //color 64 means blank!
    for ( int i =0 ; i < 256; i++ )
    {
        if ( scanline_buf[i][0] != 64 && !scanline_buf[i][1] ) 
        {
            //replace color
            nes_framebuffer[base + i] = scanline_buf[i][0];
        } else if ( scanline_buf[i][0] != 64 )
        {
            nes_framebuffer[base+i] = scanline_buf[i][0];
        }
    }

    

    
}


void updateSprites()
{
}
void updateTiles()
{
}

void ppu::updateEndScanLine()
{
    if ( show_background || show_sprites )
    {
        if ( ADDR & 0x7000 == 0x7000 )
        {
                ADDR ^= 0x7000;
                if ( ADDR & 0x3E0 == 0x3A0 )
                {
                    ADDR ^= 0xBA0;

                } else if (ADDR & 0x3E0 == 0x3E0 )
                {
                    ADDR ^= 0x3E0;
                } else
                {
                    ADDR += 20;
                }


        } else {
            ADDR +=0x1000;
        }
        ADDR = (ADDR & 0x7BE0) | (vram_latch & 0x41f);
    }

    else {
        ADDR += vram_address_inc;
    }
}

void ppu::step()
{ 
   //262 scanlines per frame
  // 341 ppu clock cycles per scanline ( 1 CPU cycle = 3 PPu cycles)
    if ( scanline == -1) 
    {
        if ( cycle == 1)
        {
            setSprite0Hit(false);
            setSpriteOverflow(false);
        }
        else if (cycle == 304)
        {
            if( show_background || show_sprites)
            {
                ADDR = vram_latch;
            }
        }
    } 
    else if (scanline >=0 && scanline <= 239 )
    {
        if ( cycle ==255)
        {
            
                if (show_background ) 
                {
                    renderBG();

    //                printf("scanline = %d\n", scanline);
                }

                if (show_sprites) 
                {
            //        setSprite0Hit(true);
             //       renderSprites();
                }
        }
        else if (cycle == 256)
        {
            if ( show_background)
            {
            //    updateEndScanLine();
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
            PPU->convertFramebuffer();
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
        if(cycle == 1)
        {
            setVblank(false);
        }

        else if( cycle == 340 ) 
        {
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

void ppu::convertFramebuffer()
{
    color_palette[64] = color_palette[readVram(universal_bg_color)];
//    printf("unicolor = %X\n", color_palette[64]);
    //convert from nes color space to rgb
    for ( unsigned int i = 0 ; i < resolution ;i++ )
    {
//        pthread_mutex_lock(&framebuffer_mutex);
        uint32_t color = color_palette[nes_framebuffer[i]];


  //      pthread_mutex_unlock(&framebuffer_mutex);
        rgb_framebuffer[(3*i)] = ((color & 0xFF0000) >> 16) & 0xFF;
        rgb_framebuffer[(3*i)+1] = ((color & 0xFF00) >> 8) & 0xFF;
        rgb_framebuffer[(3*i)+2] = ((color & 0xFF));
    }
}










