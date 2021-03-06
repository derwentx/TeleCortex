#include "panel.h"
#include "gcode.h"
#include "eeprom.h"


// Must be declared for allocation and to satisfy the linker
// Zero values need no initialization.

char *GCodeParser::command_ptr,
    *GCodeParser::value_ptr;
char GCodeParser::command_letter;
int GCodeParser::codenum;
int GCodeParser::arg_str_len;
char *GCodeParser::command_args; // start of parameters
long GCodeParser::linenum;

// Create a global instance of the GCode parser singleton
GCodeParser parser;

/**
* Clear all code-seen (and value pointers)
*
* Since each param is set/cleared on seen codes,
* this may be optimized by commenting out ZERO(param)
*/
void GCodeParser::reset()
{
    arg_str_len = 0;      // No whole line argument
    command_letter = '?'; // No command letter
    codenum = 0;          // No command code
    linenum = -1;
}

void GCodeParser::parse(char *p)
{
    reset(); // No codes to report

    // Skip spaces
    while (IS_SPACE(*p))
        ++p;

    // Skip N[-0-9] if included in the command line
    if (*p == LINENUM_PREFIX && NUMERIC_SIGNED(p[1]))
    {
        p += 1;
        linenum = strtol(p, NULL, 10);
        while (NUMERIC(*p))
            ++p; // skip [0-9]*
        while (IS_SPACE(*p))
            ++p; // skip [ ]*
    }

    // *p now points to the current command
    command_ptr = p;

    // Get the command letter
    const char letter = *p++;

    // Nullify asterisk and trailing whitespace
    char *starpos = strchr(p, CHECKSUM_PREFIX);
    if (starpos)
    {
        --starpos; // *
        while (IS_SPACE(*starpos))
            --starpos; // spaces...
        starpos[1] = '\0';
    }

    // Bail if the letter is not a valid command prefix
    if(!IS_CMD_PREFIX(letter)){
        return;
    }

    // Skip spaces to get the numeric part
    while (IS_SPACE(*p))
        p++;

    // Bail if there's no command code number
    if (!NUMERIC(*p))
        return;

    // Save the command letter at this point
    // A '?' signifies an unknown command
    command_letter = letter;

    // Get the code number - integer digits only
    codenum = 0;
    do
    {
        codenum *= 10, codenum += *p++ - '0';
    } while (NUMERIC(*p));

    // Skip all spaces to get to the first argument, or nul
    while (IS_SPACE(*p))
        p++;

    command_args = p; // Scan for parameters in seen()

    while (const char code = *(p++))
    {
        while (IS_SPACE(*p))
            p++; // Skip spaces between parameters & values


        #if DEBUG_GCODE
            const bool has_arg = HAS_ARG(p);
            SER_SNPRINTF_COMMENT_PSTR(
                "PAR: Got letter %c at index %d ,has_arg: %d",
                code,
                (int)(p - command_ptr - 1),
                has_arg);
        #endif

        while(HAS_ARG(p))
            p++;

        //skip all the space until the next argument or null
        while (IS_SPACE(*p))
            p++;
    }
}
int GCodeParser::unknown_command_error()
{
    SNPRINTF_MSG_PSTR("Unknown Command: %s (%c %d)", command_ptr, command_letter, codenum);
    return 11;
}

