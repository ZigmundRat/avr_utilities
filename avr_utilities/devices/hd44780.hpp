/*
 * hd44780.hpp
 *
 *  Created on: Nov 11, 2011
 *      Author: danny
 */

#ifndef HD44780_HPP_
#define HD44780_HPP_
#include "avr_utilities/pin_definitions.hpp"
#include <util/delay.h> // for _delay_ms


namespace hd44780
{
typedef uint8_t byte;

namespace commands
{

inline byte if_( bool condition, byte value)
{
    return condition?value:0;
}

inline byte clr() { return 0x01;}
inline byte home() { return 0x02;}

/**
 * Select the entry mode.  inc determines whether the address counter
 * auto-increments, shift selects an automatic display shift.
 */
inline byte entry_mode( bool increment, bool shift)
{
    return 0x04 | if_(increment, 0x02) | if_(shift, 0x01);
}

/**
 * Selects display on/off, cursor on/off, cursor blinking
 * on/off.
 */
inline byte display_control( bool display, bool cursor, bool blink)
{
    return 0x08 | if_(display, 0x04) | if_( cursor, 0x02) | if_( blink, 0x01);
}

/**
 * With shift = 1, shift display right or left.
 * With shift = 0, move cursor right or left.
 */
inline byte shift( bool shift, bool right)
{
    return 0x10 | if_( shift, 0x08) | if_( right, 0x04);
}

/**
 * Function set.  if8bit selects an 8-bit data path, two_lines arranges
 * for a two-line display, font_5x10 selects the 5x10 dot font (5x8
 * dots if clear).
 */
inline byte function_set( bool if8bit, bool two_lines, bool font_5x10)
{
    return 0x20 | if_(if8bit, 0x10) | if_(two_lines, 0x08) | if_( font_5x10, 0x04);
}

/**
 * Set the next character generator address to addr.
 */
inline byte cg_addr( byte addr)
{
    return 0x40 | (addr & 0x3f);
}

/**
 * Set the next display address to addr.
 */
inline byte dd_addr( byte addr)
{
    return 0x80 | (addr & 0x7f);
}

}

template< typename pins_type>
class lcd
{
public:

    lcd()
    {
        init();
    }

    static inline void init( )
    {
        using namespace commands;
        make_output( pins.e | pins.rw | pins.rs | pins.data);
        reset( pins.rs);

        // following the Hitachi datasheet
        // HD44780U (LCD-II)/ADE-207-272(Z)/'99.9/rev 0.0
        // page 46
        // for initialising lcd for 4-bit instructions

        // initialisation sequence
        _delay_ms( 15);
        outnibble( 0x03);
        _delay_ms( 4.1);
        outnibble( 0x03);
        _delay_ms( 0.1);
        outnibble( 0x03);

        // switch to 4 bit mode (this is still an 8-bit instruction,
        // therefore only send one nibble).
        outnibble( function_set( false, true, false)>>4);
        wait_ready();

        // now we can send 4 bit commands (one nibble at a time).
        command_out( function_set( false, true, false));
        command_out( display_control( false, false, false));


        command_out( clr());
        command_out( entry_mode( true, false));
        command_out( display_control( true, false, false));
        command_out( function_set( false, true, false));
    }

    static void cls()
    {
        command_out( commands::clr());
        command_out( commands::home());
        command_out( commands::dd_addr(0));
    }

    static const char* string_out( const char *string)
    {
        while (*string) data_out( *string++);
        return string;
    }

    static void data_out( byte data)__attribute__((always_inline))
    {
        outbyte( data, true);
    }

    static void command_out( byte command)
    {
        outbyte( command, false);
    }

    static byte data_in()
    {
        return inbyte( true);
    }

    static byte command_in()
    {
        return inbyte( false);
    }

private:


    static const byte BUSY_FLAG = 0x80;

    static pins_type pins;
    static void wait_ready()
    {
        while (command_in() & BUSY_FLAG);
    }

    static void outbyte( byte byte, bool set_rs)
    {
        if (set_rs)
        {
            set( pins.rs);
        }
        else
        {
            reset( pins.rs);
        }
        outnibble( byte >> 4);
        outnibble( byte & 0x0f);
        wait_ready();
    }

    static void outnibble( byte nibble)
    {
        reset( pins.rw);
        write( pins.data, nibble);
        set( pins.e);
        delay_500ns();
        reset( pins.e);
    }

    static byte inbyte( bool set_rs)
    {
        if (set_rs)
        {
            set( pins.rs);
        }
        else
        {
            reset( pins.rs);
        }
        byte result =innibble() << 4;
        return result | innibble();
    }

    static byte innibble()
    {
        byte result;
        set( pins.rw);
        make_input( pins.data);
        set( pins.e);
        delay_500ns();
        result = read( pins.data);
        reset( pins.e);
        make_output( pins.data);
        return result;
    }

    static void delay_500ns()
    {
#if F_CPU > 4000000UL
        _delay_us(0.5);
#else
        /*
         * When reading back, we need one additional NOP, as the value read
         * back from the input pin is sampled close to the beginning of a
         * CPU clock cycle, while the previous edge on the output pin is
         * generated towards the end of a CPU clock cycle.
         */
        __asm__ volatile("nop");
#  if F_CPU > 1000000UL
        __asm__ volatile("nop");
#    if F_CPU > 2000000UL
        __asm__ volatile("nop");
        __asm__ volatile("nop");
#    endif /* F_CPU > 2000000UL */
#  endif /* F_CPU > 1000000UL */
#endif

    }
};
}
#endif /* HD44780_HPP_ */
