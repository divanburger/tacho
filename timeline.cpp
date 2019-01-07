//
// Created by divan on 05/01/19.
//

#include <cassert>
#include <cstdio>
#include <cstring>
#include "timeline.h"
#include "util.h"

void tm_init(Timeline *timeline) {
    timeline->first = nullptr;
    timeline->last = nullptr;

    timeline->start_time = 0;
    timeline->end_time = 0;
}

TimelineEntry *tm_add(Timeline *timeline, int depth, const char *name, const char *path, int line_no) {
    TimelineChunk *chunk = timeline->last;

    if (!chunk || chunk->entry_count == chunk->entry_capacity) {
        TimelineChunk *new_chunk = raw_alloc_type_zero(TimelineChunk);

        new_chunk->entry_capacity = 1024;
        new_chunk->entries = raw_alloc_array(TimelineEntry, new_chunk->entry_capacity);

        new_chunk->prev = timeline->last;
        new_chunk->next = nullptr;
        timeline->last = new_chunk;

        if (chunk) {
            new_chunk->prev->next = new_chunk;
        } else {
            timeline->first = new_chunk;
        }

        chunk = new_chunk;
    }

    assert(chunk);
    assert(chunk->entry_count < chunk->entry_capacity);
    TimelineEntry *entry = chunk->entries + (chunk->entry_count++);

    entry->events = 0;
    entry->depth = depth;
    entry->name = str_copy(&timeline->arena, name);
    entry->path = str_copy(&timeline->arena, path);
    entry->line_no = line_no;
    return entry;
}

bool tm_read_file(Timeline *timeline, const char *filename) {
    tm_init(timeline);

    FILE *input = fopen(filename, "rb");
    if (!input) return false;

    char name_buffer[4096];
    char filename_buffer[4096];
    char method_buffer[4096];

    int stack_index = 0;
    TimelineEntry *stack[1024];

    while (!feof(input)) {
        uint64_t header;
        fread(&header, sizeof(uint64_t), 1, input);

        char type = (char) (header & 0xFF);
        int64_t time = (header >> 8);

        if (type == 'C') {
            CallBody call_body;
            fread(&call_body, CALL_BODY_BYTES, 1, input);

            fread(&method_buffer, call_body.method_name_length, 1, input);
            method_buffer[call_body.method_name_length] = 0;

            fread(&filename_buffer, call_body.filename_length, 1, input);
            filename_buffer[call_body.filename_length] = 0;

            memcpy(name_buffer + call_body.filename_offset, filename_buffer, call_body.filename_length);
            name_buffer[call_body.filename_offset + call_body.filename_length] = 0;

            auto entry = tm_add(timeline, stack_index, method_buffer, name_buffer, call_body.line_no);
            entry->start_time = time;

            for (int i = 0; i < stack_index; i++) {
                stack[i]->events++;
            }

            stack[stack_index] = entry;
            stack_index++;
        } else if (type == 'R' && stack_index > 0) {
            stack_index--;

            auto entry = stack[stack_index];
            entry->end_time = time;
        } else if (type == 'S') {
            uint32_t name_length;
            fread(&name_length, sizeof(uint32_t), 1, input);

            fread(&name_buffer, name_length, 1, input);
            timeline->name = str_copy(&timeline->arena, name_buffer, name_length);
        } else if (type == 'F') {
            timeline->end_time = time;
            break;
        }
    }

    for (; stack_index >= 0; stack_index--) {
        stack[stack_index]->end_time = timeline->end_time;
    }

    fclose(input);
    return true;
}