#if DEBUG_GCODE
void GCodeParser::debug()
{
    const char * debug_prefix = "PAD";

    SER_SNPRINTF_COMMENT_PSTR("%s: Command: %s (%c %d)", debug_prefix, command_ptr, command_letter, codenum);
    if(linenum >= 0){
        SER_SNPRINTF_COMMENT_PSTR("%s: LineNum: %d", debug_prefix, linenum);
    }
    SER_SNPRINTF_COMMENT_PSTR("%s: Args: %s", debug_prefix, command_args);
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        if (seen(c))
        {
            SER_SNPRINTF_COMMENT_PSTR("%s: Code '%c'", debug_prefix, c);
            if (has_value())
            {
                SER_SNPRINT_COMMENT_PSTR( "PAD: (has value)");
                if (arg_str_len) {
                    STRNCPY_PSTR(
                        fmt_buffer, "%c%s: ->    str (%d) : '%%n%%%ds'", BUFFLEN_FMT);
                    snprintf(
                        msg_buffer, BUFFLEN_FMT, fmt_buffer,
                        COMMENT_PREFIX, debug_prefix, arg_str_len, arg_str_len);
                    strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
                    int msg_offset = 0;
                    snprintf(
                        msg_buffer, BUFFLEN_MSG, fmt_buffer, &msg_offset, "");
                    strncpy(
                        msg_buffer + msg_offset, value_ptr,
                        MIN(BUFFLEN_MSG - msg_offset, arg_str_len));
                    SERIAL_OBJ.println(msg_buffer);

                    // snprintf(
                    //     fmt_buffer, BUFFLEN_FMT, "%%s: ->    str (%d) : '%%%ds'",
                    //     COMMENT_PREFIX, debug_prefix, arg_str_len, arg_str_len);
                    // snprintf(
                    //     msg_buffer, BUFFLEN_MSG, fmt_buffer, value_ptr
                    // );
                    // SERIAL_OBJ.println(msg_buffer);
                }
                // if(HAS_NUM(value_ptr)){
                //     SER_SNPRINTF_COMMENT_PSTR("%s: ->  float: %f", debug_prefix, value_float());
                //     SER_SNPRINTF_COMMENT_PSTR("%s: ->   long: %d", debug_prefix, value_long());
                //     SER_SNPRINTF_COMMENT_PSTR("%s: ->  ulong: %d", debug_prefix, value_ulong());
                //     // SER_SNPRINTF_COMMENT_PSTR("%s: -> millis: %d", debug_prefix, value_millis());
                //     // SER_SNPRINTF_COMMENT_PSTR("%s: -> sec-ms: %d", debug_prefix, value_millis_from_seconds());
                //     SER_SNPRINTF_COMMENT_PSTR("%s: ->    int: %d", debug_prefix, value_int());
                //     SER_SNPRINTF_COMMENT_PSTR("%s: -> ushort: %d", debug_prefix, value_ushort());
                //     SER_SNPRINTF_COMMENT_PSTR("%s: ->   byte: %d", debug_prefix, (int)value_byte());
                //     SER_SNPRINTF_COMMENT_PSTR("%s: ->   bool: %d", debug_prefix, (int)value_bool());
                // }
            }
            else
            {
                SER_SNPRINT_COMMENT_PSTR("PAD: (no value)");
            }
        }
    }
}
#endif

bool validate_int_parameter_bounds(char parameter, int value, const int *min_value = NULL, const int *max_value = NULL){
    if((min_value != NULL) && (value < *min_value)){
        SNPRINTF_MSG_PSTR(
            "%c Parameter less than minimum %d: %d",
            parameter, *min_value, value
        );
        return false;
    }

    if((max_value != NULL) && (value > *max_value)){
        SNPRINTF_MSG_PSTR(
            "%c Parameter greater than maximum %d: %d",
            parameter, *max_value, value
        );
        return false;
    }
    return true;
}

// bool validate_base64_parameter(char parameter, char * value, int *min_length = NULL)

/**
 * GCode functions
 */

