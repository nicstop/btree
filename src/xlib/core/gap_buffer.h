/*
 * $Creator: Nikita Smith
 * $Date: Dec 18 2019
 */

#ifndef XLIB_CORE_GAP_BUFFER_INCLUDE_H
#define XLIB_CORE_GAP_BUFFER_INCLUDE_H

typedef struct GapBuffer {
    Char8 *text_buffer;
    U32 text_buffer_size;

    U32 gap_size;
    U32 gap_offset;

    U32 cursor_index;
} GapBuffer;

x_api GapBuffer
x_gap_buffer_init(Char8 *buffer, U32 buffer_size);

x_api U32
x_gap_buffer_paste_char(GapBuffer *gap_buffer, Char32 uni_char);

x_api U32
x_gap_buffer_paste_string(GapBuffer *gap_buffer, const Char8 *text, U32 text_size);

x_api U32
x_gap_buffer_copy_string(const GapBuffer *gap_buffer, U32 copy_count, void *buffer_out, U32 buffer_size);

x_api U32
x_gap_buffer_cut_string(GapBuffer *gap_buffer, U32 cut_count, void *buffer_out, U32 buffere_size);

x_api void
x_gap_buffer_get_text_before_cursor(GapBuffer *gap_buffer, Char8 **text_out, U32 *text_size_out);

x_api void
x_gap_buffer_get_text_after_cursor(GapBuffer *gap_buffer, Char8 **text_out, U32 *text_size_out);

x_api U32
x_gap_buffer_move_cursor(GapBuffer *gap_buffer, S32 cursor_delta);

x_api U32
x_gap_buffer_delete(GapBuffer *gap_buffer);

x_api U32
x_gap_buffer_backspace(GapBuffer *gap_buffer);

x_api void
x_gap_buffer_from_cursor(GapBuffer *gap_buffer, S32 count, Char8 **text_out, U32 *text_size_out);

x_api Char32
x_gap_buffer_codepoint_under_cursor(const GapBuffer *gap_buffer);

x_api U32
x_gap_buffer_get_cursor_index(const GapBuffer *gap_buffer);

#endif

#ifdef XLIB_CORE_GAP_BUFFER_IMPLEMENTATION

x_api GapBuffer
x_gap_buffer_init(Char8 *text_buffer, U32 text_buffer_size)
{
    GapBuffer gap;
    gap.text_buffer = text_buffer;
    gap.text_buffer_size = text_buffer_size;
    gap.gap_size = text_buffer_size;
    gap.gap_offset = 0;
    gap.cursor_index = 0;
    return gap;
}

x_api U32
x_gap_buffer_paste_char(GapBuffer *gap_buffer, Char32 uni_char)
{
    Char8 utf8_buffer[8];
    U32 utf8_buffer_size;

    /* TODO(nick): UTF8 */
    x_assert(uni_char < 256);
    utf8_buffer[0] = (Char8)uni_char;
    utf8_buffer_size = sizeof(Char8);

    return x_gap_buffer_paste_string(gap_buffer, &utf8_buffer[0], utf8_buffer_size);
}

x_api U32
x_gap_buffer_paste_string(GapBuffer *gap_buffer, const Char8 *text, U32 text_size)
{
    if (text_size > gap_buffer->gap_size) {
        text_size = gap_buffer->gap_size;
    }

    x_memcpy(gap_buffer->text_buffer + gap_buffer->gap_offset, text, text_size);

    gap_buffer->gap_offset += text_size;
    gap_buffer->gap_size -= text_size;

    return text_size;
}

x_api U32
x_gap_buffer_copy_string(const GapBuffer *gap_buffer, U32 copy_count, void *buffer_out, U32 buffer_size)
{
    return 0;
}

x_api U32
x_gap_buffer_cut_string(GapBuffer *gap_buffer, U32 cut_count, void *buffer_out, U32 buffere_size)
{
    return 0;
}

x_api void
x_gap_buffer_get_text_before_cursor(GapBuffer *gap_buffer, Char8 **text_out, U32 *text_size_out)
{
    if (gap_buffer->gap_offset > 0) {
        *text_out = gap_buffer->text_buffer + 0;
        *text_size_out = gap_buffer->gap_offset;
    } else {
        *text_out = "";
        *text_size_out = 0;
    }
}