/**
* GCode M508
* Write Code to EEPROM
*/
int gcode_M508() {
    const char * debug_prefix = "GCO_M508";
    int code_offset = 0;
    char *code_payload = NULL;
    int code_payload_len = 0;
    static const int eeprom_code_len = EEPROM_CODE_END - EEPROM_CODE_START;

    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("%s: Calling M%d", debug_prefix, parser.codenum);
    #endif

    if (parser.seen('S')) {
        code_offset = parser.value_int();
        #if DEBUG_GCODE
            SER_SNPRINTF_COMMENT_PSTR("%s: -> code_offset: %d", debug_prefix, code_offset);
        #endif

        int min_code_offset = 0;
        if(!validate_int_parameter_bounds('S', code_offset, &min_code_offset, &eeprom_code_len)){
            return 13;
        }
    }

    if (parser.seen('V')) {
        code_payload = parser.value_ptr;
        code_payload_len = parser.arg_str_len;
        #if DEBUG_GCODE
            STRNCPY_PSTR(
                fmt_buffer, "%c%s: -> payload: (%d) '%%n%%%ds'", BUFFLEN_FMT
            );
            snprintf(
                msg_buffer, BUFFLEN_FMT, fmt_buffer,
                COMMENT_PREFIX, debug_prefix, code_payload_len, code_payload_len
            );
            strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
            int msg_offset = 0;
            snprintf(
                msg_buffer, BUFFLEN_MSG, fmt_buffer, &msg_offset, "");
            strncpy(
                msg_buffer + msg_offset, code_payload,
                MIN(BUFFLEN_MSG - msg_offset, code_payload_len));
            SERIAL_OBJ.println(msg_buffer);
        #endif

        // validate code_payload is base64
        // TODO: This check might not be necessary if handled in parser
        // Test with M2600 V/-//
        char *p = code_payload;
        int i = 0;
        while((i < code_payload_len) && (p[i] != '\0')){
            if(!IS_BASE64(p[i])){
                SNPRINTF_MSG_PSTR(
                    "panel payload is not encoded in base64. code_payload: %s offending char: %c, offending index: %d",
                    code_payload, p, i
                );
                return 14;
            }
            i++;
        }

        //TODO: check payload is '\0' terminated?
    }

    // Validate code_payload not empty
    // Test with M2600 V
    if(code_payload_len <= 0){
        SNPRINTF_MSG_PSTR(
            "panel payload must not be empty",
            NULL
        );
        return 14;
    }

    char eep_buffer[code_payload_len * 3 / 4];
    int dec_len = base64_decode(eep_buffer, code_payload, code_payload_len);
    #if DEBUG_GCODE
        STRNCPY_PSTR(
            fmt_buffer, "%c%s: -> decoded payload: (%d) 0x", BUFFLEN_FMT
        );
        snprintf(
            msg_buffer, BUFFLEN_FMT, fmt_buffer,
            COMMENT_PREFIX, debug_prefix, dec_len, dec_len * 2
        );
        strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
        int offset_payload_start = snprintf(
            msg_buffer, BUFFLEN_MSG, fmt_buffer
        );
        for(int i=0; i<dec_len; i++){
            sprintf(
                msg_buffer + offset_payload_start + i*2,
                "%02x",
                eep_buffer[i]
            );
        }
        SERIAL_OBJ.println(msg_buffer);
    #endif

    write_eeprom_code(eep_buffer, code_offset);
    // for(int eep_count = 0; eep_count < dec_len; eep_count++){
    //     int eep_addr = EEPROM_CODE_START + code_offset + eep_count;
    //     EEPROM.write(eep_addr, eep_buffer[eep_count]);
    // }

    return 0;
}

/**
 * GCode M509
 * Hexdump EEPROM
 */
int gcode_M509() {
    dump_eeprom_code();
    return 0;
}

inline bool panel_payload_gcode(){
    return (parser.codenum == 2600 || parser.codenum == 2601);
}

inline bool panel_single_gcode(){
    return (parser.codenum == 2602 || parser.codenum == 2603);
}

int gcode_M260X()
{
    const char * debug_prefix = "GCO_M260X";
    int panel_number = 0;
    int pixel_offset = 0;
    char *panel_payload = NULL;
    int panel_payload_len = 0;
    int panel_len = 0;

    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("%s: Calling M%d", debug_prefix, parser.codenum);
    #endif

    if (parser.seen('Q'))
    {
        panel_number = parser.value_int();
        #if DEBUG_GCODE
            SER_SNPRINTF_COMMENT_PSTR("%s: -> panel_number: %d", debug_prefix, panel_number);
        #endif
        // validate panel_number

        int min_panel_number = 0;
        int max_panel_number = MAX_PANELS;

        if(!validate_int_parameter_bounds('Q', panel_number, &min_panel_number, &max_panel_number)){
            return 12;
        }
    }

    panel_len = panel_info[panel_number];

    if (parser.seen('S'))
    {
        pixel_offset = parser.value_int();
        #if DEBUG_GCODE
            SER_SNPRINTF_COMMENT_PSTR("%s: -> pixel_offset: %d", debug_prefix, pixel_offset);
        #endif

        int min_pixel_offset = 0;
        int max_pixel_offset = panel_len - 1;

        if(!validate_int_parameter_bounds('S', pixel_offset, &min_pixel_offset, &max_pixel_offset)){
            return 13;
        }
    }
    if (parser.seen('V'))
    {
        panel_payload = parser.value_ptr;
        panel_payload_len = parser.arg_str_len;
        #if DEBUG_GCODE
            STRNCPY_PSTR(
                fmt_buffer, "%c%s: -> payload: (%d) '%%n%%%ds'", BUFFLEN_FMT);
            snprintf(
                msg_buffer, BUFFLEN_FMT, fmt_buffer,
                COMMENT_PREFIX, debug_prefix, panel_payload_len, panel_payload_len);
            strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
            int msg_offset = 0;
            snprintf(
                msg_buffer, BUFFLEN_MSG, fmt_buffer, &msg_offset, "");
            strncpy(
                msg_buffer + msg_offset, panel_payload,
                MIN(BUFFLEN_MSG - msg_offset, panel_payload_len));
            SERIAL_OBJ.println(msg_buffer);
            SERIAL_OBJ.flush();
        #endif

        // validate panel_payload is base64
        // TODO: This check might not be necessary if handled in parser
        // Test with M2600 V/-//
        char *p = panel_payload;
        int i = 0;
        while((i < panel_payload_len) && (p[i] != '\0')){
            if(!IS_BASE64(p[i])){
                SNPRINTF_MSG_PSTR(
                    "panel payload is not encoded in base64. panel_payload: %s offending char: %c, offending index: %d",
                    panel_payload, p, i
                );
                return 14;
            }
            i++;
        }

        // validate panel_payload_len is multiple of 4 (4 bytes encoded per 3 pixels (RGB))
        // Test with M2600 V/////
        if((panel_payload_len % 4) != 0){
            SNPRINTF_MSG_PSTR(
                "base64 panel payload should be a multiple of 4 bytes. panel_payload (%d): '%s'",
                panel_payload_len, panel_payload
            );
            return 14;
        }

        if( panel_payload_gcode() ) {
            // Test with M2600 S332 V////////
            if((panel_payload_len / 4) > (panel_len - pixel_offset)){
                SNPRINTF_MSG_PSTR(
                    "base64 panel payload too long for panel. panel_payload_len (encoded bytes): %d pixel_offset: %d, panel_len: %d",
                    panel_payload_len, pixel_offset, panel_len
                );
                return 14;
            }
        } else if (panel_single_gcode()) {
            // Test with M2600 S332 V/////
            if(panel_payload_len != 4){
                SNPRINTF_MSG_PSTR(
                    "encoded panel payload should be 4 chars not: %d",
                    panel_payload_len
                );
                return 14;
            }
        }
    }

    // Validate panel_payload not empty
    // Test with M2600 V
    if(panel_payload_len <= 0){
        SNPRINTF_MSG_PSTR(
            "panel payload must not be empty", panel_payload_len
        );
        return 14;
    }

    char pixel_data[3];
    int dec_len = (panel_payload_len * 3 / 4);

    #if DEBUG_GCODE
        if( panel_payload_gcode() ) {
            SER_SNPRINTF_COMMENT_PSTR("%s: -> decoded payload: (%d) 0x", debug_prefix, dec_len);
            for (int pixel = 0; pixel < (panel_payload_len / 4); pixel++)
            {
                // every 4 bytes of encoded base64 corresponds to a single RGB pixel
                base64_decode(pixel_data, panel_payload + (pixel * 4), 4);
                snprintf(msg_buffer, BUFFLEN_MSG, "%02X%02X%02X", pixel_data[0], pixel_data[1], pixel_data[2]);
                SERIAL_OBJ.print(msg_buffer);
            }
            SERIAL_OBJ.println();
        }
    #endif

    if( panel_payload_gcode() ) {
        for (int pixel = 0; pixel < (panel_payload_len / 4); pixel++)
        {
            // every 4 bytes of encoded base64 corresponds to a single RGB pixel
            base64_decode(pixel_data, panel_payload + (pixel * 4), 4);
            if (parser.codenum == 2600)
            {
                set_panel_pixel_RGB(panel_number, pixel_offset + pixel, pixel_data);
            }
            else if (parser.codenum == 2601)
            {
                set_panel_pixel_HSV(panel_number, pixel_offset + pixel, pixel_data);
            }
        }
    } else if (panel_single_gcode()) {
        base64_decode(pixel_data, panel_payload, 4);
        if (parser.codenum == 2602)
        {
            set_panel_RGB(panel_number, pixel_data, pixel_offset);
        }
        else if (parser.codenum == 2603)
        {
            set_panel_HSV(panel_number, pixel_data, pixel_offset);
        }
    }

    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("%s: done", debug_prefix);
    #endif

    return 0;
}

int gcode_M2610() {
    const char * debug_prefix = "GCO";
    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("%s: Calling M2610", debug_prefix);
    #endif
    FastLED.show();
    return 0;
}

int gcode_M2611() {
    // TODO: Is this even possible?
    return 0;
}