x_api void
x_gap_buffer_get_text_after_cursor(GapBuffer *gap_buffer, Char8 **text_out, U32 *text_size_out)
{
    if (gap_buffer->gap_offset < gap_buffer->text_buffer_size) {
        *text_out = gap_buffer->text_buffer + (gap_buffer->gap_offset + gap_buffer->gap_size);
        *text_size_out = gap_buffer->text_buffer_size - (gap_buffer->gap_offset + gap_buffer->gap_size);
    } else {
        *text_out = "";
        *text_size_out = 0;
    }
}

x_api U32
x_gap_buffer_move_cursor(GapBuffer *gap_buffer, S32 cursor_delta)
{
    S32 i;
    U32 cursor;

    U32 cp_src_offset = 0;
    U32 cp_dst_offset = 0;
    U32 cp_size = 0;

    if (cursor_delta > 0) {
        U32 cursor_cap;

        cursor = gap_buffer->gap_offset + gap_buffer->gap_size;
        cursor_cap = cursor + gap_buffer->text_buffer_size - (gap_buffer->gap_offset + gap_buffer->gap_size);

        x_assert(cursor <= gap_buffer->text_buffer_size);
        x_assert(cursor <= gap_buffer->text_buffer_size);

        for (i = 0; i < cursor_delta; ++i) {
            U32 char_size;

            if (cursor >= cursor_cap) {
                break;
            }

            /* TODO(nick): UTF8 */
            char_size = sizeof(Char8);
            cursor += char_size;
        }
        gap_buffer->cursor_index += (U32)i;

        cp_size = cursor - (gap_buffer->gap_offset + gap_buffer->gap_size);
        cp_src_offset = gap_buffer->gap_offset + gap_buffer->gap_size;
        cp_dst_offset = gap_buffer->gap_offset;

        x_assert(gap_buffer->gap_offset + cp_size <= gap_buffer->text_buffer_size);
        gap_buffer->gap_offset += cp_size;
    } else {
        cursor = gap_buffer->gap_offset;

        cursor_delta = -cursor_delta;
        for (i = 0; i < cursor_delta; ++i) {
            U32 char_size;

            if (cursor == 0) {
                break;
            }
            /* TODO(nick): UTF8 */
            char_size = sizeof(Char8);
            cursor -= char_size;
        }
        gap_buffer->cursor_index -= (U32)i;

        cp_size = gap_buffer->gap_offset - cursor;
        cp_src_offset = cursor;
        cp_dst_offset = gap_buffer->gap_offset + gap_buffer->gap_size - cp_size;

        x_assert(gap_buffer->gap_offset >= cp_size);
        gap_buffer->gap_offset -= cp_size;
    }

    x_assert(cp_size <= gap_buffer->gap_size);
    x_memcpy(gap_buffer->text_buffer + cp_dst_offset, gap_buffer->text_buffer + cp_src_offset, cp_size);

    return cp_size;
}

x_api U32
x_gap_buffer_delete(GapBuffer *gap_buffer) 
{
    U32 cp_size = x_gap_buffer_move_cursor(gap_buffer, +1);
    if (cp_size > 0) {
        cp_size = x_gap_buffer_backspace(gap_buffer);
    }
    return cp_size;
}

x_api U32
x_gap_buffer_backspace(GapBuffer *gap_buffer)
{
    U32 cp_size = x_gap_buffer_move_cursor(gap_buffer, -1);
    if (cp_size > 0) {
        x_assert(gap_buffer->gap_offset + gap_buffer->gap_size + cp_size <= gap_buffer->text_buffer_size);
        gap_buffer->gap_size += cp_size;
    }
    return cp_size;
}

x_api void
x_gap_buffer_from_cursor(GapBuffer *gap_buffer, S32 count, Char8 **text_out, U32 *text_size_out)
{
    GapBuffer temp = *gap_buffer;
    *text_size_out = x_gap_buffer_move_cursor(gap_buffer, count);

    if (count > 0) {
        *text_out = gap_buffer->text_buffer + temp.gap_offset + temp.gap_size;
    } else if (count < 0) {
        *text_out = gap_buffer->text_buffer + gap_buffer->gap_offset;
    }

    *gap_buffer = temp;
}

x_api Char32
x_gap_buffer_codepoint_under_cursor(const GapBuffer *gap_buffer)
{
    Char32 codepoint = '\0';
    if (gap_buffer->gap_offset + gap_buffer->gap_size < gap_buffer->text_buffer_size ) {
        codepoint = (Char32)gap_buffer->text_buffer[gap_buffer->gap_offset + gap_buffer->gap_size]; /* TODO(nick): UTF8 */
    }
    return codepoint;
}

x_api U32
x_gap_buffer_get_cursor_index(const GapBuffer *gap_buffer)
{
    return gap_buffer->cursor_index;
}

#endif
